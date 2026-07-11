#!/usr/bin/env bash
#SBATCH --job-name=pt-g2-sector-rate-v2
#SBATCH --partition=cluster
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem-per-cpu=1G
#SBATCH --time=08:00:00
#SBATCH --array=0-63
#SBATCH --output=/scratch/a/A.Otaifi/paratoric_g2_sector_rate_scan_v2_20260711/logs/%x_%A_%a.out
#SBATCH --error=/scratch/a/A.Otaifi/paratoric_g2_sector_rate_scan_v2_20260711/logs/%x_%A_%a.err

set -euo pipefail

REPO_ROOT="${REPO_ROOT:-/project/theorie/a/A.Otaifi/ParaToric}"
PARATORIC_EXE="${PARATORIC_EXE:-${REPO_ROOT}/build-current-clang-portable/paratoric}"
OUT_ROOT="${OUT_ROOT:-/scratch/a/A.Otaifi/paratoric_g2_sector_rate_scan_v2_20260711}"
SEED_BASE="${SEED_BASE:-7111200}"

L="${L:-120}"
BETA="${BETA:-120}"
N_THERM="${N_THERM:-1000000000}"
N_SAMPLES="${N_SAMPLES:-1500}"
N_BETWEEN="${N_BETWEEN:-5000000}"
N_RESAMPLES="${N_RESAMPLES:-200}"

S_VALUES=(0.10 0.16 0.22 0.28 0.34 0.40 0.46 0.50)
TASK_ID="${SLURM_ARRAY_TASK_ID:?SLURM_ARRAY_TASK_ID is required}"
S_INDEX=$((TASK_ID / 8))
CHAIN_INDEX=$((TASK_ID % 8))

if (( S_INDEX < 0 || S_INDEX >= ${#S_VALUES[@]} )); then
  echo "Unknown SLURM_ARRAY_TASK_ID=${TASK_ID}; computed S_INDEX=${S_INDEX}" >&2
  exit 2
fi

S_VALUE="${S_VALUES[${S_INDEX}]}"
SEED=$((SEED_BASE + 100 * S_INDEX + CHAIN_INDEX))
TAG="sector_rate_v2_L${L}_beta${BETA}_s${S_VALUE}_chain${CHAIN_INDEX}_Nth${N_THERM}_Ns${N_SAMPLES}_Nbs${N_BETWEEN}_seed${SEED}"
RESULT_FILE="${OUT_ROOT}/results/${TAG}_job-${SLURM_JOB_ID}_${TASK_ID}.txt"

mkdir -p "${OUT_ROOT}/logs" "${OUT_ROOT}/results"

if [[ ! -x "${PARATORIC_EXE}" ]]; then
  echo "Missing executable: ${PARATORIC_EXE}" >&2
  exit 2
fi

{
  echo "date=$(date --iso-8601=seconds)"
  echo "host=$(hostname)"
  echo "repo=${REPO_ROOT}"
  git -C "${REPO_ROOT}" rev-parse HEAD
  git -C "${REPO_ROOT}" status --short
  echo "scan=G2 wide-s sector-rate v2"
  echo "task_id=${TASK_ID}"
  echo "s_index=${S_INDEX}"
  echo "chain_index=${CHAIN_INDEX}"
  echo "L=${L}"
  echo "beta=${BETA}"
  echo "basis=z"
  echo "mu=1.0"
  echo "J=1.0"
  echo "h=${S_VALUE}"
  echo "lmbda=${S_VALUE}"
  echo "N_thermalization=${N_THERM}"
  echo "N_samples=${N_SAMPLES}"
  echo "N_between_samples=${N_BETWEEN}"
  echo "N_resamples=${N_RESAMPLES}"
  echo "seed=${SEED}"
  echo "observables=energy anyon_count percolation_probability percolation_strength"
  echo "full_time_series=1"
  echo "output_hdf5=${OUT_ROOT}/${TAG}/obs.h5"
  echo "accepted_by_update_index_map=0:double_single_spin_flip,1:single_spin_flip_move,2:global_single_spin_flip,3:global_tuple_flip,4:double_tuple_flip,5:single_tuple_flip_move,6:spin_tuple_combination"
  echo "memory_reason=1G per task; observed reference is 3.2GB at L1000 beta400, so L120 beta120 single-chain state plus HDF5 output is expected well below 1G."
  echo "--- run ---"
  start_ms=$(date +%s%3N)
  set +e
  "${PARATORIC_EXE}" \
    --simulation etc_sample \
    --lattice_backend boost \
    --N_thermalization "${N_THERM}" \
    --N_samples "${N_SAMPLES}" \
    --N_between_samples "${N_BETWEEN}" \
    --N_resamples "${N_RESAMPLES}" \
    --custom_therm 0 \
    --seed "${SEED}" \
    --beta "${BETA}" \
    --h_constant "${S_VALUE}" \
    --h_constant_therm "${S_VALUE}" \
    --mu_constant 1.0 \
    --J_constant 1.0 \
    --lmbda_constant "${S_VALUE}" \
    --lmbda_constant_therm "${S_VALUE}" \
    --observables energy anyon_count percolation_probability percolation_strength \
    --basis z \
    --lattice_type square \
    --system_size "${L}" \
    --boundaries periodic \
    --default_spin 1 \
    --snapshots 0 \
    --full_time_series 1 \
    --process_index "${TASK_ID}" \
    --output_directory "${OUT_ROOT}" \
    --folder_name "${TAG}"
  rc=$?
  set -e
  end_ms=$(date +%s%3N)
  wall_ms=$((end_ms - start_ms))
  printf "wall_seconds=%d.%03d\n" "$((wall_ms / 1000))" "$((wall_ms % 1000))"
  exit "${rc}"
} 2>&1 | tee "${RESULT_FILE}"
