#!/usr/bin/env bash
#SBATCH --job-name=pt-g2-sector-rate
#SBATCH --partition=cluster
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem-per-cpu=1G
#SBATCH --time=20:00:00
#SBATCH --array=0-7
#SBATCH --output=/scratch/a/A.Otaifi/paratoric_g2_sector_rate_scan_20260710/logs/%x_%A_%a.out
#SBATCH --error=/scratch/a/A.Otaifi/paratoric_g2_sector_rate_scan_20260710/logs/%x_%A_%a.err

set -euo pipefail

REPO_ROOT="${REPO_ROOT:-/project/theorie/a/A.Otaifi/ParaToric}"
PARATORIC_EXE="${PARATORIC_EXE:-${REPO_ROOT}/build-current-clang-portable/paratoric}"
OUT_ROOT="${OUT_ROOT:-/scratch/a/A.Otaifi/paratoric_g2_sector_rate_scan_20260710}"
SEED_BASE="${SEED_BASE:-710120}"

L="${L:-120}"
BETA="${BETA:-120}"
N_THERM="${N_THERM:-1000000000}"
N_SAMPLES="${N_SAMPLES:-10000}"
N_BETWEEN="${N_BETWEEN:-5000000}"
N_RESAMPLES="${N_RESAMPLES:-200}"

mkdir -p "${OUT_ROOT}/logs" "${OUT_ROOT}/results"

case "${SLURM_ARRAY_TASK_ID}" in
  0) S_VALUE="0.10" ;;
  1) S_VALUE="0.16" ;;
  2) S_VALUE="0.22" ;;
  3) S_VALUE="0.28" ;;
  4) S_VALUE="0.34" ;;
  5) S_VALUE="0.40" ;;
  6) S_VALUE="0.46" ;;
  7) S_VALUE="0.50" ;;
  *) echo "Unknown SLURM_ARRAY_TASK_ID=${SLURM_ARRAY_TASK_ID}" >&2; exit 2 ;;
esac

SEED=$((SEED_BASE + SLURM_ARRAY_TASK_ID))
TAG="sector_rate_L${L}_beta${BETA}_s${S_VALUE}_Nth${N_THERM}_Ns${N_SAMPLES}_Nbs${N_BETWEEN}_seed${SEED}"
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
  echo "scan=G2 wide-s sector-rate"
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
    --observables energy energy_h energy_lmbda anyon_count \
    --basis z \
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
