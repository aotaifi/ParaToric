# Linsel no-PT L=10/L=20 blocking-validity audit

Date: 2026-07-20  
Jobs: L=10 `15149119`; L=20 `15159599`  
Primary series: `plaquette_percolation_strength`; primary reported statistic: raw ratio `<P^4>/<P^2>^2`.

The registered `scripts/ensemble_diagnostics.py blocking-validity` command was run on all 144 `obs.h5` files with `--sample-period-sweeps 25`, four cold plus four hot chains, and four suffix windows. Blocking tau is estimated per strength-series chain; a point with any unresolved chain is reported as a lower bound and Neff upper bound. The raw-ratio bootstrap column is the existing full-series IPS-sized hierarchical block bootstrap (`no-pt-benchmark`, 1,000 resamples, seed 20260720). The raw-ratio between-chain error is `std(population)(eight per-chain raw ratios)/sqrt(8)`. Cold/hot z and suffix p below are also computed on the raw ratio. All suffix windows were non-degenerate.

| L | h | blocking status | tau_blocking (sweeps) | chain tau range (sweeps) | Neff per chain | old IPS/bootstrap raw-ratio error | raw-ratio between-chain error | between/bootstrap | raw-ratio cold/hot z | raw-ratio suffix p |
|---:|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 10 | 0.305629247317 | TAU-UNRESOLVED;TAU-SPREAD-FLAG | >=32790.41262 | 19.52354661:32790.41262 | <=606.8847084 | 0.06199833854 | 0.05337633586 | 0.8609317139 | 1.555549145 | 0.8686852391 |
| 10 | 0.318989179906 | TAU-UNRESOLVED;TAU-SPREAD-FLAG | >=27138.96109 | 2247.858836:27138.96109 | <=733.2631464 | 0.04404942297 | 0.04399257121 | 0.9987093643 | 0.4813833507 | 0.01536682386 |
| 10 | 0.322000000000 | TAU-UNRESOLVED | >=25641.5017 | 13106.55633:25641.5017 | <=776.085591 | 0.02337526678 | 0.02236448511 | 0.9567584971 | 0.7125611211 | 0.01493113243 |
| 10 | 0.326000000000 | TAU-UNRESOLVED;TAU-SPREAD-FLAG | >=21632.59503 | 7576.265708:21632.59503 | <=919.908128 | 0.008375103874 | 0.007765576459 | 0.9272215098 | 0.6072709322 | 0.07239954202 |
| 10 | 0.330615254621 | TAU-UNRESOLVED;TAU-SPREAD-FLAG | >=16113.88185 | 6008.410479:16113.88185 | <=1234.96003 | 0.01014653547 | 0.009960422901 | 0.9816575254 | 0.05009431591 | 0.2730291482 |
| 10 | 0.335362847648 | TAU-UNRESOLVED;TAU-SPREAD-FLAG | >=10650.97764 | 3982.638357:10650.97764 | <=1868.373089 | 0.007951752595 | 0.007786741147 | 0.9792484178 | 0.149910529 | 0.5894224758 |
| 10 | 0.339573841268 | TAU-UNRESOLVED | >=5362.797314 | 2835.998039:5362.797314 | <=3710.749975 | 0.006496267576 | 0.006176416468 | 0.9507638649 | 0.04647997743 | 0.1538230408 |
| 10 | 0.343520981103 | TAU-UNRESOLVED;TAU-SPREAD-FLAG | >=2502.938148 | 1174.108659:2502.938148 | <=7950.655919 | 0.002762914519 | 0.002010426657 | 0.7276470708 | 0.3277858352 | 0.03573714541 |
| 10 | 0.347398758564 | TAU-UNRESOLVED;TAU-SPREAD-FLAG | >=1682.67025 | 529.5755364:1682.67025 | <=11826.44074 | 0.003813231892 | 0.003423837635 | 0.897883405 | 1.866343515 | 0.7743139356 |
| 20 | 0.305629247317 | TAU-RESOLVED | 21.59557778 | 21.30022701:24.21289437 | 1477154.273 | 0.0003424084873 | 0.0002410501289 | 0.7039840947 | 0.3282572472 | 0.3280062215 |
| 20 | 0.318989179906 | TAU-RESOLVED | 21.1243447 | 20.27770625:22.37800888 | 1510106.015 | 0.0003796870955 | 0.0002996144842 | 0.7891089471 | 2.183486442 | 0.9945946417 |
| 20 | 0.322000000000 | TAU-RESOLVED | 24.17870787 | 20.48339862:28.91673701 | 1319342.629 | 0.0002965117955 | 0.0001454337683 | 0.4904822355 | 1.171749065 | 0.1462650042 |
| 20 | 0.326000000000 | TAU-UNRESOLVED;TAU-SPREAD-FLAG | >=953.9651511 | 21.98550625:953.9651511 | <=33439.37665 | 0.002141896223 | 0.002095020558 | 0.9781148757 | 0.9651353172 | 0.6152983942 |
| 20 | 0.330615254621 | TAU-UNRESOLVED;TAU-SPREAD-FLAG | >=791.5011575 | 35.93893331:791.5011575 | <=40303.16279 | 0.004555611418 | 0.004630632959 | 1.016467941 | 1.472832425 | 0.6109032441 |
| 20 | 0.335362847648 | TAU-UNRESOLVED;TAU-SPREAD-FLAG | >=1536.741374 | 415.0204209:1536.741374 | <=20758.20989 | 0.003989056773 | 0.003872962207 | 0.9708967376 | 0.7600004625 | 0.00389281384 |
| 20 | 0.339573841268 | TAU-UNRESOLVED | >=2157.001764 | 1198.245579:2157.001764 | <=14789.04679 | 0.002859437966 | 0.002673423387 | 0.9349471536 | 0.4187693642 | 0.2021229828 |
| 20 | 0.343520981103 | TAU-SPREAD-FLAG | 565.8010583 | 325.5074248:809.4643964 | 56380.24096 | 0.002161167507 | 0.001889569171 | 0.8743279569 | 1.016073791 | 0.3963590799 |
| 20 | 0.347398758564 | TAU-RESOLVED | 104.2154264 | 91.29551677:153.4631517 | 306096.7183 | 0.001559194956 | 0.001339054957 | 0.8588117555 | 0.3958651022 | 0.9932107137 |

## Pre-committed predictions and verdict

> PREDICTION 1: L=20 control h=0.305629247317 PASSES: flat blocking plateau ~44 sweeps, bootstrap and between-chain errors AGREE (ratio near 1), full-ladder constant-fit p already ~0.0528 (i.e. should now clear the >0.05 bar or be very close).

**FAIL as stated.** The control is otherwise valid (`TAU-RESOLVED`, cold/hot z `0.3282572472`, suffix p `0.3280062215`), but the plateau is `21.59557778` sweeps with chain range `21.30022701:24.21289437`, not approximately 44 sweeps. Its raw-ratio between/bootstrap ratio is `0.7039840947`.

> PREDICTION 2: L=20 decisive point h=0.339573841268: error bars grow >=2x (vs whatever the OLD IPS-based error was previously reported as), tau resolves to >=3300 sweeps OR comes back TAU-UNRESOLVED, and stationarity p rises above 0.05.

**FAIL.** The tau clause passes via `TAU-UNRESOLVED` (`tau >=2157.001764` sweeps) and raw-ratio suffix p is `0.2021229828`, but the error grows by `0.9349471536x`, not `>=2x`: old IPS/bootstrap `0.002859437966`, between-chain `0.002673423387`.

Overall decision-rule verdict: **STILL REAL**. Both predictions do not hold, so step 3 remains justified. Across all 18 points, between/bootstrap ratios span `0.4904822355:1.016467941`; none shows the predicted upward error inflation, and the decisive point is near one. This refutes the proposed systematic error-bar defect; keep the existing bars and do not clear the L=20 void.

## Tau-location secondary check

L=20 has the largest reported lower bound at `h=0.339573841268` (`>=2157.001764` sweeps), with `h=0.335362847648` at `>=1536.741374` and `h=0.343520981103` at `565.8010583`. However, the claim of a genuine isolated peak with a flat floor elsewhere is **not established**: `h=0.326000000000` and `h=0.330615254621` are also unresolved at `>=953.9651511` and `>=791.5011575` sweeps, so unresolved neighboring values cannot be ranked as a flat floor or below the peak.

L=10 does show a strict monotonic rise toward smaller h in the reported lower bounds: `>=1682.67025`, `>=2502.938148`, `>=5362.797314`, `>=10650.97764`, `>=16113.88185`, `>=21632.59503`, `>=25641.5017`, `>=27138.96109`, `>=32790.41262` sweeps from h `0.347398758564` down to `0.305629247317`. Every L=10 point is tau-unresolved, so this supports the monotonic frozen-side pattern but does not resolve plateau tau values.

## Instrument caveat

The `blocking-validity` subcommand's printed `existing_pooled_mean_error`, `between_chain_error`, `cold_hot_z`, and `suffix_constant_p` are statistics of the mean strength, not the nonlinear raw ratio. Moreover, its two printed mean-strength errors are the same eight chain means with `ddof=1` versus `ddof=0`, making their printed ratio identically `sqrt(7/8)=0.9354143467` at every point. The primary table therefore supplements the unmodified blocking-tau output with the registered raw-ratio bootstrap and direct per-chain raw-ratio calculations; no source code was changed.
