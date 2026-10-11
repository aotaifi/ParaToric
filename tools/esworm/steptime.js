'use strict';
// usage: node steptime.js L nSteps seed -- wall time per global gsw step and core size (memory: read VmHWM afterwards)
const { Config, RNG } = require('./qmc.js'); const { gswStep } = require('./gsw.js');
const [L, n, seed] = process.argv.slice(2).map(Number);
const c = new Config(L, L, { mu: 1, J: 1, h: 0.3391, lmbda: 0.3391 }), r = new RNG(seed);
for (let i = 0; i < n; i++) { const t = Date.now(); const o = gswStep(c, () => r.uniform(), {});
  console.log(`L=${L} step ${i} ${((Date.now() - t) / 1000).toFixed(2)} s nVar ${o.nVar} rows ${o.nRows} core vars ${o.coreVars}`); }
const st = require('fs').readFileSync('/proc/self/status', 'utf8').split('\n').find(l => l.startsWith('VmHWM'));
console.log(st);
