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

## Running (2026-10-11): continuous-time generalised SW (switch after two failed worm versions)
- gsw.js (commit 7e0611d): FK for all four terms (h, mu cuts; lmbda, J bonds), uniform GF(2) solution by dense bitset
  elimination. L=2 ED gate passes (39 comparisons, rms z 0.97, max 2.27, incl. h=0); J-bond rate x0.5 fails (35/39).
- Probes (240 s, one chain each): tau(plaquette) per SW step 3.4 / 4.3 / 5.5 at L = 4 / 6 / 8; per CPU 37x (L=6) and
  66x (L=8) faster than local. Prior (70% freeze in the flux sector) was wrong.
- Scan job 17127772 (18 single-core tasks: gsw L 8/10/12/16/20 x ord/rnd; local L 8/12/16/20 x ord/rnd; node-speed
  calibration in every run), launch4.sbatch, out4/, series4/, watcher4 pid 591083.
- Kill line (set before the scan): tau per SW step must grow with exponent <= 1.5 between L=6 and 12 (local: 3.7).
  RESULT: PASS, exponent 0.54 (tau 5.1/5.9/6.3/7.4 SW steps at L 8/10/12/16). Per CPU gain 36 (L=8) falling to 6-30
  (L=16-20) because dense elimination costs 28 -> 1280 local sweeps per step (L 8 -> 20). Report s11.
- NEXT (critical path): a solver whose cost per step stays a fixed number of sweeps. Plan: peel variables occurring
  in one equation (XORSAT leaf removal), components separately, sparse elimination on the core; measure cost per step
  at L = 8-24. Done = cost per step <= ~20 sweeps at L=20 with the same tau per step. Budget 2-3 days.
  Then the C++ port (production source on a branch: needs the PI).
  - Solver v2 (commit 7a88084): row dedupe + XORSAT peeling + dense core; L=2 ED gate passes (max |z| 2.04), broken arm
    fails. Workstation timing: 4.3 / 14 / 59 / 219 ms per step at L = 8/12/16/20, i.e. ~19/17/35/51 local sweeps using
    the scan calibrations (was 28/133/420/1280). Scan job 17128161 (gsw L 8-24 x ord/rnd, out5/), watcher5 pid 647333.
    RESULT: tau per step 5-10 at L 8-24; cost per step 14/28/61/130/250 sweeps (L 8/12/16/20/24, ~L^3.3); gain per CPU
    71/69/59/68/~52: a constant ~50-70x, not growing. Report s12.
- PI 2026-10-11: target L = 200-400. Approved: C++ port on branch worm/gsw-cpp (worker gsw-cpp, gates: L=3 open ED,
  JS prototype at L=4/6, production means at L=16/24, timing L=8-32); lead works on scaling.
- Scaling track: exact BLOCK moves (commit 5f5f8f6; outside an ell x ell spatial block the canonical decomposition is
  pinned; twists of vertices on pinned edges pinned; outside stars/kinks unchanged: 0 of 200 moves changed them).
  Block-only is not ergodic (ell=2 freezes star parities): always mix with local moves. Probes (400 s, + 1 local sweep
  per sample): tau in local-sweep units: global gsw 183 (L=12); ell=8: 2095 (L=12), 3538 (L=16) => ~L^1.8 (two points);
  ell=12 at L=16: 493 (global there: ~420). Scan job 17128991 (L 12/16/24/32 x ell 8/12), /scratch/a/A.Otaifi/esworm_block/.
  Hypothesis: fixed-ell blocks give tau ~ L^(1.8-2) per sweep vs local ~L^2.8: gain growing ~L; plus blocks parallelise.
- Block scan 17128991 RESULT (out/): tau in sweep-equivalents, ell=12: 1459 / 1915 / 2506 at L = 16/24/32 (~L^0.8);
  ell=8: 1410 / 2880 / 13280 (too small at L=32). BUT block chains sit 3-16 sigma off equilibrium (plaquette high, star
  low), like local ordered-start chains: blocks cannot change global sectors (all-vertex star parity, edge-loop windings).
  Fix under test: electric SW step (O(N), flips those sectors) every sample. Job 17129569 (L=6 correctness vs global;
  L 16/24/32 ell=12 ord+rnd; L=32 ell=16), out2/, watcher2.
- Corrected block scan 17129569 RESULT (out2/): electric SW fixes the star sector (star 0.9029-0.9036, tau 2-8 samples,
  both starts); L=6 block+esw agrees with global gsw (plaquette 0.9038(4)/0.9041(5) vs 0.9040(3)). But with sectors
  mixing, flux tau with ell=12 is 11000/13900 (L=24) and 3500/18500 (L=32, starts disagree) sweep-equivalents: fixed
  blocks do not decorrelate the long-wavelength flux. ell = L/2 works (L=32 ell=16: 1470) but costs ~ global.
- STEP-BACK (scaling, 2026-10-11; second scaling design failed):
  tool: shrink the redraw's GF(2) system by reductions + coreductions (peeling, unit propagation, parity union-find)
    before elimination; scales only if the irreducible core is o(N).
  reference: Mischaikow & Nanda, Discrete Comput. Geom. 2013 (Morse reductions shrink complexes; PDF-level skim);
    Fugacci, Iuricich, De Floriani arXiv:1811.04485 (reductions + coreductions; abstract).
  result (cheap test, reduce.js): core = 25-28% of variables, 51-54% of equations at L = 6/8/12/16: constant. KILLED.
  => exact global redraw costs ~N^2 per step (nested-dissection bound); practical ceiling L ~ 96-128 with a C++
     nested-dissection solver (derived). L = 200-400 needs a new idea (no exact cluster method known for this model).
- PI 2026-10-11 (after the step-back): push on, aim L = 200-400; do not give up. Lead's correction: the "30 h/sample at
  L=200" estimate extrapolated the single-thread JS dense solver; constants, not exponents, were the problem.
- Nested dissection test (ndtest.js, sparse elimination of the peeled core in ND order): front width 221 / 586 / 1254,
  top separator 1115 / 3246 / 5936, work 1.4e6 / 4.5e7 / 2.4e8 at L = 16 / 24 / 32 (L=24/32 from 6 warm steps only).
  Fronts ~L^2 as expected for 3D. Cost model (derived): L=200 front ~5e4, dense bit-packed front ~2e12 word-ops ->
  ~4-40 s per step on a 64-core node, ~300 MB; L=400 ~64x that.
- SCALING PLAN (milestones, each with a done-test):
  M1 C++ global gsw (worker gsw-cpp, running): gates G1-G4. Done = gates pass.
  M2 C++ multifrontal GF(2) sampler (ND order, bit-packed dense fronts, OpenMP) replacing the dense core.
     Done = same ED gate + identical distribution to M1 at L=16 + step time L=32/48/64/96 measured; target <= 10 s
     per step at L=96 on one node.
  M3 tau per step and start/seed agreement at L = 48-96 (C++), cross-checked against production at L <= 32.
     Kill: tau per step growing faster than ~L^1 between 32 and 96.
  M4 L = 128, 200 production design.
  Running now: JS global gsw L=32/40 both starts (job 17131136, out6/); production reference table (worker).
- CROSS-CHECK vs MCP-lead production (2026-10-11): reference table /scratch/a/A.Otaifi/esworm_xcheck/production_reference.md
  (55 runs, L 8-32; worker; reproduces hladder plaquette 0.902945(881)). gsw prototype chains (job 17128161, ord+rnd)
  vs production PT-only pooled means with scatter errors: 30 comparisons (plaquette, star, E/L^2, sigma_x, sigma_z at
  L = 8, 16, 24), max |z| 2.22 (gsw_vs_production.txt, tools/esworm/xcheck.js). gsw starts agree with each other.
  Production single replicas (local moves) do NOT agree among themselves at L >= 16: star splits ~0.899 / ~0.904,
  ordered vs random up to 15 sigma at L=16, 78/120 pairs > 3 sigma at L=24, pil32_Lam trapped 12-18 sigma; same
  signature as the prototype's local chains from ordered starts (slow global sectors). Relevant to the main project's
  error bars: report to the PI (the main PLAN is not ours to edit).
- MCP lead reply (2026-10-11, session a-otaifi-76): agrees with the cross-check and sector reading (his own: 8 L=24
  singles scatter 3.5-6x beyond block errors). Figure of merit for nu is the scatter error of kappa3(S) and kappa2(delta)
  per core-hour from >= 4 chains, not plaquette tau. Decisive check: kappa3(S) at L=16, s=0.339: -1.2022e-6(31)
  (nu_even_first_20261010/cums.json). His gain estimate: 10-250x pending C++ benchmark + kappa3 check. L=48 on hold
  until then. Requests: sector observables + production per-sample observables in the C++ port (sent to gsw-cpp).
  RUNNING: kappa3 check job 17131275 (8 gsw chains, L=16, s=0.339, /scratch/a/A.Otaifi/esworm_xcheck/k3/).
- L=32/40 global gsw JS (job 17131136, out6/, series6/): tau per step 5.5 / 6.6 at L=32 (486 / 950 samples).
  Second-half means: L=32 ord vs rnd plaquette 0.9010(10) / 0.9036(7) (2.2 sigma), sigma^x 2.5 sigma; L=40 (100 samples
  each, ~15 tau, block errors underestimated ~2x) plaquette 0.9061 / 0.9017, ord star 0.8997. INCONCLUSIVE: chains too
  short; no equilibration claim beyond L=24 until long C++ chains with sector observables (first job after C++ ED gate).
- KAPPA3 CHECK PASSED (job 17131275, /scratch/a/A.Otaifi/esworm_xcheck/k3/, kappa.js, 10% discarded, 8 chains, scatter
  errors): L=16, s=0.339: kappa3(S) = -1.2079e-6 +- 6.1e-8 vs MCP lead -1.2022e-6 +- 3.1e-8 (0.08 sigma); kappa2 =
  4.7728e-4(18) vs his chains ~4.76e-4; mean S 1.80609(7) vs ~1.8062. Cost: 8 x 3.5 h = 28 core-h (JS) vs his 6 PT
  chains 1768 core-h (sacct 17120726-33 excl. cancelled: 70.7 h x 25 cores). Variance x cost: 1.0e-13 vs 1.7e-12
  => 16x better per core-hour in JS; block-error chi2/dof for k3 1.72 (scatter error used).
- MCP lead accepts kappa3 check (recomputed it himself from our series). KAPPA2(delta) at L=16, s=0.339 also passes:
  6.6409e-4(31) vs his 6.6267e-4(38) (0.29 sigma), same 8 chains. His remaining gates before moving L=48/64 onto gsw:
  (1) C++ port + ED (L=3 open) + delta_tau_avg / energy_h / energy_lmbda recorded; (3) L=32 at s*=0.3391 vs his sizing
  run (/scratch/a/A.Otaifi/paratoric/ctladder_prod_20261011/prod/L32_size: kappa3(S) -4.92e-8(1.37e-8), <S> 1.80542,
  kappa2(delta) 9.59e-5(4.6e-6)) with measured C++ cost per core-hour. His L=32/L=48 ladders (done Tue ~10 UTC /
  Wed-Thu) become our large-L references.
- NEXT, two tracks: (a) C++ port with bit-parallel dense core (production source on a branch: PI approval) to turn the
  constant into production speed for L <= 32; (b) a solver with ~linear cost per step (needed for L >= 48: dense core
  memory ~N^2). (a) is the practical win; (b) is research with unknown odds.

## Dead ends
- Guided worm (open weight exp(-alpha*(dSpace+dTime)), b445287): gain at L=12 0.08 / 0.26 / 0.05 for alpha 0.25 / 0.5 / 1;
  L=16 timed out. Job 17127191, out3/, report s10. (A 300 s probe had claimed 6.5: heavy-tailed worm cost.)
- ES worm v1 (constraint checked only at closing): gain per CPU 3.5 / 6.9 / 1.2 / 0.09 at L = 6 / 8 / 10 / 12, mean worm
  length 4.7e3 -> 7.5e7; tau per ES step flat (27-44). Job 17126323, /scratch/a/A.Otaifi/esworm_tau_20261010/out2/, report s9.
- (none measured yet in this sub-project; move-mix reweighting and CT block heat bath are closed in the main PLAN.md)
