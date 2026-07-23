# Failure-mode analysis — why the same class of error keeps surviving review

Date: 2026-07-21. Author: Feynman, at the PI's request.
Question asked: *"is it miscommunication between workers? some weird statistical
errors that don't get caught beforehand and need two reviews. What is the root —
bad model, bad reasoning?"*

Short answer: **none of those three.** It is structural, it is predictable, and it
is fixable without adding a single reviewer. Evidence and reasoning below.

--------------------------------------------------------------------------------
## 1. The record, classified

Every significant error or catch in this campaign, sorted by TYPE not by blame.

**A. Real code bugs — CAUGHT WELL, no action needed**
| # | Error | Found by | Cost |
|---|---|---|---|
| A1 | PT swap partner chosen by MPI rank while ladder labels migrate | Jarvis, independently | weeks of failed ladders |
| A2 | Per-pair `MPI_Bcast` on the world communicator, undefined at N>=4 | Jarvis/Clark | blocked N>=4 |

Code review works. These were found by reading code against intent. Keep doing it.

**B. Statistics/meaning errors — THE PROBLEM CLASS**
| # | Error | Survived | Found by |
|---|---|---|---|
| B1 | Plotted `U=1-r/3` vs Linsel's raw ratio `r` — a CONVENTION mismatch | multiple reviews, a "wrong channel" scare | comparison to the SOURCE figure |
| B2 | Assumed stored errors were naive when they were stationary-bootstrap | a full hypothesis + report cycle | reading the code that makes the error |
| B3 | Feynman read a blocking curve at 8-16 blocks (too few to read a plateau) | two of my own UPDATEs | between-chain error, one command |
| B4 | **L=10 is not ergodic at low h; 8 chains sit in 3 different states** | **~15 handlings of that dataset, 27 review invocations** | **printing the per-chain table, one command** |
| B5 | "Round-trips necessary for low autocorrelation" written into run manual F3/F7 AND harness gate G1.2 | 4 ladder attempts scored NO-GO on it | PI, from physics |

**C. Reasoning-from-assumption — Feynman's, caught by others**
frozen-base prediction; s* mis-derived four times by geometry; winding-sector
ergodicity; the circular z=2 calibration (Jarvis caught, saved ~3,919 task-hours);
"benchmark-line z sets the M2 budget" (PI caught); "PT delivers a speedup" (PI
caught).

**D. Process/tooling**
transcription error 0.0000817 vs 0.0008170 (Jarvis, self-caught and self-reported);
bundling submit+wait+analyze into one dispatch, orphaning background workers
(Jarvis, self-diagnosed).

--------------------------------------------------------------------------------
## 2. The root cause

Look at what class B has in common. Every one of them lives in the layer BETWEEN
the raw data and the summary number, and every review looked at the summary number.

The analysis pipeline is:

```
raw series  ->  per-chain statistics  ->  POOLED statistics  ->  derived quantity  ->  verdict
                                          ^^^^^^^^^^^^^^^^^^
                                          errors live here
                                                                  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                                                                  reviews look here
```

**Aggregation is irreversible.** Once eight chains become `mean +/- error`, the
information that they disagreed is *gone from the artifact*. A reviewer downstream
cannot recover it — not because they are careless, but because it is not there.

That is why B4 survived 27 review invocations. It is not a competence result, it is
an information-theoretic one. **Two reviews of a pooled number cannot find a pooling
error. Two hundred could not.** Adding reviewers to this class of problem has a
mathematically guaranteed return of zero, which is exactly what we have observed.

The same shape explains the rest of class B:
- B1: the artifact (a smooth monotonic curve) looked *correct*. Only the SOURCE
  convention it was supposed to match revealed the mismatch.
- B2: the artifact (z-values) looked fine. Only the code that produced the error bar
  showed the assumption was wrong.
- B3: the artifact (a rising blocking curve) looked like a real slow mode. Only the
  block COUNT behind each point showed it was noise.
- B5: the artifact was a *rule*, written once and thereafter cited rather than
  re-derived. No data was ever behind it.

Generalized: **we review artifacts, not the transformations that produced them.**

--------------------------------------------------------------------------------
## 3. Answering the PI's three candidate causes, directly

**"Miscommunication between workers?" — No, and the evidence is clear.**
Jarvis's reports are accurate, hedged appropriately, and repeatedly correct when it
would have been easier to be agreeable: he caught the circular calibration, refused a
pre-authorized 3,919 task-hour job, correctly excluded the void points, refused to
quote the 144x/64x quotients as speedup bounds, and self-reported his own
transcription error unprompted. That is a strong record.
BUT there is a communication-shaped failure and it is **mine**: my directives
specified WHAT TO CONCLUDE and not WHAT TO PRINT. I asked for "cold/hot z and
stationarity p" — aggregates. Jarvis delivered exactly, correctly, what I asked for.
The per-chain table was never withheld; it was never requested. **The gap is in the
specification, not the transmission.** It looks like a worker problem and it is an
instruction problem.

**"Bad model?" — No, and this is testable.**
Every class-B error was found *instantly* the moment the right quantity was printed:
B3 and B4 each took one command. A capability ceiling does not look like that; it
looks like struggling with the quantity once you have it. This looks like nobody
printed it. A stronger model would help at the margin; making disaggregation
mandatory helps far more and costs nothing.

**"Bad reasoning?" — Partly, and I own my share.**
Class C is real and it is mine. B3 is the sharpest example: I wrote the warning
against reading a plateau from too few blocks in one UPDATE and then violated it in
the previous one. That is a genuine reasoning failure, not a structural one. But
class C has a good detection record — the PI and Jarvis catch it — whereas class B
had a *zero* detection record until we changed what gets printed. Class B is the
expensive one.

--------------------------------------------------------------------------------
## 4. The pattern that actually works

Sort every resolution in this campaign by what resolved it:

| Resolved by a NEW MEASUREMENT (a probe) | Rounds to resolve |
|---|---|
| winding-sector hypothesis -> sector-exchange counter | 1 |
| distributed vs localized bottleneck -> marker test | 1 |
| observable trustworthiness -> static-config calibration | 1 |
| depth-solvable or not -> depth ramp | 1 |
| Feynman's error-bar claim -> between-chain error | 1 command |
| L=10 ergodicity -> per-chain table | 1 command |

| Resolved by REVIEWING existing numbers | Rounds |
|---|---|
| L=10 non-ergodicity | ~15 handlings, never |
| round-trip rule | 4 failed ladders, never |
| Binder convention | multiple cycles, never (source comparison did it) |

**Reviews check that a number was computed correctly from its input. Probes check
whether the input means what we think it means.** Nearly every expensive failure
here was the second kind, and we have been spending the review budget on the first.

--------------------------------------------------------------------------------
## 5. The fixes — structural, cheap, no new reviewers

1. **MANDATORY DISAGGREGATION.** Any pooled number ships with its per-unit table:
   a pooled mean ships with the per-chain values; a per-chain tau ships with its
   block-count; a fitted exponent ships with its points and residuals. Automated in
   `scripts/ensemble_diagnostics.py`, not requested ad hoc. *This alone would have
   caught B3 and B4 at zero cost.*
2. **SPECS NAME THE EVIDENCE, NOT ONLY THE VERDICT.** Feynman's job: every directive
   states what must be PRINTED, not just what must be DECIDED. My failure, my fix.
3. **RULES CARRY PROVENANCE.** Every invariant in the harness or run manual cites the
   measurement that established it, or is explicitly marked UNVALIDATED. B5 was an
   unvalidated rule that became authoritative purely by being cited. Audit the
   existing invariants for others like it.
4. **CONVENTION CHECK AGAINST SOURCE.** Any quantity compared to literature states
   the source's exact definition beside ours before comparison. Catches B1's class.
5. **BUDGET SHIFT: one new probe beats two more reviews.** When something is
   uncertain, the first question is "what is the cheapest NEW measurement that
   discriminates?", not "who else should look at this?". The table in sec 4 is the
   argument.
6. **A REVIEW THAT SEES ONLY AGGREGATES SHOULD DECLINE.** If a reviewer is handed a
   pooled number without its disaggregation, the correct response is to ask for the
   table, not to approve. Reviewing what cannot contain the error manufactures false
   confidence, which is worse than no review.

--------------------------------------------------------------------------------
## 6. The one-line version

We kept reviewing the *answer* when the errors were in the *averaging*, and averaging
destroys the evidence of its own failure. Print the disaggregation by default and
this whole class disappears.
