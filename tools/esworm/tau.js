'use strict';
// Autocorrelation per CPU-second: local updates only vs local + ES step (esworm.js, worm v1), s* (h = lmbda = 0.3391,
// mu = J = 1), beta = L.
// usage: node tau.js L arm seed thermSweeps nSamples sweepsBetween [esPerSample=1] [out=file] [start=ord|rnd]
//   arm = local | es | sw (sw: electric SW step only, no worm)
// One sample = sweepsBetween sweeps of local moves (1 sweep = nE steps) followed, in arms es/sw, by esPerSample ES steps.
// Prints per observable: mean, binned error, tau_int (samples) and tau_int in CPU-seconds (single thread).
const fs = require('fs');
const { Config, RNG } = require('./qmc.js'); const { esStep } = require('./esworm.js');
const { mean, binnedError, tauInt } = require('./stats.js');
const a = process.argv.slice(2);
const L = +a[0], arm = a[1], seed = +a[2], therm = +a[3], nS = +a[4], sb = +a[5];
const kv = {}; for (const s of a.slice(6)) { const i = s.indexOf('='); if (i > 0) kv[s.slice(0, i)] = s.slice(i + 1); }
const esPer = kv.esPerSample ? +kv.esPerSample : 1;
const sCoupling = kv.s ? +kv.s : 0.3391; // h = lmbda = s on the self-dual line
const beta = L, cfg = new Config(L, beta, { mu: 1, J: 1, h: sCoupling, lmbda: sCoupling }), rng = new RNG(seed);
if (kv.start === 'rnd') cfg.randomizeSpins(rng);
const d = 0.5;
const par = { worm: arm === 'es', alphaS: kv.alphaS ? +kv.alphaS : 0, alphaT: kv.alphaT ? +kv.alphaT : 0, dOpen: d, dTime: d, dHop: d, pTime: .35, pHop: .25, pUnhop: .25, pClose: .1, pSwap: .05 };
par.C = 1 / (par.pClose * cfg.nP * beta * 2 * d);
const { gswStep } = require('./gsw.js');
const ell = kv.ell ? +kv.ell : 0; // block move of ell x ell vertices (0 = whole lattice)
const esw = kv.esw === '1'; // also one electric SW step (O(N); flips global twist / edge-loop sectors) per sample
const parSW = { worm: false };
const es = arm === 'gsw' ? () => { const o = ell > 0 && ell < L ? { block: { x0: Math.floor(rng.uniform() * L), y0: Math.floor(rng.uniform() * L), ell } } : {};
                             gswStep(cfg, () => rng.uniform(), o); if (esw) esStep(cfg, () => rng.uniform(), parSW);
                             return { opened: 0, closed: 0, wormSteps: 0 }; }
                         : () => esStep(cfg, () => rng.uniform(), par);
const OBS = ['plaquette', 'star', 'sigx', 'sigz', 'energy'];
const nE = cfg.nE;
for (let i = 0; i < therm; i++) { for (let k = 0; k < nE; k++) cfg.localStep(rng); if (arm !== 'local' && (sb === 0 || i % sb === 0)) for (let j = 0; j < esPer; j++) es(); }
// node-speed calibration: seconds per local sweep on this node (lets CPU times from different node types be compared)
let tSweep; { const nCal = Math.max(20, Math.ceil(2e5 / nE)); const tc = process.hrtime.bigint();
  for (let i = 0; i < nCal * nE; i++) cfg.localStep(rng); tSweep = Number(process.hrtime.bigint() - tc) / 1e9 / nCal; }
const ser = {}; for (const o of OBS) ser[o] = new Float64Array(nS);
let fd = kv.out ? fs.openSync(kv.out, 'w') : null; if (fd !== null) fs.writeSync(fd, '# ' + OBS.join(' ') + '\n');
let buf = []; let esOpened = 0, esClosed = 0, wormSteps = 0;
const t0 = process.hrtime.bigint();
const maxSec = kv.maxSec ? +kv.maxSec : Infinity; let nDone = 0;
for (let s = 0; s < nS; s++) {
  if (Number(process.hrtime.bigint() - t0) / 1e9 > maxSec) break;
  for (let i = 0; i < sb * nE; i++) cfg.localStep(rng);
  if (arm !== 'local') for (let j = 0; j < esPer; j++) { const st = es(); esOpened += st.opened; esClosed += st.closed; wormSteps += st.wormSteps; }
  const m = cfg.measure(); for (const o of OBS) ser[o][s] = m[o]; nDone = s + 1;
  if (fd !== null) { buf.push(OBS.map(o => m[o].toPrecision(9)).join(' ')); if (buf.length >= 100) { fs.writeSync(fd, buf.join('\n') + '\n'); buf = []; } }
}
const secs = Number(process.hrtime.bigint() - t0) / 1e9;
if (fd !== null) { if (buf.length) fs.writeSync(fd, buf.join('\n') + '\n'); fs.closeSync(fd); }
cfg.checkValid();
for (const o of OBS) ser[o] = ser[o].subarray(0, nDone);
const secPerSample = secs / nDone;
console.log(`# L=${L} arm=${arm} seed=${seed} therm=${therm} sweeps, ${nDone}/${nS} samples x ${sb} sweeps (+${arm === 'local' ? 0 : esPer} ES), ${secs.toFixed(1)} s, ${secPerSample.toExponential(3)} s/sample; ES opened ${esOpened} closed ${esClosed} mean worm steps ${(wormSteps / Math.max(1, esOpened)).toFixed(0)}; calibration ${tSweep.toExponential(3)} s per local sweep`);
for (const o of OBS) {
  const x = ser[o], be = binnedError(x, 32), ti = tauInt(x, 6);
  console.log(`${o.padEnd(10)} mean ${mean(x).toFixed(6)} err ${be.err.toExponential(2)} tau_samples ${ti.tau === null ? 'NA' : ti.tau.toFixed(1)} tau_cpu_s ${ti.tau === null ? 'NA' : (ti.tau * secPerSample).toExponential(3)} tau_sweepequiv ${ti.tau === null ? 'NA' : (ti.tau * secPerSample / tSweep).toExponential(3)}`);
}
