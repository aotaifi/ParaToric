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

## Next action
1. Redispatch Martina (read-only) to review D4 diff `e42914d` once Codex's
   usage limit resets.
2. Document plaquette-as-POP identification (Feynman item 1) in this file +
   run_tracker_multicritical.md.
3. Stage: (a) >50%-density confirmatory check (electric channel should turn
   ON above p_c=0.5 — nails the threshold story), (b) longer-thermalization
   scan at L=16-32 to find cold/hot convergence, (c) Linsel benchmark re-run
   on plaquette+rotate+long-thermalization once (b) lands.
4. Jarvis is at critical usage (~4% at last check, PI-acknowledged) — a
   ScheduleWakeup was armed (capped at the tool's 1h max, PI asked for 3h)
   as a fallback; if this session is cut by usage limit before the next
   checkpoint, a fresh session should resume directly from this file.
