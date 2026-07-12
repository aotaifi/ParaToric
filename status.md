# Jarvis status — ParaToric (rewrite in place, no diary)

Updated: 2026-07-10.

## Milestone
- milestone: T1-P1 (M0 infra + G2 pilot GO/NO-GO).
- Feynman reviewed all of M0 (2026-07-09/10), confirmed the swap-weight formula
  correct by hand (not just cache==recompute), resolved the energy_lmbda-frozen
  finding (see below), and gave GO for steps 1-3 + staging the pilot. The actual
  pilot `sbatch` (step 4) stays PI-gated (harness G2 hard stop) — separate check-in
  needed before that specific submission, per Feynman's own explicit carve-out.
- PI said go on 2026-07-10 confirming Feynman's review.

## M0 gates — all PASS (see doc/run_tracker_multicritical.md for full detail)
G0, G1.1 (checkpoint/restart, `e461686`), G1.2 (PT swap-weight + real multi-rank,
`b29ff1c`+fix `12ebdb8`), G1.3 (PT-on/off, z<1) all independently verified on real
cluster hardware. Ensemble-diagnostics tool done (`8b6fd05`). One real regression was
caught and fixed along the way (see tracker CORRECTION row + `12ebdb8`).
**Caveat carried from Feynman's audit: "M0 complete" remains premature until PT+
checkpoint composition is implemented** — not needed for the pilot (which fits in one
2-day block), but mandatory before any M2 production (L=192 needs 46-91 days/chain).

## G2 pilot — Feynman's finalized ordered plan (2026-07-10), step 1 gives a NULL result
1. **Wide-s sector-rate scan — v1 failed to measure anything (redesigned as v2),
   v2 ran cleanly but returned a NULL result across the entire tested range.
   Needs Feynman/PI physics read before proceeding — NOT an engineering fix.**

   v1 (`15012336`): only 2/8 points completed (20h walltime too short for
   higher s, cost increases with s), and — bigger problem — used
   `--full_time_series 0` so it structurally could not measure sector changes
   at all; its move-acceptance-based proxy (`accepted_by_update` index 2) was
   also confirmed by code trace to be the wrong quantity (an ordinary move,
   not a dedicated cut-changing one).

   v2 (`15034719`, 2026-07-11/12): redesigned as 8 s-values x 8 parallel
   independent chains (64 tasks, embarrassingly parallel, no MPI needed),
   1500 samples/chain, `--full_time_series 1`, `percolation_probability`/
   `percolation_strength`/`anyon_count` all recorded per-sample this time.
   Ran cleanly — 57/64 done within ~9h (no timeouts, redesign worked for
   wall-clock efficiency), remaining 6 (all s=0.50, the highest/slowest point)
   still running with headroom on their 8h cap.

   **Result: `percolation_probability`/`percolation_strength` is EXACTLY 0.0
   for every completed chain at every s-value tested (0.10 through 0.46, plus
   2 partial chains at 0.50) — zero percolation anywhere across the entire
   [0.10, 0.50] range, 84000+ samples analyzed. `anyon_count` confirmed NOT
   usable as a sector-change proxy either (91-97% of adjacent samples differ
   — ordinary local noise, not discrete topological jumps).**

   Two live interpretations, NOT distinguished yet (a physics call, not an
   engineering one): (a) the system is confined/non-percolating throughout
   this entire s-range and the actual multicritical/deconfinement region on
   the self-dual line lies OUTSIDE [0.10,0.50] — the scan never got near the
   transition; or (b) a measurement issue (percolation observable needs far
   more than 1500 samples/chain to see even one percolating cluster at this
   L, or a convention/threshold issue in how it's computed). Flagged to
   PI/Feynman — NOT proceeding to step 2 on this data pending that read.
2. Ladder spacing by DIRECT per-link acceptance (NOT snapshot variance — that was
   the resolved methodology error, see below): pick a trial ladder between the
   scan's endpoints, run one short real PT trial, read `swap_accepted_by_link`/
   `swap_attempted_by_link` from HDF5 (io.cpp:307-313), refine toward ~30%/link.
   Cap N_replicas<=30. NOT STARTED (waits on step 1).
3. Ensemble launch path: build a minimal driver OR stage >=4 seeded jobs by hand;
   confirm `scripts/ensemble_diagnostics.py` ingests them correctly (R-hat,
   cross-chain tau, cold/hot). NOT STARTED.
4. G2 pilot sbatch (PI-gated, separate explicit go needed): L=120 beta=120, <=2
   days, 1 node, retuned ladder from step 2, >=4 chains. GO requires BOTH (a)
   mechanism: per-link acceptance in band + >=3 replica round-trips, AND (b)
   physics: replicas demonstrably change winding/cut sector during the run AND
   tau(percolation_strength) drops >=5x vs the L120 baseline. (a) without (b) =
   NO-GO. NOT STARTED.

### Resolved this session: the energy_lmbda-frozen ladder-design question
Snapshot `energy_lmbda` (`diag_single_energy_cache_`) is the frozen net/cut-sector
proxy — moved only by rare cut-changing moves, NOT what the swap weight actually
uses (that's `cached_total_integrated_edge_energy()` + kink count, both of which
thermalize fine via ordinary kink create/delete). Feynman's verdict: design the
ladder from direct per-link acceptance (step 2), not snapshot variance — my
original 3-option framing was a false choice, both "use energy_h alone" and
"run a long snapshot-variance probe" were solving the wrong problem. Separately,
Feynman flagged (then self-corrected after a PI challenge) that no O(1) move
changes the winding sector — but confirmed this accumulation is SLOW, not ZERO
(tau~1e4 baseline means it does flip), so PT is a legitimate, fair test of a
coexistence-limited slow mode, not a long shot to be pre-judged NO-GO. The wide-s
scan (step 1, running now) is the honest test of whether the ladder can actually
reach the fast-mixing window. If it can't, the fallback is the WORM algorithm
(polynomial-cost winding change) — explicitly ON HOLD pending a new PI decision,
not the R5 winding-loop move (still permanently out), and not to be pre-built.

### Infra note: Codex sandbox has intermittent Slurm-controller DNS failures
Same root cause as the earlier concise-alert wrapper failures (both hit
`kng-sv-slurm23.hpc.physik.uni-muenchen.de` DNS resolution). Confirmed NOT a real
cluster outage (Jarvis's own shell reaches Slurm fine throughout). Workaround used
successfully once: escalate the Codex dispatch to `danger-full-access` sandbox,
which is JARVIS.md's own narrow pre-authorized exception ("restore network
reachability for an already-authorized action"). Use this if a future dispatch
hits the same DNS error twice in workspace-write.

## Active workers
None currently running. **v2 sector-rate scan submitted and running**: Slurm array
`15034719_[0-63]` (43 RUNNING, 1 PENDING at last check), redesigned per PI decision
after v1 (`15012336`) only completed 2/8 points and lacked `full_time_series`.
v2 design: 8 s-values x 8 independent parallel chains each (64 tasks total,
embarrassingly parallel — no MPI/node constraints, unlike the actual pilot),
1500 samples/chain (12k aggregate/s-value, more depth than v1's 10k), same
N_between_samples=5e6, `--full_time_series 1` + `percolation_probability`/
`percolation_strength` added to `--observables` (the actual fix — v1 couldn't
see sector changes at all without this). 8h walltime/task, generous margin
vs the measured 5.6-6.5s/sample from v1's 2 completed points; losing one of 64
short tasks to a timeout is cheap, unlike losing one of 8 long ones. Output root:
`/scratch/a/A.Otaifi/paratoric_g2_sector_rate_scan_v2_20260711`. Launcher:
`scripts/submit_g2_sector_rate_scan_v2_20260711.sbatch` (commit `3a0294d`).
Expect most tasks done within a few hours given the sizing.

## Next action
When v2 completes: analyze the per-sample `anyon_count`/`percolation_probability`
series (aggregated across each s-value's 8 chains) for actual sector-change
events — this is the real measurement step 1 was always supposed to produce.
Then proceed to step 2 (direct-acceptance ladder tuning, NOT snapshot variance)
and step 3 (ensemble launch path). Step 4 (actual pilot sbatch) still needs a
separate explicit PI go after that, per the harness G2 hard stop.
