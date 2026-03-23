#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-build-step2-mpi}"
OUT_ROOT="${2:-${ROOT_DIR}/runs_pt_compare_$(date +%Y%m%d_%H%M%S)}"
RANKS="${3:-2}"
SEED="${4:-123}"

if [[ "${RANKS}" -lt 2 ]]; then
  echo "Error: RANKS must be >= 2 for PT run." >&2
  exit 1
fi

BIN="${ROOT_DIR}/${BUILD_DIR}/paratoric_tampered"
if [[ ! -x "${BIN}" ]]; then
  echo "Error: executable not found: ${BIN}" >&2
  exit 1
fi

MPI_LAUNCHER="${MPI_LAUNCHER:-srun}"
if [[ "${MPI_LAUNCHER}" == "srun" ]]; then
  MPI_PT=(srun --mpi=pmix -n "${RANKS}")
  MPI_REF=(srun --mpi=pmix -n 1)
else
  MPIEXEC="${MPIEXEC:-mpirun}"
  MPI_PT=("${MPIEXEC}" -np "${RANKS}")
  MPI_REF=("${MPIEXEC}" -np 1)
fi

if ! command -v "${MPI_PT[0]}" >/dev/null 2>&1; then
  echo "Error: MPI launcher not found: ${MPI_PT[0]}" >&2
  exit 1
fi

BOOST_LIB_DEFAULT="/software/opt/el_9/x86_64/spack/2024.07/spack/opt/spack/linux-almalinux9-x86_64_v2/gcc-13.2.0/boost-1.85.0-3zjda6mcvjj4h3x34qxh7lhtrksykzbc/lib"
HDF5_LIB_DEFAULT="/software/opt/el_9/x86_64/spack/2024.07/spack/opt/spack/linux-almalinux9-x86_64_v2/gcc-13.2.0/hdf5-1.14.3-ghn7qazcaoy6ajxy6cpcqv52cahwttj3/lib"
BOOST_LIB="${BOOST_LIB:-${BOOST_LIB_DEFAULT}}"
HDF5_LIB="${HDF5_LIB:-${HDF5_LIB_DEFAULT}}"
export LD_LIBRARY_PATH="${BOOST_LIB}:${HDF5_LIB}:${LD_LIBRARY_PATH:-}"
: "${HDF5_USE_FILE_LOCKING:=FALSE}"
export HDF5_USE_FILE_LOCKING

PT_ON_ROOT="${OUT_ROOT}/swap_on"
PT_OFF_ROOT="${OUT_ROOT}/swap_off"
mkdir -p "${PT_ON_ROOT}" "${PT_OFF_ROOT}"

COMMON_ARGS=(
  --simulation etc_sample
  --basis x
  --lattice_type square
  --system_size 4
  --beta 4.0
  --N_thermalization 200
  --N_samples 50
  --N_between_samples 20
  --N_resamples 100
  --observables energy energy_J plaquette_z
  --seed "${SEED}"
)

echo "[1/2] Running swap-on PT (${RANKS} ranks)..."
"${MPI_PT[@]}" "${BIN}" \
  "${COMMON_ARGS[@]}" \
  --output_directory "${PT_ON_ROOT}" \
  --folder_name swap_on_pt \
  --pt_enabled true \
  --pt_parameter J \
  --pt_replicas "${RANKS}" \
  --pt_swap_period 10 \
  --pt_ladder_min 0.8 \
  --pt_ladder_max 1.2

echo "[2/2] Running swap-off reference (single rank, same seed)..."
"${MPI_REF[@]}" "${BIN}" \
  "${COMMON_ARGS[@]}" \
  --output_directory "${PT_OFF_ROOT}" \
  --folder_name swap_off_ref \
  --pt_enabled false

echo
echo "Completed swap-off vs swap-on runs."
echo "Swap-off root: ${PT_OFF_ROOT}"
echo "Swap-on root:  ${PT_ON_ROOT}"
TRACE_FILE="${PT_ON_ROOT}/swap_on_pt/output_test.txt"
MERGED_FILE="${PT_ON_ROOT}/swap_on_pt/pt_observables_by_ladder.tsv"
echo "Swap-on trace: ${TRACE_FILE}"
echo "Swap-on merged-by-ladder: ${MERGED_FILE}"
if [[ -f "${TRACE_FILE}" ]]; then
  echo "PT summary:"
  tail -n 2 "${TRACE_FILE}"
fi
