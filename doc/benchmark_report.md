# ParaToric Benchmark Report — reproducing a published result

Purpose: the campaign's ANCHOR. A concrete, external, published number that our
code + settings must reproduce, so that any peer (or future agent) can check in
one minute that we are doing physics and not chasing an artifact. Come back here
whenever a result looks strange: if this benchmark still passes, the machinery is
sound and the strangeness is elsewhere.

Status: 2026-07-21. PROVISIONAL; exact-size L=10/L=20 progress, benchmark not
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
h=0.305629247317 and h=0.339573841268 are restored: the corrected blocking audit
gives cold/hot z=0.3283/0.4188 and suffix p=0.3280/0.2021. Tau remains unresolved
at h=0.339573841268 (blocking lower bound >=2157.001764 sweeps), but the mean and
its bootstrap error pass the validity gates. L=30 has not run. A finite-L L=10/L=20
crossing is not Linsel's extrapolated h_c.
Trend first, number second.

--------------------------------------------------------------------------------
## 4. The path: intermediate results -> fixes -> current result

Binder-cumulant crossings (pairwise, L=20/24/28). Spread = max-min across pairs.

| stage | crossings | spread | status |
|---|---|---|---|
| A. PT, broken mechanism, uniform ladder | drifts, no convergence | — | NO-GO |
| B. PT, mechanism FIXED, uniform dh=0.025 | 0.350(1) / 0.331(1) / 0.327(1) | 0.023 | NO-GO |
| C. PT, mechanism fixed + ladder RESOURCED | **0.3406 – 0.3437** | **0.003** | PROVISIONAL |
| D. Exact-size no-PT L=10/L=20, all nine L=20 points | **linear 0.3244097798; interpolation range 0.3237135711--0.3349290674** | **0.0112154963** | PROVISIONAL / FLAG |
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
fixes, but it was not promoted. Stage D restores both formerly excluded L=20
points. The local linear crossing is unchanged because its data bracket is
h=0.322--0.326, below either restored point. Interpolation family and fit window,
not pointwise Monte Carlo noise, now set the honest uncertainty.

--------------------------------------------------------------------------------
## 5. Current result and HONEST status

Curve errors below are hierarchical chain/block-bootstrap standard errors from
1,000 resamples (seed 20260720). Both formerly excluded L=20 means are restored;
their ratio means and errors were never invalidated by the tau-estimator defect.

| h | L=10 raw ratio +/- bootstrap error | L=20 raw ratio +/- bootstrap error |
|---:|---:|---:|
| 0.305629247317 | 1.620594488 +/- 0.065789675 | **1.399854822 +/- 0.000403214** |
| 0.318989179906 | 1.443357268 +/- 0.041930118 | 1.430742096 +/- 0.000596750 |
| 0.322000000000 | 1.489079406 +/- 0.021437993 | 1.440026145 +/- 0.000346187 |
| 0.326000000000 | 1.418993177 +/- 0.013435356 | 1.451363556 +/- 0.002472915 |
| 0.330615254621 | 1.463439077 +/- 0.007419584 | 1.466906462 +/- 0.005133156 |
| 0.335362847648 | 1.479757821 +/- 0.009856900 | 1.483203689 +/- 0.004081748 |
| 0.339573841268 | 1.510839219 +/- 0.006959165 | **1.579050185 +/- 0.002549444** |
| 0.343520981103 | 1.537532272 +/- 0.004333580 | 1.702697467 +/- 0.002274703 |
| 0.347398758564 | 1.583705271 +/- 0.004110110 | 1.804171955 +/- 0.001492930 |

The with/without comparison is direct:

| L=20 treatment | local linear crossing | bootstrap statistical error | interpolation range | full spread |
|---|---:|---:|---:|---:|
| all nine points | **0.3244097798** | 0.0008659348 | **0.3237135711--0.3349290674** | **0.0112154963** |
| omit h=0.305629247317 and 0.339573841268 | **0.3244097798** | 0.0008649342 | **0.3237135711--0.3349354739** | **0.0112219028** |

The systematic scan uses 18 choices: linear, PCHIP, natural cubic, and unweighted
degree-1/2/3 polynomial curves over all points, a window dropping only the lowest
h, and the central six points. Natural cubic interpolation has multiple roots;
all are retained in the quoted envelope. The narrower nearest-linear-root spread
with all points is 0.3237135711--0.3311873879, but publishing only that selected
branch would hide the spline ambiguity. The **full 0.0112154963 spread is the
actual crossing uncertainty**, not the 0.0008659348 bootstrap-only bar.

The old source TSV actually records 0.3244097798 +/- 0.0008170099. The
0.0000817 value repeated in narrative updates was a decimal-place transcription
error; neither statistical bar is publishable as the total uncertainty.

**Prediction check: FAILED.** Restoring h=0.339573841268 did not move the local
linear crossing upward; it stayed at 0.3244097798 because the crossing is bracketed
by h=0.322 and 0.326. But the honest interpolation envelope reaches Linsel's
0.335(2) band, so the data also do **not** support a several-sigma tension. The
nonmonotonic L=10 curve makes interpolation choice the dominant limitation.

**Status remains PROVISIONAL.** This pass restores the complete L=20 curve and
removes the false precision, but does not promote benchmark closure, establish a
dynamical exponent, or say anything about the MCP. L=30 remains unrun.

--------------------------------------------------------------------------------
## 6. The anchor plot

**Current exact-size figure:**
- x: h; y: raw ratio <P^4>/<P^2>^2 of plaquette percolation strength on a log
  scale, matching Linsel Fig. 4a's convention;
- L=10 and the restored nine-point L=20 curve with bootstrap error bars;
- both formerly excluded L=20 points marked as validated, with the old omitted
  curve shown dashed for direct comparison;
- the coincident with/without linear crossings, the full interpolation envelope,
  and Linsel's 0.335(2) band.

CURRENT PLOT PATH: `doc/figures/linsel_raw_ratio_crossing_no_pt_l10_l20_restored_20260721.png`

CURRENT TABLE PATH: `doc/figures/linsel_no_pt_l10_l20_restored_diagnostics_20260721.tsv`

ENTRYPOINT: registered `scripts/ensemble_diagnostics.py no-pt-benchmark`, seeded
with `20260720`; inputs are jobs `15149119` and `15159599` under their scratch
roots stated in sec. 3.

PRIOR PT RAW-RATIO PLOT (retained): `doc/figures/linsel_raw_ratio_crossings_pt_resourced_20260716.png`

SUPERSEDED U PLOT (retained): `doc/figures/linsel_binder_crossings_pt_resourced_20260716.png`

--------------------------------------------------------------------------------
## 7. How to re-check / what would change the verdict

- The two former L=20 void points are now cleared and restored. Re-run the
  registered `no-pt-benchmark` mode with the same seed, suffix rules, and
  `--sensitivity-exclude` pair to reproduce both curves and the interpolation
  envelope.
- Promotion requires a fresh physics read of the complete result and the existing
  "same trend, same crossing" bar; this edit does not promote it as a by-product.
- Before adding L=30, investigate the L=10 nonmonotonicity, suffix dependence,
  and finite-size corrections. The observable remains statically calibrated
  (doc/paratoric_run_manual.md G5).
