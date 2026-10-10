'use strict';
// ED of the 2x2 toric code in fields. Basis: bit e of the state = 0 -> sigma^z=+1, 1 -> -1.
const L = 2, NE = 8, N = 256;
const eid = (x, y, d) => 2 * ((((x % L) + L) % L) * L + (((y % L) + L) % L)) + d;
const stars = [], plaqs = [];
for (let y = 0; y < L; y++) for (let x = 0; x < L; x++) {
  stars.push([eid(x, y, 0), eid(x - 1, y, 0), eid(x, y, 1), eid(x, y - 1, 1)]);
  plaqs.push([eid(x, y, 0), eid(x, y + 1, 0), eid(x, y, 1), eid(x + 1, y, 1)]);
}
for (const g of [...stars, ...plaqs]) if (new Set(g).size !== 4) throw new Error('degenerate geometry');
const mask = g => g.reduce((m, e) => m | (1 << e), 0);
const sm = stars.map(mask), pm = plaqs.map(mask);
const pop = x => { let c = 0; while (x) { c += x & 1; x >>= 1; } return c; };
const zsign = (s, m) => (pop(s & m) & 1) ? -1 : 1;

function buildH(mu, J, h, lm, brk) {
  const H = Array.from({ length: N }, () => new Float64Array(N));
  const lsign = brk ? -1 : 1; // deliberate break: wrong sign of lmbda term
  for (let s = 0; s < N; s++) {
    let d = 0;
    for (const m of pm) d -= J * zsign(s, m);
    for (let e = 0; e < NE; e++) d -= lsign * lm * zsign(s, 1 << e);
    H[s][s] += d;
    for (const m of sm) H[s ^ m][s] -= mu;
    for (let e = 0; e < NE; e++) H[s ^ (1 << e)][s] -= h;
  }
  return H;
}
// cyclic Jacobi; returns {E, V} with V[i][k] = component i of eigenvector k
function jacobi(A0) {
  const n = A0.length, A = A0.map(r => Float64Array.from(r));
  const V = Array.from({ length: n }, (_, i) => { const r = new Float64Array(n); r[i] = 1; return r; });
  for (let sweep = 0; sweep < 60; sweep++) {
    let off = 0;
    for (let i = 0; i < n; i++) for (let j = i + 1; j < n; j++) off += A[i][j] * A[i][j];
    if (off < 1e-30) break;
    for (let p = 0; p < n - 1; p++) for (let q = p + 1; q < n; q++) {
      const apq = A[p][q]; if (Math.abs(apq) < 1e-300) continue;
      const th = (A[q][q] - A[p][p]) / (2 * apq);
      const t = Math.sign(th || 1) / (Math.abs(th) + Math.sqrt(th * th + 1));
      const c = 1 / Math.sqrt(t * t + 1), s = t * c;
      for (let k = 0; k < n; k++) { const a = A[k][p], b = A[k][q]; A[k][p] = c * a - s * b; A[k][q] = s * a + c * b; }
      for (let k = 0; k < n; k++) { const a = A[p][k], b = A[q][k]; A[p][k] = c * a - s * b; A[q][k] = s * a + c * b; }
      for (let k = 0; k < n; k++) { const a = V[k][p], b = V[k][q]; V[k][p] = c * a - s * b; V[k][q] = s * a + c * b; }
    }
  }
  return { E: Array.from({ length: n }, (_, i) => A[i][i]), V };
}
const matvec = (H, v) => { const w = new Float64Array(N); for (let i = 0; i < N; i++) { let a = 0; for (let j = 0; j < N; j++) a += H[i][j] * v[j]; w[i] = a; } return w; };

function thermal(mu, J, h, lm, beta, brk) {
  const H = buildH(mu, J, h, lm, brk), { E, V } = jacobi(H);
  const E0 = Math.min(...E);
  let Z = 0; const w = E.map(e => { const x = Math.exp(-beta * (e - E0)); Z += x; return x; });
  let A = 0, B = 0, X = 0, Zs = 0, En = 0;
  for (let k = 0; k < N; k++) {
    const v = new Float64Array(N); for (let i = 0; i < N; i++) v[i] = V[i][k];
    let a = 0, b = 0, x = 0, z = 0;
    for (let s = 0; s < N; s++) {
      const c = v[s]; if (c === 0) continue;
      for (const m of sm) a += v[s ^ m] * c;
      for (const m of pm) b += zsign(s, m) * c * c;
      for (let e = 0; e < NE; e++) { x += v[s ^ (1 << e)] * c; z += zsign(s, 1 << e) * c * c; }
    }
    const f = w[k] / Z; A += f * a / 4; B += f * b / 4; X += f * x / 8; Zs += f * z / 8; En += f * E[k];
  }
  return { A, B, sx: X, sz: Zs, E: En, Nkink: beta * h * 8 * X, Nstar: beta * mu * 4 * A, spectrum: E, V, H };
}
module.exports = { thermal, buildH, jacobi, matvec, stars, plaqs, N };

if (require.main === module) {
  const fs = require('fs'), out = {};
  console.log('stars', JSON.stringify(stars), '\nplaqs', JSON.stringify(plaqs));
  { const r = thermal(1, 1, 0.3391, 0.2, 2); let res = 0, orth = 0;
    for (let k = 0; k < N; k++) { const v = new Float64Array(N); for (let i = 0; i < N; i++) v[i] = r.V[i][k];
      const Hv = matvec(r.H, v); for (let i = 0; i < N; i++) res = Math.max(res, Math.abs(Hv[i] - r.spectrum[k] * v[i]));
      for (let l = 0; l <= k; l++) { let d = 0; for (let i = 0; i < N; i++) d += r.V[i][k] * r.V[i][l]; orth = Math.max(orth, Math.abs(d - (k === l ? 1 : 0))); } }
    console.log('(a) max|Hv-Ev| =', res.toExponential(2), ' max|V^T V - 1| =', orth.toExponential(2)); }
  const testB = brk => { let mx = 0; for (const [h, l, b] of [[0.3, 0.7, 1], [0.2, 0.5, 2], [0, 0.3391, 2], [0.6, 0.1, 3]]) {
      const r = thermal(0, 0, h, l, b, brk), rr = Math.hypot(h, l), t = Math.tanh(b * rr);
      mx = Math.max(mx, Math.abs(r.sx - h / rr * t), Math.abs(r.sz - l / rr * t)); } return mx; };
  console.log('(b) mu=J=0 max dev from tanh formula =', testB(false).toExponential(2));
  console.log('(d) same with wrong-sign lmbda term: max dev =', testB(true).toExponential(2), '(must be large)');
  // (c) h=lm=0: independent count. Joint eigenvalues a_s,b_p=+-1 with prod a = prod b = 1: 8x8 sectors, degeneracy 256/64=4 each.
  let mxc = 0; for (const [mu, J, b] of [[1, 1, 1], [0.7, 1.3, 3], [1, 0.4, 2]]) {
    let Zc = 0, Ac = 0, Bc = 0;
    for (let a = 0; a < 16; a++) { const sa = [0, 1, 2, 3].map(i => (a >> i & 1) ? -1 : 1); if (sa[0] * sa[1] * sa[2] * sa[3] !== 1) continue;
      for (let c = 0; c < 16; c++) { const sb = [0, 1, 2, 3].map(i => (c >> i & 1) ? -1 : 1); if (sb[0] * sb[1] * sb[2] * sb[3] !== 1) continue;
        const SA = sa.reduce((x, y) => x + y), SB = sb.reduce((x, y) => x + y), wt = 4 * Math.exp(b * (mu * SA + J * SB));
        Zc += wt; Ac += wt * SA / 4; Bc += wt * SB / 4; } }
    const r = thermal(mu, J, 0, 0, b);
    const dev = Math.max(Math.abs(r.A - Ac / Zc), Math.abs(r.B - Bc / Zc)); mxc = Math.max(mxc, dev);
    console.log(`(c) mu=${mu} J=${J} beta=${b}: ED <A>=${r.A.toFixed(10)} <B>=${r.B.toFixed(10)} count <A>=${(Ac / Zc).toFixed(10)} <B>=${(Bc / Zc).toFixed(10)}`); }
  { const r = thermal(1, 1, 0, 0, 1), E0 = Math.min(...r.spectrum); console.log('(c) ground-level degeneracy from ED (expect 4):', r.spectrum.filter(e => Math.abs(e - E0) < 1e-9).length, ' E0=', E0); }
  console.log('(c) max dev =', mxc.toExponential(2));
  const pts = { P1: [1, 1, .3391, .3391, 1], P2: [1, 1, .3391, .3391, 2], P3: [1, 1, .3391, .3391, 4], P4: [1, 1, .2, .5, 2], P5: [1, 1, .6, .1, 2], P6: [.7, 1.3, .3391, .3391, 3], P7: [1, 1, .3391, 0, 2], P8: [1, 1, 0, .3391, 2] };
  for (const [k, p] of Object.entries(pts)) { const r = thermal(...p);
    out[k] = { mu: p[0], J: p[1], h: p[2], lmbda: p[3], beta: p[4], A: r.A, B: r.B, sigma_x: r.sx, sigma_z: r.sz, H: r.E, N_kink: r.Nkink, N_star: r.Nstar };
    console.log(k, p.join(','), '<A>', r.A.toFixed(8), '<B>', r.B.toFixed(8), '<sx>', r.sx.toFixed(8), '<sz>', r.sz.toFixed(8), '<H>', r.E.toFixed(8), 'Nkink', r.Nkink.toFixed(6), 'Nstar', r.Nstar.toFixed(6)); }
  fs.writeFileSync(__dirname + '/reference.json', JSON.stringify(out, null, 1));
}
