# Field thresholds — version 2.1

Target remains the inspected E:\Cricket07\Cricket07.exe, SHA-256 d776dba0cc506b968f7d228e036cf42aeb4abe45a8d1ad0d7226cdebf91553a2. New ASI SHA-256: 96c1ae3e7cfbdee3f6a19d100729e3c3b7e1b87c71c657db0da732178c79c8a3.

## What version 2.0 did

The original 0059F840 selector executed first. Short formats retained its selection. Long formats constrained the returned family to the configured phase mask; that could override a native Defensive result. Native thresholds were therefore evaluated but did not necessarily determine the final long-format family.

## New explicit rule

The user confirmed that the first duplicated "50 overs" row means 45 overs. The implementation follows the game's existing either-threshold interpretation: after powerplay, Normal only while striker runs are at/below the stated limit AND the first Defensive over has not begun. The alternative interpretation was offered for clarification; the rule used was stated before implementation.

| Length | Powerplay | Run limit | First unconditional Defensive over |
|---|---:|---:|---:|
| 5 | 1 | None; immediately Defensive afterwards | 2 |
| 10 | 3 | 10 | 7 |
| 20 | 6 | 20 | 13 |
| 45 | 9 | 40 | 37 |
| 50 | 10 | 40 | 41 |

The cutoff comparison uses completed overs 6/12/36/40. Powerplay takes priority over score. A score equal to the limit remains Normal before the cutoff; one run above triggers Defensive. These are innings runs of the current striker, read from match+D538 -> player+A4 bits20..30, the same field read by native 0059F840. Bank and variant number are retained when remapping family.

Forty-over policy, all bowling plans, and all outside-circle limits remain as in 2.0. Early Defensive templates in 45/50 overs remain subject to four outside during phase2. Short formats keep their native physical placement. No extra hooks or executable signatures were introduced; the same nine ABI boundaries apply.

FIELD SELECT logs the format, one-based over, striker ID, runs, chosen family, outside limit and original/final preset. Repeated identical calls from one preparation are deduplicated.

## Validation

- All prior 31,500 scheduler cases and 521 resumes passed.
- All nine executable detour/trampoline tests and actual EXE-prefix checks passed, including shared-page RX restoration.
- Packed striker-run extraction tested over 0..2047 while unrelated packed bits are set, and after switching the striker pointer.
- Boundary cases for each changed format: last powerplay over at 2047 runs, first post-powerplay over at the limit/limit+1, last pre-cutoff over at zero/limit/limit+1, first cutoff over at zero/limit, and striker changes.
- All three native bowling banks, all three incoming families and variant numbers 0/4/9 exercised for each boundary. Native variant and bank remain unchanged while the requested family is enforced.
- Five-over innings tested Open at over1 and Defensive for every remaining over, regardless of score.
- Simulation tested with a stale global over counter; innings counter still triggers the correct cutoff. Existing human-field protection, circle-limit, practice, physical-placement and unsupported-format tests passed.
- Forty-over selection retains its prior phase masks; no new threshold was inferred for it.

Version 2.1 has not yet been checked in a live match. Historical 2.0 captures are retained as historical evidence and do not establish these new score/cutoff results. No executable, roster or XML files were edited.
