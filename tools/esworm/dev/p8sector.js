// Sector-resolved reference for h=0: with no kinks every vertex has the same star-event parity, and the
// algorithm (no combination move at h=0) stays in the sector it starts in (all even).
// Z_even = (Z + Z_twist)/2, Z_twist = Tr exp(-beta H) with mu_0 -> -mu_0 (weight (-1)^{n_0}).
// Observables from derivatives of ln Z_even (central differences).
const ED = require('../ed/ed2x2.js');
const mask = (g) => g.reduce((m, e) => m | (1 << e), 0);
const sm0 = mask(ED.stars[0]);
function lnZeven(beta, mu, J, lm, twistMu) {
  const Z = (tw) => {
    const H = ED.buildH(mu, J, 0, lm);
    if (tw) for (let s = 0; s < ED.N; s++) H[s ^ sm0][s] += 2 * mu; // flips sign of vertex-0 star term
    const E = ED.jacobi(H).E, E0 = Math.min(...E);
    let z = 0; for (const e of E) z += Math.exp(-beta * (e - E0));
    return { z, E0 };
  };
  const a = Z(false), b = Z(true);
  // combine with common reference energy
  const E0 = Math.min(a.E0, b.E0);
  return Math.log((a.z * Math.exp(-beta * (a.E0 - E0)) + b.z * Math.exp(-beta * (b.E0 - E0))) / 2) - beta * E0;
}
function point(mu, J, lm, beta) {
  const d = 1e-4;
  const f = (b, m, j, l) => lnZeven(b, m, j, l);
  const dJ = (f(beta, mu, J + d, lm) - f(beta, mu, J - d, lm)) / (2 * d);
  const dL = (f(beta, mu, J, lm + d) - f(beta, mu, J, lm - d)) / (2 * d);
  const dM = (f(beta, mu + d, J, lm) - f(beta, mu - d, J, lm)) / (2 * d);
  const dB = (f(beta + d, mu, J, lm) - f(beta - d, mu, J, lm)) / (2 * d);
  return { B: dJ / (4 * beta), sz: dL / (8 * beta), A: mu * dM / (4 * beta * mu), H: -dB };
}
const ref = require('../ed/reference.json').P8;
const t0 = Date.now();
const r = point(ref.mu, ref.J, ref.lmbda, ref.beta);
require('fs').writeFileSync(__dirname + '/../ref_p8_even.json', JSON.stringify({ mu: ref.mu, J: ref.J, h: 0, lmbda: ref.lmbda, beta: ref.beta, A: r.A, B: r.B, sigma_x: 0, sigma_z: r.sz, H: r.H,
  note: 'P8 (h=0) restricted to the all-even star-parity sector, which the local updates cannot leave at h=0; derived from ED via the vertex-0 twist trick (dev/p8sector.js)' }, null, 1));
console.log('P8 even-sector reference:', JSON.stringify(r), ' (full ED:', ref.B, ref.A, ref.sigma_z, ref.H, ')  [' + ((Date.now() - t0) / 1000).toFixed(1) + ' s]');
