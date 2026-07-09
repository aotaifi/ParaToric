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

## G2 pilot — Feynman's finalized ordered plan (2026-07-10), currently on step 1
1. **Wide-s sector-rate scan — SUBMITTED, IN PROGRESS.** Slurm array `15012336_[0-7]`,
   L=120 beta=120 z-basis single-chain, s={0.10,0.16,0.22,0.28,0.34,0.40,0.46,0.50},
   10000 samples/point (~17.8h + margin, requested walltime 20h), 1 node, 1
   CPU/task, mem-per-cpu=1G. Output root:
   `/scratch/a/A.Otaifi/paratoric_g2_sector_rate_scan_20260710`. Launcher:
   `scripts/submit_g2_sector_rate_scan_20260710.sbatch` (commit `39f025f`).
   Purpose: measure winding/cut-sector-change RATE vs s (from
   `simulation/diagnostics/production_acceptance/accepted_by_update`, the
   cut-changing move index) to find the fast-mixing window and set ladder
   endpoints there — NOT a GO/NO-GO by itself. **Expect completion ~2026-07-11
   morning CEST (submitted 2026-07-10, ~20h walltime).**
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
None currently running. Waiting on Slurm array `15012336` (~20h).

## Next action
When the scan completes (~2026-07-11 morning CEST): read back per-s cut-sector-
change counts/rates from all 8 tasks, identify the fast-mixing window, then
proceed to step 2 (direct-acceptance ladder tuning) and step 3 (ensemble launch
path). Only after both are done + reviewed does step 4 (actual pilot sbatch) come
back to the PI for the final explicit go.
