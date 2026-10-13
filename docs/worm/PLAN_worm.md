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
- C++ GATE G1 PASSED (2026-10-12, verified by lead; worker gsw-cpp branch worm/gsw-cpp, build
  /scratch/a/A.Otaifi/gsw_cpp_20261011/build/paratoric, runs g1/{A,B,C,D}, ED ed/*/exact_*.json, compare via
  scripts/gsw/compare_chains.py on compute node, srun 17155478, 1000 samples discarded): L=3 open, 4 coupling points
  (A,B: s*=0.3391 beta 1,4; C: h .6 lm .25 mu 1.2 J .8 beta 2; D: h .15 lm .9 mu .6 J 1.5 beta 3), 8 chains per arm,
  6 observables incl. delta_tau_avg, energy_h, energy_lmbda. gsw-only and gsw+local: 48 comparisons, max z 2.72
  (scatter errors, 7 dof; block errors max 1.75). Broken arm (J pin rate x0.5): fails at all 4 points, max z 163-1118.
  = MCP lead's gate (1). Chains at L=3 cost ~7 s each.
- C++ TIMING MEASURED (2026-10-12, lead, /scratch/a/A.Otaifi/gsw_cpp_20261011/lead_timing, one core, small partition,
  gsw-only, beta=L, s*): 0.85 ms / 17 ms / 0.24 s / 1.93 s per step at L = 8/16/24/32; solve is 98% at L=32; dense core
  rows 443 / 3.6k / 12.2k / 30.3k; peak RSS 240 MB (L=32). L=16->32 per-step cost x113 (~L^6.8). vs JS on the cluster at
  L=16 (k3 check: 28 core-h / 926k steps = 0.109 s/step): C++ 6.4x faster, not the assumed 20x.
  => kappa3(S) at L=16 to the k3-check precision would cost 4.4 core-h in C++ vs 1768 for the PT ladder (half the error)
  => ~100x better variance per core-hour (derived). Dense extrapolation (L^6.8, unvalidated): L=48 ~30 s/step, L=64
  ~3.5 min/step and ~8 GB core matrix. Beyond L~48 the nested-dissection solver (M2) is required.
- RUNNING: gate (3), job 17155547 (gate3_L32/launch.sbatch, watcher gate3_L32/watch.sh PID 1346141 PPID 1 ->
  WATCHER_VERDICT.txt): L=32 s*, 8 chains (default_spin +1 x4, -1 x4), 300 therm + 12000 gSW steps each (~6.6 h),
  checkpoints every 30 min. Done = kappa3(S), kappa2(delta), <S> compared with L32_size (scatter errors over 8 chains,
  10% discard) + first-half/second-half and +1/-1 start agreement. Expected kappa3 error ~1e-8 (derived from the L=16
  error assuming kappa2(S) falls like kappa2(delta), x6.9: unvalidated). Cost 53 core-h.
- GATE (3) PRELIMINARY, 6 of 8 chains (c2-c7, rome nodes, 2.3 h each = 14.0 core-h; kappa_h5.py 0.1 discard,
  scatter errors): kappa3(S) -3.75e-8(1.17e-8) vs ref -4.92e-8(1.37e-8): 0.65 sigma; kappa2(delta) 9.21e-5(0.15e-5) vs
  9.59e-5(0.46e-5): 0.79 sigma; <S> 1.805907(76) vs 1.80542 (ref error not yet known; ct32_size row in
  production_reference: plaq 0.903271(201), star 0.902148(233)); +1 and -1 starts agree; <delta> -1.6e-4(0.9e-4)
  (self-dual point). Cost: ref job 17128038 = 4.59e6 CPU-s on 258 threads = 637 physical core-h (whole 129-rung ladder).
  Variance x core-h: kappa3 62x, kappa2(delta) 450x in favour of gsw at s* (the ladder also gives the other rungs).
  Per-step speed on rome nodes 0.68 s vs 1.93 s on hua08 (node type matters 2.8x). Per-rank obs.h5 of L32_size are
  walkers, not rungs (rank128 plaq 0.9008): reference values taken from the MCP lead's own analysis.
  MCP lead (session mcp-lead, 2026-10-12): <S> 1.80542 = physical rung 128, second half (30k samples), stitched
  across walkers by ladder_index_by_sample (his k3.py, scratchpad of session 9fb6266c); no error computed; block error
  >= ~3e-4 => offset <= 1.5 sigma. He considers gate (3) passed once the 8-chain finals arrive; send them.
- PI (via mcp-lead, 2026-10-12): approves L=48/64 (then 96) production on gsw at s*, beta=L, conditional on gate-3
  finals: >= 8 chains (half +1, half -1 start), per-sample star_x, plaquette_z_tau_avg, delta_tau_avg, energy_h,
  energy_lmbda + sector observables; target kappa3(S) <= 3% relative from chain scatter. Cost/ETA to mcp-lead first.
- L=48 TIMING MEASURED (lead_timing48/, srun 17158804, th-cl-rome01n4, 20 steps from ordered start): L=32 1.27 s/step,
  L=48 21.5 s/step (ratio 16.9 = L^7.0), core rows 34k / 117k, peak RSS 0.23 / 2.4 GB. (gate-3 chains on rome: 0.68
  s/step at L=32 averaged over 12k steps; so L=48 ~11.5-21.5 s/step.) Extrapolated L=64: ~87 s/step, ~10 GB (dense).
- COST MODEL for kappa3(S) relative error r (derived): var(k3) ~ 6 k2^3 / n_eff (checked at L=32: gives n_eff 10.2k
  for 64.8k steps, tau_eff 6.4 steps), so r = sqrt(6/n_eff)/|gamma1|, gamma1 = k3/k2^1.5 = 0.115 (L=16), 0.078 (L=32,
  +-30%). Extrapolated gamma1 ~ L^-0.56: 0.062 (L=48), 0.053 (L=64). Steps for r=3% (tau 7): L=32 7e6 (1.3k core-h),
  L=48 1.2e7 (38-72k core-h), L=64 1.7e7 (~410k core-h dense). => 3% at L>=48 needs M2 or a looser target.
- MCP lead decision (2026-10-12, "within the PI's go"; PI to confirm budget in session): L=48 target ~10% (nu fit
  16-48 gives ~+-0.014; s* systematic ~0.03 dominates). Approved: L=32 to 3% (~1.3k core-h) + L=48 to ~10% (<= 7k
  core-h). L=64/96 held until the sparse solver is benchmarked. Submit only after gate-3 finals + sector/random-start
  gates + a checkpoint-restart test with gsw; send job ids and ETA to mcp-lead.
- M2 STARTED 2026-10-12: worker gsw-nd (Sonnet, high effort: performance-critical algorithm), branch worm/gsw-nd
  (worktree paratoric_worktrees/gsw-nd, off the WIP commit), build_nd/, runs nd/. Design: per-variable space-time
  coords, recursive coordinate bisection ND on the peeled core, sparse elimination in ND order with shortest-row pivots,
  dense bit-packed tail for the last K variables, --gsw_solver dense|nd. Gates N1 (uniformity chi2 on brute-forced small
  systems + broken variant), N2 (L=3 ED), N3 (dense vs nd L=8/16), N4 (timing L=16-48/64). Derived expectation (front
  rows w_r^2 x sep w_c /64 word-ops, ndtest fronts): L=48 ~0.2-0.5 s/step (~40x dense), L=64 ~1-3 s, L=200 ~15 min
  single core / ~15-30 s on 64 cores. Unvalidated. If N4 confirms >=10x at L=48, switch the L=48 run to nd mid-way.
- Worker gsw-cpp STOPPED 2026-10-12 ~15:40 UTC (silent since 10-11 08:27, no reply to two messages); its uncommitted
  port backed up to /scratch/a/A.Otaifi/gsw_cpp_20261011/src_backup_20261012/. New worker gsw-sector (Sonnet,
  medium): WIP commit of the port, sector observables (star_parity, wilson_x/y, winding_x/y), --random_start, gates
  S1-S3, build into build2/ (build/ is in use by job 17155547).
- Worker gsw-cpp (2026-10-12 brief, superseded): sector observables, random start, G2 vs JS at L=6. Silent 15 h before that.
- NEXT, two tracks: (a) C++ port with bit-parallel dense core (production source on a branch: PI approval) to turn the
  constant into production speed for L <= 32; (b) a solver with ~linear cost per step (needed for L >= 48: dense core
  memory ~N^2). (a) is the practical win; (b) is research with unknown odds.

## Dead ends
- Guided worm (open weight exp(-alpha*(dSpace+dTime)), b445287): gain at L=12 0.08 / 0.26 / 0.05 for alpha 0.25 / 0.5 / 1;
  L=16 timed out. Job 17127191, out3/, report s10. (A 300 s probe had claimed 6.5: heavy-tailed worm cost.)
- ES worm v1 (constraint checked only at closing): gain per CPU 3.5 / 6.9 / 1.2 / 0.09 at L = 6 / 8 / 10 / 12, mean worm
  length 4.7e3 -> 7.5e7; tau per ES step flat (27-44). Job 17126323, /scratch/a/A.Otaifi/esworm_tau_20261010/out2/, report s9.
- (none measured yet in this sub-project; move-mix reweighting and CT block heat bath are closed in the main PLAN.md)

## Update 2026-10-12 evening: gate 3 final, production submitted
- GATE (3) FINAL, 8/8 chains (job 17155547, gate3_L32/final_8chains.txt): kappa3(S) -3.67e-8(0.87e-8) vs -4.92e-8(1.37e-8)
  0.77 sigma; kappa2(delta) 9.19e-5(0.12e-5) vs 9.59e-5(0.46e-5) 0.84 sigma; <S> 1.805886(64) vs 1.80542 (>= 3e-4 error,
  <= 1.5 sigma); kappa2(S) 6.18e-5(0.04e-5); <delta> -1.0e-4(0.8e-4); halves agree. Cost 24.3 core-h. Variance x core-h
  vs ladder 17128038 (637 core-h): kappa3 65x, kappa2(delta) 390x. Skewness gamma1(L=32) = 0.0755 (+-24%); tau_eff for
  k3 = 4.6 steps (n_eff 18.7k of 86.4k). Sent to mcp-lead.
- Lead verified gsw-sector gates (srun re-analysis): S1 gsw-only vs gsw+local max |z| 1.68 over 7 obs; broken arm
  |z| 143-2133; local-only per-chain wilson_x means 0.001-0.66 vs 0.184 (trapped at L=4), flips 36-82x fewer than gSW.
  S2 random start vs ED: max z 1.91 (4 chains), 1.66 / 0.93 (8 chains). (The section below was appended to this file by
  another writer, not this session; numbers now verified.)
- CHECKPOINT-RESTART TEST PASSED (gsw_cpp_20261011/restart_test, build2, L=8): restarted chain bit-identical to the
  uninterrupted chain on all 9 observables (400 samples, restart at 200).
- PRODUCTION SUBMITTED 2026-10-12 ~17:30 UTC (/scratch/a/A.Otaifi/gsw_prod_20261012/, launch.sbatch, frozen binary
  bin/paratoric_95a1731 sha256 9fba3828...f886, -C amd, gsw-only, 300 therm, seeds 1000L+T, even T +1 start, odd -1):
  L=32 job 17159255, 64 chains x 94000 steps (6.0e6 steps; 3% needs 3.5-9.3e6 given gamma1 +-24%; extend if short),
  -t 3 d; L=48 job 17159256, 64 x 12500 steps (8.0e5; ~10% if gamma1(48) ~0.062: unvalidated), -t 6 d. Expected cost
  L=32 1.2-3.2k, L=48 2.5-4.7k core-h. Watcher watch.sh PID 1502739 PPID 1 -> WATCHER_VERDICT.txt. PI budget confirmation
  asked in session (approval so far relayed by mcp-lead).

## TAKEOVER 2026-10-12 ~17:50 UTC (PI away; defaults accepted by silence, plus PI: "of course we run on faster solver if it works")
- Ceiling: 8,300 core-h production (L=32 + L=48) + 300 core-h solver tests; L=32 may extend to 9.3e6 steps.
- If ND passes N1-N4: move remaining L=48 to ND, and run L=64 production on ND (PI explicit), paid from the L=48
  savings, total still <= 8,600 core-h. Size L=64 from the measured ND timing; target ~10% on kappa3(S).
- No email except emergency; cancel only own failed jobs; branches pushed to aotaifi, never main; no paper edits.
- Check-in: Tue 2026-10-13 ~16:00 UTC (L=32 final, L=48 interim, ND timing).
- LOG:
  - 17:50 L=32 17159255 47/64 running; L=48 17159256 pending; watcher PID 1502739; gsw-nd worker running.
  - Interim tools: tools/esworm/ckptseries.js (reads the series out of ckpt.bin; exact vs obs.h5 on restart_test/P, 9
    obs x 200 samples, max diff 0) and tools/esworm/interim.js (per-chain cumulants, chain-scatter errors, +1/-1 split).
  - L=32 interim after ~35 min (62 chains, 141.7k samples, 10% discard): kappa3(S) -2.71e-8(0.51e-8) (18.8%), consistent
    with gate 3 (-3.67(0.87)e-8, 0.95 sigma); kappa2(delta) 9.22e-5(0.13e-5); <S> 1.805860(57); +1/-1 starts agree (0.7 /
    0.2 sigma); <star_parity> -0.002(2), <wilson_x> 0.039(1), <winding_x> 0.001(3). Projection: 3% needs ~5.5e6 samples
    (planned 6.0e6, 5.4e6 after discard): on the edge; extend ~10% if the final is > 3.1%. Speed 0.70 s/step.
  - 19:00 both arrays fully running (64 + 64). gsw-nd idle since 16:55 without reporting; lead read its files:
    N1 PASS (nd/n1_full_seed1.txt: 200 random systems x 3 nd variants, uniformity KS p 0.08-0.96, 0 violations;
    broken variants fail 108/109 core>0 systems at p<1e-6). N4 (nd/n4.log, 20 steps from ordered start, one core):
    L=48 nd 0.52-0.63 s/step (solve 10.5-13.6 s/20) vs dense 23.1 s/step same run => ~40x; 0.26 GB vs 2.4 GB.
    L=64 nd 1.7-1.8 s/step, 0.7 GB. Per-step nd cost L^3.6 (48->64). N2/N3 sruns 17158994/6 cancelled before start
    (cause unknown); worker asked to rerun N2/N3 and report. Derived (unvalidated) nd costs for kappa3(S) 10%: L=64
    ~0.5k core-h, L=96 ~3.6k, L=128 ~13k.
  - ~20:30 ND GATES VERIFIED BY LEAD (worker ran them 19:01-19:10 but did not report; nd/lead_check.sh, nd/lead_n3.py):
    N2 L=3 ED, points A-D, 8 chains/arm, two nd configs (g1_prod dv4096 leaf64; g1_sparse dv0 leaf4): correct arms max z
    3.03 / 3.13 (both D gsw-only), broken arm z 129-1292. Follow-up D with 32 fresh chains per config (nd/lead_D32):
    max z 1.01 (prod), 0.98 (sparse), 1.12 (dense) => the 3.0 was a fluctuation. N3 nd vs dense L=8 (nd0) and L=16 (nd0,
    nd4096), 4 chains, 2e4 steps: max |z| 1.39 over 4 observables. => ND solver exact; production use pending the merged
    build (sector observables + nd), restart test and L=48 checkpoint compatibility (worker gsw-nd, build_prod2).
  - Merged build verified by lead (worm/gsw-nd 7b37b7c = 5a3551c nd + 95a1731 sector; restart_nd/): nd restart bit-identical
    at L=8 (all 9 obs, prod and sparse settings); a COPY of dense L=48 checkpoint c1 (56 samples) restarted with nd runs
    and appends 20 physical samples (plaquette 0.902, star 0.900; compat/lead_check.py); ED point A max z 0.87.
    N4 table also: L=32 nd 0.15 s/step vs dense 1.32 (same node) => 9x.
  - 2026-10-13 05:07 SWITCH TO ND: scancel own arrays 17159255/6 (dense; ran 17:00-05:07); all 128 ckpt.bin read cleanly
    (ckpt writes are atomic tmp+rename) and backed up as ckpt_dense_final.bin. Dense-phase results:
    L=32 3.61e6 samples (64 chains, ~56k each): kappa3(S) -4.144e-8(0.101e-8) = 2.4% (3% target already met);
    kappa2(delta) 9.304e-5(0.022e-5); <S> 1.805890(12); +1 vs -1 starts: <S> z -1.5, kappa3 z 2.2 (watch).
    gamma1(32) = 0.085, tau_eff ~2.2 steps (better than the 4.6 assumed).
    L=48 77k samples (~1.2k/chain, naples nodes ~29 s/step dense): kappa3(S) -3.6e-9(1.4e-9) (40%), kappa2(S) 1.85e-5,
    gamma1(48) ~0.045 (+-40%; extrapolation said 0.062).
  - Resubmitted on nd (launch_nd.sbatch, bin/paratoric_7b37b7c sha256 fff82892...edf3, dv4096 dr0 leaf64, restart from
    ckpt): L=32 job 17167263 NS 94000 (to ~1.9%, ~100 core-h more); L=48 job 17167264 NS 70000 (target ~5%: 4.5e6
    samples, ~0.9k core-h); NEW L=64 job 17167265 NS 31000 x 64 (2.0e6 steps, target ~10% if gamma1(64)~0.04, ~1-1.2k
    core-h, ~13-19 h/chain). Watcher watch_nd.sh PID 1775706 -> WATCHER_VERDICT_nd.txt. c0 L=32 restart confirmed from
    provenance.txt. Core-h so far ~0.8k (dense phase); projected total ~3k of 8.6k ceiling => L=96 (10% ~3.6k derived)
    fits: PI's call at check-in.
  - 05:30 PI: "run them all and better parallelise to save walltime". Done by more independent chains (thermalisation
    300 steps/chain = 3-4% overhead): L=48 +128 chains (job 17167643, c64-191, NS 23000), L=64 +192 chains (17167644,
    c64-255, NS 8000), L=96 256 chains (17167975, NS 9000, --mem 5G; timing96/: 15 steps in 110 s incl. setup => <= 7.4
    s/step, 2.3 GB, th-cl-rome02n3). Watcher watch_nd2.sh PID 1785070 -> WATCHER_VERDICT_nd2.txt. ~670 chains running at
    05:35. Stopping rule: analyse checkpoints (interim.js); when a size reaches its target (L=48 ~5%, L=64 ~10%, L=96
    ~10%) cancel its remaining chains (data are in ckpt series). Projected core-h total ~7.9k of 8.6k (L=96 ~4.9k at
    full NS). ETA: L=32 ~08 UTC, L=48/L=64 ~10-11 UTC Tue, L=96 Wed 05-15 UTC.
  - 05:40 mcp-lead: approval was <= 8k core-h; no more chains. Max at current NS ~9.3-11.3k => CAPS (binding 8.0k):
    spent 0.8 + L32 0.14 + L48 <= 0.45 (back to 10%, not 5%) + L64 <= 1.1 + L96 <= 5.5k. Stop each size at target by
    cancelling remaining chains. L=96 skewness from interim at ~09-10 UTC decides (10% needs 4.5k if skew 0.031, ~17k if
    0.015). L=32 split by start (dense phase): kappa3 +1 -3.927(0.13)e-8 vs -1 -4.361(0.15)e-8, z 2.20; sector obs
    split |z| <= 1.82 (no trapping signal). Report split again at final.
  - mcp-lead accepts caps (8.0k total; L=48 and L=64 stop at 10%). Wants the L=96 projection at 09-10 UTC with skewness
    and its error. Keep every ckpt.bin of stopped chains (he recomputes from them).
  - 05:40 L=32 nd speed: c0 17.6k steps in 30.5 min = 0.104 s/step (rome05n2), 6.7x the dense production rate.
  - ~06:00 mcp-lead requests s-scan (says PI approved +3k core-h; NOT yet confirmed by PI in session => not submitted):
    self-dual line, s = 0.3361/0.3376/0.3406/0.3421 at L=24/32/48 + L=24 at s*, 32 chains/point, stop at Binder
    kappa4(delta)/kappa2(delta)^2 error <= 1% or caps L24 50 / L32 100 / L48 250 core-h (L=48 low priority).
    Launcher /scratch/a/A.Otaifi/gsw_scan_20261013/launch_scan.sbatch (walltime = cap/32, ckpt every 10 min, --nice for
    L=48), sent for his review. Measured: Binder ratio(delta) at L=32 s* = +0.253(0.0075) (2.95% from 3.25e6 samples) =>
    1% needs ~2.8e7 samples/point (0.28k / 0.79k / ~5.5k core-h at L=24/32/48); at caps ~2.4 / 2.8 / 4.7% (derived).
    Dry run: start Oct 19 (queue full with own production).

## Update 2026-10-12 (sector observables, worker gsw-sector)
- build2 (/scratch/a/A.Otaifi/gsw_cpp_20261011/build2/paratoric, gsw-cpp 95a1731, local only): star_parity, wilson_x/y,
  winding_x/y, --random_start. S2 random start vs ED L=3 open re-run by lead: max z 1.91 (pass). S1 L=4 periodic: gsw arms
  agree (max z 1.68), broken arm fails (z 143-2133); gSW flips sector parities 36-82x more per sample than local-only;
  local-only wilson_x chain means 0.001-0.66 vs 0.1835(6) (trapped). winding_x == star parity in columns 0,1 (identity,
  0 violations). build2 reproduces build/ chain at L=32, cost +2-3.5%. Use build2 for M3 long chains.
- GATE (3) L=32 (job 17155547, gate3_L32/analyse.py, srun 17159237; 8 chains x 12000 steps, 10% discard, scatter errors):
  kappa3(S) -3.67e-8(0.87e-8) vs L32_size -4.92e-8(1.37e-8) 0.8 sigma; kappa2(delta) 9.19e-5(0.12e-5) vs 9.59e-5(0.46e-5)
  0.8 sigma; <S> 1.805886(64) vs 1.80542 (ref error unknown; single 129-rung ladder, start=1): 7 of our sigma, OPEN.
  [SUPERSEDED by the lead's entry above: the MCP lead gives his <S> error >= 3e-4 => <= 1.5 sigma; gate passed. Cost is
  24.3 physical core-h (sacct counts 2 threads/core). The MCP lead is session "mcp-lead"; the finals were delivered.]
  +1/-1 starts agree 0.9 sigma, halves agree. Cost 51 core-h. Message to MCP lead not delivered (a-otaifi-76 gone).
