# ParaToric Run Manual — picking parameters & validating runs

Purpose: a standalone, do-not-bury checklist for running ParaToric correctly the
FIRST time. Every rule here was paid for by a real mistake this campaign (see the
Pitfall Catalog, Part C). Running ParaToric is delicate — percolation is a slow,
subtle observable and the MCP amplifies every one of these. Read Part D (minimal
recipe) before any run; read Part C when a run "fails".

Authority: this manual operationalizes harness invariants I1-I6
(doc/multicritical_execution_harness.md). If this manual and the harness ever
conflict, the harness wins and this file is fixed.

--------------------------------------------------------------------------------
## GOLDEN RULES (the five that cost us the most)

G1. THERMALIZATION NEEDS TWO CHECKS, NOT ONE:
    (a) COLD/HOT AGREE (I5): cold-start (default_spin=+1) vs hot-start (=-1) means
        agree (z<3 / R-hat~1). This rules out chains STUCK IN DIFFERENT BASINS.
        Never judge thermalization by within-chain Neff — Neff measures
        decorrelation WITHIN a basin and is blind to a chain in the wrong basin.
    (b) OBSERVABLE STATIONARY vs DEPTH: the mean must PLATEAU across increasing
        thermalization (e.g. 2M/4M/8M means agree within error). Cold/hot and
        R-hat measure BETWEEN-chain dispersion — they are BLIND to ALL chains
        DRIFTING TOGETHER. A monotonic common drift (esp. non-decelerating =
        algebraic/critical-slowing) with flat density but drifting POP = the
        GLOBAL winding structure not yet equilibrated, even though cold/hot agree.
    BOTH are required. (a) without (b) passed us an under-thermalized L=32,h_c
    point whose POP kept climbing 0.699->0.717->0.734 at 2M/4M/8M while all 8
    chains agreed. For a critically-slow raw observable, prefer a RATIO (Binder
    cumulant) which often plateaus while raw moments still drift.

G2. THERMALIZATION DEPTH MUST BE NON-DECREASING IN L at fixed critical parameters.
    Critical slowing: tau ~ L^z (z>=1). If L=24 at h_c needs 2M sweeps, L=32 needs
    MORE, never less. Any depth table or scan that gives a bigger lattice LESS
    thermalization at the same critical point is WRONG — treat as a red flag, not
    a result.

G3. >=4 INDEPENDENT CHAINS, minimum (I4); >=8 for any FSS/crossing point. TWO
    chains (cold+hot) can DETECT a problem (I5) but can NEVER establish convergence
    or a crossing. Do not set depths or declare a point good from 2 chains.

G4. JUDGE SAMPLING BY Neff OF THE WORST OBSERVABLE, NOT RAW N (I6). Worst observable
    is percolation_strength (tau 2k-1e4). Need N>=100*tau (so tau is trustworthy)
    AND Neff=N/(2*tau)>=30 for a production point. Measure tau from the full time
    series; never assume a fixed sample spacing is independent.

G5. CALIBRATE THE OBSERVABLE ON KNOWN STATIC CONFIGS BEFORE TRUSTING A DYNAMIC RUN.
    A static hand-built config with a known answer (all +1, all -1, one winding
    string, dense random) is the only way to know what the instrument returns.
    Dynamic-run numbers are uninterpretable until this is done.

G6. CHECKPOINT LADDERS MUST BE GEOMETRIC, FIRST CHECKPOINT WELL BELOW EXPECTED
    CONVERGENCE DEPTH (~1/8 or less). A ladder whose first checkpoint already sits
    at or past the convergence depth measures nothing — it cannot show the
    relaxation SHAPE, only confirm an endpoint. Cost: the L=20 no-PT depth-ramp
    (2026-07-19) opened its ladder at 8M sweeps; most h-points were already
    converged there, leaving no genuine plateau-entry depth to calibrate against,
    and a control point (h=0.305629) never stabilized through 64M — the ladder's
    coarseness masked it. Fix: space checkpoints geometrically (e.g. x2 per step)
    starting well before the best available depth estimate, not just under it.

--------------------------------------------------------------------------------
## PART A — Picking run parameters

A1. Lattice / aspect ratio: beta = L (T=1/L), the campaign choice (z=1 assumed;
    matches Linsel Fig 4). Verify beta=L is actually set in the job script — do
    not assume.

A2. Basis: production PT is z-basis only. The electric (e) sector = z-basis; the
    magnetic (m) sector = x-basis (em self-duality). For a topological "both e & m
    deconfined" check you must run BOTH bases (see A6).

A3. Couplings: mu=J=1 always (toric-code stabilizers). The self-dual line is the
    single parameter s = h = lmbda. Off-self-dual (benchmark) lines vary one field
    at fixed other (e.g. lmbda=0.3, scan h = Linsel Ising line).

A4. Parameter LOCATION / units (do not confuse parametrizations):
    - Our variable = quantum field s = h = lmbda.
    - Self-dual MCP: s* ~ 0.33-0.35 (Linsel arXiv:2401.08770, OUR model; measure it).
    - Linsel Ising benchmark: h_c = 0.335(2) at lmbda=0.3 (OFF the self-dual line).
    - Tupitsyn/BPV MCP lambda_bond*~0.226 is CLASSICAL gauge-Higgs — NOT our s.

A5. Thermalization depth: set PER (L, region) and NON-DECREASING in L (G2).
    - Off-critical points: cheap (often ~100k sweeps at small L).
    - Near a transition/MCP: expensive (critical slowing). If L24 needs 2M, budget
      L32 >= 2M (scale ~ (L2/L1)^z, z~1-2 => >=1.8x). Ramp until cold/hot agree.
    - Minimal-but-sufficient: start at the smallest depth that PASSES the I5
      cold/hot check at THIS L and region; larger L or nearer the transition
      requires more.

A6. Chains: >=4 independent seeds per point (I4); >=8 at FSS/crossing points.
    Always run cold (default_spin=+1) AND hot (=-1) starts for the I5 check.

A7. Sample spacing & count: do NOT pick a fixed spacing and assume independence.
    After thermalization, record a time series; measure tau_int; keep
    N >= 100*tau and Neff >= 30 (production) of percolation_strength. If tau is
    unknown, over-sample and thin in analysis.

A8. Observable / channel:
    - RE-VALIDATED on clean post-fix data (2026-07-13): z-basis deconfinement
      POP = the PLAQUETTE channel (plaquette_percolation_probability), CONFIRMED
      empirically on the resourced-benchmark data (jobs 15118635/36/37):
      plaquette POP is ~0.82-0.89 at small h falling to ~0.22-0.38 at large h,
      the direct/electric channel stays near-zero throughout. The earlier
      "UNVALIDATED" flag on this line (raised after a trend-inversion scare) is
      RESOLVED: the channel was correct all along. The actual bug was plotting
      U=1-<P^4>/(3<P^2>^2) (decreasing in the ratio) where Linsel's Fig.4a plots
      the raw ratio <P^4>/<P^2>^2 directly (log-scale 10^0-10^2, unbounded,
      increasing) — a plotting-transform mismatch, not a channel error. Plot the
      raw ratio, not U, when comparing against Linsel's figure directly.
    - The two z-basis channels are complementary (same sigma_z=-1 edges, direct vs
      dual graph) — they percolate on OPPOSITE sides; do not expect both positive
      from z-basis alone. m-sector needs the x-basis.
    - For DUALITY / e-vs-m / crossing LOCATION: use PROBABILITY (dimensionless,
      crosses). Correct dual pairs at the self-dual point: z-perc==x-plaq,
      z-plaq==x-perc (duality swaps sigma_z<->sigma_x AND vertices<->plaquettes).
    - For beta/nu: use STRENGTH, each channel against ITSELF only. Do NOT
      cross-compare absolute strengths (vertex=#edges/2L^2 vs plaquette=
      #plaquettes/L^2 differ by a fixed ~1.8x normalization — not a bug).
    - Percolation is measured on a ROTATED imaginary-time slice (rotate_imag_time
      once per snapshot, all equal-time obs on the SAME slice). Without rotation it
      reads the frozen tau=0 base = the initial condition.
    - Companion: record largest_cluster unconditionally; if percolation_strength is
      >50% exact zeros near a putative continuous transition (intermittency), switch
      analysis to largest_cluster.

--------------------------------------------------------------------------------
## PART B — Post-run validity checklist (run EVERY time, in order)

B1. I1 energy bookkeeping: final integrated-potential almost_equal check passed
    (the run itself throws otherwise). Any mismatch => invalidate, do not tune
    tolerances.

B2. I3 bounds: 0 <= POP/POS <= 1; susceptibilities >= 0; Binder in range.
    Out-of-bounds mean = bug, never physics.

B3. I5 THERMALIZATION (the one we keep failing): cold vs hot means agree, z<3, at
    a reference point per L. Do this with the AUTOCORRELATION-AWARE error
    (ensemble_diagnostics.py, bootstrap/blocked). If cold!=hot => UNDER-THERMALIZED
    (or ergodicity-broken); increase thermalization (G2) and/or chains. NEVER pass
    B3 on within-chain Neff alone (G1).
    - Extra tell: if the SAME point's z grows when you add samples, that is a
      real fixed discrepancy being resolved (under-thermalization), not noise.

B4. I4 SAMPLING: for each observable, report tau_int and Neff=N/(2*tau). Point
    counts only if N>=100*tau AND Neff>=30 (production) AND >=4 chains agree
    (means z<3, tau within factor 2). tau from a single short chain is NOT evidence.

B5. I2 DUALITY (self-dual points): x-basis star obs == z-basis plaquette duals,
    z<3. For percolation use the PROBABILITY dual pairs (A8). Systematic violation
    = mapping/impl error, not physics. (Strength pairs differ by the ~1.8x
    normalization — expected, use probability.)

B6. TRIPWIRES (abort + report, do not silently "fix"):
    - tau > N/10 anywhere => point unusable; lengthen/add chains.
    - acceptance of any move type < 1e-4 at production => dead weight; flag.
    - observable mean shifting >3 sigma between checkpoint segments => state
      corruption; discard chain, file bug.

--------------------------------------------------------------------------------
## PART C — Pitfall catalog (symptom -> cause -> fix). Each one actually happened.

C1. Percolation identically 0 everywhere (incl. deep in the phase).
    Cause: measured on the un-rotated tau=0 base (frozen init) OR density below the
    direct-percolation threshold (electric channel <50% density) OR wrong channel.
    Fix: rotate_imag_time; use the plaquette channel; calibrate on known configs.

C2. Same physical point gives contradictory z across two runs; MORE samples ->
    MORE sigma. Cause: NOT naive error (bootstrap handles autocorrelation) — it is
    UNDER-THERMALIZATION; correct errors faithfully report a growing significance
    as a fixed real discrepancy resolves. Fix: G1/G2 — thermalize more, judge by
    cold/hot (I5), not Neff.

C3. Crossing drifts AWAY from the known value at larger L (e.g. L24/L32 worse than
    L16/L24). Cause: the larger L is under-thermalized (G2 violation in the depth
    table). Fix: scale thermalization up with L; re-judge with >=4 chains.

C4. nu comes out wildly wrong (e.g. 1.2-1.9 vs 0.65). Cause: (a) too few/too small
    L, (b) local-slope method on noisy data, (c) wrong crossing quantity (POP=0.5
    instead of the Binder-cumulant/probability CROSSING between L). Fix: recognize
    that even Linsel got nu=0.65(27) at L<=30 — do not demand a tight nu; use the
    dimensionless-crossing method; the benchmark's job is h_c location + Ising
    COMPATIBILITY, not a precise nu.

C5. z-basis "electric" and "plaquette" channels disagree / aren't both positive.
    Cause: they are the SAME sigma_z=-1 edges on direct vs dual graphs (NOT e/m);
    complementary. Fix: use plaquette as the z POP; get m from the x-basis.

C6. Cross-basis e-vs-m numbers disagree. Cause: compared same-graph channels;
    duality swaps vertices<->plaquettes too. Fix: compare z-perc==x-plaq,
    z-plaq==x-perc, via PROBABILITY.

C7. PT round-trips look great at L=8 but PT does nothing at L=120. Cause:
    swap_period in MOVES not sweeps (label travel; ~0.03 sweeps/swap at L=120).
    Fix: swap_period in SWEEPS, scale with L^2; judge by strict round-trips +
    tau_ergodic (pt_architecture.md).

C8. A convergence/depth scan says a bigger L needs LESS thermalization. Cause:
    the scan used too few chains (2) and was fooled. Fix: G2 + G3; discard such a
    scan result as physically impossible.

--------------------------------------------------------------------------------
## PART D — Minimal-but-sufficient recipe (default for a new point)

The smallest run that still catches the failures above:

1. PICK: beta=L (verify in script); mu=J=1; the target s (or benchmark h at fixed
   lmbda). Know where s* is in OUR units (~0.33-0.35), not the classical 0.226.
2. CHAINS: 4 seeds cold (default_spin=+1) + 4 seeds hot (=-1) = 8 chains. (For a
   pure detection/smoke, 2 cold + 2 hot; never fewer for anything you'll quote.)
3. THERMALIZE: ramp depth (e.g. 2M -> 4M -> 8M near a transition; ~100k off-
   critical at small L) and STOP when cold/hot means agree z<3 (I5), depth
   NON-DECREASING in L. Do not fix a depth from a 2-chain scan.
4. SAMPLE: after thermalization, record a time series; measure tau_int of
   percolation_strength; keep N>=100*tau, Neff>=30. Errors via the
   autocorrelation-aware analyzer (ensemble_diagnostics.py), never fixed-spacing-
   assumed-independent.
5. OBSERVABLE: plaquette channel for the z POP, on a rotated slice; probability for
   location/duality, strength (own channel) for beta/nu; largest_cluster companion.
6. VALIDATE: run Part B in order. A point is USABLE only if B1-B5 pass. If B3
   fails, the point is under-thermalized — nothing downstream is trustworthy.

--------------------------------------------------------------------------------
## PART E — MCP-specific cautions (the delicate regime)

E1. Critical slowing is worst AT s* — expect the largest thermalization and tau
    there; budget the most chains and depth for the MCP points.
E2. If the MCP is (weakly) first-order: energy histograms go bimodal; cold/hot may
    NEVER converge with local updates (metastability, not slow-but-converging);
    a single s-ladder PT gets a central-rung acceptance bottleneck (H5). Diagnose
    with energy-histogram bimodality + per-link acceptance, do not force it.
E3. Reservoir for PT sits JUST BELOW s* (~0.28-0.34 if s*~0.35), NOT deep in the
    deconfined phase — the winding sector is MORE frozen the deeper you go (exactly
    conserved at s=0). Concentrate ladder rungs near s*.
E4. The decidable claim is order (first-order vs continuous), NOT XY-vs-new-class
    (exponent-degenerate at reachable L, R1). Do not over-demand nu precision.

--------------------------------------------------------------------------------
## PART F — PT run diagnosis (PRELIMINARY — thresholds to calibrate with experience)

Not a physics run but a sampler: judge a PT run by ladder health, not just by
observable means. Every PT run report MUST include the following (incomplete
reports do not gate decisions). Deep rationale + ladder-health invariants H1-H6:
doc/pt_architecture.md. Numeric bars below are starting points, refine with real
MCP PT data.

F1. swap_period IN SWEEPS (never moves). >= ~1 sweep (scale with L^2) so the
    config genuinely evolves between swaps. swap_period << 1 sweep = LABEL TRAVEL
    (round-trips on a frozen config, useless). [C7 / H1]
F2. PER-LINK (EDGE) swap acceptance, every link. Target ~20-40%. Any link < 1e-4
    = dead/bottleneck. A single acceptance HOLE localizes a bottleneck rung; near
    s* it may be a first-order coexistence gap (H5), not just bad spacing.
F3. STRICT ROUND-TRIPS (visited_both_ranks) — replicas that genuinely reached BOTH
    ends, NOT endpoint touches (touches overcount: real=6 vs touch=10 on job
    14982619). Pilot bar: >=3 strict round-trips or the ladder is broken.
F4. UP/DOWN FLOW: visited-LOW and visited-HIGH per replica -> flow fraction f(rung);
    should be ~monotonic; non-monotonic f / plateau pins the bottleneck rung.
F5. tau_ergodic (round-trip time, IN SWEEPS) — this, not raw sample count, sets the
    effective decorrelation at the s* rung. Budget Neff at s* in tau_ergodic units.
F6. tau(percolation_strength) at s* — the PHYSICAL slow mode. PT GO requires it to
    actually DROP (>=5x vs no-PT baseline) AND replicas to change the winding sector.
F7. KEY WARNING: round-trips are NECESSARY but NOT SUFFICIENT — a label can
    round-trip on a frozen config (F1). Pair RT/flow (F2-F4) with genuine
    decorrelation (F5-F6). Mechanism without physics = NO-GO.
F8. Reservoir & first-order (Part E): useful reservoir is JUST BELOW s*; a
    first-order MCP gives a central-rung acceptance hole (F2) + stalled strict
    round-trips (F3) + bimodal energy = sampling failure AND physics signal.
    Diagnose, do not force.

TODO (calibrate with experience): real acceptance/round-trip/tau_ergodic bars at
the L=120 MCP pilot; feedback-placement trigger; first-order handling. Update
after the pilot.
