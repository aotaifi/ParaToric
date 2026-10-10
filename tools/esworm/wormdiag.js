'use strict';
// usage: node wormdiag.js L nES seed bondFactor [h]
// Worm-length distribution and close statistics at beta = L, mu=J=1, lmbda=0.3391 (bonds scaled by bondFactor;
// bondFactor=0 removes the electric constraint entirely, leaving the bare vison weight W_m). Worm capped for the
// diagnostic only (cap = 2e6 steps, reported).
const { Config, RNG } = require('./qmc.js'); const { esStep } = require('./esworm.js');
const [L, nES, seed, bf, hh] = process.argv.slice(2).map(Number);
const h = hh || 0.3391, beta = L;
const cfg = new Config(L, beta, { mu: 1, J: 1, h, lmbda: 0.3391 }), rng = new RNG(seed);
const d = 0.5;
const par = { worm: true, dOpen: d, dTime: d, dHop: d, pTime: .35, pHop: .25, pUnhop: .25, pClose: .1, pSwap: .05, bondFactor: bf === 0 ? 1e-12 : bf };
par.C = 1 / (par.pClose * cfg.nP * beta * 2 * d);
for (let i = 0; i < 300 * cfg.nE; i++) cfg.localStep(rng);
const lens = []; let tries = 0, inf = 0, opened = 0;
const t0 = Date.now();
for (let i = 0; i < nES; i++) {
  const s = esStep(cfg, () => rng.uniform(), par);
  if (s.opened) { opened++; lens.push(s.wormSteps); tries += s.closeTries || 0; inf += s.closeInfeasible || 0; }
  for (let k = 0; k < cfg.nE; k++) cfg.localStep(rng);
  if (Date.now() - t0 > 240000) break;
}
lens.sort((a, b) => a - b); const q = f => lens[Math.min(lens.length - 1, Math.floor(f * lens.length))];
console.log(`L=${L} h=${h} bondFactor=${bf}: worms ${opened}; length median ${q(0.5)} p75 ${q(0.75)} p90 ${q(0.9)} max ${lens[lens.length - 1]} mean ${(lens.reduce((a, b) => a + b, 0) / Math.max(1, opened)).toFixed(0)}; ` +
  `close attempts ${tries}, infeasible ${inf} (${(100 * inf / Math.max(1, tries)).toFixed(1)}%)`);
