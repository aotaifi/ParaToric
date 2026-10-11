'use strict';
// Driver.  usage:
//   node run.js L beta mu J h lmbda nTherm nSamples nBetween seed [start=ord|rnd] [out=file] [rot=N] [bug=N] [quiet=1]
// nTherm and nBetween are numbers of local Metropolis steps; a suffix "s" means sweeps (1 sweep = nE steps),
// e.g. nBetween=2s.  One measurement per nBetween steps.  rot = steps between global time shifts (default: one
// per sweep; the C++ shifts after every step: rot=1).  bug=N activates a deliberate bug (validation-gate test).
const fs = require('fs');
const { Config, RNG } = require('./qmc.js');
const { mean, binnedError, tauInt } = require('./stats.js');

const OBS = ['plaquette', 'star', 'sigx', 'sigz', 'energy'];

function parseCount(s, nE) {
  if (/s$/.test(s)) return Math.round(parseFloat(s) * nE);
  return Math.round(parseFloat(s));
}

function runSim(p) { // p: {L,beta,mu,J,h,lmbda,nTherm,nSamples,nBetween,seed,start,rot,bug,out}
  const cfg = new Config(p.L, p.beta, { mu: p.mu, J: p.J, h: p.h, lmbda: p.lmbda },
    { rotateEvery: p.rot, bug: p.bug || 0, flatWindow: !!p.flat });
  const rng = new RNG(p.seed);
  if (p.es && p.es.mode === 'gsw') { // generalised SW (gsw.js); jFactor != 1 only for the broken-arm test
    const { gswStep } = require('./gsw.js');
    cfg.esStats = { calls: 0, opened: 0, closed: 0, wormSteps: 0 };
    cfg.pExtra = p.es.pExtra;
    cfg.extraUpdate = (c, r) => { gswStep(c, () => r.uniform(), { jFactor: p.es.jFactor || 1 }); c.esStats.calls++; return true; };
  } else if (p.es) { // ES-dressed worm (esworm.js): p.es = { mode: 'sw'|'worm', pExtra, brokenNf, bondFactor }
    const { esStep } = require('./esworm.js');
    const d = Math.min(0.5, 0.45 * p.beta);
    const par = { worm: p.es.mode !== 'sw', dOpen: d, dTime: d, dHop: d, pTime: 0.35, pHop: 0.25, pUnhop: 0.25, pClose: 0.1, pSwap: 0.05,
      alphaS: p.es.alphaS || 0, alphaT: p.es.alphaT || 0, bugHop: !!p.es.bugHop, brokenNf: !!p.es.brokenNf, bondFactor: p.es.bondFactor || 1, debug: !!p.es.debug };
    par.C = 1 / ((par.pClose / 1.0) * cfg.nP * p.beta * 2 * par.dOpen);
    cfg.esStats = { calls: 0, opened: 0, closed: 0, wormSteps: 0 };
    cfg.pExtra = p.es.pExtra;
    cfg.extraUpdate = (c, r) => { const s = esStep(c, () => r.uniform(), par); const S = c.esStats;
      S.calls++; S.opened += s.opened; S.closed += s.closed; S.wormSteps += s.wormSteps; return s.closed === 1; };
  }
  if (p.start === 'rnd') cfg.randomizeSpins(rng);
  for (let i = 0; i < p.nTherm; i++) cfg.localStep(rng);
  cfg.checkValid();
  const series = {}; for (const o of OBS) series[o] = new Float64Array(p.nSamples);
  let fd = null;
  if (p.out) { fd = fs.openSync(p.out, 'w'); fs.writeSync(fd, '# ' + OBS.join(' ') + '\n'); }
  const t0 = process.hrtime.bigint();
  let lines = [];
  for (let s = 0; s < p.nSamples; s++) {
    for (let i = 0; i < p.nBetween; i++) cfg.localStep(rng);
    const m = cfg.measure();
    for (const o of OBS) series[o][s] = m[o];
    if (fd !== null) {
      lines.push(OBS.map((o) => m[o].toPrecision(10)).join(' '));
      if (lines.length >= 1000) { fs.writeSync(fd, lines.join('\n') + '\n'); lines = []; }
    }
  }
  if (fd !== null) { if (lines.length) fs.writeSync(fd, lines.join('\n') + '\n'); fs.closeSync(fd); }
  const dt = Number(process.hrtime.bigint() - t0) / 1e9;
  cfg.checkValid();
  const res = {};
  for (const o of OBS) {
    const x = series[o];
    const be = binnedError(x, 32), ti = tauInt(x, 6);
    res[o] = { mean: mean(x), err: be.err, binSize: be.size, nbins: be.nbins, tau: ti.tau };
  }
  const steps = p.nSamples * p.nBetween;
  return { res, cfg, movesPerSec: steps / dt, seconds: dt, steps };
}

module.exports = { runSim, OBS, parseCount };

if (require.main === module) {
  const a = process.argv.slice(2);
  if (a.length < 10) {
    console.error('usage: node run.js L beta mu J h lmbda nTherm nSamples nBetween seed [start=ord|rnd] [out=file] [rot=N] [bug=N]');
    process.exit(1);
  }
  const L = parseInt(a[0], 10), nE = 2 * L * L;
  const kv = {};
  for (const s of a.slice(10)) { const i = s.indexOf('='); if (i > 0) kv[s.slice(0, i)] = s.slice(i + 1); }
  const p = {
    L, beta: parseFloat(a[1]), mu: parseFloat(a[2]), J: parseFloat(a[3]), h: parseFloat(a[4]), lmbda: parseFloat(a[5]),
    nTherm: parseCount(a[6], nE), nSamples: parseInt(a[7], 10), nBetween: parseCount(a[8], nE), seed: parseInt(a[9], 10),
    start: kv.start || 'ord', rot: kv.rot !== undefined ? parseInt(kv.rot, 10) : undefined,
    bug: kv.bug ? parseInt(kv.bug, 10) : 0, flat: kv.flat === '1', out: kv.out || ('series_L' + L + '_s' + a[9] + '.dat'),
  };
  const r = runSim(p);
  console.log('# L=%d beta=%s mu=%s J=%s h=%s lmbda=%s nTherm=%d nSamples=%d nBetween=%d seed=%d start=%s',
    p.L, p.beta, p.mu, p.J, p.h, p.lmbda, p.nTherm, p.nSamples, p.nBetween, p.seed, p.start);
  console.log('# observable  mean  err(binned, size %s)  tau_int(samples)', '');
  for (const o of OBS) {
    const q = r.res[o];
    console.log('%s  %s +- %s  (bin %d, %d bins)  tau_int=%s', o.padEnd(10), q.mean.toFixed(6), q.err.toExponential(2), q.binSize, q.nbins, q.tau.toFixed(2));
  }
  const names = ['dblSingle', 'moveKink', 'globSingle', 'globStar', 'dblStar', 'moveStar', 'combo'];
  console.log('# acceptance: ' + names.map((n, i) => n + '=' + (r.cfg.nAcc[i] / Math.max(1, r.cfg.nAtt[i])).toFixed(3)).join(' '));
  console.log('# moves/s = %s  (%d steps in %s s)  series -> %s', r.movesPerSec.toExponential(3), r.steps, r.seconds.toFixed(2), p.out);
}
