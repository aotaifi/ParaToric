'use strict';
// Validation gate: QMC at L=2 vs exact diagonalisation (tools/esworm/ed/reference.json), points P1..P8.
// The points run as parallel local node processes.
// usage: node gate.js [nSamples=4000000] [nBetween=64] [bug=0] [seed=1] [points=P1,P2,...] [outdir=/scratch/a/A.Otaifi/esworm_gate]
//   nBetween = local steps between samples (L=2 has 8 edges, so 64 steps = 8 sweeps)
const fs = require('fs'), path = require('path'), cp = require('child_process');
const { runSim } = require('./run.js');
const kv = {};
for (const s of process.argv.slice(2)) { const i = s.indexOf('='); if (i > 0) kv[s.slice(0, i)] = s.slice(i + 1); }
const nSamples = kv.nSamples ? parseInt(kv.nSamples, 10) : 4000000;
const nBetween = kv.nBetween ? parseInt(kv.nBetween, 10) : 64;
const bug = kv.bug ? parseInt(kv.bug, 10) : 0;
const seed0 = kv.seed ? parseInt(kv.seed, 10) : 1;
const outdir = kv.outdir || '/scratch/a/A.Otaifi/esworm_gate';
const ref = JSON.parse(fs.readFileSync(path.join(__dirname, 'ed', 'reference.json'), 'utf8'));
const allPts = Object.keys(ref);
// P8 has h=0: no kinks, every vertex has the same star-event parity and the updates (the combination move needs
// h>0) cannot change it, so the chain samples only the all-even sector.  Compare against that sector
// (ref_p8_even.json, from dev/p8sector.js); fullP8=1 compares with the full-trace ED value instead.
if (!kv.fullP8 && !kv.es) Object.assign(ref.P8, JSON.parse(fs.readFileSync(path.join(__dirname, 'ref_p8_even.json'), 'utf8')));
const map = [['plaquette', 'B'], ['star', 'A'], ['sigx', 'sigma_x'], ['sigz', 'sigma_z'], ['energy', 'H']];

if (kv.worker) { // child: one point, JSON to stdout
  const P = kv.worker, r = ref[P], ip = allPts.indexOf(P);
  const out = runSim({ L: 2, beta: r.beta, mu: r.mu, J: r.J, h: r.h, lmbda: r.lmbda, nTherm: 20000, nSamples, nBetween,
    seed: seed0 * 1000 + ip + 1, start: ip % 2 ? 'rnd' : 'ord', bug,
    es: kv.es ? { mode: kv.es, pExtra: parseFloat(kv.pExtra || '0.1'), jFactor: kv.jFactor ? parseFloat(kv.jFactor) : 1, alphaS: kv.alphaS ? parseFloat(kv.alphaS) : 0, alphaT: kv.alphaT ? parseFloat(kv.alphaT) : 0, bugHop: kv.bugHop === '1', brokenNf: kv.brokenNf === '1', bondFactor: kv.bondFactor ? parseFloat(kv.bondFactor) : 1, debug: kv.debug === '1' } : null, out: kv.series ? path.join(outdir, 'gate_' + P + '_bug' + bug + '.dat') : null });
  console.log(JSON.stringify({ P, res: out.res, steps: out.steps, seconds: out.seconds, es: out.cfg.esStats || null }));
} else {
  const pts = kv.points ? kv.points.split(',') : allPts;
  let pending = 0;
  const results = {};
  const finish = () => {
    console.log('# L=2 gate: %d samples per point, %d local steps between samples, bug=%d, seed=%d, es=%s pExtra=%s brokenNf=%s bondFactor=%s', nSamples, nBetween, bug, seed0, kv.es || '-', kv.pExtra || '-', kv.brokenNf || '0', kv.bondFactor || '1');
    for (const P of pts) if (results[P].es) { const e = results[P].es; console.log('# %s ES calls %d opened %d closed %d mean worm steps %s', P, e.calls, e.opened, e.closed, (e.wormSteps / Math.max(1, e.opened)).toFixed(1)); }
    console.log('point  observable   QMC +- err            ED            diff/err   tau_int(samples)');
    const zs = []; let nbad = 0, steps = 0, secs = 0;
    for (const P of pts) {
      const R = results[P]; steps += R.steps; secs += R.seconds;
      for (const [o, k] of map) {
        const q = R.res[o], ed = ref[P][k];
        const z = (q.mean - ed) / q.err;
        let zstr;
        if (isFinite(z)) { zs.push(z); if (Math.abs(z) >= 3) nbad++; zstr = z.toFixed(2); }
        else if (Math.abs(q.mean - ed) < 1e-9) zstr = 'both 0';
        else { nbad++; zstr = 'inf'; }
        console.log('%s     %s %s +- %s  %s  %s   %s', P, o.padEnd(10), q.mean.toFixed(5).padStart(10), q.err.toExponential(1), ed.toFixed(5).padStart(10), zstr.padStart(8), (q.tau !== null && isFinite(q.tau)) ? q.tau.toFixed(1) : '-');
      }
    }
    const n = zs.length;
    console.log('# %d comparisons with finite error: mean z = %s, rms z = %s, max |z| = %s, n(|z|>=3) = %d',
      n, (zs.reduce((a, b) => a + b, 0) / n).toFixed(2), Math.sqrt(zs.reduce((a, b) => a + b * b, 0) / n).toFixed(2), Math.max(...zs.map(Math.abs)).toFixed(2), nbad);
    console.log('# %s steps/s per process', (steps / secs).toExponential(2));
    console.log(nbad === 0 ? 'GATE: PASS' : 'GATE: FAIL');
  };
  for (const P of pts) {
    pending++;
    const args = [__filename, 'worker=' + P, 'nSamples=' + nSamples, 'nBetween=' + nBetween, 'bug=' + bug, 'seed=' + seed0, 'outdir=' + outdir];
    if (kv.series) args.push('series=1');
    if (kv.fullP8) args.push('fullP8=1');
    for (const k of ['es', 'pExtra', 'jFactor', 'alphaS', 'alphaT', 'bugHop', 'brokenNf', 'bondFactor', 'debug']) if (kv[k]) args.push(k + '=' + kv[k]);
    const ch = cp.spawn(process.execPath, args, { stdio: ['ignore', 'pipe', 'inherit'] });
    let buf = '';
    ch.stdout.on('data', (d) => { buf += d; });
    ch.on('close', () => { results[P] = JSON.parse(buf.trim().split('\n').pop()); if (--pending === 0) finish(); });
  }
}
