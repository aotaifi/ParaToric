'use strict';
// usage: node validate.js CKPT L BETA [wrong]
// Checks that the parsed checkpoint and the assumed geometry reproduce the code's own redundant data.
// T1 parse ends at EOF, vectors sorted, flip-count caches match.
// T2 every edge's flip list == its kinks merged with both endpoints' star events (pins the geometry).
// T3 per-edge sigma^z integrals == stored integrated_edge_energy (up to a constant factor, reported).
// T4 plaquette integrals: <B> per site, and per-plaquette comparison with the stored vector.
// 'wrong' uses a deliberately wrong edge ordering: T2 must then fail.
const { parseCheckpoint, geometry, edgeIntegral } = require('./ckpt.js');
const [path, Ls, betas, wrong] = process.argv.slice(2);
const L = +Ls, beta = +betas;
const c = parseCheckpoint(path, L);
const G = geometry(L, wrong ? 'wrong' : 'insertion');
const out = [];
const sorted = a => { for (let i = 1; i < a.length; i++) if (a[i] < a[i - 1]) return false; return true; };
let nK = 0, nS = 0, allSorted = true;
c.kinks.forEach(a => { nK += a.length; allSorted &&= sorted(a); });
c.flips.forEach(a => { allSorted &&= sorted(a); });
c.star.forEach(a => { nS += a.length; allSorted &&= sorted(a); });
out.push(`T1 bytesLeft=${c.bytesLeft} headerBytes=${c.headerBytes} sorted=${allSorted} kinks=${nK} (cache ${c.caches.n_single}) stars=${nS} (cache ${c.caches.n_star})`);
const t1 = c.bytesLeft === 0 && allSorted && nK === c.caches.n_single && nS === c.caches.n_star;

let bad = 0, oddParity = 0;
for (let e = 0; e < c.nE; e++) {
  const m = Array.from(c.kinks[e]).concat(Array.from(c.star[G.src[e]]), Array.from(c.star[G.tgt[e]])).sort((a, b) => a - b);
  const f = c.flips[e];
  let ok = m.length === f.length;
  for (let i = 0; ok && i < m.length; i++) if (m[i] !== f[i]) ok = false;
  if (!ok) bad++;
  if (f.length % 2) oddParity++;
}
out.push(`T2 edges whose flips != kinks+endpoint stars: ${bad}/${c.nE}; edges with odd flip count: ${oddParity}`);
const t2 = bad === 0 && oddParity === 0;

let maxdev = 0, sumZ = 0, ratio = null;
for (let e = 0; e < c.nE; e++) {
  const I = edgeIntegral(c.spin[e], c.flips[e], beta); sumZ += I;
  if (ratio === null && Math.abs(I) > 1) ratio = c.edgeE[e] / I;
}
for (let e = 0; e < c.nE; e++) {
  const I = edgeIntegral(c.spin[e], c.flips[e], beta);
  maxdev = Math.max(maxdev, Math.abs(c.edgeE[e] - ratio * I));
}
out.push(`T3 stored/computed edge integral factor=${ratio}, max deviation=${maxdev.toExponential(2)}; <sigma^z>=${(sumZ / (beta * c.nE)).toFixed(4)}; tot cache=${c.caches.tot_edgeE.toFixed(3)} vs computed*factor=${(ratio * sumZ).toFixed(3)}`);
const t3 = maxdev < 1e-6 * beta;

// plaquette integrals from the four edges
let sumB = 0, pmax = 0; const Bint = new Float64Array(c.nP);
for (let p = 0; p < c.nP; p++) {
  const ev = [];
  for (const e of G.plaq[p]) for (const t of c.flips[e]) ev.push(t);
  ev.sort((a, b) => a - b);
  let s = 1; for (const e of G.plaq[p]) s *= c.spin[e];
  let t = 0, acc = 0;
  for (const tt of ev) { acc += s * (tt - t); t = tt; s = -s; }
  acc += s * (beta - t); Bint[p] = acc; sumB += acc;
}
// stored per-plaquette vector order is unknown: compare as sorted multisets
const a = Array.from(Bint).sort((x, y) => x - y), b = Array.from(c.plaqE).sort((x, y) => x - y);
let rB = b[b.length - 1] / a[a.length - 1];
for (let i = 0; i < a.length; i++) pmax = Math.max(pmax, Math.abs(b[i] - rB * a[i]));
const nPlaqFlips = c.plaqFlips.reduce((s, v) => s + v.length, 0);
out.push(`T4 <B_p>=${(sumB / (beta * c.nP)).toFixed(4)}; stored plaquette integrals vs computed (sorted multisets): factor=${rB}, max dev=${pmax.toExponential(2)}; stored plaquette flip events=${nPlaqFlips} (cache ${c.caches.n_plaq})`);
const t4 = pmax < 1e-6 * beta;
let sumA = 0; for (let v = 0; v < c.nV; v++) sumA += c.star[v].length;
out.push(`info star events per vertex per unit tau = ${(sumA / (c.nV * beta)).toFixed(4)}; kinks per edge per unit tau = ${(nK / (c.nE * beta)).toFixed(4)}`);
out.push(`PASS T1=${t1} T2=${t2} T3=${t3} T4=${t4}`);
console.log(out.join('\n'));
