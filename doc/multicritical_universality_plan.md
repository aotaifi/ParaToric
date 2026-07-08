# Plan: Universality Class of the Toric-Code Multicritical Point

Date: 2026-07-07. Owner context: ParaToric review (commit d085509).
Goal: decide the universality class of the multicritical point where the two
transition lines of the toric code in fields meet (self-dual point of the
(2+1)D Z2 gauge-Higgs model), via large-L FSS with beta=L.

Verdict from code review: feasible ONLY after Markov-chain upgrades (PT +
checkpointing + ensembles). Backend/CPU work is capped at ~2x and is not on
the critical path. Worst observable: percolation_strength (tau 2k-10k).

## Competing scenarios to discriminate

1. First-order point (weakly first-order plausible).
2. Multicritical XY* / enhanced symmetry (Bonati-Pelissetto-Vicari prediction).
3. New self-dual universality class (Somoza-Serna-Nahum scenario, nu ~ 0.7).

## Milestones

### M0 — Infrastructure + go/no-go pilot (blocking everything)
- Add lattice-state checkpoint/restart (no serialization exists today).
- Add parallel tempering in (h, mu) or along the self-dual line; swap weights
  are O(1) from cached totals (kink counts + integrated potentials). MPI hook
  `PARATORIC_LINK_MPI` already in CMake.
- Ensemble driver: K independent seeds per point; cross-chain tau/R-hat check.
- Optional cheap move: non-contractible winding-loop flip over [0,beta]
  (compose cached O(1) global-single-flip diffs along a straight loop; O(L)/attempt).
- PILOT: PT ladder through the multicritical region at L=120. GO if worst-tau
  drops >=5x; else re-scope (winding move / worm) before any production.
- Sanity A/B: one run with -ffast-math OFF (default is ON), same seed, confirm
  statistically identical physics before multi-week production.

### M1 — Locate the multicritical point
- Duality (x<->z basis) pins the point to the self-dual line: 1-parameter scan
  along it for where the single transition line splits / two lines meet.
- Observables: percolation_probability crossings + Binder cumulants from both
  dual arms; duality consistency as a systematic check.
- Deliverable: (h*, lambda*) with error bar, L up to ~96.

### M2 — FSS ladder at the multicritical point
- Sizes: L in {32, 48, 64, 96, 128, 192} (extend to 256 only if M3 ambiguous),
  beta = L, several points bracketing (h*, lambda*).
- Bar: Neff = N/(2 tau) >= 30 for percolation_strength at every (L, point);
  require N >= 100*tau so tau itself is trustworthy; >=8 chains/point.
- Measure equal-time observables at several rotated time slices per sample
  (variance reduction, nearly free).

### M3 — Exponent extraction + scenario discrimination
- nu: from d(crossing observable)/d(param) scaling at crossings.
- beta/nu: percolation_strength (order-parameter-like) at criticality.
- gamma/nu: susceptibilities.
- First-order test: energy-histogram double peak + Lee-Kosterlitz FSS of the
  barrier; Binder minimum scaling.
- Scenario calls: XY* -> exponents match 3D XY with symmetry-enhanced scaling;
  new class -> stable non-XY exponents; first-order -> barrier grows with L^(d-1).

### M4 — Paper
- Decisive continuous result or clean first-order proof: PRL/PRX target.
- Ambiguous exponents: PRB with phase diagram + method (percolation observables
  + PT for gauge-Higgs) as the backbone.

## Observables (all already in ParaToric)

| Observable | Role |
|---|---|
| percolation_probability / _strength (+ plaquette duals) | order parameter, crossings, beta/nu; worst tau -> sets the sampling bar |
| Binder cumulants (auto-output) | crossings, first-order diagnostic |
| sigma_x/z static susceptibilities | gamma/nu |
| energy (+ full time series) | histograms, Lee-Kosterlitz first-order test |
| fredenhagen_marcu | confinement cross-check |
| anyon_count | topological-phase diagnostic |
| dual pairs (x vs z basis) | self-duality consistency / variance halving |

Companion to add (cheap): record largest_cluster unconditionally, not gated on
percolating — better-behaved slow-mode series than percolation_strength.

## Literature (refs verified via Crossref/arXiv 2026-07-07)

- Fradkin & Shenker, PRD 19, 3682 (1979) — Z2 gauge-Higgs phase diagram.
- Tupitsyn, Kitaev, Prokof'ev, Stamp, PRB 82, 085114 (2010) — topological
  multicritical point of toric code / 3D lattice gauge-Higgs model, QMC.
- Somoza, Serna, Nahum, PRX 11, 041008 (2021) — self-dual criticality in 3D
  Z2 gauge theory with matter; candidate new-class scenario.
- Bonati, Pelissetto, Vicari, PRB 105, 165138 (2022) (arXiv:2112.01824) —
  multicritical point of 3D Z2 gauge-Higgs; multicritical XY scenario. KEY
  COMPETING CLAIM: they argue one line at the MCP is FIRST-order + two Ising
  lines; check consistency of our setup with their geometry.
- Bonati, Pelissetto, Vicari, Phys. Rep. (2025) (arXiv:2410.05823) — review
  of 3D Abelian/non-Abelian gauge-Higgs criticality.
- Vidal, Dusuel, Schmidt, PRB 79, 033109 (2009) — low-energy effective theory,
  parallel field; Vidal, Thomale, Schmidt, Dusuel, PRB 80, 081104 (2009) —
  self-duality of toric code in transverse field.
- Linsel, Bohrdt, Homeier, Pollet, Grusdt, PRB 110, L241101 (2024) —
  percolation as confinement order parameter in Z2 LGTs (basis of the
  percolation observables in this code).
- Lee & Kosterlitz, PRL 65, 137 (1990) — first-order FSS.
- Hukushima & Nemoto, JPSJ 65, 1604 (1996) — parallel tempering.

## Budget sketch

- Engineering (M0): 2-4 weeks.
- Compute: O(10^5) core-hours across M1-M3 (dominated by L>=128 ladder).
- Kill criteria: M0 pilot fails (<2x tau gain after PT+winding move), or M3
  shows drifting effective exponents at L=192 with no first-order signal ->
  publish PRB-scope result, do not chase L=256+ without a new algorithm.
