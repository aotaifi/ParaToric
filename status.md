# Jarvis status — ParaToric (rewrite in place, no diary)

Updated: 2026-07-08.

## Milestone
- milestone: T1-P1 (M0 infra: checkpoint/restart -> PT along self-dual line
  h=lmbda,mu=J=1 -> ensembles -> G0-G2 harness gates).
- stop condition: G2 pilot GO/NO-GO recorded in doc/run_tracker_multicritical.md, then
  hard-stop for PI review before any FSS production (binding regardless of GO/NO-GO).
- budget: <=15 codex-worker dispatches before mandatory PI checkpoint; self-extend once
  (cap 3x) only if genuinely converging, per JARVIS.md override rule.
- spent so far: 8 dispatches (James G0.1/G0.3/G0.2; Martina G0.2 verification; Jones
  paratoric_tampered scan; Clark PT implement + MPI-continue + bugfix-continue; Martina
  PT review round 1 + round 2 fix-verify). Converging: yes.

## Gate status
- Both go-conditions met 2026-07-08: Feynman ratified thesis T1 + plan T1-P1 `status: GO`
  in `doc/feynman_thesis_tojarvis.md`; PI said go in this chat. Dispatching is authorized.

## Progress
- G0.1/G0.3/G0.2: all PASS (see doc/run_tracker_multicritical.md for numbers).
- Cross-project scan (paratoric_tampered): real MPI PT code exists there but not directly
  portable (48 commits diverged, no checkpoint at HEAD, partial validation only). Using
  as a DESIGN reference only (swap-weight formula, MPI odd/even pattern in
  `paratoric_tampered/src/mcmc/parallel_tempering.hpp`); implementing fresh against
  current ParaToric's templated `ExtendedToricCodeQMC<Basis,LatticeT>`.
- **G1.1 checkpoint/restart: PASS**, commit `e461686`. Clark implemented; Martina
  independently verified (not just re-stated) RNG full-engine state, sort-invariant
  enforcement on load, no mid-update tear risk. Known non-blocking limitation:
  `--snapshots` spool state isn't checkpointed (campaign doesn't use live snapshots).
  Tracker updated commit `b6209c8`; continuity files added commit `566d4e8`.
- 3 guardrail-slip self-review flags logged (`ai-workflow/jarvis_selfreview.md`), all
  exceeding the 25-call hard-split rule (James 53, Jones 41, Clark 69/88/99). 3rd
  escalated per discipline to a PROPOSED CHANGE: the 25-call ceiling may be miscalibrated
  for single coherent "implement + self-validate" C++ dispatches specifically (as opposed
  to open-ended investigation, where James/Jones's slips were genuinely avoidable).
  **Awaiting PI ratification** on that carve-out; not self-granted, not blocking further
  work. (Tightly-pinned Martina re-verify pass only needed 5 calls — supports the
  diagnosis that scope-pinning, not raw task size, is the lever.)
- **G1.2 PARTIAL PASS, commit `b29ff1c`**: self-dual ladder + O(1) cached swap weight
  validated (max deviation 1.8e-14/1.6e-14 across two configs, bar <1e-8). Round-1 Martina
  review found 2 real bugs (susceptibility mis-normalization under PT; MPI_Init with no
  Finalize/return-code checks) + 2 minor (no pt_ladder validation; thin test coverage) —
  all fixed, round-2 Martina confirmed all 4 fixes. **BLOCKED**: real multi-rank `mpirun`
  validation (G1.2 round-trips/acceptance rate, all of G1.3 PT-on-vs-off) impossible in
  the Codex sandbox — `mpirun` can't open sockets there (confirmed sandbox limitation, not
  a code bug, by 2 independent reviews). OpenMPI 4.1.1 IS installed on the actual cluster
  (`/usr/lib64/openmpi/{bin,lib}`, just not on default PATH) — this needs a real Slurm
  `srun` job, not a Codex dispatch.

## Active workers
None currently running — **BLOCKED on Codex usage limit** ("Try again at 8:40 PM",
observed 17:40 CEST 2026-07-08). Cannot dispatch further Codex workers until the limit
resets; PI needs to resume this manually, ScheduleWakeup is a /loop-only tool and doesn't
apply to a plain conversation.

## Slurm validation attempts so far (all diagnosed, none yet succeeded)
1. Job `14981055`: FAILED in 16s — build used system default compiler, no working
   `<format>` support. Root cause found (known-good build pins `clang++-20` + a Boost
   shim include); fix given to James.
2. Job `14981225`: build succeeded with the fix, but runtime FAILED in 4s —
   `ompi_mpi_init: ompi_rte_init failed, Returned "Not found" (-13)` under
   `srun --mpi=pmix`. Diagnosis: PMIx version mismatch between this cluster's Slurm and
   system OpenMPI 4.1.1 — a cluster integration issue, not a code bug. Fix identified
   (bypass Slurm PMI entirely, use plain `mpirun -np 6 --oversubscribe` since it's a
   single-node job — OpenMPI can fork ranks locally without needing Slurm's PMI) but
   NOT YET EXECUTED — the dispatch to apply it hit the Codex usage limit before running.
3. Separately: `ai-workflow/bin/slurm_concise_alert.sh submit <jobid>` failed twice
   (DNS resolution failure for the Slurm controller hostname) — flagged to the PI,
   not yet resolved, not blocking (monitoring done directly via squeue/sacct instead).

## Next action (when Codex access resumes)
Re-dispatch James with the plain-`mpirun` fix (already fully specified, ready to send
verbatim) for the L=8 6-rank ladder job. On success: read back real swap acceptance/
round-trip numbers (G1.2) and PT-on vs PT-off z-values (G1.3), have Martina review if
anything looks off, log the gate result, then move to ensemble driver and G2 pilot (L=120)
staging. On repeated failure: report to the PI rather than continuing to guess at cluster
MPI configuration — 2 fallback attempts (plain mpirun, then vader-forced mpirun) is the
agreed ceiling before escalating instead of continuing to iterate blindly.
