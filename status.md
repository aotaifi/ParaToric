# Jarvis status — ParaToric (rewrite in place, no diary)

Updated: 2026-07-12.

## Milestone
- milestone: T1-P1 (M0 infra + G2 pilot GO/NO-GO).
- **Feynman's earlier "finalized next steps" (2026-07-10) is SUPERSEDED** by the
  "MASTER PRE-PILOT SEQUENCE" in `doc/feynman_thesis_tojarvis.md` (2026-07-10,
  authored after reading `doc/pt_architecture.md`) — triggered by the sector-rate
  scan's null percolation result plus a separately-discovered blocking bug
  (swap_period defined in moves, not sweeps — quantitatively broken at L=120).
  PI reviewed and gave go-ahead 2026-07-12 to execute the full master sequence.

## M0 gates — all PASS, but "M0 complete" still premature (see tracker for detail)
G0, G1.1 (checkpoint/restart, `e461686`), G1.2 (PT swap-weight + real multi-rank,
`b29ff1c`+fix `12ebdb8`), G1.3 (PT-on/off, z<1) all independently verified on real
cluster hardware — but G1.2/G1.3's "round-trips look fine" result is now understood
to be an L=8-only artifact (swap_period bug, see below) that does NOT transfer to
L=120. Ensemble-diagnostics tool done (`8b6fd05`). "M0 complete" premature until
PT+checkpoint composition exists (M2 blocker) AND the master sequence below clears.

## THE MASTER PRE-PILOT SEQUENCE (Feynman 2026-07-10, PI-authorized 2026-07-12)
Full spec: `doc/feynman_thesis_tojarvis.md` ("MASTER PRE-PILOT SEQUENCE" +
"RESOLVE FIRST" sections) and `doc/pt_architecture.md` (full D1-D4 rationale +
H1-H6 ladder-health invariants). Order matters — each step depends on the one
before it. Pilot sbatch stays PI-gated regardless of how this sequence resolves.

0. **D1 [BLOCKING, DONE — see Progress section below]**: `swap_period` was defined in raw
   Metropolis MOVES, not sweeps (types.hpp:118 default 1000 moves). At L=8 that's
   ~7.8 sweeps (healthy — why G1.2/G1.3's "6/4 round-trips" looked fine). At L=120
   that's ~0.035 sweeps (~29 swaps PER sweep) — replicas barely evolve between
   swaps, so round-trips would be LABEL TRAVEL, not genuine decorrelation. No PT
   run at L>8 is valid until this is fixed. Fix: swap_period_moves = k*(2*L^2),
   k=O(1-5) sweeps. This invalidates trusting the L=8 G1.2/G1.3 round-trip counts
   as evidence for anything at L=120 — they were never wrong, just not
   transferable (pt_architecture.md Sec 3).
1. Validate the percolation OBSERVABLE against Linsel arXiv:2401.08770's published
   benchmark (no PT, small L=10-30, beta=L): lmbda=0.3 fixed (OFF the self-dual
   line, mu=J=1), scan h across the known 3D Ising critical point h_c=0.335(2),
   nu=0.65. Run BOTH percolation_* and plaquette_percolation_* channels to
   identify which one is the electric-string POP (the one reproducing the 0.33
   crossing). Decides: is the sector-rate scan's all-zero POP result (a) a real
   phase-placement issue (ladder entirely confined, s* elsewhere) or (b) an
   observable/basis bug? Convention (PI-corrected 2026-07-10): POP>0 <=>
   DECONFINED, POP=0 <=> CONFINED (Linsel Fig.3; an earlier note had this
   backwards, retracted). percolation_probability = is_winding_percolating()
   (binary, locates crossing s*); percolation_strength = largest_cluster/#links
   (order-parameter magnitude, for beta/nu) — both gated together, both read 0
   in the confined phase.
2. Confirm the DECONFINED RESERVOIR on the self-dual line: at small s (e.g.
   0.05/0.10/0.20) confirm POP>0 AND percolation_strength>0, AND directly measure
   tau(percolation) at a small-s rung to verify it decorrelates the slow
   (percolation/winding) mode, not just fast local modes — if it doesn't, PT has
   nothing to borrow from (H3 in pt_architecture.md).
3. LOCATE s*: wide-s scan on the self-dual line, BOTH directions, no
   pre-committed direction (an earlier "s*<0.22, scan down" guess was retracted
   as unverified) — cross-check against Linsel Fradkin-Shenker / BPV self-dual
   MCP. This repeats/extends the earlier sector-rate scan but now informed by a
   validated observable.
4. **D2**: build an OFFLINE PT diagnostics analyzer over `ladder_index_by_sample`
   producing: strict round-trips (visited_both_ranks, not just endpoint
   touches), tau_ergodic (round-trip time, in SWEEPS), up/down flow f(rung)
   (non-monotonic = bottleneck). Pilot prerequisite — without this, "PT works"
   is unfalsifiable (H2/H3 need to be measurable, not assumed from round-trip
   counts alone).
5. **D4**: generalize PT from the hardwired self-dual line (s:=h=lmbda) to
   independent (h, lmbda) ladders — mostly plumbing (the swap weight already has
   both terms: kink-fugacity in h, integrated-edge in lmbda). Purpose: run PT
   ACROSS Linsel's KNOWN 3D Ising line (lmbda=0.3 fixed, vary h) at L=20-30 and
   confirm it recovers h_c=0.335, nu=0.65 WITH PT on. This is a
   correctness-AT-SCALE check against ground truth, NOT an acceleration
   benchmark (the Ising line is easy/uninformative about the hard MCP) — our
   only current PT-correctness check (G1.3) is an L=8 EASY-point comparison;
   we've never validated PT near a real critical point at moderate L, and the
   self-dual line has no independent known answer to check against (its
   transition IS the unknown MCP), hence the need to go off-line.
6. **D3**: port the PM (acceptance-rate) adaptive ladder builder from
   `code_ruby_duy_pinning_lab` (cross-project reference, PI-authorized),
   adapted to s/(h,lmbda), with robustness guards (windowed/smoothed
   acceptance, alpha-blend with previous ladder, enforce_min_spacing,
   fixed-iteration cutoff — raw PM is noise-sensitive with few swaps/rank).
   PM chosen over LOGR (PI empirical finding: acceptance mode refines grids
   better here). Do NOT port the beta-tempering time-rescaling logic from the
   reference project — ParaToric tempers s with beta FIXED, no rescaling needed.
7. **THEN** stage the actual G2 pilot (PI-gated sbatch, harness hard stop
   regardless of outcome): GO requires BOTH (a) mechanism: per-link acceptance
   in the 20-40% band, no dead link (<1e-4), strict round-trips, AND (b)
   physics: per-replica sector change confirmed + tau(percolation_strength)
   drops >=5x vs the L120 baseline. (a) without (b) = NO-GO.

## STANDING RULE (Feynman 2026-07-10): every future PT run report must include
swap_period in SWEEPS (not moves), per-link edge acceptance (all links, flag any
<1e-4 dead link), STRICT round-trips (visited_both_ranks), up/down flow f(rung)
per replica, tau_ergodic in sweeps, and tau(percolation_strength) at the s* rung.
A PT run report without all of these is incomplete and must not gate a decision.

## Explicitly ON HOLD (do not scope/start without a new explicit PI decision)
- WORM algorithm — correct fallback IF the pilot is MARGINAL/NO-GO, but stays on
  hold until the pilot actually reports. NOT the R5 winding-loop move (which
  stays permanently out) — a worm adds open worldline ends (head/tail) that
  change winding number at polynomial cost; ParaToric has no such machinery
  currently (verified: all current moves are closed pair-create/destroy +
  local/global flips). A bespoke directed-loop design for this constrained
  (A_s,B_p) model, real research/engineering scope — a PI call, not something
  to pre-build.
- PT+checkpoint composition — M2 production blocker, not a pilot item, not now.

## Known non-blocking items carried forward
- Job `15038796` (sampling-window thermalization scan) int32-overflow bug:
  FIXED, commit `af0a3c2` (widened SimSpec::N_thermalization + CLI/C-API/
  pybind to int64_t, pure type-widening, clean rebuild 4/4 ctest, new
  regression test parses 3e9). Rerun of the 12 previously-failed tasks
  (L in {24,32} x 3 h x 2 starts, at 2M sweeps) submitted as job `15038977`
  (Codex sandbox DNS issue recurred even under danger-full-access this time;
  submitted directly from Jarvis's own shell instead). Readout pending.

## Sampling-window thermalization readout: FINALIZED (jobs 15038796 + 15038977)
L=24,h=0.335 (the one red flag, 3.26sigma density disagreement at 500k)
RESOLVED at 2M sweeps: cold/hot now agree (POP z=0.83, strength z=0.71,
density z=1.28 — all well under 3sigma). All other 11 rerun tasks (2M sweeps)
cross-checked against their already-converged 100k values: max z=2.95,
consistent. Final safe thermalization depth per (L,h):

| L  | h=.15 | h=.335 | h=.50 |
|----|------:|-------:|------:|
| 16 | 100k  | 100k   | 100k  |
| 24 | 100k  | **2M** | 100k  |
| 32 | 100k  | 100k   | 100k  |

**Recommended per-L depth for the upcoming Linsel h-scan** (conservative,
covers untested intermediate h since h_c=0.335 is generally the hardest
point): L=16 -> 100k sweeps, L=24 -> 2M sweeps, L=32 -> 100k sweeps. (L=24's
2M requirement specifically at/near h_c is plausible — closer to criticality
-> slower dynamics — and doesn't need to generalize to L=16/32 which tested
clean at 100k throughout.)

## Linsel benchmark re-run (job 15039466, all 54/54 COMPLETED): INCONCLUSIVE
Per-L crossing (plaquette POP=0.5): L16 h=0.384, L24 h=0.367, L32 h=0.364 —
NOT a single common crossing (pairwise: 16/24=0.336, 16/32=0.343, 24/32=0.363),
though all in the right ballpark of h_c=0.335(2), not obviously narrowing with
L as genuine FSS would predict. No credible nu: local-slope fits give
window-dependent nu~1.2-1.9, not 0.65, not stable.
**Cold/hot disagreements at SEVERAL (L,h) points** (>2sigma POP and/or
strength): L16 h={0.25,0.325}; L24 h={0.25,0.335}; L32 h={0.15,0.37}.
**IMPORTANT INTERNAL INCONSISTENCY**: L24,h=0.335 — the EXACT point just
validated as converged (0.83sigma/0.71sigma POP/strength) in the
thermalization-depth-finalization run (jobs 15038796/15038977, SAME 2M-sweep
depth, same physical parameters) — now shows 3.55sigma/3.28sigma disagreement
in THIS run. Same point, same depth, different run, contradictory verdict.
This smells like underestimated error bars from autocorrelation (percolation
observables near a critical point can have long correlation times; both runs'
sampling windows — 100 samples/25 sweeps in the validation run, 1000
samples/25 sweeps here — may be far shorter than the true autocorrelation
time, making naive stderr too small and turning small real fluctuations into
spurious multi-sigma "disagreements"), not necessarily a physics or wiring
problem — but this is a statistics/methodology judgment with real physics
implications (is percolation reliable near h_c at all with current tooling?),
so reporting to Feynman rather than deciding unilaterally, consistent with
this session's practice on other physics-adjacent forks. Full data:
`/scratch/a/A.Otaifi/paratoric_linsel_percolation_benchmark_fixed_20260712/`.

**Feynman's naive-error-artifact hypothesis: AUDITED and REFUTED (commit
`187e490`)**. Feynman's UPDATE 2026-07-12h diagnosed the sigma-grows-with-N
pattern as a naive/uncorrected stderr problem and directed: fix the
estimator (autocorrelation-aware, tau/Neff), switch to Linsel's actual
method (strength Binder-cumulant crossing, not raw POP=0.5), recalibrate the
pass bar (crossing near 0.335 + 3D-Ising-compatible, not tight nu), do NOT
rerun sampling. Clark audited: the stored `_error` field was ALREADY
autocorrelation-corrected via a stationary bootstrap
(`src/statistics/bootstrap.hpp`), not naive. At L24,h=0.335 the PROPERLY
corrected z-scores are still 3.55/3.28 sigma (Neff 832.6/1000 for
probability, 1000/1000 for strength — not severely autocorrelated either).
**The disagreement survives correction — it is real, not an estimator
artifact.** Binder-cumulant crossings ALSO disagree across L-pairs (16/24 at
h=0.336, 24/32 at h=0.361) and only 2 chains/point (cold+hot) violates this
project's own I4 four-chain bar for a proper R-hat check. Made
`ensemble_diagnostics.py` autocorrelation-aware by default anyway (good
standing-rule fix regardless), but the specific hypothesis for THIS
disagreement is refuted. Reported to Feynman.

**Feynman's SECOND diagnosis (UPDATE 2026-07-12i): retracted the naive-error
hypothesis, identified a real physical inconsistency instead** — the
depth-setting scan said L24,h=0.335 needs 2M sweeps but L32,h=0.335 only
needs 100k: BACKWARDS, since thermalization time near a critical point grows
with L (critical slowing, tau~L^z), so L32 at h_c should need AT LEAST as
much as L24, not 20x less. Root cause: that scan used only 2 chains (1
cold+1 hot) — too weak to catch non-convergence via cold/hot disagreement
(I5); within-chain Neff being high does NOT prove convergence, only
decorrelation within whatever basin the chain is stuck in. **DECISIVE TEST
STAGED**: job `15039919`, 8 independent chains (4 cold/4 hot, distinct
seeds) at L=32,h=0.335, checkpoints at 2M/4M/8M sweeps, judged via cold-
GROUP vs hot-GROUP agreement (proper 4-chain R-hat, not 2-chain z-score).

**RESULT (all 8/8 COMPLETED): cold/hot groups agree at EVERY depth tested**
(Rhat 1.0001-1.003 on POP/strength/density, all PASS at 2M/4M/8M) —
literally applying Feynman's rule ("groups converge at some depth =>
under-thermalization confirmed") says CONFIRMED, earliest tested depth 2M.
**BUT: both groups DRIFT TOGETHER as depth increases** (POP
0.699->0.717->0.734, strength 0.427->0.439->0.449, density flat ~0.409) —
i.e. cold/hot never disagreed with EACH OTHER, but neither has clearly
reached a STATIONARY value even at 8M sweeps. I5 (cold==hot) rules out
"stuck in different basins," not "still relaxing together in the same
direction." This is a nuance Feynman's binary decision rule didn't
explicitly cover — reporting it rather than picking an interpretation
myself. Does not support the observable/wiring hypothesis (that would
predict a persistent cold/hot split, not a coherent joint drift). Full
data: `/scratch/a/A.Otaifi/paratoric_thermalization_decisive_l32_20260712/`.
**Feynman CONFIRMED the drift finding (UPDATE 2026-07-13a)**: cold/hot+R-hat
is BLIND to common drift (measures between-chain dispersion, not
stationarity) — fixed the project's standing G1 gate to require both. Drift
steps are non-decelerating (0.018,0.017 per doubling) => algebraic/critical
slowing, not near-plateau; deeper thermalization (16M/32M) likely wouldn't
resolve it either. Prescribed a FREE check on EXISTING data (no new runs):
compute the Binder cumulant of plaquette_percolation_strength at each depth
— if STATIONARY while raw moments drift, the Binder-crossing method
(Linsel's actual method) is viable at 8M; if the Binder ALSO drifts, that's
not a benchmark failure, it's direct evidence the observable is critically
slow WITHOUT PT (the campaign's own raison d'etre showing up here).

**RESULT: Binder cumulant ALSO drifts**, non-decelerating, matching the raw
moments (pooled U: 0.491(10) @2M -> 0.504(12) @4M -> 0.517(7) @8M; cold and
hot groups independently show the same monotonic trend). **Verdict: the
no-PT Linsel benchmark at L32,h_c is not resolvable by more thermalization
depth alone — this is confirmed physics (critical slowing without PT), not
a bug or a benchmark failure.** Per Feynman: proceed toward the PT-BASED
benchmark (D4's validation run: PT ladder across Linsel's fixed-lmbda=0.3
line, confirm h_c=0.335/nu=0.65 recovery WITH PT) rather than continuing to
push the no-PT single-chain approach deeper. D4 (independent h,lmbda PT) is
already implemented, tested, and Martina-reviewed (`e42914d`) — ready to use.

## D4/Step-5 PT-based Linsel benchmark: STAGED and RUNNING (commit `1b2d674`)
3 Slurm/MPI jobs, one per L in {20,24,28}, 15-rank ladders, h in [0.15,0.50]
(dh=0.025) bracketing h_c=0.335, lmbda=0.3 CONSTANT across all ranks (D4's
generalization), mu=J=1, z-basis, swap_period=2.0 sweeps, thermalization
scaled ~L^2 (500k/720k/980k sweeps, first diagnostic pass not a final
answer). Jobs `15041840` (L20), `15041841` (L24), `15041842` (L28) —
confirmed genuinely RUNNING (an initial squeue snapshot suggested a
July-17 start, which was a stale scheduler estimate at submission instant,
not real; sacct confirms all 3 running normally). Hard cap 3h.
**ALL 3 FAILED with OUT_OF_MEMORY** shortly after starting: `--mem=6G` was
requested for the WHOLE node but shared across 15 MPI ranks (~400MB/rank,
clearly insufficient). Simple resource-sizing bug, not a physics/code issue
— fixed the sbatch script's `--mem` to 32G and resubmitted directly
(Jarvis, not a worker dispatch, since this was a one-line mechanical fix):
`15048478` (L20), `15048479` (L24), `15048480` (L28), same
ladder/thermalization params, fresh seeds. Queued (PENDING/Priority, not
stuck). Also
persisted Feynman's new `doc/paratoric_run_manual.md` (consolidates this
whole session's G1-G5 golden rules, B1-B6 validity checklist, C1-C8 pitfall
catalog, minimal recipe, and F1-F8 PT diagnosis checklist) — now mandatory
bootstrap reading per AGENTS.md.
Readout: jobs `15041840/41/42` OOM'd (6G/15 ranks too little), fixed to 32G,
resubmitted as `15048478/79/80` (L20/24/28), all 3 COMPLETED cleanly.

## PT-based Linsel benchmark verdict: NO-GO — plus a CRITICAL, verified code bug
Mechanism: F3 strict round-trips = 0/15 at ALL THREE L values (ladder is not
mixing at all). Per-link acceptance overwhelmingly near-zero (many links
<1e-3, isolated spikes at 0.16-1.0) — far below the 20-40% healthy target.
Physics: Binder-cumulant crossing NOT stationary within the sampling window
(z=1.6-3.5 first-half vs second-half at several h) and drifts across L-pairs
(0.332/0.331 for L20/24 -> 0.327/0.330 for L24/28 -> not converging on
0.335(2)).

**CRITICAL FINDING (independently verified by Jarvis via direct code read,
not just relayed from the worker): the swap-partner-selection logic has a
real bug.** `extended_toric_code_qmc.hpp` (~line 3976): partner is chosen by
FIXED MPI RANK (`partner = (pt_rank%2==phase) ? pt_rank+1 : pt_rank-1`), but
on every ACCEPTED swap, `pt_ladder_index` (and h/lmbda) MIGRATE between
ranks (`pt_ladder_index = partner_index` on accept). Traced through
concretely: after even ONE accepted swap between ranks R,R+1, rank R ends
up holding a LARGER ladder_index than rank R+1 — the rank-axis is no longer
sorted by ladder position. Every subsequent "adjacent-RANK" swap attempt
thereafter can end up pairing ranks holding FAR-APART ladder indices (huge
h-gap => near-zero acceptance almost always, with rare spikes when rank
adjacency happens to coincide with index adjacency again) — this exactly
matches the observed near-total mechanism failure. **Invisible with only 2
ranks** (every prior PT test this session — G1.2/G1.3, D1, D4's own unit
tests — used exactly 2-rank ladders, where rank-adjacency and
index-adjacency are trivially the same single pair regardless of
migration). This is the FIRST run this session to exercise 3+ ranks at
scale, and it appears to have caught a real, previously-invisible bug in
the core swap mechanism — not a physics problem, not a resource/sizing
issue, and not something more thermalization or more chains would fix.

**This calls into question PT-mechanism trust for every run that used
2-rank ladders as "validated" evidence of correctness at scale** (the
2-rank tests remain valid for what they actually tested — the swap-WEIGHT
formula itself — but say nothing about whether the swap-PARTNER selection
generalizes correctly to N>2 ranks, which is precisely what production runs
at L=120 will need).

Full diagnostic data:
`/scratch/a/A.Otaifi/paratoric_pt_linsel_benchmark_20260713/results/`.

## Swap-partner bug: FIXED, unit-reviewed, integration validation pending
Feynman confirmed the bug independently (UPDATE 2026-07-13b), RETRACTED
G1.2/G1.3's 6-rank round-trip claim (job 14982619) as spurious (never
actually validated PT at N>2), and gave an exact fix spec (port from
code_ruby_duy_pinning_lab/worm.update.cpp: MPI_Allgather ladder_index,
build+validate a rank_of_ladder_index permutation, pair by LADDER POSITION
not rank) + a 6-part smoke suite S0-S6 (pt_architecture.md sec 7b). PI gave
go-ahead. Fix implemented and committed (`0289b6c`): new
`parallel_tempering_partner_rank`/`parallel_tempering_rank_of_ladder_index`
free functions, applied identically to both swap-block copies
(thermalization + main sampling loops), swap-weight/migration logic
untouched. Unit-level tests (S1 validation cases, S2 exact partner-mapping
cases incl. the explicit bug-regression assertion) PASS, clean rebuild,
4/4 ctest. **Martina's independent review: PASS on all 6 spec points, no
defect found.** MPI-dependent integration tests (S1b/S3-S6, need N=4/N=8
ranks) could not run in the Codex sandbox (PMIx/socket init failure) —
staged on the real cluster instead: jobs `15052942` (N=4 PT),
`15052943` (N=8 PT), `15052944` (4 independent PT-off references, DONE
cleanly).

## N=4 integration run (15052942): FAILED — the NEW validator caught a
## SEPARATE, real issue (not the partner-selection bug, which is fixed)
Exception: `PT ladder index 2 is reported by both rank 0 and rank 3` —
the permutation-integrity check (working exactly as designed, per spec:
"ABORT on duplicate/invalid") correctly caught a genuine invariant
violation. This is NOT the partner-selection bug (that's independently
Martina-reviewed clean) — this is either (a) a pre-existing
synchronization/race issue in the swap-accept/migration logic that the OLD
code never checked for (no validator existed before today), now correctly
surfaced by the new safety net, or (b) something subtly wrong in how the
fix's Allgather call interacts with the two separate swap-block copies
(thermalization loop vs main loop) at their boundary. Do NOT dismiss the
"h=0.28 lambda=0.28 for all 4 ranks" line in the log as the same issue —
that's the placeholder --h_constant/--lmbda_constant CLI value printed
before PT initializes runtime_h from the ladder, expected and harmless.
N=8 run (15052943) still in progress; PT-off references (15052944)
completed cleanly (unaffected, as expected — no swap logic involved).
NOT resubmitting blindly — dispatching a focused root-cause investigation
before any further integration attempts. Reported to Feynman.

## SECOND bug found and independently confirmed: MPI_Bcast misuse on MPI_COMM_WORLD
Root cause (Clark's static hypothesis, INDEPENDENTLY VERIFIED by Jarvis via
direct code read, `extended_toric_code_qmc.hpp:4044,4201`):
`MPI_Bcast(&accepted, 1, MPI_INT, std::min(pt_rank, partner), MPI_COMM_WORLD)`
is called PER SWAP PAIR, but targets the FULL world communicator. Whenever
a phase has MULTIPLE simultaneous disjoint pairs (any N>=4 — e.g. (0,1) and
(2,3) in the same phase), each pair's Bcast call declares a DIFFERENT root
(0 vs 2) on the SAME communicator — a genuine MPI-standard violation
(collective calls on a communicator must agree on the root across ALL
participants). This is undefined behavior: some ranks can receive the
WRONG pair's "accepted" value, causing asymmetric/incorrect migration —
exactly the mechanism that would produce a duplicate ladder index. This is
a SEPARATE bug from the already-fixed partner-selection issue, likely
PRE-EXISTING (would have affected the ORIGINAL code too, at any N>=4,
independent of the partner-selection bug) — meaning the G1.2/G1.3 6-rank
retraction (job 14982619) may have TWO independent root causes, not one.
Instrumented local-MPI reproduction was blocked (same PMIx/socket sandbox
issue as before), so this is confirmed by static code analysis (by two
independent readers: Clark and Jarvis) rather than an observed instrumented
trace — still solid given the exact code-level match to MPI's documented
collective semantics, but flagging that caveat.
Fix implemented and committed (`0e51978`): world-communicator Bcast
replaced with pair-scoped Send/Recv (deciding rank sends `accepted`
directly to its exact partner via new tags 7106/7208, distinct from
existing 7101-7104/7201-7207). Martina's independent review: PASS on all
6 spec points (tags distinct, no self-partner/deadlock case, no leftover
Bcast anywhere, swap-weight/migration untouched, partner-selection fix
from `0289b6c` untouched). Resubmitted the real integration tests against
this new commit: `15053792` (N=4), `15053793` (N=8) — BOTH COMPLETED
cleanly (no permutation-validator exception this time).

**Independent second-opinion review (James, gpt-5.6-sol, PI-requested):**
PASS, independently confirms Martina's PASS on both `0289b6c` and
`0e51978`. Traced an N=5 (odd) case by hand and found the algorithm holds
(unmatched endpoint safely skips pair traffic, permutation preserved).
Confirmed no other latent MPI collective-misuse patterns in the PT code
path. Confirmed `validate_swap_weight`'s brute-force cross-check still
works correctly given the accept-decision timing change. Confirmed
MPI_Allgather overhead is negligible at production scale (60 bytes/rank/
phase at N=15 vs 57,600 local updates between swaps at L=120). Two minor,
non-blocking hardening suggestions for later: (a) add an explicit odd-N
test case, (b) the permutation-integrity failure path throws collectively
rather than using `MPI_Abort` per the spec's literal wording — currently
safe since all ranks see identical gathered data and would throw
identically, but worth tightening for defense-in-depth.
## S1b/S3/S4/S5/S6 readout: N=4 FULL PASS, N=8 PARTIAL (S3 traversal short)
| Check | N=4 | N=8 |
|---|---|---|
| S1b | PASS (0/8000 invalid permutations) | PASS (0/8000 invalid) |
| S3  | PASS (4/4 ranks span 3, 32 strict RT >=4) | **FAIL**: 6/8 ranks span 7 (rank1 span5, rank6 span6); total RT=16 >=8 (RT bar itself PASSES) |
| S4  | PASS (energy z=[0.80,1.73,1.72,0.15], sigma_z z=[0.62,2.35,0.26,0.41], all <3) | N/A (S4 spec is N=4 only) |
| S5  | PASS (21.4%/14.3%/10.6%, healthy) | PASS (23.9-48.1% across all 7 links, old near-zero+spike fingerprint ABSENT) |
| S6  | PASS (canonical=independent=32) | PASS (canonical=independent=16) |

**Verdict: N=4 is a clean GO. N=8 is NO-GO on the LETTER of S3's strict
full-span bar, but the failure mode is qualitatively benign**: acceptance
is healthy on every link (23-48%, no dead links, the old bug's fingerprint
completely absent) and round-trips already exceed N (16>=8) — this reads
as a finite-smoke-test-budget shortfall (2 of 8 ranks hadn't yet visited
both ladder ends within this run's modest sweep count), NOT a recurrence
of the severed-ladder mechanism failure (which showed near-zero acceptance
everywhere and 0 round-trips). Extending the N=8 run's sweep budget and
rerunning that one check, rather than reopening the code investigation.
Two independent reviewers (Martina, James/gpt-5.6-sol) have already
cleared both fixes; this is a sampling-budget gap in the smoke test, not
new evidence of a bug. Resubmitted N=8 with 3x production sweeps
(N_SAMPLES 8000->24000): job `15054939`, COMPLETED, **N=8 now FULLY
PASSES**: all 8 ranks span the full ladder (0-7), 66 strict round-trips
(>>8 bar), all 7 links healthy (19.1-48.9% acceptance, no dead-link
fingerprint). S6 independent recompute matches (66=66).

## GATE CLEAR: S0-S6 all PASS at N=4 AND N=8 — PT mechanism validated at N>2
Both bugs (partner-selection-by-rank, MPI_Bcast collective-semantics
misuse) fixed, twice independently reviewed clean (Martina, James/
gpt-5.6-sol), and now fully validated by real multi-rank integration tests
at N=4 (full S0-S6 pass) and N=8 (full S0-S6 pass, after a sampling-budget
extension). Per pt_architecture.md sec 7's gate ("S0-S6 all PASS on a
CLEAN rebuild + independent review, AND G1.2/G1.3 re-run at N>=3, BEFORE
any further PT"), the campaign can now proceed to D4 (re-run the PT-based
Linsel benchmark with the fixed code), D3, and eventually the pilot.
Reported to Feynman. This closes out what became a genuinely major,
multi-hour detour (from a 15-rank benchmark's NO-GO through two real,
previously-invisible MPI correctness bugs to a validated fix) — but the PT
mechanism is now on solid footing for the first time this campaign at
N>2 ranks, which none of the prior G1.2/G1.3 claims (job 14982619)
actually were.

## D4 Linsel benchmark REDO with fixed code: STAGED and RUNNING
Same design as the earlier NO-GO attempt (L={20,24,28}, 15-rank ladders,
h in [0.15,0.50] bracketing h_c=0.335, lmbda=0.3 constant, swap_period=2
sweeps, same thermalization depths for a controlled comparison), but built
from the fixed+validated commit `331e37d`. Correct 32G memory sizing
reused. Jobs `15101895` (L20), `15101896` (L24), `15101897` (L28) — ALL COMPLETED
(11/22/37 min respectively).

## D4 Linsel benchmark redo readout: NO-GO, but for a DIFFERENT, benign reason
**Bug fingerprint CONFIRMED GONE**: acceptance profiles are now smooth and
monotonic across links (not the old jagged near-zero-with-random-spikes
pattern) — proves the mechanism fix works. **But raw acceptance is
uniformly too low for the fixed Δh=0.025 ladder spacing**: L20 declines
1.9%->0.014% across links (0/14 formally dead but the tail is marginal);
L24 0.24%->0.0066% (5/14 dead); L28 ALL 14/14 links dead (0.003-0.01%).
0 strict round-trips at every L (need spacing dense enough for adjacent
rungs to actually overlap in energy). Binder-cumulant crossings drift
within the sampling window and don't land consistently on 0.335(2)
(L20/24=0.350(1), L20/28=0.331(1), L24/28=0.327(1) — half-window drift
z up to 6.8).
**Diagnosis: this is a LADDER-DESIGN problem (spacing too coarse,
uniform-in-h rather than adapted to where energy fluctuations actually
require density), not a code bug** — physically expected since
energy-fluctuation scale grows with system volume, so fixed Δh spacing
gives exponentially collapsing acceptance as L grows (matches
pt_architecture.md's own H4/H5 warnings and is exactly what D3, already
built and tested this session, exists to fix). Reported to Feynman. Next:
use D3 (scripts/adaptive_ladder_pm.py) to retune the ladder from this
run's measured acceptance data before another PT-based Linsel attempt.
- `--snapshots`/`sigma_x_tau_avg_obs` O(events) perf regression (not used by
  this campaign) — flagged, not fixed.
- Codex sandbox has intermittent Slurm-controller DNS failures (same root cause
  as earlier concise-alert wrapper failures). Confirmed not a real cluster
  outage. Workaround: escalate the Codex dispatch to `danger-full-access`
  sandbox (JARVIS.md's narrow pre-authorized exception) if it recurs.
- Concise-alert Slurm notifier still has an unresolved DNS issue of its own
  (separate from the above) — monitoring jobs directly via squeue/sacct instead.

## Active workers
None currently running.

## Progress on the master sequence
- **D1 (swap_period in sweeps): DONE**, commit `8e54db2`. Independently verified
  by two separate clean rebuilds. Process note: also had to restore a Boost.Log/
  clang++-20 compatibility shim (`src/logging/trivial_log.hpp`) that an earlier
  review this session had incorrectly concluded was unnecessary — that earlier
  verdict was itself a stale-incremental-build false negative, confirmed by
  rebuilding the pre-D1 commit fresh from a `git archive` extraction. Logged in
  `ai-workflow/jarvis_selfreview.md`.
- **Step 1 (Linsel POP benchmark): root cause diagnosed by Feynman, confirmatory
  test REFUTED the specific numeric prediction — paused again pending Feynman's
  read of that refutation.** Feynman's diagnosis (`doc/feynman_thesis_tojarvis.md`
  "STEP-1 BLOCKER RESOLVED", 2026-07-12): percolation observables (qmc.hpp:
  481-505) never call `rotate_imag_time()` before measuring, unlike
  susceptibilities — so they read the frozen tau=0 base config (same mechanism
  as the earlier energy_lmbda finding). Prediction: `default_spin=+1` -> frozen
  all-up base -> POP~0; `default_spin=-1` -> frozen all-down base -> POP~1.
  PI said proceed; ran the prescribed cheap confirmatory test (no code change)
  first: ELECTRIC `percolation_probability`/`percolation_strength` at L=32,
  h=0.15, `default_spin=-1`, 3 seeds — all came back exactly 0/0, matching
  `default_spin=+1`, NOT the predicted ~1. Sizing looked adequate (not an
  under-thermalization confound as far as I can tell). Candidate explanation
  I have NOT acted on: the electric channel's percolating objects may be
  trivially string-free in ANY uniform base state (both signs), for a reason
  unrelated to the rotate_imag_time diagnosis, while the PLAQUETTE channel's
  asymmetry (0.970 vs 0.132 — the original Finding 2) remains the real
  evidence for the frozen-base mechanism. Reported to Feynman
  (`jarvis_report_tofeynman.md`, 2026-07-12) rather than proceeding to the
  `rotate_imag_time` code fix on a diagnosis whose confirmatory test just came
  back unexpected. Full data:
  `/scratch/a/A.Otaifi/paratoric_electric_defaultspin_minus_confirm_20260712/`.

## Feynman's Step-1 resolution (2026-07-12, PI: "proceed") — closes Step 1
Electric-channel puzzle is REAL PHYSICS, not a bug: dynamic base density
relaxes to 37-40%, square-lattice bond percolation p_c=0.5 -> direct/electric
correctly reads 0 below threshold (calibration confirms across all-(-1)[100%]
->1, dense-random[~53%]->1, dynamic[40%]->0, straddling p_c). The PLAQUETTE
channel is the deconfinement POP (matches Linsel's deconfined-percolates);
electric is the correctly-near-zero dual quantity in this regime.
Finalized next steps: (1) adopt plaquette_percolation_* as POP + document
[still open, Jarvis], (2) apply earned rotate_imag_time fix [DONE, see below],
(3) thermalize much longer, find L=16-32 convergence [open], (4) re-run Linsel
benchmark on plaquette+rotate+long-thermalization, scan h for h_c~0.335/nu~0.65
[open, blocked on (3)].

## D4 (generalize PT to independent h,lmbda): DONE, commit `e42914d`
ladder_lmbda added alongside existing ladder_s (h-ladder); empty ladder_lmbda
= self-dual fallback, bit-for-bit identical to old behavior. Swap log-weight
decoupled: kink-fugacity term now scales with h, integrated-edge term with
lmbda (was single conflated s). CLI: --pt_ladder_lmbda_start/_end. Clean
rebuild, 3/3 ctest (3 old PT tests unmodified + 2 new: h!=lmbda cached-vs-
bruteforce max dev 0; explicit-lmbda-equals-h vs empty-fallback exact match,
2.75e-14). Martina review dispatched but hit Codex's OWN usage-limit wall
(not ours) — retry after Codex's stated reset (~10:43, exact TZ unconfirmed).
Actual Linsel-line-WITH-PT validation run still gated on (3)/(4) above.

## rotate_imag_time same-slice fix: DONE, commit `f08fd9d`
Real bug found: get_sample dispatched observables without a shared rotation;
some individual observable lambdas rotated independently -> evaluation ORDER
determined which tau-slice each observable saw, so percolation observables
could read a different slice than energy/susceptibility observables in the
SAME sample. Fixed: exactly one rotate_imag_time() now precedes observable
dispatch per sample (production/hysteresis/thermalization loops); per-
observable rotations removed. New regression test confirms identical
rotation index across all 4 percolation observables over 3 samples. Clean
rebuild, 3/3 ctest.

## POP identification — SUPERSEDED then CORRECTED (Feynman UPDATE 2026-07-12d)
Earlier entry here (2026-07-12 15:00, "plaquette=POP, electric=its dual") is
WRONG and RETRACTED by Feynman's own follow-up correction. Code fact: both
`is_winding_percolating` (lattice.cpp:3334, clusters VERTICES via sigma_z=-1
edges) and `is_winding_plaquette_percolating` (:3597, clusters PLAQUETTES via
the SAME sigma_z=-1 edge set) are two VIEWS of the same ELECTRIC (sigma_z)
STRING SECTOR — direct-graph vs dual-graph clustering of identical data, NOT
independent e/m order parameters. The genuine magnetic (m, flux/sigma_x-
string) sector requires an X-BASIS run (em self-duality). The p_c=0.5 "planar
duality" story from the earlier entry is explicitly WITHDRAWN as an assertion
(the empirical calibration numbers themselves still stand — all-(-1)[100%]->1,
dense-random[~53%]->1, dynamic[40%]->0 — just not that rigorous-derivation
framing of why).
Consequences (Feynman, unchanged/new):
- Linsel benchmark: UNCHANGED — still a single-electric-sector check; validate
  whichever z-channel reproduces h_c~0.335. The >50%-density confirmatory data
  point (dense-random->1) still stands as calibration evidence either way.
- Topological/MCP characterization: NEW required sub-step — needs BOTH bases
  (z-basis e-percolation AND x-basis m-percolation), with EQUALITY at the
  self-dual point (h=lmbda) as the real MCP/topological signature. Cheap at
  small L, no PT needed.
- VERIFY BEFORE LOCKING (Feynman's explicit instruction, not yet done): run
  the same benchmark point in the X-BASIS, confirm its electric channel shows
  complementary/magnetic behavior, and confirm z-basis-e == x-basis-m at a
  self-dual point. Added to Next action below.

## Next action
1. Job `15038690` (L={16,24,32} x h={0.15,0.335,0.50} x default_spin{+1,-1}
   thermalization checkpoint scan): all 18/18 COMPLETED. Readout done, but
   INCONCLUSIVE by design, not a negative result: the checkpoint scan
   (reusing the decisive-experiment single-snapshot pattern) records ONE
   configuration per checkpoint, not a sampling window with mean/error — so
   "cold/hot differ" cannot be distinguished from ordinary sampling noise at
   any of the 9 (L,h) points (Jones's readout: closest was L=32,h=0.15,
   final density/strength diffs 0.0005/0.0225, but noisy en route — not a
   clean plateau-together or plateau-apart signal anywhere). This is a
   MEASUREMENT-DESIGN gap (single snapshot vs proper statistics), not a
   physics question — fixing directly rather than routing to Feynman, same
   class of fix as the earlier G2 sector-rate scan v1->v2 redesign.
   NEXT: redispatch with a proper sampling-window design (mean+stderr over
   real N_samples per candidate depth, cold vs hot) at a bounded subset of
   depths (not all 7 checkpoints) to actually establish convergence.
2b. Feynman ROOT-CAUSE + confirmatory test (UPDATE 2026-07-12f): my earlier
   cross-basis comparison paired the WRONG channels — self-duality swaps
   BOTH z<->x AND vertices<->plaquettes (direct<->dual graph) simultaneously,
   so the correct pairs are z-percolation<->x-plaquette_percolation (E<->M')
   and z-plaquette_percolation<->x-percolation (E'<->M), NOT the same-graph
   channels (z-percolation vs x-percolation) I compared before. Confirmatory
   test at the equilibrated s=0.22, L=8, 1M-sweep point, 4 seeds:
     z-perc        .2863(32)/.07893(93)   z-plaquette   .7451(32)/.54831(267)
     x-perc        .7420(33)/.30455(146)  x-plaquette   .2938(44)/.14200(207)
   PROBABILITY (POP) MATCHES on both correct pairs (E<->M' 1.37sigma,
   E'<->M 0.66sigma).
   **RESOLVED (Feynman UPDATE 2026-07-12g): e/m duality is CONFIRMED via
   probability; the strength mismatch is a NORMALIZATION/DEFINITION
   difference, NOT a bug** — verified by code read (lattice.cpp:3729,3782):
   `percolation_strength` = (#-1 edges in largest cluster)/edge_count(2L^2);
   `plaquette_percolation_strength` = (#plaquettes in largest
   cluster)/plaq_count(L^2) — different objects, different denominators.
   Proof it's definitional not a bug: BOTH dual pairs are off by the SAME
   basis-independent factor ~1.80 (.142/.079=1.80, .548/.305=1.80) — a
   mapping/compute bug would not produce an identical constant factor.
   Both channels remain exponent-safe (P~L^(-beta/nu) either way, prefactor
   cancels). DIRECTIVE: do not force the two strengths equal; use
   PROBABILITY for the duality/I2 check (passes); use each strength channel
   ONLY for its own FSS, never cross-compared; document the ~1.80 factor so
   it isn't re-flagged. Optional low-pri, not yet done: check if ~1.80 is
   L-independent (trivial geometric normalization) or L-dependent (different
   geometric content — keep channels separate regardless, one POP is used
   for the campaign anyway). **e/m sub-step UNBLOCKED, topological/MCP
   characterization can proceed** whenever prioritized (not urgent right
   now — main line is still the single-z-sector Linsel benchmark).
2. X-BASIS self-duality check: FIRST PASS was uncontrolled for thermalization
   (Feynman UPDATE 2026-07-12e retracted the "duality failure" reading;
   flagged a specific alternative — WINDING-SECTOR TRAPPING, since all
   current moves are local-only (no worm; R5 winding-loop move DOA) and
   could in principle leave cold/hot stuck in different topological sectors
   FOREVER, a fundamentally different failure from slow thermalization with
   the OPPOSITE fix, PT/worm vs "run longer"). Feynman prescribed a decisive
   discriminating check: cold/hot vs sweeps, both bases, fixed s=0.22,
   L=8, checkpoints to 1M sweeps.
   **RESULT (DECISIVE): cold/hot CONVERGE cleanly WITHIN each basis** (gap
   0.010 z-prob / 0.004 x-prob at 1M sweeps, both closing monotonically from
   10k) — rules out BOTH under-thermalization at s=0.22 AND sector-trapping.
   **But z-e and x-m PLATEAU ~0.45(prob)/~0.22(strength) APART**, flat from
   50k through 1M sweeps (z~0.29, x~0.75, no trend). This is a genuine,
   reproducible CROSS-BASIS discrepancy that basis-internal thermalization
   cannot explain — points to a basis mapping/parameter-convention or
   observable-wiring bug, not physics, not sector-trapping. Full data:
   `/scratch/a/A.Otaifi/paratoric_angle_b_convergence_s022_20260712/
   checkpoint_means.csv`. Reported to Feynman. PAUSED again pending
   Feynman's read — do not guess at the specific wiring bug myself. Does
   NOT affect D4, the rotate_imag_time fix, or the single-z-sector Linsel
   benchmark plan.
3. Once (1) lands: re-run the Linsel benchmark (item 4, single z-sector,
   unaffected by the correction) on the converged thermalization depth,
   scan h for h_c~0.335/nu~0.65.
2. Once unblocked: (a) Martina reviews D4 diff `e42914d`; (b) James stages
   the longer-thermalization L={16,24,32} x h={0.15,0.335,0.50} x
   default_spin{+1,-1} checkpoint scan (extending the decisive-experiment
   pattern past 10k sweeps) to find where cold/hot converge on the plaquette
   channel.
3. Once (2b) lands: re-run the Linsel benchmark (item 4) on
   plaquette+rotate+long-thermalization, scan h for h_c~0.335/nu~0.65.

## Feynman's revised D4 plan (UPDATE 2026-07-13c): properly-resourced ladder, not PM-first
My "use D3/PM to retune" proposal was WRONG: PM only redistributes spacing
across a FIXED rung count (preserves span+count, per adaptive_ladder_pm.py's
own guard: mean_p<=1e-12 returns the ladder untouched) — it cannot rescue
dead links, confirmed by D3's own earlier "PM alone does NOT solve a
bottleneck case" finding. The actual problem: energy-fluctuation scale
sigma_Delta ~ dh*L^(3/2) (Var E ~ volume ~ L^3 at beta=L), so fixed
dh=0.025 gives acceptance collapsing as L^(3/2) — matches the measured
1.9%->0.005% L20->L28 falloff. To hold acceptance fixed, dh ~ L^(-3/2) =>
N_rungs ~ L^(3/2). AND rung count is not a free knob either: round-trip
time tau_RT ~ N^2/A(N) is minimized near ~23% acceptance (the 20-40% rule
of thumb); overshooting N by 4x makes round-trips ~5x SLOWER, not faster.

Action plan (PI: "proceed"):
1. Redo the benchmark properly resourced (NOT PM-first): L20 ~35-40 rungs
   (dh~0.008 near h_c, ~0.013 away), L24 ~45-50, L28 ~55-60 (dh~0.006),
   non-uniform, DENSER NEAR h_c=0.335. 40-60 ranks = 1-2 nodes (needs a
   node-capacity check, see item 4).
2. THEN apply D3/PM to equalize residual unevenness once links are alive.
3. Calibrate rather than trust the derived constant: short acceptance-vs-dh
   scan at L=20 and L=28 (~1h), fit dh_good(L)~L^(-3/2), run alongside (1).
4. Resource check: max ranks/node on this cluster + whether inter-node PT
   is viable (needed since N_opt will exceed 1 node at L=120).
5. DEFERRED (explicit, not now): L=120 pilot walltime scaling is severe
   (tau_RT~N^2~L^3 sweeps, wallclock/RT~L^6 overall — naive scaling implies
   weeks at L=120 even for a narrow h-range). This is a CAMPAIGN-LEVEL
   resourcing question (the original pilot budget was sized from walltime
   alone, never from the acceptance-driven rung count) — Feynman explicitly
   says raise this AFTER step 3's calibration data lands, not speculate now.

Dispatching: calibration scan (3) + resource check (4) + properly-resourced
benchmark staging (1) as a combined James task.

## Calibration + properly-resourced benchmark: STAGED and RUNNING
Resource check confirmed: 128 physical cores/node available on the cluster
partition, so all designs (40/48/60 ranks) fit on ONE node — no multi-node
MPI complexity needed. Codex sandbox's Slurm-DNS issue recurred for
submission; submitted directly from Jarvis's own shell.
- Calibration (acceptance vs dh at L=20/28, near h_c): job `15118489`,
  8-task array, RUNNING.
- Properly-resourced Linsel benchmark (Feynman's a priori L^(-3/2) ladders,
  non-uniform density denser near h_c, per his fallback prescription since
  calibration runs in parallel per his own "alongside" instruction):
  L20=40 rungs (job `15118635`), L24=48 rungs (`15118636`),
  L28=60 rungs (`15118637`) — all RUNNING, built from commit `98c45e8`.
Readout pending both.

## Calibration + resourced benchmark readout: MAJOR PROGRESS, still NO-GO (localized bottleneck, not under-resourcing)
**Calibration confirms the scaling law**: fitted exponent ~1.69 (close to
Feynman's predicted 1.5). dh for ~25-30% acceptance: L20 ~0.0087,
L28 ~0.0049 (Feynman's a priori 0.008/0.006 were in the right ballpark,
slightly coarse at L28).

**Mechanism, resourced ladders (39/47/59 links)**: median acceptance is
now HEALTHY at all 3 L (20-23%, squarely in the 20-40% target band) — a
huge improvement over the earlier uniform ladder's fully-dead L28. BUT
still 0 strict round-trips at every L. Root cause is now different: flow
is sharply BOTTLENECKED at one specific rung transition per L (e.g. L28:
f_up 0.997->0.007 between adjacent rungs near the middle of the ladder) —
a LOCALIZED collapse, not the earlier uniform-collapse-with-L pattern.
This matches pt_architecture.md's own flagged H5 concern: a (weakly)
first-order-like MCP gives a central-rung acceptance/flow bottleneck even
when the REST of the ladder is healthy — this could be genuine physics
(coexistence near h_c) or just needing D3/PM fine-tuning of that one
region now that links are alive (Feynman's own precondition for using PM
is now satisfied: "resource FIRST, PM SECOND... PM needs a live signal").

**Physics**: Binder-cumulant crossings are now MUCH tighter/more
self-consistent (0.3406-0.3437 across all 3 L-pairs) than either prior
attempt (0.336/0.343/0.363 initial; 0.350/0.331/0.327 uniform-fixed-
mechanism) — genuine improvement, though still showing some within-window
drift and not perfectly centered on 0.335(2).

Verdict: NO-GO on the strict round-trip bar, but this is now a
qualitatively different, more specific, and more promising problem than
before. Not deciding myself whether the bottleneck is D3-fixable ladder
mechanics or a genuine first-order physics signal — routing to Feynman.

## Three-number diagnostic result: too-short REFUTED, unhealthy-link CONFIRMED at h_c, PLUS a separate seam artifact
| L | attempts/replica (actual) | attempts needed for 3 RT | ratio | bottleneck link acceptance vs median | bottleneck h-range | branch |
|---|---:|---:|---:|---|---|---|
| 20 | 341,250 | 22,459 | **15.2x over** | 7.60% vs 21.37% | 0.335-0.342 (AT h_c) | unhealthy link |
| 24 | 450,417 | 30,601 | **14.7x over** | 6.01-7.05% vs 22.59% | 0.341-0.353 (AT h_c) | unhealthy link(s) |
| 28 | 580,167 | 53,190 | **10.9x over** | 3.43% vs 20.30% | 0.340-0.344 (AT h_c) | unhealthy link |

**Branch 1 (too-short) REFUTED**: every run had 11-15x MORE swap attempts
than needed for 3 round-trips, yet got 0. Not a time problem.
**Branch 2 (unhealthy specific link) CONFIRMED**, and it sits RIGHT AT
h_c=0.335 in all 3 L (not at the piecewise seams h=0.25/0.42) — the
bottleneck link's acceptance is 3-7x below the ladder's own median.
**Bonus finding, Branch 3 confirmed separately**: a SECOND, even worse
acceptance hole exists near h~0.43-0.44 (2.2-5.3%, below even the h_c
bottleneck) — just above the upper piecewise seam at h=0.42 — this DOES
look like the seam-artifact Feynman flagged (piecewise dh discontinuity),
distinct from the h_c dip.
Physical read (not yet confirmed with Feynman, offering candidate): a mild
acceptance dip AT h_c is not surprising even for a continuous transition
(critical fluctuations enhance Var(E) right at criticality) — the smooth-
profile prescription just needs to reflect the ACTUAL local Var(E)(h),
not an assumed smooth curve. PM is now legitimate to apply (links are
alive) and would naturally redistribute density toward BOTH weak spots
(h_c dip + seam artifact) since it works from measured acceptance
directly, regardless of cause.
Reported to Feynman. Proceeding to apply D3/PM to the measured per-link
acceptance data from this run (subsumes the "smooth the profile" fix,
since PM redistributes based on measured data, not the piecewise
construction) and rerun — this follows directly from Feynman's own
pre-specified decision tree (branch 2 -> "PM fine-tunes that region, now
legitimate").

## Anchor plot: DONE (commit e8857f6)
doc/figures/linsel_binder_crossings_pt_resourced_20260716.png — Binder
cumulant vs h, L=20/24/28, autocorrelation-aware block-bootstrap error
bars, shaded h_c=0.335(2) band, annotated crossings. Recomputed values
match earlier readout (L20/24=0.34373, L20/28=0.34155, L24/28=0.34059).
doc/benchmark_report.md sec 6 updated with the path; PROVISIONAL status
left untouched (0 strict round-trips remains the open confound).

## PM coordinate-drift issue: CONFIRMED independently (Martina)
James's finding holds up: scripts/adaptive_ladder_pm.py redistributes by
LINK INDEX, and each of its iterations rebuilds from the already-moved
ladder — so the physical h-value at a given index drifts across
iterations, meaning multi-iteration PM (James ran 4) can end up denser at
the WRONG physical location relative to the measured acceptance data. No
built-in physical-coordinate anchoring mode exists. Mitigation:
`--max-iterations 1` avoids the multi-step drift (single PM step applied
directly to the measured physical-acceptance snapshot, no re-drift
possible since there's only one step). True h-anchoring for multi-iteration
use would need a code change (remeasure/reanchor between iterations) —
out of scope for now. Proceeding with single-iteration PM per this
narrow, confirmed technical fix (not a new design question, so not
blocking on Feynman for this specific mechanical correction — reporting
for awareness alongside the plot).

## Single-iteration PM ALSO fails the h_c density check — STOPPING, need Feynman
James's second attempt (--max-iterations 1, the confirmed mitigation)
STILL fails: h=0.42-0.44 seam correctly gets denser (e.g. L20:
0.011429->0.006411, right direction), but h_c region (h~0.340) gets
COARSER, not denser (L20: 0.007083->0.008672 — WRONG direction), even
with only one PM step. James correctly stopped again rather than submit.
This means the "single-iteration avoids drift" mitigation (which Martina
and I both endorsed) was itself insufficient. Candidate alternative
explanation (not yet confirmed, offering for Feynman): PM conserves TOTAL
SPAN and redistributes proportionally to inverse-acceptance, so with TWO
simultaneous weak spots of DIFFERENT severity (h_c at 3-7% vs the seam at
an even-worse 2-5%), PM may correctly/legitimately prioritize the WORSE
spot (the seam) at the expense of the already-partially-dense h_c region
— a genuine zero-sum tradeoff under fixed span, not necessarily a
"coordinate drift" bug at all. Two mechanical fix attempts have now failed
(4-iteration and 1-iteration); STOPPING further PM-based attempts here
rather than continuing to guess — reporting to Feynman for direction
(possible options: widen the total span so both regions can densify
without trading off against each other, treat the two weak spots
sequentially rather than simultaneously, or increase rung count directly
near h_c rather than relying on redistribution).

## TREND INVERSION RESOLVED: channel was RIGHT, transform was WRONG
Free decisive check (existing clean resourced-benchmark data, both z-basis
channels, no new sims): `plaquette_percolation_probability` clearly shows
the correct deconfined-electric-POP pattern (high ~0.82-0.89 at small h,
falling to ~0.22-0.38 at large h, for every L) — **CONFIRMS the earlier
plaquette=POP identification was right all along**, refuting the
wrong-channel hypothesis. `percolation_probability` (direct channel) stays
near-zero at small h and rises only modestly — the complementary pattern,
as expected.
**Actual root cause**: Linsel's Fig.4a plots the RAW ratio r=<P^4>/<P^2>^2
directly (unbounded, log-scale 10^0->10^2 axis), but our anchor plot
computed the TRANSFORMED U=1-r/3 (the standard bounded Ising-convention
Binder cumulant). Since 1-r/3 is monotonically DECREASING in r, plotting
U instead of r inverts the trend even with the identical underlying
formula and the CORRECT channel — a plotting-transform mismatch, not a
channel or physics bug. (This also corrects my own earlier "sign
convention, probably h-axis or channel" guess to the user — the specific
mechanism is now nailed down: it's literally "U vs raw ratio", not the
channel or the h-axis direction.)
Next: redo the anchor plot plotting the RAW ratio (not U) from the
already-plaquette-correct existing data, as a free/cheap first check;
then, per Feynman's order-of-work, redo at Linsel's exact L={10,20,30}
(needs new L=10/30 runs, properly resourced per the calibration-informed
N~L^1.5 scaling) before promoting benchmark_report.md off PROVISIONAL.

## Corrected raw-ratio plot: DONE, trend now matches Linsel (commit 0a32e19)
doc/figures/linsel_raw_ratio_crossings_pt_resourced_20260716.png — same
plaquette-channel data, raw ratio <P^4>/<P^2>^2 (log scale) instead of U,
now correctly INCREASES with h for all 3 L, visually matching Linsel's
Fig.4a shape. Crossings unchanged (0.34373/0.34155/0.34059, as expected
since crossings are invariant under the monotonic transform). Updated
paratoric_run_manual.md A8 to resolve Feynman's "UNVALIDATED channel" flag
— channel confirmed correct, bug was the transform.
Remaining before promoting benchmark_report.md off PROVISIONAL (Feynman's
order-of-work): (1) get round-trips (0 strict RT still open, needs the
localized-bottleneck ladder fix), (2) rerun at Linsel's EXACT L={10,20,30}
(we used 20/24/28) for a true overlay-able benchmark.

## Round-trip fix: increased rung budget (not PM redistribution) — STAGED and RUNNING
Per PI priority ("get round-trips"), diagnosed the PM failures as a
genuine zero-sum tradeoff (PM conserves total span/rung count, so with
two weak spots of different severity it prioritizes one at the expense
of the other) rather than a residual bug. Fix: new
scripts/acceptance_informed_ladder.py builds a continuous,
measured-acceptance-informed density profile across the WHOLE h range
(not 3-segment piecewise), with rung count increased ~50% (L20 40->60,
L24 48->72, L28 60->90) so both weak spots (h_c bottleneck, h~0.42-0.44
seam) can densify without competing. Verified BEFORE submitting: gap
ratios (new/original) at both spots are <1 for all 3 L (genuinely
denser this time, unlike either PM attempt). Jobs `15126362` (L20),
`15126364` (L24), `15126363` (L28) — confirmed running, ~15/30/40 min
ETA. Readout will check specifically: >=3 strict round-trips per L.

## Acceptance-informed ladder readout: seam FIXED, h_c bottleneck PERSISTS (was masked)
Seam (h~0.435) is fully healed: production-only acceptance 37-40% at all
L, matching target. BUT round-trips still 0/0/0 for L=20/24/28. Root
cause found: the standing acceptance diagnostic COMBINES
thermalization-phase + production-phase swap counters, and
thermalization-phase acceptance was masking a persistent problem —
PRODUCTION-ONLY (stationary) acceptance at the h_c link is still only
4.2%/3.9%/9.9% (L20/24/28) vs the combined-counter view's misleadingly
healthy 18.6%/22.5%/26.9%. Flow remains non-monotonic at h_c (0.92->0.09,
0.93->0.13, 0.79->0.20) — a genuine, still-unresolved stationary-regime
bottleneck, not fixed by the uniform +50% budget increase (which mostly
helped diffusion elsewhere/during thermalization, not the persistent
h_c-specific problem). Binder crossings: L20/28 and L24/28 land close to
h_c (0.336, 0.336), but L20/24 shows THREE crossings (noisy/non-monotonic
in that specific pair) — consistent with the same h_c mixing problem.
**Verdict: PARTIAL, still NO-GO on round-trips.** Process fix identified:
future acceptance diagnostics MUST separate production-only from
thermalization-phase counters (the combined view is misleading).
Physical candidate (not yet confirmed): Var(E) may have a genuine SHARP
PEAK right at h_c (expected for any critical point, even continuous ones,
via specific-heat-like divergence in a finite system) requiring much MORE
aggressive, narrowly-targeted density there than a broad-based scaling
gives — not just moderately denser, but specifically pinned tighter in a
narrow h_c window. Next: build an even more targeted ladder refinement
using PRODUCTION-ONLY acceptance data (not combined), concentrating
substantially more density in a narrow window right at h_c specifically.

## Targeted h_c ladder: STAGED, with a submission mixup resolved + a new seed-overflow bug found
James's dispatch accidentally double-submitted (a DNS-retry race created
two separate script/directory variants): the "canonical" set (jobs
15134326/27/28, dir paratoric_pt_linsel_hc_targeted_20260717) FAILED
immediately with `Exception caught: the argument ('202607172063') for
option '--seed' is invalid` — a NEW int32-overflow-class bug: a
timestamp-derived seed (~2e11) exceeds the CLI's --seed argument range
(same class as the earlier N_thermalization overflow, fixed in `af0a3c2`,
but --seed itself was never widened). The OTHER set (jobs 15134342/43/44,
dir paratoric_pt_linsel_targeted_hc_20260717, 66/78/96 rungs) used a
different/valid seed and is confirmed RUNNING CLEANLY (5-6min in, no
crash). No data-corruption risk — the two attempts wrote to genuinely
separate output directories. Treating 15134342/43/44 as the real run;
the failed canonical set is dead, nothing to clean up.
Carried-forward known issue (not blocking, not fixed now): --seed CLI
argument needs the same int64 widening as --N_thermalization got in
af0a3c2, to avoid this recurring with large timestamp-derived seeds.
Local h_c-window rung density (per James, production-only informed):
subdivided the minimum-acceptance link + one shoulder each side by 3x;
new rung counts L20=66, L24=78, L28=96 (up from 60/72/90). Readout
pending, will check specifically for >=3 strict round-trips via
production-only trajectory motion (not combined counters).

## Targeted h_c ladder (3rd attempt): STILL NO-GO, 0/0/0 round-trips — STOPPING iteration, reporting up
h_c window itself is now VERY healthy (72.7-76.4% L20, 67.1-76.3% L24,
62.3-75.5% L28 — arguably over-corrected) and the seam remains healthy
(36-42%). BUT round-trips are still 0/0/0 for all L. Root cause: the
narrow local refinement created NEW unsupported gaps immediately ADJACENT
to the h_c spike (zero cross-rung flow mixing in large regions: L20
[0.346,0.363], L24 [0.328,0.371], L28 [0.313,0.406]) — acceptance there
(13-20%) isn't dead, but flow analysis shows literally no traversal
activity through those specific gaps, blocking end-to-end round-trips
even though every individual region looks locally reasonable.
**Implementation discrepancy found**: James's report claimed 66/78/96
rungs with "minimum link + one shoulder on each side" refined; the actual
submitted job used 62/74/92 rungs with only ONE link refined (not the
shoulders as instructed) — a real gap between what was asked/reported and
what was implemented, which likely explains why new adjacent gaps formed
(shoulders were the natural place to prevent them).
**This is the 3rd distinct ladder-design iteration (uniform -> broad
acceptance-informed -> narrow targeted), each fixing the SPECIFIC problem
found but revealing a new, narrower one elsewhere.** Per this project's
own loop-containment discipline (max 3 rounds before stop-and-report,
not open-ended iteration), STOPPING here rather than dispatching a 4th
mechanical attempt. Reporting to Feynman/PI: the pattern suggests
per-round hand-patching of individual weak links may not converge:
recommend either (a) a genuinely smooth, continuous, FULLY-recomputed
density profile fit to ALL measured production-only acceptance data at
once (not patching one region at a time), verified end-to-end before
submission, or (b) reconsidering whether round-trips at this rung count
are achievable at all without a fundamentally different ladder strategy.
Binder crossings (informational): now single-valued per pair (the L20/24
3-crossing noise issue resolved) but spread 0.00797 across the 3 pairs
(0.3330/0.3376/0.3410) — not yet stable, consistent with the ladder still
not actually mixing (0 round-trips means these are NOT trustworthy
equilibrium crossings).

## LANE 1 free check: NUANCED — median-A says "run was long enough", harmonic-mean-A (the physically correct statistic for serial bottlenecks) says L24/L28 were still short
Using the ORIGINAL (N=40/48/60) properly-resourced ladder's own data
(jobs 15118635/36/37), computed tau_RT two ways:
- **Median acceptance** (Feynman's simple check): actual production swap
  attempts already EXCEED 3*tau_RT for all 3 L (ratios 4.4x/3.2x/1.7x) —
  this reading says "ladder was fine, just needed nothing more."
- **Harmonic-mean acceptance** (the mathematically correct statistic for
  a SERIAL chain of links with uneven acceptance — dominated by the
  weakest link, like resistors in series, not simple averaging): ratios
  are 2.84/0.78/0.13 for L20/24/28 — L20 borderline-adequate, **L24 and
  L28 genuinely SHORT of 3*tau_RT**, L28 badly so (only 13% of needed
  length). Minimum production acceptance values found: L20 2.7%, L24
  0.44%, L28 0.028% — real individual weak links exist even in the
  ORIGINAL, not-yet-over-corrected design; the median statistic averages
  these away.
**Reading**: this REFINES rather than contradicts Feynman's core insight
(don't uncontrollably grow N, avoid over-correcting local acceptance) —
it means the correct combined fix is a SMOOTH, modest density adjustment
informed by harmonic-mean-aware analysis (not median) PLUS adequate run
length computed from the CORRECT (harmonic, bottleneck-aware) tau_RT, not
the naive median-based one. Reported to Feynman for their read before
concluding "just run longer" definitively.

## LANE 2 (L=10/20/30 overlay benchmark): STAGED and RUNNING
Ladders sized via the sqrt(Var E)/L^(3/2) scaling law, reusing L=20's
ORIGINAL (N=40, not over-corrected) design as reference; L10=14 rungs,
L30=67 rungs. Pre-submission design-vs-intended verification added this
time (PRE_SUBMISSION_CHECK.md: empty diff PASS, exact equality PASS,
strictly increasing links PASS) — addresses the earlier "submitted !=
intended" gap Feynman flagged. Generous run-length safety margin (3-5x
original production sweeps) applied given Lane 1's harmonic-mean finding.
Jobs `15140239` (L10), `15140247` (L20), `15140246` (L30) — submitted,
3h caps, ~5-10/20-30/55-75 min expected once running.

## Lane 2 (L=10/20/30 exact-size overlay): NO-GO — 4th consecutive round-trip failure
Round-trips: 0/0/0 (production-only), even with the generous 3-5x
run-length safety margin. Trend correctly matches Linsel (monotonic
increase). Crossings 0.3441/0.3421/0.3408 (spread 0.0033) — NOT
meaningfully more stable than the earlier L={20,24,28} attempts (spread
was also ~0.003 there). L30 shows a severe, specific bottleneck: min
acceptance 0.24% at h=[0.339,0.344], right at h_c — harmonic-mean
acceptance is 6.3% vs 21.5% median, confirming (again) the harmonic/median
gap matters and a genuine serial bottleneck persists even with the longer
run.
**This is the 4th consecutive failed attempt at round-trips specifically**
(3 ladder redesigns + this longer-run variant), all blocked by a
persistent, severe minimum-acceptance bottleneck right at h_c. STOPPING
further mechanical iteration here per loop-containment discipline — this
now looks like it needs a strategic decision, not another tactical patch.
**Candidate strategic alternative worth Feynman's/PI's consideration**
(not deciding myself): the L=10/20/30 exact-size overlay benchmark's
PURPOSE is to reproduce Linsel's KNOWN answer at sizes where THEY did not
need PT (their paper's methods do not describe PT for this Ising-line
benchmark). Our own critical-slowing-without-PT finding was specifically
at L=32 (near the TOP of their range, not L=10-20). It may be that a
properly-deep NO-PT single-chain run is sufficient at L=10/20/30 (as it
evidently was for Linsel), decoupling "do we reproduce the known Ising
answer" from "does our PT mechanism work" — the latter is already
separately, cleanly validated at the N=4/N=8 unit-test scale (S0-S6 all
PASS). Fighting PT-ladder engineering specifically to reproduce an
overlay benchmark that may not need PT at all could be solving a harder
problem than necessary.
Reported to Feynman with this finding + candidate alternative; benchmark_report.md
remains PROVISIONAL. Not proceeding to a 5th mechanical ladder attempt

## Feynman GO (2026-07-18): decouple benchmark from PT + marker test + beta-bridge documented (not built)
Feynman's UPDATE 2026-07-18 in doc/feynman_thesis_tojarvis.md confirms Jarvis's
decouple proposal, adds two more decisions, ORDER: (1) close benchmark NO-PT,
(2) marker test on an existing failed PT run, (3) resume self-dual PT path.
beta-bridge spec already written (pt_architecture.md sec 9, 2026-07-18) — DOCUMENT
only, build IFF self-dual PT stalls at s* with first-order signature.

## L=10 NO-PT pilot: NOT VALIDATED — genuine h_c-localized critical slowing even at the cheapest L
Executing Decision 1. Deep NO-PT (pure local Metropolis) pilot at L=10, 14 h-values
in [0.28,0.40] denser near h_c=0.335, 4 cold+4 hot chains, checkpoints
200k/500k/1M/2M/4M sweeps, 2M production window. Job **15141215**, 112/112
COMPLETED cleanly. Full Part B checklist run (ensemble_diagnostics.py).
**VERDICT: NOT VALIDATED, do not scale to L=20/30 yet.** Failures cluster tightly
in h=[0.318989,0.343521] (brackets h_c exactly); everything outside that window
passes cleanly. Inside: non-plateaued drift right at h_c (ratio still falling
1.665->1.446 across 200k->4M checkpoints, slope z=3.71), cold/hot disagreement up
to z=29.6, tau_int blowup to ~7,375 sweeps (~300-500x off-critical) with Neff
135-172 built on a non-stationary series. global_single_spin_flip B6 tripwire
(<1e-4 accept, 112/112 chains) is NOT new — already established as expected
(run_tracker_multicritical.md:264, Martina 2026-07-11), not investigating further.
**Strategic flag**: this is the same PT-shaped failure mode (basin disagreement +
non-plateaued drift, localized at h_c) already showing at L=10, the cheapest size
in the set — does not necessarily falsify Decision 1 (L=10 is cheap enough that
20-40M sweeps, a 10-20x extension, is trivial: 4M sweeps took only ~5-7 min
wall-clock), but "Linsel didn't need PT so this should just work" is not
automatically true even at their smallest L. Reported to Feynman as a RESULT
entry, flagging the premise risk rather than assuming a depth extension resolves
it cleanly. Candidate next step (not decided): targeted depth extension for L=10
h in [0.318989,0.343521] to 20-40M sweeps, re-run Part B, before touching L=20/30
(where the same window would be far more expensive to push through by brute depth).
Full data: /scratch/a/A.Otaifi/paratoric_linsel_no_pt_l10_pilot_20260718/.

## Decision 2 marker test: h_c CONFIRMED as a genuine localized barrier (not distributed under-resourcing)
Ran on L=30 Lane-2 PT run (job 15140246, 67-rung ladder, min acceptance 0.24% at
h=[0.339,0.344]) per Feynman's Decision 2. **Hypothesis A (localized) confirmed,
Hypothesis B (distributed) rejected**, with a quantitative refinement over the
literal falsifiable prediction: h_c link 588/249,412 accepted (0.235%), only 4/67
replicas ever crossed it, and those that do bounce back ~99.8-99.9% of the time;
every OTHER link had >=11,400 crossings (median ~107,683) — 2-3 orders of
magnitude more permeable. Turnarounds cluster hard at the two rungs bracketing
h_c. Real, physical, localized barrier, consistent with specific-heat-like Var(E)
enhancement — not a whack-a-mole artifact. Per Decision 3's own rule
(pt_architecture.md sec 9.4), this does NOT by itself authorize building the
beta-bridge (continuous benchmark, not the self-dual MCP) — treating it as
confirmation the marker-test method itself is trustworthy, to be re-applied at
s* if the self-dual PT stalls there. Reported to Feynman as a RESULT entry.

## Feynman FINALIZED GO (UPDATE 2026-07-18c): L=10 depth-ramp to 40M sweeps — the single decisive open thread
Feynman's read on the L=10 NOT-VALIDATED finding: RIGHT, refines (does not refute)
Decision 1. Benchmark is continuous 3D Ising -> no thermodynamic barrier -> cold/hot
MUST converge at sufficient depth; z=29.6 split is NOT first-order metastability.
Leading mechanism: global_single_spin_flip (the sector-changing move) is dead, so
the winding sector may only mix by slow string accumulation — critically slow at
h_c. Pre-committed EXACT SPEC, submitted verbatim (job **15149119**, 72 tasks =
9 h-points x 4cold+4hot, output
/scratch/a/A.Otaifi/paratoric_linsel_no_pt_l10_depth_ramp_20260718/):
- DECISIVE h: 0.318989179906, 0.322, 0.326, 0.330615254621, 0.335362847648,
  0.339573841268, 0.343520981103 (the pilot's failing window).
- POSITIVE CONTROLS h: 0.305629247317, 0.347398758564 (must stay converged or the
  whole readout is void).
- 40M sweeps/chain, checkpoints at 2/4/8/16/24/32/40M (post-hoc sliced from a
  continuous --full_time_series 1 series, not live restart snapshots — confirmed
  `percolation_probability` IS in the recorded observables this time, fixing the
  pilot's gap that would have made the sector-exchange mechanism probe impossible).
PRE_SUBMISSION_CHECK PASS. Concise alert notifier attached (job 15149172) after
one manual retry (DNS transient, same class as the pilot's).
**PRE-COMMITTED BINARY DECISION RULE** (do not re-litigate): PASS (cold/hot z<3 on
BOTH ratio and base-edge-density at worst points AND decelerating increments) =>
depth-solvable, Decision 1 holds, calibrate depth law, extrapolate to L=20/30.
FAIL (still split z>>3, non-decelerating increments, and/or zero in-window
sector-exchange events while controls exchange) => winding sector not ergodic
under local updates at ALL => PT alone cannot fix it => WORM becomes MANDATORY,
reorders the whole M0 strategy — biggest finding of the campaign if so.
Pre-authorized ONE tiebreak extension to 80M sweeps only if ambiguous (decelerating
increments but z not yet <3); any other ambiguity or a drifted control => stop and
report. LOOP CONTAINMENT: one ramp (+ authorized tiebreak), then STOP — no ladder
work, no PT, no new h-points until this binary is in. Nothing else in scope
(explicitly supersedes Step 2/3 self-dual-line work until resolved).

## L=10 depth-ramp binary: PASS — Decision 1 HOLDS, no worm needed (2026-07-18)
Job 15149119 (72 tasks, EXACT SPEC) ran to full completion — 72/72 COMPLETED
(exit 0:0), independently verified via HDF5 (series length exactly 1,592,000
samples/chain = full 40M sweeps, not truncated). **VERDICT: PASS.** Both
controls stayed converged (z40<=1.03, p=0.97/0.84). All 7 decisive points:
cold/hot z<3 on BOTH ratio and density at 40M (max 1.52, most near-zero,
including AT h_c: 0.041/0.767). Increments plateau into the late checkpoints
(h=0.318989 shakiest, constant-fit p=0.054 borderline, but late-window
16-40M slope-z drops to 0.609 from overall 3.24 — drift concentrated early,
consistent with approaching plateau not runaway). Sector-exchange events:
every chain (decisive AND control) shows hundreds of thousands of exchange
events over 40M sweeps — the FAIL signature (zero in-window exchanges) is
absent, directly killing the sector-trapping hypothesis. Reading: the
pilot's z=29.6 split was ordinary depth-solvable critical slowing, not
ergodicity breaking; tau_int at h_c (10,792 sweeps) matches the pilot's
earlier estimate (~7,375 sweeps), same physical autocorrelation time, now
resolved with real Neff (1848 vs 136) thanks to the larger production
window. Reported to Feynman as a RESULT entry, with NEXT step (calibrate
depth law, scale to L=20/30 at matched depth) flagged for confirm before
committing the larger spend — not self-authorized. Aside: an earlier "they
all timed out" report from the PI traced to unrelated TIMEOUT jobs in a
different project (code_ruby_duy_pinning_lab), confirmed via sacct WorkDir
— not this run; no action needed, noted for the record only.
Full data: /scratch/a/A.Otaifi/paratoric_linsel_no_pt_l10_depth_ramp_20260718/.

## L=20 no-PT depth-ramp submitted; L=30 proposal flagged, NOT submitted (cost + unmeasured exponent)
Per PI's "1. first" (scale to L=20/30), calibrated a depth law from the L=10
checkpoint ladder: plateau entry ~16M sweeps, adopted z=2 as a **conservative
physics PRIOR, not a fit** (worker's own caveat — only ONE L point exists, an
exponent cannot actually be fit from a single depth-ramp). L=20 submitted: job
**15159599** (72 tasks, 9h x 8 chains, 64M sweeps, checkpoints 8/16/32/48/64M,
24h walltime cap = 2x margin over the 10-12h/chain estimate), running cleanly.
Notifier attached (15159644) after 3 DNS retries.
**L=30 NOT submitted — flagged for explicit confirm.** Proposed design: 144M
sweeps, projected **52-60h/chain** (~72 tasks -> thousands of core-hours if run
as proposed). This is a real resource decision, not a mechanical scale-up: the
z=2 exponent behind it is a guess, not measured. L=20's own checkpoint ladder
(once it completes, up to 24h) will give a SECOND real calibration point,
letting us actually FIT z from L=10 vs L=20 instead of assuming it — much lower
risk of wasting the L=30 spend on a wrong guess. Recommending: wait for L=20,
refit z from two real points, then design L=30 — not submitting L=30 on the
current single-point guess. Awaiting PI direction.

## L=20 readout: AMBIGUOUS, not clean PASS — L=30 correctly NOT submitted (calibration untrustworthy)
Job 15159599 (L=20, 72 tasks, 64M sweeps) completed fully (verified via HDF5 series
length 2,552,000/chain, untruncated). Readout found: at the final 64M checkpoint,
z<3 everywhere and sector-exchange healthy at all h — no catastrophic FAIL
signature. BUT most h-points already looked "converged" at the FIRST checkpoint
(8M), too coarse to show real relaxation shape, and **two points never achieve a
stable joint (z<3 AND stationary) state through 64M**: positive control
h=0.305629247317 and decisive point h=0.339573841268 keep drifting in/out.
The naive z-fit from L=10(16M)->L=20(64M) reproduces z=2 exactly — but this is
CIRCULAR: 64M was chosen by extrapolating the z=2 guess in the first place, so it
proves nothing independent. **L=30 was NOT submitted** — worker correctly declined
despite being pre-authorized, given the calibration is unreliable. L=30's real
projected cost if attempted: median 52.9h/mean 54.4h/worst 69.9h per chain,
~3,919 task-hours total (72 tasks) — a large, genuine resource commitment that
should not rest on an unverified exponent. Recommended NEXT (worker's, endorsed):
extend checkpoints for L=20 PAST 64M (targeting the two non-converged points
specifically) to get a real plateau-entry depth, THEN fit z properly, THEN design
L=30. Awaiting PI direction — this is a real cost/schedule tradeoff, not a
mechanical next step.

## Feynman GO 2026-07-20: L=10 PASS accepted, L=20 formally VOID (not ambiguous), circularity was Feynman's spec error, sector-exchange rate hints z~3.7. 3-step plan issued (~100 task-hours vs 3,919 for L=30). G6 added to run manual (geometric checkpoint ladders). STEP 1 done, STOPPED per PI instruction.
STEP 1 (free, existing data): tau_int extracted for L=10/L=20 (bootstrap/
jackknife errors), L=10/20 raw-ratio overlay plot produced
(doc/figures/linsel_raw_ratio_crossing_no_pt_l10_l20_20260720.png),
benchmark_report.md sec 4/5/6 updated, PROVISIONAL correctly NOT promoted.
**Crossing = 0.3244098 +/- 0.0000817 vs Linsel's 0.335(2) — ~5 sigma tension,
a real finding, benchmark not closed.** **tau_int(L20)/tau_int(L10) at h_c =
0.00498 +/- 0.00179 (L20 tau ~200x SMALLER than L10) — flagged SUSPECT, not
accepted as a confirmed cross-check of Feynman's sector-exchange z~3.7 hint.**
Reason for suspicion: suppression is NOT localized to h_c, it's systematic
across almost the entire L=20 window (tau~18-27 sweeps nearly flat vs h,
while L=10 spans 846-71,086 and peaks near h_c as expected) — physically
backwards (larger L should slow down, not speed up); ruled out an obvious
between_sweeps/sample-period mismatch (both=25) but did not investigate
further. Density-observable tau ratio (1.145 +/- 0.013) is far more
physically plausible and likely the more trustworthy of the two. Reported
to Feynman with this caveat prominent — NOT presenting the tau_int reversal
as confirming z~3.7. STOPPED after Step 1 per direct PI instruction; STEP 2
(L=12/14/16 short h_c-only runs, ~20-60 task-hours) and STEP 3 (targeted
L=20 void-clearing extension to 128M, geometric ladder, ~170-340 task-hours)
are designed but NOT dispatched. Also flagged: a bad tau_int estimator would
corrupt STEP 2's fit too, so this may need diagnosing before STEP 2 proceeds.

## STEP 0 done: estimator fixed (4 review rounds), tested vs Feynman's 2 predictions on real data. VERDICT: STILL REAL, with nuance.
Clark wrote blocking-plateau tau + between-chain-error estimator in
scripts/ensemble_diagnostics.py (new `blocking-validity` subcommand) per
Feynman's STEP 0 spec. Review took 4 rounds (PI-authorized extension beyond
the normal 3-round cap, with round 4 using gpt-5.6-sol + higher reasoning
per PI instruction): rounds 1-2 found real defects (wrong std convention,
plateau criterion gameable by error-bar widening — Grace gave a concrete
counterexample, aggregation bug hiding unresolved chains behind a median,
NaN stationarity, wrong bootstrap-ratio denominator, raw h5py.File instead
of the existing reader). Round 3 split (Martina REJECT on the missing
constant-fit-p gate / Grace APPROVE); round 4 added the gate explicitly,
Martina-only review (Grace dropped for this turn per PI) APPROVED.
Ran on real L=10 (15149119) + L=20 (15159599) data, tested Feynman's 2
pre-committed predictions: **BOTH FAIL as literally stated** (control's
tau=21.6 sweeps not ~44; h=0.339574's error did NOT grow >=2x, came out
~same, ratio 0.935) — **verdict per Feynman's own decision rule: STILL
REAL, step 3 stays justified.** BUT: (1) instrument caveat — the driver's
raw mean-strength error-ratio is mathematically DEGENERATE (jackknife SE
of a simple mean is algebraically identical to ddof=1 sample SE, so its
ratio to a ddof=0 version is sqrt(7/8)=0.935 for ANY data, not a real
diagnostic); real numbers used are separately-computed raw-ratio-specific
values (the nonlinear <P^4>/<P^2>^2 quantity, whose bootstrap error
genuinely depends on tau). (2) h=0.339574's z=0.42 and suffix p=0.20 both
look FINE — only tau itself remains unresolved at 64M sweeps, which may be
a narrower/cheaper fix than the full 128M step-3 rerun. (3) Secondary
tau-peak check: not cleanly established (0.326, 0.330615 also unresolved,
so can't confirm an isolated peak vs a broad slow region); L=10's
monotonic frozen-side rise IS supported but also all-unresolved.
Reported to Feynman with both nuances flagged. STOPPED per loop
containment — no 2A/2B/step-3 submission yet, awaiting Feynman's read.
Full table: doc/figures/linsel_blocking_validity_l10_l20_20260720.md.

## Feynman's 20d-20h saga resolved: L=20 void CLEARED (tau estimator was fine after all, nuanced), doc fixes done, crossing re-derived (2026-07-21)
Caught up on an extensive Feynman self-correction chain (20d->20h) that happened
between sessions: a separate Jarvis session already built a blocking-plateau tau
estimator (scripts/ensemble_diagnostics.py, 4 review rounds) and ran STEP 1 (crossing
= 0.3244098, tau_int ratio flagged suspect), then stopped per PI instruction.
Feynman then went 20d ("void is an analysis artifact") -> 20f ("no, refuted myself,
void is real") -> 20g (final: void genuinely CLEARED, but for a different reason —
error bars were fine all along per a tau-free between-chain cross-check; the
STATIONARITY test was what used the bad IPS tau, and with corrected blocking-plateau
tau both disputed L=20 points actually PASS). Net: **STEP 3 (128M rerun) CANCELLED**
— tau at those points stays unresolved but that doesn't block anything downstream.
tau_int confirmed permanently WRONG INSTRUMENT for a dynamic exponent (conflates
falling self-averaged amplitude with rising decay time) — STEP 4 (z-fit from
tau_int(L)) retired as invalid, not just optional.
Executed the ratified NEXT actions (20h loop containment):
1. **Doc fixes**: run_manual.md F3/F7, pt_architecture.md H1/H2 + new correction-
   scope/retrospective, harness.md G1.2 — round-trips now correctly "neither
   necessary nor sufficient," continuous-vs-first-order distinction + thesis-T1
   caveat added, retrospective on the 4 NO-GO calls using the wrong metric (without
   walking back the marker test's real 0.235% h_c barrier). Clark wrote, Martina
   APPROVED (Grace timed out, proceeded on Martina alone — low-risk wording fix).
2. **Crossing re-derivation**: restored both former-void L=20 points. Feynman's
   pre-committed prediction FAILED — crossing stayed at 0.3244098 identically
   with/without the 2 points (bracketed by h=0.322/0.326, nowhere near them). BUT
   honest interpolation-systematic scan (18 method/window choices) gives a full
   spread of 0.0112 (13x the naive bootstrap bar) that DOES reach Linsel's 0.335(2)
   band — so neither "confirmed no tension" nor "confirmed real tension" landed;
   reads as "consistent within honest systematics," dominated by L=10's nonmonotonic
   curve making interpolation choice underdetermined. Also caught: the previously
   quoted "+/-0.0000817" crossing error was a decimal transcription error (real
   value +/-0.000817, still not the total uncertainty either way) — propagated
   across several of my own earlier status.md entries, now corrected.
benchmark_report.md sec 4-6 updated, correctly kept PROVISIONAL. New figure:
doc/figures/linsel_raw_ratio_crossing_no_pt_l10_l20_restored_20260721.png.
Reported to Feynman. Proceeding to 2A (locate s*) + 2B (PT speedup) in parallel
per loop containment. L=30/L=24 still HOLD throughout.

## 2A + 2B readouts: 2A INCONCLUSIVE-as-expected, s* provisionally [0.325,0.350]; 2B INCONCLUSIVE from a real design gap (baseline/rung mismatch + unresolved tau), NOT a null result on PT (2026-07-21)
Both jobs completed (2A: 15177203, 528/528; 2B: 15178251, 1h27m). Process lesson:
original James/Jones dispatches bundled submit+wait+analyze into one call each,
exceeded the 30-min MCP idle timeout, orphaned two background codex processes
(confirmed still running server-side via `ps`, real work, but their reports were
lost) — re-dispatched clean readout-only calls once both Slurm jobs were
independently confirmed COMPLETED via sacct. Split submit/readout into separate
dispatches going forward.
**2A (locate s*)**: duality 200/201 PASS (1 FLAG at L=8,s=0.275@200k sweeps,
resolves by 400k — thermalization blip, not a mapping bug). Energy-gap: 14
trajectories SHRINKING (ordinary), ZERO stable-separated (no coexistence
signature). ALL 528 histograms unimodal — per the asymmetry rule this is
INCONCLUSIVE on order, explicitly NOT evidence of continuity (exactly the
anticipated, acceptable L=8-16 outcome). s* bracket: PRIMARY method found no
usable 3-size crossing; SECONDARY (interval-window) method gives PROVISIONAL
s* in [0.325,0.350], consistent with Linsel's prior but resting on the weaker
method; 13 pairs NOT_GATED. Recommend denser [0.325,0.350] sampling next.
**2B (PT speedup)**: pre-check PASSED (fixed-h reconstruction from rank-local+
ladder-index series verified valid against the C++ schema — methodologically
sound). But: the Lane-2 14-rung PT ladder and the 9-point no-PT L=10 baseline
(job 15149119) use DIFFERENT h-grids — only 2/14 rungs land close enough to
compare at all, and BOTH those points are tau-UNRESOLVED (blocking-plateau
lower bounds) on both sides, so the computed "144x"/"64x" quotients are
explicitly NOT valid speedup bounds (worker correctly flagged this, did not
spin it as evidence PT is fast). Equal-core-hour Neff/core-hour comparison
also indeterminate for the same reason (both sides censored). New genuine
flag: the PT-derived finite-L crossing (0.336613) sits 0.0017 OUTSIDE the
no-PT crossing envelope (0.3237-0.3349) — small but real, needs investigation
before trusting PT at the MCP regardless of speed. Excursion stats: mean
range only 1.33 rungs (median 1), only 21.4% reach >=2 rungs — limited
transport, consistent with the marker test's localized-barrier finding.
Harmonic/median acceptance gap persists (0.099 vs 0.237). Swap-period: only
swap_period=2 ran (1/5/10 correctly skipped, ~4x cost); single point not a
curve. **DECISION RULE NOT TRIGGERED** — both taus censored, no honest
speedup factor to quote either way; recorded as UNRESOLVED, not a negative
result on PT. Recommend (not deciding): retry needs a matched h-grid between
PT ladder and no-PT baseline, enough depth to actually resolve tau at the
rungs that matter, and investigation of the crossing shift before more spend.
Reported to Feynman. STOPPING per loop containment — no L=30, no L=24, no PT
ladder work until Feynman's read on this lands.
Full data: 2A /scratch/a/A.Otaifi/paratoric_selfdual_step2a_20260721/ (job
15177203); 2B /scratch/a/A.Otaifi/paratoric_pt_speedup_l10_20260721/ (job
15178251).

## PI-ratified PHASE 1 (20m) executed: crossing genuinely DISAPPEARS in clean window; 2A audit revealed a code defect (fixed+reviewed) AND an unresolved statistical-power-vs-contamination question (2026-07-21)
Per PI-ratified UPDATE 2026-07-20m: L=10 no-PT baseline found NOT ERGODIC at low
h (per-chain spread up to 0.4433, I4 factor-2 violated by 3-4 orders of magnitude
at 8/9 points) — invalidates the earlier "0.3244098, consistent within
systematics" read. L=20 is clean everywhere (self-averaging, factor ~153/doubling)
— small-L-specific disease, M2 program at L=120-192 NOT threatened.
**1A (clean-window crossing)**: re-derived using only h where both sizes pass the
spread gate. **No crossing exists at any threshold** (0.10/0.05/0.03) — not
narrower, not contested, genuinely undefined (no sign change, no bracketing
root). Old 0.3244098 statement fully superseded. benchmark_report.md updated,
stays PROVISIONAL, states why. Figure
doc/figures/l10_chain_disagreement_and_pt_repair_20260721.png in place.
**1B (2A per-chain audit)**: L10/L20 already covered by Feynman's own 20i/20j
numbers. NEW 2A work (job 15177203) hit a real process gap — an analysis
dispatch wrote ~1967 lines directly into scripts/ensemble_diagnostics.py without
Clark/review. Sent to Martina: REJECTED (`--window-mode fixed` was a no-op,
spread gate computed but never actually thresholded into PASS/FLAG). Clark
fixed both + added a real legacy-TSV SHA-256 regression assertion; Martina
re-reviewed and APPROVED. Grace unavailable both rounds (timeout, then HTTP
504) — proceeded on Martina alone. **Re-running with the FIXED code changed
the 2A verdict substantially**: original "clean s<=0.300 (30/30), contaminated
s>=0.325" had been read off the WRONG window (`final_interval`, 32k samples)
despite `--window-mode fixed` being requested; under the properly-wired
`final_fixed` (400-sample) window + real spread gate, 29/30 previously-clean
rows now FAIL (26 on spread alone). s>=0.325 unchanged (36/36 fail either way).
**NOT resolving myself**: is this genuine widespread contamination across
nearly the whole 2A scan, or a statistical-power artifact of the 400-sample
window being too short to trust (structurally the same class of mistake as
Feynman's own 20d-20f "too few blocks reads noise as unresolved" episode)?
Flagged prominently to Feynman rather than either alarming-number-as-truth or
quietly keeping the incomplete original claim.
**1C**: code fixed, reviewed, APPROVED. Between-chain-error column + run_manual
per-chain-table doc addition NOT yet done — left as open Phase 1C scope.
**1D**: done via 1A's dispatch.
Reported to Feynman. STOPPING per loop containment (20m: "do not run Phases 2/3
before Phase 1 lands") — the 2A ambiguity directly gates Phase 3's scope (how
broad the L=8-16 contamination is), and Phase 2 (PT-at-L=10) is now motivated
by two independent findings (20i ergodicity-repair + 1A's "no crossing exists
without it") rather than one. Awaiting Feynman's read.

## 20p executed: 2A ambiguity resolved (statistical noise at s<=0.300, dead instrument at s>=0.350), s* bracket confirmed ~0.332, two new gates wired+reviewed, report-file ordering fixed (2026-07-23)
Feynman's 20p answered the 2A open question I refused to decide: s<=0.300 was
confirmed genuine 1/sqrt(window) NOISE (not basin contamination) — my original
"clean" report was correct, the 29/30 fail was a false alarm from applying a
spread gate to a statistically-powerless 400-sample window. s>=0.350 is a
THIRD thing: the raw-ratio instrument itself dies there (>50% exact zeros in
percolation_strength, this project's own documented-but-never-wired
intermittency tripwire) — the vanishing IS the transition (s*~0.32-0.36).
**Housekeeping**: jarvis_report_tofeynman.md was out of chronological order
(a 07-19/07-20 block had landed below newer 07-21 entries) — archived
pre-07-18 content to doc/archive/jarvis_report_tofeynman_archive_pre_20260718.md
(Feynman's own naming convention), restored strict order, added
NEWEST-AT-BOTTOM marker, verified zero content loss programmatically before
writing.
**4(d)**: s* re-derived from percolation_probability (bounded, no vanishing-
moment division) + largest_cluster sanity check on existing 2A data (free).
Finite-L percolation_probability crossings cluster tightly: z-basis
0.330968/0.331043/0.331141, x-basis 0.338041/0.331170/0.328490 → **s*~0.332**.
Bracket [0.325,0.350] numerically unchanged from the broken-ratio estimate
but now instrumentally defensible, confirmed no divide-by-zero at s>=0.350
with these instruments.
**4(a)**: superseded by a proper 3-way partition (see 4b/4c) rather than a
binary restore: 26 PASS, 10 genuine FAIL, 30 OBSERVABLE-DEGENERATE (of 66
total L,s,basis points).
**4(b)+4(c)**: two new gates in scripts/ensemble_diagnostics.py — spread-
window-scaling check (NOISE vs REAL vs SPREAD-UNRESOLVED) and
OBSERVABLE-DEGENERATE tripwire (>50% exact zeros, ratio numbers suppressed to
NA at both summary and per-chain granularity). 2-round review: Martina
REJECTED round 1 (degenerate per-chain rows still said "OK" with real-
looking numbers), Clark fixed to NA, Martina APPROVED round 2. Grace
unavailable both times (timeout, HTTP 504) — proceeded on Martina alone,
narrow/concrete fixes. Legacy-TSV SHA-256 self-test PASS throughout, no
regression. One boundary case correctly NOT swept into DEGENERATE: L=8,
s=0.350, x-basis (zero_fraction=0.4952, just under threshold) stays a
genuine FAIL — flagged as possibly a real, non-instrumental issue, not yet
investigated.
**Standing debt**: between-chain-error column + run_manual per-chain-table
doc addition (Phase 1C) and the invariant-provenance audit (20n item 3) not
yet done.
Reported to Feynman. Phase 1 (20m) is now fully complete per its own loop
containment. Ready for Phase 2 (PT-at-L=10, redesigned ladder) on go.

## Operational: codex-worker bridge model drift (2026-07-18)
Bridge backend's Codex CLI account no longer supports `gpt-5.3-codex` (ParaToric/
AGENTS.md's stated "Model routing policy" default) OR `gpt-5.6` (a name that does
not exist) — both hard-reject with "model not supported when using Codex with a
ChatGPT account." Root cause of the FIRST dispatch failure this session: should
have routed as **James** (run staging/analysis persona, ai-workflow/JARVIS.md:34-36,
model=gpt-5.6-sol, reasoning=medium) rather than falling back to the tool's bare
default. Verified working this session: `gpt-5.4` (ad hoc, pre-persona-check) and
`gpt-5.6-sol` (James's actual contracted model, confirmed via codex_inspect
sanity check). PI confirmed the full persona table is current and matches
JARVIS.md (James/Jones=gpt-5.6-sol, Clark=gpt-5.6-terra, Martina=gpt-5.4-mini,
Grace=z-ai/glm-5.2). ParaToric/AGENTS.md's "Model routing policy" section
(lightweight-task default, GPT-5.3-codex) is stale/broken on this backend and
was flagged to the PI rather than silently edited — needs a fix to a working
lightweight model id before it's relied on again for mechanical tasks.
without explicit direction.

## L=10 deep NO-PT benchmark pilot: SUBMITTED with checkpointed cold/hot ramps
Per the 2026-07-18 PI/Feynman decision to DECOUPLE the exact-size Linsel
overlay from PT, staged and submitted a one-node no-PT L=10 pilot under
`/scratch/a/A.Otaifi/paratoric_linsel_no_pt_l10_pilot_20260718/` as Slurm
array job `15141215`. Grid = 14 h points spanning `0.280000000000` to
`0.400000000000`, with the dense core borrowed from the old targeted L20
h_c ladder (`0.305629247317, 0.312438864933, 0.318989179906,
0.325131841384, 0.330615254621, 0.335362847648, 0.339573841268,
0.343520981103, 0.347398758564`). Each h gets 8 independent chains
(4 cold `default_spin=+1`, 4 hot `default_spin=-1`), pure local
Metropolis, beta=L=10, mu=J=1, lmbda=0.3, z basis, Boost square periodic,
observables `plaquette_percolation_probability`,
`plaquette_percolation_strength`, `largest_cluster`, `sigma_z`. Thermalization
is judged via continuous checkpoint windows at 200k/500k/1M/2M sweeps; each
chain then continues to 4M total sweeps so the 2M->4M tail is available as
the post-thermalization production window if the deepest checkpoint is needed.
Pre-submission verification PASS in
`/scratch/a/A.Otaifi/paratoric_linsel_no_pt_l10_pilot_20260718/PRE_SUBMISSION_CHECK.md`:
`bash -n` PASS, rendered design-vs-intended diff empty PASS, full 112-task map
PASS, scheduler `--test-only` PASS. Immediate queue confirmation before a
transient controller-DNS outage showed tasks `0-28` RUNNING and `29-111`
PENDING on `QOSMaxJobsPerUserLimit`. The concise alert wrapper hit the same
DNS issue and did not create a notifier job; main array `15141215` itself was
already accepted.

## Phase 2 SUBMITTED: redesigned PT-at-L=10 ladder, job 15213796 (2026-07-23)
PI said "go" on Phase 2 (feynman_thesis_tojarvis.md ~1535-1551). Dispatched James
(workspace-write, gpt-5.6-sol, reasoning=medium, FRESH) to design+submit ONLY
(readout is a separate later dispatch, per process rule). Job `15213796` RUNNING
(single sacct check, node th-cl-hua22) at
`/scratch/a/A.Otaifi/paratoric_pt_l10_phase2_redesigned_ladder_20260723/`.
Reused registry: ran `pt_diagnostics.py` for real per-link acceptance off the
old Lane-2 job 15178251's HDF5 (median 0.237420, harmonic 0.098802 — matches the
already-reported gap); extended `adaptive_ladder_pm.py` with a new `--target-count`
flag (algorithmic PM resampling to fixed N=14, not hand-spaced) and reused
`submit_pt_speedup_l10.sbatch` as the harness template. New 14 h-values, all in
[0.2925,0.42] as specified (dropped the 5 sub-0.28 rungs + 2 super-0.42 rungs):
0.2925, 0.313004, 0.333509, 0.347632, 0.360480, 0.369418, 0.376402, 0.382348,
0.387257, 0.392587, 0.398761, 0.405143, 0.412572, 0.42.
Config: 4 swap-period variants (2,1,5,10), each 40M thermalization + 8M
production = 48M sweeps/rung, 14 MPI ranks/variant, 1 node, 4G, 18:00:00.
PRE_SUBMISSION_CHECK PASS (bash -n, self-tests, 14-rung range/uniqueness,
task map, cold/hot count, exe/MPI linkage, design-vs-reference diff,
`sbatch --test-only`). One prior attempt (15213777) failed pre-simulation on a
ladder-quoting bug, caught and corrected before this submission.
CAVEATS: (1) PT checkpoint/restart still unimplemented (throws) — continuous
blocks used instead, per the known M0 gap. (2) Concise-alert notifier failed,
same controller-DNS issue as 2026-07-18 — no notifier job exists for 15213796,
must check via sacct manually. (3) Self-review flag: this dispatch ran 48 Codex
tool-calls in one session, over JARVIS.md's 25-call FRESH-split threshold —
logged in `ai-workflow/jarvis_selfreview.md`; work product looks sound but the
dispatch should have been split (e.g. ladder-design-only, then submit-only).
NEXT: readout as a separate FRESH dispatch once 15213796 completes — gate is
per-rung agreement + init-memory -> 0, NOT round-trips.

## Phase 2 job 15213796 FAILED (OOM); root-caused, resubmitted as 15213796->15215269 RUNNING (2026-07-23)
`sacct` showed 15213796 FAILED, step `.0` OOM-killed after 2h31m, `MaxRSS=302160K`
(~295MB/rank, still climbing) against `--mem=4G`/14 ranks. First fix dispatch
(James) computed a THEORETICAL worst case from `extended_toric_code_qmc.hpp:
3644-3654` (`acc_ratio_vector`/`observable_vector` both `.reserve(thermalization_
count)`, thermalization_count = 40M sweeps x 200 moves/sweep = 8e9, 7 observables
x sizeof(variant)=24B) and got ~25TB required — WRONG, verdict STOPPED, no
resubmission. Refuted empirically (checked myself via `sacct` on the reference job
15178251: same code path, 16M-sweep thermalization, `--mem=8G`, COMPLETED with
`MaxRSS=485100K`, ~474MB/rank) — `reserve()` capacity is virtual, not committed
RAM (standard Linux overcommit/demand-paging for large allocations); real RSS
tracks actual touched pages, not nominal reserve size. Second, narrow James
dispatch (told explicitly not to re-derive the sizeof math) set `--mem=96G`
(empirically-grounded, generous margin over the ~20-30GB linear-growth
extrapolation from the 295MB/2h31m data point) and resubmitted as job `15215269`.
`sbatch --test-only`, `sinfo`, and the dispatch's own `sacct` check all failed on
a transient controller-DNS outage (same recurring issue as 2026-07-18); I verified
directly afterward (once DNS recovered) — `15215269` RUNNING, 96G, 14 tasks,
confirmed via my own `sacct` call. No source files touched (`git diff --stat --
src/` empty, verified both dispatches). Concise notifier failed (same DNS issue),
no notifier job exists for either 15213796 or 15215269 — check manually.
**Self-review**: 3rd flag this session — the first fix dispatch's mistaken
25TB/"impossible" verdict is a CONTRACT-GAP (codex workers reasoning from
allocation-size theory instead of checking empirical `sacct` MaxRSS data that was
sitting right there from a near-identical completed job) — logged in
`ai-workflow/jarvis_selfreview.md`.
NEXT: readout (unchanged) once 15215269 completes or reaches a checkpoint;
consider whether 96G is enough for all 4 swap-period variants or only variant 1
(swap_period=2) — worth a memory sanity check at the next natural checkpoint
rather than waiting for a second OOM.

## Job 15215269 CONFIRMED COMPLETED (8h13m, exit 0:0, 96G sufficient); readout NOT YET OBTAINED — Codex bridge flaky (2026-07-24)
Verified via `sacct`: all 4 swap-period steps (.0-.3) COMPLETED, peak MaxRSS
1,935,808K (~1.9GB/rank, step .1/sp1) — 96G was ample, no second OOM. 56 result
dirs present (`results/pt_L10_phase2_job15215269_sp{1,2,5,10}_rank{0..13}_
init{1,-1}/`); 14 leftover PARTIAL dirs from the OOM'd `job15213796_sp2_rank*`
attempt still sit alongside them in the same `results/` folder and must be
excluded from any readout.
**Readout dispatch (Jones, read-only, reasoning=high) attempted 3x, none
returned**: attempt 1 hung 1800s (bridge reconnect churn, 3 concurrent
`mcp_server.py` processes observed via `ps`, unrelated jupyter MCP server also
dropped/reconnected around the same time); attempt 2 hung 1800s again; a cheap
PING probe then surfaced the real cause progressively: first
`ERROR: You've hit your usage limit... try again at Jul 29th, 2026 10:34 AM`
(Codex quota), then on retry `Error: Your access token could not be refreshed
because your refresh token was revoked` (auth), then a PING succeeded cleanly
moments later, but the SAME full readout dispatch (attempt 3, identical prompt)
still hung another 1800s. Root cause of the hang itself remains unconfirmed —
possibly the large multi-script analysis + reasoning=high combination is
genuinely slow/unstable on this bridge right now, separate from the quota/auth
blips already surfaced. No result obtained from any of the 3 attempts; nothing
lost/damaged (all read-only, job data untouched).
NEXT: retry the readout dispatch (same prompt is staged above in this file's
history / repeat verbatim); consider first re-running the cheap PING probe to
confirm the bridge is healthy before re-attempting the full analysis, and/or
splitting the 4 deliverables into 2 smaller dispatches if a full retry hangs
again a 4th time.

## JARVIS.md self-edit: new "split by scope" rule (2026-07-24)
PI asked for a direct diagnosis of the token/quota burn this session (two
separate Codex usage-limit hits: a 5-day reset then a same-day 2:59 PM reset).
Root cause of the larger one: the Phase 2 design+submit dispatch bundled
design + registry-script extension + submit + a mid-task debug-and-retry (job
15213777's ladder-quoting bug) into ONE FRESH session — 48 calls, ~5.4M tokens
(already logged as a guardrail-slip, see above). PI's instruction: submission
doesn't need design's context, so these should have been split into separate
dispatches to save tokens. Added a new rule to `ai-workflow/JARVIS.md` (after
the existing 25-call hard-split rule, in "Every dispatch must declare"):
**"Split by scope, not just by call-count"** — before bundling adjacent steps,
check whether the later step needs the earlier step's full reasoning/context or
only its OUTPUT; if only the output, split into separate FRESH dispatches and
pass the output forward as a pointer/value, not inherited session context. Cites
this exact incident as the motivating case.
**Self-edit justification (per JARVIS.md's own rule)**: edited directly, no
Clark/Martina review — Codex is quota-exhausted right now (both the 5-day and
the same-day limit are active), so routing through a Codex-based reviewer isn't
possible until the quota resets. This is a small, self-contained prose addition
to Jarvis's own process contract, not a scientific or code change.

## Phase 2 READOUT COMPLETE: GATE=FAIL, MIXED by rung. Reported to Feynman (2026-07-24)
Full readout obtained (partly via direct manual script execution during the
Codex outage, partly via Jones once the bridge/credentials recovered — see
below). **Deliverable (iv)** operational calibration: acceptance flat across
sp1/2/5/10 (median ~0.645, harmonic ~0.484-0.486) — physically expected,
swap acceptance depends on config not attempt frequency; sp1 best round-trips
(137) but that alone doesn't determine the gate.
**Deliverable (i) GATE, the project's stated criterion**: per-rung
agreement + init-memory (cold/init+1 vs hot/init-1 origin), computed by Jones
via `ensemble_diagnostics.py`'s PT reconstruction, `/tmp/phase2_init_memory_
job15215269.tsv` (56 rows) + `_summary.tsv`. **FAIL overall, MIXED by rung**:
all 40 tau-RESOLVED high-h cases (rung>=4) PASS z<3; all 16 low-h cases
(rung 0-3, h=0.2925-0.347632) are TAU-UNRESOLVED, 12/16 retain >=3sigma init
memory (e.g. sp1 rung0 z=5.42, rung2 z=7.24) with NO monotonic drive to zero
across sp1->sp10 (low-h fail counts 4/4, 3/4, 3/4, 2/4). Per the ratified
plan's own decision rule ("IF PT CANNOT drive the low-h init memory to zero,
report that plainly"): this is that outcome.
**Deliverable (ii)+(iii)**: only matched_rungs=2/14 land on the no-PT baseline
h-grid (job 15149119) — same grid-mismatch as the original 2B attempt, NOT
fixed by this ladder redesign. Both matched points
INDETERMINATE_BOTH_TAU_LOWER_BOUNDS — no valid speedup number exists.
sp2/sp5 each resolve a crossing (~0.3363, near Linsel 0.335(2)) but sp1 finds
0 brackets and sp10 finds 5 — Jones's read (accepted): sp2/sp5 agreement is
NOT independent confirmation, both built from the same 2 indeterminate-tau
points; do not report this as evidence.
**RECOMMENDATION per the standing decision rule**: this rejects the TESTED
ladder+depth combination, NOT PT as a method and NOT the beta-bridge. Likely
fix is more low-h depth (not swap-period tuning — all 4 tested values show
the same qualitative failure) and/or a ladder that actually lands on the
no-PT h-grid. Not deciding the next step myself — real fork for the PI/Feynman.
**Process history this readout**: Codex hit a real 5-day quota exhaustion,
then a revoked-auth-token error, then (on new credentials) a different account
that also had exhausted usage and didn't support the persona models at all —
all logged in `jarvis_selfreview.md`. During the outage I ran the already-
registered, already-reviewed analysis scripts directly via Bash myself
(`pt_diagnostics.py`, `ensemble_diagnostics.py pt-speedup`), using
`--resamples 50` instead of default 1000 to fit a ~10-minute practical
execution ceiling discovered this session (longer background runs were
silently killed and FALSELY reported as "completed exit 0" by this session's
task-notification system — confirmed via direct `REAL_EXIT=143` in redirected
logs; treat any future background-task "completed" claim with independent
verification until this is understood). Once the bridge/original account
recovered, Jones (read-only, reasoning=high) independently reviewed this
manual work, computed the missing init-memory deliverable, and confirmed the
resamples=50 reduction did not change any RESOLVED/UNRESOLVED call or
crossing bracket.
Full evidence: `/tmp/phase2_init_memory_job15215269*.tsv`,
`/tmp/pt_speedup_sp{1,2,5,10}_{table,physics}.tsv`,
`/tmp/phase2_partA_crossing_roots.tsv`, `/tmp/sp{1,2,5,10}_{motion,flow,
excursions}.tsv`. Reported in full to Feynman (`jarvis_report_tofeynman.md`).
NEXT: awaiting PI/Feynman decision on the fork above. L=30/L=24 stay HOLD.

## Feynman OVERTURNED Phase 2 gate to PASS (20t-20w); M1 done, M2 (Phase 2B-redo) next (2026-07-25)
Feynman re-derived from job 15215269 and overturned: gate is **PASS** on replica-
level errors (14 replicas as statistical units, no tau/block model) — z=0.04-0.82
at the low-h rungs vs my/Jones's block-error z=5.4-7.2 (block errors ~10x too
small when replicas share a slow mode; corroborated by cold-hot gap SIGN being
random across the 4 swap-period runs: +,+,-,-). Traversal is a genuine milestone:
14/14 replicas visited all 14 rungs, every swap period. Crossing/speedup stayed
unresolved because of Feynman's OWN ladder-span error, not PT or depth: 12/14
rungs (86%) sat outside the benchmark window [0.305,0.350]. Swap-period "optimum"
(20u, sp=5) was ALSO withdrawn (20v): tau(energy)~5400 sweeps is independent of
swap period; all 4 tested values (1,2,5,10) sit in the same over-attempting
regime — sp should be ~50-200 sweeps instead, one value, not a 4-way scan.
CONSOLIDATED GO = UPDATE 2026-07-20w, supersedes 20t/20u/20v specs; this is the
plan of record now. Full text: `doc/feynman_thesis_tojarvis.md` lines 2308-2406.
**Standing rules added**: replica-level errors now the DEFAULT gate basis
(`doc/paratoric_run_manual.md` new G7); background-task false-"completed" bug
documented as a harness issue with a sentinel-based mitigation
(`ai-workflow/JARVIS.md` Cost rules section).
**M1 (free, done)**: James — Task A benchmark extrapolation:
h_c(L->inf) = 0.323577 +/- 0.004243 from the 3 finite-L PT crossings
(20/24=0.34373, 20/28=0.34155, 24/28=0.34059), figure
`doc/figures/linsel_crossing_extrapolation_pt_20260725.png`, added to
`benchmark_report.md`. **Lies below Linsel's 0.335(2) band** (~2.4sigma tension,
one-dof fit, provisional PT-input crossings, caveated in the figure caption per
spec — does NOT close the benchmark). Task B energy-Binder s* cross-check on
existing 2A data (job 15177203): **NEGATIVE** — no energy-Binder crossing found,
no independent confirmation of s*=0.333+/-0.004. Both reported plainly, neither
softened.
NEXT: M2 (Phase 2B-redo, the one new run) — re-span ladder to cover
h=[0.305,0.350] with >=8 rungs including exact no-PT h-values, keep 3-4 reservoir
rungs above 0.36, ONE swap period ~50-200 sweeps (not a scan), same depth as
15215269, NEW mandatory pre-submission window-coverage check. M4(1)
invariant-provenance audit still needs scoping.

## M4(2) DONE: replica-level error wired as default gate basis, Martina-reviewed APPROVE (2026-07-25)
Clark added `pt-replica-gate` mode to `ensemble_diagnostics.py` (new code
`:1495-1591` grouping+SE+quadrature+z logic, `:3143-3157` CLI subcommand),
extending the EXISTING `between_chain_error`/`independent_chain_mean_error`
machinery already used by `run_cold_hot` for no-PT chains — same statistical
pattern, generalized to PT replicas grouped by ORIGIN rank init. Self-test
reproduced Feynman's hand-derived 20t numbers almost exactly: sp1 z=0.8236
(Feynman 0.82), sp2 z=0.2410 (0.24), sp5 z=0.4336 (0.43), sp10 z=0.0447 (0.04).
`pt_diagnostics.py` has no group-comparison gate path to extend (trajectory-only
diagnostics) — Clark reported this rather than force-fitting, correct per scope.
Registry extended, legacy-TSV SHA-256 regression PASS, no self-approval.
**Review**: Martina APPROVE with concrete file:line evidence (confirmed
origin-init grouping via `init(?P<spin>-?1)` regex not current occupancy,
quadrature via `math.hypot`, explicit `group_gate_basis=REPLICA_LEVEL` vs
`within_replica_block_errors=NON_GATE_BASIS` labeling — spec's "block errors
alongside, never sole gate basis" requirement is met). One flagged caveat:
origin-init parsed from `rank*_init+-1` directory naming, not HDF5 metadata —
a portability note, not a correctness defect on this dataset. Grace/OpenRouter
REJECTED, but on inspection every finding was "UNEVIDENCED" — her read-access
tool failed on "absolute paths are not permitted" before reading any real
content (likely my path-format mistake in the dispatch prompt, not a real code
issue) — treating this as Grace-unavailable (tooling failure), not a
substantive objection, consistent with her documented unreliability this
session; proceeding on Martina's concrete, verified review alone.

## M2 (Phase 2B-redo) DESIGNED + SUBMITTED: job 15232180 RUNNING (2026-07-25)
Design (James, FRESH, workspace-write): 14-rung ladder at
`/scratch/a/A.Otaifi/paratoric_pt_l10_phase2b_redo_20260725/` — 9 exact no-PT
baseline h-matches (job 15149119's grid) + 1 hand-placed transition rung
(0.358408493726, midpoint of the seam) + 4 reservoir rungs reused from
15215269's high-h end (0.369418,0.387257,0.405143,0.420000). NEW mandatory
pre-submission window-coverage check (`check_measurement_window`, extends the
existing inline verification pattern): PASS on new ladder (inside=9,
outside=5), correctly FAILS as negative control on the OLD 15215269 ladder
(inside=3, outside=11, exit 1, explicit ERROR) — the two-line check Feynman
said would have caught the original design error now exists and works.
Swap period: ONE value, 100 sweeps (central in the specified 50-200 range,
per 20v's tau(energy)~5400-sweep analysis — no scan this time). Depth
unchanged from 15215269 (40M therm + 8M production/rung). --mem=96G reused
(same per-rank depth/rank count, single swap-period variant only REDUCES
memory pressure vs the 4-variant 15215269). Design NOT self-submitted —
reviewed by me before authorizing submission.
Submit-only dispatch hit the same transient Slurm-controller-DNS issue seen
before (2026-07-18, 2026-07-23) on `sbatch --test-only` — correctly STOPPED
without submitting rather than force it. DNS cleared minutes later (confirmed
via my own `sacct` call); since the design was already fully verified and
this was a purely mechanical retry, I ran `sbatch --test-only` then
`sbatch` myself directly (no worker dispatch needed for this step, per
JARVIS.md's cheap-mechanical-step allowance). **Job 15232180 RUNNING**
(1 node, 14 tasks), concise-alert notifier `15232181` submitted cleanly.
Projected runtime ~2h (roughly 1/4 of 15215269's 8h13m, since this is one
swap-period variant not four).
NEXT: readout once 15232180 completes — traversal stats, replica-level
cold/hot gap via the new `pt-replica-gate` mode, tau at the 9 matched h-values
vs no-PT baseline 15149119 (with the concatenation-bias caveat per 20u sec 4),
Neff/core-hour, and tau(energy) for this new ladder (re-check the swap-period
criterion, do not re-scan). Gate: traversal complete AND tau_config AND
replica-level z<3, all three legs reported together, never traversal alone.

## M2 READOUT COMPLETE: GATE=PASS (all 3 legs), crossing NOW COMPUTABLE but no
## error bar + PT-tau artifact suspected. Reported in full to Feynman (2026-07-25)
Job 15232180 confirmed COMPLETED (1h27m, exit 0:0) via my own sacct check before
trusting the readout dispatch. Jones (read-only, reasoning=high, 31 calls —
over the 25-call split threshold, self-review-worthy but the task genuinely
needed 4 registry-script invocations across a 14-rung dataset; noting rather
than re-splitting after the fact).
**GATE PASS**: (1) traversal complete, 14/14 replicas both endpoints, 130
strict round trips — re-span did NOT degrade transport. (2) tau(energy)
finite: IPS 8325+/-893, blocking-lower 6113+/-324 sweeps; sp=100 confirmed
61-83x below tau(energy), ~10x above the ~10-sweep repetition floor — safety
criterion holds as a by-product check, no re-scan. (3) replica-level max
z=2.88 at rung10 (h=0.369418, a RESERVOIR rung OUTSIDE the benchmark window)
— inside the window that matters (rungs 0-8) max z=1.68, comfortable PASS.
Full table: /tmp/phase2b_redo_jones_15232180/replica_gate_raw_ratio.tsv.
**CROSSING NOW EXISTS**: h*=0.336519, bracket [0.335363,0.339574], close to
Linsel 0.335(2) — first computable L10-vs-L20 crossing this campaign has had.
TWO CAVEATS, not yet a closure: (a) uncertainty=NOT_ESTIMATED, no error bar
propagated; (b) PT tau at the 9 matched h-values is suspiciously CONSTANT
(50-52 sweeps at every TAU-RESOLVED point regardless of h, vs no-PT's
1184-13732 sweep range over the same h) — looks like the concatenation-of-
disjoint-visits artifact (20u/20v), not real physics; NOT resolved, flagged
not trusted-or-dismissed. Consequence: the 4 LOWER_BOUND "speedup" values
(254x/274x/128x/70x) rest on that same suspect tau — per Jones's recommendation
(accepted), NOT folding this into M2 as a demonstrated speedup lever;
cost-normalized bounds near the crossing are only 1.5-6.6x and likely
optimistic-biased. Plain-h-ladder benefit stays UNPROVEN (not disproven).
**SEPARATE FLAG**: PT vs no-PT-L10 raw_ratio disagree at 6/9 matched points
(z>2), 2/9 exceed z=3 (0.326: z=3.99; 0.343521: z=4.91) — but the no-PT L=10
side is the SAME ensemble Phase 1 found non-ergodic; its errors are plausibly
too small for the same block-vs-replica reason M4(2) just fixed for PT — no
replica-level correction applied to the no-PT side yet, flagged as the likely
next diagnostic.
Full evidence: /tmp/phase2b_redo_jones_15232180/*.tsv (traversal, replica gate,
matched-h speedup, Neff/core-hour, replica-following taus, crossing summary).
Reported in full to Feynman (jarvis_report_tofeynman.md), stopping per 20w's
own loop containment ("stop and report before M3"). M3 (denser s* grid) and
M4(1) (invariant-provenance audit) both remain queued, neither started.

## CORRECTION (Feynman 20aa): artifact suspicion CONFIRMED with mechanism; real
## PT speedup = 1.19x median, 1.21x at h_c. STRIKE the 254x/274x/128x/70x numbers
## above — they were never real (2026-07-25)
Mechanism: swap_period=100 sweeps = the sample interval, so PT tau~50-52 sweeps
was 0.51 SAMPLES — exactly tau_int's white-noise floor (0.5 + sum rho(t), rho
all ~0 => 0.5 identically). Mean dwell per rung visit was 1.15-1.89 SAMPLES —
concatenating these disjoint sub-series destroyed the correlation it was
measuring. Confirmed directly on job 15232180 (per-rung dwell/rho(1) table in
20aa sec 1). **The 254x/274x/128x/70x LOWER_BOUND numbers reported in the M2
readout above are struck — not lower bounds, not optimistic, meaningless
(no-PT tau / estimator-floor).**
FIX: real-time fixed-h reconstruction (whichever replica occupies rung k at
each timestep, continuous not concatenated) at the 9 matched h-values gives
PT tau DECREASING monotonically with h (606->226 sweeps, physically correct
direction, vs the old flat-50 artifact) and:
  MEDIAN COST-NORMALIZED SPEEDUP (PT uses 14 cores) = **1.19x**. AT h_c = **1.21x**.
h=0.305629 row uninterpretable (no-PT tau=45 there is the frozen-chain-reports-
low-tau non-ergodic trap, not fast mixing) — excluded from the median.
**PRE-COMMITTED DECISION RULE (20c) FIRES**: speedup~1 at h_c => a plain
single-parameter h-ladder is NOT justified at MCP scale on SPEED grounds;
beta-bridge stays ranked above it. BUT this is about SPEED only — PT's
separate, already-passed three-leg gate (traversal+tau_config+replica-level
z<3) means PT still buys CORRECTNESS/ERGODICITY-REPAIR at L=10, which no
amount of no-PT depth or extra chains can do (independent chains populate
basins by arrival, not weight). **Synthesis: PT buys correctness, not
throughput. Which one dominates at s* (thesis T1) is still undecided.** This
does NOT retire the beta-bridge and does NOT mean "PT doesn't help" either.
**Crossing now has a real error bar**: h_cross=0.3366+/-0.0010 (bracketed in
71% of resamples, reproduces my point estimate 0.336519). This is a FINITE-L
(10/20) crossing, lies ABOVE h_c(inf) BY CONSTRUCTION (same FSS logic as the
now-closed benchmark) — NOT directly comparable to Linsel 0.335(2), NOT a
second benchmark confirmation.
**No-PT-side tension (6/9 points z>2) is now MOOT for physics purposes**: the
no-PT L=10 ensemble is KNOWN non-ergodic (I4 violated 3-4 orders of magnitude)
so it's not a valid reference at any error bar — this campaign doesn't use it
for physics anywhere; benchmark closed via the separate PT-resourced
L=20/24/28 route instead. No further diagnostic needed on this point.
**M1(a) superseded, not wrong**: my h_c(inf)=0.323577+/-0.004243 used p=1 per
Feynman's OWN 20s spec; Feynman corrected the FSS drift exponent afterward
(p=omega+1/nu=2.42 for 3D Ising, correct for a dimensionless-ratio crossing) —
redone with proper error propagation: **BENCHMARK CLOSED, h_c(inf)=0.3359+/-
0.0049 vs Linsel 0.335(2), agreement at 0.2sigma** (`benchmark_report.md`
rewritten 304->245 lines, verdict up front, two residuals recorded not
buried: R1=provisional PT input data quality; R2=whether Linsel's 0.335(2)
is itself finite-L or infinite-L, still open, doesn't block). M1(b) energy-
Binder NEGATIVE accepted as a legitimate null, not retried — s* now rests on
percolation_probability alone.
NEXT per Feynman 20aa sec 8: **M4(1) invariant-provenance audit is now the
TOP item** (free, unscoped, highest-value debt). M3 denser s*-grid proceeds
but its design requirement got STRONGER (20x sec 6): sizing a crossing
extrapolation to fit an exponent (as just done for the benchmark) needs MORE
than 3 sizes and a WIDER L range than the original 20w M3 spec states — s*
work at the MCP cannot borrow a known exponent the way the benchmark could,
since determining the universality class IS the campaign.

## M4(1) AUDIT COMPLETE: 84 rules found across harness/manual/architecture docs,
## only 5 properly validated+wired. Reported to Feynman, NOT remediated (2026-07-25)
James (read-only, reasoning=high, 18 calls) audited every numbered/lettered
rule in `multicritical_execution_harness.md`, `paratoric_run_manual.md`, and
`pt_architecture.md` against actual code enforcement in `scripts/*.py`.
**84 distinct rules: 5 VALIDATED+WIRED, 14 VALIDATED-NOT-WIRED (the
intermittency-tripwire disease — real evidence, no code check), 15
WIRED-NOT-VALIDATED (the old F3/F7 disease — a check exists, no cited
measurement justifies its threshold), 44 NEITHER (pure assertion, no
citation, no code), 6 ALREADY CORRECTED (known F3/F7/H1/H2/tripwire cases).**
Full 84-row table: `/project/theorie/a/A.Otaifi/ai-workflow/agent_bridge/logs/msg_codex_1784989171.txt`.
**Single highest-stakes gap**: harness G2 and run-manual F6 — the actual
L=120 PT GO(>=5x)/marginal(2-5x)/NO-GO(<2x)/budget-guard decision rule for
committing MCP-scale compute to PT — is **NEITHER validated by a real
measurement NOR wired into any code check**. `ensemble_diagnostics.py` reports
a speedup number but never applies the GO/NO-GO bands, L=120 condition,
winding-sector-change condition, or budget guard. Given M2 just established a
real PT-speedup-measurement methodology (the real-time fixed-h reconstruction
fix from 20aa), this is now both urgent AND cheap to wire — it would directly
reuse code that already exists.
Other notable findings: harness G1.3/pt_architecture S4 ("PT-on marginal
equals PT-off, z<3") computes the z-value but never applies the pass/fail
threshold — a correctness REGRESSION check that currently can't fail no
matter what the number is. `paratoric_run_manual.md`'s own C7 pitfall entry
still contains STALE "judge by strict round-trips" wording that contradicts
the already-corrected F7 rule — a live in-doc inconsistency. I5's threshold
is inconsistent between files (harness says z<2, manual says z<3, code
defaults differ by command).
Worker's own risk-ordered remediation priority (not yet executed, Feynman's
call): (1) G2/F6 PT GO/NO-GO + budget gate; (2) H3/H4b/H5/H6/F1/F2/F8 +
Decision 9.4 ladder-health/barrier rules; (3) G1.1-G1.5/S0-S6 correctness
gates (pre-expensive-PT checks); (4) manual G2/G6/C3/C8 thermalization-depth
rules; (5) harness G4 universality/first-order claim gates; (6) G5/C1/C5/C6
observable semantics.
NOT REMEDIATED — audit only, per the task scope. This is a substantial new
work item on its own; scoping/prioritizing the fix is Feynman/PI's call, not
mine to start unilaterally given its size (84 rules) relative to the rest of
this session's already-large scope.

## Feynman OVERTURNED his own 20aa verdict (20ab): 1.19x/1.21x speedup INVALID
## -- no-PT baseline fails ergodicity at h_c itself. Two-stage gate implemented,
## Martina-reviewed APPROVE (2026-07-25)
Checking harness G2's actual wording (part of the M4(1) audit) surfaced a live
contradiction: G2 says GO/NO-GO on RAW tau (>=5x/<2x), UPDATE 20c says judge on
COST-NORMALIZED Neff/core-hour — SAME M2 data gives raw speedup 7-29x (G2: GO)
vs cost-normalized 1.19x (20c: NO-GO), and neither doc said which metric
governs. Worse: the no-PT baseline used for that 1.19x/1.21x number FAILS this
project's own per-chain spread/ergodicity gate at 6-8 of 9 matched h-values
**including h_c itself** — dividing PT tau by a non-ergodic reference. Per 20i,
a basin-trapped chain reports LOW tau, so no-PT tau is UNDERestimated exactly
there, meaning **1.21x at h_c is not a measurement, it's a lower bound off a
broken denominator — UNMEASURABLE, not "measured and low."** Feynman withdrew
the 20aa "decision rule fires" conclusion. Only at 3 points where the baseline
IS valid (h=0.339574/0.343521/0.347399, the fast/easy end nobody needs PT for)
is cost-normalized speedup genuinely measured: 0.51-1.19x.
**Fix, Feynman's design**: G2/F6 need a TWO-STAGE gate, not a single number.
STAGE 1 CAPABILITY (always runs): is the no-PT reference ergodic at this h
(per-chain spread/I4 gate)? If NO -> `CAPABILITY_REQUIRED`, cost-normalization
UNDEFINED, PT/beta-bridge/worm needed on capability grounds regardless of
speed. STAGE 2 EFFICIENCY (only if stage 1 passes): cost-normalized
Neff/core-hour vs G2's original bands (now explicitly applied to the
cost-normalized ratio, resolving the metric ambiguity).
**Implemented by Clark** (workspace-write, 5 calls): new `pt-capability-gate`
mode (`ensemble_diagnostics.py:1771-1858`), G1.3/S4 z<3 assertion actually
wired (`:1668-1734`, was computed-but-never-applied before), D4 Ising-line
regression test wired (`:2034-2070`, self-test PASS: h_c=0.335668+/-0.00483,
z=0.128 vs the closed benchmark). Doc rewrites: `multicritical_execution_
harness.md:58-78` (G2), `paratoric_run_manual.md:279-288` (F6),
`pt_architecture.md:212-217,309-313` (D4/S4). Registry updated, legacy-TSV
SHA-256 regression PASS. Clark's self-test found the REAL gate flags 8/9
points as CAPABILITY_REQUIRED (not Feynman's hand-quoted 6/9) — Clark
correctly declined to weaken the gate to force a match, flagged the
discrepancy rather than silently picking a side.
**Martina APPROVE, and adjudicated the discrepancy**: Clark's code is a
faithful reuse of the EXISTING, already-established spread/I4 gate
(`per_chain_raw_ratio_spread`, `spread_window_scaling_status`, confirmed via
file:line evidence) — Feynman's hand-table was a looser informal raw-spread
read, not the actual codified criterion. So the code is right; the hand-table
undercounted. One caveat: the z<3 check emits PASS/FLAG text but doesn't set
a nonzero exit code — consistent with this campaign's read-the-printed-status
convention throughout, not treated as a defect. Grace/OpenRouter failed on a
genuine backend error (HTTP 504, zero inspection calls, not a prompt-format
issue this time) — proceeding on Martina's substantive review alone.
NEXT: report this resolution to Feynman; M3 was explicitly gated on this item
landing (20ab sec 6: "M3 waits until item 1 is done") — now unblocked.

## M3 DESIGNED + L16 PILOT SUBMITTED: jobs 15263626 (z-basis)/15263627 (x-basis)
## PENDING (2026-07-25)
PI chose "design and submit now" (no separate approval checkpoint) when asked
how to proceed on M3. Design (James, FRESH, workspace-write) at
`/scratch/a/A.Otaifi/paratoric_selfdual_m3_denser_grid_20260725/`
(`DESIGN.md`+`design.tsv`): 5 sizes L={8,12,16,20,24} (existing 2A data only
had 3 — satisfies 20x's "more sizes, wider range" exponent-fit requirement),
grid s={0.325,0.330,0.335,0.340,0.345,0.350}, both bases, 16-rank PT per
(L,basis) — PT chosen on EVIDENCE not assumption: existing L=16 near-s* data
already shows FAIL (x-basis s=0.325, tau factor 2.23>I4 bar) and
OBSERVABLE-DEGENERATE (both bases s=0.350, zero-fraction 0.81-0.83) under
plain chains. 10 unique size-pair crossings nominal (d.o.f.=8) but correctly
flagged as CORRELATED (shared sizes) — conservative independent adjacent-pair
count is only 4 crossings (d.o.f.=2); design mandates joint replica
bootstrap/full covariance AND leave-one-size-out stability checks, avoiding
the benchmark's own "3 points, zero power to determine p" mistake. Cost:
~37-56 task-hours across 10 one-node jobs, 32G each. Design's OWN stop
condition: stage L=16 both-basis pilot first, gate on capability+ladder-
health, do NOT release L=8/12/20/24 until that pilot passes.
I followed the design's own staging recommendation rather than submitting
the full 10-job array at once: submitted ONLY the L=16 pilot (2 jobs, one per
basis). Self-dual PT ladder convention verified against actual CLI parsing
(`src/cli/paratoric.cpp`/`extended_toric_code_qmc.hpp:3821-3823`: omitting
`--pt_ladder_lmbda_start/end` sets `runtime_lmbda=runtime_h`, confirming
s=h=lambda tracks together per rung, NOT Phase 2's h-only convention).
Binary reuse verified independently (not just trusted from DESIGN.md): SHA-256
`a804b53f...`, commit `98c45e8e` confirmed ancestor of current HEAD, `git diff
--quiet` over `src`/`CMakeLists.txt` returned zero — safe to reuse without a
rebuild. Ladder: 16 rungs, 6 exact measurement s-values + 10 reservoir/bridge
rungs, `0.285...0.380`.
Worker's own `sbatch --test-only` hit a SANDBOX socket restriction this time
(`Operation not permitted`, not the recurring transient controller-DNS issue —
a genuine capability gap in the worker's execution sandbox, distinct from the
earlier known DNS flakiness) — correctly stopped without forcing it, all
local checks (bash -n, ladder correctness, binary provenance, task maps)
already PASS. I ran `--test-only` (PASS both) and the real `sbatch` myself
directly (unrestricted Bash access). **Jobs 15263626 (z) / 15263627 (x)
PENDING**, notifiers 15263628/15263629 submitted cleanly.
NEXT: readout the L16 pilot once both complete — capability gate (per-chain
spread/I4 via the new `pt-capability-gate`), ladder health (traversal,
per-link acceptance, tau_config), replica-level agreement. Only if that PASSES
does the L=8/12/20/24 array release; the design's own stop condition, not
mine to waive.

## L16 pilot MIXED: z-basis COMPLETED clean; x-basis FAILED instantly — hard
## architectural limit, NOT a transient bug, and NOT actually needed (2026-07-25)
Verified independently via `sacct` before trusting "job done": z-basis
(15263626) COMPLETED 19m56s exit 0:0. x-basis (15263627) FAILED in 1 second,
exit 2:0, ALL 16 ranks: `"Exception caught: Parallel tempering requires
basis=z and lattice_backend=boost."` Checked the source directly
(`src/mcmc/extended_toric_code_qmc.hpp:3771-3772`): this is a COMPILE-TIME
`if constexpr` check — PT is only template-instantiated for `Basis=='z'` with
the Boost backend. **This is a permanent architectural limit, not a transient
failure — retrying would fail identically every time**, and fixing it for
real would need actual C++ template work, not a config change. James's
pre-submission smoke test did not catch this (a real gap in that check).
**But it turns out not to matter**: confirmed directly via h5py on the
COMPLETED z-basis run's `obs.h5` that it already records BOTH channel
families — `percolation_probability`/`percolation_strength` (z-native) AND
`plaquette_percolation_probability`/`plaquette_percolation_strength`
(z-basis's plaquette channel). Per this project's own established duality
mapping (`z-percolation_probability == x-plaquette_percolation_probability`,
`z-plaquette_percolation_probability == x-percolation_probability`, already
used for the I2 duality check and cited in `DESIGN.md`), **the "x-basis"
physics this campaign needs is already fully present in the z-basis PT run's
plaquette channel.** The M3 design's inclusion of a literal separate x-basis
PT job was the actual mistake — not the job's failure. A working PT-based
duality check needs z-basis-only PT plus the existing direct-vs-plaquette
comparison within that SAME run, not two separate basis runs.
**Consequence**: dropping the 4 x-basis rows from `design.tsv`'s
`HOLD_AFTER_L16_PILOT` set roughly HALVES the total M3 array cost (10 jobs ->
5 jobs, ~37-56 task-hours -> ~19-28). Revising `design.tsv` accordingly before
any array release.
NEXT: readout the SUCCESSFUL z-basis pilot (capability gate, ladder health,
replica agreement, AND the direct-vs-plaquette duality cross-check within
that same run standing in for the cancelled x-basis arm) — this is the real
gate for releasing L=8/12/20/24 (z-basis only now).

## L16 z-basis pilot readout: NO-GO. MY OWN duality-workaround reasoning was
## WRONG — correcting it, not glossing over it (2026-07-25)
Jones (read-only, reasoning=high) assessed the z-basis pilot against the
design's own 5-leg stop condition. Results:
1. **Traversal PASS**: strict_roundtrips=46, all 16 replicas visited both
   endpoints — full traversal confirmed.
2. **Production motion UNRESOLVED**: the analysis tool errored
   (`ValueError: production-motion acceptance requires at most one swap
   phase between stored samples`) — this swap_period/sample_period
   combination doesn't satisfy that tool's assumption. A cruder combined
   thermalization+production link-acceptance number was obtained instead
   (median=0.391, harmonic=0.157, min=0.0274) but is NOT the same
   measurement and is flagged as such.
3. **Capability gate PARTIAL**: only s=0.325 and s=0.350 (the 2 points with
   EXISTING no-PT reference data from job 15177203) could be checked —
   both CAPABILITY_REQUIRED (tau factor 4.98 and 8.56 respectively),
   confirming the original design rationale. The 4 NEW s-values
   (0.330/0.335/0.340/0.345) have NO_BASELINE — no existing no-PT data to
   compare against, so capability is simply unassessed there, not passed.
4. **Duality: MY OWN ERROR, corrected here.** I had instructed the readout
   to compare z-basis's direct channel (percolation_probability) against
   z-basis's plaquette channel (plaquette_percolation_probability) as a
   substitute for the failed x-basis PT arm, reasoning that the established
   duality mapping meant this was equivalent. **Jones correctly caught that
   this is wrong**: those two z-basis channels are COMPLEMENTARY quantities
   WITHIN the same basis (see `doc/paratoric_run_manual.md:129`), not a dual
   PAIR with each other — the actual duality relation is CROSS-basis
   (z-direct <-> x-plaquette, z-plaquette <-> x-direct). Comparing them
   directly gives huge, physically-expected disagreement (9.6-75.7 sigma)
   that means NOTHING about duality — it was never a valid test. **Duality
   remains genuinely UNTESTED**, not satisfied by a shortcut, since x-basis
   PT cannot run at all (architectural limit) and no x-basis data exists for
   this ladder/s-grid otherwise. This is my mistake to own, not Jones's or
   Clark's — I conflated "both channels present in one run" with "duality
   checked," and it took an independent read to catch it.
5. **Replica-level agreement PASS**: all 6 s-values comfortably under z<3
   (z=0.83-2.47).
Also flagged: stationarity concerns at s=0.325 (direct channel) and s=0.340
(plaquette channel) specifically — not pooled across all 6 points.
**GATE: NO-GO on releasing L=8/12/20/24.** Per Jones's explicit list of what's
needed first: (a) production-motion analysis compatible with this
swap_period/sample_period combination (or an explicit acceptance of the
cruder combined number as good enough); (b) actual no-PT controls at the 4
new s-values, OR an explicit decision to skip capability-checking new points
and trust the existing s=0.325/0.350 evidence generalizes; (c) a genuine
x-basis duality control (via no-PT x-basis chains, since PT can't do it) or
an explicit decision to defer/waive duality checking for this campaign; (d)
resolve the 2 flagged stationarity points.
NOT resolving these myself in this session — this needs either more design
work (item c in particular is a real open question: build no-PT x-basis
controls despite their own possible ergodicity risk near s*? skip duality
entirely? something else?) or Feynman/PI's read on which gaps are acceptable
to waive vs must be closed. Full evidence:
`/tmp/paratoric_m3_l16_gate_20260725/{pt_traversal.txt,
production_motion_attempt.txt, capability_measurement.tsv,
replica_measurement.tsv, duality_measurement.tsv, i5_measurement.tsv}`.
