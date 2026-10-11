'use strict';
// Core-size test for the generalised-SW linear system (scaling step-back, 2026-10-11).
// usage: node reduce.js L nWarm seed
// Equilibrates with global gsw steps, then builds one system and reduces it to a fixed point with
//   (a) leaf peeling        : a variable in exactly one equation is eliminated with that equation (reduction)
//   (b) unit propagation    : an equation with one variable fixes it; substitute everywhere (coreduction)
//   (c) parity union-find   : an equation x + y = b merges y into x (y := x + b everywhere)
// and reports the irreducible core (equations, variables) as a fraction of the original. If the core fraction falls
// with L, exact global redraws can scale; if it stays ~constant, they cost ~N^2 per step.
const { Config, RNG } = require('./qmc.js'); const { gswStep } = require('./gsw.js');
const [L, nWarm, seed] = process.argv.slice(2).map(Number);
const cfg = new Config(L, L, { mu: 1, J: 1, h: 0.3391, lmbda: 0.3391 }), rng = new RNG(seed);
for (let i = 0; i < nWarm; i++) gswStep(cfg, () => rng.uniform(), {});
const sys = gswStep(cfg, () => rng.uniform(), { returnSystem: true });
const nVar = sys.nVar;
// rows as Sets for easy substitution
let rows = sys.rows.map(r => new Set(r)); const rhs = Array.from(sys.rhs);
const alive = rows.map(() => true);
const occ = Array.from({ length: nVar }, () => new Set());
rows.forEach((r, i) => r.forEach(v => occ[v].add(i)));
const removeVarFromRow = (i, v) => { rows[i].delete(v); occ[v].delete(i); };
const addVarToRow = (i, v) => { if (rows[i].has(v)) removeVarFromRow(i, v); else { rows[i].add(v); occ[v].add(i); } };
const killRow = i => { alive[i] = false; for (const v of rows[i]) occ[v].delete(i); rows[i].clear(); };
let changed = true, stats = { peel: 0, unit: 0, merge: 0 };
while (changed) {
  changed = false;
  for (let i = 0; i < rows.length; i++) {
    if (!alive[i]) continue;
    const r = rows[i];
    if (r.size === 0) { killRow(i); changed = true; continue; }
    if (r.size === 1) { // unit: v = rhs; substitute into other rows (move value into their rhs)
      const v = r.values().next().value, val = rhs[i];
      killRow(i);
      for (const j of Array.from(occ[v])) { removeVarFromRow(j, v); rhs[j] ^= val; }
      stats.unit++; changed = true; continue;
    }
    if (r.size === 2) { // merge: y = x + b
      const [x, y] = Array.from(r); const b = rhs[i];
      killRow(i);
      for (const j of Array.from(occ[y])) { removeVarFromRow(j, y); addVarToRow(j, x); rhs[j] ^= b; }
      stats.merge++; changed = true; continue;
    }
  }
  for (let v = 0; v < nVar; v++) { // leaf peeling
    if (occ[v].size === 1) { const i = occ[v].values().next().value; killRow(i); stats.peel++; changed = true; }
  }
}
// dedupe the remaining rows
const keys = new Set(); let coreRows = 0; const coreVars = new Set();
rows.forEach((r, i) => { if (!alive[i]) return; const k = Array.from(r).sort((a, b) => a - b).join(','); if (keys.has(k)) return; keys.add(k); coreRows++; r.forEach(v => coreVars.add(v)); });
console.log(`L=${L} nVar ${nVar} rows ${sys.rows.length} | core rows ${coreRows} (${(100 * coreRows / sys.rows.length).toFixed(1)}%) core vars ${coreVars.size} (${(100 * coreVars.size / nVar).toFixed(1)}%) | peel ${stats.peel} unit ${stats.unit} merge ${stats.merge}`);
