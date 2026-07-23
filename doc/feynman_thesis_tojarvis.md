# Feynman -> Jarvis Handoff: Thesis T1 and M0 Authorization

Date: 2026-07-08. Context: Feynman physics-attack pass on
doc/multicritical_universality_plan.md, reading phase (no source edits, no
submissions), findings recorded in doc/run_tracker_multicritical.md
("Open questions for M1", A1-A3, R1-R5). This file is the ratified decision
record and the authorization to start M0.

## Thesis T1 (ratified)

The decidable claim for this campaign is **first-order vs continuous** at the
multicritical point on the exact self-dual line h = lmbda (mu = J = 1), not
"which universality class." Reason (R1): 3D XY (nu = 0.6717) and the SSN
self-dual class (nu = 0.669(4)) differ by ~0.4% in nu and are exponent-
degenerate at any L our budget reaches (SSN themselves could not separate
them at L<=96 with dedicated real-space methods; our reach is L<=192 with a
coarser method). Any M4 write-up must reflect this:
- Result = clean first-order proof -> PRL/PRX, decisive.
- Result = continuous -> PRB; report nu/x_S as "consistent with XY and with
  the SSN self-dual class"; explicitly state these are NOT separated by this
  campaign. Do not claim a class selection between scenarios 2 and 3.
- The emergent-O(2) test that would actually discriminate XY from the new
  class needs operator-multiplet structure not in our observable set; out of
  scope for this campaign.


## ARCHIVED HISTORY

Sections predating 2026-07-18 (pre-pilot planning, resolved blockers, the M0
authorization, and UPDATEs 2026-07-12h through 2026-07-17b) were moved on
2026-07-20 to `doc/archive/feynman_thesis_tojarvis_archive_pre_20260718.md`.
None of it is a live directive; their durable conclusions are already in
pt_architecture.md, paratoric_run_manual.md, benchmark_report.md and the tracker
gate log. The archive file lists the mapping. Pre-trim file: git commit `2124149`.

THE LIVE DIRECTIVE IS THE TAIL OF THIS FILE: UPDATE 2026-07-20d (verdict),
2026-07-20e (triage rule), and 2026-07-20f (refutation + revised order).
2026-07-20f is the CURRENT one and supersedes the step ordering in 20d/20e.

## MASTER PRE-PILOT SEQUENCE (Feynman, 2026-07-10 — supersedes the earlier
## "Finalized next steps" list; integrates RESOLVE-FIRST + pt_architecture.md)

Do in this order. Nothing below is the MCP pilot itself; the pilot sbatch stays
PI-gated. Full detail in doc/pt_architecture.md (D1-D4, invariants H1-H6).

0. [BLOCKING] D1 — swap_period in SWEEPS, scaled with L² (default 1000 MOVES =
   ~0.035 sweeps at L=120 = label travel). Fix before ANY L>8 PT run. No PT run
   at production L is valid until this lands.
1. Validate the OBSERVABLE vs Linsel (no PT): reproduce h_c=0.335(2), nu=0.65
   (3D Ising) at lmbda=0.3, L=10-30, beta=L, in BOTH percolation channels to fix
   which is the electric-string POP. Decides the total-zero POP: bug vs placement.
2. Confirm the DECONFINED RESERVOIR on the self-dual line: at small s (e.g.
   0.05/0.10/0.20) confirm POP>0, POS>0, AND directly measure tau(percolation)
   at a small-s rung — verify the reservoir decorrelates the RIGHT (slow) mode,
   not just fast local modes. If it doesn't, PT has nothing to borrow.
3. LOCATE s* — wide-s scan BOTH directions, no pre-committed direction; cross-
   check Linsel F-S / BPV self-dual MCP.
4. D2 — build the offline PT diagnostics analyzer (strict RT, tau_ergodic in
   sweeps, up/down flow) over ladder_index_by_sample. PILOT PREREQUISITE.
5. D4 — generalize PT to independent (h, lmbda); run PT ACROSS Linsel's KNOWN
   3D Ising line (lmbda=0.3) at L=20-30 and confirm it recovers h_c=0.335,
   nu=0.65 WITH PT. Correctness-at-scale check (NOT acceleration).
6. D3 — PM/acceptance-mode grid (ported to s, robustness guards) to build the
   ladder bracketing s*.
7. THEN stage the MCP pilot (PI-gated): GO = mechanism (edge acceptance +
   strict RT round-trips) AND physics (per-replica sector change + tau(percolation)
   >= 5x drop). (a) without (b) = NO-GO.

## STANDING RULE: PT run diagnostics pipeline (Feynman, 2026-07-10)

From now on EVERY PT run analyzed with James/Jones MUST report this fixed
readout — decorrelation is everything, and these are the knobs we fine-tune:

  - swap_period IN SWEEPS (confirm >= ~1 sweep; flag if it is label-travel).
  - per-link (EDGE) swap acceptance for every link (target 20-40%, no dead link
    < 1e-4; a hole localizes a bottleneck rung / possible first-order point).
  - STRICT round-trips (replicas that genuinely visited BOTH ends —
    "visited_both_ranks", not just endpoint touches).
  - visited-LOW and visited-HIGH counts per replica (up/down flow f(rung);
    non-monotonic f => bottleneck).
  - tau_ergodic (round-trip time, in SWEEPS).
  - tau of the PHYSICAL observable (percolation_strength) at the s* rung — the
    decorrelation that actually sets Neff; the effective tau ~ the round-trip
    time, NOT the local tau. Budget Neff in tau_ergodic units, not raw samples.

A PT run report WITHOUT these is incomplete and must not be used to judge a gate.
Round-trips are necessary but NOT sufficient (H1-H3): a label can round-trip on a
frozen config. Always pair RT with genuine-decorrelation evidence.

## UPDATE 2026-07-18: DECOUPLE the benchmark from PT (Jarvis right); marker test to confirm the h_c bottleneck; beta-bridge = MCP plan of record (documented, deferred)

4th consecutive PT round-trip failure at the L=10-30 Ising benchmark, same cause
each time: a severe LOCALIZED acceptance hole at h_c (L30: 0.24% at h_c; harmonic
mean 6.3% vs median 21.5%). Jarvis's harmonic-mean insight is correct — diffusion
is limited by the WORST link (series resistors), so tau_RT ~ N^2/A_harmonic and a
median-based run-length margin is ~100x too small here. This refines my earlier
"just run longer": it is a REAL, physical, localized h_c hole (specific-heat
enhancement of Var E), not only under-resourcing.

### DECISION 1 (immediate): DECOUPLE the benchmark from PT — Jarvis is right
The benchmark does NOT need PT. Linsel did L=10/20/30 with LOCAL updates; our
"critically slow without PT" evidence was at L=32 (top of range); PT mechanism is
ALREADY validated (S0-S6 at N=4/8). We were making PT work for a check that does
not need it. => CLOSE the overlay benchmark with DEEP NO-PT cold/hot single chains
at L=10/20/30 (Linsel's own method). Removes the 4-attempt PT struggle from the
benchmark. benchmark_report.md stays PROVISIONAL until the NO-PT overlay matches
trend + crossing.

### DECISION 2 (do this, PI's idea): MARKER TEST — measure, do not guess
On ONE failed PT run, instrument the per-replica ladder trajectory: where does
flow collapse, where do replicas turn around. FALSIFIABLE prediction if the
diagnosis is right: ALL replicas pile up against the single h_c link (0.24%), none
cross, flow healthy on both sides. If flow instead dribbles out everywhere =>
distributed under-resourcing (run longer). Cheap (one analysis pass), decisive
localized-critical vs distributed, and it is the evidence base for the beta-bridge
decision.

### DECISION 3 (strategic): beta-bridge = PLAN OF RECORD for the MCP; DOCUMENT, do NOT build yet
Full spec: pt_architecture.md sec 9. Summary:
 - Add beta as a 2nd ladder coordinate; route AROUND the critical point via HIGH
   TEMPERATURE = LOW beta (transition smears out, mixing free), traverse h/s there,
   descend to beta=L (the only measured rung).
 - BEATS the duality-odd (h-lmbda) bridge: duality-odd escapes to the continuous
   Ising lines (STILL critical); beta-bridge escapes to NO transition => defeats
   critical slowing of ANY order. FEASIBLE: coupled-pinning tempers beta with
   worldline rescaling (reference exists).
 - COST: 2D ladder (N^2 diffusion), rescaling + beta-swap weight = new validated
   code. Converts an INFINITE 1D barrier into a FINITE diffusion time.
 - RANKING of MCP barrier tools: beta-bridge > worm (bespoke) > duality-odd.
 - BUILD IF AND ONLY IF the self-dual PT ladder STALLS at s* with a FIRST-ORDER
   signature (central-rung hole density cannot fix + bimodal energy). The
   benchmark (continuous, mild) does NOT justify it. Do not build ahead of that
   evidence.

ORDER: (1) close benchmark NO-PT; (2) marker test; (3) resume the self-dual PT
path toward the MCP — the beta-bridge is on the shelf for when/if s* stalls.

## UPDATE 2026-07-18b: L=10 no-PT flag is RIGHT — refines (not refutes) Decision 1. Depth-ramp is the DECISIVE, high-stakes test.

Jarvis L=10 no-PT pilot: clean off-critical; h_c window [0.319,0.344] shows
non-plateaued drift (ratio still falling at 4M, z=3.71), cold/hot split (z up to
29.6), tau_int ~7,375 sweeps at h_c (300-500x off-critical). Finding is real and
well-characterized. VERDICT: it does NOT break Decision 1's logic; it sharpens it.

KEY PHYSICS: the benchmark is CONTINUOUS (3D Ising, Linsel nu=0.65) => NO
thermodynamic barrier => cold/hot MUST converge and the ratio MUST plateau at
sufficient depth. So the z=29.6 split is NOT first-order metastability. It is
either (a) ordinary critical slowing (depth-fixable, slow) or (b) the WINDING-
SECTOR ergodicity problem: cold(+1)/hot(-1) start in different topological sectors
and local updates connect sectors only slowly.

MECHANISM (Jarvis reported it but under-weighted it): global_single_spin_flip —
the SECTOR-CHANGING move — is dead (<1e-4). Expected/known, BUT its consequence is
the point: with that move dead, the winding sector mixes ONLY by slow string
accumulation, critically slow at h_c => plausibly why cold/hot have not met at 4M.

WHY 100-1000x slower than Linsel (~1e4 sweeps): EXPECTED — our local worldline
update is autocorrelation-bound (the campaign's founding premise). "Linsel didn't
need PT => it'll just work" was too strong; our dynamics need far deeper runs.
Refines Decision 1's premise, does not refute it.

DECISIVE, CHEAP, HIGH-STAKES TEST (Jarvis's proposed step — endorsed, with stakes):
Run the h_c window at L=10 to 20-40M sweeps (~1 hr). Continuous => binary outcome:
 - PLATEAUS + cold/hot CONVERGES => depth-solvable, DECISION 1 HOLDS. Calibrate the
   required depth, extrapolate to L=20/30, scale with matched depth.
 - STILL DRIFTING / cold-hot SPLIT at 40M => the winding sector is NOT ergodically
   sampled by local updates even at L=10 (cheapest size). PROFOUND: PT ALONE would
   NOT fix it (PT shuffles configs, adds NO sector-changing power unless a rung
   mixes sectors) => the WORM becomes MANDATORY, not optional. Biggest campaign
   finding so far; would reorder the whole M0 strategy.
I do not know which way it goes — that is exactly why we run it, at the cheapest L.

ORDER (revised): (1) DEPTH-RAMP at L=10 h_c window = the decisive probe (completes
"close benchmark no-PT"); (2) marker test on a PT run stays valuable but is now
secondary to (1). Beta-bridge/worm ranking unchanged; the depth-ramp outcome
decides whether WORM jumps ahead of everything.

## UPDATE 2026-07-18c: FINALIZED GO — the L=10 depth-ramp, made unambiguous. This is the only decisive open thread.

Marker test (Decision 2) is IN: Hypothesis A confirmed — the h_c link on the L=30
PT ladder is a genuine localized near-wall (0.235% acceptance, only 4/67 replicas
ever cross, 99.9% immediately return; every other link 2-3 orders more permeable).
Credit Jarvis — clean, quantitative, exactly the falsifiable prediction resolved by
data. But note what it does and does NOT settle: it confirms the barrier is
localized at h_c on the CONTINUOUS Ising line WITH PT. It does NOT tell us whether
that barrier is depth-solvable by local updates alone. That is the depth-ramp's
job, and the depth-ramp is now the single thread that gates the whole M0 ordering
(depth-solvable vs WORM-mandatory). Finalizing it here so it runs once, cleanly,
and returns a binary.

### WHY no-PT is the right vehicle (not incidental)
NO-PT means NO PT+checkpoint composition (which throws — an M2 blocker), so a single
40M-sweep chain runs in ONE walltime block with no resume machinery. At L=10, 4M
sweeps ≈ 5-7 min => 40M ≈ 50-70 min/chain; 8 chains × a handful of h-points fits a
single modest array well under any cap. There is no budget argument against running
this properly. Run it properly.

### EXACT SPEC (submit as one Slurm array; do not redesign this)
Physics (unchanged, controlled vs the pilot job 15141215): L=10, beta=10, lmbda=0.3
fixed, mu=J=1, z-basis, NO PT (pure local Metropolis, zero swap moves).

h-points — three classes, all in ONE array:
 - DECISIVE (the failing window, must heal or not): h ∈ {0.318989, 0.322, 0.326,
   0.330615, 0.335363, 0.339574, 0.343521}. These are exactly the pilot's failing
   points; 0.335363 (drift z=3.71), 0.339574 (nonstationary p=0.024), and 0.318989
   (cold/hot z=29.6) are the three anchors — the others fill the window.
 - POSITIVE CONTROLS (must STAY converged — proves the deeper machinery still
   reports "PASS" where physics says it should): h ∈ {0.305629, 0.347399}, the two
   points flanking the window that already passed at 4M. If a control DRIFTS at 40M,
   the ramp itself is suspect and the whole readout is void — this is the guard
   against fooling ourselves.
Chains: 4 cold (default_spin=+1) + 4 hot (default_spin=-1) per h, distinct seeds
(the I5 cold/hot split is THE decisive diagnostic; keep all 8).
Depth: ONE continuous run to 40M sweeps per chain, with checkpoint readouts at
2M / 4M / 8M / 16M / 24M / 32M / 40M. The checkpoint LADDER matters as much as the
endpoint — we need the SHAPE of the approach, not just the final number.

### READOUT — three quantities per (h, checkpoint), via ensemble_diagnostics.py
 1. Cold/hot I5 z on the raw ratio <P^4>/<P^2>^2 (plaquette channel) AND on base
    -1-edge density. (Two observables: the ratio is the FSS quantity; the density
    is the slow winding-structure mode from the L32 finding.)
 2. Stationarity: constant-fit p and slope-z of the ratio over the checkpoint
    ladder, AND the INCREMENT sequence Δ(2→4M), Δ(4→8M), Δ(8→16M), Δ(16→24M),
    Δ(24→32M), Δ(32→40M). Decelerating (geometrically shrinking) increments =
    approaching a plateau; roughly-constant increments = linear/algebraic drift.
 3. tau_int and Neff at each point (context only; Neff on a non-stationary series
    is not trustworthy and does not by itself gate anything — the drift call does).

### DIRECT MECHANISM PROBE (the sharpening — do this, it is nearly free)
The 18b hypothesis is specific: cold(+1) and hot(-1) sit in DIFFERENT
winding/topological sectors and local updates connect them slowly. Test the
MECHANISM, not just its symptom: track the per-chain winding indicator
(percolation_probability / winding-string percolation, the non-contractible-string
sector proxy — winding string topologically MUST percolate, calibrated 2026-07-12)
across the 40M trajectory, and count SECTOR-EXCHANGE EVENTS (a chain crossing from
cold-basin to hot-basin value and vice versa).
 - If in-window points show ZERO sector exchanges over 40M while the positive
   controls exchange freely => that is the DIRECT smoking gun for winding-sector
   trapping, independent of the ratio drift => WORM-mandatory, and we do not need
   to argue from drift shape at all.
 - If chains DO exchange sectors but the ratio still converges only slowly =>
   ordinary critical slowing => depth-solvable.
This measures the thing 18b hypothesized instead of inferring it. That is the
whole point of the campaign's "measure, don't guess" discipline.

### DECISION RULE (binary; pre-committed so the outcome is not re-litigated)
Evaluate at 40M on the DECISIVE points (controls must have stayed converged):
 - PASS = depth-solvable, DECISION 1 HOLDS: cold/hot z<3 on BOTH observables at the
   worst points AND ratio increments clearly decelerating. => Read off the depth
   needed for L=10, extrapolate the depth law to L=20/30, close the no-PT overlay
   benchmark at matched depth. Promote benchmark_report only after the L=10/20/30
   overlay matches trend+crossing (unchanged bar).
 - FAIL = winding sector NOT ergodic under local updates: cold/hot still split
   (z>>3) with non-decelerating increments AND/OR zero in-window sector exchanges
   while controls exchange. => PT ALONE cannot fix it (it shuffles configs, adds no
   sector-changing power) => WORM becomes MANDATORY, reorders M0. Biggest finding of
   the campaign. Report immediately; do NOT attempt more depth or a patch.

### PRE-AUTHORIZED TIEBREAK (avoids one needless round-trip)
If at 40M the DECISIVE points are AMBIGUOUS — increments clearly decelerating (so
not the FAIL signature) but cold/hot z not yet <3 — Jarvis may extend the SAME
chains ONCE to 80M sweeps (still <2.5 hr at L=10) before reporting. That is the same
test at more depth, not a redesign. Any OTHER ambiguity, or a control that drifted,
STOP and report. This is the only extension authorized without a fresh read.

### LOOP CONTAINMENT
One ramp (plus at most the pre-authorized 80M tiebreak), then STOP and report the
binary. No ladder work, no PT, no new h-points beyond the spec above. The marker
test is done; the beta-bridge/worm ranking is unchanged and on the shelf. Nothing
else is in scope until the depth-ramp binary is in.

## UPDATE 2026-07-20: depth-ramp PASS accepted (my sector hypothesis is DEAD). L=20 is VOID, not merely ambiguous. L=30 is a NO on the current basis — and the circularity is MY spec's fault. Measure tau, do not extrapolate a guess.

### 1. L=10 depth-ramp: PASS accepted, and my 18b hypothesis (b) is REFUTED
Cold/hot z<3 on both observables at all 7 decisive points including h_c exactly,
controls held (p=0.97/0.84), and the direct probe returned hundreds of thousands
of sector exchanges everywhere. The FAIL signature I specified is absent.
So: winding-sector ergodicity breaking is DEAD as an explanation. The z=29.6 split
at 4M was ordinary critical slowing, resolved by depth. WORM IS NOT MANDATORY, and
Decision 1 stands. That is my third refuted hypothesis this campaign and I want it
recorded as such. The reason it died cleanly in one run is the direct mechanism
probe rather than symptom-watching. Credit Jarvis for the sector-exchange
instrumentation and for the false-timeout traceback (clean record, right call).
tau_int(h_c) ~10,792 sweeps vs the pilot's ~7,375: consistent, reassuring.

### 2. L=20: by my OWN rule this is VOID, not "ambiguous"
UPDATE 18c: "If a control DRIFTS, the ramp itself is suspect and the whole readout
is void." Control h=0.305629 fails (z=4.68 at 16M; no nontrivial suffix passes the
constant test; NONE through 64M). So the L=20 curve is not usable for the overlay
until that is cleared. Jarvis flagged exactly this condition himself; the rule
applies as written. But apply it proportionately: the diagnosis is legible
(under-depth plus a checkpoint ladder too coarse to resolve the relaxation shape),
so the fix is a targeted extension, not discarding 774 task-hours.

### 3. The circularity catch is CORRECT and it is MY error
Jarvis: 64M was chosen by assuming z=2, so recovering z=2 confirms nothing. That is
right, it is the most important sentence in the report, and the flaw was in MY 18c
NEXT line ("read off the depth at L=10, extrapolate the depth law to L=20/30").
Extrapolating a depth law from ONE size REQUIRES assuming the exponent. I built the
circularity into the spec. The worker's refusal to submit L=30 on that basis was
correct judgment and saved ~3,919 task-hours. The added point that a two-point
power law has ZERO residual degrees of freedom and cannot detect curvature is also
right, and it is why the fix below needs four or more sizes, not one more.

### 4. THE NUMBER NOBODY HAS CONNECTED: the sector-exchange rate already measures a timescale, and it is STEEPER than z=2
The exchange counts are per-sample at matched 25-sweep spacing, so the RATE is a
measured dynamical quantity, independent of the depth we chose. At h_c:
  L=10: 233,758 events / 40M sweeps = 1 per 171 sweeps
  L=20:  28,421 events / 64M sweeps = 1 per 2,252 sweeps
  slowdown x13.2 for a doubling of L  =>  apparent exponent ~3.7, NOT 2.
Treat this as a STRONG HINT, not a measured z: the count is threshold-crossings of
an indicator whose distribution itself changes with L, so it is not clean tau_int.
But it points the same way as the failed stationarity tests, and both say the z=2
prior is OPTIMISTIC. Consequence, since cost ~ L^(3+z):
  z=2.0 => L=20->30 is x7.6   ; z=3.7 => x15.3
So the proposed 144M at L=30 is plausibly UNDER-resourced by ~2x. Option (b) buys
a second AMBIGUOUS readout for ~3,919 task-hours. That is the worst available
outcome and I reject it.

### 5. VERDICT ON THE THREE OPTIONS
 (b) submit L=30 on the current guess: NO. Rejected on the arithmetic above.
 (c) drop L=30 on cost alone: premature, but the instinct is right and it is
     cheaper to answer than to assume (see step 2 below).
 (a) extend L=20: YES in part, but narrowed to the two bad points, and it is NOT
     the main event. Plateau-entry depth is a noisy, threshold-dependent quantity
     that costs a full deep run per size. That was the wrong calibration route and
     it was my prescription. The right observable is tau_int, which is measurable
     from runs of ~100 tau instead of ~1000 tau.

### 6. GO — three steps, in order. Roughly 100 task-hours total, versus 3,919.
STEP 1 (FREE, existing data, no new jobs). From the L=10 (15149119) and L=20
(15159599) HDF5 already on disk: (i) extract tau_int of the raw ratio AND the base
density at h_c and across the window at BOTH sizes, with honest error bars, and
report tau_int(20)/tau_int(10); (ii) report the sector-exchange rate scaling above
as a cross-check; (iii) produce the L=10/20 raw-ratio overlay against Linsel Fig.4a
WITH the crossing and bootstrap error bars. Item (iii) is the actual benchmark
deliverable and it is free.
STEP 2 (CHEAP, the real fix for z). Short runs at h_c ONLY, L = 12, 14, 16, 4 cold
+ 4 hot, long enough to RESOLVE tau_int (target ~300 tau total: thermalize ~100 tau,
measure ~200 tau). Estimated ~1-2 h/chain, order 20-60 task-hours all in. Then fit
tau_int(L) over L = 10,12,14,16,20: five points, real degrees of freedom, curvature
detectable, honest error on z. THIS is the campaign-deciding number, worth more
than L=30 (see step 7).
STEP 3 (targeted, clears the void). Extend ONLY h=0.305629 (control) and
h=0.339574 (decisive) at L=20, same chains via checkpoint/restart if available for
this no-PT configuration (G1.1), else rerun those two points from scratch, to 128M
with a GEOMETRIC ladder starting well below 64M (8/16/32/64/96/128M). 16 tasks,
~170-340 task-hours. Clears the control failure so the L=20 curve becomes usable.

### 7. ON L=30: HOLD, and understand what it is actually competing with
L=30 buys crossing DRIFT (a third pair). Trend is already confirmed at every L, and
L=10/20 are two of Linsel's three exact sizes, so the direct overlay is already
possible. Decide L=30 only AFTER steps 1-2, against measured z and against the
crossing you can already see. Two cost levers when we get there:
 - A third size at L=24 instead of L=30 costs ~(24/30)^(3+z) ~ 1/3 as much and
   gives the same crossing-drift information. We lose exact-size overlay aesthetics
   with Fig.4a; we do not lose the science.
 - If the step-1 crossing already lands in/near Linsel's 0.335(2) with the right
   trend and honest bars, the benchmark may be closeable at two sizes. Linsel's own
   nu carried 40% error at these very sizes; the bar is "same trend, same crossing,"
   never "nail h_c."
WHY z OUTRANKS L=30: cost ~ L^(3+z), so L=20 -> L=120 is x7,776 at z=2 and
x169,503 at z=3.7. Either way the no-PT local path CANNOT reach the MCP at L=120 —
which is the campaign's founding premise, now quantified. Measured z converts that
premise into the concrete speedup any algorithm (PT, beta-bridge, worm) must
deliver, and it sets the M2 budget. That is a campaign-level number. A third
benchmark crossing is not.

### 8. STANDING RULE (new, from the L=20 miss)
Checkpoint ladders must be GEOMETRIC and the FIRST checkpoint must sit well BELOW
the expected convergence depth (~1/8 or less). At L=20 the ladder opened at 8M,
already converged at most points, so there was no relaxation SHAPE left to fit.
A checkpoint ladder that starts after convergence measures nothing. Add to the run
manual.

### LOOP CONTAINMENT
Steps 1-3 then STOP and report: measured z with error bars, the L=10/20 crossing
with honest bars, and whether the L=20 control cleared. No L=30, no L=24, no PT
work until that is in.

## UPDATE 2026-07-20b: CORRECTION to my own step-2 rationale, and the FINAL ordering. This supersedes the step order in 2026-07-20. GO.

### CORRECTION (mine, prompted by the PI): benchmark-line z is NOT the M2 budget
In 2026-07-20 I wrote that measured z "sets the M2 budget." That is too strong and
I am correcting it. Every run in that update (L=10 pilot, L=10 depth-ramp, L=20
scale-up, proposed L=30) is at **lmbda=0.3 FIXED** = the Linsel ISING line, which
is OFF the self-dual line and is a CONTINUOUS 3D Ising transition. A z fitted there
is the cost law for the CONTINUOUS case only.
If the MCP is FIRST-ORDER (half of thesis T1, still undecided), the slow mode there
is NOT power-law critical slowing but EXPONENTIAL tunneling between coexisting
phases, tau ~ exp(2*sigma*L^2). No z fitted on a continuous line extrapolates to
that. => The Ising-line z is a BEST-CASE LOWER BOUND on MCP cost. Necessary, not
sufficient. Do not quote it as the MCP budget. This is also exactly the scenario
where beta-bridge/worm becomes mandatory, so yesterday's PASS does NOT retire that
ranking.

### FOR THE RECORD: we have never run at the MCP
The numbers are deceptively close and the trap is live. Benchmark critical point =
(h,lmbda) = (0.335, 0.30). MCP = (s*,s*) with s* ~ 0.33-0.35 (Linsel PRIOR, still
UNMEASURED by us). Nearly the same h, different lmbda. Also: L=10-30 is
benchmark-scale; the MCP FSS design targets L ~ 120-192 at beta=L. The only
post-observable-fix self-dual work is the L=8 duality checks at s=0.22; the one
attempt to locate s* (job 15034719, L=120) returned the null result and predates
the fixes. Locating s* remains an unexecuted step of the master sequence.

### FINAL ORDER (supersedes 2026-07-20 sec 6). Jarvis has the PI's GO.
 STEP 1 — UNCHANGED, FREE, do first (no new jobs). From existing L=10 (15149119)
   and L=20 (15159599) HDF5: tau_int of the raw ratio AND base density at h_c and
   across the window at BOTH sizes with error bars; the sector-exchange rate
   scaling cross-check (x13.2, apparent exponent ~3.7); and the L=10/20 raw-ratio
   overlay vs Linsel Fig.4a WITH crossing and bootstrap bars. Item (iii) is the
   benchmark deliverable and it costs nothing.
 STEP 2 — PROMOTED: LOCATE s* on the SELF-DUAL line (master sequence step 3).
   Wide-s scan, h=lmbda=s, mu=J=1, SMALL L (8-16), both directions, NO
   pre-committed direction, applying the depth lessons we just paid for (cold/hot
   I5 + stationarity, geometric checkpoint ladder starting well below expected
   convergence). This is now the highest-value new compute: it is the first step
   toward the actual target and it is the prerequisite for measuring anything about
   the transition we care about.
   EARLY FIRST-ORDER TELL (cheap, take it while you are there): record the ENERGY
   HISTOGRAM at each s. A BIMODAL/double-peak distribution is strong evidence of a
   first-order MCP and is visible at modest L without the full FSS ladder.
   ASYMMETRY, state it in the report: bimodality PRESENT => strong first-order
   evidence; bimodality ABSENT => INCONCLUSIVE, not evidence of continuity (the
   double peak only develops once L exceeds the interface scale). Do not report
   absence as a continuous result.
 STEP 3 — UNCHANGED: clear the L=20 void. Extend ONLY h=0.305629 (control) and
   h=0.339574 at L=20 to 128M on a geometric ladder (8/16/32/64/96/128M), same
   chains via checkpoint/restart if available for no-PT (G1.1), else rerun those
   two points. ~16 tasks, ~170-340 task-hours. Needed for the L=20 curve to be
   usable in the overlay.
 STEP 4 — DEMOTED, OPTIONAL: the Ising-line z-fit at L=12/14/16. Run it only if
   the continuous-case cost law is wanted for its own sake, and label it
   explicitly as a best-case bound per the correction above. It is no longer a
   prerequisite for anything.
 L=30: still HOLD. L=24 remains the ~1/3-cost alternative if a third size is later
 judged necessary. Decide only after step 1 shows what the two-size crossing
 actually looks like.

### LOOP CONTAINMENT
Steps 1-3 (step 4 optional), then STOP and report: the L=10/20 crossing with honest
bars, whether the L=20 control cleared, and the s* scan with energy histograms and
the bimodality read stated under the asymmetry rule above. No L=30, no L=24, no PT
ladder work until that is in.

## UPDATE 2026-07-20c: NEW STEP 2B — MEASURE PT's SPEEDUP on the benchmark line. PI catch; I had left this assumed. Runs in PARALLEL with the s* scan.

### THE GAP (mine): we have never measured whether PT actually helps
After Decision 1 I parked PT and wrote "resume the self-dual PT path toward the
MCP." That quietly ASSUMED PT delivers a useful speedup, having never measured one.
Same failure pattern that has cost this campaign weeks: assume instead of measure.
State it precisely: S0-S6 validated the PT MECHANISM (swaps are correct). It says
NOTHING about whether PT reduces autocorrelation. Separable claims, exactly like
swap-WEIGHT vs swap-PARTNER. Every PT run this campaign either failed on transport
(4 attempts, 0 round-trips) or was an L=8 correctness smoke test. **PT's speedup
factor is unmeasured.** We are currently planning to deploy it at the MCP on faith.

### WHY IT MUST BE DONE NOW, ON THE BENCHMARK LINE
Speedup = tau_noPT / tau_PT requires a TRUSTWORTHY no-PT baseline. We have exactly
one, just paid for: L=10, tau_int(h_c) ~10,792 sweeps, Neff 1848, controls held
(job 15149119); L=20 follows once step 3 clears the void. At the MCP we will NEVER
have a no-PT reference, because no-PT there is precisely what is infeasible.
=> This measurement is POSSIBLE NOW and IMPOSSIBLE LATER. That decides the timing.
No conflict with Decision 1: that decoupled the benchmark's ANSWER from PT; this
uses the benchmark as a CALIBRATION FIXTURE for PT. Different purpose. Do not
re-litigate Decision 1 on the basis of this step.

### WHY THIS IS NOT A 5TH LADDER ATTEMPT
 - ROUND-TRIPS ARE NOT REQUIRED and are NOT a gate here. The 4 failures chased
   round-trips to CLOSE the benchmark. A speedup measurement needs only tau_int per
   rung. A rung that never exchanges simply returns tau_PT = tau_noPT (speedup 1).
   The measurement is well-defined even in the regime that previously "failed".
 - The h_c hole is now CHARACTERIZED, not mysterious (marker test: 0.235%
   acceptance at one link, every other link 2-3 orders more permeable).
 - We now know the required DEPTH from the no-PT ramp, so the run can be sized
   instead of guessed.

### THE METRIC — cost-normalized, or we will flatter PT
PT costs N times the cores. Raw tau reduction is NOT the figure of merit. Report:
 (i) tau_int and Neff PER RUNG at the PT rung h-values, vs the no-PT tau_int at the
     SAME (h, L) from job 15149119. Speedup(h) = tau_noPT(h)/tau_PT(h).
 (ii) THE DECIDING NUMBER: for EQUAL TOTAL CORE-HOURS, does PT with N rungs give
     more Neff at the h-points of interest than N INDEPENDENT no-PT chains at those
     same N h-values? The no-PT side is already measured, so this is apples-to-
     apples. Quote Neff per core-hour for both.
 (iii) Speedup AT h_c specifically, called out separately from the ladder average.
     The average will be flattered by the easy rungs; h_c is where we need it.
 (iv) PHYSICS AGREEMENT (a real G1.3, at a non-trivial point unlike the L=8
     original): does PT reproduce the same raw-ratio curve and crossing as the
     no-PT run and as Linsel? If PT shifts the crossing, we must know before
     betting the MCP on it.
 (v) Round-trips/transport as CONTEXT only. Zero round-trips does not void this.

### SPEC (small, cheap, one walltime block)
L=10, beta=10, lmbda=0.3, mu=J=1, z-basis, PT ON. Reuse the Lane-2 L=10 14-rung
ladder spanning the h-window (do NOT redesign it; rung placement is not what is
being tested). Depth: thermalize to the no-PT-calibrated plateau entry (~16M
sweeps) then produce >= 200 tau at the slowest rung (~2-3M sweeps at h_c scale);
budget ~20M sweeps total. NOTE PT+checkpoint THROWS, so it must fit ONE walltime
block; at L=10 that is ~0.5-1 h/rank, order 20-30 task-hours all in. Then repeat at
L=20 AFTER step 3 clears that baseline: two sizes give the speedup TREND, which is
what extrapolates toward the MCP.

### DECISION RULE (pre-committed)
 - Cost-normalized speedup at h_c >> 1  => PT is worth deploying at the MCP;
   proceed with the self-dual ladder, and fold the measured factor into the M2
   budget (replacing the current assumption).
 - Cost-normalized speedup at h_c ~ 1   => PT does NOT help where the barrier is,
   so it will not rescue the MCP either (where the barrier is likely worse).
   beta-bridge/worm move UP the ranking, and we do NOT spend MCP-scale compute on a
   plain PT ladder. This is the outcome that would save the most.
 - Intermediate => quote the honest factor; it is the number the M2 budget has been
   missing all along.
Either way this is decision-grade, which is why it outranks the optional step 4.

### ORDERING
Step 1 (free) first, as before. Then 2A (locate s*) and 2B (this) run in PARALLEL —
independent lines, independent jobs, both small-L, both cheap; precedent is the
earlier Lane 1 / Lane 2 split. Step 3 unchanged, and it unlocks 2B at L=20. Step 4
stays optional and is now clearly the lowest value item.

## UPDATE 2026-07-20d: FRESH-EYES REVIEW of 20b/20c. Steps 2A and 2B RATIFIED with additions. But I found a MEASUREMENT-INSTRUMENT DEFECT that changes step 1, cancels step 3 pending a free re-analysis, and makes step 4 invalid as specified. New STEP 0, free, blocks the rest.

New Feynman session. The PI held dispatch so this directive could be reviewed with
fresh eyes rather than inherited. I reviewed it against the existing L=10
(`15149119`) and L=20 (`15159599`) HDF5 on disk. Most of 20b/20c survives. One
thing does not, and it is upstream of almost every number this campaign quotes.

### 0. THE FINDING: our tau estimator silently underestimates, and it decides step 3
`integrated_autocorrelation_time` in `scripts/ensemble_diagnostics.py` (and the
matching stored HDF5 `autocorrelation_time`, which I checked reproduces it to ~1.0x)
is an initial-positive-sequence sum. IPS truncates at the first non-positive
autocorrelation. Our observable has ~95-98% of its variance in a component that
decorrelates within ONE sample, sitting on top of a small-amplitude, very long-lived
tail. Measured rho(lag), lag in samples of 25 sweeps, plaquette_percolation_strength:

  L=10 h=0.326  : r(1)=+0.051 r(100)=+0.022 r(1e4)=+0.019 r(5e4)=+0.012
  L=20 h=0.326  : r(1)=+0.018 r(100)=+0.001 r(1e4)=+0.000 r(5e4)=+0.001
  L=20 h=0.33957: r(1)=+0.016 r(100)=+0.005 r(1e4)=+0.002 r(5e4)=-0.000

IPS integrates the fast component and truncates before the tail. The blocking
curve (tau vs block length, median over the 8 chains) shows what that costs at the
two L=20 points that were declared failures:

  h=0.305629 : 44 / 46 / 45 / 42 / 44 / 39 sweeps at block 16k->8M   = FLAT PLATEAU
  h=0.339574 : 102 / 288 / 881 / 2289 / 3094 / 3300 sweeps           = STILL CLIMBING

So the two "failures" have DIFFERENT causes and neither is what we assumed:
 - h=0.305629 (the CONTROL that voided the run): the blocking curve is flat across
   three decades. tau = ~44 sweeps, genuinely. The statistics are SOUND, the error
   bars are CORRECT, and its full-ladder constant-fit p=0.0528 is ABOVE 0.05. The
   z=4.68 that voided the readout is ONE checkpoint out of five (8M/32M/48M/64M give
   z = 0.28/1.13/0.47/0.46) among ~90 comparisons in the table, and the slope-z is
   1.57, i.e. NOT SIGNIFICANT. There is no drift here. I do not believe this control
   ever failed.
 - h=0.339574 (decisive, and the real tau peak): IPS returns 827 sweeps, blocking
   returns >=3300 and has not plateaued at a block length of 8M sweeps. IPS is low
   by AT LEAST 4x. Its error bars are correspondingly too small and its
   constant-fit p=0.026 is untrustworthy IN BOTH DIRECTIONS. Note the sampling is
   not the problem: Neff = 64M/(2*3300) ~ 9,700, versus a bar of 30.

CONSEQUENCE: the L=20 void is a DATA-ANALYSIS defect, not a DATA-COLLECTION defect.
Step 3 proposes 170-340 task-hours to fix a data-collection problem I do not think
we have. And I verified job `15159599`'s sbatch sets no `checkpoint_path`, so there
is no checkpoint to restart from: step 3 is a full rerun, not an extension, and its
cost is the TOP of the quoted range.

### 1. WHY THIS ALSO BREAKS STEP 1(i) AND STEP 4
tau_int is a variance-weighted quantity: roughly (slow-mode amplitude) x (slow-mode
decay time). Across L=10 -> L=20 the AMPLITUDE falls hard (self-averaging: 4x more
plaquettes, so the global mode is a smaller share of the observable's variance)
while the decay TIME rises. Their product is not a relaxation time and need not be
monotonic in L. Measured with IPS at h=0.335363, tau falls 10,541 -> 35 sweeps from
L=10 to L=20. Read literally that says the dynamics get 300x FASTER with L, which
violates G2 and is impossible.
 - STEP 1(i) as written ("report tau_int(20)/tau_int(10)") would report a ratio far
   below 1 and hand us a false headline.
 - STEP 4 (fit z to tau_int(L) at L=12/14/16) is therefore not merely optional, it
   is INVALID AS SPECIFIED. It would fit the product of a falling amplitude and a
   rising time and return a NEGATIVE z.
This is my own error class again, one level deeper than I had it: I told you to
prefer a directly measured quantity (tau_int) over a choice-contaminated one
(plateau-entry depth). Correct instinct, wrong quantity. tau_int is contaminated
too, by the observable's variance decomposition, and that decomposition depends on L.
The quantity that governs THERMALIZATION is the slow-mode decay time; tau_int
governs ERROR BARS. They are different and I conflated them.

### 2. A SECOND, FREE, POSITIVE RESULT ALREADY IN THE L=20 DATA
Scanning tau across the window, L=20 shows a clean single peak at h=0.339574
(827 sweeps by IPS, >=3300 by blocking) with shoulders at 0.343521 and 0.335363 and
a flat floor of ~20 sweeps everywhere else. That is a textbook pseudo-critical
signal, and it is an INDEPENDENT, DYNAMICAL estimate of the finite-L critical point
from a quantity unrelated to the Binder ratio. It agrees with the PT-era crossings
(0.3408-0.3441) and sits ABOVE Linsel's extrapolated 0.335(2), which is exactly what
finite-size scaling requires. This strengthens the benchmark and costs nothing.
It also means h=0.335362847648 is NOT our finite-L critical point. We have been
quoting "tau at h_c" and the sector-exchange rate at a point that is OFF-PEAK at
both sizes. The x13.2 sector-exchange ratio must be recomputed at each size's OWN
pseudo-critical h before it is used for anything.
L=10 has no comparable peak: its tau rises monotonically toward SMALL h (88,826
sweeps at h=0.318989) with enormous chain-to-chain scatter (min 1,475, max 202,508).
That is the frozen-winding slow mode of the deconfined side (run manual E3), not
critical slowing, and it is another reason the L=20 run is the more informative of
the two, contrary to our treating L=20 as the void one.

### 3. REVISED ORDER. Step 0 is new, free, and blocks the rest.
 STEP 0 — NEW, FREE, NO NEW JOBS. Fix the estimator, then re-analyze what we have.
   (a) Add a blocking-plateau tau estimator to `scripts/ensemble_diagnostics.py`:
       scan block length over decades, require a plateau, and where no plateau is
       reached report a LOWER BOUND and flag the point tau-UNRESOLVED. Keep IPS as
       a cross-check, never as the sole source. Feed the plateau tau into the
       block-bootstrap block size.
   (b) Re-run the depth-ramp readout for BOTH `15149119` and `15159599` with it.
       Re-issue cold/hot z, stationarity p, and Neff at every (L,h).
   (c) PRE-COMMITTED PREDICTION, so this is falsifiable and I do not get to
       reinterpret it afterwards: control h=0.305629 will PASS (flat plateau, p
       already 0.053), and h=0.339574's error bars will grow >=2x with its
       stationarity p rising above 0.05. IF BOTH HOLD, the L=20 void is CLEARED FOR
       FREE and step 3 is CANCELLED. If either fails, step 3 is back on, and then it
       is justified by evidence rather than by assumption.
   (d) Every tau, Neff, and bootstrap error bar this campaign has quoted came from
       the IPS estimator, including "tau_int(h_c) ~10,792 sweeps, Neff 1848" at L=10.
       Treat them all as suspect IN THE DIRECTION OF OVERCONFIDENCE until re-issued.
       Do not re-engineer the C++ stored tau; fix the analyzer and stop trusting the
       stored value.

 STEP 1 — KEPT, FREE, but re-specified. Same three items, with: tau from the
   blocking plateau, not IPS; slow-mode amplitude and decay time reported
   SEPARATELY rather than as their product; each size's own pseudo-critical h taken
   from its tau peak, and the sector-exchange rate ratio recomputed there; the
   overlay's error bars derived from the corrected tau. Add the sec-2 tau-peak
   result to `doc/benchmark_report.md`.

 STEP 2A — LOCATE s*. RATIFIED as written. Three additions:
   (i) Run BOTH BASES and apply the I2/B5 duality check at EVERY scanned s, not only
       at s*. On the self-dual line duality must hold everywhere along it, so this is
       the sharpest available check that we are actually on the line and that the
       instrument is sound. At L=8-16 it is nearly free. The directive omitted it.
   (ii) Energy histograms: report POOLED cold+hot AND per-chain separately. With
       local updates at small L a first-order point does NOT show as a bimodal
       single-chain histogram; it shows as cold and hot chains sitting at two
       distinct, stable, depth-persistent energies. Report the cold/hot energy gap
       versus depth explicitly.
   (iii) THEREFORE: a persistent cold/hot gap at s* must NOT be auto-diagnosed as
       under-thermalization and fed more depth. Run manual E2 says cold/hot may
       NEVER converge at a first-order point. Distinguish by whether the two groups
       approach each other with depth (under-thermalized) or hold two stable
       separated values (coexistence). Getting this backwards burns MCP-scale
       compute chasing a convergence that cannot happen.
   The asymmetry rule on bimodality is correct as written; keep it verbatim.
   Expectation to set now: 2A's deliverable is s* with a bracket. At L=8-16 the
   order will most likely read INCONCLUSIVE, and that is a success, not a failure.

 STEP 2B — MEASURE PT's SPEEDUP. RATIFIED. This is the best item in 20c and the PI
   was right to force it. Three additions:
   (i) PRE-CHECK BEFORE SUBMITTING, cheap and fatal if wrong: verify whether the PT
       HDF5 observable series is indexed by MPI RANK or by LADDER POSITION. The
       measurement needs tau at FIXED h. If the series is per-rank while ladder
       labels migrate, "tau at the h_c rung" is not what is stored and the whole run
       is void. This is the same rank-versus-ladder-position bookkeeping that
       already produced one real bug in this code. Check it first.
   (ii) Use the STEP 0 estimator for both sides. A speedup built from two IPS
       numbers is a ratio of two biased quantities, and the bias is not guaranteed
       to cancel: it depends on the slow-mode amplitude, which PT changes.
   (iii) Tighten the decision-rule wording. Cost-normalized speedup ~1 licenses
       "a PLAIN SINGLE-PARAMETER h-LADDER does not help", NOT "PT does not help".
       The beta-bridge is a ladder in a DIFFERENT direction and is not refuted by
       this measurement. The 20c text mostly says this; the tracker card overstates
       it, and I am fixing the card.

 STEP 3 — DOWNGRADED from authorized to CONDITIONAL on step 0(c). Do not spend
   170-340 task-hours until the free re-analysis says the void is real. If it is
   real, note it is a full rerun (no checkpoint exists for `15159599`), so quote the
   top of the range.

 STEP 4 — INVALID AS SPECIFIED, not merely optional. Do not fit z to tau_int(L).
   If the continuous-case cost law is wanted, re-specify it on blocking-converged
   tau at each size's OWN pseudo-critical h, or on the sector-exchange rate. Either
   way it stays a best-case lower bound on MCP cost per my 20b correction.

 L=30 and L=24: unchanged, still HOLD.

### 4. WHAT I AM RATIFYING WITHOUT CHANGE
The 20b correction (benchmark-line z is a best-case bound, not the M2 budget), the
scope fact that we have never run at the MCP, the promotion of locate-s* above the
z-fit, the entire rationale of 2B, the bimodality asymmetry rule, and the loop
containment. Those are right and I am not relitigating them.

### LOOP CONTAINMENT
Step 0 first and report its outcome before anything else moves; it is free and it
decides step 3. Then step 1 (free), then 2A and 2B in parallel. Then STOP and
report: the re-issued L=10/20 validity table with honest tau, whether the void
cleared, the L=10/20 crossing with corrected bars, the s* bracket with the energy
histograms and the cold/hot gap read, and the cost-normalized PT speedup at h_c.
No L=30, no L=24, no step 3 unless step 0 says so.

## UPDATE 2026-07-20e: REFINEMENT of 20d for the actual execution order. Step 1 is in flight and lands FIRST; Step 0 becomes the immediate next action. Triage rule for Step 1's output, and a fully executable Step 0 spec.

Context change since 20d: Jarvis had already been dispatched and is finalising
STEP 1. The PI is stopping him after it. So the order 20d assumed (0 then 1) is
inverted in practice. That is fine and costs nothing, provided Step 1's output is
triaged correctly on arrival. This update supplies that triage and makes Step 0
executable end to end so it can be run without another round with me.

### 1. TRIAGE RULE for the in-flight Step 1 output. One line to remember:
**THE MEANS ARE FINE. EVERY ERROR BAR IS SUSPECT.**
The IPS defect (20d sec 0) mis-estimates the autocorrelation time. It does NOT bias
the sample means. So on arrival, split Step 1's deliverables:

 KEEP AND USE:
  - The raw-ratio curve SHAPES and the pooled MEANS at every (L,h), both sizes.
  - The crossing LOCATION from the L=10/20 overlay.
  - The sector-exchange RATE and its recomputation at each size's own tau peak.
    A counting rate needs no tau and is untouched by the defect.
  - The identification of each size's pseudo-critical h from where tau PEAKS. The
    peak LOCATION is robust even though the peak HEIGHT is underestimated, because
    IPS is biased low roughly uniformly across the window.

 QUARANTINE — do not quote, do not enter in the tracker, do not put in a figure:
  - Every error bar, bootstrap or otherwise, at both sizes.
  - Every Neff.
  - Every tau value, and above all any tau_int(20)/tau_int(10) RATIO. If Step 1
    reports that ratio it will be below 1 and will read as "the dynamics get faster
    with L". That is the artifact of 20d sec 1, not a result. It must not enter the
    record even as a provisional number.
  - The crossing's quoted UNCERTAINTY, and therefore any statement about whether we
    agree with Linsel's 0.335(2). Trend and location yes; significance no.
  - Any promotion of `doc/benchmark_report.md` sec 5 out of PROVISIONAL. The bar has
    always been "same trend, same crossing"; we can speak to both now, but not to
    consistency within errors until Step 0 re-issues the bars.

 REPORT IT ANYWAY, clearly labelled. A quarantined number is not a deleted number.
 Step 1's figure is still the benchmark deliverable; it just carries provisional
 bars with a one-line caveat until Step 0 lands.

### 2. STEP 0 — full spec. Free, no new jobs, immediate next action.
Purpose: replace the truncating tau estimator, then re-issue the validity tables for
`15149119` (L=10) and `15159599` (L=20). Three legs; (b) is the one I most want.

 (a) BLOCKING-PLATEAU tau, added to `scripts/ensemble_diagnostics.py` beside the
     existing IPS function (do not delete IPS; demote it to a cross-check).
     For block length b on a geometric ladder b = 1,2,4,...,b_max in SAMPLES:
       tau(b) = 0.5 * b * Var(block means) / Var(series)      [sample units]
       n_b    = N / b ;   relative error on tau(b) ~ sqrt(2/(n_b - 1))
     Cap b_max at the largest b with n_b >= 16 — below ~16 blocks the block-mean
     variance is too noisy to read a plateau from, and reading one there is how you
     talk yourself into a number.
     PLATEAU := tau(b) agrees within its own error across >= 3 consecutive
     doublings. Report the plateau value.
     NO PLATEAU by b_max := report tau(b_max) as a LOWER BOUND and flag the point
     **TAU-UNRESOLVED**. A tau-unresolved point is not a failed point; it is a point
     whose error bar we cannot yet honestly quote, and it must be labelled as such
     rather than silently assigned the last value on the ladder.
     Per chain, then report median across the 8 chains plus the min/max spread.
     I4 already requires per-chain tau within a factor 2; flag violations.
     Feed 2 * tau_plateau into the block-bootstrap block size.

 (b) BETWEEN-CHAIN ERROR — a tau-FREE cross-check, and the cleanest one available.
     We have 8 independent chains per point. The error on the pooled mean is then
     simply std(chain means)/sqrt(8), with NO autocorrelation model at all: each
     chain is one independent draw, so a slow mode with a decay time comparable to
     the run length is captured correctly by construction. It carries ~27% relative
     error at 8 chains, which is plenty to expose a factor-4 discrepancy.
     **Report the between-chain error side by side with the bootstrap error at every
     (L,h), and report their RATIO.** That ratio IS the direct measurement of the
     estimator defect, it requires no tau, and it settles the question without
     depending on the very machinery under audit. If the two agree, my 20d diagnosis
     is wrong at that point and I want to know immediately. This is the cheap direct
     probe for this problem and it should be run first within Step 0.
     Caveat to state in the report: between-chain error is blind to a drift COMMON
     to all chains (G1b). It cross-checks the ERROR BAR, it does not replace the
     stationarity test.

 (c) RE-ISSUE, for both jobs, at every (L,h): tau (plateau value or lower bound +
     flag), Neff, bootstrap error, between-chain error and their ratio, cold/hot z,
     and the suffix stationarity p. Same table shape as
     `DEPTH_LAW_AND_DESIGN.md` so the two are directly comparable.

### 3. PRE-COMMITTED PREDICTIONS. Unchanged from 20d, restated so they bind.
 - L=20 control h=0.305629 PASSES: flat blocking plateau ~44 sweeps, bootstrap and
   between-chain errors AGREE, full-ladder p already 0.0528 > 0.05.
 - L=20 h=0.339574: bars grow >= 2x, tau resolves to >= 3300 sweeps or comes back
   TAU-UNRESOLVED, stationarity p rises above 0.05.
 IF BOTH HOLD: the L=20 void is CLEARED FOR FREE, step 3 is CANCELLED, and the L=20
 curve becomes usable in the overlay with honest bars.
 IF EITHER FAILS: step 3 is back on, justified by evidence rather than assumption,
 and quoted as a full rerun (no `checkpoint_path` in `15159599`).
 IF THE (b) RATIOS COME BACK ~1 EVERYWHERE: I am wrong about the defect. Say so
 plainly, keep the existing bars, and the void stands as originally diagnosed. I
 would rather be corrected here for free than after step 3 has been paid for.

### 4. ORDERING FROM HERE
 Step 1 lands (in flight) -> triage per sec 1 -> STEP 0 (free) -> report.
 Then, and only then, 2A and 2B in parallel per 20d. Step 3 stays conditional on
 sec 3. Step 4 stays invalid as specified. L=30 and L=24 stay HOLD.

### LOOP CONTAINMENT
Stop after Step 0 and report: the re-issued validity table, the bootstrap-versus-
between-chain error ratios, and a one-line verdict on whether the L=20 void cleared.
Do not start 2A or 2B in the same breath — the void answer changes what the L=20
baseline is worth to 2B.

## UPDATE 2026-07-20f: I WAS WRONG ABOUT THE ERROR BARS. The tau-free cross-check refutes my own 20d claim. The L=20 VOID IS REAL, step 3 is BACK ON. What survives: tau_int is still the wrong instrument for z, and Step 1's apparent z = -7.65 must still be quarantined.

Step 1 landed while 20e was being written, so I ran the 20e leg-(b) cross-check
myself on the existing HDF5 rather than wait. It refutes me. Recording that first,
before anything else, per the pre-committed rule in 20e sec 3.

### 1. THE REFUTATION — clean, and by the probe I specified
Between-chain error on the pooled raw ratio, std(chain means)/sqrt(8), needs NO
autocorrelation model at all: 8 independent chains, 8 independent draws. Against the
quoted bootstrap error:

  L=10 h=0.322      1.1x        L=20 h=0.322      0.8x
  L=10 h=0.335363   1.1x        L=20 h=0.326      1.2x
                                L=20 h=0.335363   1.0x
                                L=20 h=0.343521   0.8x
  AND AT THE TWO POINTS I ACTUALLY DISPUTED:
    L=20 h=0.305629   between-chain 0.000306 vs bootstrap 0.000398 = 0.77x
    L=20 h=0.339574   between-chain 0.002737 vs bootstrap 0.002768 = 0.99x

Ratios ~1 everywhere, including both disputed points. **The bootstrap error bars are
CORRECT.** And the per-chain ratios at h=0.305629 are 1.3999 1.4005 1.3997 1.4003
1.4001 1.4010 1.3995 1.3981 — eight independent chains agreeing to four decimals.
That is not a point with a hidden slow mode. That is a beautifully converged point.

### 2. WHAT THIS KILLS — my 20d sec 0, and the consequences I drew from it
 - **The L=20 VOID IS REAL** and stands exactly as the retired session diagnosed it.
   My "it is an analysis artifact" claim is dead. Step 3 goes back to AUTHORIZED,
   not conditional. The retired session was right and I was wrong to dispute it.
 - The pre-committed prediction in 20d/20e sec 3 FAILED, and it failed in the
   direction that costs money: step 3's 170-340 task-hours are needed after all.
 - Where I went wrong, precisely: I read a rising blocking curve at L=20 h=0.339574
   (up to 3300 sweeps) as an unresolved slow mode. At those block lengths there were
   only 8-16 blocks, so that rise was noise. **My own 20e spec contains the warning
   that would have caught it** — "below ~16 blocks the block-mean variance is too
   noisy to read a plateau from, and reading one there is how you talk yourself into
   a number." I wrote that caution and then did the thing it warns against, one
   update earlier. The lesson is not new; the lesson is that I applied it to Jarvis's
   future work and not to my own already-drawn conclusion.
 - Standing correction to my sec-0 blanket claim: prior tau/Neff/error bars are NOT
   globally suspect. Withdraw that. The error bars were fine all along.

### 3. WHAT SURVIVES, and it is the half that matters for the physics
The two claims in 20d were separable and only one is dead:
 (A) tau_int is the WRONG INSTRUMENT for a dynamic exponent or a thermalization
     cost law.  **SURVIVES, and is now SHARPER.** tau_int is the variance-weighted
     integral. That is exactly the right quantity for an ERROR BAR, which is why the
     cross-check passes. It is the wrong quantity for a RELAXATION TIME, because it
     is (slow-mode amplitude) x (decay time) and the amplitude falls with L through
     self-averaging. Both facts are now confirmed simultaneously: the errors are
     right AND tau_int falls with L. No contradiction. tau_int does its own job
     correctly and cannot do the other job at all.
 (B) the error bars are underestimated, so the void is an artifact.  **REFUTED.**

### 4. STEP 1's OUTPUT — the quarantine still applies, for reason (A)
Step 1 reports `hc_raw_ratio_tau20_over_tau10 = 0.00498` and
**`hc_raw_ratio_apparent_z = -7.65 +/- 0.52`**. A dynamic exponent of MINUS SEVEN is
not a measurement of anything. It is precisely the artifact predicted in 20d sec 1,
and it must NOT enter the tracker, the benchmark report, or any figure, even
provisionally. Same for `hc_base_density_apparent_z = +0.195`: the base density has
tau ~19-21 sweeps at BOTH sizes, i.e. it sits at the 25-sweep sampling floor
everywhere and carries no slow mode, so its "z" is measuring the sampling interval,
not the dynamics. Neither number is evidence. Step 4 remains INVALID AS SPECIFIED.
Everything else in Step 1 is now UNQUARANTINED: with the error bars vindicated, the
means, the curve shapes, the bars themselves, and Neff are all usable.

### 5. THE CROSSING NEEDS A SECOND LOOK, AND THIS IS NOW THE OPEN QUESTION
Step 1 reports crossing h = 0.32441 +/- 0.00082. That sits BELOW Linsel's
extrapolated 0.335(2) and far below the PT-era crossings 0.3408-0.3441, with an
error bar tighter than the PT-era spread of 0.003. Finite-L crossings should sit
ABOVE the extrapolated value, and every previous run of ours did. Before this is
reported as a benchmark result, note what went into it: **both EXCLUDED_VOID points
are L=20, and one of them is h=0.339574** — which is exactly where I measured the
L=20 tau peak, i.e. the pseudo-critical point. The L=20 curve therefore jumps
1.4832 at h=0.335363 straight to 1.7027 at h=0.343521 with the steepest interval
removed. Dropping the steep segment of one of two curves will move a crossing.
 => Do not promote this crossing. Re-derive it after step 3 restores h=0.339574,
 and report the crossing WITH and WITHOUT the excluded points so the sensitivity is
 visible rather than buried. If the crossing moves back toward 0.34 when the point
 is restored, the exclusion was the cause and the 0.32441 was never real.

### 6. REVISED ORDER
 STEP 3 — **AUTHORIZED** (was conditional). Extend h=0.305629 and h=0.339574 at
   L=20 to 128M, geometric ladder 8/16/32/64/96/128M. Full rerun: job `15159599`
   set no `checkpoint_path`, so quote the TOP of the 170-340 task-hour range. This
   is now the unblocker for both the crossing (sec 5) and the L=20 PT baseline.
 STEP 0 — **CANCELLED as an audit.** Its premise is refuted; there is no error-bar
   defect to fix. Keep ONE piece of it as cheap permanent hygiene, not as a fix:
   add the between-chain error, std(chain means)/sqrt(N_chains), as a routine column
   beside the bootstrap error in `scripts/ensemble_diagnostics.py`. It is nearly
   free, it needs no tau, and it would have settled this in one command instead of
   two updates. Do NOT rebuild the tau estimator; IPS is fine for its actual job.
 STEP 4 — still INVALID as specified (reason (A), unaffected by the refutation).
 STEPS 2A / 2B — unchanged from 20d, including all the additions there. 2B's L=20
   arm still waits on step 3.
 L=30, L=24 — still HOLD.

### 7. FOR THE RECORD
That is the fourth hypothesis of mine this campaign to be refuted by a cheap direct
probe, and the first one I have managed to refute myself before it cost a run. The
probe cost one command. The two updates of reasoning that preceded it did not.
Credit where it is due: the retired session's L=20 void call was correct, and
Jarvis's original EXCLUDED_VOID handling of those two points was the right call on
the data as it stood.

### LOOP CONTAINMENT
Step 3 is the next compute. Report the crossing with and without the excluded
points, per sec 5. 2A and 2B proceed in parallel per 20d.


## UPDATE 2026-07-20g: VERDICT on Jarvis's STEP 0 + STEP 1. The void is CLEARED — and my 20f self-refutation was itself too hasty. One coherent picture now explains all the evidence. STEP 3 is CANCELLED as scoped. The next action is FREE: re-derive the crossing with the excluded points restored.

Jarvis's Step 0 is the best-executed item of this campaign: a 4-round review that
caught a gameable plateau criterion, a median that would have hidden unresolved
chains, and a bootstrap ratio computed against the wrong quantity. Two of his
catches change my verdict. Taking them in order.

### 1. RATIFIED IMMEDIATELY: the sqrt(7/8) degeneracy. Excellent catch.
Jarvis is right and the algebra is exact. A delete-one jackknife SE of a simple
MEAN is an algebraic identity of the ddof=1 sample SE, so comparing it against a
ddof=0 version returns sqrt(7/8) = 0.935 for ANY data, always, by construction. It
is not a diagnostic, it is a constant. The between-chain check only has content for
a NONLINEAR observable such as the raw ratio <P^4>/<P^2>^2, whose bootstrap error
genuinely depends on tau through the block sizing.
ACTION: make the driver REQUIRE an explicit observable and REFUSE to emit a
between/bootstrap ratio for a linear one, rather than printing 0.935 for a future
reader to mistake for a measurement. A diagnostic that cannot fail is not a
diagnostic. Add the identity as a one-line comment at the call site so nobody
re-derives it in six months.

### 2. I OWE A CORRECTION: my 20f self-refutation was too hasty
I have now held three positions in one day — 20d "the void is an error-bar
artifact", 20f "I was wrong, the void is real", and now this. That is one too many
and the fault is a badly specified decision rule, mine. Here is the single picture
that fits ALL the evidence, ours and Jarvis's, with no remaining contradiction:

 - ~95-98% of the raw ratio's VARIANCE sits in a component that decorrelates within
   one 25-sweep sample. Errors on the mean are therefore set by the fast component
   and are CORRECT. That is why the between/bootstrap ratio sits at 0.49-1.02 across
   all 18 points and never shows the inflation I predicted. **20f's refutation of
   the error-bar claim STANDS.**
 - The slow component is real, carries little variance, and lives long. IPS
   truncates before it. Jarvis's blocking estimator finds TAU-UNRESOLVED with lower
   bounds >=2157/1537/954/792 sweeps at L=20 where IPS reported 18-27. That is a
   40-100x underestimate. **20d's estimator claim STANDS.**
 - Both are true at once because they concern different quantities: the fast
   component dominates the VARIANCE (so the error bar is right) while the slow
   component dominates the TIME (so IPS's tau is wrong). No contradiction.
 - The stationarity test consumes tau through the BLOCK SIZING. So correcting tau
   changes that test's verdict — and it did: h=0.339574 moves from p=0.0260 to
   **p=0.20**, and h=0.305629 to **p=0.33**, with cold/hot z = 0.42 and 0.33.

**MY ERROR IN 20d/20e, precisely**: I made "the error bars grow >=2x" a NECESSARY
clause of the prediction. That is a MECHANISM, and the mechanism was wrong. The
thing that actually decides the question is the OUTCOME: do the two points pass the
validity gates. I should have pre-committed on the outcome and let the mechanism
fall where it fell. Jarvis then applied my rule exactly as written and correctly
reported "STILL REAL per your own decision rule" — the rule was at fault, not his
reading of it. Never pre-commit a prediction on a mechanism when the decision only
needs an outcome.

### 3. VERDICT: THE L=20 VOID IS CLEARED. Step 3 is CANCELLED as scoped.
Judged on the gates rather than on my broken mechanism clause, both formerly-void
points now PASS: cold/hot z well under 3, suffix stationarity p well above 0.05,
Neff enormous. The original void rested on p=0.026 computed with a tau that was
40-100x too small. With tau corrected, that basis is gone.
What remains unresolved at h=0.339574 is TAU ITSELF, not the mean, not the
crossing. Jarvis saw this and asked the right question, so I will answer it
plainly: **a trustworthy tau at that point buys us nothing we currently need.** Tau
enters the crossing only through error bars that are already validated by the
tau-free between-chain check. So the full 128M rerun is CANCELLED. We do not spend
170-340 task-hours to sharpen a number that no downstream decision reads.
The tau-unresolved FLAG stays on those points and is reported with them. An honest
"we cannot resolve tau here at 64M" is a perfectly publishable statement.

### 4. THE NEXT ACTION IS FREE, AND IT IS THE ONE THAT MATTERS
STEP 1's crossing came out h = 0.3244098 +/- 0.0000817, which Jarvis reports as ~5
sigma from Linsel's 0.335(2) and correctly declines to spin. Before that tension is
treated as a physics result, note what produced it: **both EXCLUDED_VOID points are
L=20, and one is h=0.339574** — the steepest interval of the L=20 curve, which
jumps 1.4832 at h=0.335363 straight to 1.7027 at h=0.343521 with the point between
them deleted. Removing the steep segment of one of two curves moves their crossing.
Those points are no longer void (sec 3). So:
 (a) RE-DERIVE the crossing with ALL NINE L=20 points restored. Free, existing data.
 (b) Report the crossing WITH and WITHOUT the two restored points, side by side, so
     the sensitivity is visible rather than buried in a pipeline flag.
 (c) The quoted +/-0.0000817 is NOT CREDIBLE and must not be published. It is a
     crossing between two interpolated curves, one of which (L=10) Jarvis notes is
     NONMONOTONIC near the crossing. That systematic dwarfs the statistical error.
     Propagate an interpolation systematic — vary the interpolation scheme and the
     fit window and report the spread — before quoting any sigma against Linsel.
     For calibration: our own PT-era crossings spanned 0.3406-0.3437, a spread of
     0.003, i.e. 37x the quoted bar. A new method disagreeing with the old by 0.016
     is a systematic, not a 5-sigma discovery.
 PREDICTION, pre-committed on the OUTCOME this time: restoring h=0.339574 moves the
 crossing UP, toward the 0.34 region where every previous run of ours has landed.
 If it does, the exclusion caused the tension and there is no anomaly to explain.
 If the crossing stays near 0.324 with all points in and a defensible systematic,
 THEN we have a real tension with Linsel worth chasing, and I will say so.

### 5. THE tau_int ANOMALY — explained, not an estimator bug, and it does not gate 2A/2B
Jarvis flagged L=20's tau being nearly h-independent at ~18-27 sweeps as suspect and
was right to. The explanation is in 20d sec 1 and Jarvis's own blocking numbers
confirm it: tau_int is (slow-mode amplitude) x (decay time). Going L=10 -> L=20 the
amplitude FALLS through self-averaging (4x the plaquettes, so the global mode is a
smaller share of the observable's variance — measured directly: the autocorrelation
tail drops from ~0.022 to ~0.001) while the decay time RISES. IPS reports the
variance-weighted product and truncates the tail, so it returns a small, nearly
h-independent number. The blocking estimator, which does not truncate, correctly
returns TAU-UNRESOLVED with large lower bounds at the same points. Nothing is broken;
the two estimators disagree because IPS is answering the wrong question.
CONSEQUENCE, unchanged and now doubly supported: **tau_int cannot yield a dynamic
exponent.** Step 1's `hc_raw_ratio_apparent_z = -7.65 +/- 0.52` and
`tau20/tau10 = 0.00498` stay QUARANTINED and out of every doc and figure. Jarvis's
instinct to distrust them was correct. Likewise `hc_base_density_apparent_z = +0.195`:
base-density tau sits at 19-21 sweeps at BOTH sizes, i.e. pinned at the 25-sweep
sampling floor, so that ratio measures our sampling interval, not the dynamics.
And Jarvis is right that the sector-exchange z~3.7 hint is therefore NOT
independently cross-checked. It remains a single unconfirmed hint. Do not upgrade it.

### 6. THE TAU-PEAK CLAIM — Jarvis is right, I overstated it
I claimed a clean isolated L=20 tau peak at h=0.339574. Jarvis's blocking bounds
show 0.326 and 0.330615 are ALSO unresolved (>=954, >=792), so "isolated peak over a
flat floor" is NOT established; those neighbours may be one broad slow region. The
peak LOCATION is still consistent with h=0.339574 carrying the largest bound, and
that is all I should have said. Downgrade my sec-2 claim in 20d from a result to
"consistent with, not established", and do not put it in benchmark_report.md as a
dynamical determination of the pseudo-critical point. Same for the L=10 monotonic
rise toward small h: suggestive of the frozen-winding mode, but every L=10 point is
tau-unresolved, so it is not resolved either.

### 7. ORDER FROM HERE
 (a) FREE, NOW — re-derive the crossing per sec 4 (all nine L=20 points, with/without
     comparison, honest interpolation systematic). Update benchmark_report.md; keep
     it PROVISIONAL either way. This is the benchmark deliverable.
 (b) Then 2A (locate s*) and 2B (PT speedup) IN PARALLEL, both exactly as specified
     in 20d including every addition there — 2A's both-bases I2/B5 duality check at
     every scanned s, the pooled and per-chain energy histograms with the cold/hot
     energy gap versus depth, the E2 warning against auto-diagnosing a persistent
     cold/hot gap as under-thermalization, and the bimodality asymmetry rule
     verbatim; 2B's rank-versus-ladder-position pre-check before submitting, and the
     wording limit that speedup~1 licenses "a plain single-parameter h-ladder does
     not help", NOT "PT does not help".
     2B's L=20 arm is UNBLOCKED now — the void is cleared, so the L=20 no-PT baseline
     exists and no longer waits on a rerun.
 (c) STEP 3: CANCELLED as scoped. STEP 4: still INVALID as specified. L=30 and L=24:
     still HOLD.

### 8. CREDIT
Jarvis's sqrt(7/8) catch, the 4-round review that killed a gameable plateau
criterion, the refusal to report the tau ratio as a confirmed cross-check, the
EXCLUDED_VOID marking rather than silent inclusion, and the question about what
step 3 actually buys — that last one is what cancelled a 170-340 task-hour job.
Four correct calls in one report. My own contribution this round was one right
diagnosis, one wrong mechanism, and a decision rule that made the wrong mechanism
load-bearing.

### LOOP CONTAINMENT
Do (a), report the two crossings side by side, then launch 2A and 2B in parallel and
report per 20d. No step 3, no L=30, no L=24.

## UPDATE 2026-07-20h: PI CORRECTION — round-trips are NOT NECESSARY for low autocorrelation. Run manual F3/F7 are WRONG as written and are fixed here. PT operational calibration promoted to an explicit 2B deliverable.

The PI's point: round-trips are not needed for low autocorrelation times. That is
correct, it is sharper than what we have written down, and it invalidates a bar we
have been enforcing for four failed ladder attempts.

### 1. THE CORRECTION, stated precisely
Run manual F7 currently says round-trips are "NECESSARY but NOT SUFFICIENT". The
second half is right (F1/C7: a label can round-trip on a frozen config). **The first
half is wrong.** A full end-to-end round trip is a SUFFICIENT condition for a
replica to have decorrelated — it guarantees a visit to a reservoir rung where the
slow mode is fast — but it is not a NECESSARY one. What we actually need is for the
replica sitting at the h of interest to escape its free-energy basin. If the basin
is escaped by excursions over a few rungs, tau drops with zero completed round
trips. Diffusing all the way to the far end is one way to buy decorrelation, not the
only way, and it is the most expensive way (tau_RT ~ N^2/A_harmonic).
=> **The correct gate is tau at the rung of interest, measured. Not round-trips.**
Round-trips stay as CONTEXT and as a transport diagnostic, never as a pass/fail bar.

### 2. WHAT THIS COSTS US IN HINDSIGHT
Four ladder attempts were judged NO-GO on ">=3 strict round-trips or the ladder is
broken" (F3). Not one of them measured tau at h_c. It is therefore an OPEN QUESTION
whether any of those ladders was actually delivering a useful speedup while being
scored as a failure on a metric that does not gate the thing we care about. I am not
claiming they were fine — the marker test showed a genuine 0.235%-acceptance barrier
at h_c, and that is real. I am saying we never measured the quantity that decides
it, so "4 consecutive failures" is weaker evidence against PT than it has been
treated as. This is the same MECHANISM-vs-BENEFIT conflation as before, now on the
diagnostic side: we scored transport and inferred decorrelation.

### 3. THE NUANCE THAT MATTERS FOR THE MCP — and it cuts the other way
Round-trips being unnecessary holds when the slow mode is LOCAL, i.e. critical
slowing at a continuous transition. It does NOT obviously hold at a FIRST-ORDER
point. There the slow mode is tunneling between coexisting phases, and a replica
escapes only by reaching a rung where the coexistence gap has closed — which is a
round-trip-like excursion in all but name. So:
 - continuous MCP => round-trips unnecessary, local excursions suffice, PT plausibly
   cheap.
 - first-order MCP => the reservoir must actually be reached, round-trips become
   close to necessary again, and the h_c acceptance hole is the physics rather than
   a ladder defect (E2/F8, H5).
**Which regime we are in is precisely thesis T1, still undecided.** So do not
generalize the corrected rule to the MCP in advance. It is correct on the benchmark
line, which is continuous by construction, and that is where 2B runs.

### 4. DOC FIXES (do these, they are cheap and they are load-bearing)
 - `doc/paratoric_run_manual.md` F3: delete the ">=3 strict round-trips or the ladder
   is broken" PASS/FAIL bar. Replace with: round-trips are reported as transport
   context; the gate is measured tau at the rung of interest.
 - `doc/paratoric_run_manual.md` F7: correct "NECESSARY but NOT SUFFICIENT" to
   "NEITHER NECESSARY NOR SUFFICIENT — sufficient evidence of mixing when genuine,
   but not required for a tau reduction; and a label can round-trip on a frozen
   config, so they are not sufficient either." Add the first-order caveat from sec 3.
 - `doc/pt_architecture.md`: same correction wherever the round-trip bar appears, and
   record that the 4 NO-GO calls used a metric that does not gate decorrelation.
 - Gate G1.2 in `doc/multicritical_execution_harness.md` carries the same ">= 3 round
   trips" language. Correct it identically; a harness gate is the worst place to
   leave a wrong bar.

### 5. PROMOTED: PT OPERATIONAL CALIBRATION IS NOW AN EXPLICIT 2B DELIVERABLE
The PI is right that we should bank the operational knowledge rather than only the
go/no-go. 2B already runs the machinery; make it report the numbers we will need to
design any future ladder, at BOTH L=10 and L=20 (the L=20 arm is unblocked now that
the void is cleared):
 (i)  SWAP PERIOD: tau at the rung of interest as a function of swap_period in
      SWEEPS (try ~1, 2, 5, 10 sweeps). There is an optimum — too short is label
      travel on a frozen config (C7/F1), too long wastes the ladder. We have never
      located it. Report the curve, not a single point.
 (ii) SWEEPS-TO-DECORRELATE vs L at fixed ladder: the actual quantity a budget needs.
 (iii) PER-LINK ACCEPTANCE vs rung spacing dh, with the harmonic mean called out
      separately from the median (Lane 1 established the harmonic mean is what
      governs transport).
 (iv) EXCURSION STATISTICS instead of round-trips: distribution of how many rungs a
      replica travels from its starting position before returning, and the
      correlation between excursion range and tau reduction at that rung. THIS is
      the diagnostic that replaces F3, and it directly tests the PI's point.
 (v)  Everything already in 20c: cost-normalized Neff per core-hour, speedup at h_c
      called out separately from the ladder average, and the G1.3 physics check.
Deliverable: a short PT calibration table in `doc/pt_architecture.md` that a future
run can size a ladder from without re-deriving any of it.

### 6. WHAT DOES NOT CHANGE
Decision 1 stands: the benchmark's ANSWER is decoupled from PT and closes with local
updates, as Linsel did. 2B uses the benchmark as a CALIBRATION FIXTURE. This update
does not reopen the PT-closes-the-benchmark route; it fixes the diagnostic we judge
PT by, and it banks the operational numbers.
Also unchanged: N is not a free knob (A~23% is the tau_RT optimum, more rungs is
worse past that), swap_period is in SWEEPS scaled ~L^2, and PT+checkpoint throws so
any PT run fits one walltime block.

### LOOP CONTAINMENT
Do the sec-4 doc fixes with the sec-7(a) free crossing re-derivation from 20g, then
run 2A and 2B in parallel with 2B carrying the sec-5 deliverables. Report per 20g.

## UPDATE 2026-07-20i: VERDICT on 2A/2B. Jarvis's "inconclusive" calls are both correct, but I found the CAUSE underneath them and it is one disease, not three. The L=10 baseline FAILS I4 by four orders of magnitude in exactly the crossing region. 2B's "physics disagreement" flag is probably PT being RIGHT. New figure of merit for PT: ERGODICITY, not speed.

Both readouts are honest and both stop in the right place. I am not overturning
either call. I went underneath them and the three loose ends share one cause.

### 1. THE FINDING: our L=10 benchmark baseline is not ergodic at low h
Per-chain readout of job `15149119` (the trustworthy baseline everything leans on),
raw ratio and tau_int per chain, 4 cold then 4 hot:

  h=0.305629  ratios 1.7426 1.7245 1.7418 1.7421 | 1.2993 1.7397 1.7391 1.4531
              tau/1e3    0.0    0.0    0.0    0.0 |    0.0    0.0    0.0  171.6

Six chains sit at ~1.74, one at 1.30, one at 1.45. Seven report tau ~0 (white noise
at 25-sweep spacing) while one reports 171,600 sweeps. These chains are not sampling
the same distribution. **I4 requires per-chain tau within a FACTOR 2. Measured
factors: 11,275x at h=0.305629; 8,042x at 0.318989; 4,595x at 0.326; 7x at 0.322.
L=10 FAILS I4 at 8 of 9 points.** We have never enforced that clause, at any point,
in this campaign.

**And the crossing is bracketed by h=0.322/0.326** — inside the worst of it. So the
benchmark crossing is being read off the part of the L=10 curve that is least
entitled to be read.

### 2. WHY EVERY EXISTING CHECK MISSED IT — this is the part worth internalizing
The cold/hot means at h=0.305629 are 1.7377 vs 1.5578 and the I5 check PASSES. It
passes BECAUSE the hot group is internally split (spread 0.4405): the heterogeneity
that should raise the alarm instead inflates the group variance and drives z down.
**I5 can be passed by a broken ensemble whose own dispersion hides the break.** The
between-chain error cross-check from 20f is likewise fooled — it correctly reports a
large error bar, and a large honest error bar on a bimodal population is still an
average over two states neither of which is the answer. G1a/G1b/I5/I4 as we have
been running them are all blind to this; only the PER-CHAIN table shows it.
Add to the run manual: **report the per-chain table, not just the group summary.**

### 3. THIS IS NOT MY REFUTED WINDING HYPOTHESIS. Do not let me resurrect it.
I checked. Sector-exchange counts at h=0.305629 are 308214 309803 306432 308256 |
147750 307440 306942 260468 — healthy everywhere. The 18b winding-sector-trapping
hypothesis STAYS DEAD and the 2026-07-20 refutation stands. Worth noting honestly:
the two anomalous chains (147,750 and 260,468) are the two with visibly SUPPRESSED
exchange, so there is a correlation between reduced sector mobility and a deviant
ratio. That is an observation, not a mechanism, and I am explicitly NOT building a
hypothesis on it this time. Name it what it is: **unidentified basin structure on
the low-h deconfined side.** E3 already told us that side freezes as you go deeper.

### 4. 2B's "PHYSICS DISAGREEMENT" FLAG IS PROBABLY BACKWARDS, AND THAT IS THE BEST NEWS IN THE REPORT
Jarvis correctly flagged that the PT crossing (0.336613) lands outside the no-PT
envelope and said it needs investigating before PT is trusted. Right call. I did the
investigation. I reconstructed the fixed-h ratios from the 14-rung PT run:

  h    0.1500 0.1833 0.2167 0.2500 0.2712 0.2925 0.3137 0.3350 0.3563 0.3775 ...
  PT   1.3026 1.3172 1.3377 1.3630 1.3828 1.4085 1.4408 1.4969 1.6908 1.9074 ...

**Cleanly MONOTONIC, as the raw ratio must be.** The no-PT L=10 curve is NOT: it
starts at 1.6206 (h=0.3056), falls to 1.4434, then wobbles up. PT interpolates to
~1.42-1.43 at h=0.3056 against no-PT's 1.6206 — and no-PT's six frozen chains are
sitting at 1.74. **The nonmonotonicity Jarvis identified as the dominant limitation
is the basin contamination, not an interpolation-method problem.**
=> Re-read the flag: PT is not shifting the crossing away from truth. PT is
plausibly the BETTER-SAMPLED curve, and its crossing moves TOWARD Linsel's 0.335(2).
NOT YET PROVEN, and I am marking the limit: PT still carries residual init memory at
low h. PT cold/hot by origin-rank init, per rung: 0.0712 (h=0.2712), 0.0663, 0.0561,
0.0386, 0.0240 (h=0.3563). Shrinking with h but nonzero. **PT is better here, not
clean.** Both curves are contaminated at low h; PT less so.

### 5. THE REFRAME THAT MATTERS: PT's figure of merit is ERGODICITY, not speed
2B set out to measure tau reduction and came back censored on both sides — Jarvis is
right that no honest factor is quotable and right to refuse to quote one. But that is
a null on the metric we CHOSE, and it is now clear we chose the wrong one. The
benefit PT demonstrably delivers on this data is repairing an ensemble that local
updates cannot mix. That is worth more to this campaign than a speed factor:
speed buys error bars, ergodicity buys CORRECTNESS, and an ergodicity failure is the
one error class no amount of runtime fixes. This also answers the PI's question about
what PT experience we bank — the answer is bigger than swap-period tuning.
Note this cuts against my own 20h framing: I said the gate is measured tau at the
rung of interest. Insufficient. **A chain frozen in a basin reports LOW tau** (7 of
8 chains above report tau~0 while being demonstrably stuck). tau alone cannot detect
this failure in either direction. The gate must be tau AND per-chain agreement.

### 6. WHAT I AM RATIFYING
2A's order call (INCONCLUSIVE, zero STABLE-SEPARATED, all histograms unimodal) is
exactly right and exactly what I told you to expect at L=8-16; reported correctly
under the asymmetry rule. The duality result is genuinely reassuring: 200/201 PASS
with the one flag resolving by 400k says the instrument is sound on the self-dual
line. 2B's decision-rule call (NOT TRIGGERED, both taus censored, 144x/64x explicitly
not speedup bounds) is correct and I am glad you refused to quote them. The 20g
crossing prediction was mine and it FAILED as you say: restoring the points moved
nothing, because the crossing sits at h=0.322/0.326, nowhere near them. Your
interpolation-systematic scan was the right instinct and it found the real symptom.
The transcription-error self-report is exactly the right reflex; thank you.

### 7. NEXT — ordered, and the first item is free
 (a) FREE, DO FIRST. Per-chain I4 audit of everything we have: `15149119`,
     `15159599`, and the 2A scan. Report the PER-CHAIN table (ratio and tau per
     chain, cold and hot labelled) at every point, plus the tau max/min factor, and
     mark every point that violates the factor-2 clause. Expect much of L=10 to fail.
     This reprices the benchmark and possibly 2A's bracket, and it costs nothing.
     Add per-chain tau-factor as a standing column in `ensemble_diagnostics.py`, and
     make I4's factor-2 clause a HARD per-point gate rather than documentation.
 (b) DO NOT promote the benchmark crossing, and do not report 0.3244 as our value.
     It is read off points that fail I4 by ~10^3-10^4. `benchmark_report.md` stays
     PROVISIONAL and should now say WHY in one line: the L=10 baseline is not
     ergodic at low h. This supersedes "consistent with Linsel within honest
     systematics" — we cannot yet claim consistency OR tension from these points.
 (c) TEST THE SEC-4 HYPOTHESIS DIRECTLY, cheap and decisive. It is falsifiable:
     if PT's curve is the better-sampled one, then a no-PT L=10 run at h=0.3056 and
     h=0.322 started from MANY independent random inits (say 16 chains, not 8) will
     spread across the same basins, and its properly-pooled ratio will move DOWN
     toward PT's ~1.43 rather than sit at 1.62. If instead it reproduces 1.62 with
     tight agreement, PT's low-h rungs are the contaminated ones and I am wrong.
     Pre-committed both ways, before either is treated as the baseline.
 (d) 2A: your denser [0.325,0.350] sampling is the right next step and I approve it,
     with one addition — carry the per-chain gate from (a) into it, because a bracket
     from a primary method that "found no common three-size crossing" while 13 pairs
     came back NOT_GATED may be the same ergodicity disease at small L on the low-s
     side. Check that before spending more on resolution.
 (e) 2B redesign: your (a)/(b)/(c) are all correct and I authorize the matched-grid
     rebuild. Reorder them — your (c), the crossing shift, is no longer a worry to
     clear before spending, it is the most interesting measurement on the list, so do
     it FIRST via sec 7(c). And add ergodicity repair (per-chain agreement restored
     at fixed cost) to the deliverables beside the speed factor.
 STILL HOLD: L=30, L=24. Step 3 unaffected.

### 8. FOR THE RECORD
I have spent two updates arguing about error bars on a dataset whose chains were not
sampling the same distribution. The error bars were never the problem. Nobody had
printed the per-chain table. It cost one command.

### LOOP CONTAINMENT
(a) and (b) are free and immediate — do them and report before any new compute.
Then (c), then (d)/(e) in parallel. Report the per-chain I4 audit as a table.
