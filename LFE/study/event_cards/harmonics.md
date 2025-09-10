# Event Card: Harmonics and Distortion

- Definition: Steady‑state spectral content at integer multiples of fundamental; interharmonics at non‑integer multiples. Summarized by THD (voltage) and TDD (current).
- Why it matters: Heating, misoperation, resonance risks, compliance obligations at PCC; interaction with capacitor banks and filters.
- Typical causes: Power electronic converters, arc furnaces, variable speed drives, saturation, unbalanced rectifiers.
- Signals/metrics: Per‑order Ih/Vh, THD, TDD, K‑factor, notch depth, interharmonics, supraharmonics (2–150 kHz, emerging topic).
- Standards: IEC 61000‑4‑7 (measurement), IEEE 519 (limits), EN 50160 (voltage THD at PCC), IEC 61400‑21‑1 (wind PQ reporting).

Measurement & Compliance

- Use IEC 61000‑4‑7 grouping/aggregation for consistent spectra; THD over 10‑cycle (50 Hz) windows; apply demand current for TDD.
- Compare against voltage THD limits (e.g., 3–8% by voltage level) and current emission tables per short‑circuit ratio.

Mitigation

- Network: Increase short‑circuit strength, detune capacitors, shunt/series filters, active filtering.
- Device/IBR: PWM strategies, LCL tuning, harmonic controllers, grid‑forming with virtual impedance.

Demo idea (lab/booth)

- Sweep converter firing/PWM parameters and grid impedance; plot THD and per‑order emissions; show compliance margin vs IEEE 519 table at a chosen PCC.

