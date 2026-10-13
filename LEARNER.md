# Learner profile: flux-worm / gSW work in ParaToric (draft by flux-worm-lead, 2026-10-13; PI to correct)

## Understands
- The goal and its stakes: equilibrated data at large L (asked for L = 200-400) to pin nu, and that the local update is too slow there.
- Cost thinking in core-hours and wall time; decides budgets fast once given a table ("Yes", "budget is free").
- Parallelism by running more independent chains to cut wall time (asked for it directly).
- That compiled C++ beats a prototype language for production, and when to switch (pushed for the port).
- Process levers: watchers, check-in intervals, prompt-cache TTL (set check-ins < 1 h himself).

## Not yet
- Why a JavaScript prototype came first: asked twice; the second time he wanted the technical reason (no compiler and
  broken python on the login node, V8 JIT), not the project-management one.
- What the global update actually does (guess): he asked for a picture "to look and understand, not read text", so the
  Edwards-Sokal / GF(2) construction is probably not yet intuitive.
- (guess) How kappa3(S), the Binder ratio and the skewness feed into nu; he follows the verdicts, not the statistics.
- (guess) Why the cost per update grows like L^7 (dense) versus L^3.6 (nested dissection).

## Glossary (used without explanation)
- gSW: continuous-time generalised Swendsen-Wang update; redraws all spins at once given random "marks".
- Edwards-Sokal marks: random cuts and pins that make every allowed history equally likely.
- GF(2): arithmetic mod 2; the update reduces to drawing a uniform solution of a mod-2 linear system.
- Peeling / XORSAT core: removing equations that fix one variable each, leaving a hard core.
- Nested dissection (ND): solving the core region by region, separators last; made L=64-128 affordable.
- s*: the coupling h = lambda = s on the self-dual line where the transition sits (now ~0.34068, not 0.3391).
- kappa2, kappa3: variance and third cumulant of S = plaquette + star; their size-scaling gives nu.
- Binder ratio kappa4/kappa2^2 of delta = plaquette - star: a crossing quantity to locate s*.
- Skewness: kappa3 / kappa2^1.5; sets how many samples a fixed precision on kappa3 costs.
- Sector observables: Wilson-loop, star-parity, winding parities; slow global modes that show trapping.
- Chain-scatter error: error bar from the spread of independent chains (not block errors).
- tau / tau_eff: autocorrelation time, in updates.
- ED: exact diagonalisation of a tiny lattice, used as the correctness gate.
- Checkpoint (ckpt.bin): saved chain state and series; restarts continue bit-identically.
- Reweighting / multi-histogram: predicting other couplings from one run's samples.

## Misconceptions seen
- (guess) Expected L = 200/400 from the first speed-up; the dense solver capped us at ~L=48 until ND.
- None clearly wrong otherwise.
