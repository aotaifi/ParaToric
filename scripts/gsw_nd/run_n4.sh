#!/bin/bash
# Gate N4: timing and memory, dense vs nd, one core, one node, s*, beta=L, periodic, gsw-only, 5 therm + 15 timed steps
# (same protocol as lead_timing48). usage: run_n4.sh OUTDIR
O=${1:?}
mkdir -p $O
T=/scratch/a/A.Otaifi/gsw_cpp_20261011/nd/time_one.sh
hostname; lscpu | grep "Model name"
run() { echo "=== $*"; $T $O "$@" 2>&1 | grep -v "Autocorrelation" | cut -c1-500; }
# args after OUTDIR: NAME L SOLVER NTH NS DV DR LEAF
run dense_L16 16 dense 5 15 4096 0 64
run nd_L16_dv0 16 nd 5 15 0 0 64
run nd_L16_dv4096 16 nd 5 15 4096 0 64
run dense_L32 32 dense 5 15 4096 0 64
run nd_L32_dv0 32 nd 5 15 0 0 64
run nd_L32_dv2048 32 nd 5 15 2048 0 64
run nd_L32_dv4096 32 nd 5 15 4096 0 64
run nd_L48_dv0 48 nd 5 15 0 0 64
run nd_L48_dv4096 48 nd 5 15 4096 0 64
run nd_L48_dv8192 48 nd 5 15 8192 0 64
run nd_L48_dv0_dr1500 48 nd 5 15 0 1500 64
run nd_L48_dv0_leaf16 48 nd 5 15 0 0 16
run nd_L48_dv0_leaf256 48 nd 5 15 0 0 256
run nd_L64_dv0 64 nd 5 15 0 0 64
run nd_L64_dv4096 64 nd 5 15 4096 0 64
run dense_L48 48 dense 5 15 4096 0 64
echo N4_DONE
