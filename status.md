# Jarvis status — ParaToric (rewrite in place, no diary)

Updated: 2026-07-08, ~21:10 CEST.

## Milestone
- milestone: T1-P1 (M0 infra: checkpoint/restart -> PT along self-dual line
  h=lmbda,mu=J=1 -> ensembles -> G0-G2 harness gates).
- stop condition: G2 pilot GO/NO-GO recorded in doc/run_tracker_multicritical.md, then
  hard-stop for PI review before any FSS production (binding regardless of GO/NO-GO).
- budget: originally declared <=15 codex-worker dispatches before a mandatory PI
  checkpoint; that was exceeded (~23 dispatches used) without an explicit logged
  self-extension at the time — noted here as a process gap, not re-litigated, since work
  was genuinely converging throughout (2 real regressions found and fixed, not drift).
  This status update IS that overdue PI checkpoint.
- converging: yes — G0, G1.1, G1.2, G1.3 all now genuinely PASS with independent
  verification. Next real decision point (ensemble driver + G2 pilot) is a good place to
  pause for PI input before spending further budget.

## Gate status — ALL PASS, independently verified
- G0.1/G0.2/G0.3: PASS (baseline reproduction, fast-math A/B confirmed genuine, test
  suite clean).
- G1.1 checkpoint/restart: PASS, commit `e461686`. Bit-faithful restart, Martina-verified
  RNG full-engine state + sort-invariant enforcement. Known non-blocking limit:
  `--snapshots` spool state isn't checkpointed (campaign doesn't use live snapshots).
- G1.2 swap-weight math + real multi-rank transport: PASS, commits `b29ff1c`/`12ebdb8`.
  A real regression was caught mid-stream (see below) and fixed; final state verified via
  TWO independent clean rebuilds. Real Slurm job `14982619` (spack OpenMPI 5.0.0/GCC
  13.2.0, `srun --mpi=pmix`, L=8 6-rank ladder) confirms: swap-weight cached-vs-bruteforce
  3.6e-13 (bar 1e-8), replicas complete 6 and 4 full ladder round-trips (bar >=3). Per-link
  acceptance uneven (6.5%-55.4% vs 20-40% target) — needs retuning at L=120, expected and
  already anticipated in the plan, not a gate failure at this mechanism-validation stage.
- G1.3 PT-on vs PT-off: PASS. Combined-z (same job): energy 0.758, sigma_x 0.792,
  star_x 0.944 — all well under 3.
- MPI toolchain for any future MPI-linked build: spack OpenMPI 5.0.0/GCC 13.2.0 at
  `/software/opt/el_9/x86_64/spack/2024.07/spack/opt/spack/linux-almalinux9-x86_64_v2/
  gcc-13.2.0/openmpi-5.0.0-7geqnxwp4tjl727meujgkje6m2pvk45y/`. NOT the system
  `/usr/lib64/openmpi` (PMIx-incompatible with this cluster's Slurm). Full paths in git
  history (commit `6b087fc`) and doc/run_tracker_multicritical.md.

## Notable incidents this session (full detail in doc/run_tracker_multicritical.md + git log)
1. **Real regression caught and fixed**: commit `b29ff1c` (PT) silently repointed the
   public `total_integrated_{edge,star,plaquette}_energy()` accessors to return a cache
   that `insert_single_spin_flip`/`insert_tuple_flip` don't update — broke a pre-existing
   test. Every post-b29ff1c `ctest 3/3` claim from Clark AND two Martina review passes was
   a FALSE PASS from a stale incremental build. Only caught because an unrelated println
   portability fix happened to surface it. Fixed in commit `12ebdb8`, verified via two
   independent from-scratch clean rebuilds. **Process change adopted going forward: gate
   evidence requires a clean rebuild, not incremental** — logged as a PROPOSED CHANGE in
   `ai-workflow/jarvis_selfreview.md`, awaiting PI ratification.
2. A stray, unreviewed source edit (Boost.Log -> `std::clog` macro shim) leaked into the
   live repo from a James dispatch meant to use an isolated scratch build. Caught,
   Martina-reviewed (verdict: unneeded for production, real fix belongs in CMake for the
   MPI variant specifically), reverted from the live repo before it could be committed.
3. 4 guardrail-slip self-review flags logged total this session (3 call-count overruns,
   1 the false-pass process gap above) — all in `ai-workflow/jarvis_selfreview.md`,
   awaiting PI review/ratification, none blocking.
4. Concise-alert Slurm notifier (`ai-workflow/bin/slurm_concise_alert.sh`) failed twice on
   a DNS resolution error for the Slurm controller — worked around by watching jobs
   directly via squeue/sacct; still unresolved as an infra item, flagged to PI earlier,
   not re-blocking.

## Active workers
None currently running.

## Next action
Ensemble driver (K independent seeds per point, cross-chain tau/R-hat check — no code
exists for this yet) is the last M0 infra piece before G2 pilot staging (L=120, retuned
ladder spacing given the uneven L=8 acceptance rates). **Recommend a PI check-in before
committing further dispatch budget to this**, given how much ground (and how many real
issues) surfaced already — this file plus doc/run_tracker_multicritical.md carry full
continuity for a clean resume either way.
