# Jarvis status — ParaToric (rewrite in place, no diary)

Updated: 2026-07-08.

## Milestone
- milestone: T1-P1 (M0 infra: checkpoint/restart -> PT along self-dual line
  h=lmbda,mu=J=1 -> ensembles -> G0-G2 harness gates).
- stop condition: G2 pilot GO/NO-GO recorded in doc/run_tracker_multicritical.md, then
  hard-stop for PI review before any FSS production (binding regardless of GO/NO-GO).
- budget: <=15 codex-worker dispatches before mandatory PI checkpoint; self-extend once
  (cap 3x) only if genuinely converging, per JARVIS.md override rule.
- spent so far: 3 dispatches (James G0.1/G0.3/G0.2; Martina G0.2 verification; Jones
  paratoric_tampered reuse scan). Converging: yes.

## Gate status
- Both go-conditions met 2026-07-08: Feynman ratified thesis T1 + plan T1-P1 `status: GO`
  in `doc/feynman_thesis_tojarvis.md`; PI said go in this chat. Dispatching is authorized.

## Progress
- G0.1 baseline reproduction: PASS (acceptance 0.09% rel diff, energy z=1.89).
- G0.3 existing tests: PASS (3/3).
- G0.2 fast-math A/B: PASS, confirmed genuine by Martina (not a build no-op) — production
  keeps FAST_MATH ON.
- Cross-project scan (paratoric_tampered): real MPI PT code exists there but not directly
  portable (48 commits diverged, no checkpoint at HEAD, partial validation only). Verdict:
  use as a DESIGN reference (swap-weight formula, MPI odd/even pattern in
  `paratoric_tampered/src/mcmc/parallel_tempering.hpp`), implement fresh against current
  ParaToric's templated `ExtendedToricCodeQMC<Basis,LatticeT>`.
- 2 guardrail-slip self-review flags logged (`ai-workflow/jarvis_selfreview.md`): James
  G0 dispatch and Jones tampered-scan dispatch both exceeded the 25-call hard-split rule.
  Both log-only (recurrence 1st, 2nd); watch for a 3rd this session.

## Active workers
None currently running. Next: Clark (checkpoint/restart implementation, G1.1 target).

## Next action
Dispatch Clark for checkpoint/restart on the Boost `Lattice` backend, pinning exact file
targets (src/lattice/lattice.hpp, src/lattice/lattice.cpp, src/rng/rng.hpp) rather than an
open-ended "add checkpointing" ask, to avoid a 3rd hard-split violation. Then Martina
review, then G1.1 validation via James/Jones.
