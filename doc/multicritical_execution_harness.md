# Execution Harness for the Multicritical Universality Plan

Authority order for the executing agent: parent `/project/theorie/a/A.Otaifi/AGENTS.md`
→ `doc/multicritical_universality_plan.md` (the plan) → this harness.
This file is the gatekeeper: NO production submission unless the relevant gate below
is PASSED with evidence written to `doc/run_tracker_multicritical.md` (create it;
one line per gate: date, evidence path, PASS/FAIL, decision).

## Global invariants (check at every stage; any violation = STOP and report)

- I1 Energy bookkeeping: final `almost_equal` integrated-potential check passes in
  every run; any mismatch throw = invalidate the run, do not tune tolerances.
- I2 Duality: at self-dual points, x-basis star observables must equal z-basis
  plaquette duals within errors (z < 3). Systematic violation = implementation or
  mapping error, not physics.
- I3 Bounds: 0 <= percolation_probability/_strength <= 1; susceptibilities >= 0;
  Binder within its allowed range. Out-of-bounds mean = bug, never report as physics.
- I4 Statistics bar: a point counts only if N_samples >= 100*tau_worst AND
  Neff = N/(2*tau_worst) >= 30 AND >= 4 independent chains agree (means z < 3,
  tau estimates within factor 2). tau from a single chain with N < 100*tau is
  NOT evidence.
- I5 Equilibration: discard-window doubling test (first vs second half means z < 2)
  per chain; cold- and hot-start chains must converge to the same means at one
  reference point per L.
- I6 Never judge progress by raw N_samples; only Neff of the worst observable
  (currently percolation_strength).

## Gate G0 — before ANY code change (M0 features)

- G0.1 Baseline reproduction: rebuild `build-current-clang-portable`, rerun one
  known smoke (L=40 beta=80 from performance_report.md) and match acceptance
  fraction to ~1% and energy within errors.
- G0.2 fast-math A/B: one L=40 pair with PARATORIC_ENABLE_FAST_MATH ON vs OFF,
  same params, 4 seeds each: all observable means z < 3. FAIL => production runs
  use OFF and note the speed cost.
- G0.3 Existing tests pass (`tests/`) before and after every feature commit.

## Gate G1 — new-feature correctness (checkpoint, PT, winding move)

- G1.1 Checkpoint: save/load must be bit-faithful (same seed: continued run ==
  uninterrupted run for observable series) or, if RNG state not restorable
  bit-exact, statistically identical (z < 3 over 4 seeds). Verify event-vector
  sort invariants after load.
- G1.2 PT swap weight: validate the O(1) cached log-weight ratio against
  brute-force recomputation (total_integrated_pot_energy + kink-count terms)
  at every swap in a debug run (L=8): max abs deviation < 1e-8. Swap acceptance
  targeted 20-40% by ladder spacing; report round-trip time as transport context.
  The gate is measured tau at the rung of interest, not round-trip count.
- G1.3 PT physics: PT-on vs PT-off means agree (z < 3) at an easy point; each
  replica's marginal must reproduce the single-chain result.
- G1.4 Winding-loop move (if implemented): (a) ΔE vs brute-force recompute at
  L=8 < 1e-8; (b) detailed balance: forward/reverse proposal symmetry argued in
  writing before coding; (c) small-L observable means unchanged (z < 3) with move
  mixed in; (d) acceptance logged per-move in existing HDF5 diagnostics.
- G1.5 Any new move or PT must pass an exact benchmark: L=2/L=3 comparison against
  exact diagonalization or high-precision reference if available in tests/.

## Gate G2 — pilot go/no-go (blocks all production compute)

- PT ladder through multicritical region at L=120, beta=120, walltime <= 2 days,
  >= 4 chains. GO if tau_worst drops >= 5x vs the L120 baseline (tau ~ 9000-10000).
- 2x-5x improvement = MARGINAL: try winding move / ladder retuning once, then
  re-decide. < 2x = NO-GO: stop, report, do not submit the FSS ladder.
- Record Neff/day at the pilot point; extrapolate the full M2 budget BEFORE
  submitting anything: if projected total > 5x the compute budget in the plan,
  stop and renegotiate scope with the user.

## Gate G3 — per-point production acceptance (M2)

- Every (L, point) must satisfy I4 + I5 before it enters any fit.
- Monitor per-update acceptance diagnostics (HDF5 `diagnostics/production_acceptance`);
  a move type with acceptance < 1e-4 at production beta is dead weight — flag it,
  do not silently keep paying for it.
- Walltime guard: single job <= 14 days with checkpoint saved >= every 24h.

## Gate G4 — before claiming a universality class (M3)

- Exponent stability: effective exponents from (L, 2L) pairs must be monotone or
  plateaued over >= 3 pairs; drifting exponents = report "not converged", never
  extrapolate a class from a drift.
- First-order check is mandatory even if data look continuous: energy histogram
  double-peak scan + Binder minimum scaling at the two largest L.
- Duality-symmetrized and raw estimates must give the same class.
- Scenario call requires exclusion, not just fit quality: state which of the three
  scenarios (first-order / multicritical XY / new self-dual class) are excluded
  and by which observable, with error bars.

## Cluster & budget guardrails (from parent AGENTS.md, restated as hard rules)

- 1 node default; ask before > 1 node. Never `--mem=0`; request estimated memory
  + margin (reference: 3.2 GB at L=1000 beta=400 Boost).
- Use `slurm_concise_alert.sh submit <jobid>` for monitored jobs.
- Stage pilots before arrays; no job array > 20 tasks without an explicit
  user-approved budget line in the tracker.

## Unphysical-constraint tripwires (abort + report, do not "fix" silently)

- tau estimate larger than N/10 anywhere in production (warning already coded) =>
  the point is unusable; lengthen or add chains, never quote it.
- percolation_strength series > 50% exact zeros near a putative continuous
  transition => intermittency regime; switch analysis to largest_cluster series.
- Acceptance fraction collapsing with beta faster than expected (compare pilot
  scaling) => phase misidentified or parameters off the critical region.
- Any observable mean changing beyond 3 sigma between checkpoint segments of the
  same chain => state corruption; discard chain, file a bug.

## Scope locks

- Reading phase of any sub-task: no source edits, no submissions (mirrors
  doc/performance_deep_dive_plan.md guardrails).
- Production backend stays Boost. flat_square is experiment-only (its observable
  gate excludes percolation observables — see src/io/io.cpp).
- Do not chase L >= 256 or a worm algorithm without an explicit new user decision.
- Every gate outcome, budget estimate, and deviation from the plan goes into
  `doc/run_tracker_multicritical.md` — the next agent bootstraps from that file.
