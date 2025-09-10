# Event Card: Voltage Sag (Dip) + Phase Jump

- Definition: RMS voltage drop to 0.1–0.9 pu for 0.5 cycle to 1 min; may include phase‑angle jump (IEEE 1159). EN 50160 characterizes occurrence at PCC.
- Why it matters: Trips sensitive loads and PLLs; stresses converters; dominates PQ complaints in distribution; key for FRT compliance.
- Typical causes: Short circuits, motor starts, transformer energization, faults on adjacent feeders/transmission.
- Signals/metrics: Vrms vs time, depth (pu), duration (ms/cycles), phase jump (deg), unbalance (%), residual voltage, THD during sag.
- Standards: IEC 61000‑4‑11/34 (immunity tests), IEEE 1159 (definitions), EN 50160 (limits), NC RfG/IEEE 2800 (ride‑through expectations).

Detection & Analysis

- Windowed RMS per IEC 61000‑4‑30 Class A; detect entry/exit thresholds with hysteresis.
- Tag instantaneous/momentary/temporary durations (IEEE 1159) and compute sag indices (SARFI‑x).
- Capture angle jump by tracking positive‑sequence phasor (αβ or PMU stream).

Mitigation

- Network: Faster fault clearing, meshing/reconfiguration, SVC/STATCOM support, transformer tap logic.
- Device/IBR: Robust PLL or PLL‑less control, DC‑link energy buffers, LVRT curves, phase‑jump ride‑through tests.

Demo idea (lab/booth)

- Inject a 0.6 pu sag with +20° phase jump for 150 ms on a test feeder model; measure converter current, PLL angle, and DC‑link ripple; verify RfG LVRT stay‑connected criteria and recovery within 500 ms.

