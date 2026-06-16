# ParaToric Performance Report

## Current verdict

- Production QMC still uses the existing Boost-backed `Lattice`.
- A square-periodic `FlatSquareLattice` scaffold now exists for the next backend-integration step.
- The strongest confirmed performance signal is memory/layout: flat contiguous arrays are much cheaper than Boost graph storage for regular square lattices.
- The flat backend benchmark is not yet full QMC, but it strongly motivates wiring the flat square backend into production QMC behind a controlled selector.

## Performance commits

- `202b655` caches common lattice observable totals.
- `31001fb` adds compact production acceptance diagnostics in HDF5.
- `e52a935` replaces a linear flip-neighbor lookup in the spin-tuple combination update with binary search.
- `3603145` adds an opt-in flat lattice backend benchmark.
- `64e823d` documents performance benchmark results.
- `a63cbcc` adds the square-periodic `FlatSquareLattice` scaffold.

## Layout benchmark evidence

### Sustained 100-sweep benchmark

Job array: `14699151`

| L | backend | updates | construct_s | update_s | observe_s | maxrss_kb |
|---:|:---|---:|---:|---:|---:|---:|
| 400 | Boost | 32,000,000 | 1.55904 | 43.875 | 0.000000533 | 246,232 |
| 400 | flat | 32,000,000 | 0.00647945 | 0.352766 | 0.00146572 | 11,244 |
| 1000 | Boost | 200,000,000 | 6.73336 | 222.455 | 0.000000361 | 1,441,248 |
| 1000 | flat | 200,000,000 | 0.0288888 | 10.2435 | 0.0136598 | 38,720 |

Interpretation:

- `L=400`: flat random-edge updates are about `124x` faster and use about `22x` less memory.
- `L=1000`: flat random-edge updates are about `22x` faster and use about `37x` less memory.

### Latest speed-smoke layout benchmark

Job array: `14705121`

| L | backend | updates | construct_s | update_s | observe_s | maxrss_kb |
|---:|:---|---:|---:|---:|---:|---:|
| 400 | Boost | 32,000,000 | 1.68403 | 53.2369 | 0.000000717 | 246,072 |
| 400 | flat | 32,000,000 | 0.00371767 | 0.146857 | 0.000842406 | 10,988 |

Interpretation:

- The latest `L=400` speed-smoke gives about `363x` faster random-edge updates and about `22x` lower memory for the flat layout benchmark.
- This is a layout/update microbenchmark, not a complete QMC replacement measurement.

## QMC smoke evidence

The first QMC smoke submission failed with `Illegal instruction` because it used a non-portable CLI binary. Rebuilding and using `build-current-clang-portable/paratoric` fixed the issue.

Retry job array: `14705159`

| L | state | output |
|---:|:---|:---|
| 40 | completed | `/scratch/a/A.Otaifi/paratoric_speed_smoke/qmc/qmc_L40_minobs/obs.h5` |
| 80 | completed | `/scratch/a/A.Otaifi/paratoric_speed_smoke/qmc/qmc_L80_minobs/obs.h5` |

QMC smoke parameters:

- `simulation=etc_sample`
- `basis=x`
- `lattice_type=square`
- `boundaries=periodic`
- `beta=4.0`
- `h=0.3`, `mu=1.0`, `J=1.0`, `lambda=0.2`
- `N_thermalization=2000`
- `N_samples=20`
- `N_between_samples=500`
- `N_resamples=50`
- observables: `energy anyon_count sigma_x star_x`
- `full_time_series=0`
- `snapshots=0`

## Practical conclusion

The current production improvements reduce memory and measurement overhead, while the flat-layout evidence shows that Boost graph storage is a serious bottleneck for very large regular square lattices.

## Real core-update backend benchmark

After `809806c`, the QMC engine can run a core Metropolis-update benchmark with either the default Boost-backed lattice or the square-periodic flat backend. This benchmark uses the production Metropolis update dispatcher and energy-difference path, but intentionally skips observable/bootstrap/HDF5 overhead.

Job array: `14705323`

Parameters:

- `simulation=etc_core_update_benchmark`
- `basis=x`
- `lattice_type=square`
- `boundaries=periodic`
- `beta=4.0`
- `h=0.3`, `mu=1.0`, `J=1.0`, `lambda=0.2`
- `sweeps=10`, where updates are `10 * 2 * L * L`

| L | backend | updates | app_time_s | attempted | accepted | acceptance_fraction |
|---:|:---|---:|---:|---:|---:|---:|
| 400 | Boost | 3,200,000 | 3.265169 | 3,200,000 | 231,852 | 0.07245375 |
| 400 | flat_square | 3,200,000 | 1.737895 | 3,200,000 | 243,427 | 0.0760709375 |
| 1000 | Boost | 20,000,000 | 23.523580 | 20,000,000 | 1,453,529 | 0.07267645 |
| 1000 | flat_square | 20,000,000 | 11.235196 | 20,000,000 | 1,527,356 | 0.0763678 |

Interpretation:

- `L=400`: flat-square core updates are about `1.9x` faster than Boost.
- `L=1000`: flat-square core updates are about `2.1x` faster than Boost.
- Acceptance statistics are similar but not bit-identical because edge/tuple ordering differs between backends.
- The real QMC-core speedup is smaller than the layout microbenchmark speedup, which means core-update costs are now dominated by flip-list and energy-difference work rather than graph lookup alone.

## Updated practical conclusion

The current production improvements reduce memory and measurement overhead. The flat square backend is now wired into a real core-update benchmark and shows about a `2x` speedup at `L=400` and `L=1000` for this smoke setup.

The next step is to validate flat-square physics against Boost at small sizes, then wire flat-square into full `etc_sample` with a restricted observable set. Only after that should large production runs such as `L=400` or `L=1000` be treated as scientific runs rather than performance smoke tests.
