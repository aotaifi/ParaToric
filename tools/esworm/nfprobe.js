'use strict';
// usage: node nfprobe.js L beta h lmbda nSteps seed
// Histogram of N_f (number of feasible global sectors) at the start of ES steps, and of N_f changes at accepted closes.
const { Config, RNG } = require('./qmc.js'); const { esStep } = require('./esworm.js');
const [L, beta, h, lm, n, seed] = process.argv.slice(2).map(Number);
const cfg = new Config(L, beta, { mu: 1, J: 1, h, lmbda: lm }), rng = new RNG(seed);
const d = Math.min(0.5, 0.45 * beta);
const par = { worm: true, dOpen: d, dTime: d, dHop: d, pTime: .35, pHop: .25, pUnhop: .25, pClose: .1, pSwap: .05 };
par.C = 1 / (par.pClose * cfg.nP * beta * 2 * d);
for (let i = 0; i < 20000; i++) cfg.localStep(rng);
const h0 = {}, dh = {}; let closes = 0;
for (let i = 0; i < n; i++) {
  for (let k = 0; k < 4; k++) cfg.localStep(rng);
  const s = esStep(cfg, () => rng.uniform(), par);
  h0[s.Nf0] = (h0[s.Nf0] || 0) + 1;
  if (s.closed) { closes++; const key = s.Nf0 + '->' + s.Nf1; dh[key] = (dh[key] || 0) + 1; }
}
console.log('L', L, 'Nf(start) histogram', JSON.stringify(h0), 'closes', closes, 'transitions', JSON.stringify(dh));
