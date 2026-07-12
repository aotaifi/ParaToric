# ParaToric Scripts

Reusable project scripts live here. Prefer extending these entrypoints before
adding new one-off analysis code.

- `ensemble_diagnostics.py`: Python CLI for multicritical harness I4/I5
  ensemble checks on ParaToric `obs.h5` files. It reuses the shared
  `/project/theorie/a/A.Otaifi/ai-workflow/qmc_common/hdf5_readers.py`
  ParaToric observable reader and adds cross-chain R-hat, mean-z, tau-ratio,
  Neff, and cold/hot agreement diagnostics.
- `pt_diagnostics.py`: Python CLI for offline parallel-tempering ladder health
  diagnostics from `ladder_index_by_sample`, including strict endpoint-cycle
  round-trips, tau_ergodic in sweeps, and per-rung up/down flow.
- `adaptive_ladder_pm.py`: Python CLI for D3 PM/acceptance-mode PT ladder
  proposals from measured per-link acceptance rates. It adapts the reference
  `temp_grid::adapt_betas` spacing logic to ParaToric's `s = h = lmbda`
  ladders, with smoothing, alpha blending, minimum spacing, and a fixed
  iteration cap.
