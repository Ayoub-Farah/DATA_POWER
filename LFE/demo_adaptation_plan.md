# LFE Demo Adaptation Plan (GF/GFL, Events over CAN)

This plan describes how to adapt the current firmware to support multi-inverter demos using a single binary, configured via ThingSet over CAN. It includes incremental steps with validation tests and acceptance criteria.

## Goals
- Single firmware runs on all power converters; role is set via CAN.
- Programmable “events” (voltage sag, ROCOF, oscillation, harmonics) are configured and triggered via CAN.
- Timing can be compressed for demos; safety protections and saturations apply.
- Multiple inverters run in parallel: one Grid-Forming (GFM), others Grid-Following (GFL).
- Support “open-circuit” follower tests (observe bus, no power exchange).

## Constraints & Touchpoints
- CAN/ThingSet enabled: `src/app.conf`, `src/app.overlay`.
- ThingSet registry, config, and callbacks: `src/user_data_objects.h`, `src/thingset_callbacks.cpp`.
- Runtime control variables consumed in the critical task: `src/new_main.cpp` (active via `-DUSE_NEW_MAIN`).
- Inverter control API (GF/GFL, PLL, references): `src/singlePhaseInverter.h/.cpp`.

## Rollout Plan

[x] Step 0 — Baseline Build & Sanity
- Implement: none (use current state).
- Test:
  - Build and flash. Confirm USB shell is available and ThingSet shell enabled: `select thingset`, then `?` to list.
  - Query measurements: `get /Meas/Val/rV1Low_V`, `get /Meas/Val/rV2Low_V`, etc.
  - Set mode to power: `set /Config/Mode/wMode 1` and observe PWM starts, duty within bounds; revert to idle: `set /Config/Mode/wMode 0`.
- Accept: sensors readable; mode toggles PWM; no faults.

[ ] Step 1 — Wire AC Role (GF/GFL) to Controller
- Implement:
  - On startup, initialize inverter mode from `/Config/Function/wAC_Mode` (GF=0, GFL=1).
  - In ThingSet `conf_func_cb` POST_WRITE, when `wAC_Mode` changes, update inverter mode and reset sync (`setSyncOff()`); for GFL, keep `setPowerOn(false)` until sync is achieved.
  - In the critical task, if GFL and not synced, compute duty but do not enable power output (or hold drivers off via leg control).
- Test:
  - Set GF unit: `set /Config/Function/wAC_Mode 0`, `set /Config/Mode/wMode 1`; observe voltage formation.
  - Set a follower: `set /Config/Function/wAC_Mode 1`, `set /Config/Mode/wMode 1`; observe PLL tracking (frequency follows bus) and sync gating before enabling output.
- Accept: role switches via ThingSet; GF runs open-loop frequency; GFL uses PLL with sync gating.

[ ] Step 2 — Define ThingSet Event Schema
- Implement: Add `/Config/Event` group with:
  - Global: `wTimeScale` (float), optional `wStartAt_tick` (uint32) for scheduled start.
  - Slots `/Config/Event/<n>` (n=0..3) with fields by type:
    - `wType` (enum): V_SAG, V_SWELL, F_STEP, F_ROCOF, OSC_DECAY, HARMONIC, OPEN_CIRCUIT, SYNC_RESET.
    - Generic: `wDuration_ms`, `wEnable` (bool), `xTrigger` (exec).
    - V-sag/swells: `wDepth_pct`, `wPhaseJump_deg`.
    - F-step/ROCOF: `wDeltaF_Hz`, `wROCOF_Hzps`.
    - Oscillation: `wOmega_radps`, `wDamping`, `wAmp_pct` (of fundamental).
    - Harmonic: `wHarm_k`, `wHarm_pct`.
  - Callbacks only validate ranges and set “armed” flags; no control logic here.
- Test:
  - In ThingSet shell, list `/Config/Event` nodes; set parameters and trigger `xTrigger`. Confirm printk logs from callbacks.
- Accept: schema visible and writable; values persist; triggers observable.

[ ] Step 3 — Event Runner in Critical Task
- Implement:
  - Add a tiny event state machine polled in the critical task before calling `setDutyCycle`.
  - Apply active event effects to controller inputs:
    - GF: modify `Vdq_ref` (depth, harmonic, oscillation) and `_w_ref` (F-step, ROCOF); optional phase jump via `theta` offset or transient `w_ref`.
    - GFL: typically no event-driven reference changes; only observe and log (unless explicit follower injections are part of demo).
  - Use `wTimeScale` to compress durations and ramp rates. Enforce saturation and slew limits on `Vdq_ref.d` and `_w_ref`.
- Test (single unit on bench):
  - V-sag: program 30% depth, 200 ms; trigger; verify Vrms drop and duration (±10%).
  - ROCOF: 1 Hz/s for 500 ms; verify frequency slope from PLL or Vab period (±15%).
  - Oscillation: inject 10% at 20 Hz with damping 0.1; verify ring-down envelope.
- Accept: each event yields measurable effect within tolerance, respects safety limits, and ends cleanly.

[ ] Step 4 — Frequency Control Hooks
- Implement:
  - Add a setter to adjust `_w_ref` from event runner. Optionally expose `/Config/AC/wFreqRef_Hz` for manual tests (GF only, guarded by bounds).
- Test:
  - Manual: set `wFreqRef_Hz` to 49 Hz and 51 Hz; verify Vab period changes accordingly.
  - Event: F-step to +0.5 Hz; verify duty and voltage phase evolution reflect new frequency.
- Accept: GF frequency references take effect with bounded slew and range.

[ ] Step 5 — Open-Circuit Support
- Implement:
  - Followers: expose a convenience flag `/Config/Event/wOpenCircuitFollowers` (bool). When true, force `wDriver=false` on all legs of GFL units, or keep `setPowerOn(false)`.
  - Ensure reads continue (measurements still published) in open-circuit.
- Test:
  - Set open-circuit true; verify drivers are disabled (ThingSet leg state reflects it) and measured follower currents ~0 A.
  - Trigger events on GF; followers observe bus changes with no power flow.
- Accept: open-circuit holds with stable measurements; easy toggle via CAN.

[ ] Step 6 — Multi-Inverter Orchestration
- Implement:
  - Standardize broadcast flow: (1) program event params on all nodes, (2) set `wStartAt_tick` in the future on all nodes, (3) arm `wEnable=true`, (4) broadcast `xTrigger`.
  - Optional: add a simple `/Config/Event/wSkewGuard_ms` to ignore late triggers.
- Test (2-node minimum):
  - Use two devices GF+GFL. Schedule a V-sag with `wStartAt_tick` ~1–2 s in future; measure start time delta between both nodes (< 10 ms target; adjust if using CAN broadcast timing).
- Accept: coordinated onset across devices within set tolerance.

[ ] Step 7 — Safety & Timing Tuning
- Implement:
  - Saturations: clamp `Vdq_ref.d` to [1 V, 0.5·Vbus] and `|Idq_ref.q|` to ≤ 3 A (initial values; adjust per hardware).
  - Rate limiters: `dV/dt`, `df/dt` capped to safe demo values, scaled by `wTimeScale`.
- Test:
  - Attempt to set extreme event parameters; verify clamps and warnings. Confirm no PWM saturation or faults.
- Accept: firmware rejects unsafe parameters gracefully; logs warnings.

[ ] Step 8 — Observability
- Implement:
  - Optional `/Diag` group: `rSync` (bool), `rTheta`, `rFreq_Hz`, `rVpk_ref`, `rIq_ref`, `rEventState`.
  - Keep ThingSet Shell quiet unless queried.
- Test:
  - Read `/Diag/*` during events; values consistent with behavior (e.g., `rFreq_Hz` slope under ROCOF).
- Accept: minimal diagnostics facilitate demo verification.

[ ] Step 9 — Demo Playbook & Scripts
- Implement:
  - Provide curated ThingSet command sequences (USB shell or CAN) for:
    - Voltage sag with phase jump
    - Oscillation ring-down
    - Frequency excursion (step, ROCOF)
  - Add timing notes and safety checks.
- Test:
  - Dry-run sequences on bench; iterate default parameters for clarity.
- Accept: repeatable demo sequences produce expected plots.

## Example ThingSet Sequences (USB Shell)

Prereq: `select thingset`

- Set roles and modes:
  - GF: `set /Config/Function/wAC_Mode 0`; `set /Config/Mode/wMode 1`
  - GFL: `set /Config/Function/wAC_Mode 1`; `set /Config/Mode/wMode 1`

- Open-circuit followers: `set /Config/Event/wOpenCircuitFollowers true`

- Program V-sag (slot 0):
  - `set /Config/Event/wTimeScale 5`
  - `set /Config/Event/0/wType 0`  (V_SAG)
  - `set /Config/Event/0/wDepth_pct 30`
  - `set /Config/Event/0/wDuration_ms 200`
  - `set /Config/Event/0/wPhaseJump_deg 10`
  - `exec /Config/Event/0/xTrigger`

- Program ROCOF (slot 1) starting in 1 s:
  - `set /Config/Event/1/wType 3`  (F_ROCOF)
  - `set /Config/Event/1/wROCOF_Hzps 1.0`
  - `set /Config/Event/1/wDuration_ms 500`
  - `set /Config/Event/wStartAt_tick <now+1000ms>`
  - `exec /Config/Event/1/xTrigger`

## Acceptance Gates Summary
- Each step has measurable pass/fail criteria (see above). Only proceed when accepted.
- Record parameter sets and measured outcomes to build a repeatable profile library.

## Rollback Strategy
- The feature is additive and configuration-guarded. If issues appear, disable events by leaving `wEnable=false` and operate only with GF/GFL role selection.

## Notes
- For precise multi-device sync, prefer scheduled `wStartAt_tick` over ad-hoc broadcast; CAN bus latency and shell timing can skew triggers otherwise.
- Always verify sensor calibration via `/Meas/<ch>/wGain` and `/Meas/<ch>/wOffset` before demos.

