'use strict';
// Multi-histogram (Ferrenberg-Swendsen / WHAM) reweighting of gSW series along the self-dual line h = lmbda = s (mu = J = 1, beta = L).
// Per-configuration log weight at coupling s, up to an s-only constant:  log w(s) = n_h ln s + s Z,
//   n_h = -beta E_h (col 4),  Z = int sum sigma^z dtau = -beta E_lmbda / s0 (col 5; lmbda = s0 of the run that produced the sample).
// WHAM:  P_s(x) ~ w_s(x) / D(x),  D(x) = sum_k N_k exp(f_k) w_k(x),  exp(-f_k) = sum_x w_k(x)/D(x),  f_0 = 0.
// Solved by Newton on the convex dual  L(f) = sum_x ln D(x) - sum_k N_k f_k  (gradient sum_x p_k(x) - N_k, Hessian sum_x diag(p) - p p^T,
// p_k = N_k e^{f_k} w_k / D), log-sum-exp throughout, until max|df| < 1e-10.
// Errors: delete-one-group jackknife over chain index (group c = chain c of every input point), full WHAM re-solve for each group.
// Assumes equal integrated autocorrelation times at all input points (N_k = number of samples, no tau_k correction).
// usage: node mhist.js <beta> <target s> <point dir> <s of point> [<point dir> <s of point> ...] [flags]
//   <point dir> holds runs/c*/ckpt_series.txt (columns as in interim.js; make them with ckptseries.js); pass the runs directory.
//   --exclude-target  drop the input point(s) with s == target from the WHAM (leave-one-out); they are still loaded for --direct
//   --direct          also print the plain cumulants of the input point(s) with s == target (mean of per-chain values, chain-scatter error)
//                     and z = (pred - direct)/sqrt(err_pred^2 + err_direct^2)
//   --discard=0.1     fraction of every chain dropped from the start
//   --thin=1          keep every thin-th sample (memory)
//   --break-dz=<d>    test only: multiply the weight of WHAM input 0 by exp(d Z)
//   --break-sign      test only: flip the sign of n_h ln s in the weight of WHAM input 0
// Observables: <S>, kappa2(S), kappa3(S), S = plaquette_z_tau_avg + star_x; kappa2(delta), Binder kappa4/kappa2^2 of delta (excess form
// (m4 - 3 k2^2)/k2^2 as in reweight.js); ESS = (sum W)^2/sum W^2 for the WHAM weights W = w_target/D over the pooled samples.
const fs = require('fs'), path = require('path');

function loadPoint(dir, s0, beta, frac, thin, minSamples) {
  const cs = fs.readdirSync(dir).filter(d => /^c\d+$/.test(d)).sort((a, b) => +a.slice(1) - +b.slice(1));
  const chunks = [];
  for (const c of cs) {
    const f = path.join(dir, c, 'ckpt_series.txt'); if (!fs.existsSync(f)) continue;
    const buf = fs.readFileSync(f); const rows = [];
    let a = 0;
    for (let nl = buf.indexOf(10, a); a < buf.length; nl = buf.indexOf(10, a)) {
      if (nl < 0) nl = buf.length;
      if (nl > a) rows.push([a, nl]);
      a = nl + 1;
    }
    const i0 = Math.floor(frac * rows.length);
    const m = Math.floor((rows.length - i0 + thin - 1) / thin); if (m < minSamples) continue;
    const ch = { id: +c.slice(1), n: m, nh: new Float64Array(m), Z: new Float64Array(m), S: new Float64Array(m), D: new Float64Array(m) };
    for (let j = 0; j < m; j++) {
      const [lo, hi] = rows[i0 + j * thin]; const v = [];
      let p = lo, col = 0;
      while (p < hi && col < 6) { let q = buf.indexOf(32, p); if (q < 0 || q > hi) q = hi; v.push(+buf.toString('latin1', p, q)); p = q + 1; col++; }
      if (v.length < 6 || v.some(Number.isNaN)) throw new Error(`bad row ${j} in ${f}`);
      ch.S[j] = v[1] + v[2]; ch.D[j] = v[3]; ch.nh[j] = -beta * v[4]; ch.Z[j] = -beta * v[5] / s0;
    }
    chunks.push(ch);
  }
  return { dir, s: s0, chains: chunks, n: chunks.reduce((a, c) => a + c.n, 0) };
}

function pooled(points) { // concatenate chains of several points into flat arrays with point and group labels
  const n = points.reduce((a, p) => a + p.n, 0);
  const d = { n, nh: new Float64Array(n), Z: new Float64Array(n), S: new Float64Array(n), D: new Float64Array(n), grp: new Int32Array(n), pt: new Int32Array(n) };
  let o = 0;
  points.forEach((p, k) => { for (const c of p.chains) { d.nh.set(c.nh, o); d.Z.set(c.Z, o); d.S.set(c.S, o); d.D.set(c.D, o); d.grp.fill(c.id, o, o + c.n); d.pt.fill(k, o, o + c.n); o += c.n; } });
  return d;
}

function solveSmall(H, g, K) { // Gaussian elimination with partial pivoting; H is K x K row-major, solves H x = g
  const A = Array.from({ length: K }, (_, i) => [...H.slice(i * K, i * K + K), g[i]]);
  for (let i = 0; i < K; i++) {
    let piv = i; for (let r = i + 1; r < K; r++) if (Math.abs(A[r][i]) > Math.abs(A[piv][i])) piv = r;
    [A[i], A[piv]] = [A[piv], A[i]];
    if (Math.abs(A[i][i]) < 1e-300) throw new Error('singular WHAM Hessian');
    for (let r = i + 1; r < K; r++) { const q = A[r][i] / A[i][i]; for (let c = i; c <= K; c++) A[r][c] -= q * A[i][c]; }
  }
  const x = new Array(K).fill(0);
  for (let i = K - 1; i >= 0; i--) { let t = A[i][K]; for (let c = i + 1; c < K; c++) t -= A[i][c] * x[c]; x[i] = t / A[i][i]; }
  return x;
}

// A[k], B[k]: log w_k(x) = nh A[k] + Z B[k]  (relative to the reference coupling sref; the shift is a common base-measure factor)
function makeWham(d, A, B) {
  const K = A.length;
  const lnD = new Float64Array(d.n);
  function counts(drop) { const N = new Array(K).fill(0); for (let i = 0; i < d.n; i++) if (d.grp[i] !== drop) N[d.pt[i]]++; return N; }
  function pass(f, N, drop, wantH) {
    const lnN = N.map(Math.log), a = new Float64Array(K), g = new Float64Array(K), H = new Float64Array(K * K);
    for (let i = 0; i < d.n; i++) {
      if (d.grp[i] === drop) continue;
      const nh = d.nh[i], Z = d.Z[i]; let m = -Infinity;
      for (let k = 0; k < K; k++) { const v = lnN[k] + f[k] + nh * A[k] + Z * B[k]; a[k] = v; if (v > m) m = v; }
      let sum = 0; for (let k = 0; k < K; k++) { a[k] = Math.exp(a[k] - m); sum += a[k]; }
      lnD[i] = m + Math.log(sum); const inv = 1 / sum;
      for (let k = 0; k < K; k++) { const p = a[k] * inv; g[k] += p; if (wantH) { H[k * K + k] += p; for (let l = 0; l < K; l++) H[k * K + l] -= p * (a[l] * inv); } }
    }
    for (let k = 0; k < K; k++) g[k] -= N[k];
    return { g, H };
  }
  function solve(f0, drop, tol = 1e-10) {
    const N = counts(drop), f = Float64Array.from(f0); f[0] = 0;
    for (let it = 0; it < 60; it++) {
      const { g, H } = pass(f, N, drop, true);
      const R = K - 1; if (R === 0) return { f, N, it };
      const Hr = new Float64Array(R * R), gr = new Array(R);
      for (let i = 0; i < R; i++) { gr[i] = g[i + 1]; for (let j = 0; j < R; j++) Hr[i * R + j] = H[(i + 1) * K + j + 1]; }
      const dx = solveSmall(Hr, gr, R); let mx = 0;
      for (let i = 0; i < R; i++) { const s = Math.max(-2, Math.min(2, -dx[i])); f[i + 1] += s; mx = Math.max(mx, Math.abs(s)); }
      if (mx < tol) return { f, N, it: it + 1 };
    }
    throw new Error('WHAM did not converge');
  }
  const W = new Float64Array(d.n);
  function observe(f, N, drop, At, Bt) { // lnD must be current for (f, N, drop): pass() was just called by the caller
    pass(f, N, drop, false);
    let mx = -Infinity;
    for (let i = 0; i < d.n; i++) if (d.grp[i] !== drop) { const r = d.nh[i] * At + d.Z[i] * Bt - lnD[i]; W[i] = r; if (r > mx) mx = r; }
    let sw = 0, sw2 = 0, mS = 0, mD = 0;
    for (let i = 0; i < d.n; i++) if (d.grp[i] !== drop) { const w = Math.exp(W[i] - mx); W[i] = w; sw += w; sw2 += w * w; mS += w * d.S[i]; mD += w * d.D[i]; }
    mS /= sw; mD /= sw;
    let s2 = 0, s3 = 0, d2 = 0, d4 = 0;
    for (let i = 0; i < d.n; i++) if (d.grp[i] !== drop) {
      const w = W[i], x = d.S[i] - mS, y = d.D[i] - mD, y2 = y * y;
      s2 += w * x * x; s3 += w * x * x * x; d2 += w * y2; d4 += w * y2 * y2;
    }
    s2 /= sw; s3 /= sw; d2 /= sw; d4 /= sw;
    return { mS, k2S: s2, k3S: s3, k2D: d2, bD: (d4 - 3 * d2 * d2) / (d2 * d2), ess: sw * sw / sw2 };
  }
  return { solve, observe, counts };
}

const OBS = ['mS', 'k2S', 'k3S', 'k2D', 'bD'];
const LABEL = { mS: '<S>', k2S: 'kappa2(S)', k3S: 'kappa3(S)', k2D: 'kappa2(delta)', bD: 'Binder(delta)' };

// Returns {pred: {obs: [value, err]}, ess, essFrac, f, ...}
function predict(points, target, beta, opts = {}) {
  const K = points.length, sref = points.reduce((a, p) => a + p.s, 0) / K;
  const A = points.map(p => Math.log(p.s / sref)), B = points.map(p => p.s - sref);
  if (opts.breakSign) A[0] = -A[0];
  if (opts.breakDz) B[0] += opts.breakDz;
  const d = pooled(points), wham = makeWham(d, A, B);
  // initial f from thermodynamic integration d ln Z / ds = <n_h>/s + <Z>  (own-sample means; Newton does the rest)
  const dl = points.map((p, k) => { let nh = 0, Z = 0; for (const c of p.chains) for (let j = 0; j < c.n; j++) { nh += c.nh[j]; Z += c.Z[j]; } return nh / p.n / p.s + Z / p.n; });
  const ord = points.map((_, k) => k).sort((a, b) => points[a].s - points[b].s), lnZ = new Array(K).fill(0);
  const sRef = ord.findIndex(k => points[k].s >= sref - 1e-12);
  for (let j = sRef + 1; j < K; j++) lnZ[ord[j]] = lnZ[ord[j - 1]] + 0.5 * (dl[ord[j]] + dl[ord[j - 1]]) * (points[ord[j]].s - points[ord[j - 1]].s);
  for (let j = sRef - 1; j >= 0; j--) lnZ[ord[j]] = lnZ[ord[j + 1]] - 0.5 * (dl[ord[j]] + dl[ord[j + 1]]) * (points[ord[j + 1]].s - points[ord[j]].s);
  const f0 = lnZ.map(v => -(v - lnZ[0]));
  const At = Math.log(target / sref), Bt = target - sref;
  const full = wham.solve(f0, -1);
  const obs = wham.observe(full.f, full.N, -1, At, Bt);
  const groups = [...new Set(Array.from(d.grp))].sort((a, b) => a - b);
  const jk = [];
  for (const g of groups) { const r = wham.solve(full.f, g); jk.push(wham.observe(r.f, r.N, g, At, Bt)); }
  const G = jk.length, pred = {};
  for (const o of OBS) { const v = jk.map(r => r[o]), m = v.reduce((a, b) => a + b, 0) / G; pred[o] = [obs[o], Math.sqrt((G - 1) / G * v.reduce((a, b) => a + (b - m) ** 2, 0))]; }
  return { pred, ess: obs.ess, essFrac: obs.ess / d.n, nTot: d.n, f: full.f, newton: full.it, groups: G };
}

function direct(point) { // plain per-chain cumulants, mean over chains, chain-scatter error (as interim.js)
  const rows = point.chains.map(c => {
    const n = c.n; let mS = 0, mD = 0; for (let j = 0; j < n; j++) { mS += c.S[j]; mD += c.D[j]; } mS /= n; mD /= n;
    let s2 = 0, s3 = 0, d2 = 0, d4 = 0;
    for (let j = 0; j < n; j++) { const x = c.S[j] - mS, y = c.D[j] - mD; s2 += x * x; s3 += x * x * x; d2 += y * y; d4 += y ** 4; }
    s2 /= n; s3 /= n; d2 /= n; d4 /= n;
    return { mS, k2S: s2, k3S: s3, k2D: d2, bD: (d4 - 3 * d2 * d2) / (d2 * d2) };
  });
  const out = {}, n = rows.length;
  for (const o of OBS) { const v = rows.map(r => r[o]), m = v.reduce((a, b) => a + b, 0) / n; out[o] = [m, Math.sqrt(v.reduce((a, b) => a + (b - m) ** 2, 0) / (n - 1) / n)]; }
  return out;
}

module.exports = { loadPoint, predict, direct, OBS, LABEL };

if (require.main === module) {
  const args = process.argv.slice(2), flags = args.filter(a => a.startsWith('--')), pos = args.filter(a => !a.startsWith('--'));
  const flag = (n, def) => { const f = flags.find(x => x.startsWith(`--${n}=`)); return f ? +f.split('=')[1] : def; };
  if (pos.length < 4 || pos.length % 2) { console.error('usage: node mhist.js <beta> <target s> <point dir> <s of point> [<dir> <s> ...] [--exclude-target] [--direct] [--discard=0.1] [--thin=1] [--break-dz=d] [--break-sign]'); process.exit(1); }
  const beta = +pos[0], target = +pos[1], frac = flag('discard', 0.1), thin = flag('thin', 1), t0 = Date.now();
  const all = []; for (let i = 2; i < pos.length; i += 2) all.push(loadPoint(pos[i], +pos[i + 1], beta, frac, thin, 200));
  const isT = p => Math.abs(p.s - target) < 1e-9;
  const inputs = flags.includes('--exclude-target') ? all.filter(p => !isT(p)) : all;
  if (!inputs.length) { console.error('no WHAM input left'); process.exit(1); }
  const smin = Math.min(...inputs.map(p => p.s)), smax = Math.max(...inputs.map(p => p.s));
  if (target < smin - 1e-12 || target > smax + 1e-12) console.error(`WARNING: target ${target} outside input range [${smin}, ${smax}] (extrapolation)`);
  const r = predict(inputs, target, beta, { breakSign: flags.includes('--break-sign'), breakDz: flag('break-dz', 0) });
  console.log(`# mhist beta ${beta} target s ${target}; inputs: ${inputs.map(p => `${p.s} (${p.chains.length} chains, ${p.n} samples)`).join(', ')}; discard ${frac} thin ${thin}`);
  console.log(`# WHAM f_k (f_0 = 0): ${Array.from(r.f).map(v => v.toFixed(6)).join(' ')}; Newton iterations ${r.newton}; jackknife groups ${r.groups}; ESS ${r.ess.toFixed(0)} of ${r.nTot} (fraction ${r.essFrac.toFixed(4)})`);
  const dir = flags.includes('--direct') ? all.filter(isT).map(direct) : [];
  console.log(`# observable value error ess${dir.length ? ' | direct value error z' : ''}`);
  for (const o of OBS) {
    let line = `${LABEL[o].padEnd(14)} ${r.pred[o][0].toExponential(6)} ${r.pred[o][1].toExponential(3)} ${r.ess.toFixed(0)}`;
    if (dir.length) { const [dv, de] = dir[0][o]; line += ` | ${dv.toExponential(6)} ${de.toExponential(3)} z ${((r.pred[o][0] - dv) / Math.hypot(r.pred[o][1], de)).toFixed(2)}`; }
    console.log(line);
  }
  console.error(`# runtime ${((Date.now() - t0) / 1000).toFixed(1)} s`);
}
