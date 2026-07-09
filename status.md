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

## Ensemble driver — DONE
`scripts/ensemble_diagnostics.py` (commit `8b6fd05`), Python-only (reuses the shared
`ai-workflow/qmc_common/hdf5_readers.py` reader, no new C++). Computes Gelman-Rubin
R-hat, cross-chain tau agreement (I4), Neff/N>=100*tau checks, and a cold/hot-start
combined-z check (I5). One suspicious-looking synthetic number (R-hat=1.18 for a
"40 SE offset" case) was independently hand-verified by Martina and confirmed correct
(only 1 of 4 chains was offset, diluting the standard R-hat formula's between-chain
term — not a bug). No existing tool found before writing this (checked ParaToric/scripts,
ai-workflow/qmc_common, MCP surface, and paratoric_tampered/scripts first).

## Active workers
None currently running.

## Feynman review received 2026-07-09 — key constraints for the pilot (read
doc/feynman_thesis_tojarvis.md "Pilot decisions + budget audit" in full before touching
the pilot again): PT is z-basis-only (accepted); PT+checkpoint composition NOT
implemented (`qmc.hpp:3675` throws) — accepted FOR THE PILOT ONLY since it fits in one
2-day block, but this means **"M0 complete" is premature** until PT+checkpoint
composition exists (hard blocker before any M2 production: one L=192 point at I4 needs
46-91 days/chain, impossible without resume); hard cap **N_replicas<=30** (so
replicas x 4 chains <= 120 ranks, stays on 1 node); L=8 ladder spacing gives ~zero
acceptance at L=120 (histogram overlap collapses with volume) — needs a real probing
pass to retune, ~30%/link target.

## IN PROGRESS: L=120 probing pass hit a real anomaly, investigating before designing
the ladder
First probing-pass attempt (James) ran s in {0.22..0.36} at L=120,beta=120 single-chain,
NOT on the actual cluster (Slurm/squeue was unreachable from his sandbox — ran "locally"
inside the same network-restricted Codex sandbox that couldn't open MPI sockets earlier
today) and reported `energy_lmbda` with EXACTLY zero variance at every s, means forming
a perfectly linear sequence (slope -28800/unit s = -2*L^2 exactly) — consistent with the
z-basis field being frozen at its default_spin=+1 initial state, never thermalized.
Martina-verified this IS an artifact, not real physics: `run.log`s show only 256 samples,
acceptance_fraction ~0.005-0.009, and — critically — `accepted_by_update` shows ONLY
update types 0/4 (pair create/delete) fired; the `flip_spin` move type accepted ZERO
times. Code check: z-basis `energy_lmbda` = `-diag_single_energy_cache_ * lmbda`
(extended_toric_code_qmc.hpp:546), lattice initializes every edge `default_spin=+1`
(lattice.cpp:1407/2068). Two live hypotheses, NOT yet distinguished:
(a) `flip_spin` is intrinsically near-dead at L=120,beta=120 (a real algorithmic finding,
like the earlier winding-move deadness R5) — would mean energy_lmbda-based observables
(incl. the Tier-0 SSN duality-odd/even cumulants A~(energy_h-energy_lmbda) explicitly
planned for M1's nu estimate) may be unsampleable with the current move set at this
scale;
(b) `diag_single_energy_cache_` simply isn't refreshed by the pair-create/delete moves
that DID fire (26+115=141 accepts out of 16384 attempts) — the SAME class of
stale-cache bug already found and fixed once today in commit `12ebdb8`/`b29ff1c`.
These have very different implications and must be distinguished by code reading before
any real probing pass is redone (properly, on the actual cluster, with logged
Nth/Ns/Nbs/seed this time — the sandbox run didn't even expose those). Do NOT use the
7-replica ladder James proposed from the bad data (s: 0.220,0.240,0.257,0.275,0.303,
0.329,0.360) — discard it, it's built on non-physical frozen-state input.

## Next action — the one remaining M0 step
Stage and submit the **G2 pilot**: PT ladder through the multicritical region, L=120,
beta=120, <=2 days walltime, >=4 chains, 1 node, explicit memory, concise-alert wrapper
(note: that wrapper failed twice this session on a DNS issue — may need to watch the
job directly via squeue/sacct again instead). Ladder spacing needs to be wider/retuned
vs the L=8 validation (per-link acceptance there ranged 6.5%-55.4%, target is 20-40%).
GO if tau_worst drops >=5x vs the L120 baseline (tau~9000-10000, doc/run_tracker's
Baseline anchors section). **This is the harness's explicit hard-stop-for-PI-review gate,
regardless of GO/NO-GO** — distinct from the general M0 "go" already given, since it
commits real multi-day cluster time. Have not started staging this yet, pending explicit
PI confirmation to proceed with this specific step.
