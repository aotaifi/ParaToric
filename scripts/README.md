# ParaToric Scripts

Reusable project scripts live here. Prefer extending these entrypoints before
adding new one-off analysis code.

- `ensemble_diagnostics.py`: Python CLI for multicritical harness I4/I5
  ensemble checks on ParaToric `obs.h5` files. It reuses the shared
  `/project/theorie/a/A.Otaifi/ai-workflow/qmc_common/hdf5_readers.py`
  ParaToric observable reader and adds cross-chain R-hat, mean-z, tau-ratio,
  Neff, and cold/hot agreement diagnostics. Its default error mode recognizes
  ParaToric's autocorrelation-aware stationary-bootstrap `mean_error`; legacy
  IID errors can be corrected with `--stored-error naive`. The
  `binder-crossing` subcommand evaluates the strength Binder cumulant from raw
  series and linearly brackets adjacent-size crossings.
  Its `pt-binder-anchor` subcommand reconstructs fixed-h PT rung series and
  writes autocorrelation-aware Linsel crossing anchor plots: the default Binder
  cumulant U or, with `--quantity raw-ratio`, the raw ratio
  <P^4>/<P^2>^2 on a log scale.
  Its `crossing-extrapolation` subcommand fits supplied finite-size crossings
  linearly against the fixed geometric-mean convention
  `1/L_eff=1/sqrt(L1*L2)` and writes the benchmark extrapolation figure.
  Its `selfdual-energy-binder` subcommand reads STEP 2A independent-chain
  energy series, writes chain-jackknife Binder curves, and reports every
  size-pair crossing bracket (or an explicit `NONE`) in both bases.
  Its `pt-speedup` subcommand uses the same sample-aligned fixed-rung
  reconstruction, applies the blocking-plateau tau estimator to PT and no-PT
  series, reports censored speedup bounds honestly, and emits PT/no-PT raw-ratio
  agreement and optional L=10/L=20 crossing checks.
  Its `depth-ramp` subcommand reads exact-depth windows, reproduces the pooled
  leave-one-chain-out ratio/density cold-hot test, and reports the earliest
  checkpoint whose complete suffix passes constant-fit and slope stationarity.
  Its `no-pt-benchmark` subcommand reads independent stationary no-PT suffixes,
  defines raw-ratio tau from the ratio estimator's influence series, attaches
  delete-one-chain jackknife tau errors, and writes a hierarchical chain/block-
  bootstrap L=10/L=20 raw-ratio crossing plot plus its TSV source table. Use
  `--sensitivity-exclude SIZE:FIELD` to retain validated points in the primary
  curve while reporting a side-by-side exclusion sensitivity. The same mode
  reports an interpolation envelope from linear, PCHIP, natural-cubic, and
  degree 1--3 polynomial curves over three fit windows; multiple roots remain
  in the envelope rather than being silently discarded.
  Its `selfdual-step2a` subcommand reads the locked both-basis locate-s* scan,
  emits exact-depth I5/stationarity and both I2/B5 probability-pair tables,
  records cold/hot energy gaps with trend classification, and writes one
  pooled-plus-per-chain energy-histogram panel per `(L,s,basis)`. Its default
  windows are the non-overlapping geometric checkpoint intervals; `fixed`
  trailing windows remain available for sensitivity checks. A conservative
  KDE peak screen writes candidates for visual review; it never promotes the
  absence of a candidate into evidence for continuity. It also writes a
  cold/hot-labelled per-chain raw-ratio and influence-series-tau audit for the
  physical strength channel in each basis, with the I4 factor-2 tau gate, for
  the final window selected by `--window-mode`; `--spread-gate` defaults to
  0.05 and combines with the I4 factor-2 tau gate in the audit PASS/FLAG.
- `pt_diagnostics.py`: Python CLI for offline parallel-tempering ladder health
  diagnostics from `ladder_index_by_sample`, including strict endpoint-cycle
  round-trips, tau_ergodic in sweeps, per-rung up/down flow, and optional
  production-only per-link acceptance inferred from adjacent trajectory motion
  when one stored sample follows each alternating swap phase. It reports the
  median and harmonic-mean link acceptance separately and can write closed
  excursion-range distributions from each replica's initial rung.
- `adaptive_ladder_pm.py`: Python CLI for D3 PM/acceptance-mode PT ladder
  proposals from measured per-link acceptance rates. It adapts the reference
  `temp_grid::adapt_betas` spacing logic to ParaToric's `s = h = lmbda`
  ladders, with smoothing, alpha blending, minimum spacing, and a fixed
  iteration cap. `--target-count` resamples the PM-shaped physical interval
  when measured out-of-window rungs are dropped but the MPI rank count is fixed.
- `acceptance_informed_ladder.py`: Python CLI that infers a smooth physical-
  coordinate thermodynamic-length density from a measured ladder's complete
  per-link acceptance profile and builds a requested, potentially larger rung
  count at equal integrated density. Use it when additional ranks must remove
  multiple acceptance holes without PM's fixed-count zero-sum tradeoff. It can
  also sharply subdivide only complete links in a local measured bottleneck
  window while preserving every rung outside that window.
- `submit_thermalization_convergence_20260712.sbatch`: cold/hot production-CLI
  sampling-window convergence array for the adopted plaquette percolation gate
  at L=16,24,32. Each independent task thermalizes to one candidate depth, then
  records 100 samples at 25-sweep spacing; base -1-edge density is derived from
  `sigma_z` as `(1-sigma_z)/2`.
- `submit_thermalization_decisive_l32_20260712.sbatch`: eight-chain decisive
  L=32, h=0.335 cold/hot test. It keeps each trajectory continuous from 2M to
  8M sweeps and records plaquette POP/POS plus sigma_z every 25 sweeps, retaining
  300-sample windows at the 2M/4M/8M checkpoints for fresh-dispatch I5 readout.
- `submit_linsel_percolation_benchmark_20260712.sbatch`: fixed-observable-pipeline
  rerun of the Linsel off-self-dual benchmark. It maps one L/h/cold-hot
  combination per array task, applies the validated per-L thermalization depth,
  and records 1000 plaquette POP/POS samples at 25-sweep spacing plus `sigma_z`.
