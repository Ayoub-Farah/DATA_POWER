# Glossary

## Phenomena & Events

- **Voltage sag/dip**: Short‑duration reduction in **RMS voltage** to 0.1–0.9 pu, typically 0.5 cycle to 1 minute (per **IEEE 1159**). Often caused by faults or motor starts.
- **Phase‑angle jump**: Sudden change in voltage angle during a disturbance (often accompanies sags); can stress **PLLs** of **grid‑following** inverters.
- **Flicker**: Visible light modulation due to voltage fluctuations; quantified per **IEC 61000‑4‑15** as **Pst** (short‑term) and **Plt** (long‑term).
- **Harmonics**: Steady‑state sinusoidal components at integer multiples of fundamental; summarized by **THD** and **TDD**; measured per **IEC 61000‑4‑7**.
- **Transient over‑voltage**: Short‑duration, high‑frequency over‑voltage (e.g., switching or lightning); may excite resonances.
- **Frequency excursion**: Deviation of system frequency from nominal (50/60 Hz) due to imbalance; see **ROCOF** for df/dt.
- **Inter‑area oscillation**: Low‑frequency (≈0.1–1 Hz) power swings between areas; damping ratio is a key KPI.
- **Converter‑driven oscillation**: Poorly damped oscillations (≈2–15+ Hz) from control interactions of power electronics and grid impedances.
- **Islanding (unintentional)**: A portion of the grid becomes electrically isolated but still energized by **DERs**; anti‑islanding required (**IEEE 1547**).
- **Voltage stability margin (VSM)**: Proximity to voltage collapse; often discussed via Q‑V curve behavior and reactive margins.

## Measurement & Metrics

- **THD (Total Harmonic Distortion)**: Ratio of RMS of harmonics to fundamental RMS; voltage THD limits context‑dependent (see **EN 50160**, **IEEE 519**).
- **TDD (Total Demand Distortion)**: Current distortion normalized by demand current; used in **IEEE 519** for emission limits.
- **ROCOF (Rate of Change Of Frequency)**: df/dt in Hz/s; used for protection, inertia estimation, and **UFLS**; thresholds governed by **SOGL**/TSO policy.
- **Pst / Plt**: Short‑term and long‑term flicker severity indices defined in **IEC 61000‑4‑15**.
- **Q‑V curve / VSM estimator**: Tools/metrics to assess distance to voltage instability.
- **PMU**: Phasor Measurement Unit, per **IEC/IEEE 60255‑118‑1**; provides time‑synchronized phasors, frequency, and ROCOF.
- **WAMS**: Wide‑Area Monitoring System using multiple PMUs and analytics for oscillation, angle, and frequency monitoring.

## Devices, Controls & Codes

- **Grid‑forming (GFM) inverter**: Converter controlling voltage magnitude and frequency (**VSM/droop**); enhances stability and **FRT** capability.
- **Grid‑following (GFL) inverter**: Converter synchronizing to grid via **PLL**; may be sensitive to sags, phase jumps, and **ROCOF**.
- **FRT (Fault Ride‑Through)**: Ability of generating units/converters to stay connected and support the grid during faults (**LVRT/HVRT** profiles in codes).
- **EN 50160**: European public LV/MV/HV supply voltage characteristics (quality delivered at **PCC**).
- **NC RfG / NC HVDC / SOGL / ER**: **ENTSO‑E** network codes for generator/HVDC requirements, system operation, and emergency restoration.
