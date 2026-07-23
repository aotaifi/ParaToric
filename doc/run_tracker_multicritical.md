# Multicritical Campaign Tracker — current-state card

Updated: 2026-07-21 (rewrite this card in place; durable results -> plan/report).

## Status
- STEP 2B L=10 PT speedup calibration submitted 2026-07-21 as one-node,
  14-rank MPI job `15178251` under
  `/scratch/a/A.Otaifi/paratoric_pt_speedup_l10_20260721/`. Fatal fixed-h
  bookkeeping pre-check PASSED: each rank-local observable sample is aligned
  with `ladder_index_by_sample`, so registered `scripts/ensemble_diagnostics.py`
  reconstructs fixed-rung series with shape and unique-owner checks. The job
  exactly reuses Lane-2 job `15140239`'s 14-rung h ladder and proven portable
  MPI binary (`b0ce5242`), with L=beta=10, lmbda=0.3, mu=J=1, z basis, swap
  period 2 sweeps, 16M thermalization plus 4M production sweeps, 2M full-series
  samples at two-sweep cadence, no checkpoint, 14 CPUs, explicit 8G, and a 3h
  cap. Proven timing gives about 95m/22.2 task-hours; budget line is 28
  task-hours. Swap-period 1/5/10 variants are deferred because four full-depth
  runs would cost about 89 task-hours and quadruple the priority measurement;
  free swap-period-2 transport/acceptance/excursion diagnostics remain scoped.
  Concise-alert submission failed on transient controller DNS and created no
  notifier job.
- STEP 2A self-dual locate-s* scan COMPLETED 2026-07-21 under
  `/scratch/a/A.Otaifi/paratoric_selfdual_step2a_20260721/`: pilot array
  `15177060` plus release array `15177203` produced 528/528 HDF5 files with
  63,900 samples/chain for `L={8,12,16}`, uniform `s=0.200:0.025:0.450`, both
  bases, and 4 cold+4 hot chains. Actual aggregate cost was 44.88305555555556
  task-hours, below the 61.1 guarded estimate. The exact submitted/pilot
  convention is fixed trailing 400-sample checkpoint windows; its final POP
  curves do not give one clean common three-pair crossing. The higher-statistics
  interval sensitivity puts all three crossings in `[0.325,0.350]`, so this is
  only a SENSITIVITY-SUPPORTED PROVISIONAL bracket, not a precision result. Of
  201 gate-eligible fixed-window I2/B5 comparisons, 200 pass and one isolated
  `L=8,s=0.275`, 200k-sweep comparison flags at z=3.85577386115; it passes at
  400k (z=0.0342393478699) and in the interval sensitivity at 200k
  (z=0.497362160032), so no systematic implementation violation is found.
  Thirteen of 66 per-point pair statuses remain NOT_GATED. All 528 fixed-window
  energy histogram screens have no bimodality candidate; no stable-separated
  energy gap occurs, while 14 trajectories show shrinking-to-zero under-
  thermalization and every final gap has z<3. ORDER INCONCLUSIVE: absence of
  bimodality at L=8--16 is never evidence for continuity. Durable readout:
  `/scratch/a/A.Otaifi/paratoric_selfdual_step2a_20260721/STEP2A_READOUT.md`.
- COMPLETED L=20 no-PT scale-up (2026-07-20): Slurm array `15159599` finished
  all 72/72 tasks; all HDF5 series have exactly 2,552,000 samples and every
  direct percolation-sector series has nonzero exchanges (minimum 26,046/chain).
  **READOUT AMBIGUOUS; L=30 NOT SUBMITTED.** At 64M all nine h-points have
  cold/hot z<3 on ratio and density, including h=0.335362847648 at
  0.764827/1.50209, so neither basin disagreement nor sector trapping remains.
  However positive-control h=0.305629247317 has a 16M ratio split z=4.68373 and
  no nontrivial stationary suffix after it; decisive h=0.339573841268 has no
  nontrivial stationary suffix through 64M (constant-fit
  p=0.0259867/0.0298351/0.0220827/0.0297133 from 8/16/32/48M). The analyzer's
  reported 64M passes are degenerate one-point suffixes (p=1, slope-z=0 by
  construction), not plateau evidence. Defensible plateau entries are 48M at
  h=0.318989179906; 8M at the other six passing h-values; NONE through 64M at
  h=0.305629247317 and 0.339573841268. Therefore no actual L=20 depth datum
  exists and the requested two-point z fit is undefined; using 64M would merely
  reproduce the guessed prior z=2 from its budget endpoint. Actual L20 timing is
  median/mean/worst 10.455972/10.752164/13.807778h per chain. Conditional on the
  unvalidated z=2 design plus L^2 per-sweep scaling, L30 would cost
  52.933359/54.432832/69.901875h per chain and 3,919.163906 task-hours total,
  but this is not an authorized fitted-exponent design. Full numeric readout is
  in `/scratch/a/A.Otaifi/paratoric_linsel_no_pt_l20_depth_ramp_20260719/DEPTH_LAW_AND_DESIGN.md`.
- FINALIZED L=10 no-PT depth-ramp submitted 2026-07-18 as one 72-task Slurm
  array `15149119`, using
  `/scratch/a/A.Otaifi/paratoric_linsel_no_pt_l10_depth_ramp_20260718/submit_linsel_no_pt_l10_depth_ramp_20260718.sbatch`.
  It exactly covers the locked nine-point grid: decisive
  `{0.318989179906,0.322,0.326,0.330615254621,0.335362847648,0.339573841268,0.343520981103}`
  plus controls `{0.305629247317,0.347398758564}`, with four cold and four hot
  chains per h and fresh unique seeds `94018000-94018071`. Every chain is one
  continuous no-PT trajectory to 40M sweeps, sampled every 25 sweeps after the
  pilot-consistent 200k initial segment; exact-depth series indices retain the
  2/4/8/16/24/32/40M readouts. `--full_time_series 1` is set and the direct
  `percolation_probability` winding proxy is explicitly requested alongside
  plaquette POP/POS, largest_cluster, and sigma_z. One node/task, one CPU, 1G,
  3h cap. Rendered design diff, 72-task map, fresh-seed exclusion, CLI HDF5-series
  smoke, and scheduler test all PASS in `PRE_SUBMISSION_CHECK.md`; 36 tasks began
  immediately and the rest were QOS-limited pending. The earlier pilot launcher
  also had full-series enabled, but did not request direct `percolation_probability`.
  Concise-alert attachment failed twice on transient controller DNS; no notifier
  job was created. Current CLI writes the full-series HDF5 only at chain end, so
  the seven readouts are retained exact-depth windows rather than progressive
  HDF5 files; do not poll or read out until the array completes.
- Linsel exact-size Lane-2 PT overlay submitted 2026-07-17: L10=14 rungs/job
  `15140239`/28G, L20=40/job `15140247`/80G, and L30=67/job
  `15140246`/134G. All are one-node, one-rank-per-physical-core MPI jobs with
  3h caps, beta=L, mu=J=1, z basis, fixed lmbda=0.3, h in [0.15,0.50], swap
  period 2 sweeps, and 500k samples every 2 sweeps = 1M production sweeps
  (5x the original production window). Thermalization is 125k/500k/1.125M
  sweeps, following the existing approximate L^2 depth rule. Rung counts obey
  N~L^(3/2): L10=round(40*(10/20)^(3/2))=14; L20 exactly reproduces job
  `15118635`'s original 40-rung ladder; L30=round(60*(30/28)^(3/2))=67 and
  preserves job `15118637`'s original three-band link-density proportions.
  The launcher's preflight rendering was diffed against the declared design
  for every L with an empty diff; the complete L20 ladder string also exactly
  matched job `15118635`'s recorded actual ladder. Evidence and job-local
  current-HEAD portable MPI builds are under
  `/scratch/a/A.Otaifi/paratoric_pt_linsel_exact_sizes_20260717/`, pinned to
  `b0ce5242eb20081241066e73ef2cc40de9d3cf00`. The single allowed
  `squeue --start` check showed all three PENDING (Priority), start N/A; do not
  poll. Expected runtime after start is about 5-10m/20-30m/55-75m for
  L10/L20/L30 (3h caps). Concise alerts failed on transient controller DNS and
  created no notifier jobs. Fresh dispatch performs strict-RT/link-motion and
  rung-reorganized raw-ratio overlay readout; the extrapolated L10/L30 ladders
  and thermalization depths remain design judgments until that readout.
- Sharp h_c-targeted Linsel PT rerun submitted 2026-07-17: L20=66 rungs/job
  `15134326`/132G, L24=78/job `15134327`/156G, and L28=96/job
  `15134328`/192G. All are one-node, one-rank-per-physical-core MPI jobs with
  3h caps, beta=L, mu=J=1, z basis, fixed lmbda=0.3, h in [0.15,0.50], swap
  period 2 sweeps, 100k samples every 2 sweeps, and unchanged
  500k/720k/980k thermalization sweeps. Registered `scripts/pt_diagnostics.py`
  production-trajectory motion isolated the unique stationary minima at
  [0.343520981103,0.347398758564]/[0.339587551480,0.342853140293]/
  [0.335661077700,0.338393915461] for L20/L24/L28, with acceptance
  0.041980+/-0.000897, 0.039320+/-0.000869, and 0.099200+/-0.001337 and
  adjacent-rung flow jumps 0.918425->0.0877398, 0.926251->0.1258, and
  0.79192->0.20138. Registered `scripts/acceptance_informed_ladder.py` sharply
  subdivided the minimum link plus one shoulder on each side by factor 3: each
  local band now has 9 links instead of 3, while every rung outside the band
  (including the healed h~0.435 seam) is unchanged. Local mean gaps changed
  0.003905377155->0.001301792385, 0.003312303923->0.001104101308, and
  0.002760893471->0.000920297824. Evidence and launcher are under
  `/scratch/a/A.Otaifi/paratoric_pt_linsel_hc_targeted_20260717/`; the job-local
  portable MPI build is pinned to submission-time HEAD
  `d67a89c4f24179076e8eb996b34612fa322cf61c`. The single allowed
  `squeue --start` check returned no row/start estimate for every job; do not
  poll. Expect roughly 15m/30m/40m after start from the preceding run (3h caps).
  Concise-alert submission hit transient Slurm-controller DNS failure and was
  not retried. Fresh dispatch must use `scripts/pt_diagnostics.py
  --production-motion` and require >=3 strict round-trips; combined stored swap
  counters remain invalid for this decision.
- Acceptance-informed expanded-ladder Linsel rerun submitted 2026-07-17:
  L20=60 rungs/job `15126362`/120G, L24=72/job `15126364`/144G, and
  L28=90/job `15126363`/180G. All are one-node, one-rank-per-physical-core MPI
  jobs with 3h caps, beta=L, mu=J=1, z basis, fixed lmbda=0.3, h in
  [0.15,0.50], swap period 2 sweeps, 100k samples every 2 sweeps, and unchanged
  500k/720k/980k thermalization sweeps. The +50% rung budget is intentional:
  it removes the fixed-count zero-sum tradeoff between the h_c and h~0.43
  acceptance holes while remaining below the verified 128-physical-core node
  ceiling. Registered `scripts/acceptance_informed_ladder.py` inferred the
  whole-range physical-coordinate metric 2*erfcinv(acceptance)/dh from jobs
  `15118635`/`15118636`/`15118637`, Gaussian-smoothed its log, and placed the
  new rungs at equal integrated metric. Physical-coordinate verification versus
  the ORIGINAL resourced ladder passed: gap ratios new/original at h=0.340 are
  0.557243/0.537862/0.525092 and at h=0.435 are
  0.434191/0.416324/0.420487 for L20/L24/L28. Evidence and the reused canonical
  launcher are under
  `/scratch/a/A.Otaifi/paratoric_pt_linsel_acceptance_informed_20260717/`.
  Job-local portable MPI builds are pinned to submission-time HEAD
  `e33f6b38f72ecc66dc24b0855f10f6907fbbca91`. The single allowed
  `squeue --start` check showed all three PENDING (Priority), with no start
  estimate; do not poll. Based on the previous 9m55s/20m03s/25m56s elapsed
  times, expect about 15m/30m/40m after start (3h caps). Concise-alert submission
  hit transient Slurm-controller DNS failure and was not retried. Fresh dispatch
  must run `scripts/pt_diagnostics.py` and decide whether each L achieved >=3
  strict round-trips; 0 strict round-trips remains possible because adding 50%
  rungs increases the diffusive N^2 path cost even while healing bottlenecks.
- Single-iteration PM redistribution was rerun on the measured per-link
  acceptances from resourced Linsel jobs `15118635`/`15118636`/`15118637`, using
  registered `scripts/adaptive_ladder_pm.py` defaults and only changing
  `--max-iterations 1`. Physical-coordinate verification FAILED at the critical
  window: the gap bracketing h=0.340 became coarser for L20
  `0.007083333333 -> 0.008671598695`, L24
  `0.006071428571 -> 0.007609812871`, and L28
  `0.004722222222 -> 0.005950881498`. The seam probe h=0.435 did improve:
  L20 `0.011428571428 -> 0.006411113025`, L24
  `0.010000000000 -> 0.005750185138`, L28
  `0.008000000000 -> 0.004598620618`. Per the explicit stop condition, no
  benchmark rerun was staged, built, submitted, or queried. Inputs and complete
  interval evidence are under
  `/scratch/a/A.Otaifi/paratoric_pt_linsel_pm1_20260716/`.
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
- RESOLVED: the L=10 depth-ramp binary came back PASS (job `15149119`) — the h_c
  bottleneck is depth-solvable ordinary critical slowing, NOT winding-sector
  ergodicity breaking. WORM is NOT mandatory; Decision 1 holds. Marker test
  (Decision 2) also DONE (Hypothesis A confirmed).
- IMPORTANT SCOPE FACT (do not lose): every benchmark run so far is at lmbda=0.3 =
  the Linsel ISING line, OFF the self-dual line. We have NEVER run at the MCP.
  Benchmark point (h,lmbda)=(0.335,0.30); MCP=(s*,s*) with s*~0.33-0.35 still
  UNMEASURED. Near-identical h, different lmbda — a live trap. Benchmark-line z is
  a BEST-CASE bound on MCP cost, not the M2 budget (a first-order MCP slows
  EXPONENTIALLY, not as a power law).
- **VOID CLEARED; STEP 3 CANCELLED (Feynman UPDATE 2026-07-20g, current verdict).**
  Jarvis's STEP 0 blocking-plateau + between-chain estimator (4 review rounds) was
  run on both jobs. Judged on the GATES rather than on Feynman's broken mechanism
  clause, both formerly-void L=20 points now PASS: h=0.339574 suffix p=0.0260 ->
  **0.20** with cold/hot z=0.42; h=0.305629 p=**0.33**, z=0.33, Neff ~1.48M. The
  original void rested on a p computed with a tau 40-100x too small. Basis gone.
  What stays unresolved is TAU ITSELF at h=0.339574 (TAU-UNRESOLVED, lower bound
  >=2157 sweeps), not the mean and not the crossing — and no downstream decision
  reads that tau, so the 128M rerun buys nothing. **STEP 3 CANCELLED as scoped**
  (170-340 task-hours saved; Jarvis asked the question that cancelled it). The
  tau-unresolved FLAG travels with those points and is reported.
- **THE ONE COHERENT PICTURE** (supersedes the 20d/20f flip-flop; both halves true
  at once, no contradiction): ~95-98% of the raw ratio's VARIANCE decorrelates
  within one 25-sweep sample, so error bars on the mean are set by the fast
  component and are CORRECT (between/bootstrap ratio 0.49-1.02 across all 18
  points — 20f stands). The slow component carries little variance but lives long,
  and IPS truncates before it, so IPS tau is 40-100x low at L=20 where blocking
  returns TAU-UNRESOLVED (20d stands). Different quantities: fast dominates the
  VARIANCE, slow dominates the TIME. The stationarity test consumes tau via block
  sizing, which is why correcting tau flipped those p-values.
- **RATIFIED — Jarvis's sqrt(7/8) catch.** A delete-one jackknife SE of a simple
  MEAN is an exact algebraic identity of the ddof=1 sample SE, so the
  between/bootstrap ratio returns 0.935 for ANY data, always, by construction. The
  check only has content for a NONLINEAR observable (the raw ratio). ACTION: the
  driver must REQUIRE an explicit observable and REFUSE the ratio for a linear one
  rather than printing a constant a future reader mistakes for a diagnostic.
- **FREE CROSSING RE-DERIVATION DONE; UPWARD-SHIFT PREDICTION FAILED.** Restoring
  all nine L=20 points leaves the data-bracketed linear crossing unchanged at
  h=0.3244097798 (with/without are identical because the bracket is h=0.322--0.326).
  The 18-choice interpolation/window scan is the controlling uncertainty: with all
  points 0.3237135711--0.3349290674, full spread 0.0112154963; without the two
  points 0.3237135711--0.3349354739, spread 0.0112219028. Thus the nominal crossing
  stayed near 0.324, but the honest envelope reaches Linsel's 0.335(2) band and no
  several-sigma tension is established. The repeated +/-0.0000817 was also a
  decimal-place transcription error: the old TSV records +/-0.0008170099, still
  far smaller than the interpolation systematic. Benchmark remains PROVISIONAL.
- **tau_int ANOMALY EXPLAINED, not a bug.** L=20's near-h-independent tau ~18-27
  sweeps (Jarvis right to flag it) is IPS reporting the variance-weighted product
  (slow-mode amplitude x decay time) and truncating the tail: the amplitude FALLS
  with L through self-averaging (measured: tail drops ~0.022 -> ~0.001) while the
  time RISES. Blocking, which does not truncate, returns TAU-UNRESOLVED at the same
  points. **QUARANTINE STANDS: `hc_raw_ratio_apparent_z = -7.65 +/- 0.52`,
  `tau20/tau10 = 0.00498`, and `hc_base_density_apparent_z = +0.195`** (base-density
  tau is 19-21 sweeps at BOTH sizes, pinned at the sampling floor, so that ratio
  measures our sampling interval). tau_int cannot yield a dynamic exponent. STEP 4
  remains INVALID AS SPECIFIED. The sector-exchange z~3.7 hint is therefore NOT
  independently cross-checked; it stays a single unconfirmed hint. Do not upgrade it.
- **CORRECTION — the tau-peak claim was overstated (Jarvis right).** L=20 h=0.326
  and 0.330615 are ALSO tau-unresolved (>=954, >=792), so "isolated peak over a flat
  floor" is NOT established; possibly one broad slow region. Downgrade to "the
  largest lower bound is at h=0.339574, consistent with but not establishing the
  pseudo-critical location". Do NOT enter it in benchmark_report.md as a dynamical
  determination. Same for the L=10 rise toward small h: suggestive of the
  frozen-winding mode, but every L=10 point is tau-unresolved too.
- **2A/2B REPORTED AND VERDICT ISSUED (UPDATE 2026-07-20i). Root cause found under
  all three loose ends: THE L=10 BASELINE IS NOT ERGODIC AT LOW h.** Per-chain
  readout of job `15149119` at h=0.305629: ratios 1.7426 1.7245 1.7418 1.7421 |
  1.2993 1.7397 1.7391 1.4531, with seven chains reporting tau~0 and one 171,600
  sweeps. **I4 requires per-chain tau within a FACTOR 2; measured factors are
  11,275x (h=0.305629), 8,042x (0.318989), 4,595x (0.326), 7x (0.322) — L=10 FAILS
  I4 at 8 of 9 points.** That clause has never been enforced in this campaign. The
  benchmark crossing is bracketed by h=0.322/0.326, i.e. read off the worst of it.
- **WHY EVERY CHECK MISSED IT:** cold/hot at h=0.305629 (1.7377 vs 1.5578) PASSES I5
  *because* the hot group is internally split (spread 0.4405) — the heterogeneity
  that should alarm us inflates the group variance and drives z down. I5 can be
  passed by a broken ensemble whose own dispersion hides the break; the 20f
  between-chain error is fooled the same way (an honest large bar on a bimodal
  population still averages two states, neither of which is the answer). Only the
  PER-CHAIN table shows it. Run manual: report per-chain, not just group summary.
- **NOT the refuted winding hypothesis — do not resurrect it.** Sector exchanges at
  h=0.305629 are 308214/309803/306432/308256/147750/307440/306942/260468, healthy;
  the 18b refutation STANDS. Honest note: the two deviant chains are the two with
  suppressed exchange (147,750 and 260,468), a correlation only. Call it
  UNIDENTIFIED BASIN STRUCTURE on the low-h deconfined side (cf. E3).
- **2B's "physics disagreement" flag is probably BACKWARDS, and it is the best news
  in the report.** Fixed-h reconstruction of the 14-rung PT run gives a cleanly
  MONOTONIC curve (1.3026 1.3172 1.3377 1.3630 1.3828 1.4085 1.4408 1.4969 1.6908
  1.9074 ... over h=0.15->0.3775), while no-PT L=10 is nonmonotonic (1.6206 at
  h=0.3056 falling to 1.4434). PT interpolates to ~1.42-1.43 at h=0.3056 vs no-PT's
  1.6206, and no-PT's six frozen chains sit at 1.74. The nonmonotonicity that made
  interpolation underdetermined IS the basin contamination. So PT plausibly has the
  BETTER-SAMPLED curve and its crossing moves TOWARD Linsel's 0.335(2). NOT PROVEN:
  PT still carries residual init memory at low h (cold/hot by origin init per rung:
  0.0712/0.0663/0.0561/0.0386/0.0240 over h=0.2712->0.3563). PT is better here, not
  clean; both curves are contaminated at low h.
- **REFRAME: PT's figure of merit is ERGODICITY, not speed.** 2B's censored tau
  bounds are a null on the metric we CHOSE. The benefit PT demonstrably delivers is
  repairing an ensemble local updates cannot mix — worth more than a speed factor,
  since speed buys error bars and ergodicity buys CORRECTNESS. This also corrects
  20h: "the gate is measured tau at the rung of interest" is INSUFFICIENT, because a
  chain frozen in a basin reports LOW tau (7 of 8 above). **Gate = tau AND per-chain
  agreement.**
- Next action (per UPDATE 2026-07-20i sec 7):
  (a) FREE, DO FIRST — per-chain I4 audit of `15149119`, `15159599` and the 2A scan:
      per-chain ratio and tau (cold/hot labelled), tau max/min factor, and a mark on
      every point violating the factor-2 clause. Add per-chain tau-factor as a
      standing column in `ensemble_diagnostics.py`; make I4's factor-2 clause a HARD
      per-point gate, not documentation.
  (b) FREE — do NOT promote the benchmark crossing; do not report 0.3244 as our
      value. `benchmark_report.md` stays PROVISIONAL and now states WHY: the L=10
      baseline is not ergodic at low h. This supersedes "consistent with Linsel
      within honest systematics" — neither consistency NOR tension is claimable.
  (c) CHEAP, DECISIVE, PRE-COMMITTED BOTH WAYS — test the sec-4 hypothesis: a no-PT
      L=10 run at h=0.3056 and h=0.322 from 16 independent random inits. If PT is
      better-sampled, the pooled ratio moves DOWN toward ~1.43; if it reproduces
      1.62 with tight agreement, PT's low-h rungs are the contaminated ones and
      Feynman is wrong.
  (d) 2A denser [0.325,0.350] sampling APPROVED, with the per-chain gate from (a)
      carried into it — "no common three-size crossing" plus 13 NOT_GATED pairs may
      be the same disease at small L on the low-s side.
  (e) 2B matched-grid rebuild AUTHORIZED (Jarvis's a/b/c), REORDERED: the crossing
      shift is no longer a worry to clear first, it is the most interesting
      measurement — do it first via (c). Add ergodicity repair to the deliverables
      beside the speed factor.
  L=30, L=24: still HOLD. Step 3 unaffected.
- **PI CORRECTION (UPDATE 2026-07-20h): ROUND-TRIPS ARE NOT NECESSARY for low
  autocorrelation.** Run manual F7's "NECESSARY but NOT SUFFICIENT" is WRONG on the
  first half. A round trip is SUFFICIENT evidence of decorrelation (it guarantees a
  reservoir visit) but not REQUIRED: what matters is the replica at the h of
  interest escaping its basin, which local excursions over a few rungs can do with
  zero completed round trips — and diffusing to the far end is the most expensive
  route (tau_RT ~ N^2/A_harmonic). **The gate is measured tau at the rung of
  interest; round-trips are transport CONTEXT, never pass/fail.**
  CONSEQUENCE IN HINDSIGHT: all four ladder NO-GO calls were scored on ">=3 strict
  round-trips" (F3) and NONE measured tau at h_c. So "4 consecutive PT failures" is
  WEAKER evidence against PT than it has been treated as. The marker test's genuine
  0.235% h_c barrier still stands; the point is we never measured the deciding
  quantity. Same mechanism-vs-benefit conflation as before, now on the diagnostic side.
  CAVEAT THAT CUTS THE OTHER WAY: this holds when the slow mode is LOCAL (continuous
  transition). At a FIRST-ORDER point the replica escapes only by reaching a rung
  where the coexistence gap closes, so round-trips become close to NECESSARY again
  and the h_c acceptance hole is physics, not a ladder defect (E2/F8/H5). Which
  regime we are in IS thesis T1, undecided — so do NOT generalize the corrected rule
  to the MCP in advance. It is correct on the benchmark line, which is where 2B runs.
- **DOC FIXES REQUIRED (cheap, load-bearing):** run manual F3 (delete the ">=3 strict
  round-trips or the ladder is broken" bar), run manual F7 (correct to "neither
  necessary nor sufficient" + the first-order caveat), `doc/pt_architecture.md`
  (same, and record that the 4 NO-GO calls used a non-gating metric), and harness
  **gate G1.2**, which carries the same ">= 3 round trips" language — a harness gate
  is the worst place to leave a wrong bar.
- **PROMOTED: PT operational calibration is now an explicit 2B deliverable** (PI's
  point — bank the experience, not just the go/no-go), at BOTH L=10 and L=20:
  (i) tau at the rung of interest vs swap_period in SWEEPS (~1/2/5/10) — there is an
  optimum between label-travel-on-a-frozen-config and wasting the ladder, and we
  have never located it; report the CURVE. (ii) sweeps-to-decorrelate vs L at fixed
  ladder — the quantity a budget actually needs. (iii) per-link acceptance vs rung
  spacing dh, HARMONIC mean called out separately from the median (Lane 1: harmonic
  governs transport). (iv) EXCURSION STATISTICS replacing round-trips — distribution
  of rungs travelled before returning, and its correlation with tau reduction at
  that rung; this is the diagnostic that replaces F3 and directly tests the PI's
  point. (v) all of 20c's cost-normalized metrics. Deliverable: a short PT
  calibration table in `doc/pt_architecture.md` a future run can size a ladder from.
- Next action (per UPDATE 2026-07-20g, with the 20h doc fixes folded into step (a)):
  (a) FREE, NOW — re-derive the crossing with all nine L=20 points; report WITH and
      WITHOUT; propagate an interpolation systematic (vary scheme and fit window).
      Update benchmark_report.md, keep PROVISIONAL either way.
  (b) THEN 2A and 2B IN PARALLEL, exactly as specified in 20d including every
      addition: 2A's both-bases I2/B5 duality check at EVERY scanned s, pooled and
      per-chain energy histograms plus the cold/hot ENERGY GAP vs depth, the E2
      warning against auto-diagnosing a persistent cold/hot gap as
      under-thermalization, and the bimodality asymmetry rule verbatim; 2B's
      rank-versus-ladder-position pre-check before submitting, and the wording limit
      that speedup~1 licenses "a plain single-parameter h-ladder does not help", NOT
      "PT does not help". **2B's L=20 arm is UNBLOCKED** — the void is cleared, so
      the L=20 no-PT baseline exists and no longer waits on a rerun.
  (c) STEP 3 CANCELLED as scoped. STEP 4 INVALID as specified. L=30 and L=24 HOLD.
- Older outstanding readouts (jobs `15134326`-`15134328`, `15052942`-`15052944`,
  exact-size Lane-2 overlay) remain SUPERSEDED as decision drivers; the PT-ladder
  round-trip chase stays parked per Decision 1 (benchmark decoupled from PT).

## Gate log (append one line per gate decision)
| date | gate | evidence | result | decision |
|---|---|---|---|---|
| 2026-07-21 | ROOT CAUSE: L=10 benchmark baseline FAILS I4 (per-chain tau within factor 2) at 8/9 points, by up to 11,275x, in the crossing region | Per-chain audit of job `15149119`: h=0.305629 ratios 1.30/1.45/1.74 across 8 chains, tau 0 to 171,600 sweeps | FAIL / GATE NEVER ENFORCED | Crossing 0.3244 NOT promotable; I4 factor-2 clause becomes a hard per-point gate. |
| 2026-07-21 | I5 cold/hot can PASS on a broken ensemble — hot-group internal split inflates variance and drives z down | h=0.305629 cold 1.7377 vs hot 1.5578, hot spread 0.4405, z passes | METHOD DEFECT | Report per-chain tables, not group summaries. |
| 2026-07-21 | 2B 'physics disagreement' REREAD: PT curve MONOTONIC, no-PT L=10 nonmonotonic; PT plausibly better-sampled and moves toward Linsel 0.335(2) | Fixed-h reconstruction of job `15178251` 14 rungs vs job `15149119`; PT residual init memory 0.0712->0.0240 | HYPOTHESIS / PRE-COMMITTED TEST SET | PT figure of merit reframed to ERGODICITY not speed; 20h tau-only gate corrected to tau AND per-chain agreement. |
| 2026-07-21 | 2A order INCONCLUSIVE + duality 200/201 PASS RATIFIED; 2B decision rule NOT TRIGGERED ratified (censored taus, 144x/64x correctly refused) | Jarvis jobs `15177203`, `15178251` | RATIFIED | Both stop calls correct; denser 2A sampling and matched-grid 2B rebuild authorized. |
| 2026-07-21 | STEP 2A full-grid locate-s* readout | `/scratch/a/A.Otaifi/paratoric_selfdual_step2a_20260721/STEP2A_READOUT.md`; registered `scripts/ensemble_diagnostics.py selfdual-step2a`; arrays `15177060`/`15177203` | PROVISIONAL BRACKET / ORDER INCONCLUSIVE | 528/528 complete at 44.88305555555556 task-hours. Physical POP size ordering reverses in s=[0.325,0.350]; interpolated crossings 0.3295374649829121--0.3328006522747564. 164 eligible duality comparisons PASS, max z=2.45583807467; 18/66 point-pair checks NOT_GATED. No bimodality candidate and no stable-separated energy gap; absence is not continuity evidence. Refine/deepen before MCP production. |
| 2026-07-21 | STEP 2A full-grid release after pilot | Pilot `15177060`; release array `15177203`; `/scratch/a/A.Otaifi/paratoric_selfdual_step2a_20260721/PRE_SUBMISSION_CHECK.md` | SUBMITTED | Remaining task IDs 0--447,464--527 released with max 64 concurrent tasks; pilot IDs excluded. Full measured budget 40.7 task-hours, guarded at 61.1. Concise-alert submission failed on transient DNS with no notifier job. |
| 2026-07-21 | STEP 2A design, cost, and pilots-before-arrays gate | `/scratch/a/A.Otaifi/paratoric_selfdual_step2a_20260721/PRE_SUBMISSION_CHECK.md`; pilot readout `/scratch/a/A.Otaifi/paratoric_selfdual_step2a_20260721/readout_pilot15177060_interval`; `/scratch/a/A.Otaifi/paratoric_selfdual_step2a_20260721/submit_selfdual_step2a_scan_20260721.sbatch`; Slurm array `15177060` | PILOT PASS / RELEASE | 16/16 chains complete with 63,900 samples, 490--664s; common I5+stationarity by 200k; both probability dual pairs pass at every eligible depth (max z=2.10583830667); no persistent energy gap; unimodal histograms are INCONCLUSIVE. Measured all-grid cost 40.7 task-hours; release budget 61.1 task-hours. Submit remaining IDs 0--447,464--527. |
| 2026-07-21 | All-nine-point L=10/L=20 raw-ratio crossing with with/without sensitivity and interpolation systematic | `doc/figures/linsel_no_pt_l10_l20_restored_diagnostics_20260721.tsv`; `doc/figures/linsel_raw_ratio_crossing_no_pt_l10_l20_restored_20260721.png`; `doc/benchmark_report.md` sections 4--6 | PREDICTION FAILED / PROVISIONAL | Linear h=0.3244097798 both ways; honest all-root interpolation spread 0.0112154963 reaches Linsel band, so neither an upward shift nor a several-sigma tension is established. |
| 2026-07-20 | PI CORRECTION: round-trips NOT NECESSARY for low tau; run manual F3/F7 + harness G1.2 bars are WRONG as written | PI physics call, UPDATE 2026-07-20h | RULE CORRECTED | Gate is measured tau at the rung of interest. All 4 PT NO-GO calls used a non-gating metric and never measured tau at h_c — that evidence is weaker than treated. First-order caveat: round-trips likely necessary again at a coexistence barrier. |
| 2026-07-20 | PT operational calibration PROMOTED to explicit 2B deliverable (swap-period optimum, sweeps-to-decorrelate vs L, harmonic acceptance, excursion stats) | PI instruction, UPDATE 2026-07-20h sec 5 | SCOPE ADDED | Bank the ladder-design experience in a pt_architecture.md calibration table. |
| 2026-07-20 | L=20 VOID **CLEARED** on the corrected estimator; STEP 3 CANCELLED as scoped (~170-340 task-hrs saved) | Jarvis STEP 0 `blocking-validity`, 4 review rounds — `doc/figures/linsel_blocking_validity_l10_l20_20260720.md`: h=0.339574 p 0.026->0.20 z=0.42; h=0.305629 p=0.33 z=0.33 | CLEARED | Only TAU stays unresolved there, and no downstream decision reads it. Jarvis asked the question that cancelled the job. |
| 2026-07-20 | RATIFIED Jarvis's sqrt(7/8) catch: jackknife SE of a MEAN is an algebraic identity of the ddof=1 SE, so the between/bootstrap ratio is 0.935 for ANY data | Jarvis report 2026-07-20 STEP 0 | ACCEPTED | Driver must require an explicit observable and refuse the ratio for a linear one. |
| 2026-07-20 | Feynman decision-rule error owned: pre-committed on a MECHANISM (error inflation) when the decision only needed an OUTCOME (do the gates pass) | UPDATE 2026-07-20g sec 2 | CORRECTED | Third position in one day; 20d's estimator claim and 20f's error-bar refutation are BOTH right, about different quantities. |
| 2026-07-20 | Crossing 0.3244098 +/- 0.0000817 NOT promoted; re-derive free with all nine L=20 points | `doc/figures/linsel_no_pt_l10_l20_diagnostics_20260720.tsv` | HELD | Excluded h=0.339574 was the steepest L=20 interval; quoted bar is 37x tighter than our own PT-era crossing spread. |
| 2026-07-20 | Feynman's error-bar defect claim REFUTED by his own tau-free probe; L=20 VOID CONFIRMED REAL; STEP 3 AUTHORIZED, STEP 0 CANCELLED | between-chain vs bootstrap error on jobs `15149119`/`15159599`: 0.77x at L20 h=0.305629, 0.99x at h=0.339574, ~1x at all 6 other points | REFUTED / CORRECTED | Retired session's void call was right. Blocking curve had been read at 8-16 blocks. |
| 2026-07-20 | Step 1 landed; QUARANTINE `hc_raw_ratio_apparent_z = -7.65` and `tau20/tau10 = 0.00498`; crossing 0.32441 NOT promoted | `doc/figures/linsel_no_pt_l10_l20_diagnostics_20260720.tsv` + `..._crossing_no_pt_l10_l20_20260720.png` | PARTIAL / QUARANTINED | tau_int is the wrong instrument for z (survives 20f); crossing excludes the L=20 tau peak h=0.339574, re-derive after step 3. |
| 2026-07-20 | Channel trimmed: pre-2026-07-18 sections archived | `doc/archive/feynman_thesis_tojarvis_archive_pre_20260718.md`; pre-trim state = git `2124149` | HOUSEKEEPING | Live channel 1726 -> 910 lines; no live directive moved; durable conclusions already in canonical docs. |
| 2026-07-20 | REFINEMENT 20e: Step 1 in flight lands FIRST, Step 0 becomes immediate next action; triage rule = MEANS FINE, ALL ERROR BARS SUSPECT; Step 0 given a tau-free between-chain cross-check | Feynman — doc/feynman_thesis_tojarvis.md UPDATE 2026-07-20e | ORDER REVISED | Quarantine every tau/Neff/error bar/tau-ratio from Step 1; keep means, curve shapes, crossing location, sector-exchange rate. |
| 2026-07-20 | FRESH-EYES REVIEW of 20b/20c by new Feynman session: 2A+2B RATIFIED with additions; step 3 CONDITIONAL; step 4 INVALID as specified | Feynman — doc/feynman_thesis_tojarvis.md UPDATE 2026-07-20d | REVIEWED / GO with corrections | Dispatch unblocked. New STEP 0 (free) blocks the rest. |
| 2026-07-20 | TAU ESTIMATOR DEFECT: IPS `integrated_autocorrelation_time` truncates before the slow tail; underestimates by >=4x at the L=20 tau peak | Direct read of jobs `15149119`/`15159599` HDF5: blocking curve at L=20 h=0.339574 = 102/288/881/2289/3094/3300 sweeps (block 16k->8M, still climbing) vs IPS 827 | DEFECT CONFIRMED | Fix analyzer (blocking plateau + tau-UNRESOLVED flag); all prior tau/Neff/error bars suspect toward overconfidence. |
| 2026-07-20 | L=20 'void' is an ANALYSIS defect, not a data defect — control h=0.305629 has a FLAT blocking plateau (~44 sweeps) and full-ladder p=0.0528 > 0.05 | Same; z=4.68 is 1 of 5 checkpoints (others 0.28/1.13/0.47/0.46), slope-z=1.57 not significant | VOID DISPUTED | Step 3 (170-340 task-hrs) DOWNGRADED to conditional on the free re-analysis. No checkpoint exists for `15159599`, so it would be a full rerun. |
| 2026-07-20 | L=20 tau peak at h=0.339574 = independent DYNAMICAL estimate of the finite-L pseudo-critical point; h=0.335363 is NOT our finite-L h_c | tau scan across the 9-point window, both sizes; agrees with PT crossings 0.3408-0.3441, above Linsel's extrapolated 0.335(2) as FSS requires | NEW RESULT / FREE | Add to benchmark_report.md; recompute the x13.2 sector-exchange ratio at each size's OWN peak. |
| 2026-07-20 | Feynman session RETIRED by PI; migration prompt rewritten; Jarvis dispatch HELD pending fresh-eyes review | PI instruction — doc/feynman_migration_prompt.md (rewritten in place, 2026-07-20) | HANDOFF / HOLD | PI retired the 2026-07-18 Feynman session after four errors in one session, three caught by the PI or Jarvis rather than by Feynman: (1) winding-sector ergodicity hypothesis REFUTED by the sector-exchange probe; (2) authored a CIRCULAR calibration instruction (extrapolate a depth law from ONE size, which forces assuming the exponent) that nearly cost ~3,919 task-hours; (3) overstated benchmark-line z as "sets the M2 budget" when it is only a best-case lower bound (a first-order MCP slows exponentially, not as a power law); (4) parked PT while ASSUMING it delivers a speedup that was never measured (S0-S6 validated the MECHANISM, not the BENEFIT). PATTERN recorded for the successor: asserts from assumption, then expensive runs discover the assumption was wrong; every unblock came from a CHEAP DIRECT PROBE (calibration unit test, marker test, sector-exchange counter, depth-ramp). Migration prompt rewritten with current state, the never-run-at-the-MCP scope trap, the quantified cost reality, four new non-negotiable rules (drifting control voids a readout; checkpoint ladders geometric with first point well below convergence; never calibrate a scaling law from one size; prefer choice-independent observables), and the expanded mistake record. JARVIS DISPATCH IS HELD: the successor's FIRST task is a fresh-eyes review of the live directive (feynman_thesis_tojarvis.md UPDATES 2026-07-20b + 2026-07-20c) and either ratification or correction in a new UPDATE block, BEFORE the PI sends the bare "go". |
| 2026-07-20 | NEW STEP 2B: MEASURE PT's speedup on the benchmark line — never measured, and only measurable HERE | PI catch + Feynman — see doc/feynman_thesis_tojarvis.md 2026-07-20c | GAP FOUND / GO | GAP (Feynman's): after Decision 1, PT was parked and "resume the self-dual PT path toward the MCP" quietly ASSUMED PT delivers a speedup, never measured. S0-S6 validated the PT MECHANISM (swaps correct); it says NOTHING about whether PT reduces autocorrelation — separable claims, like swap-WEIGHT vs swap-PARTNER. Every PT run so far either failed transport (4 attempts, 0 RT) or was an L=8 smoke test => PT speedup factor UNMEASURED, and the MCP deployment rests on faith. WHY NOW: speedup=tau_noPT/tau_PT needs a trustworthy no-PT baseline; we have exactly one (L=10 job 15149119, tau_int(h_c)~10,792, Neff 1848, controls held), L=20 after step 3. At the MCP a no-PT reference will NEVER exist (no-PT there is what is infeasible) => possible NOW, impossible LATER. NOT a 5th ladder attempt: round-trips NOT required and NOT a gate (an isolated rung just returns speedup 1); h_c hole already characterized by the marker test; depth now known from the no-PT ramp. METRIC (cost-normalized or PT gets flattered): (i) tau_int/Neff per rung vs no-PT at the SAME (h,L); (ii) DECIDING NUMBER = at EQUAL total core-hours, does PT with N rungs beat N independent no-PT chains at the same N h-values, in Neff per core-hour; (iii) speedup AT h_c called out separately from the ladder average; (iv) real G1.3 physics agreement (does PT reproduce the same crossing as no-PT and Linsel) at a non-trivial point unlike the L=8 original; (v) round-trips = context only. SPEC: L=10, beta=10, lmbda=0.3, PT ON, reuse Lane-2 14-rung ladder (do NOT redesign, placement is not what is tested), ~16M thermalize + >=200 tau at slowest rung, ~20M sweeps, ONE walltime block (PT+checkpoint THROWS), ~20-30 task-hours; repeat at L=20 after step 3 for the speedup TREND. RULE: speedup@h_c >>1 => deploy PT at MCP, fold measured factor into M2 budget; ~1 => PT does not help where the barrier is, will not rescue the MCP, beta-bridge/worm move UP and we do NOT spend MCP-scale compute on a plain PT ladder. ORDER: step 1 free first; 2A (locate s*) and 2B (this) in PARALLEL; step 3 unlocks 2B at L=20; step 4 lowest value. |
| 2026-07-20 | CORRECTION: benchmark-line z is NOT the M2 budget; s* still unmeasured; locate s* PROMOTED above the Ising-line z-fit | PI catch + Feynman — see doc/feynman_thesis_tojarvis.md 2026-07-20b | CORRECTION / GO | FEYNMAN CORRECTION: 2026-07-20 claimed measured z "sets the M2 budget" — too strong. ALL benchmark runs (L=10 pilot/depth-ramp, L=20, proposed L=30) are at lmbda=0.3 = Linsel ISING line, OFF self-dual, CONTINUOUS 3D Ising. A z fitted there is the CONTINUOUS-case cost law only; if the MCP is FIRST-ORDER the slow mode is EXPONENTIAL tunneling tau~exp(2*sigma*L^2), which no continuous-line z extrapolates to => Ising-line z is a BEST-CASE LOWER BOUND, necessary not sufficient; does NOT retire the beta-bridge/worm ranking. FOR THE RECORD: we have NEVER run at the MCP. Benchmark point (h,lmbda)=(0.335,0.30) vs MCP (s*,s*), s*~0.33-0.35 = Linsel PRIOR still UNMEASURED; near-identical h, different lmbda = live trap. L=10-30 is benchmark-scale; MCP FSS targets L~120-192. Only post-fix self-dual work is L=8 duality checks at s=0.22; job 15034719 s*-scan predates the fixes. FINAL ORDER (supersedes 2026-07-20 sec 6): (1) FREE existing-data tau_int + L=10/20 overlay/crossing with bootstrap bars = benchmark deliverable; (2) PROMOTED locate s* on self-dual line, wide-s, small L=8-16, both directions, depth lessons applied, PLUS energy histograms as a cheap early first-order tell under the stated ASYMMETRY (bimodality present=strong first-order evidence; absent=INCONCLUSIVE, never report as continuous); (3) clear L=20 void (h=0.305629+h=0.339574 to 128M, geometric ladder); (4) DEMOTED/optional Ising-line z-fit at L=12/14/16, labelled best-case bound. L=30 HOLD; L=24 = ~1/3-cost alternative if a 3rd size is later needed. |
| 2026-07-20 | L=20 VOID (control drifted); L=30 REJECTED on circular calibration; measure tau_int, do not extrapolate | Jarvis job `15159599` + `/scratch/a/A.Otaifi/paratoric_linsel_no_pt_l20_depth_ramp_20260719/DEPTH_LAW_AND_DESIGN.md` + Feynman — see doc/feynman_thesis_tojarvis.md 2026-07-20 | VOID / HOLD / GO(cheap) | L=20 64M: cold/hot z<3 everywhere and sector exchanges healthy (no trapping), BUT control h=0.305629 never reaches joint z<3+stationary through 64M => VOID by Feynman's own 18c rule, not merely ambiguous. CIRCULARITY (Jarvis, correct, and it was FEYNMAN'S spec error): 64M was chosen by assuming z=2, so recovering z=2 confirms nothing; a 2-point power law also has ZERO residual DOF. Worker's refusal to submit L=30 = correct judgment, saved ~3,919 task-hours. NEW MEASURED SIGNAL (Feynman, from existing data): sector-exchange RATE at h_c falls x13.2 from L=10 (1 per 171 sweeps) to L=20 (1 per 2,252 sweeps) => apparent exponent ~3.7 NOT 2 => z=2 prior is OPTIMISTIC and the proposed 144M at L=30 is plausibly ~2x under-resourced (cost ~L^(3+z): L20->30 x7.6 at z=2 vs x15.3 at z=3.7). Strong hint not measured z (threshold-crossing indicator, distribution changes with L). DECISION: reject option (b) submit-on-guess; L=30 HOLD. GO on 3 cheap steps (~100 task-hours vs 3,919): (1) FREE from existing L10/L20 HDF5 — tau_int at h_c both sizes, plus L=10/20 raw-ratio overlay vs Linsel Fig.4a with crossing + bootstrap bars (= the actual benchmark deliverable); (2) short h_c-only runs at L=12/14/16 (~300 tau each) => fit tau_int(L) over 5 sizes with real DOF and honest error on z; (3) targeted extension of ONLY h=0.305629 + h=0.339574 at L=20 to 128M on a geometric ladder to clear the void. WHY z OUTRANKS L=30: L20->L120 is x7,776 at z=2 vs x169,503 at z=3.7 — quantifies the campaign premise that no-PT local cannot reach the MCP, sets the speedup any algorithm must deliver + the M2 budget. If a 3rd size is later needed, L=24 costs ~1/3 of L=30 for the same crossing-drift. STANDING RULE ADDED: checkpoint ladders must be geometric with FIRST checkpoint well below expected convergence (~1/8); L=20 opened at 8M, already converged, so no relaxation shape remained to fit. |
| 2026-07-18 | L=10 depth-ramp BINARY = PASS: depth-solvable critical slowing, NOT sector trapping; WORM NOT mandatory | Jarvis job `15149119` (72/72 COMPLETED, 1,592,000 samples/chain verified in HDF5) — see doc/feynman_thesis_tojarvis.md 2026-07-20 sec 1 | PASS / DECISION 1 HOLDS | All 7 decisive h-points cold/hot z<3 on ratio AND density at 40M including h_c exactly (0.041/0.767); both controls stayed converged (constant-fit p=0.97/0.84) so readout valid; increments decelerate (h=0.318989 late-window 16-40M slope-z 0.609 vs overall 3.24). DIRECT MECHANISM PROBE KILLS THE HYPOTHESIS: every chain at every h shows ~217k-247k sector-exchange events; the specified FAIL signature (zero in-window exchanges while controls exchange) is absent => winding-sector ergodicity breaking REFUTED (Feynman 18b hypothesis (b); 3rd refuted Feynman hypothesis this campaign). tau_int(h_c)~10,792 sweeps consistent with pilot's ~7,375 (Neff 1848 vs 136). No 80M tiebreak needed. WORM stays optional; beta-bridge ranking unchanged. Credit Jarvis: sector-exchange instrumentation + false-timeout traceback (TIMEOUTs were an unrelated project). |
| 2026-07-20 | Linsel no-PT L=20 depth-law readout | Slurm array `15159599`; `/scratch/a/A.Otaifi/paratoric_linsel_no_pt_l20_depth_ramp_20260719/DEPTH_LAW_AND_DESIGN.md` | AMBIGUOUS / L30 HOLD | 72/72 full chains; final cold/hot z<3 and every chain exchanges sector, but h=0.305629247317 control and h=0.339573841268 decisive point have no nontrivial stationary suffix through 64M. The automatic last-point pass is degenerate. No actual N20 calibration means no two-point z fit and no fitted-exponent L30 submission. |
| 2026-07-19 | Linsel no-PT L=20 depth-law scale-up | Slurm array `15159599`; `/scratch/a/A.Otaifi/paratoric_linsel_no_pt_l20_depth_ramp_20260719/DEPTH_LAW_AND_DESIGN.md`; `/scratch/a/A.Otaifi/paratoric_linsel_no_pt_l20_depth_ramp_20260719/PRE_SUBMISSION_CHECK.md` | SUBMITTED / READOUT PENDING | L=10 actual plateau entry 16M at worst low-h point; adopt local-update z=2, hence L20=64M. Exact L10 nine-point grid, 4 cold+4 hot/h, checkpoints 8/16/32/48/64M, no PT, 2G/task, 24h. Expected 10-12h/chain, 720-864 core-hours. L30=144M/52-60h per chain is PROPOSED ONLY and held until L20 validation. |
| 2026-07-18 | FINALIZED L=10 no-PT depth-ramp submission | Slurm array `15149119`; `/scratch/a/A.Otaifi/paratoric_linsel_no_pt_l10_depth_ramp_20260718/PRE_SUBMISSION_CHECK.md`; launcher and `design.tsv` in the same root | SUBMITTED / READOUT PENDING | Exact locked 9 h x 8 chains = 72 tasks; seeds `94018000-94018071`; 40M continuous sweeps; exact 2/4/8/16/24/32/40M full-series windows; `--full_time_series 1` plus direct `percolation_probability`; no PT; 1G and 3h. Do not read out until complete. |
| 2026-07-18 | FINALIZED GO: L=10 no-PT depth-ramp = the single decisive thread (depth-solvable vs WORM-mandatory) | Feynman UPDATE 2026-07-18c + Jarvis marker test (Hyp A confirmed) — doc/feynman_thesis_tojarvis.md 2026-07-18c | GO / SPEC LOCKED | Marker test IN: h_c is a localized near-wall on the L30 PT ladder (0.235% accept, 4/67 replicas cross, 99.9% return) — confirms localized barrier WITH PT but NOT whether local updates alone can solve it. Depth-ramp settles that. SPEC: L=10, beta=10, lmbda=0.3, mu=J=1, z-basis, NO PT (no PT+checkpoint block => single walltime block; 4M≈5-7min => 40M≈50-70min/chain). h decisive={0.318989,0.322,0.326,0.330615,0.335363,0.339574,0.343521} + positive controls {0.305629,0.347399} (must stay converged or readout void); 4 cold+4 hot; continuous to 40M, checkpoints 2/4/8/16/24/32/40M. READOUT: cold/hot I5 z on raw ratio AND base density; stationarity via increment sequence (decelerating=plateau, constant=algebraic drift); DIRECT sector probe = count winding/percolation sector-exchange events (zero in-window while controls exchange = smoking-gun trapping). BINARY: PASS(z<3 both obs + decelerating)=depth-solvable, Decision 1 holds, scale L=20/30 at matched depth; FAIL(z>>3 non-decelerating and/or zero in-window exchanges)=winding sector non-ergodic => WORM MANDATORY, reorders M0. Pre-authorized tiebreak: if decelerating-but-not-yet-z<3 at 40M, extend SAME chains ONCE to 80M; any other ambiguity or a drifting control STOP+report. One ramp, then report the binary. |
| 2026-07-17 | Linsel exact-size Lane-2 overlay | jobs `15140239`/`15140247`/`15140246`; `/scratch/a/A.Otaifi/paratoric_pt_linsel_exact_sizes_20260717/design.tsv`; `/scratch/a/A.Otaifi/paratoric_pt_linsel_exact_sizes_20260717/PRE_SUBMISSION_CHECK.md` | SUBMITTED / READOUT PENDING | Exact L={10,20,30}; N={14,40,67}; thermalization={125k,500k,1.125M} sweeps; 1M production sweeps each. Preflight actual-vs-intended diff PASS and L20 actual job-15118635 ladder equality PASS before submission. Fresh dispatch must establish strict RT>0, link health, equilibration/statistics, and same raw-ratio trend/crossing as Linsel Fig.4a. |
| 2026-07-17 | sharp h_c-targeted Linsel ladder | jobs `15134326`/`15134327`/`15134328`; `/scratch/a/A.Otaifi/paratoric_pt_linsel_hc_targeted_20260717/diagnostics/`; `/scratch/a/A.Otaifi/paratoric_pt_linsel_hc_targeted_20260717/ladders/` | SUBMITTED / READOUT PENDING | Production-only trajectory motion confirmed one unique minimum link per L; subdivided that link and both immediate shoulders by factor 3, increasing 60/72/90 to 66/78/96 ranks without changing any rung outside the local band or the healed seam; one-node memory is 132G/156G/192G; fresh dispatch tests >=3 strict round-trips using production-only motion, never combined counters. |
| 2026-07-17 | expanded acceptance-informed Linsel ladder | jobs `15126362`/`15126364`/`15126363`; `/scratch/a/A.Otaifi/paratoric_pt_linsel_acceptance_informed_20260717/spacing_check.tsv`; `/scratch/a/A.Otaifi/paratoric_pt_linsel_acceptance_informed_20260717/window_density_check.tsv` | SUBMITTED / READOUT PENDING | Increased 40/48/60 to 60/72/90 rungs; both h=0.340 and h=0.435 probe gaps and both full weak-region mean/max gap checks are strictly smaller than the original resourced ladder for every L; one-node memory is 120G/144G/180G; fresh dispatch tests the >=3 strict-round-trip bar. |
| 2026-07-16 | D3 PM single-iteration mitigation | jobs `15118635`/`15118636`/`15118637`; `/scratch/a/A.Otaifi/paratoric_pt_linsel_pm1_20260716/spacing_check.tsv` | FAIL / STOP | h=0.340 spacing coarsened by factors 1.224226/1.253381/1.260187 for L20/L24/L28 although h=0.435 seam spacing improved by factors 0.560972/0.575019/0.574828; no benchmark jobs submitted. |
| 2026-07-07 | pre | code review (chat) + performance_report.md | — | plan approved by user |
| 2026-07-12 | D2/D3 done (parallel worktrees) | Jones (D2, `scripts/pt_diagnostics.py`) + James (D3, `scripts/adaptive_ladder_pm.py`), isolated git worktrees, merged commits `6aeee94`/`d158822`/`9a780ab` | DONE | D2: strict round-trip count on real job 14982619 data (6) is LOWER than the simple endpoint-touch count (10=6+4) - confirms H2's concern that touches overcount genuine round-trips; tau_ergodic strict mean 3.25 sweeps. D3: PM ladder builder ported from code_ruby_duy_pinning_lab, all 4 robustness guards tested; honestly reports PM alone does NOT solve a synthetic bottleneck case (matches H5's warning). |
| 2026-07-18 | L=10 no-PT flag RIGHT: continuous => depth MUST work IF sectors connect; DEPTH-RAMP is the decisive probe | Jarvis job 15141215 + Feynman — see doc/feynman_thesis_tojarvis.md 2026-07-18b | STATEMENT / GO | L=10 no-PT: clean off-critical; h_c window [0.319,0.344] non-plateaued (ratio falls at 4M, z=3.71), cold/hot split (z up to 29.6), tau_int~7375 sweeps at h_c (300-500x). Finding REAL, refines not refutes Decision 1. KEY: continuous 3D Ising => NO thermodynamic barrier => cold/hot MUST converge + ratio MUST plateau at depth; z=29.6 is NOT metastability, it is either critical slowing OR winding-SECTOR ergodicity (cold/hot in different topological sectors, local updates connect sectors slowly). MECHANISM: global_single_spin_flip (sector-changing move) dead <1e-4 (expected, but its consequence = sector mixes only by slow string accumulation) => plausibly why cold/hot unmet at 4M. 100-1000x slower than Linsel (~1e4 sweeps) is EXPECTED: our local update is autocorrelation-bound (campaign premise). DECISIVE TEST: ramp h_c window L=10 to 20-40M (~1hr). PLATEAU+converge => depth-solvable, Decision 1 holds, calibrate depth, scale L=20/30. STILL DRIFTING at 40M => winding sector NOT ergodic under local updates even at L=10 => PT alone would NOT fix it (no sector-changing power) => WORM MANDATORY; biggest finding, reorders M0. ORDER: (1) depth-ramp = decisive; (2) marker test secondary. |
| 2026-07-18 | DECOUPLE benchmark from PT; marker test; beta-bridge = MCP plan of record (deferred) | Jarvis 4th RT failure + PI beta-bridge/marker ideas + Feynman — see doc/pt_architecture.md sec 9 + feynman_thesis_tojarvis.md 2026-07-18 | 3 DECISIONS | 4th PT RT failure at L=10-30 Ising benchmark, same cause: LOCALIZED severe acceptance hole at h_c (L30 0.24%; harmonic 6.3% vs median 21.5%). Jarvis harmonic-mean insight CORRECT: diffusion limited by WORST link (series), tau_RT~N^2/A_harmonic => median-based margin ~100x too small => it is a REAL h_c hole (specific-heat Var E enhancement), not only under-resourcing. (1) DECOUPLE: benchmark does NOT need PT (Linsel used local updates; PT already validated S0-S6) => close L=10/20/30 overlay with DEEP NO-PT cold/hot chains. (2) MARKER TEST (PI): instrument per-replica trajectory on a failed run; prediction if diagnosis right = all replicas pile at the single h_c link, none cross; if dribble everywhere = distributed => run longer. Cheap, decisive. (3) beta-bridge = PLAN OF RECORD for the MCP: add beta as 2nd coord, route via HIGH T = LOW beta (no transition => defeats critical slowing of ANY order); BEATS duality-odd (which escapes to still-critical Ising lines); feasible (coupled-pinning tempers beta w/ worldline rescaling); cost 2D ladder + rescaling + beta-swap weight; ranking beta-bridge>worm>duality-odd; BUILD IFF self-dual PT stalls at s* with FIRST-ORDER signature (benchmark continuous => does not justify it). ORDER: close benchmark no-PT -> marker test -> resume self-dual PT toward MCP, beta-bridge on the shelf. |
| 2026-07-17 | 13f RESOLVED: Binder CONVENTION not wrong channel; channel CONFIRMED; 2 parallel lanes green-lit | Jarvis raw-ratio re-plot + Feynman verification — see doc/feynman_thesis_tojarvis.md 2026-07-17b | RESOLVED / GO | Trend inversion was plotting the bounded Binder U=1-<P^4>/(3<P^2>^2) instead of the RAW RATIO <P^4>/<P^2>^2 that Linsel plots (their y-axis is LOG 10^0-10^2 => can ONLY be the unbounded ratio; U<=2/3 can't be log-plotted). Code stores the ratio (bootstrap.hpp:164); earlier plot applied a spurious 1-ratio/3. Now trend INCREASES with h, matches Fig.4a; crossings unchanged (monotonic). CHANNEL ALSO CONFIRMED: increasing ratio => P robust at small h / intermittent at large h = deconfined->confined = plaquette IS the deconfinement POP (complementary channel would DECREASE). Feynman's 13f 'wrong channel' hypothesis was wrong — it was the convention; credit Jarvis. Confirmatory (free): raw POP vs h high@small-h. GREEN-LIGHT 2 parallel lanes: L1 round-trips (per 2026-07-17: STOP redesigning, cap N at A~23% ~40/48/60, smooth profile, MEASURE attempts vs 3*tau_RT, RUN LONGER not patch, verify submitted==intended); L2 rerun at Linsel EXACT L=10/20/30 for overlay. Promote benchmark_report off PROVISIONAL only after RT>0 AND L=10/20/30 overlay matches trend+crossing. |
| 2026-07-17 | 0 round-trips likely RUN TOO SHORT not ladder design; STOP redesigning; adding rungs is counterproductive | PI insight + Feynman — see doc/feynman_thesis_tojarvis.md 2026-07-17 | STOP / DIAGNOSE | 3rd ladder attempt still 0/0/0 RT. KEY: healthy acceptance != round-trips; tau_RT ~ N^2/A (diffusive RW), need run >= few x tau_RT; below that 0 RT is EXPECTED for a HEALTHY ladder. L28 N=92 A~0.2 => tau_RT~85k attempts/round-trip, ~250k for 3. THE DIAGNOSTIC (likely unchecked): total swap attempts vs ~250k. TRAP: adding rungs to fix acceptance GROWS N and tau_RT~N^2 (opt ~40/48/60 -> actual 62/74/92, h_c over-refined to 72-76% >> A~23% optimum) => design fought itself, ~2.4x worse diffusion. '13-20% accept but zero flow' = too-short-window artifact, not a block. FIX: (1) measure attempts vs 3*tau_RT — short => run longer, stop redesigning; (2) cap N at A~23% optimum, ONE smooth profile ~sqrt(VarE), size DOWN; (3) verify submitted==intended (62-vs-66 gap real); (4) run >3*tau_RT (~few hrs at L28). Jarvis (a) right if sized down; (b) false alarm at L20-28 (only bites at L=120). FLAG OUTRANKING ALL: is 13f CHANNEL CHECK (POP-vs-h both channels, trend inversion) done? If not it STILL comes FIRST — these 3 attempts may be round-trip work on the WRONG channel. Binder 0.3330/0.3376/0.3410 doubly-untrustworthy. Credit: 3-attempt loop-containment stop was correct. |
| 2026-07-13 | TOP PRIORITY: TREND INVERSION — our U_P decreases, Linsel's increases => likely WRONG CHANNEL | PI catch + Feynman verification (bootstrap.hpp:164) — see doc/feynman_thesis_tojarvis.md 13f | STOP / RE-ORDER | Our U_P(h) DECREASES with h; Linsel's INCREASES. A trend inversion is a DEFINITION/OBSERVABLE problem and outranks the 3-sigma question AND the round-trip work (a mixing fix on the wrong observable buys nothing). KEY LOGIC: a crossing is INVARIANT under any MONOTONIC map, so matching crossings (0.34 vs 0.335) + inverted trend is EXACTLY the fingerprint of plotting a monotonically-DECREASING function of their observable — the number agreeing is NOT evidence. NOT a Binder-formula mismatch (verified: ours = e4/(e2^2) = <P^4>/<P^2>^2, bootstrap.hpp:164; Linsel Fig4a y-axis is log 10^0-10^2 = same unbounded-ratio form). LEADING HYPOTHESIS: we plot the DUAL/COMPLEMENTARY channel (the two z-channels percolate on OPPOSITE sides => inverted trend, same crossing). FEYNMAN DEBT: the 'plaquette = deconfinement POP' call was made on the CONTAMINATED under-thermalized data and self-flagged as provisional ('identify the channel from the POST-FIX benchmark') — never done. Treat as UNVALIDATED. FREE DECISIVE CHECK (existing data, do FIRST): plot raw POP vs h for BOTH channels; Linsel says deconfined(small h) => electric percolates, so the correct channel has POP~1 at small h -> ~0 at large h (=> U_P increasing). Whichever matches IS the electric channel; if it's the other, revisit run_manual A8 + benchmark_report sec 3. ORDER: (1) POP-vs-h both channels; (2) redo anchor plot with RIGHT channel + Linsel's L=10/20/30; (3) compare TREND first, crossing second; (4) THEN round-trips. |
| 2026-07-13 | Benchmark MISMATCH = wrong L set (HIGH PRI); bridge parked; plaquette-hardcoding REFUTED | Feynman verification + PI questions — see doc/feynman_thesis_tojarvis.md 13e | DIRECTIVE | (A) HIGH PRI: crossings 0.3406-0.3437 vs Linsel 0.335(2) is NOT yet a benchmark. Not a convention error (ratio 1.017, no clean factor; crossings drift monotonically toward 0.335 as L grows: 0.34373->0.34155->0.34059 = finite-size corrections in the RIGHT direction). REAL MISMATCH: we ran L=20/24/28, Linsel ran **L=10/20/30** => different curves => plots cannot be overlaid. Also a finite-L crossing != h_c; their 0.335(2) is EXTRAPOLATED (apples-to-oranges). ACTION: (1) round-trips first (3 numbers from 13d); (2) rerun at L=10/20/30 (cheap); (3) overlay with their Fig 4a — bar is 'peer sees same trend/crossing'. Only then promote benchmark_report sec 5 off PROVISIONAL. (B) PARKED: PI's (beta,Gamma)-style bridge — principle transfers but beta is LOCKED (beta=L is the FSS design); the right bridge coordinate is the duality-odd (h-lmbda) since the 1st-order segment lives ONLY on the self-dual line and the flanks are continuous; stays a CURVED 1D ladder not a 2D grid; D4 is the enabler (higher value than 'benchmark-only'). Do NOT build now — zero evidence of an MCP bottleneck (current one is at lmbda=0.3, continuous Ising, H5 cannot apply). Contingency only. (C) REFUTED: all 4 tuple updates (2609/2666/2868/3185) sample the tuple RANDOMLY via get_random_plaquette_index()/get_random_vertex() — NO hardcoded plaquette; concern likely from loophex (different code). Adjacent+real+LOW PRI: staggered_imaginary_times OBSERVABLES (lattice.cpp:3239/3262) use a 1-sample random plaquette/star estimator — noisy, unused, park. |
| 2026-07-13 | Resourced benchmark: ladder ALIVE + Binder crossings TIGHTEN to 0.3406-0.3437 (anchor); (a)/H5 ruled out on physics | Jarvis jobs 15118489/15118635/36/37 + Feynman — see doc/benchmark_report.md + feynman_thesis_tojarvis.md 13d | PROGRESS / 3 QUESTIONS | GOOD: calibration confirms the law (fitted exponent 1.69 vs predicted 1.5; excess = specific-heat enhancement L^(alpha/nu) near h_c — USE 1.69 for L=120 extrapolation, makes it worse: x20.6 not x14.7). Median acceptance 20-23% at all 3 L, 0 dead links (was 14/14 dead at L28). Binder crossings 0.3406-0.3437 (spread 0.003) vs earlier 0.350/0.331/0.327 (spread 0.023) = 8x tighter, near published 0.335(2) => our ANCHOR that the code is physical. Still 0 strict round-trips. RULED OUT (a)/H5: this benchmark is at lmbda=0.3, OFF the self-dual line = CONTINUOUS 3D Ising (Linsel nu=0.65); H5 first-order is a self-dual-line MCP phenomenon. Right warning, wrong line. LEADING HYPOTHESIS (e), not on Jarvis's list: RUN TOO SHORT — tau_RT~N^2/A; ladder 15->60 rungs => (60/15)^2=16x more attempts; L28 needs ~16,400 attempts/round-trip (L20 ~7,600), >=3 for GO. 0 RT is the EXPECTED output of a HEALTHY ladder if run length wasn't scaled 16x; and a too-short run reproduces the sharp f_up step exactly. 3 NUMBERS BEFORE PM/respacing: (1) total swap attempts/replica vs 7.6k/16.4k; (2) acceptance AT the bottleneck link (median hides a 0.5% link); (3) WHERE the bottleneck is — h_c or the dense/coarse seam (a dh discontinuity = artifact of Feynman's own prescription; fix = smooth dh~1/sqrt(VarE)). ALSO: ONE anchor plot (U_P vs h, L=20/24/28, band at 0.335(2)) + doc/benchmark_report.md FILED (Feynman) — Jarvis fills plot path + updates when RT land. |
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
