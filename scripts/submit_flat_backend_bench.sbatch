#!/usr/bin/env bash
#SBATCH --job-name=paratoric-flat-bench
#SBATCH --partition=small
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem=32G
#SBATCH --time=01:00:00
#SBATCH --array=0-3
#SBATCH --output=/scratch/a/A.Otaifi/paratoric_flat_backend_bench/logs/%x_%A_%a.out
#SBATCH --error=/scratch/a/A.Otaifi/paratoric_flat_backend_bench/logs/%x_%A_%a.err

set -euo pipefail

REPO_ROOT="${REPO_ROOT:-/project/theorie/a/A.Otaifi/ParaToric}"
BENCH_EXE="${BENCH_EXE:-${REPO_ROOT}/build-current-clang-portable/flat_lattice_backend_bench}"
OUT_ROOT="${OUT_ROOT:-/scratch/a/A.Otaifi/paratoric_flat_backend_bench}"
SWEEPS="${SWEEPS:-1}"
REPEATS="${REPEATS:-1}"
SEED="${SEED:-1}"

mkdir -p "${OUT_ROOT}/logs" "${OUT_ROOT}/results"

case "${SLURM_ARRAY_TASK_ID}" in
  0) BACKEND="boost"; L="400" ;;
  1) BACKEND="flat";  L="400" ;;
  2) BACKEND="boost"; L="1000" ;;
  3) BACKEND="flat";  L="1000" ;;
  *) echo "Unknown SLURM_ARRAY_TASK_ID=${SLURM_ARRAY_TASK_ID}" >&2; exit 2 ;;
esac

if [[ ! -x "${BENCH_EXE}" ]]; then
  echo "Missing executable: ${BENCH_EXE}" >&2
  exit 2
fi

RESULT_FILE="${OUT_ROOT}/results/backend-${BACKEND}_L-${L}_job-${SLURM_JOB_ID}_${SLURM_ARRAY_TASK_ID}.txt"

{
  echo "date=$(date --iso-8601=seconds)"
  echo "host=$(hostname)"
  echo "repo=${REPO_ROOT}"
  git -C "${REPO_ROOT}" rev-parse HEAD
  echo "backend=${BACKEND}"
  echo "L=${L}"
  echo "sweeps=${SWEEPS}"
  echo "repeats=${REPEATS}"
  echo "seed=${SEED}"
  echo "--- benchmark ---"
  "${BENCH_EXE}" \
    --backend "${BACKEND}" \
    --L "${L}" \
    --sweeps "${SWEEPS}" \
    --repeats "${REPEATS}" \
    --seed "${SEED}"
} 2>&1 | tee "${RESULT_FILE}"
