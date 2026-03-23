#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-build-step2-mpi}"
OUT_ROOT="${2:-${ROOT_DIR}/runs_pt_smoke_$(date +%Y%m%d_%H%M%S)}"
RANKS="${3:-2}"
SEED="${4:-123}"

BIN="${ROOT_DIR}/${BUILD_DIR}/paratoric_tampered"

# Prefer srun on this cluster, fallback to mpirun if explicitly requested.
MPI_LAUNCHER="${MPI_LAUNCHER:-srun}"
if [[ "${MPI_LAUNCHER}" == "srun" ]]; then
  MPI_CMD=(srun --mpi=pmix -n "${RANKS}")
else
  MPIEXEC="${MPIEXEC:-mpirun}"
  MPI_CMD=("${MPIEXEC}" -np "${RANKS}")
fi

BOOST_LIB_DEFAULT="/software/opt/el_9/x86_64/spack/2024.07/spack/opt/spack/linux-almalinux9-x86_64_v2/gcc-13.2.0/boost-1.85.0-3zjda6mcvjj4h3x34qxh7lhtrksykzbc/lib"
HDF5_LIB_DEFAULT="/software/opt/el_9/x86_64/spack/2024.07/spack/opt/spack/linux-almalinux9-x86_64_v2/gcc-13.2.0/hdf5-1.14.3-ghn7qazcaoy6ajxy6cpcqv52cahwttj3/lib"
BOOST_LIB="${BOOST_LIB:-${BOOST_LIB_DEFAULT}}"
HDF5_LIB="${HDF5_LIB:-${HDF5_LIB_DEFAULT}}"

if [[ ! -x "${BIN}" ]]; then
  echo "Error: executable not found: ${BIN}" >&2
  exit 1
fi

if ! command -v "${MPI_CMD[0]}" >/dev/null 2>&1; then
  echo "Error: MPI launcher not found: ${MPI_CMD[0]}" >&2
  exit 1
fi

mkdir -p "${OUT_ROOT}"

export LD_LIBRARY_PATH="${BOOST_LIB}:${HDF5_LIB}:${LD_LIBRARY_PATH:-}"
: "${HDF5_USE_FILE_LOCKING:=FALSE}"
export HDF5_USE_FILE_LOCKING

"${MPI_CMD[@]}" "${BIN}" \
  --simulation etc_sample \
  --basis x \
  --lattice_type square \
  --system_size 2 \
  --beta 2.0 \
  --N_thermalization 20 \
  --N_samples 4 \
  --N_between_samples 5 \
  --N_resamples 10 \
  --observables energy energy_J \
  --output_directory "${OUT_ROOT}" \
  --folder_name smoke_pt_jx \
  --pt_enabled true \
  --pt_parameter J \
  --pt_replicas "${RANKS}" \
  --pt_swap_period 2 \
  --pt_ladder_min 0.8 \
  --pt_ladder_max 1.2 \
  --seed "${SEED}"

echo
echo "PT smoke run completed."
TRACE_FILE="${OUT_ROOT}/smoke_pt_jx/output_test.txt"
MERGED_FILE="${OUT_ROOT}/smoke_pt_jx/pt_observables_by_ladder.tsv"
echo "Trace file: ${TRACE_FILE}"
echo "Merged file: ${MERGED_FILE}"
if [[ -f "${TRACE_FILE}" ]]; then
  tail -n 20 "${TRACE_FILE}"
fi
