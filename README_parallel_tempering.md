# Parallel Tempering in `paratoric_tampered`

This note assumes you already know how to run the normal ParaToric CLI and only need the PT-specific additions.

## Current Scope

- PT is available through the `paratoric_tampered` executable.
- PT currently supports `simulation=etc_sample` only.
- PT currently supports `basis=z` only.
- The tempered parameter can be `h`, `mu`, `J`, or `lmbda`.
- `custom_therm=false` is required.
- MPI is required. The number of MPI ranks must match `--pt_replicas`.

## Required PT Flags

Use these extra flags on top of your usual `etc_sample` inputs:

```text
--pt_enabled true
--pt_parameter <h|mu|J|lmbda>
--pt_replicas <number of MPI ranks>
--pt_swap_period <swap attempt interval in local update steps>
```

Then choose one ladder definition:

```text
--pt_ladder_min <value>
--pt_ladder_max <value>
```

or an explicit ladder:

```text
--pt_ladder_values v1 v2 v3 ...
```

If `--pt_ladder_values` is used, its length must equal `--pt_replicas`.

## Optional Trace Flags

```text
--pt_trace_enabled true
--pt_trace_interval <steps>
```

These write detailed swap traces to `output_test.txt`.

## Minimal Example

```bash
mpirun -np 4 ./build-step2-mpi/paratoric_tampered \
  --simulation etc_sample \
  --basis z \
  --lattice_type square \
  --system_size 4 \
  --beta 4.0 \
  --N_thermalization 200 \
  --N_samples 50 \
  --N_between_samples 20 \
  --N_resamples 100 \
  --observables energy energy_h energy_mu energy_J energy_lmbda plaquette_z \
  --output_directory ./runs_pt \
  --folder_name zbasis_pt_J \
  --pt_enabled true \
  --pt_parameter J \
  --pt_replicas 4 \
  --pt_swap_period 10 \
  --pt_ladder_min 0.8 \
  --pt_ladder_max 1.2 \
  --pt_trace_enabled true \
  --pt_trace_interval 10 \
  --seed 123
```

## How the Swap Works

- For `h` and `mu`, swaps use the z-basis off-diagonal event counts.
- For `J` and `lmbda`, swaps use the z-basis integrated diagonal energies.
- Swaps are attempted between odd/even nearest neighbors on the replica ladder.

## Output Layout

PT runs write rank-local HDF5 outputs under:

```text
<output_directory>/<folder_name>/pt_rank0/obs.h5
<output_directory>/<folder_name>/pt_rank1/obs.h5
...
```

Shared PT diagnostics are written next to those rank folders:

- `output_test.txt`
- `pt_observables_by_ladder.tsv`
- `pt_ladder_visits.tsv`
- `pt_round_trips.tsv`
- `pt_feedback_diagnostics.tsv`

## Helper Scripts

Two example runners are included:

- `scripts/run_pt_jx_smoke.sh`
- `scripts/compare_pt_swap_on_off.sh`

Despite the older filenames, they are configured for the current z-basis PT path.
