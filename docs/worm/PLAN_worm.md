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
   - Next: stage 1 (update type 7 = electric SW + constrained vison worm; L=3 ED validation; tau per CPU L=16/24/32).
     Needs PI approval (production source). Budget 8-12 days + ~300 core-h.
     Fail: gain at L=24 < 3, or gain(32)/gain(16) < 2.

## Measured / derived inputs
- <sigma^z> = 0.238 (L=24), 0.237 (L=16) at s*, derived from measured energies,
  /scratch/a/A.Otaifi/paratoric/hladder_endpoint_20261010/analysis_runs.json. Bare membrane cost 2 lmbda <sigma^z> = 0.161.

## Dead ends
- (none measured yet in this sub-project; move-mix reweighting and CT block heat bath are closed in the main PLAN.md)
