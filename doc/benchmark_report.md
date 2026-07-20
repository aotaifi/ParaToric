# ParaToric Benchmark Report — reproducing a published result

Purpose: the campaign's ANCHOR. A concrete, external, published number that our
code + settings must reproduce, so that any peer (or future agent) can check in
one minute that we are doing physics and not chasing an artifact. Come back here
whenever a result looks strange: if this benchmark still passes, the machinery is
sound and the strangeness is elsewhere.

Status: 2026-07-20. PROVISIONAL; exact-size L=10/L=20 progress, benchmark not
closed (see 5). Update on each new result.

--------------------------------------------------------------------------------
## 1. The benchmark source (the paper)

Linsel, Bohrdt, Homeier, Pollet, Grusdt,
**"Percolation as a confinement order parameter in Z2 lattice gauge theories"**,
Phys. Rev. B **110**, L241101 (2024) — arXiv:**2401.08770**.
Specifically their **Fig. 4a** (QMC of the extended toric code).

Why this paper: it is the SAME model and the SAME observable family as ParaToric
(the percolation-inspired order parameters our code implements), run in the SAME
regime (quantum, T=1/L). It is the only published anchor in our own units.

--------------------------------------------------------------------------------
## 2. What we must reproduce (CONCRETE numbers we expect)

Setting (must match exactly):
- Model: extended toric code, **mu = J = 1**
- Basis: z; Boost lattice; periodic square
- **beta = L** (their T = 1/L)
- **lmbda = 0.3 FIXED**, scan **h** across the transition
  (NOTE: this is the ISING line — deliberately OFF the self-dual line.
   The transition here is CONTINUOUS 3D Ising, NOT the first-order MCP.)
- Observable: **Binder cumulant of the (plaquette) percolation strength**
- Their sizes: L = 10, 20, 30

Expected values:
| quantity | Linsel value |
|---|---|
| critical field | **h_c = 0.335(2)** |
| correlation-length exponent | **nu = 0.65(27)**  (3D Ising) |
| universality | 3D Ising (continuous) |

Note on nu: Linsel's own error is +/-0.27 (~40%). Do NOT demand a tight nu from
L<=32 — nobody gets one. The benchmark's job is **h_c location + Ising
compatibility**, not a precise nu.

--------------------------------------------------------------------------------
## 3. Our setup

Current anchor: L=10 and L=20, beta=L, lmbda=0.3, mu=J=1, z-basis, no PT,
plaquette percolation strength on rotated imaginary-time slices, eight independent
chains (four cold plus four hot) per h. These are two of Linsel's three exact
sizes. Job `15149119` ran to 40M sweeps at L=10; job `15159599` ran to 64M at
L=20, both sampled every 25 sweeps.

Analysis uses only stationary suffixes: after 16M sweeps at L=10; after 8M at
L=20, except h=0.318989179906 after its validated 48M plateau entry. L=20
h=0.305629247317 and h=0.339573841268 have no nontrivial stationary suffix and
are explicitly excluded, not silently promoted. L=30 has not run. A finite-L
L=10/L=20 crossing is not Linsel's extrapolated h_c.
Trend first, number second.

--------------------------------------------------------------------------------
## 4. The path: intermediate results -> fixes -> current result

Binder-cumulant crossings (pairwise, L=20/24/28). Spread = max-min across pairs.

| stage | crossings | spread | status |
|---|---|---|---|
| A. PT, broken mechanism, uniform ladder | drifts, no convergence | — | NO-GO |
| B. PT, mechanism FIXED, uniform dh=0.025 | 0.350(1) / 0.331(1) / 0.327(1) | 0.023 | NO-GO |
| C. PT, mechanism fixed + ladder RESOURCED | **0.3406 – 0.3437** | **0.003** | PROVISIONAL |
| D. Exact-size no-PT L=10/L=20, stationary suffixes | **formal crossing 0.32441(82)** | one pair | PROVISIONAL / FLAG |
| target (Linsel) | **0.335(2)** | — | — |

**The two fixes (short):**
1. **Swap-partner bug.** Partner was chosen by MPI *rank* while the ladder label
   *migrates* on accept => adjacent ranks held non-adjacent ladder positions =>
   the ladder severed => 0 round-trips. Fix: choose the partner by **ladder
   position** via MPI_Allgather (`rank_of_ladder_index[i+/-1]`), ported from
   `code_ruby_duy_pinning_lab/worm.update.cpp`. Details: pt_architecture.md sec 7.
2. **Ladder under-resourcing.** Acceptance is set by energy-distribution overlap:
   sigma ~ dh*sqrt(Var E) ~ dh*L^(3/2), so a fixed dh dies as L grows (L28: 14/14
   links dead). Fix: **N_rungs ~ L^(3/2)**, non-uniform, denser near h_c
   (N = 40/48/60 for L = 20/24/28). Details: pt_architecture.md sec 8.
   Calibration measured the exponent as **1.69** (vs 1.5 from pure volume; the
   excess is the specific-heat enhancement L^(alpha/nu) near h_c) — use 1.69.

Stage C's PT crossing spread collapsed 0.023 -> 0.003 after the two mechanism
fixes, but zero strict round-trips prevented closure. Stage D is the requested
existing-data exact-size deliverable. It does not confirm Stage C: the L=10 curve
is nonmonotonic at low h, two L=20 points are void, and the only central linear
sign-change is far below the published h_c. This negative result is retained
rather than selecting a visually preferred near-h_c intersection.

--------------------------------------------------------------------------------
## 5. Current result and HONEST status

The raw-ratio autocorrelation time is defined from the delta-method influence
series of r=<P^4>/<P^2>^2; it is not the tau of P itself. Values below are means
of eight independent-chain tau estimates in sweeps, with delete-one-chain
jackknife errors. Curve errors use a hierarchical bootstrap that resamples
chains, then blocks no shorter than 2*tau. Full values, ratio means/errors,
retained sample counts, and chain-tau ranges are in
`doc/figures/linsel_no_pt_l10_l20_diagnostics_20260720.tsv`.

| h | L=10 tau(raw ratio) | L=10 tau(base density) | L=20 tau(raw ratio) | L=20 tau(base density) |
|---:|---:|---:|---:|---:|
| 0.305629247317 | 9092 +/- 9071 | 20.631 +/- 0.614 | VOID | VOID |
| 0.318989179906 | 33667 +/- 20154 | 19.108 +/- 0.370 | 18.402 +/- 0.243 | 20.961 +/- 0.088 |
| 0.322000000000 | 71086 +/- 12650 | 19.026 +/- 0.265 | 18.512 +/- 0.121 | 20.929 +/- 0.102 |
| 0.326000000000 | 35043 +/- 12949 | 18.519 +/- 0.139 | 18.763 +/- 0.155 | 20.767 +/- 0.136 |
| 0.330615254621 | 22411 +/- 2888 | 18.710 +/- 0.095 | 18.658 +/- 0.422 | 20.680 +/- 0.117 |
| **0.335362847648** | **11818 +/- 2312** | **18.760 +/- 0.086** | **58.88 +/- 17.78** | **21.475 +/- 0.227** |
| 0.339573841268 | 3971 +/- 377 | 19.214 +/- 0.207 | VOID | VOID |
| 0.343520981103 | 1946 +/- 328 | 20.023 +/- 0.433 | 243.6 +/- 34.5 | 25.44 +/- 1.56 |
| 0.347398758564 | 846.3 +/- 55.7 | 23.486 +/- 0.722 | 26.85 +/- 2.78 | 20.696 +/- 0.240 |

At h=0.335362847648:
- raw-ratio tau20/tau10 = **0.004983 +/- 0.001792**, apparent
  z=log2(ratio) = **-7.649 +/- 0.519**;
- base-density tau20/tau10 = **1.14472 +/- 0.01319**, apparent
  z = **0.19499 +/- 0.01662**.

Neither independently measured tau ratio points toward steeper-than-z=2 slowing.
They disagree sharply with the sector-exchange-rate result: 233,758 events/40M
sweeps at L=10 versus 28,421/64M at L=20 gives slowdown **13.2** and apparent
exponent **about 3.7**. These are not the same observable. Sector exchange counts
threshold crossings of the direct percolation indicator, whose own distribution
changes with L; the rate is therefore not a clean tau_int. Raw-ratio tau instead
uses the plaquette-strength ratio estimator's influence series, while density is
local. The disagreement must not be converted into a dynamical-exponent claim.

The formal L=10/L=20 linear-interpolation crossing is
**h_x=0.32441 +/- 0.00082** (99.9% of hierarchical-bootstrap replicates retain a
crossing in the search window). It lies 0.01059 below Linsel's 0.335(2), about
4.9 combined sigma. This is not a successful reproduction: the L=10 curve is
nonmonotonic near the formal bracket, and the missing L=20 h=0.339574 point blocks
a trusted near-h_c bracket. It is quoted because it is the algorithmic pairwise
crossing, not because it is physically preferred.

**Status remains PROVISIONAL.** This establishes an exact-size, reproducible
negative readout from all presently usable existing data and rules out claiming
benchmark closure from the L=10/L=20 overlay. It does not establish the published
h_c, an Ising-line dynamical exponent, the L=20 full curve, or any MCP property.
L=20's two targeted points must stabilize and L=30 remains unrun.

--------------------------------------------------------------------------------
## 6. The anchor plot

**Current exact-size figure:**
- x: h; y: raw ratio <P^4>/<P^2>^2 of plaquette percolation strength on a log
  scale, matching Linsel Fig. 4a's convention;
- L=10 and L=20 with hierarchical chain/block-bootstrap error bars;
- formal pairwise crossing with its bootstrap x error and Linsel's 0.335(2) band;
- L=20 h=0.305629 and 0.339574 explicitly omitted; the L=20 line is broken at
  the missing h=0.339574 point so the plot does not interpolate through void data.

CURRENT PLOT PATH: `doc/figures/linsel_raw_ratio_crossing_no_pt_l10_l20_20260720.png`

CURRENT TABLE PATH: `doc/figures/linsel_no_pt_l10_l20_diagnostics_20260720.tsv`

ENTRYPOINT: registered `scripts/ensemble_diagnostics.py no-pt-benchmark`, seeded
with `20260720`; inputs are jobs `15149119` and `15159599` under their scratch
roots stated in sec. 3.

PRIOR PT RAW-RATIO PLOT (retained): `doc/figures/linsel_raw_ratio_crossings_pt_resourced_20260716.png`

SUPERSEDED U PLOT (retained): `doc/figures/linsel_binder_crossings_pt_resourced_20260716.png`

--------------------------------------------------------------------------------
## 7. How to re-check / what would change the verdict

- First clear the two L=20 void points on the prescribed geometric ladder. Only
  after both have a nontrivial stationary suffix may they enter the registered
  no-PT analysis and reconnect the plotted L=20 curve.
- Re-run `scripts/ensemble_diagnostics.py no-pt-benchmark` with the same seed and
  suffix rules. FULL PASS requires a stable near-h_c crossing compatible with
  0.335(2), not merely a formal sign change on a nonmonotonic segment.
- If the exact-size crossing remains incompatible after the L=20 void clears,
  investigate the L=10 nonmonotonicity, suffix dependence, and finite-size
  corrections before adding L=30. The observable itself remains statically
  calibrated (doc/paratoric_run_manual.md G5).
