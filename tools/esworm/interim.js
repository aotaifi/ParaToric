'use strict';
// Interim cumulants from running gSW production chains, read from their checkpoints (ckptseries.js format).
// usage: node interim.js discardFraction runsDir   (runsDir/c*/ckpt.bin; production --observables order:
//   0 energy 1 plaquette_z_tau_avg 2 star_x 3 delta_tau_avg 4 energy_h 5 energy_lmbda 6 sigma_x 7 sigma_z
//   8 star_parity 9 wilson_x 10 wilson_y 11 winding_x 12 winding_y)
// Per chain: n, <S>, k2(S), k3(S), k2(delta), <star_parity>, <wilson_x>, <winding_x>. Combined: mean of chain values,
// error from the scatter of chains (chains with fewer than minSamples after discard are skipped).
const fs = require('fs'), path = require('path'), { execFileSync } = require('child_process');
const [fracS, dir, minS] = process.argv.slice(2); const frac = +fracS, minSamples = +(minS || 200);
const reader = path.join(__dirname, 'ckptseries.js');
const rows = [];
for (const c of fs.readdirSync(dir).filter(d => /^c\d+$/.test(d)).sort((a, b) => +a.slice(1) - +b.slice(1))) {
  const ck = path.join(dir, c, 'ckpt.bin'); if (!fs.existsSync(ck)) continue;
  const tmp = path.join(dir, c, 'ckpt_series.txt');
  try { execFileSync(process.execPath, [reader, '13', ck, tmp], { stdio: ['ignore', 'ignore', 'ignore'] }); } catch (e) { console.log(`${c}: unreadable`); continue; }
  const X = fs.readFileSync(tmp, 'utf8').trim().split('\n').map(l => l.split(' ').map(Number));
  const x = X.slice(Math.floor(frac * X.length)); if (x.length < minSamples) continue;
  const S = x.map(r => r[1] + r[2]), D = x.map(r => r[3]);
  const mom = a => { const n = a.length, m = a.reduce((s, v) => s + v, 0) / n; let c2 = 0, c3 = 0; for (const v of a) { const d = v - m; c2 += d * d; c3 += d * d * d; } return [m, c2 / n, c3 / n]; };
  const [mS, k2S, k3S] = mom(S), [, k2D] = mom(D);
  const mD = D.reduce((s, v) => s + v, 0) / D.length; let c4 = 0; for (const v of D) c4 += (v - mD) ** 4; const bD = (c4 / D.length - 3 * k2D * k2D) / (k2D * k2D);
  const avg = j => x.reduce((s, r) => s + r[j], 0) / x.length;
  rows.push({ c, n: x.length, total: X.length, mS, k2S, k3S, k2D, bD, sp: avg(8), wx: avg(9), wnx: avg(11) });
}
for (const r of rows) console.log(`${r.c.padEnd(4)} n ${String(r.n).padStart(6)}/${r.total}  <S> ${r.mS.toFixed(6)}  k2S ${r.k2S.toExponential(3)}  k3S ${r.k3S.toExponential(3)}  k2d ${r.k2D.toExponential(3)}  star_par ${r.sp.toFixed(3)} wilson_x ${r.wx.toFixed(3)} winding_x ${r.wnx.toFixed(3)}`);
if (rows.length > 1) {
  const n = rows.length;
  for (const k of ['mS', 'k2S', 'k3S', 'k2D', 'bD', 'sp', 'wx', 'wnx']) {
    const v = rows.map(r => r[k]), m = v.reduce((a, b) => a + b, 0) / n, se = Math.sqrt(v.reduce((a, b) => a + (b - m) ** 2, 0) / (n - 1) / n);
    console.log(`combined ${k.padEnd(4)} ${m.toExponential(5)} +- ${se.toExponential(2)}` + ((k === 'k3S' || k === 'bD') ? `  (rel ${(se / Math.abs(m) * 100).toFixed(1)}%)` : ''));
  }
  // even (start +1) vs odd (start -1) chains
  for (const k of ['mS', 'k3S', 'k2D', 'bD', 'sp', 'wx']) {
    const g = p => rows.filter(r => (+r.c.slice(1)) % 2 === p).map(r => r[k]);
    const st = v => { const m = v.reduce((a, b) => a + b, 0) / v.length; return [m, Math.sqrt(v.reduce((a, b) => a + (b - m) ** 2, 0) / (v.length - 1) / v.length)]; };
    const [a, ea] = st(g(0)), [b, eb] = st(g(1));
    console.log(`start +1 vs -1 ${k}: ${a.toExponential(4)}(${ea.toExponential(1)}) vs ${b.toExponential(4)}(${eb.toExponential(1)})  z ${((a - b) / Math.hypot(ea, eb)).toFixed(2)}`);
  }
  console.log(`samples total ${rows.reduce((a, r) => a + r.total, 0)} over ${n} chains`);
}
