# ParaToric performance notes, June 2026

This note summarizes the local performance work on top of upstream
`palmbart/ParaToric` through commit `3603145`.

## Performance commits

- `202b655` caches common lattice observable totals.
  - Purpose: avoid repeated full lattice/graph scans for frequently sampled
    observables.
  - Cached quantities include diagonal single-spin totals, diagonal tuple
    totals, anyon count, and global non-diagonal flip counters.
  - This mainly reduces measurement overhead when many samples/observables are
    requested.

- `31001fb` adds compact production acceptance diagnostics.
  - Purpose: preserve production-run health information without storing a large
    per-update acceptance-ratio history.
  - HDF5 output is stored under
    `/simulation/diagnostics/production_acceptance`.
  - The data include total attempts, accepted moves, mean proposal ratio, the
    same by update type, and block-level summaries.

- `e52a935` uses binary search in the spin-tuple combination update.
  - Purpose: remove a remaining linear scan over sorted single-spin flip times.
  - The update now uses sorted-range lookup around the proposed imaginary time.
  - This is most relevant when an edge carries many single-spin flips.

- `3603145` adds a flat-lattice backend benchmark.
  - Purpose: quantify the cost of the current Boost graph layout against a
    minimal contiguous-array square-lattice layout.
  - This is a prototype layout benchmark, not yet a production QMC backend.

## Flat backend benchmark

The benchmark compares:

- current Boost-graph `Lattice` storage;
- a minimal flat square-periodic layout using contiguous arrays:
  `edge_spin`, `star_edges`, and `plaquette_edges`.

The benchmark performs one or more sweeps of random edge flips and measures:

- `construct_s`: lattice construction time;
- `update_s`: time for random edge flips;
- `observe_s`: time for three simple totals;
- `maxrss_kb`: peak resident memory reported by the benchmark process.

The benchmark executable is opt-in via:

```bash
cmake -S . -B build-current-clang-portable -DPARATORIC_BUILD_BENCHMARKS=ON
cmake --build build-current-clang-portable --target flat_lattice_backend_bench
```

The submitted Slurm wrapper is:

```bash
scripts/submit_flat_backend_bench.sbatch
```

### One-sweep benchmark

Job array: `14697380`

| L | backend | updates | construct_s | update_s | observe_s | maxrss_kb |
|---:|:---|---:|---:|---:|---:|---:|
| 400 | Boost | 320,000 | 0.67247 | 0.221898 | 0.0000005 | 243,128 |
| 400 | flat | 320,000 | 0.00280772 | 0.00137097 | 0.000736262 | 10,148 |
| 1000 | Boost | 2,000,000 | 4.07146 | 1.70759 | 0.00000045 | 1,441,252 |
| 1000 | flat | 2,000,000 | 0.011797 | 0.0123638 | 0.00492787 | 38,316 |

### Sustained 100-sweep benchmark

Job array: `14699151`

| L | backend | updates | construct_s | update_s | observe_s | maxrss_kb |
|---:|:---|---:|---:|---:|---:|---:|
| 400 | Boost | 32,000,000 | 1.55904 | 43.875 | 0.000000533 | 246,232 |
| 400 | flat | 32,000,000 | 0.00647945 | 0.352766 | 0.00146572 | 11,244 |
| 1000 | Boost | 200,000,000 | 6.73336 | 222.455 | 0.000000361 | 1,441,248 |
| 1000 | flat | 200,000,000 | 0.0288888 | 10.2435 | 0.0136598 | 38,720 |

Sustained update speedups:

- `L=400`: about `124x` faster flat random-edge updates.
- `L=1000`: about `22x` faster flat random-edge updates.

Sustained memory reductions:

- `L=400`: about `22x` lower peak memory.
- `L=1000`: about `37x` lower peak memory.

## Interpretation

The benchmark is intentionally narrow. It does not yet include the full
continuous-time QMC state, tuple-flip vectors, all observables, HDF5 output, or
all lattice families. It does, however, isolate the cost of the graph layout.

The result strongly suggests that replacing Boost graph descriptors and graph
property access with contiguous arrays could make very large regular lattices
substantially more practical.

The current cached-observable commits and the flat-backend idea address
different costs:

- cached observables make measurement readout cheap;
- flat arrays make construction, update traversal, and memory layout cheap.

Both are useful together.

## Expected impact for large systems

With the current commits, larger systems become more plausible because:

- full acceptance-ratio histories no longer dominate memory;
- common observables avoid repeated full graph scans;
- a remaining combination-update scan was replaced by binary search;
- flat-array benchmarks show that the current Boost graph layout has large
  overhead for regular lattices.

However, production feasibility for `L=400` or `L=1000` still depends on:

- inverse temperature `beta`;
- typical number of single-spin and tuple flips;
- requested observables;
- snapshot/HDF5 output choices;
- acceptance rates and autocorrelation times;
- whether the full QMC backend is actually ported to flat arrays.

The benchmark supports pursuing a production flat backend, but it does not by
itself prove that full QMC production at `L=1000` is already solved.

## Suggested upstream message

Suggested concise message for a GitHub pull request or issue:

> I have a performance branch that adds cached observable totals, compact HDF5
> production acceptance diagnostics, and a binary-search optimization in the
> spin-tuple combination update. I also added an opt-in flat-lattice layout
> benchmark. On our cluster, the sustained 100-sweep square-lattice benchmark
> shows large layout overhead from the current Boost graph storage: for
> `L=400`, flat random-edge updates are about `124x` faster with about `22x`
> lower memory; for `L=1000`, about `22x` faster with about `37x` lower memory.
> The flat backend is only a prototype benchmark so far, not a production QMC
> replacement, but it suggests a regular-lattice flat-array backend may be worth
> discussing.

