# For the MCP lead: a non-local flux update, cross-checked against your data (2026-10-11)

From the flux-worm sub-project (branch worm/es-stage0). The PI asked for your opinion: do these runs make sense, and how
much runtime do they really save you?

## The update (exact, no accept/reject)
Continuous-time generalised Swendsen-Wang for the sigma^z worldlines. Every term gets an Edwards-Sokal decomposition:
cut points (where a flip may sit) on edges = kinks + Poisson(h), on vertices = star events + Poisson(mu); pins at rate
2 lmbda on sigma^z=+1 stretches (sigma^z=+1 there) and at rate 2J on B_p=+1 stretches (B_p=+1 there). Given the marks,
all admissible histories weigh the same, so the new configuration is a uniform solution of a GF(2) linear system
(segment variables; peeling + dense core elimination). Flux and charges are redrawn together.
- Code: tools/esworm/gsw.js (node prototype), local updates ported 1:1 from extended_toric_code_qmc.hpp (qmc.js).
- Validation: 2x2 torus exact diagonalisation at 8 coupling points incl. s* and h=0: 39 comparisons, rms z 0.97,
  max 2.27; the J-pin rate halved fails 35/39. A C++ port with ED (L=3 open) and production gates is in progress
  (branch worm/gsw-cpp).
- Picture: https://claude.ai/artifact/RPViyMyACy7S5Qqyupncpd

## Cross-check against your production runs (s*, beta = L)
Reference table built from 55 of your runs: /scratch/a/A.Otaifi/esworm_xcheck/production_reference.md (+ .json).
Prototype chains (job 17128161, ordered and random start) vs your PT-only pooled means with scatter errors:
30 comparisons (plaquette, star, E/L^2, sigma^x, sigma^z at L = 8, 16, 24), max |z| = 2.22
(/scratch/a/A.Otaifi/esworm_xcheck/gsw_vs_production.txt). Example L=16: plaquette 0.903155(160) / 0.903013(170) vs
your 0.903091(43); sigma^x 0.240217(250) / 0.240272(280) vs 0.240128(59); E/L^2 -2.13200(12) / -2.13195(11) vs
-2.131858(32).

Something you should look at: your single-replica runs at L >= 16 do not agree with each other beyond their errors
(star splits ~0.899 / ~0.904; ordered vs random up to 15 sigma at L=16; 78/120 pairs > 3 sigma at L=24; pil32_Lam
12-18 sigma from the other L=32 runs). The same signature appears in the prototype with local moves from an ordered
start, and disappears with the global update: slow global sectors (all-vertex star parity, edge-loop windings) that local
moves almost never change. Single-replica error bars on star and the delta/S channels are probably too small. Not
proven: no sector observable is stored.

## Runtime
Measured (one core):
| L | your clean PT (indep16) | your local single replica (not clean) | gsw JS prototype |
|---|---|---|---|
| 16 | ~17 s per independent sample (derived: 25 rungs, 720k samples, tau 13, ~2e6 moves/s) | 3.5-9 s | 1.5 s |
| 24 | - | 110 s (tau 2050 samples x 51200 moves at 1.9e6 moves/s; jobs 17123717/18) | 25 s |
tau(plaquette) per gsw step: 5.1 / 6.3 / 6.9 / 9.3 / 6.9 at L = 8 / 12 / 16 / 20 / 24 (two chains each, they agree);
equilibrates from either start within 200 steps. Cost per step grows ~L^6 (exact linear solve), local tau ~L^2.8 per
sweep, so the gain is a large constant, not growing.
Projected (C++ assumed 20x faster than JS: not yet measured; nested-dissection cost ~L^6), core-hours for 1e4
independent samples: L=24: 3.5 (local 300), L=32: 21 (1600), L=48: 225 (17000), L=64: 1250 (92000), L=96: 14000.
Uncertainty x10 until the C++ benchmark and the L=32/40 tau (running) are in.

## Questions for you
1. Do the cross-check and the reading of your single-replica scatter (sector trapping) make sense to you?
2. Which observables / estimators do you need for nu (kappa_3(S), derivatives), and is a plaquette-tau based cost
   the right figure of merit, or does S or delta decorrelate differently?
3. Your estimate of the real runtime gain for your planned L=24/32 even-channel campaign.
Please give your answer to the PI directly.
