'use strict';
// Cross-check: generalised-SW prototype chains vs production pooled references at s*.
// usage: node xcheck.js production_reference.json chainFile1 [chainFile2 ...]   (tau.js outputs)
// Prints, per L and observable, every chain's mean +- error, the production PT-only pooled mean +- scatter error, and z.
const fs = require('fs');
const [refPath, ...chains] = process.argv.slice(2);
const ref = JSON.parse(fs.readFileSync(refPath, 'utf8')).pooled;
const map = { plaquette: 'plaquette', star: 'star', sigx: 'sigma_x', sigz: 'sigma_z', energy: 'E' };
const rows = [];
for (const f of chains) {
  const txt = fs.readFileSync(f, 'utf8'); const L = +(/# L=(\d+)/.exec(txt)[1]);
  for (const line of txt.split('\n')) {
    const m = /^(\w+)\s+mean (\S+) err (\S+)/.exec(line); if (!m || !map[m[1]]) continue;
    let mean = +m[2], err = +m[3]; if (m[1] === 'energy') { mean /= L * L; err /= L * L; }
    rows.push({ f: f.split('/').slice(-2).join('/'), L, obs: m[1], mean, err });
  }
}
const match = (r, obs) => r.obs === map[obs] || r.obs === obs || (obs === 'sigx' && r.obs === 'sigma_x_per_edge') || (obs === 'energy' && r.obs === 'E_per_vertex');
const pick = (L, obs) => ref.find(r => r.L === L && r.set === 'PTonly' && match(r, obs)) || ref.find(r => r.L === L && r.set === 'all_nonshort' && match(r, obs));
let zmax = 0, nz = 0; process.on('exit', () => console.log(`# ${nz} comparisons, max |z| = ${zmax.toFixed(2)}`));
for (const r of rows.sort((a, b) => a.L - b.L || a.obs.localeCompare(b.obs))) {
  const p = pick(r.L, r.obs);
  if (!p) { console.log(`${r.L} ${r.obs.padEnd(9)} ${r.mean.toFixed(6)}(${r.err.toExponential(1)})  no reference   ${r.f}`); continue; }
  const e = Math.max(p.scatter_sem || 0, 1e-12); const z = (r.mean - p.mean) / Math.sqrt(r.err ** 2 + e ** 2); nz++; zmax = Math.max(zmax, Math.abs(z));
  console.log(`${r.L} ${r.obs.padEnd(9)} gsw ${r.mean.toFixed(6)}(${r.err.toExponential(1)})  prod[${p.set},n=${p.n}] ${p.mean.toFixed(6)}(${e.toExponential(1)})  z ${z.toFixed(2)}   ${r.f}`);
}
