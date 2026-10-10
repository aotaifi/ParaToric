# PLAN_worm -- non-local flux update (sub-project; current state only, 2026-10-10)

Goal: a move that cuts the L-growth of tau_int(plaquette) at s* (per sweep ~L^2.8, L=16->24).
Report: docs/reports/2026-10-10_flux_worm_feasibility.md. Main ParaToric PLAN.md, jobs and workers are not ours.

## Open questions
1. **Build the vison-string worm?** PI's decision. My recommendation: no (odds >10x at L>=32 ~10%).
   - Budget if pursued: stage 0 (1 day, no jobs) before any worm code; stage 1 8-12 days + ~300 core-h.
   - Scope line: chosen = vison-string worm in the existing sigma^z representation (extended ensemble with an
     equal-time 't Hooft string). Passed over: surface worm in the dual (membrane) representation (Delgado Mercado
     et al. lose where boundaries are costly, which is the MCP: 2.5% links occupied); CBMC/Rosenbluth chain growth
     (same membrane cost, global accept); worm in the x basis (dual, same problem with stars).
   - Stage 0 fail criterion: sigma_best >= 0.03 per edge per unit tau at L=16 and 24, cost linear in area.
2. **Swendsen-Wang for the classical Z2 gauge-Higgs model** (hypothesis, ~30%, ~8 days). Needs PI approval: it is a
   change of formulation (classical, SSN-style). Kill test: z_eff(12->24) > 1.5 or gain at L=24 < 10x per CPU.
3. **ES-dressed vison worm** (section 7 of the report): SW bonds for the Higgs term make the vison membrane free;
   residual constraint = no linking with electric-cluster cycles. Odds ~35% (>10x at L>=32), revised after stage 0'.
   - Stage 0' PASSED 2026-10-10: area cost of vison loops 0.39 -> 0.005-0.009 per cell (kill line 0.03); report
     section 8; /scratch/a/A.Otaifi/paratoric/es_stage0_20261010/; tools/es_stage0/ on branch worm/es-stage0 (f86f8d2).
   - Stage 1 APPROVED by PI 2026-10-10 (max 2 workers, no rush). Budget 8-12 days + ~300 core-h.
     Fail: gain at L=24 < 3, or gain(32)/gain(16) < 2.
   - Method (lead's choice): prototype first, in node (no compiler/python on th-ws-7010m51), tools/esworm/:
     (1a) local-update QMC + exact diagonalisation of the 2x2 torus (8 spins) -> validated baseline;
     (1b) electric SW step (cuts = stars + Poisson(mu), bonds 2 lmbda on sigma^z=+1, flip clusters);
     (1c) vison worm on flux variables, weight h^Nk exp(J int B) only; at closing: existence test of a valid
          sigma^z (bond parity BFS over 8 alternatives: twist set T or complement x 4 topological edge-loop flips;
          T from kink-parity changes), then rebuild sigma^z and star events (new star at cut c iff old XOR eta-change);
     (1d) ED gate for 1b and 1b+1c, each with a deliberately broken arm;
     (1e) tau(plaquette) per CPU-second, local vs local+ES-worm, L = 8, 12, 16 (beta = L) in the prototype.
     Port to production only if (1e) shows the L-growth cut.
   - STEP-BACK 2026-10-10 (worm v1 exact but long worms: 80% of closes infeasible, mean 3569 steps at L=6 vs 113
     without bonds; the constraint is only seen at closing):
     tool: measure tau per CPU-second of v1 vs local at L=6-12 BEFORE redesigning; if v1 loses on cost, switch the
       m-sector update to continuous-time generalised Swendsen-Wang (all four terms as FK bonds/cuts, uniform GF(2)
       sampling of the flip pattern), which has no worm and no closing problem;
     reference: Pizzimenti-Duncan-Schweinhart arXiv:2507.13503 (plaquette SW, uniform cocycle sampling, z_E 0.08 in 4D
       Z2; full text, worker; tables checked); Rieger-Kawashima cond-mat/9802104 (continuous-time cluster; abstract);
     continue (measure v1); switch only on a measured failure.
     RUNNING: tau scan job 17126323 (array 0-15, one chain per task, partition small, 1 core x <=4 h; T = 4*iL + k,
       L = 6/8/10/12, k = local s1 ord, local s2 rnd, es s3 ord, es s4 rnd; maxSec 6600), launch2.sbatch, run dir
       /scratch/a/A.Otaifi/esworm_tau_20261010/ (out2/, series2/, VERDICT2_T*.txt; src sha256 in SHA256.txt, code = commit
       ad99a13), watcher2 pid 430188 (PPID 1) -> WATCHER2.log.
       Job 17126320 (first launcher, 4 chains per 4-core task) never started: --hint=nomultithread doubles the CPU count
       past the small-QoS cap (PENDING QOSMaxCpuPerJobLimit). Cancelled with PI approval 2026-10-10 (never ran). Its outputs (out/, series/)
       do not collide with 17126323's. Decision rule: gain in tau(plaquette)
       per CPU-second of es over local must grow with L (gain(12)/gain(6) >= 2) to keep the worm; else switch.
     Passed over: Dirac-sheet bookkeeping so the open worm sees the constraint (history-free but needs a cycle basis
     and spanning sheets per bond sample: days, unknown payoff); per-step consistency relative to the start
     configuration (history-dependent open weights: detailed balance not guaranteed).
   - Design fact found 2026-10-10: a vison loop encircling a vertex (in projection) flips that vertex's star parity
     (mutual statistics); with fixed cuts this is a twist of the electric sector, allowed by the existence test.

## Measured / derived inputs
- <sigma^z> = 0.238 (L=24), 0.237 (L=16) at s*, derived from measured energies,
  /scratch/a/A.Otaifi/paratoric/hladder_endpoint_20261010/analysis_runs.json. Bare membrane cost 2 lmbda <sigma^z> = 0.161.

## Running
- Guided worm (open weight exp(-alpha*(dSpace+dTime)), commit b445287, ED gate passes): alpha scan job 17127191
  (11 single-core tasks: L 8/12/16 x alpha 0.25/0.5/1, plus local L=16 x2; 6600 s each), out3/, watcher3 pid 546747.
  Probe (300 s, L=12): gain per CPU 6.5 (alpha 0.5), 3.9 (alpha 1) vs 0.09 unguided; at L=8 alpha=1 is worse than
  unguided (tau 293 vs 31 samples). Caveat: tasks land on mixed node types (met-cl, th-cl); CPU-time ratios across
  nodes carry up to ~2x noise (seen at L=6 in 17126323).

## Dead ends
- ES worm v1 (constraint checked only at closing): gain per CPU 3.5 / 6.9 / 1.2 / 0.09 at L = 6 / 8 / 10 / 12, mean worm
  length 4.7e3 -> 7.5e7; tau per ES step flat (27-44). Job 17126323, /scratch/a/A.Otaifi/esworm_tau_20261010/out2/, report s9.
- (none measured yet in this sub-project; move-mix reweighting and CT block heat bath are closed in the main PLAN.md)
