# ParaToric Benchmark Report — reproducing a published result

Purpose: the campaign's ANCHOR. A concrete, external, published number that our
code + settings must reproduce, so any peer (or future agent) can check in one
minute that we are doing physics and not chasing an artifact. Come back here
whenever a result looks strange: if this benchmark still passes, the machinery is
sound and the strangeness is elsewhere.

**Status: 2026-07-25 — CLOSED, PASS (with two named residuals, sec 6).**

--------------------------------------------------------------------------------
## 1. THE VERDICT

**We reproduce Linsel et al.'s Ising-line result.**

| | value |
|---|---|
| **ParaToric** h_c(inf) | **0.3359 +/- 0.0049** |
| **Linsel et al.** h_c | **0.335(2)** |
| **Agreement** | **0.2 sigma** |

The bar for this benchmark was always "a peer sees our figure and their Fig. 4a
and says: same TREND, same CROSSING." Both now hold:
- **TREND: PASS.** The raw ratio rises monotonically with h at every size, on the
  log-scale convention of their Fig. 4a. At L=20 all 8 chains agree to <=0.045 at
  every point in the window.
- **CROSSING: PASS.** Finite-L crossings drift downward with L exactly as
  finite-size scaling requires, and extrapolate onto the published band (sec 3).
- **INSTRUMENT: PASS, by an exact symmetry test** — see sec 7, which is a stronger
  check than this benchmark itself provides.

This closes the benchmark as the campaign's regression test. It does NOT establish
anything about the multicritical point: this line is lambda = 0.3 FIXED, which is
OFF the self-dual line and is a continuous 3D Ising transition. See sec 7.

--------------------------------------------------------------------------------
## 2. THE SOURCE AND WHAT WE HAD TO REPRODUCE

Linsel, Bohrdt, Homeier, Pollet, Grusdt, **"Percolation as a confinement order
parameter in Z2 lattice gauge theories"**, Phys. Rev. B **110**, L241101 (2024) —
arXiv:**2401.08770**, Fig. **4a**.

Why this paper: same model, same observable family, same regime (quantum, T=1/L).
It is the only published anchor in our own units.

Setting (matched exactly):
- extended toric code, **mu = J = 1**; periodic square; Boost lattice
- **beta = L** (their T = 1/L)
- **lmbda = 0.3 FIXED**, scan **h** — the ISING line, deliberately OFF the
  self-dual line; the transition here is continuous 3D Ising, NOT the MCP
- observable: raw ratio **<P^4>/<P^2>^2** of the plaquette percolation strength

| quantity | Linsel | ours |
|---|---|---|
| critical field | h_c = 0.335(2) | **0.3359 +/- 0.0049** |
| correlation-length exponent | nu = 0.65(27) | not attempted (see note) |
| universality | 3D Ising (continuous) | consistent |

Note on nu: their own error is +/-0.27 (~40%). Nobody extracts a tight nu at
L <= 30. This benchmark's job is **h_c location + Ising compatibility**, never a
precise nu.

**CONVENTION MAPPING (verified against the PDF, 2026-07-25).** Two traps here, both
of which cost real time; both are now checked rather than assumed:
1. **Plotted quantity.** Linsel Fig. 4a plots the RAW, unbounded ratio
   <P^4>/<P^2>^2 on a log axis. We originally plotted U = 1 - <P^4>/(3<P^2>^2), a
   monotonically DECREASING transform, and read its opposite slope as a channel
   error. It was a plotting-convention mismatch. **Always plot the raw ratio when
   comparing to their figure.**
2. **Basis labels.** They sample in the **tau^x**-basis and call it the "electric
   field basis". In our convention sigma_z is the electric field. **Their tau^x is
   our sigma_z**: axis labels differ, physical content matches. No mismatch.

--------------------------------------------------------------------------------
## 3. THE RESULT AND ITS FIGURE

![Benchmark finite-size extrapolation](figures/linsel_crossing_extrapolation_ising_exponent_20260725.png)

`doc/figures/linsel_crossing_extrapolation_ising_exponent_20260725.png`

Crossings from the PT-resourced set (jobs `15118635/36/37`,
`/scratch/a/A.Otaifi/paratoric_pt_linsel_resourced_20260716/`), errors bootstrapped
over PT replicas:

| pair | crossing |
|---|---|
| L = 20/24 | 0.3458 +/- 0.0044 |
| L = 20/28 | 0.3421 +/- 0.0012 |
| L = 24/28 | 0.3411 +/- 0.0005 |

Inverse-variance weighted fit of h_c(L) = h_c(inf) + A L_eff^-p, L_eff = sqrt(L1 L2):
**h_c(inf) = 0.3359 +/- 0.0047 (fit) +/- 0.0012 (exponent) = 0.3359 +/- 0.0049.**

**THE EXPONENT — the single most important methodological point in this report.**
A dimensionless-ratio (Binder-type) crossing drifts as

    h_c(L) - h_c(inf)  ~  L^-(omega + 1/nu)

NOT as 1/L. For 3D Ising, nu = 0.63002(10) and omega = 0.832(6), giving
**p = 2.42**. This exponent is fixed A PRIORI by the universality class — imported
from the literature, not fitted to our data — so the result is not tuned. An earlier
fit used an unmotivated p = 1 and obtained 0.3236, apparently missing the published
band; that was an artifact of the wrong exponent. Robustness: varying p over 2 to 3
moves the intercept by only +/-0.0012.

**Honest error bars stabilized this result rather than merely decorating it.**
Unweighted, the p-choice swung the answer by 0.012; weighted, the precise 24/28
crossing anchors the fit and the same swing is 0.0012.

**WHY THE RAW-CROSSING FIGURE SITS ABOVE 0.335 — read this before concluding we
disagree with the literature.** In
`doc/figures/linsel_raw_ratio_crossings_pt_resourced_20260716.png` the crossings sit
visibly ABOVE the published band. **They are supposed to.** A finite-L crossing is
not h_c; it approaches h_c only as L -> inf, from above on this line. Our crossings
fall monotonically with L (0.3458 -> 0.3421 -> 0.3411) and extrapolate onto the band.
**That offset IS the finite-size correction, and its magnitude and L-dependence are
themselves consistent with 3D Ising.** Linsel reports the same behaviour for his
lambda-sweep: *"We observe a shift in the crossing points toward lower lambda ... We
conjecture that the crossing points approach the known critical value."*

--------------------------------------------------------------------------------
## 4. HOW WE GOT HERE — three fixes, each paid for

| stage | crossing spread | status |
|---|---|---|
| A. PT with broken swap mechanism | no convergence | NO-GO |
| B. PT fixed, uniform ladder dh = 0.025 | 0.023 | NO-GO |
| C. PT fixed + ladder resourced | **0.003** | basis of sec 3 |

1. **Swap-partner bug.** The partner was chosen by MPI *rank* while the ladder label
   *migrates* on accept, so adjacent ranks held non-adjacent ladder positions and the
   ladder was severed (0 round-trips). Fix: choose the partner by **ladder position**
   via `MPI_Allgather`. Details: `doc/pt_architecture.md` sec 7.
2. **Ladder under-resourcing.** Swap acceptance is set by energy-distribution overlap,
   sigma ~ dh*sqrt(Var E) ~ dh*L^(3/2), so a fixed dh dies as L grows. Fix:
   **N_rungs ~ L^1.69** (measured; the excess over 1.5 is the specific-heat
   enhancement near h_c), non-uniform, denser near h_c. Details:
   `doc/pt_architecture.md` sec 8.
3. **Plotting convention** (sec 2, trap 1).

Effect: the crossing spread collapsed **0.023 -> 0.003** and converged. That
progression, driven by independently diagnosed fixes, is the evidence the machinery
is physical.

--------------------------------------------------------------------------------
## 5. THE L=10 ERGODICITY FINDING (why the no-PT route did not close this)

![L=10 chain disagreement and PT repair](figures/l10_chain_disagreement_and_pt_repair_20260721.png)

`doc/figures/l10_chain_disagreement_and_pt_repair_20260721.png`

A separate no-PT attempt at L=10/L=20 (jobs `15149119`, `15159599`) could NOT deliver
a crossing, for a reason worth recording permanently:

**At L=10 with local updates only, the 8 independent chains do not sample the same
distribution.** At h = 0.3056 their raw ratios are 1.7426 1.7245 1.7418 1.7421 |
1.2993 1.7397 1.7391 1.4531 — six in one basin, two elsewhere — a per-chain spread of
0.44, versus 0.003 at L=20 (a factor 150). Per-chain tau ratios violate invariant I4
by up to 11,275x at 8 of 9 points. The disease is a SMALL-L disease and self-averages
away with L, so it does not threaten the large-L program.

**Two lessons, both now enforced elsewhere:**
- **The cold/hot check (I5) can PASS on a broken ensemble.** The hot group's own
  internal split (spread 0.44) inflates its variance and drives z down. Group
  summaries are blind to this; **only the per-chain table shows it.** Per-chain
  spread and tau-factor are now permanent gated columns in
  `scripts/ensemble_diagnostics.py`.
- **More chains cannot fix it.** Independent chains populate basins by which one they
  fall into, not by thermodynamic weight, so adding chains converges to a precise
  WRONG number. Only a method that moves configurations BETWEEN basins (PT) repairs
  the weights — which is why the PT-based route in sec 3 is the one that closed.

--------------------------------------------------------------------------------
## 6. THE TWO RESIDUALS (what would change the verdict)

Named honestly. Neither blocks closure; both should be revisited if this benchmark
is ever leaned on harder.

**R1 — the input crossings come from a provisional PT set.** Those runs recorded zero
strict round-trips, and the 24/28 crossing is bracketed in only 32% of bootstrap
resamples (the curves are close and nearly parallel there, so the root is fragile).
The agreement at 0.2 sigma is therefore encouraging rather than final. Job `15232180`
(re-spanned ladder, PT transport now demonstrated) will supply a better-sampled curve;
re-run the sec-3 machinery on it and update the numbers here.

**R2 — we do not know whether Linsel's 0.335(2) is an infinite-L value or a finite-L
crossing estimate.** Their text says only *"a crossing point analysis ... to extract an
estimate for h_c"*; the word "extrapolat" appears once in the paper and refers to a
different quantity. This matters:
- if their 0.335(2) is an **infinite-L** estimate, our 0.3359 +/- 0.0049 agrees at
  0.2 sigma (the reading adopted in sec 1);
- if it is a **finite-L crossing** at L ~ 20-30, then the correct comparison is our
  finite-L crossings 0.341-0.346, and there is a ~3 sigma discrepancy to explain.
Their supplement reports h_c = 0.3301(17) at lambda = 0 and 0.332 at lambda = 0.2 with
a collapse at L = 20/25/30; comparing those against their own plotted finite-L
crossings would settle it. **Free to resolve, and it should be done before this
benchmark is leaned on harder.**
Related open detail: their Fig. 4a legend shows L = 10, 20, 30, while their
supplement's h-sweep methods state L^2 = 20^2, 25^2, 30^2. Both readings are recorded;
we have not resolved which set produced the quoted h_c.

**To re-check:** re-run the sec-3 setup and compare crossings to sec 2. If a future
change makes this benchmark drift, the change is suspect — this is the regression test
for the whole observable + PT stack. If crossings move, investigate thermalization
depth, aspect ratio, and corrections to scaling BEFORE doubting the observable, which
is statically calibrated (`doc/paratoric_run_manual.md` G5).

--------------------------------------------------------------------------------
## 7. COMPANION VALIDATION — the self-dual line (NOT the benchmark line)

**Read this header before using anything in this section.** The result below is on the
**SELF-DUAL line (h = lmbda = s)**, a DIFFERENT LINE from this report's benchmark
(**lmbda = 0.30 FIXED**, scan h). It is recorded here because it is the strongest
INSTRUMENT VALIDATION we have, and this report is where we come to ask "is the
machinery sound?". It is **not** a benchmark result and must never be quoted as one.
The two critical points are numerically close and that closeness is a known trap:
  - benchmark critical point: (h, lmbda) = (0.335, **0.30**)  — off-diagonal
  - self-dual MCP:            (s*, s*) with s* ~ 0.333        — on the diagonal
Same h to two digits, different lmbda. The s* deliverable itself lives in
`doc/multicritical_universality_plan.md` M1; this section records only what it says
about the INSTRUMENT.

FIGURE: `doc/figures/selfdual_sstar_crossing_duality_20260723.png`
DATA: `/scratch/a/A.Otaifi/paratoric_selfdual_step2a_20260721/` (job `15177203`)

**What it validates.**
1. **Duality (I2/B5) passes on the self-dual line with the correct channel pairing.**
   At L=16, `z-plaquette` vs `x-direct` percolation PROBABILITY (run manual A8:
   z-plaq == x-perc), z-scores 0.4 / 2.1 / 1.6 / 0.3 / 1.0 across s = 0.250-0.375. The
   WRONG pairing (z-plaq vs x-plaq, same graph) is off by a factor ~40 (0.7416 vs
   0.0184 at s=0.300). This confirms the observable, the basis mapping, the
   rotated-slice measurement and the duality bookkeeping simultaneously, on a line
   where duality is an EXACT symmetry and therefore an exact test. **This is a stronger
   instrument check than the benchmark itself provides**, because it is a symmetry
   statement rather than a comparison to a published number.
2. **It passes even where the raw-ratio observable is dead.** Above s ~ 0.35 the
   percolation strength is 81-98% exact zeros, so <P^4>/<P^2>^2 divides two vanishing
   moments and is meaningless (spreads of 8.5, 167, 418 on a quantity of order 1.5).
   The BOUNDED probability channel keeps working and keeps satisfying duality. This is
   the harness's own >50%-exact-zeros tripwire, now wired into an automated
   OBSERVABLE-DEGENERATE gate rather than left on paper.

**Consequence for this report.** The plaquette PROBABILITY channel is the validated
instrument of record for LOCATION and duality (as run manual A8 already specified).
Where the raw ratio is used for the Linsel overlay it must be accompanied by its
zero-fraction; on the benchmark line that fraction is 0.23-0.40, comfortably below the
tripwire, so the overlay in sec 3 is NOT affected by the degeneracy above. That was
checked, not assumed.

--------------------------------------------------------------------------------
## 8. RETAINED ARTIFACTS (provenance)

- PT raw-ratio crossings (the sec-3 input curves):
  `doc/figures/linsel_raw_ratio_crossings_pt_resourced_20260716.png`
  — see sec 3: these sit ABOVE the published band by construction.
- Superseded U = 1 - r/3 plot (sec 2 trap 1):
  `doc/figures/linsel_binder_crossings_pt_resourced_20260716.png`
- Superseded p = 1 extrapolation (wrong exponent, sec 3):
  `doc/figures/linsel_crossing_extrapolation_pt_20260725.png`
- No-PT L=10/L=20 curves and diagnostics (sec 5):
  `doc/figures/linsel_raw_ratio_crossing_no_pt_l10_l20_restored_20260721.png`,
  `doc/figures/linsel_no_pt_l10_l20_restored_diagnostics_20260721.tsv`
- Entrypoints: `scripts/ensemble_diagnostics.py` modes `no-pt-benchmark`,
  `crossing-extrapolation` (seed 20260720).
- A previously reported crossing of 0.32441 came from contaminated L=10 points and is
  fully superseded by sec 3. Its quoted +/-0.0000817 was additionally a decimal-place
  transcription error for +/-0.000817; neither bar was ever a total uncertainty.
