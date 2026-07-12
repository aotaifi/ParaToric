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
- `pt_diagnostics.py`: Python CLI for offline parallel-tempering ladder health
  diagnostics from `ladder_index_by_sample`, including strict endpoint-cycle
  round-trips, tau_ergodic in sweeps, and per-rung up/down flow.
- `adaptive_ladder_pm.py`: Python CLI for D3 PM/acceptance-mode PT ladder
  proposals from measured per-link acceptance rates. It adapts the reference
  `temp_grid::adapt_betas` spacing logic to ParaToric's `s = h = lmbda`
  ladders, with smoothing, alpha blending, minimum spacing, and a fixed
  iteration cap.
- `submit_thermalization_convergence_20260712.sbatch`: cold/hot production-CLI
  sampling-window convergence array for the adopted plaquette percolation gate
  at L=16,24,32. Each independent task thermalizes to one candidate depth, then
  records 100 samples at 25-sweep spacing; base -1-edge density is derived from
  `sigma_z` as `(1-sigma_z)/2`.
- `submit_linsel_percolation_benchmark_20260712.sbatch`: fixed-observable-pipeline
  rerun of the Linsel off-self-dual benchmark. It maps one L/h/cold-hot
  combination per array task, applies the validated per-L thermalization depth,
  and records 1000 plaquette POP/POS samples at 25-sweep spacing plus `sigma_z`.
