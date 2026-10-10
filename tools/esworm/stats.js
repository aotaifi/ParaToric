'use strict';
// Binning error and integrated autocorrelation time.

function mean(x) { let s = 0; for (let i = 0; i < x.length; i++) s += x[i]; return s / x.length; }

// error of the mean from power-of-two binning; reported at the largest bin size with >= minBins bins.
// Also returns the whole table (bin size, error) so a plateau can be inspected.
function binnedError(x, minBins) {
  minBins = minBins || 32;
  let cur = Array.from(x);
  const table = [];
  let size = 1;
  while (cur.length >= 2) {
    const n = cur.length, m = mean(cur);
    let v = 0; for (let i = 0; i < n; i++) v += (cur[i] - m) * (cur[i] - m);
    const err = Math.sqrt(v / (n - 1) / n);
    table.push({ size, nbins: n, err });
    if (n < minBins) { table.pop(); break; }
    const nxt = [];
    for (let i = 0; i + 1 < n; i += 2) nxt.push(0.5 * (cur[i] + cur[i + 1]));
    cur = nxt; size *= 2;
  }
  const last = table.length ? table[table.length - 1] : { size: 1, nbins: x.length, err: NaN };
  return { err: last.err, size: last.size, nbins: last.nbins, table };
}

// Sokal automatic windowing: tau_int = 1/2 + sum_{t=1}^{W} rho(t), W = smallest t with t >= c*tau(t)
function tauInt(x, c) {
  c = c || 6;
  const n = x.length, m = mean(x);
  const y = new Float64Array(n); for (let i = 0; i < n; i++) y[i] = x[i] - m;
  let c0 = 0; for (let i = 0; i < n; i++) c0 += y[i] * y[i]; c0 /= n;
  if (!(c0 > 0)) return { tau: NaN, window: 0 };
  let tau = 0.5, W = 0;
  const maxW = Math.min(n - 1, Math.floor(n / 4));
  for (let t = 1; t <= maxW; t++) {
    let s = 0; for (let i = 0; i + t < n; i++) s += y[i] * y[i + t];
    const rho = s / (n - t) / c0;
    tau += rho; W = t;
    if (t >= c * tau) break;
  }
  return { tau, window: W };
}

module.exports = { mean, binnedError, tauInt };
