'use strict';
// Reweight production series along the self-dual line h = lmbda = s from s0 to s1 and compare cumulants.
// Per-sample estimators: energy_h = -n_h/beta (n_h = number of h-kinks), energy_lmbda = -lmbda * int sum sigma^z dtau / beta,
// so the exact log weight ratio of a configuration is
//   log W(s1)/W(s0) = n_h ln(s1/s0) + (s1 - s0) int sum sigma^z = -beta E_h ln(s1/s0) - beta E_lmbda (s1 - s0)/s0.
// Reads runs/c*/ckpt_series.txt (interim.js output; columns 0 energy 1 plaq 2 star 3 delta 4 energy_h 5 energy_lmbda ...).
// Per chain: reweighted k3(S), Binder k4/k2^2(delta), effective sample fraction; combined with chain-scatter errors.
// usage: node reweight.js runsDir beta s0 s1 [discard=0.1]
const fs = require('fs'), path = require('path');
const [dir, betaS, s0S, s1S, fracS] = process.argv.slice(2); const beta = +betaS, s0 = +s0S, s1 = +s1S, frac = fracS ? +fracS : 0.1;
const out = [];
for (const c of fs.readdirSync(dir).filter(d => /^c\d+$/.test(d))) {
  const f = path.join(dir, c, 'ckpt_series.txt'); if (!fs.existsSync(f)) continue;
  const X = fs.readFileSync(f, 'utf8').trim().split('\n').map(l => l.split(' ').map(Number)); const x = X.slice(Math.floor(frac * X.length));
  if (x.length < 100) continue;
  const lw = x.map(r => -beta * r[4] * Math.log(s1 / s0) - beta * r[5] * (s1 - s0) / s0);
  const mx = Math.max(...lw); const w = lw.map(v => Math.exp(v - mx)); const W = w.reduce((a, b) => a + b, 0);
  const W2 = w.reduce((a, b) => a + b * b, 0);
  const avg = g => x.reduce((a, r, i) => a + w[i] * g(r), 0) / W;
  const mS = avg(r => r[1] + r[2]), mD = avg(r => r[3]);
  const k2S = avg(r => (r[1] + r[2] - mS) ** 2), k3S = avg(r => (r[1] + r[2] - mS) ** 3);
  const k2D = avg(r => (r[3] - mD) ** 2), k4D = avg(r => (r[3] - mD) ** 4);
  out.push({ k3S, bD: (k4D - 3 * k2D * k2D) / (k2D * k2D), ess: W * W / W2 / x.length });
}
const st = k => { const v = out.map(o => o[k]), n = v.length, m = v.reduce((a, b) => a + b, 0) / n; return [m, Math.sqrt(v.reduce((a, b) => a + (b - m) ** 2, 0) / (n - 1) / n)]; };
const [k3, e3] = st('k3S'), [bd, eb] = st('bD'), [ess] = st('ess');
console.log(`${dir.split('/').slice(-2, -1)[0]} s ${s0} -> ${s1}: chains ${out.length}, mean ESS fraction ${ess.toFixed(3)}; k3(S) ${k3.toExponential(4)} +- ${e3.toExponential(2)} (${(100 * e3 / Math.abs(k3)).toFixed(1)}%); Binder(delta) ${bd.toFixed(4)} +- ${eb.toFixed(4)}`);
