# Standards Index (Overview Table)

Note: Summaries are practical pointers; consult the official publications for binding requirements. “Year” uses the editions cited in your base doc when available.

| Standard | What it does | Year | Used by (from base doc) | Glossary section(s) | Key ref (page/table) |
| --- | --- | --- | --- | --- | --- |
| IEC 61000‑4‑30 | PQ measurement methods (Class A/S): RMS voltage, frequency, dips/swells/interruptions, flicker | 2015 + A1:2021 | Alliander, OMICRON, Swissgrid | Measurement & Metrics | — |
| IEC 61000‑4‑7 | Harmonic/interharmonic measurement method (DFT grouping/aggregation) | — | RTE, GE | Measurement & Metrics | — |
| IEC 61000‑4‑11 / ‑4‑34 | Voltage dip/short interruption immunity test profiles (≤16 A / >16 A per phase) | — | RWTH, OMICRON | Devices, Controls & Codes | — |
| IEC 61000‑4‑15 | Flicker meter spec; computes Pst/Plt | — | Alliander, OMICRON | Measurement & Metrics | — |
| IEC/IEEE 60255‑118‑1 | Synchrophasor (PMU) measurement requirements and performance classes | 2018 | RTE, Swissgrid, OMICRON | Measurement & Metrics | — |
| IEEE C37.118.2 | Synchrophasor data transfer (frames, protocol) | 2011 | TenneT, GE | Measurement & Metrics | — |
| IEEE 1159 | PQ monitoring/event taxonomy (sags, swells, transients) | 2019 | Hydro‑Québec/IREQ | Phenomena & Events | — |
| IEEE 519 | Harmonic control at PCC (voltage THD planning levels, current emission via TDD/Ih) | 2022 | RTE, Hydro‑Québec/IREQ, GE | Devices, Controls & Codes | — |
| EN 50160 | Voltage characteristics at PCC in public distribution networks (steady‑state limits, THD) | 2010/2022 | Alliander (DSO context) | Devices, Controls & Codes | — |
| ENTSO‑E NC RfG | Generator requirements (FRT profiles, Q(V), operating ranges) | 2016 | RTE, RWTH, GE, ENTSO‑E | Devices, Controls & Codes | — |
| ENTSO‑E NC HVDC | HVDC connection requirements and interactions (fault behavior, freq support) | 2016 | Elia/50Hertz, TenneT, RWTH, ENTSO‑E | Devices, Controls & Codes | — |
| ENTSO‑E SOGL | System Operation Guideline (operational security, frequency quality, ROCOF alarms) | 2017 | Swissgrid, TenneT, ENTSO‑E | Devices, Controls & Codes | — |
| ENTSO‑E ER | Emergency & Restoration (black‑start, defense, restoration plans) | 2017 | ENTSO‑E | Devices, Controls & Codes | — |
| ENTSO‑E Inertia Report | Guidance on inertia/ROCOF thresholds and system needs | 2021 | TenneT, Swissgrid, ENTSO‑E | Devices, Controls & Codes | — |
| IEEE 1547 | DER interconnection and interoperability (voltage/frequency trip, ride‑through) | 2018 | Alliander, RWTH | Devices, Controls & Codes | — |
| IEEE 2800 | Transmission‑connected IBR interconnection (GFM/GFL behavior, ride‑through) | 2022 | RWTH (academia/TSO research) | Devices, Controls & Codes | — |
| IEC 61400‑21‑1 | Electrical characteristics of wind turbines (PQ/FRT measurement & reporting) | 2019 | GE, Elia/50Hertz | Measurement & Metrics; Devices, Controls & Codes | — |
| NPCC Directory 1; CSA C235 | Regional criteria and voltage regulation (North America) | — | Hydro‑Québec/IREQ | Devices, Controls & Codes | — |

Legend for “Glossary section(s)” mapping: Phenomena & Events; Measurement & Metrics; Devices, Controls & Codes (see `study/glossary.md`).

Additional standards (local copies in `Standards/`) mapped to the same schema:

| Standard | What it does | Year | Used by (from base doc) | Glossary section(s) | Key ref (page/table) |
| --- | --- | --- | --- | --- | --- |
| IEC 61000‑3‑2 (NF_61000-3-2.pdf) | Limits for harmonic current emissions (equipment ≤16 A/phase) | 2018 (ed. per NF) | Alliander (DSO PQ), GE (OEM) | Devices, Controls & Codes | Table 1 Class A limits p. 22; Table 2 Class C p. 22; Table 3 Class D p. 23; Table 4 Observation period p. 23 |
| IEC 61000‑3‑3 (NF_61000-3-3 -.pdf) | Limitation of voltage changes and flicker (≤16 A/phase) | 2013/2019 (ed.) | Alliander (DSO PQ) | Phenomena & Events; Measurement & Metrics | Table 1 Méthodes d’évaluation p. 11 |
| IEC 61000‑3‑11 (NF_61000-3-11.pdf) | Limitation of voltage changes, flicker for equipment with high inrush | 2017 (ed.) | Alliander (DSO PQ) | Phenomena & Events; Devices, Controls & Codes | Table 1 Suffixes et utilisations p. 8 (see methods) |
| IEC 61000‑2‑4 (NF_61000-2-4.pdf) | Compatibility levels in industrial environments (LV systems) | 2002/2020 (ed.) | Swissgrid/TenneT (context), Alliander (industrial customers) | Phenomena & Events; Measurement & Metrics | Table 4 Harmonic compatibility (even ranks) p. 18; Table 5 THD compatibility p. 18; see Tables 1–3 adjacent |
| IEC 61000‑2‑2 (NF_61000-2-2.pdf) | Compatibility levels in public LV systems | — | Alliander (DSO PQ) | Phenomena & Events; Measurement & Metrics | Not text‑extractable from this PDF copy (scanned); use compatibility tables in Section 5/6 |
| IEC 61800‑3 (NF_61800-3.pdf) | EMC requirements for adjustable speed power drive systems (emission/immunity, categories C1–C4) | 2017 | GE, OEMs, industrial users | Devices, Controls & Codes | Immunity reqs Tables 1–2 p. 11–12; emission limits section (see Clause 6) |
| IEC 62116 (NF_62116.pdf) | Test procedure of islanding prevention for utility‑interactive PV inverters | 2014 | RWTH, GE, Alliander (DER) | Devices, Controls & Codes; Phenomena & Events | Table 1 Real‑time measured parameters p. 9; Table 2 Generator simulator spec p. 11; Table 5 Test conditions p. 14; Tables 6–7 Load unbalance p. 17–18 |
| IEC 62109‑1/‑2 (NF_62109-1/‑2.pdf) | Safety of power converters for use in PV power systems | 2010/2011 | GE, OEMs | Devices, Controls & Codes | Key safety distances/insulation tables (clearance/creepage) per sections; page varies by ed. |
| IEC 60664‑1 (iec_60664-1.pdf) | Insulation coordination for equipment within LV systems | 2020 | GE/OEMs, RWTH | Devices, Controls & Codes | Table 1 Rainure dimensionnement p. 50; see creepage/clearance clauses |
| IEC 60664‑4 (NF_60664-4.pdf) | Insulation coordination—specific applications, tests | — | OEMs | Devices, Controls & Codes | See clearance/creepage selection and test tables (ed. dependent) |
| IEC 61000‑6‑1/‑6‑2 (NF_61000-6-1/‑6-2.pdf) | Generic EMC immunity standards (residential/industrial) | — | All | Devices, Controls & Codes | 6‑2: Table 1 Immunity (enclosure) p. 11; Table 2 Immunity (signal/control) p. 12 |
| IEC 61000‑6‑3/‑6‑4 (NF_61000-6-3/‑6-4.pdf) | Generic EMC emission standards (residential/industrial) | — | All | Devices, Controls & Codes | Emission limit tables by environment/class; see Clause 7 (tables vary by ed.) |
| IEC 62368‑1 (NF_62368-1*.pdf) | Audio/video, ICT equipment—Safety requirements (hazard‑based) | 2018/2020 | OEMs | Devices, Controls & Codes | Hazard energy source tables in early clauses; edition dependent |
| IEC 60950‑1 (NF-60950-1.pdf) | IT equipment safety (superseded by 62368‑1) | 2005/2009 | Legacy OEMs | Devices, Controls & Codes | Clearance/creepage and test tables; edition dependent |
| IEC 62477‑1/‑2 (NF_62477-1/‑2.pdf) | Safety requirements for power electronic converter systems and equipment | 2012/2017 | OEMs/GE | Devices, Controls & Codes | Protective measures and test tables in Part 1; type tests in Part 2 |
| IEC 62909‑1/‑2 (NF_62909-1/‑2.pdf) | Bi‑directional grid‑connected converters—General requirements and test methods | 2017/2019 | GE/OEMs | Devices, Controls & Codes | Part 1 general reqs; Part 2 test method tables |
