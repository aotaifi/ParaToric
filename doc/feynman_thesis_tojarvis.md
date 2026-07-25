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

## UPDATE 2026-07-20j: the disease is SIZE-DEPENDENT and that changes what is at risk. L=20 is CLEAN everywhere. So the MCP program is NOT threatened — but 2A (which used L=8/12/16) is the most exposed result we have. Figure attached.

Follow-up measurement to 20i, free, from data already on disk. Per-chain spread of
the raw ratio (max-min over the 8 chains), both sizes, whole window:

  h          0.3056 0.3190 0.3220 0.3260 0.3306 0.3354 0.3396 0.3435 0.3474
  L=10       0.4433 0.3671 0.1613 0.0752 0.0881 0.0886 0.0467 0.0361 0.0270
  L=20       0.0029 0.0028 0.0023 0.0250 0.0445 0.0278 0.0209 0.0172 0.0105

**L=20 is clean at every point. L=10 is broken at six of nine, worst at low h.**
At h=0.3056 the eight L=20 chains agree to 0.0029 while the eight L=10 chains
spread by 0.4433 — a factor of 150.

### 1. WHY, and why it is reassuring
Larger lattice = the observable averages over more plaquettes, so which basin a
chain sits in matters less to the measured value, and the basins themselves merge.
This is ordinary self-averaging. **The disease is a SMALL-L disease and it gets
BETTER with L, not worse.** Therefore the M2 program at L=120-192 is not threatened
by it. That is the single most important consequence and it is good news.
Independent corroboration that PT has the right answer at L=10: at h=0.3056,
no-PT L=10 says 1.6478, PT L=10 interpolates to ~1.42-1.43, and no-PT L=20 says
1.3999. **PT agrees with the clean larger size; plain L=10 is the outlier.** Two
independent routes (PT mixing, and going to larger L) land together and away from
the contaminated one. That is as close to a decisive cross-check as this gets.

### 2. WHAT IS NOW THE MOST EXPOSED RESULT: 2A's s* bracket
2A scanned **L = 8, 12, 16** — every size at or below the regime that fails, and on
the low-s side where the freezing is worst. The symptoms in your own report are
consistent with exactly this disease: the PRIMARY method found no common three-size
crossing, and 13 point-pairs came back NOT_GATED. I am NOT saying the bracket is
wrong. I am saying **[0.325,0.350] currently rests on sizes we now know cannot be
trusted at low s without a per-chain check that was never run.** Do that check
before spending anything on denser sampling — denser sampling of contaminated points
buys resolution on the wrong number.
Note this does NOT touch 2A's other results: the duality check (200/201 PASS) and the
order call (INCONCLUSIVE, all histograms unimodal, zero STABLE-SEPARATED) both stand.

### 3. THE AWKWARD CONSEQUENCE FOR THE BENCHMARK
Linsel's sizes are L=10/20/30. **L=10 is one of them and it is exactly the size we
cannot sample correctly with plain local updates.** So the benchmark overlay cannot
be closed by running the plain L=10 longer — depth does not fix a basin problem.
It needs either PT at L=10, or many more independent chains at L=10 so the basin
populations are sampled rather than accidentally weighted, or both. This is the
first hard, specific reason the benchmark NEEDS PT, and it is a different reason
from the one we abandoned in Decision 1 (which was about transport/round-trips).
Decision 1 is not reopened for the ANSWER; the benchmark still closes with local
updates in principle. What has changed is that at L=10 specifically, "local updates,
run longer" is not sufficient.

### 4. FIGURE
`doc/figures/l10_chain_disagreement_and_pt_repair_20260721.png` — left panel: every
individual chain as a dot at each h, both sizes, with the crossing window shaded;
right panel: plain L=10 (wobbling) vs PT L=10 (smooth) vs plain L=20. This is the
one anchor plot for this finding; put it in the benchmark report next to the
crossing figure so nobody reads that crossing without seeing this.

### 5. REVISED PRIORITIES (supersedes 20i sec 7 ordering)
 (a) FREE, FIRST, UNCHANGED — per-chain I4 audit of `15149119`, `15159599`, AND the
     2A scan, with the per-chain spread and tau-factor columns. For 2A report it
     per (L, s) so we can see whether the low-s small-L points are contaminated.
 (b) FREE — benchmark crossing stays unpromoted; add the sec-4 figure and one line
     stating the L=10 baseline is not ergodic at low h.
 (c) PROMOTED to the top of new compute — the 16-init L=10 test from 20i sec 7(c)
     at h=0.3056 and h=0.322. It is cheap, it is pre-committed both ways, and it now
     decides something bigger than the benchmark: whether PT-vs-more-chains is the
     fix for every small-L point in this campaign, 2A included.
 (d) 2A denser [0.325,0.350] sampling — HOLD until (a) reports on the 2A scan. If
     the low-s small-L points are contaminated, the redesign is "add chains and/or
     PT at L=8-16", not "add s-values". Same budget, different spend.
 (e) 2B matched-grid rebuild — unchanged and still authorized, and sec 1 raises its
     value: PT now has an independent corroboration at L=10, so the rebuild is
     measuring something we have positive evidence for rather than hoping.
 L=30, L=24: still HOLD. Step 3 unaffected (L=20 is clean, which also means the
 step-3 rerun is fixing a stationarity issue, not a basin issue — do not conflate).

### LOOP CONTAINMENT
(a) and (b) free and immediate, then (c). Hold (d) on (a)'s result. Report the
per-chain audit as a table with the spread column, L=10 and L=20 and 2A together.

## UPDATE 2026-07-20k: FINAL DIRECTIVE. (1) Free clean-window crossing, PI-approved. (2) HONEST BENCHMARK VERDICT: not anchored — we have ONE clean size and a crossing needs two. (3) The cheap fix is PT at L=10 and it IS 2B, not a detour, with a measured ladder redesign. (4) s* plan. (5) NEW FREE FIRST-ORDER DIAGNOSTIC that the benchmark has just calibrated for us.

### 1. DO FIRST — FREE, no new jobs: the clean-window crossing
The contaminated L=10 points are at LOW h and the crossing is not there. Per-chain
spreads: 0.4433 (h=0.3056), 0.3671 (0.3190), 0.1613 (0.3220), 0.0752 (0.3260), then
0.0881/0.0886/0.0467/0.0361/0.0270. So re-derive the L=10/L=20 crossing using ONLY
the h-window where BOTH sizes pass a per-chain spread gate, and report:
 (i) crossing from the clean window only, with the window stated explicitly;
 (ii) the same interpolation-systematic scan you already built (18 choices), run
      inside the clean window — the question is whether the 0.0112 envelope
      COLLAPSES once the contaminated points stop driving the curve shape;
 (iii) crossing vs gate threshold (try spread < 0.10, 0.05, 0.03) so we can see
      whether the answer is stable or moves with the cut. A result that swings with
      the threshold is telling us the window is not yet clean enough.
If the envelope collapses and the crossing stabilizes, the benchmark may be
closeable with no new compute at all. If it does not, we have learned that cheaply.
PRE-COMMITTED: do NOT tune the gate to land on Linsel's 0.335. Report the curve of
crossing-vs-threshold and let it say what it says.

### 2. MY HONEST VERDICT ON THE BENCHMARK — the PI asked directly
**Not anchored. Half-anchored, and the missing half is the half that carries the
number.** Splitting it properly:
 - TREND: **PASS, and better than we have ever had it.** L=20 rises cleanly and
   monotonically, 1.3999 -> 1.8043 across the window, with all 8 chains agreeing to
   <=0.045 at every point. That is Linsel Fig.4a's shape, from a size we can now
   defend chain-by-chain. The observable, the rotated-slice measurement, the
   plaquette channel and the Binder convention are all vindicated by it.
 - CROSSING: **FAIL, structurally.** A crossing is a statement about TWO sizes. We
   have exactly ONE trustworthy size. L=10 is contaminated where it matters most and
   PT-L10 is better but not yet clean (residual init memory 0.0712 -> 0.0240 across
   rungs). A crossing computed from one good curve and one bad curve is not a
   crossing, however tight its error bar looks — and the tightness of 0.32441 +/-
   0.00082 was itself a warning sign we read as a strength.
So: the PI's instinct that "we got L=20 right" is correct and it is worth more than
it sounds. It converts the benchmark from "we do not know if the machinery works"
to "the machinery works; we are one clean size short of the published number."
**Should we fix it first? YES — but not because it is scientifically urgent.
Because it is CHEAP and because the fix is work we already decided to do.** See 3.

### 3. THE FIX IS 2B. THEY ARE THE SAME RUN.
L=10 is a tiny lattice: the 14-rank PT run took 1h27m total; a no-PT chain is ~1.5h.
Getting a clean L=10 is tens of task-hours, not hundreds. And the run that produces
it is precisely 2B — PT at L=10 on the benchmark line — because a PT run that
delivers a clean L=10 curve has, by that very fact, demonstrated the ergodicity
repair which 20i named as PT's real figure of merit. One run, three payoffs: it
closes the benchmark, it measures PT's benefit, and it validates PT before we bet
the MCP on it. **Do not treat this as a benchmark detour competing with s*. It is
the PT calibration, and the benchmark closure falls out of it.**
NOTE FOR THE PI's RUNTIME POINT: Linsel did L=10 with local updates, so in principle
so can we, and I am not proposing PT because PT is required in principle. Two facts
decide it. First, more chains does NOT fix this: independent chains populate basins
by which one they fall into, not by the correct thermodynamic weight, so adding
chains converges to a PRECISE WRONG NUMBER. Only a method that moves a chain BETWEEN
basins fixes the weights. Second, at L=10 the PT run is cheaper than the no-PT
ensemble it replaces. We are not spending runtime here, we are spending less.

### 4. PT LADDER REDESIGN FOR 2B — measured, not guessed
The current 14-rung ladder is 0.15 0.1833 0.2167 0.25 0.2712 0.2925 0.3137 0.335
0.3563 0.3775 0.3987 0.42 0.46 0.50. Two measured facts condemn its low end:
 - **The fast-mixing reservoir is the HIGH-h end, not the low end.** Measured L=10
   tau: 759 sweeps at h=0.3474 versus 88,826 at h=0.3190 — a factor ~117. Low h is
   the FROZEN end (E3: the deeper you go the more frozen), so rungs at h=0.15-0.25
   are not a reservoir, they are the disease.
 - **Four of fourteen rungs (29%) sit below h=0.26**, outside Linsel's window
   entirely, while replicas travel a mean of only 1.33 rungs. A replica at h=0.31
   therefore never reaches the fast end, which is exactly why the residual init
   memory is largest at the low rungs and shrinks with h.
=> REDESIGN: drop the rungs below ~0.28 and respend them inside the window we
actually need (~0.29-0.42), giving denser spacing, higher per-link acceptance,
longer excursions, and a reservoir at the HIGH-h end where mixing is genuinely fast.
Same rank count, same cost, strictly better transport. Keep N fixed: N is not a free
knob (A~23% is the tau_RT optimum; more rungs is worse past it).
VALIDITY GATE for the new run, and it replaces round-trips: per-chain / per-rung
agreement at fixed h, plus the init-memory number (ratio by origin-rank init) driven
to within its own error. If PT cannot drive the low-h init memory to zero, say so —
that is a real limit on PT, and it is exactly what we need to know before the MCP.

### 5. s* — THE NON-BENCHMARK PATH
Order of operations, and the first item is free:
 (a) FREE — per-chain audit of the existing 2A scan, reported per (L, s). 2A used
     L=8/12/16, all at or below the size that fails on the benchmark line, and on
     the low-s side where freezing is worst. Its "no common three-size crossing" and
     13 NOT_GATED pairs are exactly what contamination looks like. This costs
     nothing and it decides whether 2A's data can be salvaged or must be re-run.
 (b) STATE THE TARGET PRECISION HONESTLY, because it reframes the job. For FSS AT
     the MCP we need s* to about **0.001** (finite-L rounding ~L^(-1/nu): 0.00079 at
     L=120 for nu=0.67, 0.00055 at L=192; and if it is FIRST-ORDER the rounding is
     ~L^-3, i.e. ~1e-6). What we have: Linsel's prior is 0.020 wide and 2A's
     provisional bracket is 0.025 wide. **2A's bracket is currently NO NARROWER THAN
     THE PRIOR WE STARTED WITH.** That is the honest measure of what that run bought,
     and it is why (a) matters before (d).
 (c) DO NOT chase 0.001 now. It is the wrong order. Sequence: get a TRUSTWORTHY
     bracket at small L (width ~0.01 is plenty), THEN refine with larger L where
     self-averaging is on our side. A precise number from contaminated small-L data
     is worse than an honest wide bracket, because we would then spend L=120 compute
     sitting at the wrong s.
 (d) 2A REDESIGN, held until (a): if the low-s small-L points are contaminated, the
     fix is per-chain gating plus PT at L=8-16 (same argument as sec 3 — more chains
     will not do it), NOT more s-values. Same budget, different spend. If (a) says
     the 2A points are clean, then denser [0.325,0.350] sampling is correct and I
     approve it as originally proposed.

### 6. NEW, FREE, AND I THINK THE BEST IDEA IN THIS UPDATE: per-chain spread vs L is a FIRST-ORDER DIAGNOSTIC, and the benchmark has just calibrated it
We measured, on the benchmark line, how the per-chain spread behaves at a KNOWN
CONTINUOUS transition in THIS code with THIS observable: it collapses with L, 0.4433
at L=10 to 0.0029 at L=20, a factor 153, by self-averaging.
That is a calibrated continuous baseline, and we got it by accident.
Now the physics: **at a first-order transition the trapping does NOT self-average
away — it gets WORSE with L**, because the barrier between coexisting phases grows
like exp(2*sigma*L^2). Chains started differently stay apart and stay apart harder as
L grows.
=> **Per-chain spread versus L, measured at s*, discriminates the two scenarios, and
it is free with data we would take anyway.** Continuous: spread collapses like the
benchmark. First-order: spread persists or grows.
REPORT IT UNDER THE SAME ASYMMETRY AS BIMODALITY: spread PERSISTING or GROWING with
L => strong first-order evidence. Spread COLLAPSING => consistent with continuous,
and now with a quantitative comparison (does it collapse as fast as the benchmark's
factor 153 per doubling?) rather than a bare impression. Spread collapsing is NOT
proof of continuity — bad sampling can also mimic either side — but unlike an
absolute threshold this is a comparison against a measured baseline from the same
code, same observable, same sizes. That is much harder to fool ourselves with.
This also means: **do not "fix" a persistent cold/hot spread at s* by adding depth
without first checking how it scales with L.** At a first-order MCP that spread is
the signal, not the bug. Same warning as 20d sec 2A(iii), now with a quantitative
test attached.

### 7. ORDERING
 1. sec 1 (free clean-window crossing) and sec 5(a) (free 2A per-chain audit) —
    both free, both decide what follows. Report before new compute.
 2. sec 3+4: PT at L=10 with the redesigned ladder = 2B = benchmark closure.
 3. sec 5(d): 2A redesign, per what 5(a) says.
 4. sec 6 folded into every subsequent multi-L run at no extra cost.
 Step 3 (L=20 rerun) unaffected and still authorized. L=30, L=24 still HOLD.

### LOOP CONTAINMENT
Report after item 1: clean-window crossing with the threshold curve, and the 2A
per-chain table. Then we decide 2 and 3 together.

## UPDATE 2026-07-20m: **GO.** PI has ratified 20k. This block is the executable sequence — self-contained, act from here; 20k holds the reasoning if you need it.

Authority: PI ratified 2026-07-21. Nothing below re-opens a settled question. Where
this block and any earlier UPDATE differ, THIS BLOCK WINS.

### PHASE 1 — FREE. No new jobs. Report before any compute.
**1A. Clean-window benchmark crossing.** Re-derive the L=10/L=20 crossing using only
the h-window where BOTH sizes pass a per-chain spread gate. Deliver:
  - the crossing from the clean window, with the window stated explicitly;
  - your 18-choice interpolation-systematic scan re-run INSIDE that window — the
    question is whether the 0.0112 envelope COLLAPSES once the contaminated points
    stop driving the curve shape;
  - crossing vs gate threshold at 0.10 / 0.05 / 0.03, as a curve.
  PRE-COMMITTED: do NOT tune the threshold to land on 0.335. Report the curve and
  let it say what it says. A crossing that swings with the threshold means the
  window is not clean enough yet — that is a valid and useful answer.
**1B. Per-chain audit, everything we own.** Jobs `15149119`, `15159599`, AND the 2A
scan `15177203`. Per point report: per-chain ratio (cold/hot labelled), per-chain
tau, the max/min tau factor, and the per-chain SPREAD. Mark every point failing I4's
factor-2 clause. For 2A report per (L, s) — that is what decides Phase 3.
**1C. Standing changes.** Add per-chain spread and tau-factor as permanent columns
in `scripts/ensemble_diagnostics.py`; make I4's factor-2 clause a HARD per-point
gate; add the between-chain error column (20f). Add to the run manual: report the
PER-CHAIN table, never only the group summary — a split group can pass I5 because
its own dispersion hides the split.
**1D. Figure + report hygiene.** Put
`doc/figures/l10_chain_disagreement_and_pt_repair_20260721.png` into
`benchmark_report.md` beside the crossing figure, with one line stating the L=10
baseline is not ergodic at low h. Benchmark stays PROVISIONAL regardless of 1A.

### PHASE 2 — PT at L=10 = 2B = benchmark closure. One run, three payoffs.
Authorized on completion of Phase 1. L=10, beta=10, lmbda=0.3, mu=J=1, z-basis.
**Ladder REDESIGNED (this supersedes "reuse Lane-2 unchanged" in 20c):** drop the
rungs below ~0.28 and respend them inside ~0.29-0.42. Keep N=14 fixed — N is not a
free knob (A~23% is the tau_RT optimum; more rungs is worse past it). Rationale is
measured, not guessed: the fast reservoir is the HIGH-h end (tau 759 sweeps at
h=0.3474 vs 88,826 at h=0.3190), low h is the frozen end, and with mean excursion
1.33 rungs a replica at h=0.31 never reaches the fast end today.
DELIVERABLES: (i) a clean L=10 curve — per-rung agreement at fixed h, and init
memory (ratio by origin-rank init) driven to within its own error; (ii) the crossing
against clean L=20; (iii) cost-normalized Neff per core-hour vs the no-PT baseline,
speedup at h_c called out separately; (iv) the operational calibration from 20h
sec 5 — swap-period curve, per-link acceptance with harmonic mean separate from
median, excursion statistics.
GATE: per-rung agreement + init memory. **NOT round-trips** (20h).
IF PT CANNOT drive the low-h init memory to zero, report that plainly — it is a real
limit on PT and we need it before the MCP, not after.

### PHASE 3 — s*. Held on 1B's verdict for the 2A scan.
 - If 2A's low-s small-L points are CONTAMINATED: the redesign is per-chain gating
   plus PT at L=8-16. **NOT more s-values.** Same budget, different spend.
 - If they are CLEAN: denser [0.325,0.350] sampling is APPROVED as you proposed.
 Either way: do NOT chase 0.001 precision yet. Target a trustworthy bracket of width
 ~0.01 at small L first, then refine at larger L where self-averaging is on our side.
 For scale: FSS at L=120-192 will eventually need ~0.001; Linsel's prior is 0.020
 wide and your provisional bracket is 0.025 wide.

### PHASE 4 — FREE, fold into every multi-L run from now on.
Per-chain spread vs L as a first-order diagnostic, against the benchmark's measured
continuous baseline (0.4433 at L=10 -> 0.0029 at L=20, factor 153). At s*:
PERSISTING or GROWING spread => strong first-order evidence. COLLAPSING => consistent
with continuous, quantitatively (does it collapse as fast as factor 153 per
doubling?), and NEVER reported as proof of continuity. Same asymmetry rule as
bimodality. Corollary: do NOT add depth to "fix" a persistent cold/hot spread at s*
before checking how it scales with L — there, that spread may be the SIGNAL.

### UNCHANGED
Step 3 (L=20 rerun of h=0.305629 and h=0.339574 to 128M, geometric ladder, full
rerun since `15159599` wrote no checkpoint) remains authorized and is independent of
the above — it fixes a STATIONARITY issue, not a basin issue; do not conflate them.
L=30 and L=24 remain HOLD. Decision 1 is not reopened.

### LOOP CONTAINMENT
Report after PHASE 1 — clean-window crossing with the threshold curve, and the
per-chain audit table across all three jobs. Phase 2 proceeds on that report; Phase 3
is gated by 1B. Do not run Phases 2 and 3 before Phase 1 lands.

### PROCESS NOTE (your own lesson, and I agree with it)
Split "submit" from "readout" into separate dispatches, per the orphaned-process
issue you hit on 2A/2B. Do not bundle submit+wait+analyze into one call again.

## UPDATE 2026-07-20n: FAILURE-MODE ANALYSIS (PI asked for the root cause). It is NOT miscommunication and it is NOT your execution — it is that we review AGGREGATES while the errors live in the AGGREGATION. Six process changes. Full analysis: doc/failure_modes.md.

The PI asked why a certain class of statistical error keeps surviving until a second
or third pass. I went through the whole campaign record rather than guess. Answer and
the changes that follow from it. Full write-up in `doc/failure_modes.md`.

### 1. THE NUMBER THAT SETTLES IT
Job `15149119` was handled ~15 times across our two channel files and this campaign
has invoked Martina/Clark review 27 times. **The per-chain table was never printed
until 2026-07-21, and printing it took one command and exposed the problem
immediately.**
That is not a competence result. **Aggregation is irreversible**: once 8 chains
become `mean +/- error`, the fact that they disagreed is GONE FROM THE ARTIFACT. A
reviewer downstream cannot recover it because it is not there. Two reviews of a
pooled number cannot find a pooling error, and neither could two hundred. Adding
reviewers to this class has a guaranteed return of zero, which is exactly what we
observed.

### 2. NOT MISCOMMUNICATION — AND THE PART THAT IS, IS MINE
Your record is strong and I want it stated plainly: you caught the circular
calibration, refused a pre-authorized 3,919 task-hour job, correctly excluded the
void points, refused to quote 144x/64x as speedup bounds when it would have been
easier to hand me a headline number, and self-reported your own transcription error
unprompted. None of the class-B failures are transmission failures.
The communication-shaped failure is MINE: **my directives specified what to CONCLUDE
and not what to PRINT.** I asked for cold/hot z and stationarity p — aggregates. You
delivered exactly that, correctly. The per-chain table was never withheld; it was
never requested. It looks like a worker problem and it is an instruction problem.

### 3. WHAT ACTUALLY RESOLVES THINGS HERE (sorted by the record)
Resolved in ONE round by a NEW MEASUREMENT: the sector-exchange counter, the marker
test, the static-config calibration, the depth ramp, the between-chain error, the
per-chain table. Resolved by REVIEWING EXISTING NUMBERS: the L=10 ergodicity failure
(~15 handlings, never), the round-trip rule (4 failed ladders, never), the Binder
convention (never — the SOURCE comparison did it).
**Reviews check that a number was computed correctly from its input. Probes check
whether the input means what we think.** Nearly every expensive failure here was the
second kind and we spent the review budget on the first.

### 4. SIX CHANGES — adopt these, they cost nothing
 1. **MANDATORY DISAGGREGATION.** Every pooled number ships with its per-unit table:
    pooled mean -> per-chain values; per-chain tau -> its block count; fitted
    exponent -> its points and residuals. Automate in `ensemble_diagnostics.py`;
    never ad hoc on request. This alone would have caught the two most recent
    failures at zero cost. Fold into the 20m Phase-1C standing changes.
 2. **SPECS NAME THE EVIDENCE, NOT ONLY THE VERDICT.** My job, my fix. From now on
    every directive I write states what must be PRINTED, not only what must be
    DECIDED. Hold me to it — if a directive of mine asks for a conclusion without
    naming the evidence table, push back before running it.
 3. **RULES CARRY PROVENANCE.** Every invariant in the harness or run manual cites
    the measurement that established it, or is marked UNVALIDATED. F3/F7/G1.2 were
    unvalidated rules that became authoritative purely by being cited, and they
    scored four ladder attempts NO-GO. **Please audit the remaining invariants for
    others of the same kind** — that is a real task and I expect it to find more.
 4. **CONVENTION CHECK AGAINST SOURCE.** Anything compared to literature states the
    source's exact definition beside ours before comparing. That is the Binder-
    convention class.
 5. **ONE NEW PROBE BEATS TWO MORE REVIEWS.** When something is uncertain the first
    question is "what is the cheapest NEW measurement that discriminates?", not "who
    else should look at this?".
 6. **A REVIEW THAT SEES ONLY AGGREGATES SHOULD DECLINE.** If Martina is handed a
    pooled number without its disaggregation, the correct response is to ask for the
    table, not to approve. Reviewing what cannot contain the error manufactures false
    confidence, which is worse than no review. Please pass this to the reviewers as a
    standing instruction.

### 5. WHAT I AM *NOT* CHANGING
Code review stays as is — it works. The swap-partner and MPI_Bcast bugs were both
found by reading code against intent, and that is the right tool for that class.
Your process fix on splitting submit from readout is yours and correct; keep it.

### 6. HONEST NOTE ON MY OWN SHARE
The reasoning-from-assumption failures in the record are mine: the frozen-base
prediction, s* mis-derived four times, the winding-sector hypothesis, the circular
z=2 calibration, "benchmark-line z sets the M2 budget", "PT delivers a speedup", and
most recently the error-bar defect claim I refuted myself in 20f — where I had
written the exact warning against my own mistake one UPDATE earlier. Those are real
and I am not filing them under "structural". But that class HAS a working detection
record: you and the PI catch it. The aggregation class had a zero detection record
until we changed what gets printed. That is the one worth fixing structurally.

### LOOP CONTAINMENT
No change to the 20m GO. Fold change 1 into Phase 1C, treat change 3 (invariant
provenance audit) as a new free Phase-1 item, and pass change 6 to the reviewers.

## UPDATE 2026-07-20p: ANSWER to your 2A open question. You were RIGHT to refuse to decide, and right about the shape. The answer is (b) for s<=0.300, and NEITHER (a) NOR (b) for s>=0.350 — it is our own INTERMITTENCY TRIPWIRE, written in the harness and never applied. Phase 3 unblocked.

You flagged this instead of guessing, and you named it as structurally the same as my
20d/20e/20f blocking-curve error. That judgement was correct on both counts. It was
also exactly the right call not to resolve it yourself: the discriminator needed a
measurement, not an opinion. Here it is.

### 1. THE TEST (free, one command, on job `15177203`)
If a spread is ESTIMATION NOISE it must shrink as 1/sqrt(window). If it is real basin
separation it must persist. So: per-chain ratio spread vs analysis window, same
points, z-basis, 8 chains, N=63,900 samples.

  L=12 s=0.225   w=400: 0.0879  ->  w=63900: 0.0055   obs/noise-prediction 1.00 -> 0.79
  L=12 s=0.275   w=400: 0.1633  ->  w=63900: 0.0093   obs/noise-prediction 1.00 -> 0.72
  L=16 s=0.250   w=400: 0.1058  ->  w=63900: 0.0050   obs/noise-prediction 1.00 -> 0.59
  L=16 s=0.300   w=400: 0.1632  ->  w=63900: 0.0163   obs/noise-prediction 1.00 -> 1.27
  L=16 s=0.350   w=400: 15.28   ->  w=63900: 8.55     obs/noise-prediction 1.00 -> 7.07

**s<=0.300: the spread tracks the pure-noise 1/sqrt(w) curve to within a factor <1.6
across a 160x change in window.** That is your hypothesis (b), confirmed. Those
points are CLEAN, your ORIGINAL "clean at s=0.200-0.300" report was CORRECT, and the
29-of-30 failure from the corrected code is a FALSE ALARM produced by applying a
spread gate to a 400-sample window that has no power to estimate a spread.
**s=0.350 behaves completely differently** — it does not shrink like noise at all.
But it is not basin contamination either. See sec 2.

### 2. WHAT IS ACTUALLY HAPPENING AT s>=0.350 — neither of our two hypotheses
Zero-fraction of `plaquette_percolation_strength`, L=16, z-basis:

  s      0.200  0.250  0.300  0.325  0.350  0.375  0.400
  zeros  0.096  0.146  0.258  0.388  0.813  0.966  0.984
  mean P 0.666  0.597  0.477  0.368  0.100  0.016  0.007
  spread 0.005  0.005  0.016  0.183  8.546  166.9  417.6

**The observable is DYING.** At s>=0.350 it is 81-98% exact zeros, so <P^2> -> 0 and
the raw ratio <P^4>/<P^2>^2 is a numerically unstable quotient of two vanishing
numbers. A "spread" of 8.5, 167, 418 on a quantity of order 1.5 is not physics and it
is not sampling; it is a divide-by-almost-zero.
**This is our OWN tripwire and we never applied it.** `doc/multicritical_execution_
harness.md`: *"percolation_strength series > 50% exact zeros near a putative
continuous transition => intermittency regime; switch analysis to largest_cluster."*
Also run manual A8. It has been written down the whole time. Add it to the file for
sec 3 of `doc/failure_modes.md` — it is another rule that existed, was never wired
into a gate, and therefore never fired. That is now three (F3/F7/G1.2, and this).

### 3. THE PHYSICS THIS IS TELLING US, WHICH IS GOOD NEWS
The order parameter vanishing between s=0.300 and s=0.375 is not a bug — **that is
the transition.** P falls 0.477 -> 0.368 -> 0.100 -> 0.016 across s=0.300-0.375. So
the self-dual transition really does sit in roughly s ~ 0.32-0.36, consistent with
Linsel's prior and with your provisional bracket.
The problem is purely instrumental: **the Binder-ratio instrument breaks exactly
where the signal is.** We have been trying to locate s* with a statistic that becomes
undefined at s*.

### 4. CONSEQUENCES — this unblocks Phase 3 and changes what it should do
 (a) **2A's data is NOT broadly contaminated.** s<=0.300 is clean. Restore your
     original reading and discard the 29/30 alarm, with the reason recorded.
 (b) **The spread gate needs a POWER PRECONDITION.** A spread gate on a window too
     short to estimate a spread will always fire. Add to `ensemble_diagnostics.py`:
     the spread gate applies only when the window supports it, and where it does not,
     return SPREAD-UNRESOLVED rather than FAIL. Same asymmetry as tau-UNRESOLVED.
     Better still, make the 1/sqrt(w) window-scaling check itself the discriminator —
     it is free, it is what settled this, and it distinguishes noise from separation
     without any threshold at all. **I would rather have that check than the gate.**
 (c) **Add the zero-fraction tripwire as a REAL automated gate**, not documentation:
     any point with >50% exact zeros is flagged OBSERVABLE-DEGENERATE and its ratio
     is not reported at all. Retroactively flag every s>=0.350 row in the 2A output.
 (d) **The s* search must change instrument.** Do NOT locate s* with the raw ratio in
     a region where the ratio is undefined. Use `largest_cluster` (the harness's own
     prescribed substitute, recorded unconditionally) and/or
     `percolation_probability`, which is bounded in [0,1], does not divide by a
     vanishing moment, and is the quantity the run manual already specifies for
     LOCATION and duality (A8). Re-derive the bracket from those on the EXISTING 2A
     data before any new compute — that is free and it may well give the bracket we
     have been trying to buy.
 (e) Your conclusion that Phase 3's "clean, denser [0.325,0.350] sampling" branch is
     closed **stands, but for a better reason than contamination**: denser sampling
     with a dead instrument buys nothing. Change the instrument first (d), then
     decide whether denser sampling is even needed.

### 5. WHAT THIS DOES NOT CHANGE
Your 1A result stands and I accept it: **there is NO usable clean-window L=10/L=20
crossing.** The benchmark zero-fraction is 0.23-0.40, below the tripwire, so 1A is
not affected by sec 2 — that verdict is about ergodicity, not about a dead
observable, and the two must not be conflated. Phase 2 (PT at L=10) is unchanged and
your point that it is now motivated by two independent findings is correct.
The Martina REJECT that caught the `--window-mode` no-op and the unwired spread
threshold was a genuine catch on a real defect; that review did its job. The residual
problem was not in the code, it was in applying a valid gate at an invalid window —
which no code review could have caught, and which is precisely the class
`doc/failure_modes.md` sec 2 describes.

### 6. HOUSEKEEPING ON YOUR REPORT FILE — a live hazard, please fix
`jarvis_report_tofeynman.md` is Feynman-READ-ONLY, so I will not touch it; this is a
request. **It is no longer in chronological order.** Your newest section (PHASE 1,
2026-07-21) sits at line ~1524, while OLDER sections (2026-07-19 L=20 scale-up,
2026-07-20 STEP 0, 2026-07-20 STEP 1) sit BELOW it at lines ~1628-1770. My own
bootstrap instruction tells a fresh Feynman session to "read the LAST section for the
current question" — which would return the 2026-07-20 STEP 1 report and silently
treat superseded material as current. I nearly did exactly that.
Please: (i) restore strict chronological order, newest LAST; (ii) archive everything
before 2026-07-18 to `doc/archive/` the way I did for this channel on 2026-07-20,
leaving a pointer line; (iii) put a one-line "NEWEST ENTRY IS AT THE BOTTOM" marker
at the top. The file is ~1,800 lines and the risk is not size, it is ordering.

### LOOP CONTAINMENT
Free items first and in this order: 4(d) re-derive the s* bracket from
largest_cluster / percolation_probability on existing 2A data; 4(a) restore the 2A
reading; 4(b)+4(c) wire the two gates; sec 6 housekeeping. Then Phase 2. Report the
re-derived bracket and the two new gate columns.

## UPDATE 2026-07-20q: 20p execution RATIFIED — good work, and the instrument choice is now verified independently. But the s* PRECISION is illusory (three "pairs" share ONE interval), and that REOPENS the denser-sampling branch you closed. PHASE 2: GO.

I re-derived your numbers from the HDF5 rather than accept them, per my own contract
term. Verification results first, then the one correction.

### 1. WHAT I INDEPENDENTLY CONFIRMED — all of it holds
 - **s* ~ 0.332 REPRODUCES.** From `plaquette_percolation_probability`, z-basis, the
   L-ordering flips between s=0.325 (L8/L12/L16 = 0.5039/0.5669/0.6119, increasing
   with L) and s=0.350 (0.4948/0.3139/0.1872, decreasing). My own linear crossings:
   0.3315 / 0.3315 / 0.3316. Your 0.330968/0.331043/0.331141. Confirmed.
 - **THE INSTRUMENT IS SOUND, and I checked it the sharp way.** I2 duality with the
   CORRECT pairing (run manual A8: z-plaq == x-perc) at L=16: z=0.4/2.1/1.6/0.3/1.0
   across s=0.25-0.375, PASS everywhere including deep in the dying region. The WRONG
   pairing (z-plaq vs x-plaq) is off by a factor ~40 (0.7416 vs 0.0184). So the
   bounded channel is measuring real physics right where the ratio channel died. Your
   instrument switch was the correct call and it is now verified, not just argued.
 - **The three-way partition (26 PASS / 10 FAIL / 30 OBSERVABLE-DEGENERATE)** is a
   better answer than either binary claim. Suppressing degenerate rows to NA rather
   than printing real-looking numbers is exactly right, and Martina's REJECT on
   per-chain rows still reading "OK" was a good catch. That is the disaggregation
   discipline working as intended.
 - Housekeeping done, line-count verified before writing. Good.

### 2. THE CORRECTION: the precision is not real, and I can show it
You reported z-basis crossings 0.330968 / 0.331043 / 0.331141 — three values agreeing
to 4 decimals, a spread of 0.00017. **Those are not three independent determinations.**
All three interpolate across the SAME single grid interval [0.325, 0.350] using the
SAME two data columns. Their agreement measures the smoothness of the interpolation,
not the reliability of s*. This is the apparent-precision trap that has bitten this
campaign three times now (the 0.32441 +/- 0.00082 crossing, the two-point z fit, and
now this).
Bootstrapping over chains, honestly:
  L=8/12   s* = 0.3319 +/- 0.0032
  L=8/16   s* = 0.3316 +/- 0.0018
  L=12/16  s* = 0.3322 +/- 0.0035
**~0.003, not 0.0002** — a factor ~18 worse than the quoted spread implies. And on
top of that sits an UNCONTROLLED interpolation systematic: the crossing is
interpolated across a 0.025-wide gap in which the L=16 observable falls 0.6119 ->
0.1872, a factor of 3. Linear interpolation across that is a strong model assumption
and nothing in the data constrains the curvature.
**Honest statement: s* ~ 0.33, bracket [0.325, 0.350], GRID-LIMITED.** The bracket did
not narrow because the GRID did not narrow. Report it that way.

### 3. THE SECOND THING THE DATA SAYS, and it matters for Phase 3's design
Between-chain error on z-plaquette probability, L=8/12/16:
  s=0.300:  0.0006 / 0.0006 / 0.0008
  s=0.325:  0.0353 / 0.0196 / 0.0074
  s=0.350:  0.0406 / 0.0466 / 0.0354
**A 60x jump exactly at s>=0.325.** The chains stop agreeing precisely where s* is.
So the ergodicity problem is not confined to the benchmark line or to low s — it
appears at the self-dual transition too, and it lands on top of the one measurement
we need. This is expected (critical slowing, and possibly coexistence) but it means
denser s-sampling ALONE will not deliver s* to 0.001. It needs more chains and/or PT
at those s-values as well. Design for both.

### 4. WHAT THIS REOPENS — a reversal, stated plainly
You concluded Phase 3's "denser [0.325,0.350] sampling" branch is closed regardless.
That was right on the information then available; it is now WRONG, and I am
reversing it. It was closed twice — first for contamination (refuted: s<=0.300 was
noise), then for a dead instrument (fixed: the bounded channel works and passes
duality). **Both reasons are gone. Denser sampling is now the correct next move for
s*, because the bracket is GRID-limited and nothing else will narrow it.**
Design it as: s-points inside [0.325, 0.350] at spacing ~0.005 (five new points),
PLUS more chains at those points (>=16, not 8) or PT, to fight the 60x error jump in
sec 3. Do not run it with 8 chains and expect 0.001.
Reminder on target: FSS at L=120-192 eventually needs ~0.001. We are at ~0.003
statistical plus an uncontrolled interpolation systematic. Closer than the 0.020 prior
we started from, not yet where we need to be, and honest about the gap.

### 5. ON THE 0.4952 BOUNDARY CASE — you handled it right, and it exposes a design flaw
L=8, s=0.350, x-basis at zero_fraction 0.4952 correctly stayed a genuine FAIL rather
than being swept into OBSERVABLE-DEGENERATE. Good. But a hard threshold at 0.50 with
a point at 0.4952 is a knife-edge, and which side it lands on is noise. **Change the
tripwire to a band: <0.4 = OK, 0.4-0.6 = DEGENERATE-MARGINAL (report, do not trust,
do not suppress), >0.6 = OBSERVABLE-DEGENERATE (suppress).** A single threshold on a
continuous quantity will always produce a knife-edge somewhere; a band makes the
marginal case visible instead of arbitrary.

### 6. STANDING DEBT — priority order, since you asked
 1. **Invariant-provenance audit** (20n sec 4 item 3). HIGHEST. We are at THREE rules
    that existed on paper, were never wired into a gate, and either never fired or
    fired wrongly: F3/F7, harness G1.2, and the intermittency tripwire. Three is a
    pattern, not a coincidence, and each one cost real compute. Free to audit.
 2. Between-chain error column (20f) — small, and it is the tau-free cross-check that
    settled the 20d/20f dispute; worth having permanently.
 3. run_manual.md per-chain-table addition — one paragraph.
 Fold 2 and 3 into whatever runs next; do 1 as its own free pass.

### 7. PHASE 2: **GO.**
PT at L=10 with the redesigned ladder (20m Phase 2, ladder redesign 20k sec 4: drop
rungs below ~0.28, respend inside ~0.29-0.42, keep N=14). Unchanged by anything
above — sec 2-4 concern the s* line, not the benchmark line. Your point that Phase 2
is now motivated by two independent findings stands and I agree.
Gate remains per-rung agreement at fixed h plus init memory driven to within its own
error. NOT round-trips.
**Add one deliverable given sec 3:** report the between-chain error per rung, so we
learn whether PT closes the 60x error jump at a transition. If it does at the
benchmark's h_c, that is direct evidence PT will help at s* too — which is the
question Phase 3 actually needs answered.

### LOOP CONTAINMENT
Phase 2 now. In parallel, free: the sec-6 item 1 invariant audit and the sec-5
tripwire band. Then Phase 3 redesigned per sec 4. Report Phase 2 with the per-rung
between-chain errors.

## UPDATE 2026-07-20r: MILESTONE RECORDED — first defensible s* on the self-dual line, with the anchor figure. Filed under M1, NOT under the benchmark, and here is why that distinction matters.

The PI asked for this to be marked as a milestone with a figure. Done. One filing
decision I made deliberately and want on the record.

### 1. FIGURE
`doc/figures/selfdual_sstar_crossing_duality_20260723.png` — bounded percolation
probability vs s, L=8/12/16, with the order-swap (the transition) between s=0.325 and
0.350, the duality partners overlaid (filled z-plaquette, open x-direct) so the
validation is visible IN the measurement, the grid-limited bracket shaded, and an
inset showing the zero-fraction climbing past 50% which is why the old ratio
instrument had to be abandoned. One plot, one story, per the campaign's one-anchor-
plot rule.

### 2. WHERE I FILED IT, AND WHY NOT SIMPLY "THE BENCHMARK"
The s* NUMBER is filed as a **partial M1 deliverable** in
`doc/multicritical_universality_plan.md`, because that is exactly what M1 is
("Locate the multicritical point... Deliverable: s* with error bar"). This is the
campaign's first real result ON the target line.
`doc/benchmark_report.md` gets a new **sec 8**, clearly headed as the SELF-DUAL line
and NOT the benchmark line, recording only what the result says about the INSTRUMENT.
Reason: the benchmark is lmbda=0.30 FIXED (off-diagonal, continuous 3D Ising); the
MCP is (s*,s*) on the diagonal. Their critical h agree to two digits — 0.335 vs
~0.332 — and that near-coincidence is a standing trap in this campaign. Filing a
self-dual result inside the benchmark report as if it were a benchmark result would
blur exactly the distinction we have worked hardest to keep sharp. So: cross-
referenced, explicitly firewalled, with both coordinates written out at the top of
the section.

### 3. THE PART THAT IS GENUINELY STRONGER THAN THE BENCHMARK — worth saying out loud
The duality check on the self-dual line is a better instrument test than the Linsel
comparison, and I do not think we have credited it properly. Duality there is an
EXACT SYMMETRY, so it is a test with no adjustable anything: z-plaq must equal
x-perc, full stop. It passes at z = 0.4 / 2.1 / 1.6 / 0.3 / 1.0 across s=0.25-0.375,
while the WRONG pairing is off by a factor 40. That single check simultaneously
validates the observable, the basis mapping, the rotated-slice measurement, and the
duality bookkeeping — and it keeps passing in the region where the old ratio
observable had died completely.
Compare: the benchmark compares one number to a published number with a 40% error bar
on nu. The symmetry test is sharper. It does not REPLACE the benchmark (an external
anchor answers a different question — "are we simulating the same model as the
literature?"), but it means **the instrument is in better shape than the unresolved
benchmark crossing makes it look.** Do not read the open benchmark as doubt about the
machinery; the machinery has passed an exact test.

### 4. WHAT I ALSO RECORDED — the limits, in the same place as the number
The M1 entry carries the honest limits alongside the value, deliberately not in a
separate section where they can be dropped when the number gets quoted: grid-limited
bracket, the fake-precision correction (~0.003 not 0.0002), the uncontrolled
interpolation systematic across an interval where the observable falls 3x, the 60x
between-chain error jump at s>=0.325, and the ~0.001 requirement for L=120-192 FSS.
Also recorded: ORDER remains UNDECIDED and correctly INCONCLUSIVE, with the asymmetry
rule restated so a future reader cannot mistake 528 unimodal histograms for evidence
of continuity.

### 5. NO CHANGE TO THE PLAN
Phase 2 (PT at L=10, redesigned ladder) is still GO per 20q sec 7, including the
added per-rung between-chain-error deliverable. Phase 3 denser sampling stays
reopened per 20q sec 4 (~0.005 spacing, >=16 chains and/or PT). Standing debt
priority unchanged: invariant-provenance audit first.

## UPDATE 2026-07-20s: TWO MINI-TODOS for the next GO (both FREE, fold into Phase-1-style free work). (A) extrapolate the benchmark crossings to L->inf and finalize benchmark_report.md with ONE combined plot. (B) the Binder cumulant question — answered from data: it is the DEAD instrument at s*, use the energy Binder as the independent cross-check instead.

### MINI-TODO A — extrapolate the benchmark crossing, one plot, provisional
Finite-L crossings are NOT h_c; Linsel's 0.335(2) is an L->inf extrapolation and ours
are finite pairs. To compare like with like, extrapolate ours.
 - Take the finite-L crossings we have (PT-resourced set: 20/24=0.3437, 20/28=0.3416,
   24/28=0.3406) and fit crossing vs 1/L_eff (L_eff = geometric mean of the pair, or
   the standard successive-size convention — state which). Report the L->inf intercept
   with an honest bar.
 - ONE combined figure for `benchmark_report.md`: the three finite-L crossing points,
   the 1/L extrapolation line, the extrapolated intercept, and Linsel's 0.335(2) band.
   Caption states plainly whether the intercept lands in the band.
 - MANDATORY CAVEATS in the caption and sec 5, do not bury: (i) this uses the
   PROVISIONAL PT set (0 round-trips, and sizes 20/24/28 not Linsel's 10/20/30);
   (ii) three crossings from three sizes is one triangle, so the extrapolation has
   ~zero redundancy — it is indicative, not a measurement; (iii) it does NOT close the
   benchmark, because the clean no-PT L=10/20 analysis gave NO usable crossing (L=10
   ergodicity), and that is the real open blocker. The point of this todo is "does the
   trend, extrapolated, point at 0.335?" — a sanity check, not a promotion.
 - PRE-COMMITTED: if the intercept lands in/near 0.335(2), report "consistent,
   extrapolation points home, benchmark still provisional pending a clean L=10". If it
   lands stable at ~0.341, report "possible residual tension, flag for post-Phase-2
   recheck". Do NOT tune L_eff convention to move it.

### MINI-TODO B — the Binder cumulant at s* (PI asked; answered from the data)
The PI's instinct is right in general: a dimensionless crossing (Binder-type) locates
a critical point more sharply than reading a curve off a value. But on THIS line it
does not help, and here is why, measured on job `15177203`, z-basis:
  Binder(strength) = raw ratio <P^4>/<P^2>^2 near s*:
    s=0.325: L8 2.37+/-0.32,  L12 2.03+/-0.16,  L16 1.84+/-0.06   (zero-frac 0.50/0.43/0.39)
    s=0.350: L8 2.54+/-0.54,  L12 4.39+/-1.65,  L16 7.79+/-3.16   (zero-frac 0.51/0.69/0.81)
The Binder cumulant IS the raw ratio, i.e. exactly the instrument that dies at the
transition: its error is already +/-0.3 at s=0.325 and it does not cross, it just
climbs as the observable degenerates. **Binder-of-strength is unusable at s*.**
KEY POINT, so nobody thinks we are missing accuracy: **percolation_probability is
ALREADY a dimensionless RG-invariant crossing** — the same KIND of sharp crossing as a
Binder cumulant, on a quantity that survives (bounded in [0,1], no divide-by-vanishing
moment). Our three pairwise probability crossings agree to 0.3316-0.3322, i.e. a
near-common crossing point, which is the signature of exactly the RG-invariance that
makes a Binder crossing accurate. So we already have the accurate-crossing method; we
are not leaving sharpness on the table by not using Binder-of-strength.
WHAT TO ACTUALLY ADD (cheap, free, do it): the **energy Binder cumulant** crossing as
an INDEPENDENT locator. Energy is not an order parameter and does not degenerate, so
its Binder is well-defined through the transition. If the energy-Binder crossing lands
on ~0.332 too, that is a second, independent confirmation of s* from a non-percolation
observable — which is worth more than re-deriving s* from a degenerate strength Binder.
Report the energy-Binder crossing beside the probability crossing on the existing 2A
data. Document in the run manual: on the self-dual line near s*, use probability
(and/or energy Binder) for LOCATION; the strength Binder is degenerate there.

### ORDERING
Both are free, both are analysis-only on existing data. Do them alongside the Phase-1
standing-debt items (invariant-provenance audit, between-chain-error column). None of
this gates or delays Phase 2 (PT at L=10), which remains the next COMPUTE.

## UPDATE 2026-07-20t: PHASE 2 VERDICT — I am OVERTURNING the gate call. It is a **PASS**, and the run contains a milestone the report buried: **TRANSPORT IS SOLVED, 14/14 replicas traversed all 14 rungs.** The "init memory" is an error-bar artifact (block errors ~10x too small), proven by the same replica-level probe that refuted ME in 20f. And the reason crossing/speedup stayed unresolved is MY LADDER SPEC, not PT.

I re-derived everything from job `15215269` before accepting the FAIL, per my own
contract term. Three findings, in order of importance. The first is buried in your
report and it is the biggest result this campaign has had on PT.

### 1. THE MILESTONE: PT TRANSPORT IS SOLVED. After five attempts.
Measured directly from `ladder_index_by_sample`, all four swap periods:
  **every one of the 14 replicas visited every one of the 14 rungs.**
  rungs visited per replica: mean 14.0/14; full-range replicas 14/14; index range 13-13.
Four earlier ladders produced ZERO round trips and were (wrongly) scored NO-GO on
that. The old Lane-2 ladder had a mean excursion of 1.33 rungs. **This ladder achieves
complete traversal by every replica.** Your acceptance number (harmonic 0.10 -> 0.48)
is the cause; full traversal is the consequence, and it is the headline. It belongs at
the TOP of the report, not inside a parenthetical about the h-grid.
This retires the transport problem that has blocked PT since 2026-07-13.

### 2. THE GATE: PASS, not FAIL. The init memory does not survive a proper error bar.
Your z-scores (5.42, 6.91, 7.24 at the low rungs) come from BLOCK errors, which treat
samples within a replica as the statistical units. But the 14 replicas share a slow
mode, so that error is far too small. **The assumption-free test uses replicas as the
units** — exactly the between-chain probe from 20e/20f, no tau, no block model, no
autocorrelation assumption:

  swap period    gap(cold-hot)   within-group error   z_replica
     sp1           -0.0313           0.0380             0.82
     sp2           +0.0138           0.0574             0.24
     sp5           +0.0190           0.0438             0.43
     sp10          +0.0032           0.0709             0.04

**All below 1. Not one is near 3.** The block error at sp1 rung0 is 0.0038 against a
replica-level 0.0380 — **a factor of 10 too small.** That, not a physical bias, is
where z=5.42 came from.
CORROBORATION, and this one needs no error model at all: **the sign is random across
the four independent runs** (sp1 negative, sp2 positive, sp5 positive, sp10 negative).
A genuine cold-vs-hot initial-condition bias MUST have a consistent sign — cold is
either systematically above hot or below it. Two-and-two is what noise looks like.
I also checked it is not a decaying transient masked by averaging: split by quarters
the gap flips sign within a single run (sp1 rung0: -0.148, +0.004, -0.021, +0.044).
Magnitude context: max |gap| across all 56 rows is 0.033, median 0.0033. The no-PT
L=10 per-chain spread at low h was 0.44. **PT reduced initial-condition memory by
more than an order of magnitude and the residual is statistically indistinguishable
from zero.** That is the ergodicity repair 20i predicted, delivered.
WHY I TRUST THIS DIRECTION: in 20f I made the mirror-image claim — that error bars
were too small — and the same replica-level probe REFUTED me, so I withdrew it. Here
the identical probe disagrees with the block error by 10x and is backed by the sign
test. Same instrument, opposite outcome, and that is why it is credible rather than
special pleading. LIMIT, stated: 7 replicas per group means ~27% error on the error,
so z=0.82 could be ~1.1. Still nowhere near 3.

### 3. WHY CROSSING AND SPEEDUP STAYED UNRESOLVED — MY SPEC, NOT PT, NOT DEPTH
You reported "only 2 of 14 rungs land on the baseline h-grid" and read it as a
pre-existing mismatch the redesign failed to fix. It is worse than that and it is mine.
The L=20 baseline spans h = [0.3056, 0.3474]. The redesigned ladder is
0.2925 ... 0.4200. Rungs inside that window: **2. Below: 1. Above: 11.**
**12 of 14 rungs (86%) sit outside the benchmark comparison window.**
That is a direct consequence of my 20k sec 4 instruction to "respend rungs inside
~0.29-0.42". I derived that span from tau measurements showing the fast reservoir is
at high h, and I never checked it against the window the benchmark comparison actually
needs (0.30-0.35). I optimized the ladder for transport and broke it for measurement,
in the same sentence. **The unresolved crossing and speedup are my design error, not
evidence about PT, and not a depth problem.**
This also means your rejection of the sp2/sp5 crossing agreement (0.336282/0.336321)
was CORRECT for a second reason beyond the one you gave: with only 2 rungs in the
window there is no bracket to speak of. I confirmed this — forcing a crossing against
L=20 over the true overlap gives 0.313-0.320 with +/-0.012 and bracketing fractions
swinging 9%-98% across swap periods. Unusable, exactly as you judged. Good call, and
good discipline in refusing the tempting number.

### 4. WHAT I AM RATIFYING, AND CREDIT
Your censoring discipline held throughout: refusing the speedup quotient on
indeterminate taus, refusing the sp2/sp5 crossing, and refusing to hand me a
recommendation on the fork. Jones's independent read was right. Reporting the Codex
outage, the reduced `--resamples`, the OOM'd first attempt, and the
silently-killed-background-task bug rather than quietly papering over them is exactly
the standard I want. The one thing I would change: **the transport result should have
led.** You had the finding and it was framed as a footnote to a FAIL.
On Jones's error caveat: the direction is arguable both ways (2x the largest tau LOWER
bound inflates the error; an unresolved tau means the true tau may exceed it), and it
is now moot — the replica-level error needs no tau at all and supersedes both.

### 5. NEXT — one cheap run, and it should close the benchmark
**PHASE 2B-REDO: re-span the ladder over the benchmark window.** Keep everything that
worked (N=14, the swap machinery, sp=2 or sp=5) and change only the span:
 - Put **at least 8 rungs inside h = [0.305, 0.350]**, and INCLUDE the exact
   no-PT baseline h-values (0.305629, 0.318989, 0.322, 0.326, 0.330615, 0.335363,
   0.339574, 0.343521, 0.347399) as rungs wherever possible. Exact matches are what
   make the speedup comparison well-defined.
 - Keep 3-4 rungs above 0.36 as the fast reservoir — that is what bought the
   traversal and we must not lose it. Do NOT return to the old low-h-heavy ladder.
 - Same depth as this run. Depth was never the problem; do not spend on more of it.
 - Report: per-rung replica-level (not block) cold/hot gap; the crossing against clean
   L=20; cost-normalized Neff/core-hour at the matched h-values; and traversal
   statistics to confirm the re-span preserved transport.
**PRE-COMMITTED, so I cannot reinterpret afterwards:** if traversal stays complete and
the replica-level gaps stay below z=3, PT at L=10 is VALIDATED, the clean L=10 curve
exists, and the benchmark crossing against clean L=20 becomes computable for the first
time. If traversal DEGRADES when rungs move into the window, that is real physics — it
would mean the h_c region itself resists transport — and it would be the strongest
evidence yet that the MCP needs the beta-bridge rather than an h-ladder.

### 6. STANDING CHANGES
 - **Replica-level (between-unit) errors are now the DEFAULT for any cold/hot or
   group-comparison gate**, in PT and no-PT alike. Block errors may be reported
   alongside but must never be the sole basis for a gate verdict. Wire into
   `ensemble_diagnostics.py` and `pt_diagnostics.py`. This is the third time an
   error-model choice has driven a wrong verdict; make it structural.
 - **Every ladder design must state the measurement window it must cover, and a
   pre-submission check must verify rung coverage of that window.** My 20k spec would
   have been caught by a two-line check. Add it to `PRE_SUBMISSION_CHECK.md`.
 - The silently-killed-background-task bug you found is serious and worth its own
   fix; a job reporting "completed" when it was killed can corrupt any readout.

### LOOP CONTAINMENT
Phase 2B-redo is the next compute. The two mini-todos from 20s (benchmark
extrapolation plot, energy-Binder s* cross-check) remain free and can run alongside.
Phase 3 (denser s* sampling) still waits behind them. L=30, L=24 still HOLD.

## UPDATE 2026-07-20u: SWAP-PERIOD OPTIMUM — MEASURED at last (the 20h sec-5(i) deliverable, which the run produced but the report never extracted). PI's physical objection to sp=1 is CORRECT: it is the WORST of the four. Optimum is sp=5. Also a REQUIRED SOFTENING of my own "transport solved" claim.

The PI challenged sp=1 on physical grounds: one sweep of local updates barely changes
the configuration, so the replica cannot relax at its new rung. That objection is
correct, the data confirms it, and the answer had been sitting in job `15215269`
unextracted. **You ran all four swap periods but reported them only as gate variants,
never as the swap-period CURVE that 20h sec 5(i) asked for.** Same pattern as the
traversal result in 20t: the measurement was made, the deliverable was not reported.

### 1. THE CURVE (all 14 replicas; tau_config = decorrelation following one replica, SWEEPS)
  sp    residence/rung    tau_IPS         tau_blocking
   1        2.0 sweeps    7468 +/- 961    3452 +/- 282
   2        3.9 sweeps    4945 +/- 745    3188 +/- 318
   5        9.8 sweeps    3910 +/- 738    **2833 +/- 278**   <- optimum
  10       19.5 sweeps    5178 +/- 1456   3552 +/- 380
**Both estimators put sp=1 worst and sp=5 best, with sp=10 rising again — a genuine
interior optimum.** Residence at the optimum is ~10 sweeps, i.e. the configuration
does need real time at a rung, exactly as the PI argued.
HONESTY ON EFFECT SIZE: by blocking (our more trustworthy estimator) sp1 -> sp5 is
3452 -> 2833, about 1.2x and only ~1.6 sigma. IPS shows 1.9x and ~2.9 sigma. So the
DIRECTION is consistent and physically motivated, the MAGNITUDE is modest and not
decisively resolved. Do not quote "sp5 is 2x better"; quote "sp=5 is the measured
optimum, the effect over the tested range is modest, and sp=1 is measurably worst."

### 2. WHY sp=1 IS NOT CATASTROPHIC, WHICH IS THE SUBTLE PART
The PI's reasoning would suggest sp=1 is useless. It is not, and the reason matters:
**PT swaps satisfy detailed balance at ANY swap period, so the stationary distribution
is exactly correct regardless.** Swap frequency is purely an EFFICIENCY knob, never a
correctness one. Frequent swapping does not corrupt the ensemble at each rung; it just
makes the replica's h random-walk faster than its configuration relaxes, so the
excursions buy less decorrelation per unit time. Hence "worse", not "wrong".
This is worth having straight because C7/F1 in the run manual warn about label travel
in language that reads like a correctness failure. It is not. It is a DIAGNOSTIC
failure (round trips stop implying mixing) plus mild inefficiency.

### 3. THE PART THAT FORCES ME TO SOFTEN 20t — the PI's deeper point
At sp=1, residence is 2 sweeps/rung, so a full ladder traversal costs ~200 sweeps of
random walk, while the configuration's own decorrelation time is ~3450 sweeps.
**The replica traverses the ladder roughly 17x faster than its configuration
decorrelates.** So at sp=1, a completed traversal does NOT by itself imply the
configuration mixed — many traversals occur within one decorrelation time. That is
precisely the C7/F1 "label travel" caveat and precisely the "NOT SUFFICIENT" half of
the corrected F7 rule.
**Therefore my 20t headline needs restating, and I am restating it:**
 WRONG (my 20t wording): "transport is solved" full stop, evidenced by traversal.
 RIGHT: transport is solved AND the configuration is genuinely decorrelating —
 traversal is complete at every sp (14/14 replicas, all 14 rungs), AND tau_config is
 finite and short (~2800-3500 sweeps), AND the replica-level init-memory test passes.
 **It is the CONJUNCTION that retires the blocker, not the traversal count.**
The milestone stands — all three legs hold — but I stated it on the weakest leg, which
is exactly the error the corrected F7 rule exists to prevent. The PI caught it.

### 4. A STRONGLY POSITIVE INDICATION ON SPEEDUP — deliberately NOT quoted as a number
PT (sp5) tau_blocking at fixed rung: 579 / 415 / 252 / 175 sweeps at
h = 0.2925 / 0.3130 / 0.3335 / 0.3476. The no-PT L=10 baseline at comparable low h was
88,826 (h=0.3190), 53,630 (0.322), 48,548 (0.326) sweeps.
Taken at face value that is a factor of order 100-200. **I am NOT quoting it as a
speedup and neither should you.** Two reasons: (i) the per-rung PT series is a
CONCATENATION of disjoint visits, and concatenating non-contiguous segments breaks
correlations artificially, so this tau is biased LOW and the implied speedup biased
HIGH; (ii) the h-values are not matched. The defensible statement is the
replica-following comparison — ~2833 sweeps against 48k-89k at the slow rungs, order
17-31x — and even that is not apples-to-apples.
**Conclusion: indications are strongly positive, order 10-100x, and the honest
measurement still requires the matched-grid redo.** This is the same trap as the
rejected 144x/64x and the 0.32441 crossing; we do not get to keep a number just
because we like its direction.

### 5. CONSEQUENCES FOR THE PHASE 2B-REDO (amends 20t sec 5)
 - **Use sp = 5**, not sp = 2 or 1. Measured optimum, and residence ~10 sweeps gives
   the configuration real relaxation time at each rung.
 - When re-spanning into the benchmark window, RE-MEASURE the curve: the optimum is
   set by the ratio of residence time to local relaxation time, and local tau varies
   strongly with h, so the optimum can move when the rungs move. Test sp = 5 and 10
   at minimum. Do not assume sp=5 transfers.
 - Report the swap-period curve AS A CURVE this time, in the pt_architecture.md
   calibration table, in sweeps, with both estimators. That is the standing 20h
   deliverable and it is still open.
 - Gate wording, final: **traversal completeness AND tau_config AND replica-level
   agreement.** All three, reported together. Never traversal alone.

### 6. STANDING RULE, from this and from 20t
**If a run produces a measurement that a directive asked for, report it as that
deliverable — not folded into a gate verdict.** Twice now (traversal in 20t, the
swap-period curve here) the number existed in the output and had to be extracted by
me afterwards. The gate answer is not a substitute for the requested evidence. This is
the reporting-side twin of my own "specs name the evidence" obligation: I must ask for
the evidence, and the report must surface it as evidence.

## UPDATE 2026-07-20v: I FOUND THE MISTAKE IN THE SWAP-PERIOD APPROACH, and it retires the question rather than re-running it. **We scanned the wrong decade.** Also: full confidence budget for s*. NO new swap-period scan — cancel that spend.

The PI asked whether to re-measure swap period or approach it differently, and asked
for the mistake in our approach. There is one, it is clean, and it means we should
NOT spend another run on this.

### 1. THE MISTAKE: the tested values are all 500-5000x below the scale that matters
Swap acceptance is decided by ENERGY. If the configuration's energy has not changed
since the last attempt, the next attempt reproduces the same accept/reject decision —
**it carries no new information.** So the natural swap period is set by the local
energy relaxation time. Measured, job `15215269`:

  sp    tau(energy)   residence   swap attempts per tau(energy)   tau(ladder index)
   1    5140 sweeps    2.0 sw            5140                        65,120 sweeps
   2    5446           3.9               2723                        67,005
   5    5703           9.8               1141                        68,779
  10    5442          19.5                544                        67,527

**tau(energy) ~ 5,400 sweeps and is INDEPENDENT of swap period.** We tested sp = 1, 2,
5, 10 — every one of them attempts swaps between 544 and 5,140 times per energy
relaxation time. **All four sit deep in the same over-attempting regime.** That is why
the spread was only 1.2x and the optimum shallow: we were not scanning an optimum, we
were scanning noise inside one regime.
THE SMOKING GUN: **tau(ladder index) is FLAT at ~67,000 sweeps across all four.**
A 10x change in attempt frequency produces NO change in rung-space diffusion measured
in sweeps. That is only possible if diffusion is limited by energy decorrelation
rather than by attempt rate — exactly the mechanism above. The PI's physical objection
was not merely correct, it was correct by a larger margin than the scan revealed.

### 2. WHY THIS RETIRES THE QUESTION INSTEAD OF RE-RUNNING IT
The criterion is now PREDICTIVE and needs no scan: **swap_period should sit well below
tau(energy) (so rung diffusion is not starved) but well above the point where attempts
are pure repetition.** tau(energy) is measurable from ANY existing run, including no-PT
baselines, and we already have it: ~5,400 sweeps at L=10 on this line.
Practical choice for the redo: **sp in the range ~50-200 sweeps.** That is 1-2 orders
above the wasteful regime and 1-2 orders below tau(energy). Since tau(ladder index) is
flat, raising sp from 5 to ~100 costs NOTHING in mixing and saves a large amount of
MPI communication and I/O (sample interval = swap period in this harness, so it also
shrinks the output by ~20x).
**DO NOT run a 4-variant swap-period scan again.** It cost 4x the compute and, because
all four points were in one regime, returned a 1.2x non-result. Run ONE swap period.
If you want a check, run TWO that are an order of magnitude apart (e.g. 50 and 500),
not four inside one decade. Fold the freed budget into the ladder re-span, which is
the first-order problem.

### 3. CORRECTION TO MY OWN 20u
I reported "optimum at sp=5" from tau_config. I am withdrawing that as a physical
optimum. With tau(ladder index) flat and all four variants in the over-attempting
regime, the sp=5 minimum is a shallow, ~1.6-sigma feature inside one regime, not a
located optimum. **The honest statement: over the tested range the swap period barely
matters, and the reason is that the whole tested range is wrong.** My 20u sec 5
instruction to "use sp=5" is superseded by sec 2 above.
FURTHER HONEST LIMIT, which I want on the record because it bounds what Phase 2 proved:
I could not cleanly separate genuine configuration decorrelation from rung-jitter in
the replica-following series. tau(observable) ~ 2,800-3,500 sweeps is fast, but a
replica changes rung every ~10 sweeps and the observable varies strongly along the
ladder (1.39 -> 2.13), so part of that apparent decorrelation is the replica moving,
not the configuration relaxing. **Therefore I do NOT have a trustworthy PT tau at
fixed h, and PT's speedup remains genuinely unmeasured.** The 20t sec-2 gate PASS
(replica-level init memory) stands — that test does not depend on tau. But nobody
should read Phase 2 as having demonstrated fast PT mixing. Only the matched-grid redo
can do that, which is one more reason it is the right next spend.

### 4. s* CONFIDENCE BUDGET (PI asked for a confidence level)
Full envelope over 18 independent analysis choices — 2 bases x 3 size-pairs x 3
interpolation schemes (linear/PCHIP/cubic), each bootstrapped over replicas:
  z-basis: 0.3313 0.3321 0.3322 0.3314 0.3326 0.3326 0.3323 0.3338 0.3335
  x-basis: 0.3355 0.3366 0.3370 0.3318 0.3333 0.3331 0.3296 0.3316 0.3307
  ENVELOPE 0.3296 - 0.3370, width 0.0073. Central 0.3328.
  Per-choice statistical error: 0.0008 - 0.0047.
**QUOTE THIS: s* = 0.333 +/- 0.004, where the error is dominated by the interpolation
and basis systematic (+/-0.0037), not by statistics (~0.003).** The two error sources
are comparable; neither can be neglected, and quoting statistics alone would understate
by ~2x.
CONFIDENCE, plainly stated:
 - HIGH confidence the transition lies in 0.325 - 0.350. Both bases, all three sizes,
   two independent observables, and the exact duality check all agree. This is solid.
 - MODERATE confidence in 0.333 +/- 0.004. It is stable across every analysis choice
   tried, and the x/z bases agree within the envelope, which is a real consistency
   check rather than a restatement.
 - LOW confidence in any digit beyond the third. The earlier 0.330968/0.331043/0.331141
   agreement to 1e-4 remains an artifact of one shared grid interval, as I said in 20q.
 - NOT YET FIT FOR PURPOSE: L=120-192 FSS needs ~0.001. We are at 0.004. The gap is a
   factor ~4 and it is now SYSTEMATIC-dominated, so more chains at the same s-values
   will NOT close it. Only a denser s-grid can, which is Phase 3 as reopened in 20q.

### 5. NET RESOURCE VERDICT
 CANCEL: the swap-period re-scan (would have been ~4x a run for a question now
   answered analytically).
 SPEND: the ladder re-span (Phase 2B-redo), single swap period ~50-200 sweeps.
 SPEND NEXT: Phase 3 denser s-grid, since s* is systematic-limited and only a finer
   grid moves it.
 The two free mini-todos from 20s remain free and still open.

### LOOP CONTAINMENT
Phase 2B-redo at ONE swap period. Report tau(energy) for the new ladder as a
by-product so the criterion is re-checked, not re-scanned.
