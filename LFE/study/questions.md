# Flashcards (Q&A)

Basics

Q: What is a voltage sag per IEEE 1159?
A: A reduction in RMS voltage to 0.1–0.9 pu for 0.5 cycle to 1 minute.

Q: Which standards define how to measure sags and flicker?
A: IEC 61000‑4‑30 (measurement methods) and IEC 61000‑4‑15 (flicker meter).

Q: What is ROCOF and why is it important?
A: Rate of Change of Frequency (Hz/s); used for protection and inertia assessment; high values can trip DER and destabilize PLLs.

Harmonics

Q: What’s the difference between THD and TDD?
A: THD is voltage/current distortion relative to fundamental; TDD is current distortion normalized to demand current (IEEE 519 compliance metric).

Q: Which document sets harmonic current emission limits at the PCC?
A: IEEE 519 (tables by short‑circuit ratio and voltage level).

Oscillations

Q: Typical frequency range for inter‑area oscillations?
A: Roughly 0.1–1 Hz; monitored via PMUs and mode meters.

Q: Which KPI summarizes oscillation health?
A: Damping ratio (%); many operators target > 3–5% for critical modes.

Sags & FRT

Q: What is LVRT?
A: Low Voltage Ride‑Through—IBR must remain connected and provide support during specified voltage dips, per NC RfG/IEEE 2800 profiles.

Q: Why do phase‑angle jumps matter for inverters?
A: Large jumps challenge PLLs of grid‑following inverters, causing phase errors, current spikes, or trips; GFM mitigates via internal angle reference.

Frequency/DER

Q: Name two ways to mitigate poor frequency nadir in low‑inertia grids.
A: Commit synchronous inertia and enable fast frequency response or grid‑forming controls on IBRs.

Q: Which guideline governs frequency quality in Europe?
A: ENTSO‑E SOGL (System Operation Guideline).

Compliance & Measurement

Q: Which standard defines PMU measurement requirements?
A: IEC/IEEE 60255‑118‑1.

Q: What’s EN 50160 about?
A: Voltage characteristics (quality) at the PCC in public distribution systems in Europe.

