# Why Bpt_on runs are ~10× slower than Bpt_off (beta60 1M100k)

## Observed times (from slurm output)

- **Bpt_off** (swap_enabled=false): ~2000 s for 1M production sweeps (one beta).
- **Bpt_on** (swap_enabled=true): ~21464 s for 1M production sweeps (one beta).

So wall time per 1M sweeps is ~10× higher when PT (swap) is on.

## Cause: synchronization + load imbalance

With **swap_enabled=true**:

1. **Sync every `swap_period` (20k) sweeps**  
   In `worm.update.cpp`, when `do_swap_global` is true (every 20k sweeps), the code does:
   - `MPI_Allgather` so all 60 ranks exchange `beta_index`.
   - Then 30 swap pairs do `MPI_Sendrecv` (energy, vertex count, accept).

   So **all 60 ranks must reach the same sweep count** (20k, 40k, …) before any can proceed. That acts as a **global barrier** every 20k sweeps.

2. **Wall time = pace of the slowest rank**  
   The time for each 20k-sweep segment is the **max** over the 60 ranks. So total wall time for 1M sweeps is set by the **slowest** rank, not by rank 0.

3. **High-beta replicas are much slower per sweep**  
   High β (e.g. β≈60) has many more vertices in the operator string, so each sweep (same number of update attempts) does more work. So one or a few “high-beta” ranks can be an order of magnitude slower per sweep than low-beta ranks.

4. **Result**  
   Rank 0 (low β) could finish 1M sweeps in ~2000 s on its own, but with PT on it has to wait at each 20k boundary for the slow ranks. So the **reported elapsed time** (wall clock) is ~21464 s: mostly **waiting at sync points**, not “more updates” or “heavier updates” on rank 0.

So the slowdown is **not** from doing more or heavier local updates when swap is on; it’s from **synchronization and load imbalance**: the run is paced by the slowest replica.

## Code references

- `worm.update.cpp` (around 74–145): `swap_enabled` block; `need_mapping` → `MPI_Allgather` when `do_swap_global` (every `swap_period`); then `SWAP(partner, …)`.
- Progress is printed by rank 0 only; “elapsed” is wall clock since run start, so it includes waiting at barriers.

## What can be done

1. **Accept longer wall time for PT-on**  
   For 60 betas × 1M sweeps with PT on, wall time is dominated by the slowest replica; ~15 days per run is consistent with that.

2. **Reduce load imbalance**  
   - Use fewer betas so the slowest replica is not so extreme, or  
   - Use a smaller max β, or  
   - Coarsen the grid so the “slow” β values are fewer.

3. **Larger `swap_period`**  
   Increasing `swap_period` (e.g. 40k or 50k) reduces sync frequency and might reduce barrier overhead slightly, but the main cost is still waiting for the slowest rank each time, so the gain is limited.

4. **Shorter runs when PT is on**  
   For the same wall-time budget (e.g. 3 days), reduce total sweeps or number of betas so that the slowest rank can finish within the budget.

---

## Heartbeat / per-rank tracking runs (run_tracker)

A separate **finalize-heartbeat diagnostics** batch was submitted so we can see output from **each rank** and keep track of where ranks stall.

**What it does:** Prints from every rank during the **finalize stage** (after `sim.run()`, at `MPI_Barrier` and collective_merge). With `finalize_rank_verbose=true`, each rank logs to stderr lines like `# [finalize][rank N] enter/leave/waiting in ...` and a heartbeat every `finalize_heartbeat_seconds` (120 s) while waiting in the barrier. So you can see which rank has not yet reached or left the barrier (useful for finalize stalls).

**Where:** Run tracker entry **2026-03-13 18:39 CET — Finalize-heartbeat diagnostics batch**; batch index **C**.

**Job IDs (from run_tracker):**
- `13176288` — Bptdiag_fin_grid_build_on_seed30_all_down_100k
- `13176289` — Bptdiag_fin_manual_grid_input_off_seed30_all_down_100k
- `13176290` — Bptdiag_fin_manual_grid_input_on_seed30_all_down_100k

**Run root:** `/scratch/a/A.Otaifi/RUBY_SS/000_temp_B_pt_qmc_loop_beta60_finalize_diag_100k_20260313`

**Binary:** `qmc_swap_qmc_loop_finalize_diag_20260313` (md5 `233b9de...`). Source: `ruby_clsuter/code/code_ruby_tampered_qmc_loop_lab/`.

**Params (in `params.ini`):** `finalize_diag=true`, `finalize_rank_verbose=true`, `finalize_heartbeat_seconds=120`, `finalize_watchdog_seconds=1800`, `finalize_abort_on_watchdog=false`.

**Setup script:** `ruby_clsuter/scripts/setup_submit_qmc_loop_beta60_finalize_diag_100k_20260313.sh`  
**Job list:** `ruby_clsuter/last_jobs_pt_qmc_loop_beta60_finalize_diag_100k_20260313.txt`

**Note:** This heartbeat is **finalize-stage only** (post–sweep-loop barriers). It does not print per-rank progress during the sweep loop; main production still reports progress only from rank 0. To track per-rank progress *during* the run you’d need a separate in-loop heartbeat (e.g. periodic progress from all ranks).
