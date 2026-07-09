# ParaToric Scripts

Reusable project scripts live here. Prefer extending these entrypoints before
adding new one-off analysis code.

- `ensemble_diagnostics.py`: Python CLI for multicritical harness I4/I5
  ensemble checks on ParaToric `obs.h5` files. It reuses the shared
  `/project/theorie/a/A.Otaifi/ai-workflow/qmc_common/hdf5_readers.py`
  ParaToric observable reader and adds cross-chain R-hat, mean-z, tau-ratio,
  Neff, and cold/hot agreement diagnostics.

