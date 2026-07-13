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
Readout pending: per-link acceptance, strict round-trips, tau_ergodic
(standing PT diagnostics rule) + Binder-cumulant h_c crossing check.
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
