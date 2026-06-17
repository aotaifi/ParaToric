#!/usr/bin/env bash
#SBATCH --job-name=paratoric-prod-backend
#SBATCH --partition=small
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem=32G
#SBATCH --time=14-00:00:00
#SBATCH --array=0-8
#SBATCH --output=/scratch/a/A.Otaifi/paratoric_backend_production_grid/logs/%x_%A_%a.out
#SBATCH --error=/scratch/a/A.Otaifi/paratoric_backend_production_grid/logs/%x_%A_%a.err

set -euo pipefail

REPO_ROOT="${REPO_ROOT:-/project/theorie/a/A.Otaifi/ParaToric}"
PARATORIC_EXE="${PARATORIC_EXE:-${REPO_ROOT}/build-current-clang-portable/paratoric}"
OUT_ROOT="${OUT_ROOT:-/scratch/a/A.Otaifi/paratoric_backend_production_grid}"
SEED_BASE="${SEED_BASE:-24601}"
N_THERM="${N_THERM:-1000000000}"
N_SAMPLES="${N_SAMPLES:-2000}"
N_BETWEEN="${N_BETWEEN:-5000000}"
N_RESAMPLES="${N_RESAMPLES:-500}"

mkdir -p "${OUT_ROOT}/logs" "${OUT_ROOT}/results"

case "${SLURM_ARRAY_TASK_ID}" in
  0) BACKEND="boost";       L="200" ;;
  1) BACKEND="flat_square"; L="200" ;;
  2) BACKEND="boost";       L="400" ;;
  3) BACKEND="flat_square"; L="400" ;;
  4) BACKEND="boost";       L="600" ;;
  5) BACKEND="flat_square"; L="600" ;;
  6) BACKEND="boost";       L="1000" ;;
  7) BACKEND="flat_square"; L="1000" ;;
  8) BACKEND="boost";       L="200"; OBS_MODE="simple" ;;
  *) echo "Unknown SLURM_ARRAY_TASK_ID=${SLURM_ARRAY_TASK_ID}" >&2; exit 2 ;;
esac

BETA=$((2 * L))
SEED=$((SEED_BASE + SLURM_ARRAY_TASK_ID))
OBS_MODE="${OBS_MODE:-${BACKEND}}"

if [[ "${OBS_MODE}" == "boost" ]]; then
  OBSERVABLES=(energy percolation_probability percolation_strength star_x plaquette_z)
else
  OBSERVABLES=(energy star_x plaquette_z)
fi

TAG="prod_${BACKEND}_${OBS_MODE}obs_L${L}_beta${BETA}_Nth${N_THERM}_Ns${N_SAMPLES}_Nbs${N_BETWEEN}"
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
  echo "seed=${SEED}"
  echo "obs_mode=${OBS_MODE}"
  echo "observables=${OBSERVABLES[*]}"
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
    --observables "${OBSERVABLES[@]}" \
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
