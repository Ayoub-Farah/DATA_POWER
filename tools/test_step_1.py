#!/usr/bin/env python3
"""
Step 1 Validation: AC Role (GF/GFL) and Gating via ThingSet.

This script connects to the device's ThingSet shell and validates:
 - /Config/Function/wAC_Mode switches between GF (0) and GFL (1)
 - Entering power mode (/Config/Mode/wMode = 1) reflects gating behavior:
   * GF: per-leg wOn remains steadily True (proxy when DC bus is low)
   * GFL: per-leg wOn blinks (observed as both True and False over time)

It reuses the shell utilities from tools/thingset_autotest.py to avoid duplication.

Usage:
  python tools/test_step_1.py [--port /dev/ttyACM0] [--baud 115200]
                              [--leg-index 0] [--use-thingset-prefix]
                              [--verbose]
"""

from __future__ import annotations

import argparse
import sys
import time
from pathlib import Path
from typing import List, Optional


def _import_shell_helpers():
    here = Path(__file__).resolve().parent
    if str(here) not in sys.path:
        sys.path.insert(0, str(here))
    try:
        import thingset_autotest as tsauto  # type: ignore
    except Exception as e:
        print("ERROR: cannot import tools/thingset_autotest.py.\n"\
              "Ensure you run from repo root and that file exists.")
        raise
    return tsauto


def get_bool(sh, path: str) -> Optional[bool]:
    ok, resp = sh.get_value(path)
    if not ok or resp is None:
        return None
    _, val = sh.parse_scalar(resp) if hasattr(sh, 'parse_scalar') else (None, None)
    # Fallback: use module-level parser if ThingSetShell has no bound parse
    if val is None:
        from thingset_autotest import parse_scalar  # type: ignore
        _, val = parse_scalar(resp)
    if isinstance(val, bool):
        return val
    s = str(val).strip().lower()
    if s in ("true", "1"):  # best-effort
        return True
    if s in ("false", "0"):
        return False
    return None


def poll_bools(sh, path: str, duration_s: float, every_s: float) -> List[bool]:
    samples: List[bool] = []
    t0 = time.time()
    while (time.time() - t0) < duration_s:
        b = get_bool(sh, path)
        if b is not None:
            samples.append(b)
        time.sleep(max(0.02, every_s))
    return samples


def set_uint(sh, path: str, value: int) -> bool:
    ok, _ = sh.set_value(path, str(int(value)))
    return ok


def ensure_nodes(sh, needed_paths: List[str]) -> bool:
    try:
        nodes = sh.list_nodes()
    except Exception:
        return False
    present = {n.path for n in nodes}
    missing = [p for p in needed_paths if p not in present]
    if missing:
        print("Missing ThingSet nodes:")
        for m in missing:
            print(f"  - {m}")
        return False
    return True


def validate_role_gating(sh, leg_index: int, verbose: bool = False) -> int:
    role_path = "/Config/Function/wAC_Mode"
    mode_path = "/Config/Mode/wMode"
    leg_on_path = f"/Config/Leg/{leg_index}/wOn"

    # Check required nodes exist
    if not ensure_nodes(sh, [role_path, mode_path, leg_on_path]):
        return 2

    def _log(msg: str):
        if verbose:
            print(msg)

    # Helper: enter IDL, set role, enter PWR
    def switch_to(role: int) -> bool:
        if not set_uint(sh, mode_path, 0):
            _log("Failed to set IDL mode")
            return False
        if not set_uint(sh, role_path, role):
            _log("Failed to set AC role")
            return False
        if not set_uint(sh, mode_path, 1):
            _log("Failed to set PWR mode")
            return False
        return True

    # GF sequence: role=0 then expect steady True on leg wOn
    print("[GF] Setting role GF (0) and entering PWR …")
    if not switch_to(0):
        return 2
    # Give background task time to reflect state
    time.sleep(0.3)
    gf_samples = poll_bools(sh, leg_on_path, duration_s=1.6, every_s=0.3)
    print(f"[GF] Observed wOn samples: {gf_samples}")
    if not gf_samples or not all(gf_samples) or (set(gf_samples) != {True}):
        print("GF FAIL: leg wOn not steadily True (expected steady ON)")
        # Continue to GFL but mark failure
        gf_ok = False
    else:
        print("GF PASS: leg wOn steady True")
        gf_ok = True

    # GFL sequence: role=1 then expect toggling over time
    print("[GFL] Setting role GFL (1) and entering PWR …")
    if not switch_to(1):
        return 2
    time.sleep(0.3)
    gfl_samples = poll_bools(sh, leg_on_path, duration_s=2.0, every_s=0.25)
    print(f"[GFL] Observed wOn samples: {gfl_samples}")
    toggled = (True in gfl_samples) and (False in gfl_samples)
    if not gfl_samples or not toggled:
        print("GFL FAIL: leg wOn did not toggle (expected blinking before sync)")
        gfl_ok = False
    else:
        print("GFL PASS: leg wOn toggles as expected")
        gfl_ok = True

    ok = gf_ok and gfl_ok
    print("\nRESULT:")
    if ok:
        print("✔ PASS Step 1 (role + gating proxy)")
        return 0
    else:
        print("✖ FAIL Step 1 (see messages above)")
        return 1


def main():
    tsauto = _import_shell_helpers()

    ap = argparse.ArgumentParser(description="Step 1 validation: AC role + gating proxy test.")
    ap.add_argument("--port", help="Serial port (auto-discover if omitted)")
    ap.add_argument("--baud", type=int, default=115200, help="Baud rate")
    ap.add_argument("--leg-index", type=int, default=0, help="Leg index to probe (default 0)")
    ap.add_argument("--use-thingset-prefix", action="store_true", help="Prefix commands with 'thingset '")
    ap.add_argument("--verbose", action="store_true", help="Verbose logging")
    args = ap.parse_args()

    ser = tsauto.discover_device(args.port, args.baud, args.verbose, args.use_thingset_prefix)
    if not ser:
        sys.exit(2)

    sh = tsauto.ThingSetShell(ser, verbose=args.verbose, cmd_prefix=("thingset " if args.use_thingset_prefix else ""))
    assert sh.enter_thingset()

    try:
        rc = validate_role_gating(sh, leg_index=args.leg_index, verbose=args.verbose)
    finally:
        try:
            ser.close()
        except Exception:
            pass
    sys.exit(rc)


if __name__ == "__main__":
    main()

