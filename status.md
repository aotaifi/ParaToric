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

## G2 pilot — Feynman's finalized ordered plan (2026-07-10), step 1 needs REDESIGN
1. **Wide-s sector-rate scan — RAN, BUT CANNOT ANSWER ITS OWN QUESTION. Needs
   redesign before resubmitting.** Slurm array `15012336_[0-7]` finished: only
   tasks 0 (s=0.10, 15h29m) and 1 (s=0.16, 18h03m) COMPLETED; tasks 2-7
   (s=0.22-0.50) all hit the 20h walltime and TIMED OUT with ZERO output (this
   code only writes `obs.h5` once, at the very end — no partial/incremental
   output exists for the 6 timed-out points). Real, empirically-observed cost
   INCREASES with s (55768s at s=0.10 -> 65012s at s=0.16, +16.6% for a modest
   s step) — the uniform N_samples/N_between_samples across all 8 points was
   not a safe assumption; higher-s points need either more walltime or fewer
   samples/attempts.
   **More important finding (Martina code-trace, this session)**: the scan's
   entire premise — infer winding/cut-sector-change RATE from
   `accepted_by_update` — is not achievable with what was actually measured.
   `global_single_spin_flip` (index 2, exactly zero accepts in both completed
   runs over ~7.14e9 attempts each) is just an ordinary single-edge global flip,
   NOT a dedicated cut-changing move (contradicts an earlier, wrong assumption
   from this same session). The TRUE cut/winding-relevant behavior is a rare
   "wrap-around" special case buried inside the ordinary accept logic of update
   types 1/3/5, and **no diagnostic anywhere separates that sub-case from their
   ordinary (common, high-acceptance) behavior** — so indices 1/3/5's reported
   11-98% acceptance rates are uninformative for this question, dominated by
   their ordinary non-wrap behavior. Worse: this run used `--full_time_series 0`,
   so the one thing that COULD reveal sector changes directly (a per-sample
   `anyon_count`/`percolation_probability` series) was never even recorded.
   **Net result: ~34 core-hours of real compute (2 completed points) plus 6
   timeouts produced move-acceptance statistics, but NOT the winding-sector-
   mixing-rate-vs-s answer the scan exists to produce.**
   REDESIGN NEEDED before resubmitting: rerun with `--full_time_series 1` and
   analyze the `anyon_count`/`percolation_probability` per-sample series for
   sector changes (or add a dedicated wrap/cut-accept counter to the code —
   more invasive, would need its own review cycle); also re-size N_samples/
   N_between_samples per-s given the observed cost increase with s, rather
   than reusing one size across the whole [0.10,0.50] range. NOT YET
   REDESIGNED OR RESUBMITTED — flagging to PI/Feynman before spending another
   ~20h wall-clock cycle on this.
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
None currently running. Array `15012336` finished (2 completed, 6 timed out).

## Next action
Redesign the sector-rate scan (full_time_series=1, per-s-scaled sample counts)
before resubmitting — flagged to PI given the real wall-clock cost already spent
(~1 day) without the needed answer. Once a real sector-mixing-rate-vs-s signal
exists, proceed to step 2 (direct-acceptance ladder tuning) and step 3 (ensemble
launch path). Step 4 (actual pilot sbatch) still needs a separate explicit PI go
after that.
