# Demo Playbook

Purpose: Turn the three scenarios in `grid_event_study.md` into concise, testable demos aligned with standards and audience expectations.

General setup

- Tools: Real‑time simulator or EMT/phasor sim, PMU stream (or Class A recorder), plotting dashboard. Open‑source options: PowSyBl for planning, OpenSTEF for forecasting context; for EMT use local tools.
- Signals to capture: Vrms, angle, frequency, ROCOF, P/Q flows, converter currents, DC‑link voltage, mode meter outputs.
- KPIs: Compliance against code/test profile, recovery time, damping ratio, THD/indices.

1) Voltage Sag with Phase Jump

- Profile: Step sag to 0.6 pu for 150 ms with +20° angle jump at fault inception; recover to 1.0 pu with 5% overshoot max.
- Standards references: IEC 61000‑4‑11/34 templates; NC RfG/IEEE 2800 LVRT stay‑connected region; EN 50160 context at PCC.
- Acceptance: No trip; current limited within capability; PLL or GFM phase recovers < 500 ms; THD < 8% during fault at PCC; DC‑link stays within safe band.
- What to show: Side‑by‑side traces with and without improved control (e.g., PLL bandwidth, virtual impedance, or GFM mode).

2) Oscillation Ring‑Down

- Profile: Small disturbance excites ≈0.4 Hz mode with initial damping 1–2%.
- Instrumentation: PMU time series, mode meter (Prony/LS) computing frequency and damping every second.
- Acceptance: Damping ratio improves to > 5% after enabling damping controls (PSS/HVDC/GFM); ring‑down time constant reduced by > 50%.
- What to show: Mode frequency/damping trend; spatial mode shape (arrows on buses) before/after.

3) Frequency Drop and ROCOF

- Profile: Simulate generator/HVDC loss causing ROCOF ≈ 0.5–1.0 Hz/s; compare base vs added fast frequency response (FFR) or GFM.
- Acceptance: Nadir improvement ≥ 0.2 Hz; ROCOF peak reduced ≥ 30%; recovery within SOGL quality window; no DER nuisance trips per IEEE 1547 hysteresis.
- What to show: Frequency and ROCOF traces; inferred inertia from initial slope; FFR power injection timeline.

Packaging tips

- Keep each scenario to ≤ 3 slides: objective, method, results vs acceptance.
- Label plots with standard terms (e.g., LVRT, Pst, damping ratio) to resonate with TSOs/DSOs/OEMs.

