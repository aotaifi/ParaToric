'use strict';
// Self-tests of qmc.js (structure, invariants).  usage: node test.js
const { Config, RNG } = require('./qmc.js');
let fails = 0;
function check(name, ok, info) { console.log((ok ? 'PASS ' : 'FAIL ') + name + (info ? '  ' + info : '')); if (!ok) fails++; }

for (const L of [2, 3, 5]) {
  const cfg = new Config(L, 2.5, { mu: 1.1, J: 0.8, h: 0.6, lmbda: 0.45 }, { rotateEvery: 7 });
  const rng = new RNG(12345 + L);
  cfg.randomizeSpins(rng);
  // geometry: every edge in exactly 2 plaquettes and 2 stars; stars/plaquettes have 4 distinct edges
  let geo = true;
  for (let e = 0; e < cfg.nE; e++) if (cfg.edgePlaq[e].length !== 2 || cfg.edgePlaq[e][0] === cfg.edgePlaq[e][1]) geo = false;
  for (let v = 0; v < cfg.nV; v++) if (new Set(cfg.star[v]).size !== 4 || new Set(cfg.plaq[v]).size !== 4) geo = false;
  const cnt = new Int32Array(cfg.nE);
  for (let v = 0; v < cfg.nV; v++) for (const e of cfg.star[v]) cnt[e]++;
  for (let e = 0; e < cfg.nE; e++) if (cnt[e] !== 2 || (cfg.src[e] === cfg.tgt[e])) geo = false;
  check('L=' + L + ' geometry', geo);

  let maxDiff = 0, validOK = true, rotDiff = 0, starPlaqDiff = 0;
  for (let it = 0; it < 200000; it++) {
    // star-only updates must leave sum B_p unchanged (the shortcut used in the local energies)
    const P0 = cfg.totalPlaq();
    cfg.rotateEvery = 0;
    const r = rng.index(3);
    if (r === 0) cfg._globalTupleFlip(rng); else if (r === 1) cfg._doubleTupleFlip(rng); else cfg._singleTupleFlipMove(rng);
    starPlaqDiff = Math.max(starPlaqDiff, Math.abs(cfg.totalPlaq() - P0));
    // plus the full seven-move dispatcher
    cfg.rotateEvery = 7;
    cfg.localStep(rng);
    if (it % 500 === 0) {
      try { cfg.checkValid(); } catch (err) { validOK = false; console.log(err.message); break; }
      for (let p = 0; p < cfg.nP; p++) maxDiff = Math.max(maxDiff, Math.abs(cfg.plaqIntegral(p) - cfg.plaqIntegralFull(p)));
      const E0 = cfg.diagEnergyIntegral(), n0 = cfg.nKinkTotal() + cfg.nStarTotal();
      cfg.rotate(undefined, rng);
      rotDiff = Math.max(rotDiff, Math.abs(cfg.diagEnergyIntegral() - E0), Math.abs(cfg.nKinkTotal() + cfg.nStarTotal() - n0));
    }
  }
  check('L=' + L + ' state stays valid (parity, order, range) over 2e5 steps', validOK);
  check('L=' + L + ' int B_p (kink-only merge) == brute force over all flips', maxDiff < 1e-10, 'max diff ' + maxDiff.toExponential(2));
  check('L=' + L + ' star-only updates leave sum int B_p unchanged', starPlaqDiff < 1e-10, 'max diff ' + starPlaqDiff.toExponential(2));
  check('L=' + L + ' time-origin shift preserves energy integral and event count', rotDiff < 1e-9, 'max diff ' + rotDiff.toExponential(2));
  const acc = Array.from(cfg.nAcc).map((a, i) => (a / Math.max(1, cfg.nAtt[i])).toFixed(3)).join(' ');
  console.log('   acceptance per update (0..6): ' + acc + '   events: kinks ' + cfg.nKinkTotal() + ' stars ' + cfg.nStarTotal());
}
console.log(fails === 0 ? 'ALL PASS' : fails + ' FAILED');
process.exit(fails === 0 ? 0 : 1);
