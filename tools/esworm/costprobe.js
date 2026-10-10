'use strict';
// usage: node costprobe.js L nES seed [dWin]
// Cost of one ES step vs one local step at s* (beta = L), and worm statistics.
const { Config, RNG } = require('./qmc.js'); const { esStep } = require('./esworm.js');
const [L, nES, seed, dW] = process.argv.slice(2).map(Number);
const beta = L, cfg = new Config(L, beta, { mu: 1, J: 1, h: 0.3391, lmbda: 0.3391 }), rng = new RNG(seed);
const d = dW || 0.5;
const par = { worm: true, dOpen: d, dTime: d, dHop: d, pTime: .35, pHop: .25, pUnhop: .25, pClose: .1, pSwap: .05 };
par.C = 1 / (par.pClose * cfg.nP * beta * 2 * d);
for (let i = 0; i < 300 * cfg.nE; i++) cfg.localStep(rng);
let t0 = process.hrtime.bigint(); const nLoc = 50 * cfg.nE;
for (let i = 0; i < nLoc; i++) cfg.localStep(rng);
const tLoc = Number(process.hrtime.bigint() - t0) / 1e9 / nLoc;
let opened = 0, closed = 0, steps = 0, kinkChange = 0; const lens = [];
t0 = process.hrtime.bigint();
for (let i = 0; i < nES; i++) {
  const nk0 = cfg.kinks.reduce((s, a) => s + a.length, 0);
  const s = esStep(cfg, () => rng.uniform(), par);
  opened += s.opened; closed += s.closed; steps += s.wormSteps; if (s.opened) lens.push(s.wormSteps);
  kinkChange += Math.abs(cfg.kinks.reduce((s2, a) => s2 + a.length, 0) - nk0);
  for (let k = 0; k < cfg.nE; k++) cfg.localStep(rng);
}
const tTot = Number(process.hrtime.bigint() - t0) / 1e9;
const tES = (tTot - nES * cfg.nE * tLoc) / nES;
lens.sort((a, b) => a - b);
console.log(`L=${L} local step ${(tLoc * 1e6).toFixed(2)} us; ES step ${(tES * 1e3).toFixed(2)} ms = ${(tES / tLoc).toFixed(0)} local steps = ${(tES / (tLoc * cfg.nE)).toFixed(2)} sweeps; ` +
  `opened ${opened}/${nES} closed ${closed}; worm steps mean ${(steps / Math.max(1, opened)).toFixed(0)} median ${lens[lens.length >> 1]} p90 ${lens[Math.floor(lens.length * 0.9)]}; mean |dN_kink| per ES ${(kinkChange / nES).toFixed(2)}`);
