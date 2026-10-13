'use strict';
// Permutation test of the start split (+1 = even task id, -1 = odd): is max |z| over observables larger than for random
// halvings of the same chains? Accounts for correlations between observables. Reads runs/c*/ckpt_series.txt (as written
// by interim.js). usage: node splittest.js discardFraction runsDir [nPerm=2000]
const fs = require('fs'), path = require('path');
const [fracS, dir, nPermS] = process.argv.slice(2); const frac = +fracS, nPerm = +(nPermS || 2000);
const OBS = ['mS', 'k3S', 'k2D', 'bD', 'sp', 'wx'];
const rows = [];
for (const c of fs.readdirSync(dir).filter(d => /^c\d+$/.test(d))) {
  const f = path.join(dir, c, 'ckpt_series.txt'); if (!fs.existsSync(f)) continue;
  const X = fs.readFileSync(f, 'utf8').trim().split('\n').map(l => l.split(' ').map(Number)); const x = X.slice(Math.floor(frac * X.length));
  if (x.length < 50) continue;
  const S = x.map(r => r[1] + r[2]), D = x.map(r => r[3]), n = x.length;
  const m = a => a.reduce((s, v) => s + v, 0) / a.length; const mS = m(S), mD = m(D);
  let k2S = 0, k3S = 0, k2D = 0, k4D = 0; for (let i = 0; i < n; i++) { const a = S[i] - mS, b = D[i] - mD; k2S += a * a; k3S += a * a * a; k2D += b * b; k4D += b ** 4; }
  k2S /= n; k3S /= n; k2D /= n; k4D /= n;
  rows.push({ even: (+c.slice(1)) % 2 === 0, v: [mS, k3S, k2D, (k4D - 3 * k2D * k2D) / (k2D * k2D), m(x.map(r => r[8])), m(x.map(r => r[9]))] });
}
const zmax = lab => { let best = 0; const zs = [];
  for (let j = 0; j < OBS.length; j++) { const st = sel => { const v = rows.filter((_, i) => lab[i] === sel).map(r => r.v[j]); const mm = v.reduce((a, b) => a + b, 0) / v.length; return [mm, v.reduce((a, b) => a + (b - mm) ** 2, 0) / (v.length - 1) / v.length]; };
    const [a, va] = st(true), [b, vb] = st(false); const z = (a - b) / Math.sqrt(va + vb); zs.push(z); best = Math.max(best, Math.abs(z)); }
  return { best, zs }; };
const obs = zmax(rows.map(r => r.even));
let ge = 0; const lab = rows.map(r => r.even);
for (let p = 0; p < nPerm; p++) { for (let i = lab.length - 1; i > 0; i--) { const j = Math.floor(Math.random() * (i + 1)); [lab[i], lab[j]] = [lab[j], lab[i]]; } if (zmax(lab).best >= obs.best) ge++; }
console.log(`${rows.length} chains; split z: ${OBS.map((o, i) => `${o} ${obs.zs[i].toFixed(2)}`).join(', ')}; max|z| ${obs.best.toFixed(2)}; permutation p = ${(ge / nPerm).toFixed(3)} (${nPerm} random halvings)`);
