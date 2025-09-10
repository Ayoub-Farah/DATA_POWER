# Event Card: Inter-Area and Converter-Driven Oscillations

- Definition: Poorly damped electromechanical or control‑mode oscillations visible in angle, frequency, and power flows.
- Bands: Inter‑area ≈0.1–1 Hz; local/gen‑unit ≈1–3 Hz; converter/control interactions ≈2–15+ Hz.
- Why it matters: Can limit transfer capability, trigger protection, or escalate to separation.
- Signals/metrics: PMU angle/frequency, active/reactive power, modal frequency, damping ratio (%), mode shape; ring‑down time constant.
- Standards/practice: PMU per IEC/IEEE 60255‑118‑1; SOGL monitoring expectations; utility KPI often targets damping ratio > 3–5% for critical modes.

Detection & Analysis

- Ring‑down identification (Prony/LS), spectral methods, stochastic subspace; real‑time mode meters in WAMS.
- Mode shape identification across PMU buses; correlate to HVDC/IBR setpoints to locate sources.

Mitigation

- Control tuning (PSS/FACTS/IBR), HVDC damping, remedial action schemes, operating point adjustments; virtual inertia/damping in GFM.

Demo idea (lab/booth)

- Excite a 0.4 Hz mode with low damping, then enable synthetic inertia/damping; show improved damping ratio and faster ring‑down on PMU plots.

