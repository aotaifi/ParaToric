# ParaToric — Child Agent Instructions

Read parent `/project/theorie/a/A.Otaifi/AGENTS.md` first; this file is the
ParaToric scope lock. Do not mix sibling-project notes/scripts.

## Bootstrap order (keep context minimal)
1. `doc/run_tracker_multicritical.md` — current-state card + gate log (authority
   for campaign status; rewrite in place, don't append diaries).
2. `doc/multicritical_universality_plan.md` — milestones, observables, literature.
3. `doc/multicritical_execution_harness.md` — HARD gates/invariants; no
   production submission without the relevant gate PASSED in the tracker.
4. Only then task-specific evidence: `performance_report.md`,
   `doc/performance_notes_2026-06.md`, `doc/performance_deep_dive_plan.md`.

## Fixed facts (verified 2026-07, commit d085509)
- Production binary: `build-current-clang-portable/paratoric` (portable; other
  build dirs may use -march=native and die with Illegal instruction on nodes).
- Build: CMake, C++23; `PARATORIC_ENABLE_FAST_MATH` defaults ON — production
  policy pending harness gate G0.2.
- Production backend = Boost `Lattice`. `flat_square` is experiment-only: gated
  in `src/io/io.cpp` to 5 simple observables (no percolation/susceptibility),
  and hour-scale runs show it ~1.1-1.3x SLOWER (but 1.1-1.6x less memory).
- Bottleneck = autocorrelation, not CPU: all 7 Metropolis moves are spatially
  local (dispatcher `src/mcmc/extended_toric_code_qmc.hpp:metropolis_step`);
  worst observable percolation_strength (tau ~2200 @ L200 b400; ~1e4 @ L120
  multicritical). Judge everything in Neff = N/(2*tau_worst), never raw samples.
- No checkpointing, no MPI parallelism yet (M0 of the plan adds them).
- Per-update acceptance diagnostics already in HDF5:
  `simulation/diagnostics/production_acceptance`.

## Scope locks
- Reading/review tasks: no source edits, no Slurm submissions.
- Slurm: 1 node default, explicit memory (ref: 3.2 GB @ L1000 b400), concise
  alert wrapper, pilots before arrays.
- No L>=256 production or worm-algorithm work without explicit user approval.
