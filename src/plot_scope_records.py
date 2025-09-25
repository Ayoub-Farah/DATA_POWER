"""Plot the latest ScopeMimicry captures for the automated tests.

This helper scans the ``scope_records`` directory (created by
``SPIN_COMM_test_script.py``) and opens the most recent CSV files for the
three automated sequences:

* réponse en échelon (``TEST_SENSI``)
* balayage fréquentiel (``TEST_CHIRP``)
* sinusoïde à fréquence fixe (``TEST_FIXED_FREQ``)

Each capture is plotted in its own matplotlib figure so the behaviour of the
converter can be inspected visually after a test run.
"""

from __future__ import annotations

import argparse
import csv
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Sequence

try:
    import matplotlib.pyplot as plt
except ModuleNotFoundError as exc:  # pragma: no cover - user feedback only
    raise SystemExit(
        "matplotlib est requis pour tracer les captures (pip install matplotlib)."
    ) from exc


@dataclass(frozen=True)
class TestConfig:
    """Describe how CSV files are named for a given automated test."""

    display_name: str
    file_suffix: str


TESTS: Sequence[TestConfig] = (
    TestConfig(display_name="TEST_SENSI", file_suffix="test_sensi"),
    TestConfig(display_name="TEST_CHIRP", file_suffix="test_chirp"),
    TestConfig(display_name="TEST_FIXED_FREQ", file_suffix="test_fixed_freq"),
)


def find_latest_csv(directory: Path, suffix: str) -> Path | None:
    """Return the newest CSV file whose name ends with ``suffix``.

    The acquisition files are timestamped (``YYYYmmdd-HHMMSS-ffffff``). Sorting
    alphabetically therefore also sorts chronologically, so the last entry is
    the most recent capture.
    """

    pattern = f"*_{suffix}.csv"
    matches = sorted(directory.glob(pattern))
    if not matches:
        return None
    return matches[-1]


def load_csv_series(csv_path: Path) -> Dict[str, List[float]]:
    """Load numerical series from ``csv_path``.

    Columns that cannot be converted to ``float`` are silently skipped.
    """

    series: Dict[str, List[float]] = defaultdict(list)
    with csv_path.open("r", newline="") as stream:
        reader = csv.DictReader(stream)
        for row in reader:
            for column, value in row.items():
                if value is None or value == "":
                    continue
                try:
                    series[column].append(float(value))
                except ValueError:
                    # Skip non-numeric columns
                    continue
    return series


def plot_series(test: TestConfig, series: Dict[str, List[float]]) -> None:
    """Render the measurement series in a dedicated matplotlib figure."""

    if not series:
        print(f"Aucune donnée exploitable pour {test.display_name}.")
        return

    plt.figure()
    for column, values in series.items():
        if not values:
            continue
        plt.plot(range(len(values)), values, label=column)

    plt.title(f"Dernière capture – {test.display_name}")
    plt.xlabel("Échantillon")
    plt.ylabel("Valeur")
    plt.grid(True, which="both", linestyle="--", linewidth=0.5)
    plt.legend()


def main(argv: Iterable[str] | None = None) -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "directory",
        nargs="?",
        default="scope_records",
        type=Path,
        help="Dossier contenant les captures CSV (par défaut: scope_records)",
    )
    args = parser.parse_args(argv)

    directory: Path = args.directory
    if not directory.exists():
        raise SystemExit(f"Le dossier {directory} est introuvable.")

    for test in TESTS:
        csv_path = find_latest_csv(directory, test.file_suffix)
        if csv_path is None:
            print(
                f"Aucun fichier correspondant à {test.display_name} dans {directory}."
            )
            continue

        print(f"Chargement de {csv_path} pour {test.display_name}…")
        series = load_csv_series(csv_path)
        plot_series(test, series)

    plt.show()


if __name__ == "__main__":
    main()
