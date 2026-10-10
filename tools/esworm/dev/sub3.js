// compare subsets of moves against ED for given couplings: node dev/sub3.js mu J h lm beta moves(comma list of 0..6) nsamp
const { Config, RNG } = require('../qmc.js');
const ED = require('../ed/ed2x2.js');
const { binnedError, mean, tauInt } = require('../stats.js');
const a = process.argv.slice(2).map(Number);
const [mu, J, h, lm, beta] = a;
const moves = process.argv[7].split(',').map(Number);
const nsamp = parseInt(process.argv[8], 10);
const names = ['_doubleSingleSpinFlip', '_singleSpinFlipMove', '_globalSingleSpinFlip', '_globalTupleFlip', '_doubleTupleFlip', '_singleTupleFlipMove', '_spinTupleCombination'];
const ed = ED.thermal(mu, J, h, lm, beta);
const c = new Config(2, beta, { mu, J, h, lmbda: lm }, { rotateEvery: 8 });
const r = new RNG(7);
const S = { plaquette: [], star: [], sigx: [], sigz: [], energy: [] };
for (let i = 0; i < 20000; i++) c[names[moves[r.index(moves.length)]]](r);
for (let s = 0; s < nsamp; s++) {
  for (let i = 0; i < 8; i++) { c[names[moves[r.index(moves.length)]]](r); }
  c.rotate(undefined, r);
  const q = c.measure(); for (const k in S) S[k].push(q[k]);
}
c.checkValid();
const ref = { plaquette: ed.B, star: ed.A, sigx: ed.sx, sigz: ed.sz, energy: ed.E };
for (const k in S) { const be = binnedError(S[k], 32); const m = mean(S[k]); console.log(k.padEnd(10), m.toFixed(5), '+-', be.err.toExponential(1), 'ED', ref[k].toFixed(5), 'z=', ((m - ref[k]) / be.err).toFixed(2), 'tau', tauInt(S[k]).tau.toFixed(1)); }
console.log('acc', Array.from(c.nAcc).map((x, i) => (x / Math.max(1, c.nAtt[i])).toFixed(3)).join(' '));
