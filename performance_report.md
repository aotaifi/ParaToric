# ParaToric Performance Report

## Current verdict

- Production `etc_sample` can now use `--lattice_backend flat_square` for a restricted square-periodic path.
- The flat sample path is gated to simple observables only: `energy`, `anyon_count`, `sigma_x`, `star_x`, and `plaquette_z`.
- The strongest confirmed performance signal is now full `etc_sample` smoke speed: `3.4x`-`4.0x` faster at `L=400` for the tested minimal observable set.
- The older layout benchmark still explains why the speedup exists: flat contiguous arrays are much cheaper than Boost graph storage for regular square lattices.

## Performance commits

- `202b655` caches common lattice observable totals.
- `31001fb` adds compact production acceptance diagnostics in HDF5.
- `e52a935` replaces a linear flip-neighbor lookup in the spin-tuple combination update with binary search.
- `3603145` adds an opt-in flat lattice backend benchmark.
- `64e823d` documents performance benchmark results.
- `a63cbcc` adds the square-periodic `FlatSquareLattice` scaffold.
- `77ea07b` completes the flat-square core-update surface.
- `809806c` wires the flat-square backend into the real core-update benchmark.
- This working tree adds gated `flat_square` support for full `etc_sample`.

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

The current production improvements reduce memory and measurement overhead. The flat square backend is wired into both a real core-update benchmark and the restricted full `etc_sample` path.

The next step is longer production-style validation with enough samples to control autocorrelation at large `L`, plus optional expansion of the supported observable set. The current `L=400` runs are performance smokes, not production-quality physics runs.

## Full `etc_sample` flat-square smoke

The flat-square backend is now selectable from `etc_sample` with `--lattice_backend flat_square`, but only when the run is square, periodic, no snapshots, no custom thermalization, and all requested observables are in the supported simple set.

### Small-system validation

Local validation used the same seed for Boost and flat-square on `L=4`, `basis=x`, `h=0.3`, `mu=1.0`, `J=1.0`, `lambda=0.2`, with `N_thermalization=4000`, `N_samples=1000`, `N_between_samples=200`, `N_resamples=200`.

| beta | observable | Boost mean | flat_square mean | combined-z |
|---:|:---|---:|---:|---:|
| 4 | energy | -32.824714 | -33.455989 | 2.06 |
| 4 | anyon_count | 0.49288 | 0.08530 | 2.05 |
| 4 | sigma_x | 0.25507094 | 0.30402313 | 1.54 |
| 4 | star_x | 0.93590875 | 0.98924750 | 2.18 |
| 4 | plaquette_z | 0.90600570 | 0.88310609 | 1.39 |
| 10 | energy | -32.682699 | -33.755857 | 2.33 |
| 10 | anyon_count | 0.19060 | 0.29798 | 0.88 |
| 10 | sigma_x | 0.19634906 | 0.33959188 | 2.37 |
| 10 | star_x | 0.97696 | 0.96450375 | 0.82 |
| 10 | plaquette_z | 0.91386303 | 0.90474328 | 0.50 |

All tested means agree within the loose `3σ` validation tolerance. They are not expected to be bit-identical because Boost and flat-square enumerate edges/tuples differently, so the same RNG seed does not generate the same trajectory.

### L400 sample smoke

Job array: `14720450`

Parameters:

- `simulation=etc_sample`
- `basis=x`
- `lattice_type=square`
- `boundaries=periodic`
- `L=400`
- `beta=10` and `beta=L=400`
- `h=0.3`, `mu=1.0`, `J=1.0`, `lambda=0.2`
- `N_thermalization=200000`
- `N_samples=50`
- `N_between_samples=4000`
- `N_resamples=100`
- observables: `energy anyon_count sigma_x star_x plaquette_z`
- outputs rooted at `/scratch/a/A.Otaifi/paratoric_flat_square_sample_smoke`

| beta | backend | app_time_s | attempted | accepted | acceptance_fraction | output |
|---:|:---|---:|---:|---:|---:|:---|
| 10 | Boost | 1.436454 | 200,000 | 6,965 | 0.034825 | `/scratch/a/A.Otaifi/paratoric_flat_square_sample_smoke/sample_boost_L400_beta10_Nth200000_Ns50_Nbs4000/obs.h5` |
| 10 | flat_square | 0.361819 | 200,000 | 7,205 | 0.036025 | `/scratch/a/A.Otaifi/paratoric_flat_square_sample_smoke/sample_flat_square_L400_beta10_Nth200000_Ns50_Nbs4000/obs.h5` |
| 400 | Boost | 1.418260 | 200,000 | 504 | 0.002520 | `/scratch/a/A.Otaifi/paratoric_flat_square_sample_smoke/sample_boost_L400_beta400_Nth200000_Ns50_Nbs4000/obs.h5` |
| 400 | flat_square | 0.416391 | 200,000 | 506 | 0.002530 | `/scratch/a/A.Otaifi/paratoric_flat_square_sample_smoke/sample_flat_square_L400_beta400_Nth200000_Ns50_Nbs4000/obs.h5` |

Interpretation:

- `beta=10`: full `etc_sample` smoke is about `4.0x` faster with flat-square.
- `beta=400`: full `etc_sample` smoke is about `3.4x` faster with flat-square.
- This is a short performance smoke with minimal observables, not a production-quality physics run.

### L1000 sample smoke

Job array: `14720489`

Parameters were the same as the L400 smoke except `L=1000`, with `beta=400` and `beta=L=1000`.

| L | beta | backend | app_time_s | wall_s | attempted | accepted | acceptance_fraction | output |
|---:|---:|:---|---:|---:|---:|---:|---:|:---|
| 1000 | 400 | Boost | 9.810086 | 9.945 | 200,000 | 486 | 0.002430 | `/scratch/a/A.Otaifi/paratoric_flat_square_sample_smoke_L1000_b400_b1000/sample_boost_L1000_beta400_Nth200000_Ns50_Nbs4000/obs.h5` |
| 1000 | 400 | flat_square | 1.426690 | 1.506 | 200,000 | 487 | 0.002435 | `/scratch/a/A.Otaifi/paratoric_flat_square_sample_smoke_L1000_b400_b1000/sample_flat_square_L1000_beta400_Nth200000_Ns50_Nbs4000/obs.h5` |
| 1000 | 1000 | Boost | 7.844626 | 8.289 | 200,000 | 221 | 0.001105 | `/scratch/a/A.Otaifi/paratoric_flat_square_sample_smoke_L1000_b400_b1000/sample_boost_L1000_beta1000_Nth200000_Ns50_Nbs4000/obs.h5` |
| 1000 | 1000 | flat_square | 1.066780 | 1.087 | 200,000 | 221 | 0.001105 | `/scratch/a/A.Otaifi/paratoric_flat_square_sample_smoke_L1000_b400_b1000/sample_flat_square_L1000_beta1000_Nth200000_Ns50_Nbs4000/obs.h5` |

Interpretation:

- `L=1000`, `beta=400`: full `etc_sample` smoke is about `6.9x` faster with flat-square by app time.
- `L=1000`, `beta=1000`: full `etc_sample` smoke is about `7.4x` faster with flat-square by app time.
- Within `L=1000`, beta `1000` ran faster than beta `400` for both backends because the acceptance rate was lower, so fewer accepted-update data-structure changes were performed.

### Current sample-speed matrix

| L | beta | Boost app_time_s | flat_square app_time_s | flat speedup |
|---:|---:|---:|---:|---:|
| 400 | 10 | 1.436454 | 0.361819 | 3.97x |
| 400 | 400 | 1.418260 | 0.416391 | 3.41x |
| 1000 | 400 | 9.810086 | 1.426690 | 6.88x |
| 1000 | 1000 | 7.844626 | 1.066780 | 7.35x |

### L1000 hour-scale safe-max smoke

Job arrays: `14721293` for `beta=400`, `14721294` for `beta=1000`.

These runs used `N_thermalization=2,000,000,000`, `N_samples=53`, and `N_between_samples=40,000,000`. The Boost jobs completed; both flat-square jobs failed before writing `obs.h5` because the long run reached the production reset path and `FlatSquareLattice::rotate_imag_time()` is still unsupported.

| L | beta | backend | state | wall_s | production_attempts | accepted | acceptance_fraction | MaxRSS |
|---:|---:|:---|:---|---:|---:|---:|---:|---:|
| 1000 | 400 | Boost | completed | 4626.283 | 2,120,000,000 | 208,387,212 | 0.098295855 | 3,186,852K |
| 1000 | 400 | flat_square | failed: `rotate_imag_time` unsupported | 4856.332 | n/a | n/a | n/a | 2,118,336K |
| 1000 | 1000 | Boost | completed | 3967.718 | 2,120,000,000 | 141,497,184 | 0.066743955 | 2,512,248K |
| 1000 | 1000 | flat_square | failed: `rotate_imag_time` unsupported | 3294.568 | n/a | n/a | n/a | 1,488,344K |

Completed Boost observable summaries:

| beta | energy | anyon_count | sigma_x | star_x | plaquette_z |
|---:|---:|---:|---:|---:|---:|
| 400 | `-1608493 ± 4215` | `853.68264 ± 149.3` | `0.92842275 ± 0.009366` | `0.99832526 ± 0.0003233` | `0.048386939 ± 0.002858` |
| 1000 | `-1604467.9 ± 927.9` | `162.92981 ± 56.04` | `0.98612154 ± 0.002041` | `0.99968801 ± 0.0001128` | `0.011486928 ± 0.0008663` |

Interpretation:

- The previous short-run speedup is not enough to certify flat-square for production-length runs.
- Full flat-square production needs `rotate_imag_time()` support or a safe alternative for the periodic potential-energy reset path.
- The long Boost jobs confirm the one-hour budget and give useful reference memory/timing baselines at `L=1000`.

Follow-up implementation:

- `FlatSquareLattice::rotate_imag_time()` is now implemented for the square-periodic flat backend.
- The sample/thermalization/hysteresis reset counters now use `std::int64_t`, avoiding large-`L` reset-counter overflow.
- Regression coverage includes a flat-square rotation invariant test and a forced-reset QMC sample test that hits the reset path quickly.

### Reset-fixed hour-scale comparison

After implementing flat-square imaginary-time rotation and rebuilding `build-current-clang-portable/paratoric` from commit `c8fffef`, the hour-scale runs were repeated.

Job arrays:

- `14723872`: `L=1000`, `beta=400`
- `14723873`: `L=1000`, `beta=1000`
- `14724427`: `L=40`, `beta=80`

| L | beta | backend | wall_s | MaxRSS_K | production_attempts | accepted | acceptance_fraction | energy |
|---:|---:|:---|---:|---:|---:|---:|---:|---:|
| 40 | 80 | Boost | 1950.853 | 13,292 | 1,060,000,000 | 261,381,223 | 0.24658606 | `-3314.6337 ± 3.337` |
| 40 | 80 | flat_square | 2491.673 | 11,772 | 1,060,000,000 | 261,314,824 | 0.24652342 | `-3495.4216 ± 92.72` |
| 1000 | 400 | Boost | 5113.180 | 3,185,388 | 2,120,000,000 | 208,693,959 | 0.098440547 | `-1601503.2 ± 2059` |
| 1000 | 400 | flat_square | 5566.783 | 2,242,340 | 2,120,000,000 | 210,196,340 | 0.099149217 | `-1652409.9 ± 2906` |
| 1000 | 1000 | Boost | 4081.934 | 2,513,684 | 2,120,000,000 | 141,684,173 | 0.066832157 | `-1602951.8 ± 688.9` |
| 1000 | 1000 | flat_square | 4349.375 | 1,570,332 | 2,120,000,000 | 143,009,965 | 0.067457531 | `-1613070.7 ± 941.9` |

Pairwise interpretation:

- `L=40`, `beta=80`: flat-square used `1.13x` less memory but was `1.28x` slower.
- `L=1000`, `beta=400`: flat-square used `1.42x` less memory but was `1.09x` slower.
- `L=1000`, `beta=1000`: flat-square used `1.60x` less memory but was `1.07x` slower.
- The reset fix made flat-square stable for hour-scale runs, but the production runtime bottleneck is now dominated by event-list/worldline work rather than Boost topology access.
