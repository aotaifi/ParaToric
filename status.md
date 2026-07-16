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
