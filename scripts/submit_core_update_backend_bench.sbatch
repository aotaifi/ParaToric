#!/usr/bin/env bash
#SBATCH --job-name=paratoric-core-backend
#SBATCH --partition=small
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem=16G
#SBATCH --time=02:00:00
#SBATCH --array=0-3
#SBATCH --output=/scratch/a/A.Otaifi/paratoric_core_backend_bench/logs/%x_%A_%a.out
#SBATCH --error=/scratch/a/A.Otaifi/paratoric_core_backend_bench/logs/%x_%A_%a.err

set -euo pipefail

REPO_ROOT="${REPO_ROOT:-/project/theorie/a/A.Otaifi/ParaToric}"
PARATORIC_EXE="${PARATORIC_EXE:-${REPO_ROOT}/build-current-clang-portable/paratoric}"
OUT_ROOT="${OUT_ROOT:-/scratch/a/A.Otaifi/paratoric_core_backend_bench}"
SEED="${SEED:-24601}"
SWEEPS="${SWEEPS:-10}"

mkdir -p "${OUT_ROOT}/logs" "${OUT_ROOT}/results"

case "${SLURM_ARRAY_TASK_ID}" in
  0) BACKEND="boost";       L="400" ;;
  1) BACKEND="flat_square"; L="400" ;;
  2) BACKEND="boost";       L="1000" ;;
  3) BACKEND="flat_square"; L="1000" ;;
  *) echo "Unknown SLURM_ARRAY_TASK_ID=${SLURM_ARRAY_TASK_ID}" >&2; exit 2 ;;
esac

UPDATES=$((SWEEPS * 2 * L * L))
TAG="core_${BACKEND}_L${L}_s${SWEEPS}"
RESULT_FILE="${OUT_ROOT}/results/${TAG}_job-${SLURM_JOB_ID}_${SLURM_ARRAY_TASK_ID}.txt"

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
  echo "backend=${BACKEND}"
  echo "L=${L}"
  echo "sweeps=${SWEEPS}"
  echo "updates=${UPDATES}"
  echo "--- run ---"
  "${PARATORIC_EXE}" \
    --simulation etc_core_update_benchmark \
    --lattice_backend "${BACKEND}" \
    --N_thermalization "${UPDATES}" \
    --N_samples 0 \
    --N_between_samples 0 \
    --N_resamples 1 \
    --custom_therm 0 \
    --seed "${SEED}" \
    --beta 4.0 \
    --h_constant 0.3 \
    --h_constant_therm 0.3 \
    --mu_constant 1.0 \
    --J_constant 1.0 \
    --lmbda_constant 0.2 \
    --lmbda_constant_therm 0.2 \
    --observables energy \
    --basis x \
    --lattice_type square \
    --system_size "${L}" \
    --boundaries periodic \
    --default_spin 1 \
    --snapshots 0 \
    --full_time_series 0 \
    --process_index "${SLURM_ARRAY_TASK_ID}" \
    --output_directory "${OUT_ROOT}" \
    --folder_name "${TAG}"
} 2>&1 | tee "${RESULT_FILE}"
