'use strict';
// Read-only parser for the lattice section of a ParaToric sample checkpoint (Boost Lattice backend,
// square, periodic, z basis), written by Lattice::write_checkpoint_state (src/lattice/lattice.cpp).
// The header (observable series, RNG text state) is skipped by locating the lattice signature
// (uint64 vertex, edge, plaquette counts); the parse must then end exactly at EOF.
// Geometry follows init_lattice_graph("square", periodic): edges inserted for x outer, y inner,
// first (v, right) then (v, down), v = y*L + x; boost::edges() on an undirected vecS graph
// returns insertion order. This is checked against the data in validate.js, not assumed.
const fs = require('fs');

function parseCheckpoint(path, L) {
  const buf = fs.readFileSync(path);
  const nV = L * L, nE = 2 * L * L, nP = L * L;
  const sig = Buffer.alloc(24);
  sig.writeBigUInt64LE(BigInt(nV), 0); sig.writeBigUInt64LE(BigInt(nE), 8); sig.writeBigUInt64LE(BigInt(nP), 16);
  const start = buf.lastIndexOf(sig);
  if (start < 0) throw new Error('lattice signature not found');
  let off = start + 24;
  const u64 = () => { const v = Number(buf.readBigUInt64LE(off)); off += 8; return v; };
  const i32 = () => { const v = buf.readInt32LE(off); off += 4; return v; };
  const f64 = () => { const v = buf.readDoubleLE(off); off += 8; return v; };
  const dvec = () => { const n = u64(); const a = new Float64Array(n); for (let i = 0; i < n; i++) a[i] = f64(); return a; };
  const star = [], starE = [];
  for (let v = 0; v < nV; v++) { star.push(dvec()); starE.push(f64()); }
  const spin = new Int32Array(nE), edgeE = new Float64Array(nE), flips = [], kinks = [];
  for (let e = 0; e < nE; e++) { spin[e] = i32(); edgeE[e] = f64(); flips.push(dvec()); kinks.push(dvec()); }
  const plaqFlips = []; for (let p = 0; p < nP; p++) plaqFlips.push(dvec());
  const plaqE = new Float64Array(nP); for (let p = 0; p < nP; p++) plaqE[p] = f64();
  const caches = {
    diag_single: i32(), diag_tuple_x: i32(), diag_tuple_z: i32(), anyon: i32(),
    n_single: u64(), n_plaq: u64(), n_star: u64(),
    tot_edgeE: f64(), tot_starE: f64(), tot_plaqE: f64(),
  };
  return { L, nV, nE, nP, star, starE, spin, edgeE, flips, kinks, plaqFlips, plaqE, caches,
           bytesLeft: buf.length - off, headerBytes: start };
}

// Geometry (square, periodic). Vertex v = y*L + x. Edge index 2*(x*L+y) + d, d=0: x-edge (v, v+x), d=1: y-edge (v, v+y).
function geometry(L, variant = 'insertion') {
  const nE = 2 * L * L;
  const src = new Int32Array(nE), tgt = new Int32Array(nE);
  const eid = (x, y, d) => variant === 'insertion' ? 2 * (((x % L + L) % L) * L + ((y % L + L) % L)) + d
                                                  : 2 * (((y % L + L) % L) * L + ((x % L + L) % L)) + d; // deliberately wrong order (negative test)
  for (let x = 0; x < L; x++) for (let y = 0; y < L; y++) {
    const v = y * L + x;
    src[eid(x, y, 0)] = v; tgt[eid(x, y, 0)] = y * L + (x + 1) % L;
    src[eid(x, y, 1)] = v; tgt[eid(x, y, 1)] = ((y + 1) % L) * L + x;
  }
  // plaquette with lower-left corner (x,y): edges x(x,y), x(x,y+1), y(x,y), y(x+1,y)
  const plaq = [];
  for (let y = 0; y < L; y++) for (let x = 0; x < L; x++)
    plaq.push([eid(x, y, 0), eid(x, y + 1, 0), eid(x, y, 1), eid(x + 1, y, 1)]);
  return { src, tgt, plaq, eid };
}

// sigma^z integral of one edge over [0, beta): spin at tau=0 flips at each entry of flips (sorted).
function edgeIntegral(spin0, fl, beta) {
  let s = spin0, t = 0, acc = 0;
  for (let i = 0; i < fl.length; i++) { acc += s * (fl[i] - t); t = fl[i]; s = -s; }
  return acc + s * (beta - t);
}

module.exports = { parseCheckpoint, geometry, edgeIntegral };
