'use strict';
// Stage 0': Edwards-Sokal decomposition of the Higgs (lmbda sigma^z) term on equilibrium snapshots, and the
// consistency probability of time-like vison loops once the electric sector may be repaired by cluster flips.
//
// usage: node es.js CKPT L BETA LMBDA MU NOMEGA SEED [selftest]
//
// Construction (continuous-time Swendsen-Wang for the gauge-parity field, Rieger-Kawashima style):
//  - cut points on each vertex timeline = star events at that vertex  U  Poisson(MU) points;
//    nodes = cyclic segments between consecutive cuts (one segment = whole circle if no cut);
//  - bond points on each edge at Poisson rate 2*LMBDA, kept only where sigma^z(e,tau) = +1;
//    a bond point at tau joins the segments of both endpoints that contain tau.
// A vison loop flips sigma^z on a membrane M. With the bonds fixed, the move is consistent iff the set F of
// bond points inside M is a cut of the bond graph (some flip pattern of whole segments, i.e. of star events,
// re-satisfies every bond). Checked by BFS 2-colouring of the clusters touched by F.
// Frozen-electric baseline: the move is allowed only if F is empty.
const { parseCheckpoint, geometry, edgeIntegral } = require('./ckpt.js');

function mulberry32(a) { return function () { a |= 0; a = a + 0x6D2B79F5 | 0; let t = Math.imul(a ^ a >>> 15, 1 | a);
  t = t + Math.imul(t ^ t >>> 7, 61 | t) ^ t; return ((t ^ t >>> 14) >>> 0) / 4294967296; }; }

function buildBonds(c, G, beta, lmbda, mu, rnd) {
  const L = c.L;
  // segments per vertex
  const segStart = [], segOff = new Int32Array(c.nV + 1), cutsV = [];
  let nNodes = 0;
  for (let v = 0; v < c.nV; v++) {
    const cuts = Array.from(c.star[v]);
    let t = -Math.log(1 - rnd()) / mu;
    while (t < beta) { cuts.push(t); t += -Math.log(1 - rnd()) / mu; }
    cuts.sort((a, b) => a - b);
    cutsV.push(cuts);
    segOff[v] = nNodes; nNodes += Math.max(1, cuts.length);
  }
  segOff[c.nV] = nNodes;
  // segment index of vertex v containing tau: segment k spans [cuts[k], cuts[k+1]); the last one wraps to cuts[0]
  const segOf = (v, tau) => {
    const cuts = cutsV[v]; if (cuts.length === 0) return segOff[v];
    let lo = 0, hi = cuts.length; // first index with cuts[i] > tau
    while (lo < hi) { const m = (lo + hi) >> 1; if (cuts[m] > tau) hi = m; else lo = m + 1; }
    const k = lo === 0 ? cuts.length - 1 : lo - 1;
    return segOff[v] + k;
  };
  // bond points
  const bE = [], bT = [], bA = [], bB = [];
  const bondsOnEdge = [];
  for (let e = 0; e < c.nE; e++) {
    const fl = c.flips[e]; const list = [];
    let s = c.spin[e], t0 = 0;
    for (let i = 0; i <= fl.length; i++) {
      const t1 = i < fl.length ? fl[i] : beta;
      if (s === 1) {
        let t = t0 - Math.log(1 - rnd()) / (2 * lmbda);
        while (t < t1) {
          const id = bE.length; bE.push(e); bT.push(t); bA.push(segOf(G.src[e], t)); bB.push(segOf(G.tgt[e], t)); list.push(id);
          t += -Math.log(1 - rnd()) / (2 * lmbda);
        }
      }
      s = -s; t0 = t1;
    }
    bondsOnEdge.push(list); // sorted by time
  }
  // adjacency (CSR)
  const nB = bE.length, deg = new Int32Array(nNodes + 1);
  for (let b = 0; b < nB; b++) { deg[bA[b] + 1]++; deg[bB[b] + 1]++; }
  for (let i = 0; i < nNodes; i++) deg[i + 1] += deg[i];
  const adjN = new Int32Array(2 * nB), adjB = new Int32Array(2 * nB), fill = deg.slice(0, nNodes);
  for (let b = 0; b < nB; b++) { adjN[fill[bA[b]]] = bB[b]; adjB[fill[bA[b]]++] = b; adjN[fill[bB[b]]] = bA[b]; adjB[fill[bB[b]]++] = b; }
  // segment lengths (for space-time volume)
  const segLen = new Float64Array(nNodes);
  for (let v = 0; v < c.nV; v++) { const cuts = cutsV[v];
    if (cuts.length === 0) { segLen[segOff[v]] = beta; continue; }
    for (let k = 0; k < cuts.length; k++) segLen[segOff[v] + k] = (k + 1 < cuts.length ? cuts[k + 1] : cuts[0] + beta) - cuts[k]; }
  return { nNodes, nB, bE, bT, bA, bB, bondsOnEdge, deg, adjN, adjB, segLen, segOff, cutsV };
}

// clusters with winding detection (weighted union-find on displacement (dx, dy, dt))
function clusters(c, G, beta, B) {
  const L = c.L, n = B.nNodes;
  const comp = new Int32Array(n).fill(-1), dx = new Float64Array(n), dy = new Float64Array(n), dt = new Float64Array(n);
  const vOfNode = new Int32Array(n), startOf = new Float64Array(n);
  for (let v = 0; v < c.nV; v++) for (let k = B.segOff[v]; k < B.segOff[v + 1]; k++) {
    vOfNode[k] = v; const cuts = B.cutsV[v]; startOf[k] = cuts.length ? cuts[k - B.segOff[v]] : 0; }
  const stats = []; const queue = new Int32Array(n);
  for (let s = 0; s < n; s++) {
    if (comp[s] >= 0) continue;
    const id = stats.length; let head = 0, tail = 0; queue[tail++] = s; comp[s] = id; dx[s] = dy[s] = dt[s] = 0;
    let nodes = 0, vol = 0, bonds2 = 0, wx = false, wy = false, wt = false;
    while (head < tail) {
      const u = queue[head++]; nodes++; vol += B.segLen[u];
      for (let j = B.deg[u]; j < B.deg[u + 1]; j++) {
        const w = B.adjN[j], b = B.adjB[j]; bonds2++;
        // displacement u -> w through bond b
        const e = B.bE[b], tau = B.bT[b];
        const vu = vOfNode[u], vw = vOfNode[w];
        let ddx = (vw % L) - (vu % L), ddy = Math.floor(vw / L) - Math.floor(vu / L);
        if (ddx > 1) ddx -= L; if (ddx < -1) ddx += L; if (ddy > 1) ddy -= L; if (ddy < -1) ddy += L;
        const posU = ((tau - startOf[u]) % beta + beta) % beta, posW = ((tau - startOf[w]) % beta + beta) % beta;
        const ddt = (startOf[w] + posW) - (startOf[u] + posU) + (posU - posW) * 0; // reference: segment start
        // time displacement between segment references measured along the bond: (start_w - start_u) adjusted so
        // that the bond instant sits at the same unwrapped time in both
        const tU = dt[u] + posU; const tWref = tU - posW;
        if (comp[w] < 0) { comp[w] = id; dx[w] = dx[u] + ddx; dy[w] = dy[u] + ddy; dt[w] = tWref; queue[tail++] = w; }
        else {
          if (Math.abs(dx[w] - dx[u] - ddx) > 0.5) wx = true;
          if (Math.abs(dy[w] - dy[u] - ddy) > 0.5) wy = true;
          if (Math.abs(dt[w] - tWref) > beta / 2) wt = true;
        }
        void ddt;
      }
    }
    stats.push({ nodes, vol, bonds: bonds2 / 2, cyc: bonds2 / 2 - nodes + 1, wx, wy, wt });
  }
  return { comp, stats };
}

// is F (array of bond ids) a cut? BFS 2-colouring of every cluster touched by F
function isCut(B, Fset, Flist, comp, mark, col, stamp) {
  for (const b0 of Flist) {
    const s = B.bA[b0]; if (mark[s] === stamp) continue;
    const q = [s]; mark[s] = stamp; col[s] = 0;
    for (let h = 0; h < q.length; h++) {
      const u = q[h];
      for (let j = B.deg[u]; j < B.deg[u + 1]; j++) {
        const w = B.adjN[j], par = Fset.has(B.adjB[j]) ? 1 : 0;
        if (mark[w] !== stamp) { mark[w] = stamp; col[w] = col[u] ^ par; q.push(w); }
        else if ((col[w] ^ col[u]) !== par) return false;
      }
    }
  }
  return true;
}

function bondsInWindow(B, e, ta, T, beta) {
  const out = [], lst = B.bondsOnEdge[e];
  for (const b of lst) { const d = ((B.bT[b] - ta) % beta + beta) % beta; if (d < T) out.push(b); }
  return out;
}

function main() {
  const [path, Ls, betas, lms, mus, nos, seeds, mode] = process.argv.slice(2);
  const L = +Ls, beta = +betas, lmbda = +lms, mu = +mus, nOmega = +nos, rnd = mulberry32(+seeds);
  const c = parseCheckpoint(path, L); const G = geometry(L);
  const Rs = [1, 2, 3, 4, 6, 8, 12].filter(r => r <= L / 2), Ts = [0.25, 0.5, 1, 2, 4, 8, 12].filter(t => t <= beta / 2);
  const nTrial = 200;
  const acc = {}, frozen = {}, fsize = {};
  const agg = { nodes: 0, bonds: 0, maxFrac: 0, maxVolFrac: 0, wind: 0, cyc: 0, cycInMax: 0, comps: 0 };
  let selftestReport = '';
  for (let o = 0; o < nOmega; o++) {
    const B = buildBonds(c, G, beta, lmbda, mu, rnd);
    const C = clusters(c, G, beta, B);
    let maxN = 0, maxV = 0, maxIdx = 0, totV = 0, cyc = 0, wind = 0;
    C.stats.forEach((s, i) => { totV += s.vol; cyc += s.cyc; if (s.nodes > maxN) { maxN = s.nodes; maxIdx = i; } maxV = Math.max(maxV, s.vol); if (s.wx || s.wy) wind++; });
    agg.nodes += B.nNodes; agg.bonds += B.nB; agg.maxFrac += maxN / B.nNodes; agg.maxVolFrac += maxV / totV; agg.wind += wind; agg.cyc += cyc; agg.cycInMax += C.stats[maxIdx].cyc; agg.comps += C.stats.length;
    const mark = new Int32Array(B.nNodes), col = new Uint8Array(B.nNodes); let stamp = 0;
    if (mode === 'selftest' && o === 0) {
      // (a) a single bond lying on a cycle is never a cut; (b) the boundary of a random node set always is
      let a = 0, aN = 0, bb = 0, bN = 0;
      // {b} is a cut iff b is a bridge; ground truth by BFS from one end with b removed
      for (let b = 0; b < B.nB && aN < 300; b++) { const cs = C.stats[C.comp[B.bA[b]]]; if (cs.cyc === 0) continue;
        aN++;
        const seen = new Uint8Array(B.nNodes), q = [B.bA[b]]; seen[B.bA[b]] = 1;
        for (let h = 0; h < q.length; h++) { const u = q[h]; for (let j = B.deg[u]; j < B.deg[u + 1]; j++) {
          if (B.adjB[j] === b) continue; const w = B.adjN[j]; if (!seen[w]) { seen[w] = 1; q.push(w); } } }
        const bridge = !seen[B.bB[b]];
        if (bridge === isCut(B, new Set([b]), [b], C.comp, mark, col, ++stamp)) a++; }
      for (let k = 0; k < 300; k++) { const inS = new Uint8Array(B.nNodes); const root = Math.floor(rnd() * B.nNodes);
        // S = a BFS ball of random radius around root
        const rad = 1 + Math.floor(rnd() * 6); let fr = [root]; inS[root] = 1;
        for (let r = 0; r < rad; r++) { const nf = []; for (const u of fr) for (let j = B.deg[u]; j < B.deg[u + 1]; j++) { const w = B.adjN[j]; if (!inS[w] && rnd() < 0.7) { inS[w] = 1; nf.push(w); } } fr = nf; }
        const F = []; for (let b = 0; b < B.nB; b++) if (inS[B.bA[b]] !== inS[B.bB[b]]) F.push(b);
        bN++; if (F.length === 0 || isCut(B, new Set(F), F, C.comp, mark, col, ++stamp)) bb++; }
      // (c) Poisson check: bond density per edge per unit tau vs 2*lmbda*P(sigma^z=+1)
      let sumZ = 0; for (let e = 0; e < c.nE; e++) sumZ += edgeIntegral(c.spin[e], c.flips[e], beta);
      const pPlus = (1 + sumZ / (beta * c.nE)) / 2;
      selftestReport = `SELFTEST single bonds in cyclic clusters: cut test agrees with brute-force bridge test ${a}/${aN} (want all); random node-set boundaries flagged cut: ${bb}/${bN} (want all); ` +
        `bond density ${(B.nB / (c.nE * beta)).toFixed(4)} vs 2*lmbda*P(+)=${(2 * lmbda * pPlus).toFixed(4)}`;
    }
    for (const R of Rs) for (const T of Ts) {
      const key = `${R},${T}`; acc[key] ??= 0; frozen[key] ??= 0; fsize[key] ??= 0;
      for (let k = 0; k < nTrial; k++) {
        const x0 = Math.floor(rnd() * L), y0 = Math.floor(rnd() * L), ta = rnd() * beta, dir = rnd() < 0.5 ? 0 : 1;
        const F = [];
        for (let j = 1; j <= R; j++) {
          // path of plaquettes along x (crossing y-edges) or along y (crossing x-edges)
          const e = dir === 0 ? G.eid(x0 + j, y0, 1) : G.eid(x0, y0 + j, 0);
          for (const b of bondsInWindow(B, e, ta, T, beta)) F.push(b);
        }
        fsize[key] += F.length;
        if (F.length === 0) { frozen[key]++; acc[key]++; continue; }
        if (isCut(B, new Set(F), F, C.comp, mark, col, ++stamp)) acc[key]++;
      }
    }
  }
  const N = nOmega * nTrial;
  const lines = [];
  if (selftestReport) lines.push(selftestReport);
  lines.push(`clusters (mean over ${nOmega} bond samples): nodes ${(agg.nodes / nOmega).toFixed(0)}, bonds ${(agg.bonds / nOmega).toFixed(0)}, mean degree ${(2 * agg.bonds / agg.nodes).toFixed(3)}, ` +
    `components ${(agg.comps / nOmega).toFixed(0)}, largest cluster node fraction ${(agg.maxFrac / nOmega).toFixed(4)}, vol fraction ${(agg.maxVolFrac / nOmega).toFixed(4)}, ` +
    `spatially winding clusters ${(agg.wind / nOmega).toFixed(2)}, independent cycles total ${(agg.cyc / nOmega).toFixed(1)} (in largest ${(agg.cycInMax / nOmega).toFixed(1)})`);
  lines.push('R T  P_consistent(ES)  P_noBond(frozen)  <|F|>   [N=' + N + ' per cell]');
  for (const R of Rs) for (const T of Ts) { const key = `${R},${T}`;
    lines.push(`${R} ${T}  ${(acc[key] / N).toFixed(4)}  ${(frozen[key] / N).toFixed(4)}  ${(fsize[key] / N).toFixed(2)}`); }
  console.log(lines.join('\n'));
}
main();
