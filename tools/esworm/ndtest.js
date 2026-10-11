'use strict';
// Cost of exact elimination of the gsw system under a nested-dissection order (scaling test, 2026-10-11).
// usage: node ndtest.js L nWarm seed
// 1. equilibrate with global gsw steps, build one system (rows over GF(2), variable coordinates)
// 2. peel leaves (as in the sampler); on the core build the variable adjacency graph
// 3. nested dissection: recursive coordinate bisection (longest box side, torus ignored: wrap neighbours land in the
//    separator); order = left part, right part, separator
// 4. sparse Gaussian elimination in that order, counting XOR work (sum of row lengths touched) and the largest front
//    (rows active at a pivot); compare with the natural order.
const { Config, RNG } = require('./qmc.js'); const { gswStep } = require('./gsw.js');
const [L, nWarm, seed] = process.argv.slice(2).map(Number);
const cfg = new Config(L, L, { mu: 1, J: 1, h: 0.3391, lmbda: 0.3391 }), rng = new RNG(seed);
for (let i = 0; i < nWarm; i++) gswStep(cfg, () => rng.uniform(), {});
const sys = gswStep(cfg, () => rng.uniform(), { returnSystem: true });
const nVar = sys.nVar, beta = sys.beta;
// peel
let rows = sys.rows.map(r => r.slice());
{ const deg = new Int32Array(nVar), occ = Array.from({ length: nVar }, () => []);
  rows.forEach((r, i) => r.forEach(v => { deg[v]++; occ[v].push(i); }));
  const alive = new Uint8Array(rows.length).fill(1), st = []; for (let v = 0; v < nVar; v++) if (deg[v] === 1) st.push(v);
  while (st.length) { const v = st.pop(); if (deg[v] !== 1) continue; const i = occ[v].find(j => alive[j]); if (i === undefined) continue; alive[i] = 0; for (const u of rows[i]) { deg[u]--; if (deg[u] === 1) st.push(u); } }
  rows = rows.filter((_, i) => alive[i]); }
const coreVars = Array.from(new Set(rows.flat())); const idx = new Int32Array(nVar).fill(-1); coreVars.forEach((v, i) => (idx[v] = i));
const n = coreVars.length; rows = rows.map(r => r.map(v => idx[v]));
const X = new Float64Array(n), Y = new Float64Array(n), T = new Float64Array(n);
coreVars.forEach((v, i) => { X[i] = sys.coord[3 * v]; Y[i] = sys.coord[3 * v + 1]; T[i] = sys.coord[3 * v + 2] * (L / beta); });
const adj = Array.from({ length: n }, () => new Set()); for (const r of rows) for (const a of r) for (const b of r) if (a !== b) adj[a].add(b);
// nested dissection
const order = []; let topSep = 0;
function nd(vars, depth) {
  if (vars.length <= 48) { order.push(...vars); return; }
  let best = 0, ext = -1; const coords = [X, Y, T];
  for (let d = 0; d < 3; d++) { let lo = Infinity, hi = -Infinity; for (const v of vars) { lo = Math.min(lo, coords[d][v]); hi = Math.max(hi, coords[d][v]); } if (hi - lo > ext) { ext = hi - lo; best = d; } }
  const s = vars.slice().sort((a, b) => coords[best][a] - coords[best][b]); const half = s.length >> 1;
  const inLeft = new Set(s.slice(0, half)), inSet = new Set(vars);
  const sep = [], left = [], right = s.slice(half);
  for (const v of s.slice(0, half)) { let cross = false; for (const u of adj[v]) if (inSet.has(u) && !inLeft.has(u)) { cross = true; break; } (cross ? sep : left).push(v); }
  if (depth === 0) topSep = sep.length;
  nd(left, depth + 1); nd(right, depth + 1); order.push(...sep);
}
// elimination cost for a given order
function eliminate(ord) {
  const pos = new Int32Array(n); ord.forEach((v, i) => (pos[v] = i));
  const R = rows.map(r => Int32Array.from(r.map(v => pos[v])).sort());
  const col = Array.from({ length: n }, () => new Set()); R.forEach((r, i) => r.forEach(p => col[p].add(i)));
  const used = new Uint8Array(R.length); let work = 0, maxFront = 0, fillMax = 0;
  for (let p = 0; p < n; p++) {
    const cand = Array.from(col[p]).filter(i => !used[i]); if (cand.length === 0) continue;
    maxFront = Math.max(maxFront, cand.length);
    const piv = cand[0]; used[piv] = 1; const pr = R[piv];
    for (const q of pr) col[q].delete(piv);
    for (const i of cand.slice(1)) {
      const a = R[i]; const out = []; let x = 0, y = 0; // XOR of sorted arrays
      while (x < a.length || y < pr.length) { if (y >= pr.length || (x < a.length && a[x] < pr[y])) out.push(a[x++]); else if (x >= a.length || pr[y] < a[x]) out.push(pr[y++]); else { x++; y++; } }
      work += a.length + pr.length;
      for (const q of a) col[q].delete(i); for (const q of out) col[q].add(i); R[i] = Int32Array.from(out); fillMax = Math.max(fillMax, out.length);
    }
  }
  return { work, maxFront, fillMax };
}
nd(Array.from({ length: n }, (_, i) => i), 0);
const t0 = Date.now(); const a = eliminate(order); const tND = (Date.now() - t0) / 1000;
console.log(`L=${L} vars ${nVar} core vars ${n} core rows ${rows.length} | ND: top separator ${topSep}, work ${a.work.toExponential(2)}, max front ${a.maxFront}, max row ${a.fillMax}, ${tND.toFixed(1)} s`);
