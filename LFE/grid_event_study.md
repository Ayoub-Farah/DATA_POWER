Got it ✅ — here’s your report restructured as a **Markdown document** for clarity and easier reuse.

---

# AC and DC Grid Event Research at LF Energy Summit 2025 Participants

## Summary

Key power system players at the **LF Energy Summit Europe 2025** (Aachen) – including TSOs, DSOs, vendors, and research labs – are actively studying a broad range of AC **and DC** grid disturbances. Common focus areas include:

* **Voltage sags (dips) & phase jumps**
* **Harmonic distortions**
* **Flicker**
* **Transient over-voltages**
* **Frequency excursions & ROCOF**
* **Inter-area oscillations** (and emerging **converter-driven oscillations**)
* **Short-circuit faults & ride-through capability**
* **Unintentional islanding**
* **Voltage stability margins**
* **Blackout/extreme events**

They leverage advanced tools (PMU/WAMS, real-time simulators, open source libraries like PowSyBl/OpenSTEF) and often align their work with **international standards** – from power quality monitoring norms to grid code requirements.

This report provides:

1. A structured overview of institutions and their research focus.
2. An event × institution heatmap.
3. A consolidated standards index.
4. Three high-impact demo scenario suggestions.
5. Institution-specific insights for booth conversations.

---

## Participant Research & Publications by Institution

| **Institution**         | **Top Event Types**                                                    | **Methods / Tools**                                                         | **Representative Publications**                                         | **Key Standards Cited**                                                  | **Relevance for Live Demo**                                     |
| ----------------------- | ---------------------------------------------------------------------- | --------------------------------------------------------------------------- | ----------------------------------------------------------------------- | ------------------------------------------------------------------------ | --------------------------------------------------------------- |
| **RTE (France)**        | Oscillations, low inertia frequency events, FRT & phase jumps          | PMUs, modal analysis, AI for voltage control                                | PMU-based modal analysis (2024); RTE/IEA 2050 scenarios (2021)          | IEC/IEEE 60255-118-1 (PMU); ENTSO-E RfG; IEEE 519; CE 1 Hz/s ROCOF limit | Oscillation ring-down and FRT demo will resonate.               |
| **Swissgrid**           | System splits, inter-area oscillations, voltage stability margins      | WAMS with PMUs, ABB oscillation tools, voltage margin estimation            | Oscillation monitoring (2013); CE split/oscillation lessons (2022)      | IEC/IEEE 60255-118-1; ENTSO-E SOGL; NPCC criteria; IEC 61000-4-30        | Emulating oscillations or splits will appeal strongly.          |
| **Elia & 50Hertz**      | Converter-driven oscillations, synthetic inertia, operational security | HVDC interaction studies, ENTSO-E HVDC guidelines, real-time simulations    | ENTSO-E HVDC guidelines (2020); CIGRÉ demo on damping (2022)            | NC HVDC; ENTSO-E HVDC guidance; ENTSO-E RfG; IEC 61400-21-1              | Demo of HVDC oscillations & synthetic inertia damping is ideal. |
| **Alliander (NL DSO)**  | Voltage dips, harmonics & flicker, islanding/DC microgrids             | Wide-area PQ monitoring, PMUs in distribution, DC pilot grids               | PQ propagation study (2015); “Making our grid sweat” (2025)             | EN 50160; IEC 61000-4-30; IEC 61000-4-15; IEEE 1547                      | Voltage sag + PQ compliance demo matches DSO challenges.        |
| **TenneT (NL/DE)**      | Low inertia events, blackout defense, HVDC integration                 | Inertia Dashboard, frequency stability studies, grid-forming converter R\&D | CIGRÉ inertia paper (2025); ENTSO-E frequency reports (2021/23)         | ENTSO-E SOGL; Project Inertia report; IEEE C37.118.2; NC HVDC            | Fast frequency drop (ROCOF) demo ties directly to their work.   |
| **ENTSO-E**             | Pan-European incidents, network codes, inertia                         | Incident analysis reports, drafting NCs, WAMPAC guidelines                  | Oscillation event report (2018); Frequency stability study (2021)       | NC RfG; NC HVDC; NC ER; SOGL; Inertia report (2021)                      | Demo framed with ENTSO-E grid code terms wins credibility.      |
| **Hydro-Québec / IREQ** | Voltage sags, harmonics, SSR, islanding                                | PQ monitoring, SSR damping, Hypersim simulations                            | Harmonics detection under sags (2021); SVC SSR control study            | NPCC Directories; CSA C235; IEEE 1159; IEEE 519                          | Sag demo + harmonic resonance discussion will connect well.     |
| **RWTH Aachen (ERC)**   | Grid-forming converter FRT, WAMPAC, hybrid AC/DC grids                 | Real-time HIL, MIGRATE project, SEAPATH (LF Energy)                         | MIGRATE stability report (2019); WAMPAC talk (2025)                     | ENTSO-E RfG & HVDC; IEC 61850; IEEE 2800; IEC 61000-4-11/34              | FRT demo with grid-forming inverters resonates with academics.  |
| **GE Vernova**          | FRT in renewables, converter-driven oscillations, PMU monitoring       | Simulation & compliance testing, WAMS solutions                             | LVRT/HVRT strategies (2021); synthetic inertia oscillation study (2020) | IEEE 519; IEC 61400-21; ENTSO-E RfG; IEEE C37.118                        | Demo of wind/PV FRT aligned with RfG will impress OEMs.         |
| **OMICRON**             | PQ analyzer compliance, relay testing, PMU calibration                 | Event waveform simulators, automated PQ test plans                          | PQ instrument testing note (2020); relay test app note (2018)           | IEC 61000-4-30; IEC 62586-2; IEC 61000-4-11/34; IEEE/IEC 60255-118-1     | Demo with standard dip waveforms ties to OMICRON’s business.    |

---

## Event Type × Institution Heatmap

| **Event Type**               | **RTE** | **Swissgrid** | **Elia/50Hz** | **Alliander** | **TenneT** | **HQ/IREQ** | **RWTH** | **GE** | **OMICRON** |
| ---------------------------- | :-----: | :-----------: | :-----------: | :-----------: | :--------: | :---------: | :------: | :----: | :---------: |
| **Voltage sags/dips**        |    1    |       1       |       1       |     **2**     |      1     |    **2**    |     1    |  **2** |    **2**    |
| Harmonics                    |    1    |       1       |       1       |       1       |      0     |    **2**    |     1    |    1   |    **2**    |
| Flicker                      |    0    |       0       |       0       |     **2**     |      0     |      1      |     0    |    0   |      1      |
| Inter-area oscillations      |  **3**  |     **2**     |       1       |       0       |      1     |      0      |     1    |    1   |      0      |
| Frequency excursions & ROCOF |    1    |       1       |       1       |       0       |    **3**   |      1      |     0    |    0   |      0      |
| Fault Ride-Through (FRT)     |    1    |       0       |       1       |       0       |      1     |      0      |   **2**  |  **3** |      1      |
| Voltage stability margins    |    1    |     **2**     |       1       |       0       |      1     |      0      |     0    |    0   |      0      |
| DC grid events               |    0    |       0       |     **2**     |       0       |      1     |      0      |     1    |    0   |      0      |

---

## Standards Index

**Most cited standards in participant publications:**

* **IEC 61000-4-30:2015+A1:2021** – PQ measurement (Alliander, OMICRON).
* **IEC 61000-4-7** – Harmonic measurement (RTE, GE).
* **IEC 61000-4-11/34** – Sag & interruption immunity (RWTH, OMICRON).
* **IEC 61000-4-15** – Flicker meter spec (Alliander, OMICRON).
* **IEEE 1159-2019** – PQ event definitions (Hydro-Québec).
* **IEEE 519-2022** – Harmonic limits (RTE, HQ, GE).
* **IEC/IEEE 60255-118-1:2018** – Synchrophasors (RTE, Swissgrid, OMICRON).
* **IEEE C37.118.2-2011** – PMU data transfer (TSOs).
* **EN 50160:2010/2022** – Voltage quality in EU grids (Alliander, DSOs).
* **ENTSO-E NC RfG (2016)** – Generator requirements (RTE, GE, RWTH).
* **ENTSO-E NC HVDC (2016)** – HVDC interaction requirements (Elia, RWTH).
* **ENTSO-E SOGL (2017)** – Operational security/ROCOF (Swissgrid, TenneT).
* **ENTSO-E ER (2017)** – Emergency & Restoration (blackouts).
* **ENTSO-E Inertia Report (2021)** – ROCOF thresholds (TenneT, Swissgrid).
* **IEEE 1547-2018** – DER interconnection (Alliander, RWTH).
* **IEEE 2800-2022** – Grid-forming inverter requirements (RWTH, academia).
* **IEC 61400-21-1:2019** – Wind turbine PQ measurement (GE).
* **NPCC Directory 1 & CSA C235** – North American PQ/voltage standards (HQ).

---

## Three High-Impact Demo Scenarios

1. **Voltage Sag with Phase Jump**

   * *Who cares:* RTE, Elia, GE, Alliander, OMICRON.
   * *Standards:* NC RfG, IEEE 2800, IEC 61000-4-11, EN 50160.

2. **Oscillation Ring-Down**

   * *Who cares:* RTE, Swissgrid, Elia, Statnett.
   * *Standards:* IEEE 60255-118-1 (PMU), ENTSO-E SOGL (ROCOF alarms).

3. **Voltage Stability Margin Nudge**

   * *Who cares:* Swissgrid, TenneT, RTE, ENTSO-E.
   * *Standards:* EN 50160, SOGL, NC RfG (Q(V) droop requirements).

---

## Institution-Specific Insights

* **RTE:** Obsessed with **oscillations** and **low inertia**. Speak in terms of PMUs, damping, and RfG compliance.
* **Swissgrid:** Vigilant on **splits and stability**. Name-drop WAMS, ROCOF ≤1 Hz/s, and PMU standards.
* **Elia/50Hz:** Focused on **HVDC interactions** and **synthetic inertia**. Connect to ENTSO-E HVDC code.
* **Alliander:** PQ at distribution: dips, flicker, harmonics. Reference EN 50160 & IEC 61000-4-30.
* **TenneT:** Low inertia & ROCOF. Mention their “Inertia Dashboard” and SOGL frequency rules.
* **ENTSO-E:** Standards-driven. Always tie demos back to NC RfG/HVDC/ER/SOGL.
* **Hydro-Québec:** PQ & harmonics. Use IEEE/CSA/NPCC language.
* **RWTH Aachen:** Academic bridge. Mention IEEE 2800, IEC 61850, grid-forming inverter FRT.
* **GE Vernova:** Compliance & testing. Reference IEC 61400-21, IEEE 519, RfG.
* **OMICRON:** Testing accuracy. Name-drop IEC 61000-4-30, 61000-4-11, 60255-118-1.

---

Do you want me to also prepare a **shorter, presentation-style summary (like a 1–2 page handout)** with just the demo scenarios, key standards, and audience mapping that you could use directly at your booth?
