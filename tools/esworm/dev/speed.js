// local-step throughput: node dev/speed.js L [rotateEvery] [nSteps]
const { Config, RNG } = require('../qmc.js');
const L = parseInt(process.argv[2], 10);
const nE = 2 * L * L;
const rot = process.argv[3] !== undefined ? parseInt(process.argv[3], 10) : nE;
const c = new Config(L, L, { mu: 1, J: 1, h: 0.3391, lmbda: 0.3391 }, { rotateEvery: rot });
const r = new RNG(11);
const warm = 400 * nE;
for (let i = 0; i < warm; i++) c.localStep(r);
const n = process.argv[4] ? parseInt(process.argv[4], 10) : 4e6;
const t0 = process.hrtime.bigint();
for (let i = 0; i < n; i++) c.localStep(r);
const dt = Number(process.hrtime.bigint() - t0) / 1e9;
c.checkValid();
console.log('L=%d beta=%d rotateEvery=%d: %s moves/s  (events: %d kinks, %d stars)', L, L, rot, (n / dt).toExponential(3), c.nKinkTotal(), c.nStarTotal());
