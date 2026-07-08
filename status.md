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
None currently running. Next: James, to stage/submit a small Slurm `srun` job (L=8, few
replicas, 1 node, short walltime) to complete G1.2 (real acceptance/round-trips) and G1.3
(PT-on vs PT-off).

## Next action
Dispatch James (workspace-write, run-staging role) to submit the small validation job via
`srun` with OpenMPI on PATH, read back HDF5 diagnostics, and report numbers. On PASS: move
to ensemble driver, then G2 pilot (L=120) staging. On FAIL/marginal: report to PI before
retuning ladder spacing or investigating further — this is the actual go/no-go-adjacent
gate, not something to silently retry past.
