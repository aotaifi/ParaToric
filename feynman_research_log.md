# Feynman research log — ParaToric

The durable reasoning record. What we concluded, why, what was refuted, and what a
correction cost. A fresh Feynman reads this to inherit judgement, then the channel
(`doc/feynman_thesis_tojarvis.md`) for the live instruction.

Two parts: **standing facts** (kept current — these are the things not to re-derive
or re-break) and the **ledger** (append-only, newest last).

Folded in 2026-07-27 from `doc/feynman_migration_prompt.md` and `doc/failure_modes.md`,
both now deleted per the canonical-file rule in `ai-workflow/FEYNMAN.md`. Their full
text remains in git history at commit `9b14914`.

--------------------------------------------------------------------------------
# PART 1 — STANDING FACTS

## The question
Decide the universality class of the multicritical point where the toric code's two
transition lines meet on the self-dual line (h = lmbda = s, mu = J = 1) of the (2+1)D
Z2 gauge-Higgs model, by finite-size scaling at beta = L.

**The decidable claim is FIRST-ORDER vs CONTINUOUS.** Not XY-vs-new-class — those are
exponent-degenerate at reachable sizes. Do not over-demand nu precision.

## The scope fact that is easiest to lose
**Two critical points, nearly the same h, different lines.**
- Benchmark: (h, lmbda) = (0.335, **0.30**) — off-diagonal, continuous 3D Ising.
- MCP: (s*, s*) with s* ~ 0.333 — on the diagonal.
Everything at lmbda = 0.3 is instrument calibration, not the experiment. The MCP FSS
design targets L ~ 120-192; L = 10-30 is benchmark scale.

## What is measured and trustworthy
- **Benchmark: CLOSED, PASS.** h_c(inf) = 0.3359 +/- 0.0049 against Linsel's 0.335(2),
  0.2 sigma. Two residuals are recorded in `doc/benchmark_report.md` sec 6 and are not
  closed: the input crossings come from a PT set with zero strict round-trips, and we
  do not know whether Linsel's 0.335(2) is an infinite-L value or a finite-L estimate.
- **s* = 0.333 +/- 0.004**, systematic-dominated. High confidence the transition is in
  [0.325, 0.350]; low confidence beyond the third digit. FSS at L = 120-192 needs
  ~0.001, so the gap is a factor ~4 and it is SYSTEMATIC — more chains at the same
  s-values cannot close it, only a finer s-grid.
- **Order at s*: UNDECIDED**, correctly reported inconclusive at L = 8-16. All 528
  energy histograms unimodal, zero stable-separated cold/hot pairs.
- **PT transport is solved.** Complete traversal, every replica visiting every rung,
  after four earlier ladders produced zero round trips. The fix was the ladder span,
  not the swap mechanism.
- **PT's benefit is ERGODICITY, not speed.** Cost-normalized speedup is ~1.2x where it
  can be measured. At L=10 the no-PT ensemble is non-ergodic at low h and no amount of
  depth or extra chains repairs it, because independent chains populate basins by
  arrival rather than by thermodynamic weight. PT restores correct weights. That is a
  capability and it does not appear in any tau ratio.

## Physics and method facts — do not re-derive
- **Thermalization needs two checks:** cold/hot agreement AND observable stationarity
  versus depth. Never judge by within-chain Neff — it is blind to a chain stuck in one
  basin, and blind to all chains drifting together.
- **Group summaries hide broken ensembles.** Cold/hot can PASS because the hot group's
  own internal split inflates its variance and drives z down. Only the per-chain table
  shows it. Report per-chain, always.
- **A frozen chain reports LOW tau.** Being stuck looks like fast mixing. tau alone
  cannot detect an ergodicity failure in either direction.
- **tau_int is the wrong instrument for a relaxation time.** It is the variance-weighted
  integral — correct for an error bar, wrong for a cost law, because it is (slow-mode
  amplitude) x (decay time) and the amplitude falls with L through self-averaging. Any
  z fitted to tau_int(L) comes out negative.
- **A dimensionless-ratio crossing drifts as L^-(omega + 1/nu), not 1/L.** For 3D Ising
  that exponent is 2.42. This is importable only where the class is already known — at
  the MCP it is not, so this extrapolation route is unavailable there by construction.
- **Percolation must be measured on a rotated imaginary-time slice**, once per snapshot,
  all equal-time observables on the same slice. Without rotation it reads the frozen
  tau=0 base, which is the initial condition.
- **The two z-basis percolation channels are both ELECTRIC** (same sigma_z = -1 edges,
  direct vs dual graph), complementary, percolating on opposite sides. They are NOT e
  and m. Magnetic is the x-basis. The dual pairs are cross-basis: z-perc == x-plaq and
  z-plaq == x-perc. Comparing the two z-basis channels to each other is not a duality
  test.
- **Use PROBABILITY for location and duality** (bounded, no vanishing denominator);
  use STRENGTH only against its own channel for beta/nu.
- **The raw ratio dies where the order parameter dies.** Above s ~ 0.35 the strength is
  81-98% exact zeros, so <P^4>/<P^2>^2 divides two vanishing moments and returns
  nonsense. The >50%-exact-zeros tripwire is real and is now an automated gate.
- **Units.** Our s is the quantum field h = lmbda. The classical gauge-Higgs
  lambda_bond ~ 0.226 (Tupitsyn/BPV) is a different parametrization, not our s.
- **swap_period is in SWEEPS**, never raw moves. Its natural scale is the local energy
  relaxation time, ~5,400 sweeps at L=10 on the benchmark line. Attempting swaps far
  more often than that adds nothing: rung-space diffusion measured in sweeps is
  independent of attempt rate, because an unchanged energy reproduces the same
  accept/reject decision.
- **Round trips are neither necessary nor sufficient** for low autocorrelation. The
  gate is traversal AND a finite tau AND per-replica agreement, reported together.
- **PT is z-basis only** — a compile-time restriction at
  `src/mcmc/extended_toric_code_qmc.hpp:3771-3772`. PT plus checkpointing throws, so
  any PT run must fit one walltime block.
- **N is not a free knob.** Round-trip time goes as N^2 / A_harmonic; ~23% acceptance
  is the optimum. Adding rungs to fix acceptance is counterproductive.
- **Calibrate any observable on known static configurations** before trusting a
  dynamic run.

--------------------------------------------------------------------------------
# PART 2 — LEDGER (append-only, newest last)

Each entry: what was claimed, what happened, what it cost, what it changed.

**Frozen-base prediction — REFUTED.** Predicted percolation would read the frozen
tau=0 base. It did not; the fix was to rotate the imaginary-time slice. Cost: a
diagnostic cycle. Left behind the rotated-slice rule.

**Naive-error-bar hypothesis — REFUTED.** Claimed the stored errors were naive and
needed a sqrt(2 tau) correction. They were already stationary-bootstrap. Resolved by
reading the code that produces the error rather than reasoning about it.

**"Wrong channel" for the trend inversion — REFUTED.** The observable was correct all
along; we were plotting U = 1 - r/3, a decreasing transform of the raw ratio Linsel
plots directly. A plotting-convention mismatch, diagnosed as a physics error. Cost:
a scare and a redesign. Left behind: check the source's exact definition before
comparing to it.

**s* derived from geometry, four times — REFUTED each time.** s* is a MEASUREMENT.
Linsel's ~0.33-0.35 is the prior.

**Winding-sector ergodicity breaking — REFUTED in one run.** Proposed that cold and hot
chains sat in different topological sectors. A direct sector-exchange counter found
hundreds of thousands of exchanges everywhere. It was ordinary critical slowing, and it
was depth-solvable. Cost: one run, because the probe was direct. Worth noting as the
model of how to kill a hypothesis cheaply.

**Circular calibration — CAUGHT BY JARVIS BEFORE IT COST ANYTHING.** The spec said to
read the depth at one size and extrapolate the depth law. Extrapolating from one size
requires assuming the exponent, so the result could only reproduce the assumption.
Jarvis refused to submit on that basis and saved ~3,919 task-hours. Rule: never
calibrate a scaling law from one system size, and never let a design choice become its
own confirmation.

**"Benchmark-line z sets the M2 budget" — OVERSTATED, corrected by the PI.** That z is
measured on a continuous line. If the MCP is first-order its slow mode is exponential
tunneling, tau ~ exp(2 sigma L^2), which no continuous-line z extrapolates to. The
Ising-line z is a best-case LOWER BOUND on MCP cost. Necessary, not sufficient.

**"PT delivers a speedup" — ASSUMED, caught by the PI.** The S0-S6 suite validated the
swap MECHANISM. It says nothing about whether PT reduces autocorrelation. Mechanism-is-
correct and mechanism-helps are separable claims and I conflated them. Measuring the
benefit required a trustworthy no-PT baseline, which exists only on the benchmark line
and never at the MCP — so the measurement was possible then and impossible later.

**Round trips called "necessary" — WRONG, corrected by the PI.** Written into the run
manual (F3/F7) and into harness gate G1.2, and used to score four consecutive ladder
attempts NO-GO. None of those four measured tau at h_c. A round trip is sufficient
evidence of mixing when genuine, but not required: a replica can decorrelate through
local excursions. Cost: four ladder attempts scored on a metric that does not gate the
thing we care about. Caveat that survives: at a first-order point, escape requires
reaching a rung where coexistence closes, so round trips become near-necessary again.

**Error bars claimed too small — REFUTED by my own probe.** Read a rising blocking curve
at 8-16 blocks as an unresolved slow mode and concluded the L=20 void was an analysis
artifact. The tau-free between-chain check agreed with the bootstrap at every point,
including both disputed ones. The blocking curve had been read in the too-few-blocks
regime — the exact thing the same update warned against. Cost: two updates of reasoning
against one command of measurement.

**L=10 non-ergodicity — FOUND, after ~15 handlings of the same dataset.** Six chains at
ratio 1.74, one at 1.30, one at 1.45; per-chain tau ratios violating the factor-2 rule
by up to 11,275x at 8 of 9 fields, worst exactly where the crossing is read. Every
existing check missed it because they all consumed pooled numbers, and pooling destroys
the evidence of its own failure. **The general lesson, which is the most expensive one
in this log: a directive must name the evidence to PRINT, not only the verdict to
reach.** Jarvis delivered exactly what was asked for, correctly; the per-chain table was
never withheld, it was never requested. It looked like an execution problem and was an
instruction problem.

**Extrapolation exponent p = 1 — WRONG, and it inverted the benchmark verdict.** Fitting
crossing drift against 1/L gave h_c = 0.3236 and an apparent tension with Linsel. The
correct drift for a dimensionless-ratio crossing is L^-(omega + 1/nu) = L^-2.42, which
gives 0.3359 and agreement at 0.2 sigma. Three points cannot determine that exponent —
residuals are identical across p — so it must be imported from the known universality
class, not fitted. Honest error bars on the crossings then stabilized the result: the
p-systematic fell from +/-0.012 to +/-0.0012.

**PT tau of ~50 sweeps — an ARTIFACT, flagged by Jarvis before I saw it.** The swap
period equals the sample interval, so 50 sweeps is 0.51 samples, which is the exact
white-noise floor of tau_int. The per-rung series had been built by concatenating
disjoint visits with a mean dwell of ~1 sample, so consecutive entries came from
different visits and the correlation it was meant to measure had been destroyed. The
254x/274x/128x/70x speedups built on it are meaningless, not optimistic. Fix: build the
fixed-rung series in real time — at each step exactly one replica occupies each rung.

**Speedup evaluated against a broken baseline — WITHDRAWN, then partly reinstated.**
Fired a pre-committed decision rule at h_c using a no-PT reference, then found the
reference failed a spread gate there, then found the gate itself was the problem: three
defensible criteria give 3, 7 and 7 failures out of 9 and disagree exactly at h_c. The
window-scaling test — does the per-chain spread fall as 1/sqrt(window), as estimation
noise must — says h_c is clean. That criterion is preferred because it tests a
functional form rather than comparing a statistic to an arbitrary number, so it cannot
be tuned by moving a threshold. Cost: two reversals on the same number. Standing rule:
**a validated-baseline check belongs in the comparison code as a gate, not in the
analyst's memory.**

--------------------------------------------------------------------------------
# PART 3 — THE PATTERN

Stated plainly because it recurs: **assert from assumption, then let an expensive run
discover the assumption was wrong.** Most of the catches above came from the PI or from
Jarvis, not from me.

Every time this campaign got unstuck it was a cheap direct probe — the calibration unit
test, the marker test, the sector-exchange counter, the depth ramp, the between-chain
error, the per-chain table, the window-scaling scan. Nothing was ever unstuck by
re-reviewing existing numbers.

So, before asserting: name the cheap probe that would falsify this, and specify that
instead of the assertion. Distinguish mechanism-is-correct from mechanism-helps. Never
let a design choice become its own confirmation. Prefer a quantity independent of the
choices we made (rates, scaling forms) over one contaminated by them. And when a rule
exists on paper but is not wired into a gate, it will not fire — that has now happened
with the round-trip rule, harness G1.2, and the intermittency tripwire.
