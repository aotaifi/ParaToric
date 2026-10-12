'use strict';
// Read the observable time series out of a paratoric etc_sample checkpoint (ckpt.bin) without stopping the run.
// Format (checkpoint_detail::write_observable_series): u64 nObs, then per observable u64 n and n x (u8 tag, f64 value)
// (tag 1 = complex: two f64). The block sits after a header of unknown length, so it is located by scanning for
// u64 nObs followed by nObs equal-length real series.
// usage: node ckptseries.js nObs ckpt.bin [out.txt]   (out: one line per sample, observables in --observables order)
const fs = require('fs');
const [nObsS, file, out] = process.argv.slice(2); const nObs = BigInt(+nObsS);
const b = fs.readFileSync(file);
function tryAt(o) {
  if (o + 16 > b.length || b.readBigUInt64LE(o) !== nObs) return null;
  let p = o + 8; const cols = [];
  for (let k = 0; k < Number(nObs); k++) {
    if (p + 8 > b.length) return null;
    const n = b.readBigUInt64LE(p); p += 8;
    if (n > 100000000n || p + Number(n) * 9 > b.length) return null;
    if (k > 0 && Number(n) !== cols[0].length) return null;
    const c = new Float64Array(Number(n));
    for (let i = 0; i < c.length; i++) { if (b[p] !== 0) return null; c[i] = b.readDoubleLE(p + 1); p += 9; }
    cols.push(c);
  }
  return cols;
}
let found = null, at = -1;
for (let o = 0; o + 16 <= b.length; o++) { const r = tryAt(o); if (r && r[0].length > 0) { if (found) { console.error(`ambiguous: blocks at ${at} and ${o}`); process.exit(2); } found = r; at = o; } }
if (!found) { console.error('no series block found'); process.exit(1); }
console.error(`series block at byte ${at}: ${found.length} observables x ${found[0].length} samples`);
if (out) {
  const lines = []; for (let i = 0; i < found[0].length; i++) lines.push(found.map(c => c[i].toPrecision(17)).join(' '));
  fs.writeFileSync(out, lines.join('\n') + '\n');
}
