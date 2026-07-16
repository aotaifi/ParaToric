# Multicritical Campaign Tracker — current-state card

Updated: 2026-07-15 (rewrite this card in place; durable results -> plan/report).

## Status
- Fixed-mechanism PT-based Linsel D4 rerun submitted 2026-07-15 from the reused
  `/scratch/a/A.Otaifi/paratoric_pt_linsel_benchmark_20260713/submit_pt_linsel.sbatch`,
  with isolated outputs under
  `/scratch/a/A.Otaifi/paratoric_pt_linsel_benchmark_fixed_20260715/`: L20=`15101895`,
  L24=`15101896`, L28=`15101897`. The job-local MPI build is pinned to current
  main commit `331e37d4cc6f8c82f030fe568d17eb79db9488c5`, which descends from gate-clear
  `7305dde` and contains both PT fixes `0289b6c` and `0e51978`. Each job uses one
  node, 15 ranks, 1 CPU/rank, 32G, and a 3h cap. Design is unchanged for controlled
  comparison: beta=L, mu=J=1, z basis, fixed lmbda=0.3, h=0.150:0.025:0.500,
  swap period 2 sweeps, and 500k/720k/980k thermalization sweeps for L=20/24/28,
  followed by 100k samples every 2 sweeps. Expected execution times remain roughly
  0.5/1.0/1.5h after start (3h hard cap). The one permitted `squeue --start` check
  per job returned no rows; do not poll. Fresh dispatch performs Part-F PT
  diagnostics plus rung-reorganized Binder-cumulant crossing readout.
- Swap-partner-fix integration validation submitted 2026-07-13 from
  `/scratch/a/A.Otaifi/paratoric_pt_integration_20260713/`, using a clean
  job-local portable MPI build of fix commit `0289b6c`: N=4 PT S1b/S3/S4-on/
  S5/S6 job `15052942` (4 tasks, 4G, 30m), N=8 PT S3/S5/S6 job `15052943`
  (8 tasks, 6G, 30m), and four independent N=4-rung PT-off S4 references job
  `15052944` (4 tasks, 4G, 30m). N=4 uses the specified spread ladder
  {0.22,0.26,0.30,0.34}; N=8 uses the modestly widened uniform ladder
  {0.20,0.222857,0.245714,0.268571,0.291429,0.314286,0.337143,0.36}, preserving
  overlap with finer 0.02286 spacing while testing a wider span. Each PT job
  runs 4k thermalization + 16k recorded sweeps, swap period 2 sweeps, 8k aligned
  energy/sigma_z/ladder samples: 10k total and 8k recorded swap phases. The
  campaign-local independent S6 checker is staged beside `READOUT.md`; canonical
  readout remains `scripts/pt_diagnostics.py` plus rung-reorganized
  `scripts/ensemble_diagnostics.py`. The one allowed squeue snapshot hit a
  transient Slurm-controller DNS failure; subsequent direct `sbatch` calls
  returned the three IDs. Concise-alert submissions also failed on transient
  controller DNS and were not retried. Do not poll; fresh dispatch performs the
  complete S1b/S3-S6 readout.
- PT-based Linsel D4 diagnostic jobs submitted 2026-07-13 from
  `/scratch/a/A.Otaifi/paratoric_pt_linsel_benchmark_20260713/submit_pt_linsel.sbatch`:
  L20=`15041840`, L24=`15041841`, L28=`15041842`. Each is one node, 15 MPI
  ranks/tasks, 1 CPU/rank, 6G, 3h, using the job-14982619-proven Spack OpenMPI
  5/GCC 13 toolchain and `srun --mpi=pmix`; source commit `771ada3` includes D4
  commit `e42914d`. All sizes use beta=L, mu=J=1, z basis, fixed lmbda=0.3,
  evenly spaced h={0.150,0.175,...,0.500}, and swap period 2 sweeps.
  First-pass thermalization is 500k/720k/980k sweeps for L=20/24/28 (scaled
  approximately as L^2), followed by 100k full-series samples every 2 sweeps.
  Outputs retain percolation/plaquette strength and probability, energy,
  sigma_z, largest_cluster, per-link swap counts, and aligned
  ladder_index_by_sample. Fresh dispatch must aggregate observables by ladder
  index before `ensemble_diagnostics.py` Binder/tau readout, then run
  `pt_diagnostics.py`; do not treat per-process mixed-h summaries as rung
  marginals. Expected runtimes after scheduling are roughly 0.5/1.0/1.5h
  (hard cap 3h); dry-run predicted a priority-delayed start near 2026-07-17
  06:49 CEST. The single combined squeue snapshot and concise alerts failed on
  transient Slurm-controller DNS and were not retried; primary submissions had
  already returned the job IDs above.
- Decisive L32,h=0.335 thermalization array `15039919` submitted 2026-07-12
  using `scripts/submit_thermalization_decisive_l32_20260712.sbatch`: 8
  independent one-core tasks (4 default_spin=+1 cold, 4 default_spin=-1 hot),
  beta=32, lmbda=0.3, mu=J=1, z basis. Each trajectory thermalizes continuously
  to 2M sweeps then records plaquette POP/POS and sigma_z every 25 sweeps through
  8M, retaining 300-sample windows at 2M/4M/8M for a fresh-dispatch
  `scripts/ensemble_diagnostics.py` I5 readout. Per task: one node, 1 CPU, 512M,
  8h. Exact-point 2M runs took 3479-3492s, so linear 8M estimate is about 3h53m
  and the cap gives >2x margin. The int64-safe portable binary includes commit
  `af0a3c2`; maximum N_thermalization is 4,096,000,000 moves and total trajectory
  depth is 16,384,000,000 moves, both int64-safe. One permitted squeue snapshot
  showed all 8 tasks running. Concise-alert submission failed on transient Slurm
  controller DNS and was not retried. Do not poll; fresh dispatch performs readout.
- Fixed-pipeline Linsel rerun array `15039466` submitted 2026-07-12 using
  `scripts/submit_linsel_percolation_benchmark_20260712.sbatch`: 54 one-core
  tasks = L={16,24,32} x h={0.15,0.25,0.30,0.325,0.335,0.345,0.37,0.42,0.50}
  x 2 cold/hot starts. Per-task cap 2h30 and 512M; maximum allocation 135
  core-hours, expected use dominated by L=24's 2M-sweep thermalization (cited
  worst 87m). One permitted squeue snapshot showed all tasks pending. Fresh
  dispatch must read out plaquette POP/POS and cold/hot agreement; do not poll.
- Sampling-window thermalization convergence array `15038796` submitted
  2026-07-12 using `scripts/submit_thermalization_convergence_20260712.sbatch`:
  54 independent production-CLI tasks over L={16,24,32}, h={0.15,0.335,0.50},
  default_spin={+1,-1}, and depths={100k,500k,2M} sweeps. Each records 100
  samples at 25-sweep spacing for plaquette POP/POS and sigma_z (base -1-edge
  density=(1-sigma_z)/2). Resources are one node/task, 1 CPU, 512M, 2h; prior
  worst 500k segment was 15m27s, implying about 62m at 2M. One squeue snapshot
  showed the array running; concise-alert submission failed transient Slurm DNS
  resolution and was not retried. Fresh dispatch must perform readout.
- Phase: M0 gates G0-G1.3 PASS but "M0 complete" still premature. The sector-rate
  scan v2 (`15034719`) came back with a NULL result (percolation_probability/
  percolation_strength exactly 0.0 across the entire tested s in [0.10,0.50]) —
  reported to Feynman, who traced it to two issues and issued a MASTER PRE-PILOT
  SEQUENCE (see doc/feynman_thesis_tojarvis.md "MASTER PRE-PILOT SEQUENCE" +
  doc/pt_architecture.md), superseding the earlier "finalized next steps". PI
  authorized executing the full sequence 2026-07-12.
- **Separately-discovered blocking bug (D1)**: `swap_period` is defined in raw
  Metropolis moves, not sweeps. Default 1000 moves = ~7.8 sweeps at L=8 (healthy —
  why G1.2/G1.3's L=8 round-trip validation looked fine) but ~0.035 sweeps at
  L=120 (~29 swaps per sweep) — replicas barely evolve between swaps, so
  round-trips there would be label-travel, not real decorrelation. No PT run at
  L>8 is valid until this is fixed. The L=8 G1.2/G1.3 PASS itself is not wrong,
  just does not transfer to L=120 as previously assumed.
- Sequence (full detail in status.md): D1 swap_period-in-sweeps fix (BLOCKING,
  do first) -> validate percolation observable vs Linsel arXiv:2401.08770
  benchmark (no PT, small L, off self-dual line) -> confirm deconfined reservoir
  at small s on the self-dual line -> locate s* (wide-s scan, both directions,
  no pre-committed direction) -> D2 offline PT diagnostics analyzer (strict
  round-trips, tau_ergodic in sweeps, up/down flow) -> D4 generalize PT to
  independent (h,lmbda), validate against Linsel's known Ising line WITH PT ->
  D3 adaptive PM ladder builder (ported from code_ruby_duy_pinning_lab,
  PI-authorized cross-project reference) -> THEN stage the G2 pilot (PI-gated,
  harness hard stop regardless of outcome).
- BLOCKER for M2 (not the pilot): PT+checkpoint composition must be implemented
  before any production ladder — one L=192 point at I4 is 46-91 days/chain,
  impossible without resume.
- Standing rule (all future PT run reports): must include swap_period in
  sweeps, per-link edge acceptance (flag any dead link <1e-4), strict
  round-trips (visited_both_ranks), up/down flow per replica, tau_ergodic in
  sweeps, tau(percolation_strength) at the s* rung. Incomplete reports don't
  gate decisions.
- RUN MANUAL (mandatory before designing/judging any run): doc/paratoric_run_manual.md
  — parameter picking + post-run validation checklist + pitfall catalog. Golden
  rules: (G1) thermalization judged by cold/hot (I5) NOT within-chain Neff;
  (G2) thermalization depth non-decreasing in L at fixed critical h; (G3) >=4
  chains; (G4) Neff of the worst observable not raw N; (G5) calibrate observables
  on known static configs first. Added to AGENTS.md bootstrap order.
- Next action: fresh-dispatch readout of jobs `15052942`-`15052944`; no D4, D3,
  pilot, or other PT work until S1b/S3-S6 are judged against section 7b.

## Gate log (append one line per gate decision)
| date | gate | evidence | result | decision |
|---|---|---|---|---|
| 2026-07-07 | pre | code review (chat) + performance_report.md | — | plan approved by user |
| 2026-07-12 | D2/D3 done (parallel worktrees) | Jones (D2, `scripts/pt_diagnostics.py`) + James (D3, `scripts/adaptive_ladder_pm.py`), isolated git worktrees, merged commits `6aeee94`/`d158822`/`9a780ab` | DONE | D2: strict round-trip count on real job 14982619 data (6) is LOWER than the simple endpoint-touch count (10=6+4) - confirms H2's concern that touches overcount genuine round-trips; tau_ergodic strict mean 3.25 sweeps. D3: PM ladder builder ported from code_ruby_duy_pinning_lab, all 4 robustness guards tested; honestly reports PM alone does NOT solve a synthetic bottleneck case (matches H5's warning). |
| 2026-07-13 | D4 redo: PT VALIDATED at N>2 (fix works) but ladder UNDER-RESOURCED; PM cannot fix it | Jarvis jobs 15101895/96/97 + Feynman derivation — see doc/pt_architecture.md sec 8 + feynman_thesis_tojarvis.md 13c | REVISED ASSESSMENT | GOOD: swap-partner fix landed, S0-S6 PASS at N=4/N=8, bug fingerprint GONE (smooth monotonic acceptance) — PT validated at N>2 for the first time. BAD: uniform dh=0.025 => L20 1.9%->0.014%, L24 0.24%->0.0066%, L28 ALL 14/14 dead, 0 strict RT at every L. Jarvis's diagnosis (ladder-design, not mechanism) CORRECT; his fix (run D3/PM) WRONG: PM conserves rung-count AND span (adaptive_ladder_pm.py L81 + rescale; guard L87), so it only EQUALIZES — it CANNOT raise mean acceptance; all-dead => equally-dead. PM is a fine-tuner, never a rescuer; NEVER run on dead links. LAW: sigma~dh*L^(3/2) (Var E~V=L^3 at beta=L) => N~L^(3/2); and N is NOT free: tau_RT~N^2/A minimized at A~23% (= the 20-40% rule), overshoot x4 => ~5x slower. ACTION: (1) redo benchmark resourced N~40/50/60 (L=20/24/28), non-uniform denser near h_c, 1-2 nodes; (2) THEN PM; (3) calibrate dh_good(L) via acceptance-vs-dh scan at L20/28 (~1h); (4) resource check max ranks/node + internode (N_opt exceeds 1 node at L=120); (5) DEFER L=120 pilot — walls tau_RT~L^3 sweeps, wallclock/RT~L^6; needs N~150-200 => weeks; levers are Range(^2) and L(^6) or better algorithm, NOT ranks = campaign-level decision. Feynman note: earlier pilot budget sized cores from walltime, never from acceptance-driven rung count — that was the miss. |
| 2026-07-13 | CRITICAL PT swap-partner bug (by RANK not ladder-position) — mechanism broken at N>2 | Jarvis 15-rank D4 NO-GO + Feynman code verify (qmc.hpp:3980 vs coupled-pinning worm.update.cpp) — see doc/pt_architecture.md sec 7 + feynman_thesis_tojarvis.md 13b | CONFIRMED BUG / GATE | partner=pt_rank+/-1 (by RANK) but labels MIGRATE on accept => adjacent ranks hold non-adjacent ladder positions => odd-phase swaps exchange extreme-low/high labels (huge gap, rejected) => ladder SEVERS => 0/15 round-trips, crossing links <1e-3, isolated spikes. Invisible at 2 ranks. RETRACTS G1.2/G1.3 6-rank round-trips (job 14982619 spurious); PT never validated at N>2. Swap-WEIGHT math OK. FIX=port coupled-pinning: MPI_Allgather ladder_index -> rank_of_ladder_index (abort on dup/invalid) -> pair by POSITION -> partner=rank_of_ladder_index[i+/-1]. Smoke suite S1-S6 (N>=3, SPREAD ladder). GATE: fix+S1-S6+re-run G1.2/G1.3 at N>=3 before ANY further PT. Jarvis correctly stopped+read code+did not unilaterally redesign. |
| 2026-07-13 | L32 decisive test: cold/hot AGREE but COMMON DRIFT — Jarvis improves G1 | Jarvis job 15039919 (8 chains, 2M/4M/8M R-hat PASS) + Feynman read — see doc/feynman_thesis_tojarvis.md "UPDATE 2026-07-13a" | REFINE + CHEAP CHECK | Cold/hot R-hat~1.00 at all depths BUT both groups drift together: plaquette POP 0.699->0.717->0.734, strength 0.427->0.449, density FLAT ~0.409. Jarvis correct: cold/hot & R-hat measure between-chain dispersion, BLIND to common drift => thermalization needs (a) cold/hot agree AND (b) observable STATIONARY vs depth. G1 fixed in run_manual. Drift is real (steps non-decelerating = algebraic/critical-slowing, NOT near plateau); density-flat+POP-drifting = GLOBAL winding structure is the slow mode (= campaign raison d'etre). DECISIVE CHEAP CHECK (before 16M/32M): compute BINDER cumulant of plaquette strength at L32,h=0.335 from EXISTING 2M/4M/8M data. Binder stationary while POP drifts => use Binder crossing (Linsel method), 8M viable, proceed. Binder also drifts => raw obs critically-slow without PT (= evidence campaign needs PT). Free, decides if 32M needed. |
| 2026-07-12 | naive-error hypothesis REFUTED; real pitfall = L32 UNDER-THERMALIZED at h_c | Feynman (owning refutation) + Jarvis audit — see doc/feynman_thesis_tojarvis.md "UPDATE 2026-07-12i" | HYPOTHESIS (verify) | Jarvis correct: _error is stationary bootstrap not naive stderr; my 12h mechanism RETRACTED (2nd refuted Feynman hypothesis). REAL PITFALL (from depth table): L16=100k, L24=2M, L32=100k is UNPHYSICAL — critical slowing (tau~L^z) means L32 at h_c needs MORE than L24's 2M, not 20x less => L32 ~35x under-thermalized; the drifting L24/L32=0.361 crossing uses that bad data. High within-chain Neff MISSED it: Neff = decorrelation within a basin, NOT convergence to equilibrium; cold!=hot (I5) is the correct alarm and IS firing in the critical region. Depth-setting scan used only 2 chains (too weak). CORRECTS 12h "don't sample more" — we DO need longer thermalization + >=4 chains. DECISIVE TEST: L32,h=0.335, >=4-8 chains, ramp therm 2M->4M->8M, judge by cold/hot (I5) not Neff. Converge=>under-therm confirmed. Still disagree at 8M/8chains=>examine observable. STANDING: thermalization depth must be non-decreasing in L at fixed critical h. |
| 2026-07-12 | Linsel benchmark "INCONCLUSIVE" = ERROR-BAR artifact (I4/I6 violation), NOT physics failure | Feynman teardown of Jarvis job 15039466 — see doc/feynman_thesis_tojarvis.md "UPDATE 2026-07-12h" | DIAGNOSIS / DO-NOT-FAIL-IT | PROOF it's the estimator: same point L24,h=0.335 gave 0.83 sigma @100 samples but 3.55 sigma @1000 samples — MORE samples => MORE sigma = definitive underestimated-error fingerprint (naive stderr ~1/sqrt(N) on CORRELATED samples; 3.55/0.83=4.3~sqrt(10)=3.2). Runs used FIXED 25-sweep spacing, no tau, no Neff = I4/I6 violation => ALL sigma claims (crossings, cold/hot) untrustworthy. BAR MISCALIBRATED: rejecting for "no credible nu=0.65" is unfair — Linsel got nu=0.65(27) (40% err) at L=10-30; job = confirm crossing at h_c~0.335 + Ising-COMPATIBLE, not nail nu. Crossings 0.336/0.343/0.363 ARE in-ballpark. METHOD: Linsel used strength-BINDER crossing, Jarvis used raw POP=0.5 (crossing value not universally 0.5). DIRECTIVE: do NOT run more sampling (made it look worse); (1) fix estimator — tau_int, blocked/bootstrap, Neff; (2) strength-Binder crossing; (3) recalibrate bar; (4) re-judge — crossing ~0.335 w/ honest errors => PASS => proceed to D4. Make the analyzer autocorrelation-aware by default (2nd fixed-window misleading-sigma this session). |
| 2026-07-12 | Canonical phase-diagram sketch + s* units reconciliation (Tupitsyn classical vs Linsel quantum) | Feynman/PI, Tupitsyn PRB 82 085114 (arXiv:0804.3175) + Linsel arXiv:2401.08770 — see doc/multicritical_universality_plan.md "Phase-diagram sketch (reconciled)" | RECONCILED | 3-liner: (1) OFF self-dual line = two dual 2nd-order 3D-Ising lines bound deconfined phase; (2) ON self-dual line (h=lmbda) the Ising lines meet & transition becomes 1st-ORDER (bimodal energy, Tupitsyn/BPV) = the MCP; (3) MCP nature (1st-order vs XY-BPV vs new-class-SSN nu~0.67) = the project's core, decided by FSS on the self-dual s-line. UNITS TRAP: Tupitsyn/BPV MCP is CLASSICAL gauge-Higgs (lambda_bond*≈0.226, lambda_pl*≈0.752); Linsel = QUANTUM (our model) puts self-dual MCP s*≈0.33-0.35 (Fig 3; left precise value as future work = our target). OUR s*≈0.33-0.35 (measure it), NOT 0.226 (classical) and NOT my earlier thrashed 0.31/[0.15,0.25]/[0.22,0.30]. Reservoir just below s* ~[0.28,0.34]. |
| 2026-07-12 | D4 scope confirmed: production = self-dual s-line, D4 is benchmark-only + reservoir placement | Feynman/PI review — see doc/pt_architecture.md "5b" | CONFIRMED | Production PT = SINGLE param s=h=lmbda (mu=J=1), 1D ladder along self-dual line crossing the MCP. D4's decoupled (h,lmbda) is BENCHMARK-ONLY (Linsel Ising correctness check), does NOT change production (2D production ladder impractical to tune). Low-s on the self-dual line = deconfined toric phase, no transition (correct). NUANCE: benign phase != good reservoir — winding sector is EXACTLY conserved at s=0, MORE frozen deeper in deconfined, mixes FASTEST near/just-below s*; so concentrate rungs near s*, don't reach to s~0.05; verify reservoir mixes (tau(percolation) at low-s rungs). CORRECTED: s* NOT yet measured (Linsel 0.335 is an Ising-line point at lmbda=0.3, OFF-diagonal, NOT the self-dual MCP); rough interpolation of dual Ising points -> self-dual s* ~ 0.31-0.32 (placeholder, measure it). So useful reservoir "just below s*" ~ [0.22,0.30], NOT the earlier too-low [0.15,0.25]; s=0.22 (known to mix) is the LOW end. First-order H5 bottleneck still applies at the central rung. |
| 2026-07-12 | strength "normalization bug" = definition difference, NOT a bug; e/m duality CONFIRMED via probability | Feynman read of Jarvis 4-channel test (`/scratch/.../paratoric_angle_b_dualpair_s022_20260712/`) + code (lattice.cpp:3729,3782) — see doc/feynman_thesis_tojarvis.md "UPDATE 2026-07-12g" | RESOLVED / PROCEED | PROBABILITY dual pairs MATCH (z-perc vs x-plaq 1.37 sigma; z-plaq vs x-perc 0.66 sigma) => pairing correct, detection dual-consistent, e/m duality CONFIRMED; use PROBABILITY for I2/duality; e/m sub-step UNBLOCKED. STRENGTH mismatch is NORMALIZATION not a bug: percolation_strength=(#edges)/2L^2 vs plaquette_percolation_strength=(#plaquettes)/L^2. Proof definitional: BOTH pairs off by the SAME basis-independent ~1.80 (0.142/0.079, 0.548/0.305). EXPONENT-SAFE (constant prefactor cancels in beta/nu). DIRECTIVE: do NOT force strengths equal / do NOT rush a fix; use probability for duality, each strength for its OWN FSS only, document the ~1.80. Optional: check L-independence of the factor. |
| 2026-07-12 | cross-basis "wiring issue" = WRONG-PAIR comparison (duality swaps vertices<->plaquettes too) | Feynman read of Jarvis convergence study (s=0.22, cold/hot both bases to 1M) — see doc/feynman_thesis_tojarvis.md "UPDATE 2026-07-12f" | DIAGNOSIS | Convergence study DECISIVE: within-basis cold/hot converge (z gap .010, x gap .004) => under-therm AND sector-trapping BOTH ruled out; cross-basis z-perc~.29 vs x-perc~.75 flat = wiring not physics. ROOT CAUSE (Feynman's under-specified instruction): em-duality swaps sigma_z<->sigma_x AND lattice direct<->dual (VERTICES<->PLAQUETTES). Jarvis compared same-graph channels (z-perc vertices vs x-perc vertices) — NOT a dual pair. CORRECT pairs: z-percolation_* == x-plaquette_percolation_* (E<->M') and z-plaquette_* == x-percolation_* (E'<->M). E(.29) was compared to M(.75) but E pairs with M' => gap EXPECTED, no bug. CONFIRM (cheap, s=0.22 already equilibrated): measure all 4 channels, check z-perc==x-plaq AND z-plaq==x-perc. |
| 2026-07-12 | Correct dual-pair confirmatory test at s=0.22 | 4 independent seeds/basis, L=8, beta=8, 1M thermalization sweeps + 5000 one-sweep-spaced samples; `/scratch/a/A.Otaifi/paratoric_angle_b_dualpair_s022_20260712/` | **FAIL** | Probability components match: z-perc 0.2863(32) vs x-plaq 0.2938(44), 1.37 sigma; z-plaq 0.7451(32) vs x-perc 0.7420(33), 0.66 sigma. Strength components do not: z-perc 0.07893(93) vs x-plaq 0.14200(207), 27.8 sigma; z-plaq 0.54831(267) vs x-perc 0.30455(146), 80.0 sigma. Per Feynman's decision rule, correct pairs still disagree, so deeper mapping/observable audit is required; no broader physics conclusion. |
| 2026-07-12 | x-basis e-vs-m check NOT controlled for thermalization | Feynman read of Jarvis x-basis RESULT (L=8 z-e vs x-m table, 11-32sigma) — see doc/feynman_thesis_tojarvis.md "UPDATE 2026-07-12e" | HOLD | Do NOT read the 11-32sigma z-e vs x-m disagreement as a duality failure: uncontrolled for equilibration. No cold/hot convergence check for the main table (only s=0.50 probed, NOT converged at 50k sweeps). Two bases thermalize at different rates (Linsel: x-basis less efficient near self-dual); z-e==x-m is an equilibrium (I2-adjacent) statement. RED FLAG: s=0.50 not converged at 50k sweeps on L=8 (tiny) smells like WINDING-SECTOR TRAPPING (never converges, needs PT/worm) not slow therm (converges). DECISIVE: cold/hot convergence vs sweeps, BOTH bases, fixed self-dual s=0.22 — converge-to-equal => duality OK (under-therm); plateau-apart => mapping/wiring OR sector-trapping. e-vs-m sub-step PAUSED on this. Separately: rotate fix (commit f08fd9d) found+fixed a REAL cross-observable same-slice bug (obs eval ORDER determined which tau-slice each obs read) — good catch, regression test added. |
| 2026-07-12 | CORRECTION: both z-channels are electric-sector; e/m needs both bases | Feynman, PI physics catch — see doc/feynman_thesis_tojarvis.md "UPDATE 2026-07-12d" | CORRECTION | Code fact: is_winding_percolating (vertices) and is_winding_plaquette_percolating (plaquettes) BOTH cluster the SAME sigma_z=-1 edges (direct vs dual graph) — two views of the ELECTRIC sector, NOT independent e/m. Topological phase = both e AND m deconfined => both electric-string AND magnetic-string POP positive; m (magnetic/flux/sigma_x-string) lives in the X-BASIS, not measured by either z-channel. So the topological/MCP "both positive" signature needs BOTH bases (z->e, x->m), equal at the self-dual point (I2-adjacent check). Linsel single-POP benchmark unchanged. WITHDRAWN: the p_c=0.5 "planar duality" story for why plaquette lights up at ~40% while electric doesn't (empirical fact stands; mechanism not rigorously derived). VERIFY: run benchmark point in x-basis; check z-e == x-m at h=lmbda. |
| 2026-07-12 | Percolation decisive experiment | Clark, `tests/debug/percolation_decisive_experiment.cpp` commit `579061b`, data `/scratch/a/A.Otaifi/paratoric_percolation_decisive_20260712/percolation_decisive_readout.csv` | **PARTIAL RESOLUTION + NEW PUZZLE** | L=32,h=0.15,lmbda=0.3,beta=32,z-basis, both default_spin, thermalization checkpoints 100/500/2000/10000 sweeps, base-vs-rotated-slice comparison on the SAME config. Base -1-edge density RELAXES for both starts (+1-start: 38->752 of 2048; -1-start: 406->825) and is CONVERGING toward each other by 10000 sweeps - supports (a) under-thermalization, not a frozen/wrong-slice bug, for the DENSITY question. Plaquette channel tracks this relaxation physically (POP turns on as density rises) and rotation clearly matters for plaquette (base vs rotated differ, e.g. +1/10000sweeps: 0,0 -> 1,0.638). **BUT electric channel stays EXACTLY 0/0 in ALL 8 points, both base AND rotated, even at 10000 sweeps with ~37-40% base density where plaquette clearly percolates** - electric has its own separate, unresolved problem beyond thermalization. beta=L confirmed enforced in existing job scripts. Calibration re-confirmed basis-independent (z matches x for all 4 known configs). Reported to Feynman; NOT concluding independently why electric specifically stays null. |
| 2026-07-12 | D1 swap_period-in-sweeps fix | Clark implementation + Martina independent review (2 clean rebuilds, `git archive` extraction to rule out stale-build risk after an earlier false-negative this session), commit `8e54db2` | PASS | New primary CLI `--pt_swap_period_sweeps` (default 2.0), converted via `round(sweeps*2*L^2)`; legacy `--pt_swap_period` raw-move override still available, CLI rejects supplying both. Confirmed used at both thermalization and production swap-trigger gates (not just computed and dropped on one path). New regression test covers L=8/L=120 sweep-size formula, default conversion, legacy-equivalence, and override precedence — Martina-verified meaningful, not tautological. Both Clark's and Martina's clean rebuilds: ctest 3/3. Also restored `src/logging/trivial_log.hpp` (Boost.Log/clang++-20 compatibility shim) — an EARLIER session verdict that this shim was "not needed in the live repo" is now understood to have been a stale-incremental-build false negative; confirmed via a from-scratch `git archive` extraction of the pre-D1 commit that `boost/log` directly triggers a real, reproducible clang20+Boost-1.75-MPL incompatibility unrelated to any of today's changes. Self-review entry logged in `ai-workflow/jarvis_selfreview.md`. No PT run at L>8 was valid before this fix (G1.2/G1.3's L=8 "6/4 round-trips" result stands as-is, it just never transferred to L=120 as previously assumed). |
| 2026-07-12 | Step 1 Linsel POP benchmark | James/Codex Slurm arrays `15037070`, `15037083`, long repeats `15037095`/`15037096`; launcher `scripts/submit_linsel_percolation_benchmark_20260712.sbatch`; outputs `/scratch/a/A.Otaifi/paratoric_linsel_percolation_benchmark_20260712*` | **INCONCLUSIVE** | Setup: z-basis, Boost, periodic square, beta=L, mu=J=1, lmbda=0.3, h={0.15,0.25,0.30,0.335,0.37,0.42,0.50}, L={16,24,32}, both `percolation_*` and `plaquette_percolation_*`, no PT. Named electric `percolation_*` is not Linsel POP here: at L32 it is 0 for h<=0.37 in long repeats and only ~0.002 by h=0.42/0.50. Plaquette channel is the only plausible candidate (small-h POP clearly >0 in long spinminus repeat: L32 h=0.15 pPOP=0.970, pPOS=0.600), but it does NOT reproduce a clean h_c≈0.335 crossing and shows start/thermalization sensitivity (spinplus_long L32 h=0.15 pPOP=0.132 vs spinminus_long 0.970). Do not proceed to self-dual s* relocation from this gate; resolve initialization/equilibration or parameter/basis convention first. |
| 2026-07-08 | G0.1 baseline reproduction | James dispatch, L=40 beta=80 Boost, seed 24601, `/scratch/a/A.Otaifi/paratoric_g0_20260708/g0_1_baseline/...` | PASS | acceptance 0.246364 vs baseline 0.24658606 (0.09% rel diff); energy -3326.88+/-5.556 vs baseline -3314.6337+/-3.337 (z=1.89, <3) |
| 2026-07-08 | G0.3 existing tests | James dispatch, `ctest --test-dir build-current-clang-portable` | PASS | 3/3 tests passed (test_lattice, test_flat_square_lattice, test_extended_toric_code_qmc) |
| 2026-07-08 | G0.2 fast-math A/B | James dispatch + Martina verification, L=40 beta=40, seeds 24601-24604, ON vs OFF, `/scratch/a/A.Otaifi/paratoric_g0_20260708/g0_2_fastmath_ab/...` | PASS (confirmed) | combined z=0 for all observables (energy, anyon_count, sigma_x, star_x, plaquette_z) between ON/OFF. Verified NOT a build no-op: CMakeCache.txt/compile_commands.json confirm -ffast-math present only in ON build, binaries differ (md5 bcf205f.. vs 57f01ae.., cmp differs). Fast-math genuinely has no measurable effect on these observables at this scale -> production can keep FAST_MATH ON |
| 2026-07-08 | cross-project scan | Jones dispatch on /project/theorie/a/A.Otaifi/paratoric_tampered (separate repo, gitlab A.Otaifi/paratoric_tampered) | informational | real MPI PT code exists there (src/mcmc/parallel_tempering.hpp, odd/even MPI_Sendrecv swaps, z-basis kink-count + integrated-energy swap weights) but: (a) does not meet current G1.1-G1.3 harness standards (partial validation only, some OOM/failures), (b) checkpoint/restart only exists in a separate dirty worktree with PT-resume explicitly blocked, (c) current ParaToric is 48 commits past the shared base with templated Boost/flat_square structure the tampered code doesn't have. Verdict: port PT DESIGN (swap formula, MPI pattern) as reference, do not merge files; implement fresh against current structures, checkpoint first per plan order |
| 2026-07-08 | G1.1 checkpoint/restart | Clark implementation + Martina review, L=8 z-basis and x-basis self-dual check, checkpoint@sample10 resume-to-20, commit e461686 | PASS | bit-faithful restart, h5diff exact match on energy/string_number/anyon_count/production_acceptance in both bases. Martina-confirmed: RNG state is full-engine not seed-only, event-vector sort invariant enforced (throws, not silently repaired) on load, checkpoint writes only at sample boundaries (no mid-update tear risk), ctest 3/3 no regression. Known limitation (non-blocking): --snapshots spool state not checkpointed; campaign's planned production runs do not use live snapshots. Automated regression test is weaker (in-memory z-basis only) than the manual h5diff validation performed — acceptable for gate purposes, flagged for future hardening |
| 2026-07-08 | G1.2 swap-weight math (partial) | Clark implementation + 2 rounds Martina review, L=8, two seed/ladder configs, commit b29ff1c | **RETRACTED, see correction below** | cached-vs-brute-force swap weight max abs deviation 1.8e-14 / 1.6e-14 (bar <1e-8), across two distinct configs. Ladder correctly restricted to self-dual line s=h=lmbda (mu=J=1 runtime-enforced), swap hot path confirmed O(1) from cache (no event-vector walk). Round 1 review found 2 real bugs (susceptibility normalization used static config h/lmbda instead of per-replica runtime value -> would have silently misnormalized susceptibility observables under PT; MPI_Init with no MPI_Finalize/return-code checks) + 2 minor (pt_ladder CLI had no validation; test coverage was 1 trajectory) — all 4 fixed and re-confirmed by a second Martina pass. REMAINING: real multi-rank mpirun validation (swap acceptance rate, ladder round-trips, PT-on vs PT-off G1.3 comparison) not yet done — mpirun fails to launch in the Codex dev sandbox (opal_ifinit socket() failure, confirmed a sandbox/network restriction not a code bug by 2 independent reviews); OpenMPI 4.1.1 is installed on the actual cluster at /usr/lib64/openmpi (not on default PATH) — needs a real Slurm srun job to complete |
| 2026-07-08 | **CORRECTION**: G1.2/G0.3 false-pass discovered | Jones clean-rebuild bisection (separate scratch build dirs per commit, not incremental), `/scratch/a/A.Otaifi/paratoric_clean_lattice_20260708_jones` | **FAIL — real regression** | `ctest -R test_lattice` clean-built at commit `7394885` (baseline): PASS. Clean-built at `e461686` (checkpoint only): PASS. Clean-built at `b29ff1c` (+ PT): **FAILS** — `integrated_edge_energy_diff_combination_test_1` gets exactly 0 instead of expected 4 / 6.32 after `insert_single_spin_flip`/`insert_tuple_flip`. Every "ctest 3/3 passed" reported by Clark/Martina AFTER b29ff1c in this session's G1.2 round was a FALSE PASS from a stale/incrementally-rebuilt build-current-clang-portable directory that did not pick up the regression. **This invalidates the G1.2 PASS logged above** — the swap-weight cached-vs-bruteforce validation used the same (apparently broken) cached-energy machinery, so that 1e-14 agreement needs re-verification with a clean build before it can be trusted, not assumed self-consistent. Root cause not yet found — Martina's earlier (pre-this-discovery) hypothesis was that `insert_single_spin_flip`/`insert_tuple_flip` don't route through whatever `set_/add_potential_*` path actually updates the cache, but that needs confirming against what specifically changed in b29ff1c, not assumed. NEXT: Clark to find and fix the actual root cause in b29ff1c's lattice.cpp/.hpp changes, re-validate G1.1 (checkpoint still needs re-checking too, out of caution) and G1.2 swap-weight math from a clean build, get Martina to review with an explicit clean-rebuild requirement this time (not incremental). Process gate added: no gate is logged PASS in this tracker on an incremental ctest run again — clean rebuild only, going forward. |
| 2026-07-08 | G1.2 swap-weight math (re-instated) | Clark fix + Martina independent re-verification (both from separate CLEAN scratch rebuilds), commit `12ebdb8` | PASS | Root cause: b29ff1c repointed public `total_integrated_edge/star/plaquette_energy()` to return the cache instead of recomputing; `insert_single_spin_flip`/`insert_tuple_flip` don't update that cache (only the production move path does), so any caller using the raw insert path (incl. the pre-existing test) got a stale/zero value. Fix: those 3 public accessors call `recompute_total_integrated_*()` again (pre-PT behavior); PT's swap hot path is unaffected, it already calls `cached_total_integrated_edge_energy()` explicitly. TWO independent clean (`rm -rf`+from-scratch) rebuilds both give full `ctest 3/3 passed, 0 failed` (Clark's and Martina's, separate scratch dirs). Swap-weight cached-vs-bruteforce re-measured independently twice post-fix: Clark 6.2e-14/8.2e-14, Martina 4.5e-14/0 — all far under the 1e-8 bar. Known non-blocking item: `total_integrated_edge_energy()` is still called from the `--snapshots`-only `sigma_x_tau_avg_obs` path (extended_toric_code_qmc.hpp), now O(events) instead of O(1) again — this campaign's planned production runs don't use live snapshots, so not on the critical path; flagged for future hardening, not fixed now. |
| 2026-07-08 | G1.2 real multi-rank + G1.3 PT-on/off | James submitted Slurm job `14982619` (spack OpenMPI 5.0.0/GCC 13.2.0, `srun --mpi=pmix`, L=8, 6-rank ladder s={0.22..0.32}, PT-off companion at s=0.26), Jones read back HDF5 results, `/scratch/a/A.Otaifi/paratoric_pt_mpi_validation_20260708/` | **PASS** | Job completed cleanly (exit 0, 1m24s, no invariant-check failures in logs). G1.2: swap-weight cached-vs-bruteforce re-confirmed on real hardware (max 3.6e-13). Real multi-rank transport works — rank1 completes 6 full ladder round-trips, rank2 completes 4 (harness bar: >=3; the harness's actual STOP condition is round-trips, not the acceptance-rate window, which is described as a tuning target). Per-link swap acceptance (5 links): 14.5%, 6.5%, 16.2%, 26.9%, 55.4% — uneven vs the 20-40% target, needs retuning before L=120 (already anticipated in the plan: "actual retuning happens later at the L=120 pilot stage", not a gate failure at this L=8 mechanism-validation stage). G1.3: PT-on (s=0.26 replica) vs PT-off companion (s=0.26), combined-z: energy 0.758, sigma_x 0.792, star_x 0.944 — all well under the z<3 bar. Benign autocorrelation warnings (tau > 0.1*N) expected for a short L=8 mechanism-check run, not a converged physics measurement. **G1.2 and G1.3 both PASS — PT is now genuinely, independently validated on real cluster hardware.** MPI toolchain going forward for any MPI-linked build: spack OpenMPI 5.0.0/GCC 13.2.0 (paths recorded in commit history), NOT system /usr/lib64/openmpi. Separately: an unreviewed, uncommitted stray edit appeared in the live repo during this dispatch (Boost.Log includes replaced by a `#define BOOST_LOG_TRIVIAL(level) std::clog` shim in src/cli/paratoric.cpp and extended_toric_code_qmc.hpp) — Martina-reviewed, verdict: not needed for the default clang++-20 production build (which still compiles/passes 3/3 without it), reverted from the live repo; the real fix (if ever needed) belongs at the CMake level for the MPI build variant specifically, not a live source shim. |
| 2026-07-12 | Step-1: calibration DONE (math correct), finalized decisive experiment | Feynman read of Jarvis calibration test (commit 52de6a5) — see doc/feynman_thesis_tojarvis.md "UPDATE 2026-07-12b" | PLAN | Calibration on hand-built static configs: cluster MATH is CORRECT (all-1 -> electric POP=1; winding string hard-asserted PASS; channels are genuine duals). THE GAP: static all-(-1)=1 but dynamic default_spin=-1 run=0 => config at measurement != all-(-1). Fits EQUALLY (a) base RELAXED (under-thermalization, lmbda=0.3 favors +1; PI hypothesis) or (b) measurement wiring. NOT separated — do NOT apply rotate_imag_time yet. FINALIZED EXPERIMENT: one instrumented run, L=32 h=0.15 lmbda=0.3 beta=L, both default_spin +/-1, THERMALIZATION SCAN (100/500/2000/10000 sweeps), log percolation(both channels) + sigma_z=-1 base edge count + percolation on base-vs-rotated-slice. Readout splits (a)under-therm / (b)wiring / genuine-zero decisively. Alongside: verify beta=L in scripts; re-assert calibration in basis='z' (ran in 'x'). D2/D3 parallelized in worktrees (PI-authorized, independent of s*). |
| 2026-07-12 | Step-1 blocker: frozen-base diagnosis REFUTED, calibrate first | Feynman; confirmatory test (electric default_spin=-1, job 15037168, 3 seeds) gave EXACTLY 0 not the predicted ~1 — see doc/feynman_thesis_tojarvis.md "STEP-1 BLOCKER — diagnosis REFUTED" | RETRACTION+PLAN | The rotate_imag_time/frozen-base ROOT CAUSE is NOT established: the prediction rested on an unverified assumption that is_winding_percolating returns true on uniform all-(-1). Do NOT apply the rotate fix yet. Electric = exactly 0 both default_spin (smells structural); plaquette asymmetric 0.970/-1 vs 0.132/+1. NO calibration unit test exists. NEXT (instrument, don't predict): (1) CALIBRATION UNIT TEST on known configs [all+1, all-1, one winding string, dense random] asserting is_winding_percolating/percolation_strength/plaquette returns — do FIRST; (2) verify beta=L in job scripts; (3) time-series + long thermalization, cold/hot default_spin+/-1, record percolation AND -1-edge count vs sweeps; (4) ONLY THEN test rotate_imag_time hypothesis. PI candidates: beta!=L not enforced, and/or 488 sweeps too few for the slowest observable. Lesson: calibrate the instrument on known inputs before physics conclusions. |
| 2026-07-10 | master pre-pilot sequence + standing PT diagnostics rule | Feynman, folded into doc/feynman_thesis_tojarvis.md ("MASTER PRE-PILOT SEQUENCE" + "STANDING RULE") | DIRECTIVE | Ordered pre-pilot path (supersedes earlier next-steps list): 0) D1 swap_period-in-sweeps [blocking] 1) validate observable vs Linsel h_c=0.335/nu=0.65 no-PT 2) confirm deconfined reservoir + measure tau(percolation) at small-s 3) locate s* both directions 4) D2 offline RT/tau_ergodic/flow analyzer 5) D4 generalize (h,lmbda), PT across Linsel Ising line as correctness check 6) D3 PM grid to bracket s* 7) MCP pilot (PI-gated). STANDING RULE for ALL PT runs (James/Jones): report swap_period-in-sweeps, per-link edge acceptance, STRICT round-trips (visited_both_ranks), visited-low/visited-high flow, tau_ergodic (sweeps), and tau(percolation) at s* — decorrelation is everything; round-trips necessary NOT sufficient; a report without these can't judge a gate. |
| 2026-07-10 | PT architecture + ladder-health spec | Feynman, from ParaToric PT code read + cross-project read of code_ruby_duy_pinning_lab PT/grid (PI-authorized) — see doc/pt_architecture.md | SPEC | KEY FINDING: swap_period is in single MOVES not sweeps — default 1000 = ~8 sweeps at L=8 (why validation looked fine) but ~0.035 sweeps at L=120 (~29 swaps/sweep) = LABEL TRAVEL not decorrelation; must be redefined in sweeps and scaled with L² BEFORE any L=120 pilot. ParaToric PT lacks strict round-trips, tau_ergodic (sweeps), up/down flow, and adaptive grid — all of which coupledpinning already has (build_grid PM/LOGR, strict_roundtrip visited_both_ranks, tau_ergodic). Port order: (1) swap_period-in-sweeps [blocking], (2) offline round-trip/tau_ergodic/flow analyzer [pilot prereq], (3) LOGR grid for M2. KEY DIFFERENCE: coupledpinning tempers beta (rescales kink times); ParaToric tempers s with beta fixed (NO rescaling) — port spacing/scheduling/diagnostics only, not the swap payload. Ladder-health invariants H1-H6 in the doc. |
| 2026-07-10 | percolation=0 diagnosis + Linsel benchmark (CORRECTED convention) | Feynman, re Jarvis finding percolation_probability total 0 — see doc/feynman_thesis_tojarvis.md "RESOLVE FIRST" | GUIDANCE | CONVENTION (Linsel arXiv:2401.08770 Fig.3, extended toric code mu=J=1): POP>0 <=> DECONFINED, POP=0 <=> CONFINED (earlier row had this INVERTED — corrected). percolation_probability (binary winding, locate s*) != percolation_strength (largest cluster/#links = order param P, beta/nu). All-zero POP: s* location NOT established (RETRACTED an earlier "s*<0.22 scan down" guess). Linsel Fig.4a (at T=1/L=beta/L, our aspect ratio!): h_c=0.335(2) at lmbda=0.3, nu=0.65 3D Ising, and the self-dual tip sits near s~0.33 — PLAUSIBLY INSIDE [0.22,0.36], which makes total-zero POP look more like an OBSERVABLE/BASIS BUG than a phase-placement issue. RESOLVE ORDER: (1) benchmark observable vs Linsel Fig.4a (reproduce h_c=0.335, nu=0.65 3D Ising, L=10-30, beta=L, BOTH percolation channels to fix basis) — a deconfined-side point giving 0 = real bug. (2) locate s* by wide-s scan BOTH directions (no pre-committed direction). (3) re-place ladder to bracket s*. Do BEFORE sector scan/spacing/pilot. NOTE PT is self-dual-only (code hardwires runtime_lmbda=runtime_h, qmc.hpp:3716) so it cannot ladder the fixed-lmbda Fig.4a benchmark without a generalization. |
| 2026-07-10 | G2 ladder-design + sector-mixing guidance | Feynman answer to jarvis_report_tofeynman.md (energy_lmbda frozen) — see doc/feynman_thesis_tojarvis.md "Answer to Jarvis's G2 ladder-design question" | GUIDANCE | (1) Snapshot energy_lmbda is the frozen net/cut sector, NOT the swap-weight input (which uses cached_total_integrated_edge_energy, thermalizes fine). Design ladder from DIRECT per-link acceptance of a short trial PT run (io.cpp swap_*_by_link), not snapshot variance. (2) The frozen net-sector = the slow topological/winding mode = the G2 GO metric (percolation_strength). Move-set fact: we have star/plaquette (tuple) moves incl. a global-over-[0,beta] tuple flip, but those are CONTRACTIBLE (don't change winding); no non-contractible loop move (R5, out of scope) and no worm, so the sector changes only by slow accumulation. CORRECTED (PI challenge): that accumulation is slow but NON-zero (tau~1e4 means it flips ~once/1e4), so PT is a FAIR bet (textbook cure for coexistence-limited slowing), not a long shot — do not pre-judge NO-GO. Wide-s sector-rate scan reframed: place ladder endpoints to bracket the fast-mixing window, not "is there hope". Pilot GO needs BOTH swap round-trips AND per-replica sector-change + tau(percolation) 5x drop. Fallback if marginal = WORM (polynomial-cost winding change), user decision — NOT the R5 winding-loop move. |
| 2026-07-10 | G2 wide-s sector-rate scan submission | James/Codex, Slurm array `15012336`, launcher `scripts/submit_g2_sector_rate_scan_20260710.sbatch`, output root `/scratch/a/A.Otaifi/paratoric_g2_sector_rate_scan_20260710` | SUBMITTED | L=120 beta=120 z-basis single-chain scan at s={0.10,0.16,0.22,0.28,0.34,0.40,0.46,0.50}; N_thermalization=1000000000, N_samples=10000, N_between_samples=5000000, N_resamples=200; mu=J=1, h=lmbda=s; array 0-7, 1 CPU/task, mem-per-cpu=1G, walltime=20:00:00, partition=cluster. Slurm DNS/connectivity succeeded from danger-full-access Codex (`squeue -u "$USER"` returned jobs), and one post-submit confirmation showed `15012336_[0-7]` pending with reason `(Priority)`. |
| 2026-07-11 | G2 wide-s sector-rate scan OUTCOME + methodology gap found | Jarvis direct squeue/sacct check + Jones HDF5 readout + Martina code-trace of the accept-path/diagnostics | **INCOMPLETE, needs redesign** | Only 2/8 tasks completed: s=0.10 (15h29m, wall_seconds=55768) and s=0.16 (18h03m, wall_seconds=65012) — real per-attempt cost INCREASES with s (+16.6% wall time for a modest s step, not anticipated when sizing the scan uniformly). Tasks s=0.22..0.50 all hit the 20h walltime and TIMED OUT with ZERO output (no partial/incremental HDF5 writes exist in this code path). More important: Martina's code trace found the scan's premise is unachievable with what was measured — `global_single_spin_flip` (accepted_by_update index 2, exactly 0/7.14e9 in both completed runs) is an ORDINARY single-edge global flip, not a dedicated cut-changing move (an earlier same-session assumption was wrong); the true winding/cut-relevant behavior is a rare wrap-around sub-case buried inside indices 1/3/5's ordinary (11-98%) accept behavior, and **no diagnostic separates that sub-case from the ordinary one** (extended_toric_code_qmc.hpp:1363 `record_update_acceptance()` only increments the aggregate per-index counter; io.cpp:243 only writes that aggregate). This run also used `--full_time_series 0`, so the one thing that could reveal sector changes directly (per-sample anyon_count/percolation_probability series) was never recorded (io.cpp:203 only emits `series` when `full_time_series` is true). Net: ~34 core-hours + ~1 day wall-clock produced move-acceptance statistics but NOT a winding-sector-mixing-rate-vs-s signal. REDESIGN before resubmitting: `--full_time_series 1` + analyze anyon_count/percolation_probability series for sector changes (or add a dedicated wrap-accept counter, more invasive); re-size N_samples/N_between_samples per s given the observed cost increase, not one uniform size across [0.10,0.50]. Flagged to PI/Feynman before another ~20h cycle. |
| 2026-07-11 | G2 wide-s sector-rate scan v2 submission | Codex/James, Slurm array `15034719`, launcher `scripts/submit_g2_sector_rate_scan_v2_20260711.sbatch`, output root `/scratch/a/A.Otaifi/paratoric_g2_sector_rate_scan_v2_20260711` | SUBMITTED | Redesign uses 64 independent single-chain array tasks: 8 s-values `{0.10,0.16,0.22,0.28,0.34,0.40,0.46,0.50}` x 8 chains, task mapping `s_index=SLURM_ARRAY_TASK_ID/8`, `chain_index=SLURM_ARRAY_TASK_ID%8`, seed `7111200 + 100*s_index + chain_index`. L=120 beta=120 z-basis, mu=J=1, h=lmbda=s, `N_thermalization=1000000000`, `N_samples=1500`, `N_between_samples=5000000`, `N_resamples=200`, Boost backend. Corrected signal capture: `--full_time_series 1` and `--observables energy anyon_count percolation_probability percolation_strength`. Resources: 1 node/task, 1 CPU/task, `--mem-per-cpu=1G`, `--time=08:00:00`. Pre-submit `squeue -u "$USER"` succeeded; post-submit `squeue -j 15034719` showed `15034719_[0-63]` pending. |
| 2026-07-12 | G2 wide-s sector-rate scan v2 OUTCOME | Jarvis squeue/sacct monitoring + Jones HDF5 readout (per-sample series analysis), `/scratch/a/A.Otaifi/paratoric_g2_sector_rate_scan_v2_20260711/` | **NULL RESULT — needs Feynman/PI read before proceeding** | 57/64 tasks completed by 05:43 CEST (7/8 s-values with full 8/8 chains: s=0.10-0.46; s=0.50 partial, 2/8 chains, 6 still running with ~3h headroom left on their 8h cap, no timeouts observed this time — the parallel-chain redesign worked as intended for wall-clock efficiency). Signal readout: `percolation_probability`/`percolation_strength` per-sample series are **EXACTLY 0.0 for every completed chain at every s-value tested** (0.10 through 0.46, plus the 2 completed s=0.50 chains) — i.e. zero percolation events anywhere in the entire tested s-range, 84000+ samples analyzed. `anyon_count` series is NOT plateau-like (91-97% of adjacent samples differ, longest constant run 3-4 samples) — confirms it's tracking ordinary local anyon-number fluctuations, not discrete topological/winding-sector jumps, so it cannot serve as the sector-change indicator either. **Net: no winding/cut-sector-change signal was detected ANYWHERE in s in [0.10,0.50] with this measurement.** Two live interpretations, NOT yet distinguished (needs physics judgment, not an engineering call): (a) the system is confined (non-percolating) throughout this entire tested range and the actual multicritical/deconfinement region lies outside [0.10,0.50] on the self-dual line — the scan never got near the transition at all; or (b) a measurement issue (e.g. percolation_probability/strength genuinely needs many more samples than 1500/chain to see even one percolating cluster at this L, or a convention/threshold issue in how it's computed/reported). Flagging to PI/Feynman before deciding next steps (widen/re-center the s-range, investigate the percolation observable's sample-size requirements, or something else) — NOT proceeding to step 2 (ladder tuning) on this data. |
| 2026-07-09 | pilot decisions + budget audit | Feynman review of M0 (commits e461686..4a70c6c) + budget audit; recorded in doc/feynman_thesis_tojarvis.md | RATIFIED | Swap-weight formula hand-verified correct incl. the beta kink-count factor (G1.2 only proved cache==recompute, not physics; G1.3 z<1 + this derivation cover correctness). PI accepted for pilot: PT z-basis-only, PT+checkpoint unimplemented (pilot=one block), beta=120 vs b100 baseline. Prereq before submit: retune L=120 ladder (L=8 ladder ~zero acceptance at L=120), cap N_rep<=30 (1 node, 4 chains). Pilot walltime-capped ~27k samples/chain, ~4.6k core-h, ratio-diagnostic (I4 100*tau bar applies at M2 not pilot). M2 BLOCKER surfaced: PT+checkpoint mandatory before production (L=192 point at I4 = 46-91 d/chain, no resume possible). Ensemble DRIVER still absent (only the analysis script exists) — needed to launch the >=4 pilot chains. |
| 2026-07-08 | G1.2/G0.3 regression fix | Clark clean rebuild from empty `build-current-clang-portable`, clang 20 portable MPI build; `ctest --test-dir build-current-clang-portable --output-on-failure`; non-MPI debug validation driver linked against clean-built `libparatoric_core.a` | PASS (single-process only) | Root cause: b29ff1c changed `total_integrated_{edge,star,plaquette}_energy()` to return caches, while low-level insert helpers mutate event vectors only and production moves update caches separately via `add_potential_*`; direct pre-existing tests therefore saw stale zero totals. Fix: public total accessors recompute again; explicit `cached_total_*()` remains PT hot path. Clean ctest: 3/3 passed in 0.42 s. Fresh cached-vs-bruteforce PT swap-weight deviations: 6.21725e-14 and 8.19345e-14 (max 8.19345e-14 < 1e-8); differs from old 1.8e-14/1.6e-14 but still gate-clean. CLI PT validation remains blocked in sandbox by MPI_Init/OpenMPI socket/PMIx failure, so real multi-rank G1.2/G1.3 still requires Slurm. |

## Baseline anchors (do not re-derive)
- L200 b400 Boost production: 80k samples / 11.8 d; tau(percolation_strength)
  ~2200; Neff ~18 (~1.5 Neff/day). t_sample ~ 12.7 s.
- L120 b100 multicritical region: 50k samples / 3.1 d; tau ~9000-10000 near
  mu~2.2-2.25, h~0.08-0.09; Neff 2-3 (~0.8 Neff/day).
- L1000 b400 Boost hour-scale: 2.12e9 attempts / ~1.4 h, MaxRSS 3.2 GB —
  memory is not the blocker.
- G2 GO bar: PT pilot tau_worst drop >= 5x vs L120 baseline.

## Open questions for M1 — answered (Feynman physics pass, 2026-07-08)

### A1. Self-dual line in code variables (RESOLVED)
- Hamiltonian (from src energy channels): H = -mu*sum_s A_s(σ^x) - J*sum_p B_p(σ^z)
  - h*sum_e σ^x_e - lmbda*sum_e σ^z_e. CLI: --muc/--Jc/--hc/--lmbdac, all
  independent.
- em-(Wegner) self-duality exchanges stars<->plaquettes (mu<->J) and
  σ^x<->σ^z (h<->lmbda). This is EXACTLY what the code's Basis='x' vs 'z'
  templating encodes (two representations of one Z). It is an EXACT operator
  duality (Vidal-Thomale-Schmidt-Dusuel PRB 80 081104), not a single-point
  coincidence.
- => Exact self-dual line is **h = lmbda at mu = J** (take mu=J=1). M1 scan is
  1-parameter s := h = lmbda (mu=J=1); the MCP is the single s* where the
  deconfined phase terminates. PT should ladder ALONG this line (vary s), not
  in (h,mu), so every replica stays exactly self-dual and I2 stays meaningful.
- Numerical guard = invariant I2 (x-basis star obs == z-basis plaquette dual).
  If I2 fails on h=lmbda, the mapping/impl is wrong, not the physics.

### A2. BPV PRB 105,165138 geometry match (RESOLVED — geometry matches)
- BPV (unitary gauge, Eq.6): H=-J*Σσ - kappa*ΣΠ; duality Eq.7
  (J',k')=(-½ln tanh k, -½ln tanh J); self-dual line Eq.9 J+½ln tanh k=0;
  MCP bicritical at (kappa*≈0.7525, J*≈0.2258); first-order segment on the
  self-dual line for 0.688≲kappa≲0.753; two duality-related Ising lines bound
  the topological phase and meet at the MCP. u1=D(J,k) is duality-ODD (=the
  first-order/self-dual direction), u2 duality-EVEN, crossover phi=y1/y2>1,
  latent heat Δ~|u2|^θ.
- Our (h=lmbda) line is the image of BPV's self-dual line under the
  continuous-time quantum->classical map; deconfined phase bounded by two
  em-dual Ising lines meeting at one MCP. GEOMETRY MATCHES. The duality-odd
  direction (off the self-dual line, h-lmbda) is BPV's u1; the tuning along
  the line is u2.
- CAVEAT (anisotropy/z): BPV is ISOTROPIC 3D classical; we are anisotropic
  (2+1)D with beta=L, which ASSUMES z=1. Binder values and crossing locations
  are aspect-ratio dependent, so our U* will NOT numerically equal BPV/XY U*
  unless anisotropy is matched or optimized. Do not compare raw crossing
  numbers to isotropic references without an aspect-ratio correction. If z!=1
  the beta=L ladder is miscalibrated (see risk R3).

### A3. Somoza-Serna-Nahum (PRX 11,041008; arXiv 2012.15845) — what we can /
        cannot reproduce (RESOLVED)
- SSN find the self-dual transition CONTINUOUS (L<=96). Key numbers:
  duality-odd op x_A=1.224, duality-even op x_S=1.506 (~3/2), ν=1/(3-x_S)=
  0.669(4). MC-dynamics z≃2.48 (membrane stochastic dynamics — NON-physical,
  scheme-specific). Fourth exponent = fractal dim d_f of anyon-worldline
  percolation in spacetime.
- CAN reproduce with existing observables:
  * BPV energy cumulants C_nm and SSN cumulant-RG (Var(S)=L^{-2x_S}f) — build
    from per-snapshot energy_h/energy_lmbda/energy_mu/energy_J time series:
    duality-odd A ~ (E_h - E_lmbda), duality-even S ~ (E_h + E_lmbda) on the
    self-dual line. This is our route to x_S and ν.
  * Percolation-based transition location + first-order test — our
    percolation_probability/_strength/largest_cluster (Linsel et al. basis)
    are order-parameter-like and fine for crossings/histograms.
    CORRECTED 2026-07-08 (Fable re-check): these are EQUAL-TIME-SLICE spatial
    clusters (lattice.cpp largest_cluster: union-find over g[e].spin at the
    current slice). SSN's fourth exponent d_f is for anyon-WORLDLINE clusters
    in 2+1D SPACETIME — a 2D slice does not carry the 3D d_f. Quantitative
    reproduction of SSN's d_f needs spacetime cluster code (Q2-adjacent,
    beyond M0). Do not claim the d_f deliverable without it.
- CANNOT reproduce without new code / not applicable:
  * SSN x_A, x_S from real-space 2-pt/3-pt correlators G_A(r),G_S(r) and the
    conformal 3-pt test (their Fig.17): we only have q=0 integrated
    susceptibilities, giving at best a susceptibility-FSS x estimate, not the
    real-space conformal check. GAP if we want to match their decisive test.
  * SSN's MC-dynamics z≃2.48: tied to their local-Metropolis membrane
    dynamics; our continuous-time worldline + PT dynamics is different and
    this z is not a physical exponent — NOT reproducible/meaningful.
  * Emergent-O(2) test (the ACTUAL XY-vs-new-class discriminator): needs
    operator-multiplet/degeneracy structure (currents), not single exponents.
    Our observable set does not expose it. => we cannot separate scenario 2
    from 3 even in principle with the planned observables (see R1).

## Plan risk flags (Feynman physics pass, 2026-07-08)

- R1 (UNDERPOWERED — scenario 2 vs 3): XY and the SSN "new self-dual class"
  are exponent-degenerate at our reach. ν_XY=0.67169(7) vs SSN ν=0.669(4):
  gap ~0.003 (0.4%); x_S(XY)=3-1/ν_XY≈1.511 vs SSN 1.506. Excluding XY via ν
  at 3σ needs σ_ν<=0.001; an L<=192, 6-size, Neff~30 campaign realistically
  gives σ_ν~0.01-0.02 (~10x too coarse). SSN themselves could not exclude XY
  at L<=96 with dedicated methods. => Reframe M3/M4: the DECIDABLE question is
  first-order vs continuous, NOT XY vs new class. Do not promise a
  universality-class verdict on exponent values alone.
- R2 (Binder as XY discriminant is weak here): local operators have large
  scaling dim (SSN) so standard Binder is "ineffective"; our percolation_strength
  Binder tests PERCOLATION universality (d_f), a different fixed-point quantity,
  and is aspect-ratio dependent (R? A2 caveat). Binder crossings separate
  first-order (min diverges) from continuous, but not XY from new class at any
  reachable precision. Keep Binder for the first-order test; drop it from the
  XY-exclusion claim.
- R3 (z=1 assumption unverified): beta=L presupposes z=1. Cheap early check:
  anisotropic FSS / match spatial vs imaginary-time correlation lengths at one
  point before committing the ladder. Our (2+1)D setup can MEASURE z (SSN's
  isotropic model cannot) — a potential value-add, but needs a temporal
  correlator not currently output.
- R4 (winding-loop move — energy-bookkeeping trap, NOT a DB flaw): the move is
  a symmetric involution, so detailed balance holds automatically IF (i) the
  loop is chosen from a configuration-INDEPENDENT distribution and (ii) the
  acceptance uses the correct ΔE. The plan phrase "compose cached O(1)
  global-single-flip diffs along a straight loop" is DANGEROUS: single-edge
  full-period flips each change 2 stars; adjacent loop edges SHARE a star, so
  for a closed straight non-contractible loop every on-line star is flipped
  twice => ALL tuple (mu, in x-basis) contributions CANCEL and ΔE reduces to
  the pure field term ΔE = 2h*Σ_edges ∫_0^β σ^x dτ. Naively summing cached
  single-flip diffs double-counts a spurious star term. Fix: derive ΔE
  analytically with tuple sector = 0; keep G1.4(a) L=8 brute-force check as the
  gate. Also confirm the move actually changes the winding/topological sector
  (is_winding_percolating) or it is not doing its job.
- R5 (winding move DOA at production scale — SHARPENED 2026-07-08): ΔE =
  2h*Σ_e∫σˣdτ has mean ~ 2h*L*β*⟨σˣ⟩; at MCP (h≈0.34, ⟨σˣ⟩=O(0.3-0.5)),
  L=β=120 gives ΔE=O(10³) => acceptance ~ e^{-10³}. Not marginal — dead.
  Usable only at gate sizes (L=8). CONSEQUENCE: the plan's kill-criterion
  rescue "PT+winding move" is illusory; PT is the only real lever. If G2 is
  MARGINAL, the retuning option is the PT ladder, not this move. R4 nuance:
  sequentially composed single-flip diffs WITH cache updates telescope
  correctly; the trap is summing pre-flip cached diffs without applying.

## Sharpened open questions (carry into M0/M1)
- Q1: Decide the M4 claim scope NOW: target = "continuous vs first-order" (in
  reach) + report exponents "consistent with XY/SSN, cannot distinguish". Get
  user sign-off before budgeting L>=128.
- Q2: Will we add real-space A/S correlators (to attempt x_A,x_S like SSN) or
  stay with cumulant-FSS + percolation d_f only? Former is the only path to a
  genuine exponent statement but is new-code scope beyond M0.
- Q3: Confirm z=1 (R3) at one self-dual point before the FSS ladder.

## 2026-07-12 checkpoint: Step-1 closed, D4 done, rotate_imag_time bug fixed
- Feynman resolved the electric-percolation-channel puzzle: REAL PHYSICS, not
  a bug (dynamic density 37-40% vs square-lattice bond p_c=0.5; plaquette is
  the deconfinement POP, matching Linsel; electric is the correctly-near-zero
  dual quantity). Closes Step 1. PI: "proceed."
- D4 (independent h/lmbda PT ladders) DONE, commit `e42914d`. Self-dual
  fallback preserved bit-for-bit; swap weight decoupled (kink term ~ h,
  edge term ~ lmbda) per the Hamiltonian. Clean rebuild, 3/3 ctest. Martina
  review dispatched, blocked on CODEX'S OWN usage-limit wall (not a project
  finding) — retry pending.
- Real bug found + fixed while implementing Feynman's item 2: `get_sample`
  dispatched observables without one shared rotation per sample; some
  observable lambdas rotated independently, so evaluation ORDER determined
  which tau-slice each observable read (percolation could differ from
  energy/susceptibility in the same sample). Fixed, commit `f08fd9d`: one
  rotate_imag_time() per sample, all equal-time observables share it. New
  regression test confirms identical rotation index across all 4 percolation
  observables. Clean rebuild, 3/3 ctest.
- Remaining before Linsel-with-PT re-validation: adopt plaquette_percolation_*
  as POP (documentation only), >50%-density confirmatory check (electric
  should turn ON), find converged thermalization depth at L=16-32, then
  re-run the Linsel h-scan on plaquette+rotate+long-thermalization.
