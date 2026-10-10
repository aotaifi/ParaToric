'use strict';
// usage: node fit.js OUT1.txt [OUT2.txt ...]   (output files of es.js)
// Weighted least squares of log P_consistent = a - sA*R*T - sP*2(R+T), weights N P/(1-P) (inverse binomial
// variance of log P). Reports the joint fit and the area-only and perimeter-only fits with chi2/dof.
const fs = require('fs');
const N = 2000;
function solve(X, y, w, cols) {
  const k = cols.length, A = Array.from({ length: k }, () => Array(k).fill(0)), b = Array(k).fill(0);
  X.forEach((x, i) => { for (let p = 0; p < k; p++) { b[p] += w[i] * x[cols[p]] * y[i]; for (let q = 0; q < k; q++) A[p][q] += w[i] * x[cols[p]] * x[cols[q]]; } });
  const inv = A.map((r, i) => [...r, ...A.map((_, j) => (i === j ? 1 : 0))]);
  for (let i = 0; i < k; i++) {
    let piv = i; for (let r = i + 1; r < k; r++) if (Math.abs(inv[r][i]) > Math.abs(inv[piv][i])) piv = r;
    [inv[i], inv[piv]] = [inv[piv], inv[i]]; const d = inv[i][i]; for (let c = 0; c < 2 * k; c++) inv[i][c] /= d;
    for (let r = 0; r < k; r++) if (r !== i) { const f = inv[r][i]; for (let c = 0; c < 2 * k; c++) inv[r][c] -= f * inv[i][c]; }
  }
  const beta = cols.map((_, p) => cols.reduce((s, _c, q) => s + inv[p][k + q] * b[q], 0));
  let chi2 = 0; X.forEach((x, i) => { let pr = 0; cols.forEach((c, p) => (pr += beta[p] * x[c])); chi2 += w[i] * (y[i] - pr) ** 2; });
  return { beta, err: cols.map((_, p) => Math.sqrt(inv[p][k + p])), chi2, dof: X.length - k };
}
for (const f of process.argv.slice(2)) {
  const rows = fs.readFileSync(f, 'utf8').trim().split('\n').filter(l => /^\d/.test(l)).map(l => l.split(/\s+/).map(Number)).filter(r => r[2] > 0 && r[2] < 1);
  const X = [], y = [], w = [];
  for (const [R, T, P] of rows) { X.push([1, -R * T, -2 * (R + T)]); y.push(Math.log(P)); w.push(N * P / (1 - P)); }
  const both = solve(X, y, w, [0, 1, 2]), area = solve(X, y, w, [0, 1]), per = solve(X, y, w, [0, 2]);
  const g = v => v.toFixed(4);
  console.log(`${f}: joint sA=${g(both.beta[1])}(${g(both.err[1])}) sP=${g(both.beta[2])}(${g(both.err[2])}) chi2/dof=${(both.chi2 / both.dof).toFixed(1)} | ` +
    `area-only sA=${g(area.beta[1])} chi2/dof=${(area.chi2 / area.dof).toFixed(1)} | perimeter-only sP=${g(per.beta[1])} chi2/dof=${(per.chi2 / per.dof).toFixed(1)}`);
}
