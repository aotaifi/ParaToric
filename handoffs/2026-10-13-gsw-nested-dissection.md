# Handoff 2026-10-13: global flux update (gSW) with a nested-dissection solver

## What was built
- A global Monte Carlo update for ParaToric's extended toric code, the continuous-time generalised Swendsen-Wang (gSW).
  Random marks are placed on the worldlines; given the marks, every allowed configuration is equally likely, and the new
  configuration is a uniform random solution of a mod-2 (GF(2)) linear system.
- A nested-dissection solver for that system, branch worm/gsw-nd. Per update it costs ~L^3.6 instead of ~L^7 for the dense
  solver: L=48 0.6 s (was 23 s), L=64 1.5 s, L=96 7.8 s, L=128 24 s on one core.
- Production binary: /scratch/a/A.Otaifi/gsw_prod_20261012/bin/paratoric_7b37b7c. Plan and full record:
  docs/worm/PLAN_worm.md.

## Why
- The local update freezes slow global sectors at L >= 16: replicas started differently disagree beyond their errors. The
  main project's nu needs L >= 64.
- gSW decorrelates in ~2-9 updates at every L tested. Per core-hour it gave 65x (kappa3) and 390x (kappa2(delta)) smaller
  variance than the parallel-tempering ladder at L=32.

## Rejected alternatives (each measured)
- Plain vison worm: membrane cost grows with area. Killed.
- Worm dressed with electric clusters: worms could not close (length 4.7e3 -> 7.5e7 steps from L=6 to 12).
- Block moves: fixed-size blocks do not decorrelate long-wavelength flux at L >= 24.
- Algebraic reductions of the linear system: the core stays 25-28% of the variables.
- Dense GF(2) solver: correct, but ~L^7 per update and ~10 GB at L=64.

## Checks it passed
- Exact diagonalisation at L=2 and L=3, at several couplings. A deliberately broken rate fails by 100-1000 sigma.
- Agreement with production at L <= 24.
- The main lead's kappa3(S) and kappa2(delta) gates at L=16 and L=32.
- A uniform-sampling test of the solver; checkpoint restarts are bit-identical.

## Three things to understand
1. The update is exact (no accept/reject); its correctness rests on the GF(2) draw being uniform, which is tested directly.
2. Precision is limited by physics, not the algorithm: the skewness of S shrinks with L, so kappa3 gets expensive.
3. s = 0.3391 turned out to be off-critical; the scan moved s* to ~0.34068, and the nu campaign now runs there.

## Open (as of this note)
- The per-sample E_lmbda is recorded at one time slice, so exponential reweighting in s is not exact. A tau-integrated
  sigma_z observable is being added. Slopes from covariances are unaffected.
