'use strict';
// JavaScript port of the z-basis local updates of ParaToric's continuous-time worldline QMC
// (src/mcmc/extended_toric_code_qmc.hpp, Basis='z').  Seven updates, each proposed with prob. 1/7:
//   0 double single-spin flip (create/destroy a kink pair on an edge)
//   1 single-spin-flip move (move one kink inside the window set by the neighbouring flips of its edge)
//   2 global single-spin flip (flip sigma^z at all times on one edge)
//   3 global tuple flip (flip sigma^z at all times on the 4 edges of a star)
//   4 double tuple flip (create/destroy a pair of star events at one vertex)
//   5 single tuple-flip move (move a star event inside the window set by its 4 edges)
//   6 spin-tuple combination (create/destroy a star event plus one kink on each of its 4 edges)
// H = -mu sum A_s - J sum B_p - h sum sigma^x - lmbda sum sigma^z.
// Acceptance ratios are the C++ ones; the integrals are recomputed directly from the event lists
// (no caches): the local weight change is E_after - E_before of the affected edges/plaquettes.

const EPS = 2.220446049250313e-16; // PRECISION in the C++

// ---------------------------------------------------------------- RNG (xoshiro128**, seeded by splitmix32)
class RNG {
  constructor(seed) {
    let s = (Number(seed) >>> 0) ^ 0x9e3779b9;
    const sm = () => {
      s = (s + 0x9e3779b9) | 0;
      let z = s;
      z = Math.imul(z ^ (z >>> 16), 0x85ebca6b);
      z = Math.imul(z ^ (z >>> 13), 0xc2b2ae35);
      return (z ^ (z >>> 16)) >>> 0;
    };
    this.a = sm(); this.b = sm(); this.c = sm(); this.d = sm();
    if ((this.a | this.b | this.c | this.d) === 0) this.a = 1;
    for (let i = 0; i < 32; i++) this.next32();
  }
  next32() {
    let a = this.a, b = this.b, c = this.c, d = this.d;
    const b5 = Math.imul(b, 5);
    const r = Math.imul(((b5 << 7) | (b5 >>> 25)), 9) >>> 0;
    const t = b << 9;
    c ^= a; d ^= b; b ^= c; a ^= d; c ^= t;
    d = (d << 11) | (d >>> 21);
    this.a = a; this.b = b; this.c = c; this.d = d;
    return r;
  }
  // uniform double in [0,1) with 53 random bits
  uniform() {
    const hi = this.next32() >>> 5, lo = this.next32() >>> 6;
    return (hi * 67108864 + lo) / 9007199254740992;
  }
  index(n) { return Math.floor(this.uniform() * n); }
}

// ---------------------------------------------------------------- small sorted-array helpers
function upperBound(a, t) { // first index with a[i] > t
  let lo = 0, hi = a.length;
  while (lo < hi) { const m = (lo + hi) >>> 1; if (a[m] <= t) lo = m + 1; else hi = m; }
  return lo;
}
function lowerBound(a, t) { // first index with a[i] >= t
  let lo = 0, hi = a.length;
  while (lo < hi) { const m = (lo + hi) >>> 1; if (a[m] < t) lo = m + 1; else hi = m; }
  return lo;
}
function insertSorted(a, t) {
  const n = a.length;
  a.push(t);
  let i = n;
  while (i > 0 && a[i - 1] > t) { a[i] = a[i - 1]; i--; }
  a[i] = t;
  return i;
}
function removeAt(a, i) {
  const n = a.length - 1;
  for (let j = i; j < n; j++) a[j] = a[j + 1];
  a.length = n;
}
function removeValue(a, t) { // t must be present
  removeAt(a, lowerBound(a, t));
}

// ---------------------------------------------------------------- Config
class Config {
  constructor(L, beta, couplings, opts) {
    opts = opts || {};
    this.L = L; this.beta = beta;
    this.nV = L * L; this.nE = 2 * L * L; this.nP = L * L;
    const c = couplings || {};
    this.mu = c.mu; this.J = c.J; this.h = c.h; this.lmbda = c.lmbda;
    // geometry
    const eid = (x, y, d) => 2 * ((((x % L) + L) % L) * L + (((y % L) + L) % L)) + d;
    this.eid = eid;
    this.src = new Int32Array(this.nE); this.tgt = new Int32Array(this.nE);
    for (let x = 0; x < L; x++) for (let y = 0; y < L; y++) {
      const v = y * L + x;
      const e0 = eid(x, y, 0), e1 = eid(x, y, 1);
      this.src[e0] = v; this.tgt[e0] = y * L + ((x + 1) % L);
      this.src[e1] = v; this.tgt[e1] = ((y + 1) % L) * L + x;
    }
    this.plaq = []; this.star = [];
    for (let y = 0; y < L; y++) for (let x = 0; x < L; x++) {
      this.plaq.push([eid(x, y, 0), eid(x, y + 1, 0), eid(x, y, 1), eid(x + 1, y, 1)]);
      this.star.push([eid(x, y, 0), eid(x - 1, y, 0), eid(x, y, 1), eid(x, y - 1, 1)]);
    }
    this.edgePlaq = [];
    for (let e = 0; e < this.nE; e++) this.edgePlaq.push([]);
    for (let p = 0; p < this.nP; p++) for (const e of this.plaq[p]) this.edgePlaq[e].push(p);
    this.starPlaq = []; // distinct plaquettes touching the 4 edges of a star
    for (let v = 0; v < this.nV; v++) {
      const s = new Set();
      for (const e of this.star[v]) for (const p of this.edgePlaq[e]) s.add(p);
      this.starPlaq.push(Array.from(s));
    }
    // state
    this.spin0 = new Int8Array(this.nE).fill(1);
    this.kinks = []; for (let e = 0; e < this.nE; e++) this.kinks.push([]);
    this.stars = []; for (let v = 0; v < this.nV; v++) this.stars.push([]);
    this.extraUpdate = null; this.pExtra = 0;
    // steps between global time shifts (the C++ shifts after every Metropolis step; a sweep is much cheaper
    // and the shift only improves mixing, it is not needed for detailed balance)
    this.rotateEvery = opts.rotateEvery !== undefined ? opts.rotateEvery : this.nE;
    this.stepCount = 0;
    this.nAtt = new Float64Array(7); this.nAcc = new Float64Array(7); this.nAccExtra = 0; this.nAttExtra = 0;
    this.bug = opts.bug || 0; // deliberate-bug switch for the validation-gate test (0 = correct)
    // star-move window rule: false = boost Lattice (cyclic window, default backend); true = FlatSquareLattice (no wrap)
    this.flatWindow = !!opts.flatWindow;
    this._pl = 0; this._nx = 0;
  }

  randomizeSpins(rng) { for (let e = 0; e < this.nE; e++) this.spin0[e] = rng.uniform() < 0.5 ? -1 : 1; }

  // ---- integrals -------------------------------------------------------
  // int_0^beta sigma^z_e: the edge flips at kinks[e] and at the star events of both end vertices
  sigmaZIntegral(e) {
    const K = this.kinks[e], A = this.stars[this.src[e]], B = this.stars[this.tgt[e]];
    const nk = K.length, na = A.length, nb = B.length, beta = this.beta;
    let i = 0, j = 0, k = 0, s = this.spin0[e], tp = 0, acc = 0;
    for (;;) {
      let t = beta, w = 0;
      if (i < nk && K[i] < t) { t = K[i]; w = 1; }
      if (j < na && A[j] < t) { t = A[j]; w = 2; }
      if (k < nb && B[k] < t) { t = B[k]; w = 3; }
      if (w === 0) break;
      acc += s * (t - tp); tp = t; s = -s;
      if (w === 1) i++; else if (w === 2) j++; else k++;
    }
    return acc + s * (beta - tp);
  }
  // int_0^beta B_p.  [A_s,B_p]=0: B_p only changes at kinks of its four edges (each star event flips 0 or 2 of them).
  plaqIntegral(p) {
    const pe = this.plaq[p];
    const a = this.kinks[pe[0]], b = this.kinks[pe[1]], c = this.kinks[pe[2]], d = this.kinks[pe[3]];
    const na = a.length, nb = b.length, nc = c.length, nd = d.length, beta = this.beta;
    let i = 0, j = 0, k = 0, l = 0, tp = 0, acc = 0;
    let s = this.spin0[pe[0]] * this.spin0[pe[1]] * this.spin0[pe[2]] * this.spin0[pe[3]];
    for (;;) {
      let t = beta, w = 0;
      if (i < na && a[i] < t) { t = a[i]; w = 1; }
      if (j < nb && b[j] < t) { t = b[j]; w = 2; }
      if (k < nc && c[k] < t) { t = c[k]; w = 3; }
      if (l < nd && d[l] < t) { t = d[l]; w = 4; }
      if (w === 0) break;
      acc += s * (t - tp); tp = t; s = -s;
      if (w === 1) i++; else if (w === 2) j++; else if (w === 3) k++; else l++;
    }
    return acc + s * (beta - tp);
  }
  // brute-force version using all flips of the four edges (used by the self-test)
  plaqIntegralFull(p) {
    const pe = this.plaq[p];
    let s = 1; const ts = [];
    for (const e of pe) {
      s *= this.spin0[e];
      for (const t of this.kinks[e]) ts.push(t);
      for (const t of this.stars[this.src[e]]) ts.push(t);
      for (const t of this.stars[this.tgt[e]]) ts.push(t);
    }
    ts.sort((x, y) => x - y);
    let tp = 0, acc = 0;
    for (const t of ts) { acc += s * (t - tp); tp = t; s = -s; }
    return acc + s * (this.beta - tp);
  }
  totalSigmaZ() { let a = 0; for (let e = 0; e < this.nE; e++) a += this.sigmaZIntegral(e); return a; }
  totalPlaq() { let a = 0; for (let p = 0; p < this.nP; p++) a += this.plaqIntegral(p); return a; }
  nKinkTotal() { let n = 0; for (let e = 0; e < this.nE; e++) n += this.kinks[e].length; return n; }
  nStarTotal() { let n = 0; for (let v = 0; v < this.nV; v++) n += this.stars[v].length; return n; }
  // diagonal energy integral  -J int sum B - lmbda int sum sigma^z   (not divided by beta)
  diagEnergyIntegral() { return -this.J * this.totalPlaq() - this.lmbda * this.totalSigmaZ(); }

  measure() {
    const beta = this.beta, P = this.totalPlaq(), Z = this.totalSigmaZ();
    const ns = this.nStarTotal(), nk = this.nKinkTotal();
    return {
      plaquette: P / (beta * this.nP),
      star: this.mu === 0 ? 0 : ns / (beta * this.mu * this.nV),
      sigx: this.h === 0 ? 0 : nk / (beta * this.h * this.nE),
      sigz: Z / (beta * this.nE),
      // <H> = -(N_star + N_kink)/beta - J sum<B> - lmbda sum<sigma^z>   (total, not per site)
      energy: -(ns + nk) / beta - this.J * P / beta - this.lmbda * Z / beta,
    };
  }

  checkValid() {
    const beta = this.beta;
    const fail = (m) => { throw new Error('invalid state: ' + m); };
    for (let e = 0; e < this.nE; e++) {
      if (this.spin0[e] !== 1 && this.spin0[e] !== -1) fail('spin0 edge ' + e);
      const K = this.kinks[e];
      for (let i = 0; i < K.length; i++) {
        if (!(K[i] >= 0 && K[i] < beta)) fail('kink time out of range edge ' + e + ' t=' + K[i]);
        if (i > 0 && !(K[i] > K[i - 1])) fail('kinks not strictly sorted edge ' + e);
      }
      const n = K.length + this.stars[this.src[e]].length + this.stars[this.tgt[e]].length;
      if (n % 2 !== 0) fail('odd number of flips on edge ' + e);
    }
    for (let v = 0; v < this.nV; v++) {
      const S = this.stars[v];
      for (let i = 0; i < S.length; i++) {
        if (!(S[i] >= 0 && S[i] < beta)) fail('star time out of range vertex ' + v + ' t=' + S[i]);
        if (i > 0 && !(S[i] > S[i - 1])) fail('stars not strictly sorted vertex ' + v);
      }
    }
    return true;
  }

  // ---- global shift of the imaginary-time origin (Lattice::rotate_imag_time) -------------
  rotate(tau0, rng) {
    const beta = this.beta;
    if (tau0 === undefined) tau0 = rng.uniform() * beta;
    // spin0 flips for edges with an odd number of flips before tau0 (computed before the arrays are modified)
    for (let e = 0; e < this.nE; e++) {
      const n = lowerBound(this.kinks[e], tau0) + lowerBound(this.stars[this.src[e]], tau0) + lowerBound(this.stars[this.tgt[e]], tau0);
      if (n & 1) this.spin0[e] = -this.spin0[e];
    }
    const shift = (a) => {
      const n = a.length; if (n === 0) return;
      const k = lowerBound(a, tau0);
      if (k === 0) { for (let i = 0; i < n; i++) { const t = a[i] - tau0; a[i] = t === 0 ? EPS : t; } return; }
      const tmp = a.slice(k).concat(a.slice(0, k));
      for (let i = 0; i < n; i++) {
        let t = tmp[i] - tau0; if (t < 0) t += beta;
        if (t >= beta) t -= beta;
        a[i] = t === 0 ? EPS : t;
      }
    };
    for (let e = 0; e < this.nE; e++) shift(this.kinks[e]);
    for (let v = 0; v < this.nV; v++) shift(this.stars[v]);
  }

  // ---- local energy helpers --------------------------------------------
  _Eedge(e) { // -lmbda int sigma^z_e - J sum_{p containing e} int B_p
    const ep = this.edgePlaq[e];
    return -this.lmbda * this.sigmaZIntegral(e) - this.J * (this.plaqIntegral(ep[0]) + this.plaqIntegral(ep[1]));
  }
  _EstarZ(v) { // -lmbda sum_{e in star} int sigma^z_e   (plaquettes do not change under star-event moves)
    const se = this.star[v];
    return -this.lmbda * (this.sigmaZIntegral(se[0]) + this.sigmaZIntegral(se[1]) + this.sigmaZIntegral(se[2]) + this.sigmaZIntegral(se[3]));
  }
  _EstarFull(v) {
    const sp = this.starPlaq[v];
    let E = this._EstarZ(v);
    for (let i = 0; i < sp.length; i++) E -= this.J * this.plaqIntegral(sp[i]);
    return E;
  }
  // unwrapped neighbouring flips of edge e around time t (over kinks + both end-vertex star lists);
  // results in this._pl (largest flip < t, or last flip - beta) and this._nx (smallest flip > t, or first flip + beta)
  _edgeNeighbours(e, t) {
    const beta = this.beta;
    let prev = -Infinity, next = Infinity, maxAll = -Infinity, minAll = Infinity;
    for (let q = 0; q < 3; q++) {
      const a = q === 0 ? this.kinks[e] : (q === 1 ? this.stars[this.src[e]] : this.stars[this.tgt[e]]);
      const n = a.length; if (n === 0) continue;
      const lb = lowerBound(a, t), ub = upperBound(a, t);
      if (lb > 0 && a[lb - 1] > prev) prev = a[lb - 1];
      if (ub < n && a[ub] < next) next = a[ub];
      if (a[n - 1] > maxAll) maxAll = a[n - 1];
      if (a[0] < minAll) minAll = a[0];
    }
    if (prev === -Infinity) prev = maxAll - beta;
    if (next === Infinity) next = minAll + beta;
    this._pl = prev; this._nx = next;
  }

  _accept(ratio, rng) { return ratio >= 1.0 || rng.uniform() < ratio; }

  // ================================================================= the seven updates
  // 0: metropolis_step_double_single_spin_flip
  _doubleSingleSpinFlip(rng) {
    const h = this.h, beta = this.beta;
    if (h === 0) return;
    const rnd = rng.uniform();
    const e = rng.index(this.nE);
    const K = this.kinks[e], n = K.length;
    if (rnd < 0.5) { // destroy a pair of neighbouring kinks
      if (n > 1) {
        const i = rng.index(n - 1);
        const t1 = K[i], t2 = K[i + 1];
        const nn = (i !== n - 2) ? K[i + 2] : beta;
        const pv = (i !== 0) ? K[i - 1] : 0;
        const E0 = this._Eedge(e);
        removeAt(K, i); removeAt(K, i);
        const dE = this._Eedge(e) - E0;
        const w = nn - pv;
        const ratio = 2 / (h * h * w * w) * Math.exp(-dE);
        this.nAtt[0]++;
        if (this._accept(ratio, rng)) { this.nAcc[0]++; }
        else { insertSorted(K, t1); insertSorted(K, t2); }
      }
    } else { // create a pair in a randomly chosen gap of the kink list
      let tl, tr;
      if (n === 0) { tl = 0; tr = beta; }
      else {
        const g = rng.index(n + 1);
        if (g === 0) { tl = 0; tr = K[0]; }
        else if (g === n) { tl = K[g - 1]; tr = beta; }
        else { tl = K[g - 1]; tr = K[g]; }
      }
      let t1 = tl + (tr - tl) * rng.uniform(), t2 = tl + (tr - tl) * rng.uniform();
      if (t2 < t1) { const x = t1; t1 = t2; t2 = x; }
      if (Math.abs(t2 - t1) < EPS || Math.abs(t1 - tl) < EPS || Math.abs(t2 - tr) < EPS) return;
      const E0 = this._Eedge(e);
      insertSorted(K, t1); insertSorted(K, t2);
      const dE = this._Eedge(e) - E0;
      const ratio = (tr - tl) * (tr - tl) / 2 * h * h * Math.exp(-dE) * (this.bug === 1 ? 2 : 1);
      this.nAtt[0]++;
      if (this._accept(ratio, rng)) { this.nAcc[0]++; }
      else { removeValue(K, t1); removeValue(K, t2); }
    }
  }

  // 1: metropolis_step_single_spin_flip_move.  The kink moves uniformly inside the window between
  // the previous and next flip (kink or star event) of its edge; if that window contains tau=0 and the
  // kink crosses it, sigma^z at tau=0 flips.
  _singleSpinFlipMove(rng) {
    if (this.h === 0) return;
    const beta = this.beta;
    const e = rng.index(this.nE);
    const K = this.kinks[e], n = K.length;
    if (n === 0) return;
    const i = rng.index(n);
    const t = K[i];
    this._edgeNeighbours(e, t);
    const prev = this._pl, next = this._nx;
    const u = prev + (next - prev) * rng.uniform(); // unwrapped; the arc (prev,next) contains t
    let tn = u; let cross = false;
    if (u < 0) { tn = u + beta; cross = true; } else if (u >= beta) { tn = u - beta; cross = true; }
    if (Math.abs(tn - t) < EPS) return;
    if (cross && (Math.abs(tn - ((prev + beta) % beta)) < EPS || Math.abs(tn - (next % beta)) < EPS)) return;
    const E0 = this._Eedge(e);
    removeAt(K, i); const j = insertSorted(K, tn);
    if (cross) this.spin0[e] = -this.spin0[e];
    const dE = this._Eedge(e) - E0;
    this.nAtt[1]++;
    if (this._accept(Math.exp(-dE), rng)) { this.nAcc[1]++; }
    else { removeAt(K, j); insertSorted(K, t); if (cross) this.spin0[e] = -this.spin0[e]; }
  }

  // 2: metropolis_step_global_single_spin_flip
  _globalSingleSpinFlip(rng) {
    const e = rng.index(this.nE);
    const E0 = this._Eedge(e);
    this.spin0[e] = -this.spin0[e];
    const dE = this._Eedge(e) - E0;
    this.nAtt[2]++;
    if (this._accept(Math.exp(-dE), rng)) { this.nAcc[2]++; }
    else this.spin0[e] = -this.spin0[e];
  }

  // 3: metropolis_step_global_tuple_flip (flip all four edges of a star at all times)
  _globalTupleFlip(rng) {
    const v = rng.index(this.nV), se = this.star[v];
    const E0 = this._EstarFull(v);
    for (let q = 0; q < 4; q++) this.spin0[se[q]] = -this.spin0[se[q]];
    const dE = this._EstarFull(v) - E0;
    this.nAtt[3]++;
    if (this._accept(Math.exp(-dE), rng)) { this.nAcc[3]++; }
    else for (let q = 0; q < 4; q++) this.spin0[se[q]] = -this.spin0[se[q]];
  }

  // 4: metropolis_step_double_tuple_flip (pair of star events at one vertex)
  _doubleTupleFlip(rng) {
    const mu = this.mu, beta = this.beta;
    if (mu === 0) return;
    const rnd = rng.uniform();
    const v = rng.index(this.nV);
    const S = this.stars[v], n = S.length;
    if (rnd < 0.5) {
      if (n > 1) {
        const i = rng.index(n - 1);
        const t1 = S[i], t2 = S[i + 1];
        const nn = (i !== n - 2) ? S[i + 2] : beta;
        const pv = (i !== 0) ? S[i - 1] : 0;
        const E0 = this._EstarZ(v);
        removeAt(S, i); removeAt(S, i);
        const dE = this._EstarZ(v) - E0;
        const w = nn - pv;
        const ratio = (this.bug === 2 ? 1 : 2) / (mu * mu * w * w) * Math.exp(-dE);
        this.nAtt[4]++;
        if (this._accept(ratio, rng)) { this.nAcc[4]++; }
        else { insertSorted(S, t1); insertSorted(S, t2); }
      }
    } else {
      let tl, tr;
      if (n === 0) { tl = 0; tr = beta; }
      else {
        const g = rng.index(n + 1);
        if (g === 0) { tl = 0; tr = S[0]; }
        else if (g === n) { tl = S[g - 1]; tr = beta; }
        else { tl = S[g - 1]; tr = S[g]; }
      }
      let t1 = tl + (tr - tl) * rng.uniform(), t2 = tl + (tr - tl) * rng.uniform();
      if (t2 < t1) { const x = t1; t1 = t2; t2 = x; }
      if (Math.abs(t2 - t1) < EPS || Math.abs(t1 - tl) < EPS || Math.abs(t2 - tr) < EPS) return;
      const E0 = this._EstarZ(v);
      insertSorted(S, t1); insertSorted(S, t2);
      const dE = this._EstarZ(v) - E0;
      const ratio = (tr - tl) * (tr - tl) / 2 * mu * mu * Math.exp(-dE);
      this.nAtt[4]++;
      if (this._accept(ratio, rng)) { this.nAcc[4]++; }
      else { removeValue(S, t1); removeValue(S, t2); }
    }
  }

  // 5: metropolis_step_single_tuple_flip_move.  Window = (max over the 4 edges of the previous flip,
  // min over the 4 edges of the next flip); crossing tau=0 flips spin0 on all four edges.
  _singleTupleFlipMove(rng) {
    if (this.mu === 0) return;
    const beta = this.beta;
    const v = rng.index(this.nV), se = this.star[v];
    const S = this.stars[v], n = S.length;
    if (n === 0) return;
    const i = rng.index(n);
    const t = S[i];
    let wl = -Infinity, wr = Infinity;
    for (let q = 0; q < 4; q++) {
      this._edgeNeighbours(se[q], t);
      if (this._pl > wl) wl = this._pl;
      if (this._nx < wr) wr = this._nx;
    }
    if (this.flatWindow) { if (wl < 0) wl = 0; if (wr > beta) wr = beta; } // FlatSquareLattice::tuple_flip_window (no wrap)
    const u = wl + (wr - wl) * rng.uniform();
    let tn = u; let cross = false;
    if (u < 0) { tn = u + beta; cross = true; } else if (u >= beta) { tn = u - beta; cross = true; }
    if (Math.abs(tn - t) < EPS) return;
    if (cross && (Math.abs(tn - ((wl + beta) % beta)) < EPS || Math.abs(tn - (wr % beta)) < EPS)) return;
    const E0 = this._EstarZ(v);
    removeAt(S, i); const j = insertSorted(S, tn);
    if (cross) for (let q = 0; q < 4; q++) this.spin0[se[q]] = -this.spin0[se[q]];
    const dE = this._EstarZ(v) - E0;
    this.nAtt[5]++;
    if (this._accept(Math.exp(-dE), rng)) { this.nAcc[5]++; }
    else {
      removeAt(S, j); insertSorted(S, t);
      if (cross) for (let q = 0; q < 4; q++) this.spin0[se[q]] = -this.spin0[se[q]];
    }
  }

  // 6: metropolis_step_spin_tuple_combination: create/destroy a star event at tau_new together with, on each
  // of its 4 edges independently, creation or destruction of a kink (destroyed kink = nearest kink before or
  // after tau_new, chosen with prob 1/2; created kink uniform in the kink-gap around tau_new).
  _spinTupleCombination(rng) {
    const mu = this.mu, h = this.h, beta = this.beta;
    if (mu === 0 || h === 0) return;
    const rnd0 = rng.uniform();
    const v = rng.index(this.nV), se = this.star[v];
    const S = this.stars[v], n = S.length;
    let destroyTuple, tauNew, tl = 0, tr = 0, r = 1;
    if (rnd0 < 0.5) {
      destroyTuple = true;
      if (n === 0) return;
      const si = rng.index(n);
      tauNew = S[si];
      tr = si < n - 1 ? S[si + 1] : beta;
      tl = si > 0 ? S[si - 1] : 0;
      r = 1 / ((tr - tl) * mu);
    } else {
      destroyTuple = false;
      if (n === 0) { tl = 0; tr = beta; }
      else {
        const g = rng.index(n + 1);
        if (g === 0) { tl = 0; tr = S[0]; }
        else if (g === n) { tl = S[g - 1]; tr = beta; }
        else { tl = S[g - 1]; tr = S[g]; }
      }
      if (!(tl < tr)) return;
      tauNew = tl + (tr - tl) * rng.uniform();
      // reject if a flip sits exactly at tauNew on any of the four edges (C++ check_spin_flips_present_tuple)
      for (let q = 0; q < 4; q++) {
        const e = se[q];
        const K = this.kinks[e], A = this.stars[this.src[e]], B = this.stars[this.tgt[e]];
        let a = lowerBound(K, tauNew); if (a < K.length && K[a] === tauNew) return;
        a = lowerBound(A, tauNew); if (a < A.length && A[a] === tauNew) return;
        a = lowerBound(B, tauNew); if (a < B.length && B[a] === tauNew) return;
      }
      if (Math.abs(tauNew - tl) < EPS || Math.abs(tauNew - tr) < EPS) return;
      r = (tr - tl) * mu;
    }
    // per-edge kink decisions
    const create = [false, false, false, false];
    const ktime = [0, 0, 0, 0];
    let rb = 1;
    for (let q = 0; q < 4; q++) {
      const e = se[q], K = this.kinks[e], nk = K.length;
      const rc = rng.uniform();
      const ni = upperBound(K, tauNew);
      if (rc < 0.5) { // destroy a kink
        if (nk === 0) return;
        create[q] = false;
        const idx = (rng.uniform() < 0.5) ? ni - 1 : ni; // previous (-1 if none) or next (nk if none) kink
        if (idx === -1 || idx === nk) return;
        const tpc = idx > 0 ? K[idx - 1] : 0;
        const tnc = idx < nk - 1 ? K[idx + 1] : beta;
        rb *= 2 / ((tnc - tpc) * h);
        ktime[q] = K[idx];
      } else { // create a kink
        create[q] = true;
        const pT = ni > 0 ? K[ni - 1] : 0;
        const nT = ni < nk ? K[ni] : beta;
        const tk = pT + (nT - pT) * rng.uniform();
        if (Math.abs(tk - pT) < EPS || Math.abs(tk - nT) < EPS) return;
        if (Math.abs(tk - tauNew) < EPS) return;
        rb *= (nT - pT) * h / 2;
        ktime[q] = tk;
      }
    }
    const E0 = this._EstarFull(v);
    for (let q = 0; q < 4; q++) {
      const K = this.kinks[se[q]];
      if (create[q]) insertSorted(K, ktime[q]); else removeValue(K, ktime[q]);
    }
    if (destroyTuple) removeValue(S, tauNew); else insertSorted(S, tauNew);
    const dE = this._EstarFull(v) - E0;
    const ratio = Math.exp(-dE) * r * rb;
    this.nAtt[6]++;
    if (this._accept(ratio, rng)) { this.nAcc[6]++; }
    else {
      for (let q = 0; q < 4; q++) {
        const K = this.kinks[se[q]];
        if (create[q]) removeValue(K, ktime[q]); else insertSorted(K, ktime[q]);
      }
      if (destroyTuple) insertSorted(S, tauNew); else removeValue(S, tauNew);
    }
  }

  // dispatcher (metropolis_step): one of the seven updates with probability 1/7 each
  localStep(rng) {
    this.stepCount++;
    if (this.extraUpdate && this.pExtra > 0 && rng.uniform() < this.pExtra) {
      this.nAttExtra++;
      if (this.extraUpdate(this, rng)) this.nAccExtra++;
    } else {
      const r = rng.index(7);
      switch (r) {
        case 0: this._doubleSingleSpinFlip(rng); break;
        case 1: this._singleSpinFlipMove(rng); break;
        case 2: this._globalSingleSpinFlip(rng); break;
        case 3: this._globalTupleFlip(rng); break;
        case 4: this._doubleTupleFlip(rng); break;
        case 5: this._singleTupleFlipMove(rng); break;
        default: this._spinTupleCombination(rng); break;
      }
    }
    if (this.rotateEvery > 0 && this.stepCount % this.rotateEvery === 0) this.rotate(undefined, rng);
  }
}

module.exports = { Config, RNG, EPS, upperBound, lowerBound, insertSorted, removeAt, removeValue };
