# Event Card: Frequency Excursions and ROCOF

- Definition: System frequency deviates from nominal due to active power imbalance; ROCOF is df/dt (Hz/s).
- Why it matters: Protection/security (UFLS), inertia adequacy, IBR ride‑through; high ROCOF can desynchronize PLLs and cause nuisance trips.
- Typical causes: Loss of generation/load, HVDC trips, islanding, insufficient inertia.
- Signals/metrics: Frequency (Hz), ROCOF (Hz/s), nadir (Hz), overshoot, recovery time, inertia estimate (MW·s/MVA·s), primary response (MW/Hz).
- Standards/policy: SOGL frequency quality targets; ENTSO‑E inertia guidance; IEEE 1547/2800 ride‑through; TSO‑specific ROCOF alarm/trip settings.

Measurement & Analysis

- PMU‑based df/dt with filtering; window trade‑off between noise and latency. Track nadir/settling and estimate inertia from early ROCOF.

Mitigation

- System: Synchronous inertia commitment, fast frequency response, UFLS design, HVDC support.
- IBR: Grid‑forming controls, fast active power support, adaptive PLL/ROCOF ride‑through.

Demo idea (lab/booth)

- Simulate a 1 GW trip in a low‑inertia system; compare frequency nadir and ROCOF with/without fast frequency response or GFM inverters.

