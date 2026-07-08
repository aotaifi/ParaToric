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
- **G1.2 RETRACTED — real regression found in commit `b29ff1c`.** Codex usage limit
  reset at ~20:40 CEST as expected; resumed via /loop at 20:51 CEST. Found (via
  paratoric_tampered) the real MPI toolchain fix: spack OpenMPI 5.0.0/GCC 13.2.0, not
  system `/usr/lib64/openmpi` (paths recorded permanently in git history, commit
  `6b087fc`, and in doc/run_tracker_multicritical.md — no need to re-derive). Dispatched
  James with that recipe; new blocker surfaced: GCC 13.2.0 lacks `<print>`/`std::println`
  (needs GCC14+), used in 5 debug-print helpers in lattice.cpp. Had Clark replace them
  with portable `std::cout << std::format(...)` (trivial, print-only change) — but his
  own ctest run then showed `test_lattice` FAILING (`integrated_edge_energy_diff_
  combination_test_1`, expected 4/6.32, got 0/0). Rather than accept Clark's "unrelated"
  judgment, had Martina check (found it fails on committed HEAD too, i.e. not caused by
  the println diff) then had Jones run a full CLEAN-REBUILD bisection across commits
  (not incremental): baseline `7394885` PASS, `e461686` (checkpoint) PASS,
  **`b29ff1c` (PT) FAILS**. Conclusion: the PT commit broke a basic Lattice energy-diff
  cache invariant, and every "ctest 3/3 passed" claim in the G1.2 round (from both Clark
  and 2 Martina review passes) was a FALSE PASS from a stale/incremental build directory.
  **This invalidates my logged G1.2 PASS** — see the CORRECTION row in the tracker.
  Also invalidates confidence in the swap-weight cached-vs-bruteforce 1e-14 agreement,
  since it depends on the same cached-energy machinery; needs re-verification from a
  clean build, not assumed still valid. Logged as a PROPOSED-CHANGE self-review entry
  (`ai-workflow/jarvis_selfreview.md`): gate-evidence dispatches must now require a
  clean rebuild, not incremental, before I log PASS in the tracker — awaiting PI
  ratification, not blocking the fix work below.

## Active workers
None currently running.

## Next action
Dispatch Clark to find and fix the ACTUAL root cause of the b29ff1c regression in
lattice.cpp/.hpp (Martina's earlier, pre-this-discovery hypothesis: `insert_single_
spin_flip`/`insert_tuple_flip` may not route through whatever `set_/add_potential_*`
path updates the cached `total_integrated_edge_energy`/`total_integrated_star_energy` —
needs confirming against what actually changed in b29ff1c's lattice diff, not assumed).
Also apply the still-pending println->format portability fix (verified correct in
isolation, just needs to land on top of the real fix) so the GCC13/spack MPI build
compiles. Require a CLEAN rebuild (`rm -rf` the build dir first) for the final
pass/fail verification this dispatch reports — no more incremental-build "ctest passed"
claims accepted as gate evidence. Once genuinely fixed and clean-verified: re-run the
G1.2 cached-vs-bruteforce swap-weight check from scratch (don't reuse the old 1e-14
numbers), get a fresh Martina review, THEN resubmit the real multi-rank Slurm job with
the spack OpenMPI 5.0.0/GCC 13.2.0 recipe for G1.2 (round-trips/acceptance) and G1.3
(PT-on vs PT-off) — paths and launch method already fully specified in
doc/run_tracker_multicritical.md's CORRECTION row and the commit history, no need to
re-derive.
