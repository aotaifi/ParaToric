'use strict';
// Continuous-time generalised Swendsen-Wang step for the toric code in fields (sigma^z basis), on a qmc.js Config.
// All four terms get an Edwards-Sokal decomposition:
//   h  (kinks):  cut points on each edge = kinks U Poisson(h); a cut may or may not carry a kink, equal weight
//   mu (stars):  cut points on each vertex = stars U Poisson(mu); idem for star events
//   lmbda:       bond points on each edge at rate 2*lmbda on sigma^z = +1 stretches; constraint sigma^z = +1 there
//   J:           bond points on each plaquette at rate 2*J on B_p = +1 stretches; constraint B_p = +1 there
// Given cuts and bonds every admissible configuration has the same weight, so the new configuration is a uniformly
// random solution of a linear system over GF(2). Variables: s (edge segments between edge cuts), g (vertex segments
// between vertex cuts), twist bits t_v (odd number of star events at v); sigma^z(e,tau) = s_e + g_u + g_w (mod 2), the
// edge twist is t_u + t_w, and a twisted timeline flips its value on [0, first cut). Solved by dense bitset Gaussian
// elimination with uniformly random free variables.
// opts: { jFactor (broken-arm test only), debug }

function expRand(rng, rate) { return rate > 0 ? -Math.log(1 - rng()) / rate : Infinity; }
function countLE(a, x) { let lo = 0, hi = a.length; while (lo < hi) { const m = (lo + hi) >> 1; if (a[m] <= x) lo = m + 1; else hi = m; } return lo; }
function lowerBound(a, x) { let lo = 0, hi = a.length; while (lo < hi) { const m = (lo + hi) >> 1; if (a[m] < x) lo = m + 1; else hi = m; } return lo; }
function mergeSorted(...arrs) { const out = [].concat(...arrs); out.sort((p, q) => p - q); return out; }
function poissonUnion(base, rate, beta, rng) { const c = base.slice(); let t = expRand(rng, rate); while (t < beta) { c.push(t); t += expRand(rng, rate); } c.sort((p, q) => p - q); return c; }

// Uniformly random solution of the homogeneous GF(2) system rows (each row: list of variable indices, XOR = 0).
// Dense bitset Gauss-Jordan elimination; free variables uniform, pivots determined.
function solveDense(rows, nVar, rng, rhs) {
  const W = (nVar + 31) >>> 5, nr = rows.length;
  const M = new Uint32Array(nr * W), R = new Uint8Array(nr);
  for (let r = 0; r < nr; r++) { for (const v of rows[r]) M[r * W + (v >>> 5)] ^= (1 << (v & 31)) >>> 0; if (rhs) R[r] = rhs[r]; }
  const pivCol = [], pivRowOf = new Int32Array(nVar).fill(-1);
  let rank = 0;
  for (let col = 0; col < nVar && rank < nr; col++) {
    const wi = col >>> 5, bit = (1 << (col & 31)) >>> 0;
    let pr = -1; for (let r = rank; r < nr; r++) if (M[r * W + wi] & bit) { pr = r; break; }
    if (pr < 0) continue;
    if (pr !== rank) { for (let k = 0; k < W; k++) { const a = M[pr * W + k]; M[pr * W + k] = M[rank * W + k]; M[rank * W + k] = a; } const b = R[pr]; R[pr] = R[rank]; R[rank] = b; }
    for (let r = 0; r < nr; r++) if (r !== rank && (M[r * W + wi] & bit)) { const ro = r * W, po = rank * W; for (let k = wi; k < W; k++) M[ro + k] ^= M[po + k]; R[r] ^= R[rank]; }
    pivCol.push(col); pivRowOf[col] = rank; rank++;
  }
  // rows beyond rank are zero; with a right-hand side they must also have rhs 0 (the current configuration solves the system)
  for (let r = rank; r < nr; r++) if (R[r]) throw new Error('solveDense: inconsistent system');
  const x = new Uint8Array(nVar);
  for (let col = 0; col < nVar; col++) if (pivRowOf[col] < 0) x[col] = rng() < 0.5 ? 1 : 0;
  for (let i = pivCol.length - 1; i >= 0; i--) { // RREF: pivot = rhs XOR the free variables in its row
    const col = pivCol[i], ro = pivRowOf[col] * W; let v = R[pivRowOf[col]];
    for (let k = col >>> 5; k < W; k++) { let word = M[ro + k]; if (k === col >>> 5) word &= ~(((1 << (col & 31)) >>> 0) | (((1 << (col & 31)) >>> 0) - 1)) >>> 0;
      while (word) { const b = 31 - Math.clz32(word & -word); const c = (k << 5) + b; if (pivRowOf[c] < 0) v ^= x[c]; word &= word - 1; } }
    x[col] = v;
  }
  return { x, rank };
}

// Same distribution, faster: peel (XORSAT leaf removal). A variable that occurs in exactly one remaining equation can
// always be set last to satisfy it, so equation and variable are removed; this repeats until a core is left. The core
// is solved uniformly (dense), variables in no equation are uniform, peeled variables are set in reverse order.
// The map (core solution, free bits) -> full solution is a bijection, so the result is uniform over all solutions.
function solvePeel(rows, nVar, rng, rhs) {
  const nr = rows.length;
  const deg = new Int32Array(nVar), start = new Int32Array(nVar + 1);
  for (const r of rows) for (const v of r) deg[v]++;
  for (let v = 0; v < nVar; v++) start[v + 1] = start[v] + deg[v];
  const inc = new Int32Array(start[nVar]), fill = start.slice(0, nVar);
  for (let r = 0; r < nr; r++) for (const v of rows[r]) inc[fill[v]++] = r;
  const alive = new Uint8Array(nr).fill(1), peeledVar = [], peeledRow = [];
  const stack = []; for (let v = 0; v < nVar; v++) if (deg[v] === 1) stack.push(v);
  while (stack.length) {
    const v = stack.pop(); if (deg[v] !== 1) continue;
    let r = -1; for (let k = start[v]; k < start[v + 1]; k++) if (alive[inc[k]]) { r = inc[k]; break; }
    if (r < 0) continue;
    alive[r] = 0; peeledVar.push(v); peeledRow.push(r);
    for (const u of rows[r]) { deg[u]--; if (deg[u] === 1) stack.push(u); }
  }
  // core
  const coreRows = [], coreRhs = [], map = new Int32Array(nVar).fill(-1), back = [];
  for (let r = 0; r < nr; r++) if (alive[r]) { const cr = []; for (const v of rows[r]) { if (map[v] < 0) { map[v] = back.length; back.push(v); } cr.push(map[v]); } coreRows.push(cr); coreRhs.push(rhs ? rhs[r] : 0); }
  const core = coreRows.length ? solveDense(coreRows, back.length, rng, coreRhs) : { x: new Uint8Array(0), rank: 0 };
  const x = new Uint8Array(nVar), set = new Uint8Array(nVar);
  for (let i = 0; i < back.length; i++) { x[back[i]] = core.x[i]; set[back[i]] = 1; }
  const isPeeled = new Uint8Array(nVar); for (const v of peeledVar) isPeeled[v] = 1;
  for (let v = 0; v < nVar; v++) if (!set[v] && !isPeeled[v]) { x[v] = rng() < 0.5 ? 1 : 0; set[v] = 1; }
  for (let i = peeledVar.length - 1; i >= 0; i--) {
    const v = peeledVar[i], r = peeledRow[i]; let s = rhs ? rhs[r] : 0;
    for (const u of rows[r]) if (u !== v) s ^= x[u];
    x[v] = s; set[v] = 1;
  }
  return { x, rank: core.rank + peeledVar.length, coreRows: coreRows.length, coreVars: back.length };
}

function gswStep(cfg, rng, opts = {}) {
  const { nV, nE, nP, beta, mu, h, J, lmbda } = cfg;
  const jRate = 2 * J * (opts.jFactor || 1);
  // ---- cuts and segments
  const vc = new Array(nV), ec = new Array(nE);
  for (let v = 0; v < nV; v++) vc[v] = poissonUnion(cfg.stars[v], mu, beta, rng);
  for (let e = 0; e < nE; e++) ec[e] = poissonUnion(cfg.kinks[e], h, beta, rng);
  const gOff = new Int32Array(nV + 1), sOff = new Int32Array(nE + 1);
  let n = 0;
  for (let v = 0; v < nV; v++) { gOff[v] = n; n += Math.max(1, vc[v].length); } gOff[nV] = n;
  for (let e = 0; e < nE; e++) { sOff[e] = n; n += Math.max(1, ec[e].length); } sOff[nE] = n;
  const tOff = n; n += nV; // twist bits
  const nVar = n;
  const segIdx = (c, off, tau) => { if (c.length === 0) return off; const i = lowerBound(c, tau + 0); // # cuts < tau
    // segment k spans [c[k], c[k+1]); wrap segment (last) spans [c[m-1], beta) U [0, c[0])
    let k = 0; { let lo = 0, hi = c.length; while (lo < hi) { const m = (lo + hi) >> 1; if (c[m] > tau) hi = m; else lo = m + 1; } k = lo === 0 ? c.length - 1 : lo - 1; }
    void i; return off + k; };
  const pre = (c, tau) => c.length > 0 && tau < c[0]; // in the [0, first cut) part of the wrap segment
  // ---- optional block move: only vertices in a spatial ell x ell block (and edges with both ends in it) are redrawn.
  // Outside variables are pinned to the canonical decomposition of the current configuration (g_v flips exactly at
  // star events with g_v(0) = 0, t_v = star parity; s_e = sigma^z_e(0) + kink parity), which depends only on the
  // outside data. Gauge moves at inside vertices map the pinned sets of equivalent configurations onto each other and
  // act freely, so the redraw is uniform over the configurations that agree with the current one outside: exact.
  let pinned = null, cur = null;
  if (opts.block) {
    const { x0, y0, ell } = opts.block, L = cfg.L;
    const inside = v => ((v % L - x0 + L) % L) < ell && ((Math.floor(v / L) - y0 + L) % L) < ell;
    pinned = new Uint8Array(nVar); cur = new Uint8Array(nVar);
    for (let v = 0; v < nV; v++) {
      const c = vc[v], off = gOff[v], st = cfg.stars[v];
      const t = st.length & 1; cur[tOff + v] = t; if (!inside(v)) pinned[tOff + v] = 1;
      if (c.length === 0) { cur[off] = 0; } else for (let k = 0; k < c.length; k++) cur[off + k] = countLE(st, c[k]) & 1;
      for (let k = 0; k < Math.max(1, c.length); k++) if (!inside(v)) pinned[off + k] = 1;
    }
    for (let e = 0; e < nE; e++) {
      const c = ec[e], off = sOff[e], kk = cfg.kinks[e], s0 = cfg.spin0[e] === -1 ? 1 : 0;
      const ins = inside(cfg.src[e]) && inside(cfg.tgt[e]);
      if (c.length === 0) cur[off] = s0; else for (let k = 0; k < c.length; k++) cur[off + k] = s0 ^ (countLE(kk, c[k]) & 1);
      if (!ins) {
        for (let k = 0; k < Math.max(1, c.length); k++) pinned[off + k] = 1;
        // a pinned edge also pins the twists of both its ends: a twist change would move a kink on the pinned edge
        pinned[tOff + cfg.src[e]] = 1; pinned[tOff + cfg.tgt[e]] = 1;
      }
    }
  }
  // ---- equations as sparse lists of variable indices (XOR) with right-hand side (non-zero only from pinned variables)
  const rows = [], rhsArr = [];
  const seen = new Map(); // identical equations (e.g. several J bonds on the same four segments) are kept once
  const pushRow = vars => { const m = new Map(); for (const x of vars) m.set(x, (m.get(x) || 0) ^ 1); const r = []; let b0 = 0;
    for (const [x, b] of m) if (b) { if (pinned && pinned[x]) b0 ^= cur[x]; else r.push(x); }
    if (r.length === 0) { if (b0) throw new Error('gswStep: pinned variables violate a bond'); return; }
    r.sort((a, b) => a - b); const key = r.join(',');
    if (seen.has(key)) { if (seen.get(key) !== b0) throw new Error('gswStep: inconsistent duplicate equation'); return; }
    seen.set(key, b0); rows.push(r); rhsArr.push(b0); };
  // lmbda bonds on sigma^z = +1 stretches
  for (let e = 0; e < nE; e++) {
    const u = cfg.src[e], w = cfg.tgt[e];
    const fl = mergeSorted(cfg.kinks[e], cfg.stars[u], cfg.stars[w]);
    let s = cfg.spin0[e], t0 = 0;
    for (let i = 0; i <= fl.length; i++) {
      const t1 = i < fl.length ? fl[i] : beta;
      if (s === 1) { let t = t0 + expRand(rng, 2 * lmbda);
        while (t < t1) {
          const vars = [segIdx(ec[e], sOff[e], t), segIdx(vc[u], gOff[u], t), segIdx(vc[w], gOff[w], t)];
          if (pre(ec[e], t)) vars.push(tOff + u, tOff + w);
          if (pre(vc[u], t)) vars.push(tOff + u);
          if (pre(vc[w], t)) vars.push(tOff + w);
          pushRow(vars); t += expRand(rng, 2 * lmbda);
        } }
      s = -s; t0 = t1;
    }
  }
  // J bonds on B_p = +1 stretches (B_p flips only at kinks on its edges)
  for (let p = 0; p < nP; p++) {
    const pe = cfg.plaq[p];
    const fl = mergeSorted(cfg.kinks[pe[0]], cfg.kinks[pe[1]], cfg.kinks[pe[2]], cfg.kinks[pe[3]]);
    let b = 1; for (const e of pe) b *= cfg.spin0[e];
    let t0 = 0;
    for (let i = 0; i <= fl.length; i++) {
      const t1 = i < fl.length ? fl[i] : beta;
      if (b === 1) { let t = t0 + expRand(rng, jRate);
        while (t < t1) {
          const vars = [];
          for (const e of pe) { vars.push(segIdx(ec[e], sOff[e], t)); if (pre(ec[e], t)) vars.push(tOff + cfg.src[e], tOff + cfg.tgt[e]); }
          pushRow(vars); t += expRand(rng, jRate);
        } }
      b = -b; t0 = t1;
    }
  }
  // twists need a cut: vertex without cuts -> t_v = 0; edge without cuts -> t_u + t_w = 0
  for (let v = 0; v < nV; v++) if (vc[v].length === 0) pushRow([tOff + v]);
  for (let e = 0; e < nE; e++) if (ec[e].length === 0) pushRow([tOff + cfg.src[e], tOff + cfg.tgt[e]]);
  const nr = rows.length;
  if (opts.returnSystem) return { rows, rhs: Uint8Array.from(rhsArr), nVar };
  const rhsU = Uint8Array.from(rhsArr);
  const sol = opts.dense ? solveDense(rows, nVar, rng, rhsU) : solvePeel(rows, nVar, rng, rhsU);
  const x = sol.x, rank = sol.rank;
  if (pinned) for (let i = 0; i < nVar; i++) if (pinned[i]) x[i] = cur[i];
  // ---- rebuild configuration
  const oldP = opts.measureChange ? snapshotFlux(cfg) : null;
  const tw = v => x[tOff + v];
  for (let v = 0; v < nV; v++) {
    const c = vc[v], off = gOff[v], m = c.length; const ns = [];
    if (m > 0) { const wrap = x[off + m - 1];
      for (let k = 0; k < m; k++) { const before = k === 0 ? (wrap ^ tw(v)) : x[off + k - 1]; if (before !== x[off + k]) ns.push(c[k]); } }
    cfg.stars[v] = ns;
  }
  const g0 = v => { const m = vc[v].length; return m === 0 ? x[gOff[v]] : (x[gOff[v] + m - 1] ^ tw(v)); };
  for (let e = 0; e < nE; e++) {
    const c = ec[e], off = sOff[e], m = c.length, te = tw(cfg.src[e]) ^ tw(cfg.tgt[e]); const nk = [];
    let s0;
    if (m > 0) { const wrap = x[off + m - 1]; s0 = wrap ^ te;
      for (let k = 0; k < m; k++) { const before = k === 0 ? s0 : x[off + k - 1]; if (before !== x[off + k]) nk.push(c[k]); } }
    else s0 = x[off];
    cfg.kinks[e] = nk;
    cfg.spin0[e] = (s0 ^ g0(cfg.src[e]) ^ g0(cfg.tgt[e])) ? -1 : 1;
  }
  const out = { nVar, nRows: nr, rank, free: nVar - rank, coreRows: sol.coreRows, coreVars: sol.coreVars };
  if (opts.measureChange) out.fluxChange = fluxChange(cfg, oldP);
  return out;
}

// fraction of plaquette-time volume where B_p differs between two configurations
function snapshotFlux(cfg) { const res = []; for (let p = 0; p < cfg.nP; p++) { const pe = cfg.plaq[p];
  let b = 1; for (const e of pe) b *= cfg.spin0[e]; res.push({ b, ev: mergeSorted(...pe.map(e => cfg.kinks[e])) }); } return res; }
function fluxChange(cfg, oldP) {
  const now = snapshotFlux(cfg); let diff = 0;
  for (let p = 0; p < cfg.nP; p++) {
    const A = oldP[p], Bp = now[p]; const pts = mergeSorted(A.ev, Bp.ev); let t0 = 0;
    for (let i = 0; i <= pts.length; i++) { const t1 = i < pts.length ? pts[i] : cfg.beta; const tm = 0.5 * (t0 + t1);
      const ba = A.b * ((lowerBound(A.ev, tm) & 1) ? -1 : 1), bb = Bp.b * ((lowerBound(Bp.ev, tm) & 1) ? -1 : 1);
      if (ba !== bb) diff += t1 - t0; t0 = t1; }
  }
  return diff / (cfg.nP * cfg.beta);
}

module.exports = { gswStep };
