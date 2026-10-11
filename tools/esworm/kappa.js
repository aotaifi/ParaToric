'use strict';
// Cumulants of S = plaquette + star from tau.js series files (columns: plaquette star sigx sigz energy).
// usage: node kappa.js discardFraction file1 [file2 ...]
// Per chain: k2, k3, k4 of the per-sample S, errors from 16 equal blocks (block-to-block scatter / sqrt(16)) after
// discarding the first fraction. Combined: mean of the chain values with the scatter-based standard error.
const fs = require('fs');
const [fracS, ...files] = process.argv.slice(2); const frac = +fracS;
function cum(x) {
  const n = x.length; let m = 0; for (const v of x) m += v; m /= n;
  let c2 = 0, c3 = 0, c4 = 0; for (const v of x) { const d = v - m; const d2 = d * d; c2 += d2; c3 += d2 * d; c4 += d2 * d2; }
  c2 /= n; c3 /= n; c4 /= n; return { mean: m, k2: c2, k3: c3, k4: c4 - 3 * c2 * c2 };
}
const res = [];
for (const f of files) {
  const S = [];
  for (const line of fs.readFileSync(f, 'utf8').split('\n')) { if (!line || line[0] === '#') continue; const a = line.split(' '); S.push(+a[0] + +a[1]); }
  const x = S.slice(Math.floor(frac * S.length)); const nb = 16, bl = Math.floor(x.length / nb);
  const all = cum(x), blocks = []; for (let b = 0; b < nb; b++) blocks.push(cum(x.slice(b * bl, (b + 1) * bl)));
  const err = k => { const v = blocks.map(o => o[k]); const m = v.reduce((a, c) => a + c, 0) / nb; return Math.sqrt(v.reduce((a, c) => a + (c - m) ** 2, 0) / (nb - 1) / nb); };
  const r = { f: f.split('/').pop(), n: x.length, mean: all.mean, k2: all.k2, k2e: err('k2'), k3: all.k3, k3e: err('k3'), k4: all.k4, k4e: err('k4') };
  res.push(r);
  console.log(`${r.f.padEnd(28)} n ${r.n}  mean ${r.mean.toFixed(6)}  k2 ${r.k2.toExponential(4)}(${r.k2e.toExponential(1)})  k3 ${r.k3.toExponential(4)}(${r.k3e.toExponential(1)})  k4 ${r.k4.toExponential(3)}(${r.k4e.toExponential(1)})`);
}
if (res.length > 1) for (const k of ['mean', 'k2', 'k3', 'k4']) {
  const v = res.map(r => r[k]), m = v.reduce((a, c) => a + c, 0) / v.length;
  const se = Math.sqrt(v.reduce((a, c) => a + (c - m) ** 2, 0) / (v.length - 1) / v.length);
  const q = res.map(r => (r[k] - m) ** 2 / (r[k + 'e'] ** 2 || 1)).reduce((a, c) => a + c, 0);
  console.log(`combined ${k.padEnd(4)} ${m.toExponential(5)} +- ${se.toExponential(2)} (scatter, ${v.length} chains)` + (k === 'mean' ? '' : `  chi2/dof vs block errors ${(q / (v.length - 1)).toFixed(2)}`));
}
