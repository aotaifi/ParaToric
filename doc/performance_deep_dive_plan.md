# ParaToric Performance Deep-Dive Plan

Date: 2026-07-03

## Summary

This is a code-reading-first plan for deciding whether ParaToric can support large-system, low-temperature, multicritical production runs within practical walltime. The initial phase should not submit new runs and should not edit source code. The hard budget for the deep-dive chat is 130k tokens; stop immediately if this budget is close to being exceeded or if newer commits supersede the inspected code.

The main question is whether the limiting factor is CPU/runtime cost, memory growth, autocorrelation from inefficient update physics, or a combination of these.

## Reading Order

1. Inspect the production QMC loop, update dispatch, acceptance bookkeeping, and how `N_thermalization`, `N_between_samples`, and `N_samples` are consumed.
2. Inspect worldline/event storage: spin-flip containers, tuple-flip containers, insertion/deletion/search paths, integrated-energy interval logic, and memory growth with `L` and `beta`.
3. Inspect observable cost: energy/star/plaquette totals, percolation and cluster observables, susceptibility/string observables, and full-lattice or full-worldline scans.
4. Inspect update physics: the seven update types, their proposal probabilities, acceptance behavior, and whether they decorrelate winding strings, large clusters, string number, and susceptibility modes.
5. Inspect backend boundaries: what Boost still provides, what flat-square replaces, and why flat-square did not yet produce robust production speedups.

## Conceptual Attack Angle

Treat slow production not only as a code-optimization problem but also as a possible Markov-chain design problem. Near a multicritical point, a faster implementation of weak moves may still be insufficient if the slowest observable has very large autocorrelation time.

Evaluate whether the current update set misses important slow modes:

- winding-sector changes,
- large string/loop rearrangements,
- percolating cluster changes,
- string-number changes,
- susceptibility-dominated critical modes.

Candidate conceptual improvements to judge from the code:

- adaptive or parameter-dependent update probabilities,
- stronger nonlocal loop/string updates,
- winding-sector proposals,
- cluster-style proposals,
- parallel tempering or parameter tempering,
- annealing/continuation from equilibrated nearby points.

For each candidate, distinguish whether it reduces CPU cost per update, autocorrelation time, or both. Prefer changes that reduce the slowest-observable tau; a modest tau reduction can matter more than a modest raw speedup.

## Deliverables

- A code-referenced bottleneck map with categories: hot by structure, possibly hot, unlikely hot.
- A separation of CPU/runtime bottlenecks from autocorrelation/physics bottlenecks.
- A ranked list of low-risk code optimizations, including expected gain, risk, touched areas, and minimum correctness test.
- A ranked list of conceptual update improvements, including expected tau impact, implementation risk, and validation burden.
- A final recommendation on whether to optimize Boost, revive flat-square, redesign worldline/event storage, or prioritize algorithmic update changes.

## Guardrails

- Do not submit or stage new Slurm runs during the reading phase.
- Do not edit source code during the reading phase.
- Do not rely on microbenchmarks alone; claims must cite code structure or existing measured evidence.
- Do not judge usability by raw `N_samples`; use effective samples from the slowest observable.
- Production remains Boost unless code inspection gives a strong reason to change backend strategy.

## Working Assumptions

- One-week production means walltime at or below seven days, including finalization margin.
- Lowest acceptable physics bar is roughly `Neff = N_samples / (2*tau) >= 20-30` for the slowest relevant observable.
- Universality-quality conclusions need substantially stronger evidence than the lowest bar.
- The first pass is diagnostic and conceptual; implementation decisions come after the deep dive.
