'use strict';
// Edwards-Sokal-dressed vison worm (stage 1 prototype). Operates on a qmc.js Config (fields: L, beta, nV, nE, nP,
// mu, J, h, lmbda, src, tgt, plaq, edgePlaq, spin0, kinks, stars).
//
// One call of esStep(cfg, rng, par):
//  1. Bonds given the configuration: cut points on each vertex = star events U Poisson(mu); bond points on each edge at
//     rate 2*lmbda on sigma^z = +1 stretches. Exact Edwards-Sokal conditional; afterwards the lmbda term is only the
//     constraint "every bond point has sigma^z = +1" and star events may sit at any subset of cut points.
//  2. With the bonds fixed and the electric sign field marginalised, a flux configuration (kinks K, flux at tau=0) has
//     weight W_m(K) * N_f(K), W_m = h^|K| exp(J int sum_p B_p), N_f = number of the 8 global sectors (twist set T or
//     its complement) x (two non-contractible edge-loop flips) that admit a valid sigma^z. A Prokof'ev-Svistunov-type
//     worm (open / close, head time shift, hop / unhop, head-tail swap) samples this with an open sector of weight
//     C * W_m(open).
//  3. sigma^z and the star events are resampled uniformly among the valid ones for the final (K, omega): a feasible
//     sector uniformly, then every cluster of segments flipped with probability 1/2 (this step is the electric SW update).
// par: { worm: bool, C, dOpen, dTime, dHop, pTime, pHop, pUnhop, pClose, pSwap, brokenNf: bool }

const TWO_PI_DUMMY = 0;

function expRand(rng, rate) { return -Math.log(1 - rng()) / rate; }
function mod(a, b) { const r = a % b; return r < 0 ? r + b : r; }

// sorted-array helpers
function lowerBound(a, x) { let lo = 0, hi = a.length; while (lo < hi) { const m = (lo + hi) >> 1; if (a[m] < x) lo = m + 1; else hi = m; } return lo; }
function insertSorted(a, x) { a.splice(lowerBound(a, x), 0, x); }
function removeSorted(a, x) { const i = lowerBound(a, x); if (a[i] !== x) throw new Error('removeSorted: missing ' + x); a.splice(i, 1); }
function mergeSorted(...arrs) { const out = [].concat(...arrs); out.sort((p, q) => p - q); return out; }

// ---------------------------------------------------------------------------------------------------------------
// Bonds and cluster graph
function sampleBonds(cfg, rng, rateFactor = 1) {
  const { nV, nE, beta, mu } = cfg; const lmbda = cfg.lmbda * rateFactor; // rateFactor != 1 only for the broken-arm test
  const cuts = new Array(nV), segOff = new Int32Array(nV + 1);
  let n = 0;
  for (let v = 0; v < nV; v++) {
    const c = cfg.stars[v].slice();
    let t = expRand(rng, mu); while (t < beta) { c.push(t); t += expRand(rng, mu); }
    c.sort((p, q) => p - q); cuts[v] = c; segOff[v] = n; n += Math.max(1, c.length);
  }
  segOff[nV] = n;
  const segOf = (v, tau) => { const c = cuts[v]; if (c.length === 0) return segOff[v];
    let lo = 0, hi = c.length; while (lo < hi) { const m = (lo + hi) >> 1; if (c[m] > tau) hi = m; else lo = m + 1; }
    return segOff[v] + (lo === 0 ? c.length - 1 : lo - 1); };
  const bE = [], bT = [], bA = [], bB = [];
  for (let e = 0; e < nE; e++) {
    const fl = mergeSorted(cfg.kinks[e], cfg.stars[cfg.src[e]], cfg.stars[cfg.tgt[e]]);
    let s = cfg.spin0[e], t0 = 0;
    for (let i = 0; i <= fl.length; i++) {
      const t1 = i < fl.length ? fl[i] : beta;
      if (s === 1) { let t = t0 + expRand(rng, 2 * lmbda);
        while (t < t1) { bE.push(e); bT.push(t); bA.push(segOf(cfg.src[e], t)); bB.push(segOf(cfg.tgt[e], t)); t += expRand(rng, 2 * lmbda); } }
      s = -s; t0 = t1;
    }
  }
  const nB = bE.length, deg = new Int32Array(n + 1);
  for (let b = 0; b < nB; b++) { deg[bA[b] + 1]++; deg[bB[b] + 1]++; }
  for (let i = 0; i < n; i++) deg[i + 1] += deg[i];
  const adjN = new Int32Array(2 * nB), adjB = new Int32Array(2 * nB), fill = deg.slice(0, n);
  for (let b = 0; b < nB; b++) { adjN[fill[bA[b]]] = bB[b]; adjB[fill[bA[b]]++] = b; adjN[fill[bB[b]]] = bA[b]; adjB[fill[bB[b]]++] = b; }
  // components
  const comp = new Int32Array(n).fill(-1); let nComp = 0; const q = new Int32Array(n);
  for (let s = 0; s < n; s++) { if (comp[s] >= 0) continue; let h = 0, t = 0; q[t++] = s; comp[s] = nComp;
    while (h < t) { const u = q[h++]; for (let j = deg[u]; j < deg[u + 1]; j++) { const w = adjN[j]; if (comp[w] < 0) { comp[w] = nComp; q[t++] = w; } } }
    nComp++; }
  const vOfSeg = new Int32Array(n); for (let v = 0; v < nV; v++) for (let k = segOff[v]; k < segOff[v + 1]; k++) vOfSeg[k] = v;
  return { cuts, segOff, nSeg: n, nB, bE, bT, bA, bB, deg, adjN, adjB, comp, nComp, vOfSeg };
}

// ---------------------------------------------------------------------------------------------------------------
// Existence test for a closed flux configuration given bonds.
// Input: old config (cfg), new kink lists K2 (array per edge, sorted), set Q of plaquettes whose flux at tau=0 changed.
// Returns { feasible: [8 bools], Nf, data } where data allows building sigma^z for a chosen sector.
function geometryExtras(cfg) {
  if (cfg._esGeo) return cfg._esGeo;
  const { L, nV, nE, nP } = cfg;
  const eid = (x, y, d) => 2 * (mod(x, L) * L + mod(y, L)) + d;
  // non-contractible plaquette-neutral edge loops: y-edges of row y=0, x-edges of column x=0
  const W1 = [], W2 = [];
  for (let x = 0; x < L; x++) W1.push(eid(x, 0, 1));
  for (let y = 0; y < L; y++) W2.push(eid(0, y, 0));
  // vertex adjacency via edges
  const vAdj = Array.from({ length: nV }, () => []);
  for (let e = 0; e < nE; e++) { vAdj[cfg.src[e]].push([cfg.tgt[e], e]); vAdj[cfg.tgt[e]].push([cfg.src[e], e]); }
  // dual adjacency (plaquette graph) via edges
  const pAdj = Array.from({ length: nP }, () => []);
  for (let e = 0; e < nE; e++) { const [p1, p2] = cfg.edgePlaq[e]; pAdj[p1].push([p2, e]); pAdj[p2].push([p1, e]); }
  // BFS spanning tree of the dual graph rooted at plaquette 0, for solving boundary(E0) = Q
  const parent = new Int32Array(nP).fill(-1), parentEdge = new Int32Array(nP).fill(-1), order = [0]; parent[0] = 0;
  for (let i = 0; i < order.length; i++) { const p = order[i]; for (const [w, e] of pAdj[p]) if (parent[w] < 0) { parent[w] = p; parentEdge[w] = e; order.push(w); } }
  cfg._esGeo = { W1, W2, vAdj, pAdj, parent, parentEdge, order };
  return cfg._esGeo;
}

function solveE0(cfg, Qmask) { // edge flips a_e (Uint8) with each plaquette p flipped iff Qmask[p]; null if |Q| odd
  const G = geometryExtras(cfg), a = new Uint8Array(cfg.nE), need = Uint8Array.from(Qmask);
  for (let i = G.order.length - 1; i >= 1; i--) { const p = G.order[i]; if (need[p]) { const e = G.parentEdge[p]; a[e] ^= 1; need[p] = 0; need[G.parent[p]] ^= 1; } }
  if (need[0]) return null;
  return a;
}

function existence(cfg, B, K2, Qmask) {
  const { nV, nE, beta } = cfg; const G = geometryExtras(cfg);
  // kink-parity change per edge and the change times (symmetric difference of old and new kink lists)
  const dK = new Array(nE);
  const dKpar = new Uint8Array(nE);
  for (let e = 0; e < nE; e++) {
    const A = cfg.kinks[e], Bk = K2[e];
    if (A === Bk) { dK[e] = null; continue; }
    const out = []; let i = 0, j = 0;
    while (i < A.length || j < Bk.length) {
      if (j >= Bk.length || (i < A.length && A[i] < Bk[j])) out.push(A[i++]);
      else if (i >= A.length || Bk[j] < A[i]) out.push(Bk[j++]);
      else { i++; j++; }
    }
    dK[e] = out.length ? out : null; dKpar[e] = out.length & 1;
  }
  // twist set T: ds(src)+ds(tgt) = dKpar(e)
  const ds = new Int8Array(nV).fill(-1); ds[0] = 0; const st = [0];
  while (st.length) { const v = st.pop(); for (const [w, e] of G.vAdj[v]) { const want = ds[v] ^ dKpar[e]; if (ds[w] < 0) { ds[w] = want; st.push(w); } else if (ds[w] !== want) return { Nf: 0, feasible: new Array(8).fill(false) }; } }
  const E0 = solveE0(cfg, Qmask);
  if (!E0) return { Nf: 0, feasible: new Array(8).fill(false) };
  const firstCut = new Float64Array(nV); for (let v = 0; v < nV; v++) firstCut[v] = B.cuts[v].length ? B.cuts[v][0] : Infinity;
  const inW1 = new Uint8Array(nE), inW2 = new Uint8Array(nE); for (const e of G.W1) inW1[e] = 1; for (const e of G.W2) inW2[e] = 1;
  // per-bond base parity: a_e(E0) xor (number of kink changes on e before tau_b)
  const base = new Uint8Array(B.nB);
  for (let b = 0; b < B.nB; b++) {
    const e = B.bE[b], t = B.bT[b]; let par = E0[e];
    if (dK[e]) par ^= lowerBound(dK[e], t) & 1;
    base[b] = par;
  }
  const feasible = [], sols = [];
  const col = new Int8Array(B.nSeg);
  for (let combo = 0; combo < 8; combo++) {
    const tw = combo & 1, w1 = (combo >> 1) & 1, w2 = (combo >> 2) & 1;
    // twist of vertex v: ds[v] xor tw; a twisted vertex needs a cut point
    let ok = true;
    for (let v = 0; v < nV && ok; v++) if ((ds[v] ^ tw) && B.cuts[v].length === 0) ok = false;
    if (!ok) { feasible.push(false); sols.push(null); continue; }
    const r = new Uint8Array(B.nB);
    for (let b = 0; b < B.nB; b++) {
      const e = B.bE[b], t = B.bT[b], u = cfg.src[e], w = cfg.tgt[e];
      let p = base[b] ^ (w1 & inW1[e]) ^ (w2 & inW2[e]);
      if ((ds[u] ^ tw) && t < firstCut[u]) p ^= 1;
      if ((ds[w] ^ tw) && t < firstCut[w]) p ^= 1;
      r[b] = p;
    }
    // parity BFS over all segments: x_u xor x_w = r_b
    col.fill(-1);
    for (let s = 0; s < B.nSeg && ok; s++) {
      if (col[s] >= 0) continue; col[s] = 0; const qq = [s];
      for (let hh = 0; hh < qq.length && ok; hh++) { const u = qq[hh];
        for (let j = B.deg[u]; j < B.deg[u + 1]; j++) { const w = B.adjN[j], want = col[u] ^ r[B.adjB[j]];
          if (col[w] < 0) { col[w] = want; qq.push(w); } else if (col[w] !== want) { ok = false; break; } } }
    }
    feasible.push(ok); sols.push(ok ? { tw, w1, w2, col: Int8Array.from(col) } : null);
  }
  const Nf = feasible.filter(Boolean).length;
  return { Nf, feasible, sols, E0, ds, dK };
}

// Build the new configuration from a feasible sector solution; flip every component with probability 1/2.
function rebuild(cfg, B, K2, ex, combo, rng) {
  const { nV, nE } = cfg; const G = geometryExtras(cfg); const sol = ex.sols[combo];
  const flipComp = new Uint8Array(B.nComp); for (let c = 0; c < B.nComp; c++) flipComp[c] = rng() < 0.5 ? 1 : 0;
  const x = new Uint8Array(B.nSeg); for (let s = 0; s < B.nSeg; s++) x[s] = sol.col[s] ^ flipComp[B.comp[s]];
  const inW1 = new Set(G.W1), inW2 = new Set(G.W2);
  // eta at tau = 0 for each vertex: wrap segment value, times twist
  const eta0 = new Uint8Array(nV), newStars = new Array(nV);
  for (let v = 0; v < nV; v++) {
    const c = B.cuts[v], off = B.segOff[v], twisted = ex.ds[v] ^ sol.tw;
    if (c.length === 0) { eta0[v] = x[off]; newStars[v] = []; continue; }
    const m = c.length, wrap = x[off + m - 1];
    eta0[v] = wrap ^ twisted;
    const oldSet = new Set(cfg.stars[v]); const ns = [];
    for (let k = 0; k < m; k++) {
      const before = k === 0 ? (wrap ^ twisted) : x[off + k - 1], after = x[off + k];
      if (oldSet.has(c[k]) !== (before !== after)) ns.push(c[k]);
    }
    newStars[v] = ns;
  }
  for (let e = 0; e < nE; e++) {
    let f = ex.E0[e] ^ (sol.w1 & (inW1.has(e) ? 1 : 0)) ^ (sol.w2 & (inW2.has(e) ? 1 : 0)) ^ eta0[cfg.src[e]] ^ eta0[cfg.tgt[e]];
    if (f) cfg.spin0[e] = -cfg.spin0[e];
  }
  for (let v = 0; v < nV; v++) cfg.stars[v] = newStars[v];
  for (let e = 0; e < nE; e++) cfg.kinks[e] = K2[e] === cfg.kinks[e] ? cfg.kinks[e] : K2[e].slice();
}

// Debug check: all bond points satisfied by the current configuration.
function bondsSatisfied(cfg, B) {
  for (let b = 0; b < B.nB; b++) {
    const e = B.bE[b], t = B.bT[b];
    const fl = mergeSorted(cfg.kinks[e], cfg.stars[cfg.src[e]], cfg.stars[cfg.tgt[e]]);
    const s = cfg.spin0[e] * ((lowerBound(fl, t) & 1) ? -1 : 1);
    if (s !== 1) return false;
  }
  return true;
}

// ---------------------------------------------------------------------------------------------------------------
// Worm on flux variables. Working state: K (kink lists, copy-on-write), plaquette flip lists pe[p] (kinks on its edges +
// defects), bflip[p] (flux at tau=0 flipped relative to start).
function plaqEvents(cfg, K, p) { const [a, b, c, d] = cfg.plaq[p]; return mergeSorted(K[a], K[b], K[c], K[d]); }
function plaqB0(cfg, p) { let s = 1; for (const e of cfg.plaq[p]) s *= cfg.spin0[e]; return s; }

// integral of B over the arc from t to t+u (u signed, |u| < beta), B(tau) = b0 * (-1)^{#events <= tau}
function arcIntegral(ev, b0, beta, t, u) {
  const a = u >= 0 ? t : mod(t + u, beta), len = Math.abs(u);
  const segInt = (lo, hi) => { // integral over [lo, hi] within [0, beta]
    let i = lowerBound(ev, lo); let sgn = b0 * ((i & 1) ? -1 : 1), acc = 0, cur = lo;
    while (i < ev.length && ev[i] < hi) { acc += sgn * (ev[i] - cur); cur = ev[i]; sgn = -sgn; i++; }
    return acc + sgn * (hi - cur);
  };
  if (a + len <= beta) return segInt(a, a + len);
  return segInt(a, beta) + segInt(0, a + len - beta);
}
const crosses0 = (t, u, beta) => (u >= 0 ? t + u >= beta : t + u < 0);

function esStep(cfg, rng, par) {
  const { beta, nP, J, h } = cfg;
  const B = sampleBonds(cfg, rng, par.bondFactor || 1);
  const stats = { opened: 0, closed: 0, wormSteps: 0, Nf0: 0, Nf1: 0 };
  const K = cfg.kinks.slice(); // copy-on-write per edge
  const touched = new Uint8Array(cfg.nE);
  const cow = e => { if (!touched[e]) { K[e] = K[e].slice(); touched[e] = 1; } };
  let Kfinal = K, Qfinal = new Uint8Array(nP);
  if (par.worm) {
    const ex0 = existence(cfg, B, cfg.kinks, new Uint8Array(nP));
    const Nf0 = par.brokenNf ? (ex0.Nf > 0 ? 1 : 0) : ex0.Nf; stats.Nf0 = ex0.Nf;
    const pe = new Map(), b0 = new Map(), bflip = new Uint8Array(nP);
    const ev = p => { if (!pe.has(p)) { pe.set(p, plaqEvents(cfg, K, p)); b0.set(p, plaqB0(cfg, p)); } return pe.get(p); };
    const bval = p => (b0.get(p) * (bflip[p] ? -1 : 1));
    const flipArc = (p, t, u) => -2 * J * arcIntegral(ev(p), bval(p), beta, t, u); // dlogW for flipping B_p on the arc
    // open
    for (const d of [par.dOpen, par.dTime, par.dHop]) if (!(d > 0 && d < beta / 2)) throw new Error('esStep: windows must lie in (0, beta/2)');
    const sel = [par.pTime, par.pHop, par.pUnhop, par.pClose, par.pSwap]; const selSum = sel.reduce((a, b) => a + b, 0);
    const pCloseN = par.pClose / selSum; // probability that an open-state step proposes close
    const p0 = Math.floor(rng() * nP), tt = rng() * beta, u0 = (2 * rng() - 1) * par.dOpen;
    const selOpen = 1; // open is attempted once per esStep
    let dlog = flipArc(p0, tt, u0);
    let R = par.C * Math.exp(dlog) / Nf0 * pCloseN / (selOpen / (nP * beta * 2 * par.dOpen));
    if (Nf0 > 0 && rng() < R) {
      stats.opened = 1;
      // apply open
      let evp = ev(p0); insertSorted(evp, tt); insertSorted(evp, mod(tt + u0, beta)); if (crosses0(tt, u0, beta)) bflip[p0] ^= 1;
      let tail = { p: p0, t: tt }, head = { p: p0, t: mod(tt + u0, beta) };
      let logWopen = 0; // log W_m(open) - log W_m(start), accumulated
      logWopen += dlog;
      for (;;) {
        stats.wormSteps++;
        let r = rng() * selSum, mv = 0; while (r >= sel[mv]) { r -= sel[mv]; mv++; }
        if (mv === 0) { // time shift of head
          const u = (2 * rng() - 1) * par.dTime; const d = flipArc(head.p, head.t, u);
          if (rng() < Math.exp(d)) { const evh = ev(head.p); removeSorted(evh, head.t); const nt = mod(head.t + u, beta); insertSorted(evh, nt);
            if (crosses0(head.t, u, beta)) bflip[head.p] ^= 1; head.t = nt; logWopen += d; }
        } else if (mv === 1) { // hop: kink at t' = head.t + u on edge e, head moves to the other plaquette at head.t
          const e = cfg.plaq[head.p][Math.floor(rng() * 4)]; const [pa, pb] = cfg.edgePlaq[e]; const p2 = pa === head.p ? pb : pa;
          const u = (2 * rng() - 1) * par.dHop, tk = mod(head.t + u, beta);
          // flux: B_p flipped on arc head.t -> tk; B_p2 flipped on arc tk -> head.t (same arc)
          const d1 = flipArc(head.p, head.t, u);
          // apply to p temporarily to evaluate p2 correctly when p2 == p (cannot happen: edge has two distinct plaquettes)
          const d2 = flipArc(p2, head.t, u);
          // n_win after insertion: kinks on e within distance dHop of head.t
          const nwin = countWindow(K[e], head.t, par.dHop, beta) + 1;
          const Rh = h * Math.exp(d1 + d2) * (par.bugHop ? 1 : 2 * par.dHop / nwin) * (par.pUnhop / par.pHop); // bugHop: broken-arm test only
          if (rng() < Rh) {
            cow(e); insertSorted(K[e], tk);
            const e1 = ev(head.p); removeSorted(e1, head.t); insertSorted(e1, tk); if (crosses0(head.t, u, beta)) bflip[head.p] ^= 1;
            const e2 = ev(p2); insertSorted(e2, tk); insertSorted(e2, head.t); if (crosses0(head.t, u, beta)) bflip[p2] ^= 1;
            head = { p: p2, t: head.t }; logWopen += Math.log(h) + d1 + d2;
          }
        } else if (mv === 2) { // unhop: remove a kink on edge e within dHop of head.t, head moves to the other plaquette
          const e = cfg.plaq[head.p][Math.floor(rng() * 4)]; const [pa, pb] = cfg.edgePlaq[e]; const p2 = pa === head.p ? pb : pa;
          const win = windowList(K[e], head.t, par.dHop, beta); if (win.length === 0) continue;
          const tk = win[Math.floor(rng() * win.length)]; const nwin = win.length;
          let u = tk - head.t; if (u > beta / 2) u -= beta; if (u < -beta / 2) u += beta; // signed short arc head.t -> tk
          // removing the kink: B_head.p flipped on arc (tk, head.t) and B_p2 flipped on the same arc
          const d1 = flipArc(head.p, head.t, u), d2 = flipArc(p2, head.t, u);
          const Ru = Math.exp(d1 + d2) / h * (par.pHop / (2 * par.dHop)) / (par.pUnhop / nwin);
          if (rng() < Ru) {
            cow(e); removeSorted(K[e], tk);
            const e1 = ev(head.p); removeSorted(e1, head.t); removeSorted(e1, tk); if (crosses0(head.t, u, beta)) bflip[head.p] ^= 1;
            const e2 = ev(p2); removeSorted(e2, tk); insertSorted(e2, head.t); if (crosses0(head.t, u, beta)) bflip[p2] ^= 1;
            head = { p: p2, t: head.t }; logWopen += d1 + d2 - Math.log(h);
          }
        } else if (mv === 3) { // close
          if (head.p !== tail.p) continue;
          let u = head.t - tail.t; if (u > beta / 2) u -= beta; if (u < -beta / 2) u += beta;
          if (Math.abs(u) >= par.dOpen) continue;
          const d = flipArc(tail.p, tail.t, u);
          // closed candidate: flux at 0 flipped where bflip (after removing the defects)
          const Q = Uint8Array.from(bflip); if (crosses0(tail.t, u, beta)) Q[tail.p] ^= 1;
          const ex1 = existence(cfg, B, K, Q); stats.closeTries = (stats.closeTries || 0) + 1; if (ex1.Nf === 0) stats.closeInfeasible = (stats.closeInfeasible || 0) + 1;
          const Nf1 = par.brokenNf ? (ex1.Nf > 0 ? 1 : 0) : ex1.Nf;
          if (Nf1 === 0) continue;
          const Rc = Nf1 * Math.exp(d) / par.C * (selOpen / (nP * beta * 2 * par.dOpen)) / pCloseN;
          // Rc relative to the open state; the open-state weight is C*W_m(open), closed is W_m*Nf
          if (rng() < Rc) {
            const evt = ev(tail.p); removeSorted(evt, tail.t); removeSorted(evt, head.t); Qfinal = Q; stats.closed = 1; stats.Nf1 = ex1.Nf;
            Kfinal = K; break;
          }
        } else { // swap head and tail
          const tmp = head; head = tail; tail = tmp;
        }
      }
    }
  }
  // final: resample sigma^z and stars uniformly among valid configurations for (Kfinal, omega)
  const exF = existence(cfg, B, Kfinal, Qfinal);
  if (exF.Nf === 0) throw new Error('esStep: final configuration has no valid sector');
  const feas = []; exF.feasible.forEach((f, i) => { if (f) feas.push(i); });
  rebuild(cfg, B, Kfinal, exF, feas[Math.floor(rng() * feas.length)], rng);
  if (par.debug && !bondsSatisfied(cfg, B)) throw new Error('esStep: bond violated after rebuild');
  return stats;
}

function windowList(a, t, d, beta) { // entries within circular distance < d of t
  const out = [];
  const lo = t - d, hi = t + d;
  const push = (x0, x1) => { for (let i = lowerBound(a, x0); i < a.length && a[i] < x1; i++) out.push(a[i]); };
  if (lo >= 0 && hi <= beta) push(lo, hi);
  else if (lo < 0) { push(0, hi); push(lo + beta, beta); }
  else { push(lo, beta); push(0, hi - beta); }
  return out;
}
function countWindow(a, t, d, beta) { return windowList(a, t, d, beta).length; }

module.exports = { esStep, sampleBonds, existence, rebuild, bondsSatisfied, arcIntegral };
