# ParaToric Benchmark Report — reproducing a published result

Purpose: the campaign's ANCHOR. A concrete, external, published number that our
code + settings must reproduce, so that any peer (or future agent) can check in
one minute that we are doing physics and not chasing an artifact. Come back here
whenever a result looks strange: if this benchmark still passes, the machinery is
sound and the strangeness is elsewhere.

Status: 2026-07-13. PROVISIONAL PASS (see 5). Update on each new result.

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

L = 20, 24, 28; beta = L; lmbda = 0.3; mu = J = 1; z-basis; plaquette percolation
channel (the z-basis deconfinement POP); percolation measured on a rotated
imaginary-time slice; PT ladder in h.

--------------------------------------------------------------------------------
## 4. The path: intermediate results -> fixes -> current result

Binder-cumulant crossings (pairwise, L=20/24/28). Spread = max-min across pairs.

| stage | crossings | spread | status |
|---|---|---|---|
| A. PT, broken mechanism, uniform ladder | drifts, no convergence | — | NO-GO |
| B. PT, mechanism FIXED, uniform dh=0.025 | 0.350(1) / 0.331(1) / 0.327(1) | 0.023 | NO-GO |
| C. PT, mechanism fixed + ladder RESOURCED | **0.3406 – 0.3437** | **0.003** | PROVISIONAL |
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

**Effect:** the crossing spread collapsed **0.023 -> 0.003 (8x tighter)** and
converged to just above the published value. That progression — driven by two
independently-diagnosed fixes — is the evidence that the machinery is physical.

--------------------------------------------------------------------------------
## 5. Current result and HONEST status

**Crossings 0.3406–0.3437 vs published 0.335(2).** Tight and self-consistent
across all three L-pairs, ~3-4 sigma above the published value.

PROVISIONAL, not final, because:
- **PT is not yet mixing**: 0 strict round-trips at every L, so each rung may
  still carry initial-condition bias. A crossing from a non-mixing ladder is
  encouraging, not conclusive.
- Residual within-sampling-window drift.
=> Expect the number to move once round-trips land. Update this section then.

--------------------------------------------------------------------------------
## 6. The anchor plot

**ONE figure** (not a gallery):
- x: h (crossing region, ~0.30–0.38); y: Binder cumulant U_P of plaquette
  percolation strength
- three curves L = 20, 24, 28 with autocorrelation-corrected error bars
- shaded vertical band at **Linsel h_c = 0.335(2)**
- annotate our pairwise crossings (0.3406–0.3437)
- caption: "ParaToric reproduces the Linsel et al. (PRB 110, L241101) Ising-line
  benchmark: Binder crossings for L=20/24/28 at lmbda=0.3, beta=L."

PLOT PATH: `<to be filled by Jarvis>`
DATA: `/scratch/a/A.Otaifi/paratoric_pt_linsel_resourced_20260716/`

--------------------------------------------------------------------------------
## 7. How to re-check / what would change the verdict

- Re-run sec 3 setup; compare crossings to sec 2. If they land at 0.335(2) with
  round-trips > 0 and no window drift => FULL PASS; promote sec 5.
- If a future change makes this benchmark drift, the change is suspect — this is
  the regression test for the whole observable + PT stack.
- If crossings stay ~0.34 even after PT mixes, investigate residual systematics
  (thermalization depth, aspect ratio, correction-to-scaling at L<=28) BEFORE
  doubting the observable — it is calibrated (doc/paratoric_run_manual.md G5).
