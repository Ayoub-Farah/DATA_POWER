"""
Copyright (c) 2021-2024 LAAS-CNRS

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 2.1 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: LGLPV2.1
"""

# High-level communication script used to drive the OwnTech converter from a PC.
#
# The script connects to the shield, prepares the selected leg and launches
# three automated tests sequentially:
#   1. Réponse à l'échelon (``TEST_SENSI``)
#   2. Balayage fréquentiel (``TEST_CHIRP``)
#   3. Sinusoïde à fréquence fixe (``TEST_FIXED_FREQ``)
#
# After each excitation the scope dump streamed by the firmware is decoded,
# summarised on stdout and archived to
# ``scope_records/<horodatage>_<test>.csv`` so the user can inspect the
# captured waveforms offline.

from __future__ import annotations

import csv
import os
import sys
import time
from datetime import datetime
from pathlib import Path
from typing import List, Optional, Tuple

import find_devices

from Shield_Class import Shield_Device


SHIELD_VID = 0x2FE3
SHIELD_PID = 0x0101
DEFAULT_LEG = "LEG2"


def discover_port() -> str:
    """Return the serial port used by the shield, favouring an override."""

    override = os.environ.get("OWNTECH_PORT")
    if override:
        print(f"OWNTECH_PORT défini, utilisation du port {override}")
        return override

    ports = find_devices.find_shield_device_ports(SHIELD_VID, SHIELD_PID)
    print(f"Ports OwnTech détectés: {ports}")
    if not ports:
        raise SystemExit(
            "Impossible de localiser automatiquement une carte OwnTech. "
            "Branchez la carte ou fournissez le port via la variable "
            "d'environnement OWNTECH_PORT."
        )

    if len(ports) > 1:
        print(
            "Plusieurs ports correspondent au VID/PID OwnTech. Utilisation du premier."
        )

    return ports[0]


def send_and_log(shield: Shield_Device, action: str, *args, delay: float = 0.2) -> str:
    """Wrapper utilitaire pour afficher chaque commande envoyée."""

    message = shield.sendCommand(action, *args, delay=delay)
    print(f"→ {message}")
    return message


def ensure_leg_ready(shield: Shield_Device, leg: str) -> None:
    """Met la carte sous tension et active la jambe testée."""

    print(f"Préparation de {leg} pour les essais automatiques…")
    send_and_log(shield, "IDLE")
    rearm_leg(shield, leg)


def rearm_leg(shield: Shield_Device, leg: str) -> None:
    """Assure que la jambe de test est prête pour un nouvel essai."""

    send_and_log(shield, "POWER_ON")
    send_and_log(shield, "LEG", leg, "ON")
    send_and_log(shield, "CAPA", leg, "ON")
    send_and_log(shield, "DRIVER", leg, "ON", delay=1.0)


def analyze_record(test_name: str, record: dict) -> None:
    """Calcule quelques statistiques simples sur les données capturées."""

    columns: Tuple[str, ...] = record.get("columns", ())
    samples: List[dict] = record.get("samples", [])

    print(f"Analyse du scope pour {test_name} : {len(samples)} échantillons")

    if not columns or not samples:
        print("  Aucune donnée disponible dans l'enregistrement.")
        return

    for column in columns:
        values = [sample[column] for sample in samples if column in sample]
        if not values:
            continue

        minimum = min(values)
        maximum = max(values)
        average = sum(values) / len(values)
        print(
            f"  {column:<18} min={minimum:>8.4f}  max={maximum:>8.4f}  "
            f"moyenne={average:>8.4f}  départ={values[0]:>8.4f}  fin={values[-1]:>8.4f}"
        )


def persist_record(test_name: str, record: dict) -> None:
    """Sauvegarde l'enregistrement décodé au format CSV et brut."""

    samples: List[dict] = record.get("samples", [])
    columns: Tuple[str, ...] = record.get("columns", ())
    raw_payload: Tuple[str, ...] = record.get("raw", ())

    if not samples or not columns:
        print("  Rien à sauvegarder pour ce test.")
        return

    timestamp = datetime.now().strftime("%Y%m%d-%H%M%S-%f")
    safe_name = test_name.lower().replace(" ", "_")
    out_dir = Path("scope_records")
    out_dir.mkdir(exist_ok=True)

    csv_path = out_dir / f"{timestamp}_{safe_name}.csv"
    with csv_path.open("w", newline="") as csv_file:
        writer = csv.writer(csv_file)
        writer.writerow(columns)
        for sample in samples:
            writer.writerow([sample.get(column, "") for column in columns])

    raw_path = out_dir / f"{timestamp}_{safe_name}.txt"
    with raw_path.open("w", encoding="utf-8") as raw_file:
        raw_file.write("#" + ",".join(columns) + "\n")
        index_value = record.get("index")
        if index_value is not None:
            raw_file.write(f"#{index_value}\n")
        for line in raw_payload:
            raw_file.write(f"{line}\n")

    print(f"  Enregistrements sauvegardés dans {csv_path} et {raw_path}.")


_last_scope_signature: Optional[Tuple[Optional[int], Tuple[str, ...]]] = None


def capture_scope(shield: Shield_Device, timeout: float) -> dict:
    """Attend l'enregistrement du scope et retourne sa structure décodée."""

    global _last_scope_signature

    deadline = time.time() + timeout
    stale_record: Optional[dict] = None

    while True:
        remaining = deadline - time.time()
        if remaining <= 0:
            break

        try:
            record = shield.wait_for_scope_record(timeout=remaining)
        except TimeoutError as exc:
            print(f"  ⚠️  Timeout pendant la récupération du scope : {exc}")
            break
        except RuntimeError as exc:
            print(f"  ⚠️  Erreur lors du décodage du scope : {exc}")
            break

        signature = (
            record.get("index"),
            tuple(record.get("raw", ())),
        )

        if _last_scope_signature is not None and signature == _last_scope_signature:
            stale_record = record
            print(
                "  ⚠️  Enregistrement identique au précédent reçu, attente d'un nouveau dump…"
            )
            # Boucle pour attendre un nouveau record tant qu'il reste du temps.
            continue

        _last_scope_signature = signature
        return record

    if stale_record is not None:
        print(
            "  ⚠️  Aucun nouveau dump reçu avant expiration du délai, abandon de l'enregistrement périmé."
        )

    return {"columns": (), "samples": (), "raw": ()}


def run_sensitivity_test(shield: Shield_Device, leg: str, vref: float) -> None:
    print("\n=== Test de sensibilité (réponse à l'échelon) ===")
    rearm_leg(shield, leg)
    shield.getSerialObjID().reset_input_buffer()
    send_and_log(shield, "TEST_SENSI", leg, vref, delay=0.5)
    record = capture_scope(shield, timeout=30.0)
    analyze_record("TEST_SENSI", record)
    persist_record("TEST_SENSI", record)


def run_chirp_test(
    shield: Shield_Device,
    leg: str,
    start_freq: float,
    end_freq: float,
    duration: float,
    amplitude: float = 0.4,
    offset: float = 0.5,
    loop: int = 0,
) -> None:
    print("\n=== Test chirp (balayage fréquentiel) ===")
    rearm_leg(shield, leg)
    shield.getSerialObjID().reset_input_buffer()
    send_and_log(
        shield,
        "TEST_CHIRP",
        leg,
        start_freq,
        end_freq,
        duration,
        amplitude,
        offset,
        loop,
        delay=0.5,
    )
    record = capture_scope(shield, timeout=max(duration + 10.0, 20.0))
    analyze_record("TEST_CHIRP", record)
    persist_record("TEST_CHIRP", record)


def run_fixed_frequency_test(
    shield: Shield_Device,
    leg: str,
    frequency: float,
    amplitude: float = 0.4,
    offset: float = 0.5,
    run_duration: float = 2.0,
) -> None:
    print("\n=== Test sinusoïdal à fréquence fixe ===")
    serial_obj = shield.getSerialObjID()
    rearm_leg(shield, leg)
    serial_obj.reset_input_buffer()
    send_and_log(shield, "TEST_FIXED_FREQ", leg, frequency, amplitude, offset, delay=0.5)
    print(f"  Maintien de l'onde pendant {run_duration} s avant arrêt…")
    time.sleep(max(run_duration, 0.5))
    send_and_log(shield, "TEST_WAVE_STOP", leg, delay=0.5)
    record = capture_scope(shield, timeout=20.0)
    analyze_record("TEST_FIXED_FREQ", record)
    persist_record("TEST_FIXED_FREQ", record)


def main() -> int:
    # port = discover_port()
    # shield = Shield_Device(shield_port=port, shield_type="TWIST")
    shield = Shield_Device(shield_port="COM39", shield_type="TWIST") # Local test because I can't connect to the shield automatically

    leg = DEFAULT_LEG
    ensure_leg_ready(shield, leg)

    # Lecture rapide des tensions principales pour vérifier la communication.
    for measurement_name in ("V1", "V2", "V3"):
        try:
            value = shield.getMeasurement(measurement_name)
        except ValueError:
            print(f"Mesure {measurement_name} indisponible pour ce shield.")
        else:
            print(f"{measurement_name} = {value:.3f}")

    run_sensitivity_test(shield, leg, vref=15.0)
    time.sleep(1.0)
    run_chirp_test(shield, leg, start_freq=50.0, end_freq=1000.0, duration=0.1)
    time.sleep(1.0)
    run_fixed_frequency_test(shield, leg, frequency=200.0, amplitude=0.4, offset=0.5)

    send_and_log(shield, "POWER_OFF")
    send_and_log(shield, "IDLE")

    print("\nSéquence de tests terminée.")
    return 0


if __name__ == "__main__":
    sys.exit(main())