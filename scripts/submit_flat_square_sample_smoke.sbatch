#!/usr/bin/env bash
#SBATCH --job-name=paratoric-flat-sample
#SBATCH --partition=small
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem=16G
#SBATCH --time=02:00:00
#SBATCH --array=0-3
#SBATCH --output=/scratch/a/A.Otaifi/paratoric_flat_square_sample_smoke/logs/%x_%A_%a.out
#SBATCH --error=/scratch/a/A.Otaifi/paratoric_flat_square_sample_smoke/logs/%x_%A_%a.err

set -euo pipefail

REPO_ROOT="${REPO_ROOT:-/project/theorie/a/A.Otaifi/ParaToric}"
PARATORIC_EXE="${PARATORIC_EXE:-${REPO_ROOT}/build-current-clang-portable/paratoric}"
OUT_ROOT="${OUT_ROOT:-/scratch/a/A.Otaifi/paratoric_flat_square_sample_smoke}"
SEED="${SEED:-24601}"
L="${L:-400}"
BETA_LOW="${BETA_LOW:-10}"
BETA_HIGH="${BETA_HIGH:-${L}}"
N_THERM="${N_THERM:-200000}"
N_SAMPLES="${N_SAMPLES:-50}"
N_BETWEEN="${N_BETWEEN:-4000}"
N_RESAMPLES="${N_RESAMPLES:-100}"

mkdir -p "${OUT_ROOT}/logs" "${OUT_ROOT}/results"

case "${SLURM_ARRAY_TASK_ID}" in
  0) BACKEND="boost";       BETA="${BETA_LOW}" ;;
  1) BACKEND="flat_square"; BETA="${BETA_LOW}" ;;
  2) BACKEND="boost";       BETA="${BETA_HIGH}" ;;
  3) BACKEND="flat_square"; BETA="${BETA_HIGH}" ;;
  *) echo "Unknown SLURM_ARRAY_TASK_ID=${SLURM_ARRAY_TASK_ID}" >&2; exit 2 ;;
esac

TAG="sample_${BACKEND}_L${L}_beta${BETA}_Nth${N_THERM}_Ns${N_SAMPLES}_Nbs${N_BETWEEN}"
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
  echo "beta=${BETA}"
  echo "N_thermalization=${N_THERM}"
  echo "N_samples=${N_SAMPLES}"
  echo "N_between_samples=${N_BETWEEN}"
  echo "N_resamples=${N_RESAMPLES}"
  echo "observables=energy anyon_count sigma_x star_x plaquette_z"
  echo "--- run ---"
  start_ms=$(date +%s%3N)
  set +e
  "${PARATORIC_EXE}" \
    --simulation etc_sample \
    --lattice_backend "${BACKEND}" \
    --N_thermalization "${N_THERM}" \
    --N_samples "${N_SAMPLES}" \
    --N_between_samples "${N_BETWEEN}" \
    --N_resamples "${N_RESAMPLES}" \
    --custom_therm 0 \
    --seed "${SEED}" \
    --beta "${BETA}" \
    --h_constant 0.3 \
    --h_constant_therm 0.3 \
    --mu_constant 1.0 \
    --J_constant 1.0 \
    --lmbda_constant 0.2 \
    --lmbda_constant_therm 0.2 \
    --observables energy anyon_count sigma_x star_x plaquette_z \
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
  rc=$?
  set -e
  end_ms=$(date +%s%3N)
  wall_ms=$((end_ms - start_ms))
  printf "wall_seconds=%d.%03d\n" "$((wall_ms / 1000))" "$((wall_ms % 1000))"
  exit "${rc}"
} 2>&1 | tee "${RESULT_FILE}"
