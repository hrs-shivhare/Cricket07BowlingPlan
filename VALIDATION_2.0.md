# Validation and native evidence — version 2.0

Date: 24 October 2026, Asia/Calcutta. Target: `E:\Cricket07\Cricket07.exe`, SHA-256 `d776dba0cc506b968f7d228e036cf42aeb4abe45a8d1ad0d7226cdebf91553a2`.

Installed 2.0 ASI SHA-256: `4e7198a61afdc58e71aa5af4b35e5ef244a789519a8aed4c547175f049fa09ee`. Version 1.1 is backed up under `releases/1.1`; its unchanged original record remains in [VALIDATION.md](VALIDATION.md). Earlier live observations are not proof of the new 2.0 features.

## Native entry boundaries

Addresses are original executable VAs, image base `0x00400000`. Installation uses RVAs relative to the loaded image. Entry bytes match the actual supported executable.

| VA | Purpose / ABI | Copied whole-instruction bytes |
|---|---|---|
| `0x46F350` | Opening bowlers; cdecl int(match) | `53 56 8B 74 24 0C` |
| `0x471CC0` | End-over wrapper; cdecl void(match) | `56 8B 74 24 08` |
| `0x48EEF0` | Restriction duration; stdcall int(overs), RET 4 | `8B 4C 24 04 8D 41 FB` |
| `0x59F840` | Limited field selector; cdecl int(void) | `51 53 55 56 57` |
| `0x491330` | Proposed peg move; thiscall byte(self,position,reason), RET 8 | `83 EC 10 53 8B 5C 24 18` |
| `0x4914E0` | Whole-field peg legality; thiscall byte(self,reason), RET 4 | `51 53 55 56 33 DB` |
| `0x491280` | Saved-peg preset; thiscall byte(self,index,limited,pp), RET 12 | `83 EC 08 8D 44 24 04` |
| `0x6E3610` | Field-warning lookup; thiscall pointer(self,id), RET 4 | `55 8B EC 51 89 4D FC` |
| `0x5B2350` | Final field preparation; thiscall void(self) | `81 EC 38 02 00 00` |

No copied prologue contains a relative operand. All signatures/trampolines are prepared before entry writes. Setup runs through the ASI loader before play, not as a hot-attach patcher. The module is pinned while hooked. Hash/PE checks reject unsupported executables; a signature conflict prevents all nine hooks being applied.

The three frontend validator entries share a code page. Protection restoration runs in reverse so the first snapshot restores original RX after later snapshots observe temporary RWX. An executable fixture specifically checks shared-page restoration and failure before installation.

## Rotation and ownership

Version 1.1's CPU guard is retained. At end of over, CPU means nonzero simulation mode, controller slot FF, absent bowling manager or either absent controller actor. Slot 0 alone does not establish human control. Opening initialization uses the current bowling team's two frontend device assignments because actors can still belong to the previous innings. Both assignments −1 mean CPU, with simulation and FF also accepted.

Important match fields: `+58C4` scheduled overs, `+58C6` completed interactive overs, `+58C9` innings, `+58E4+2*innings` simulation overs, `+D540/+D544` current/other-end bowlers, `+D548..D55C` bowling-order records, `+D568` bowling team. Player `+A8 & 0x7FF` counts completed bowling overs. `+D560` is the last/current delivery bowler, not a keeper exclusion. Packed upper role/stat bits are preserved.

For valid supported CPU plans, the native chooser is bypassed. Tail calls `0x46FED0` and `0x4628A0` retain end changes, roles and display updates; simulation refresh and change notification remain. Human/unsupported innings and invalid/incompatible pools forward to native selection.

Every fixed sequence is in the [README](README.md). Long-format boundaries and exact quotas are:

| Overs | Phase ends | Random-phase B1–B6 | Whole-innings B1–B6 |
|---:|---|---|---|
| 50 | 10 / 20 / 40 / 50 | 2 / 2 / 4 / 6 / 2 / 4 | 10 / 10 / 10 / 10 / 6 / 4 |
| 45 | 9 / 18 / 36 / 45 | 1 / 3 / 3 / 5 / 2 / 4 | 9 / 9 / 9 / 9 / 5 / 4 |
| 40 | 8 / 16 / 32 / 40 | 2 / 2 / 3 / 4 / 2 / 3 | 8 / 8 / 8 / 8 / 5 / 3 |

Random-phase spells have two successive turns from one end. Each odd middle quota adds exactly one separate single-turn remainder for that bowler. Tests inspect actual per-end runs, not just four-over block counts. Adjacent overs always differ, including phase boundaries. Long search is memoized, reproducible and bounded at 250,000 states; it does not claim uniform full-plan sampling. Short-format uniform feasible-sequence sampling is unchanged.

Resumes reconstruct a compatible prefix with the same completed totals and last bowler. That reconstructed ordering need not equal the actual historical ordering: in-progress per-end spells cannot be uniquely recovered from these aggregates. Plans are not persisted in saves. Exact spell continuity through reload is an explicit limitation; no past statistics are changed.

## Field policy and evidence

Read-only native analysis and decoded installed resources are retained in:

- [long_cpu_fields.md](evidence/long_cpu_fields.md): selector/preparation ABIs, banks, world positions, adaptive movement and refresh timing.
- [long_field_legality.md](evidence/long_field_legality.md): peg validators, signed categories, native quirks and warning consumers.
- [odi_field_counts.json](evidence/odi_field_counts.json): all 90 ODI variants and counts under the new physical policy.

The selector calls the original first, retaining its behaviour and gameplay RNG consumption. Only a disallowed family is remapped. Bowling bank and variant number stay unchanged. Banks are SlowOdi base 0, FastOdi base 35 and MediumOdi base 140; Defend is +0..9, Normal +10..19, Open +20..29. Additional family choices use plugin PCG.

For 40/45/50, phases permit Open/two outside, then Normal/four, then Open-or-Normal/four, then Open-or-Normal-or-Defend/five. Allowed native choices are retained; this is not uniform random family selection on every delivery. All ten variants remain possible. Native adaptive placement executes before physical enforcement.

Frontend legality uses **discrete peg categories**, with phase 2 tightened to four after original checks succeed. Other native failures, leg-side restrictions and preset quirks remain. Human families/positions are not automatically changed, but editor/move/save legality gets the four-outside limit. Text ID `0x20E` supplies its warning.

### Explicit new physical geometry

The post-preparation world-coordinate test is a **new plugin policy**, not a recovered native world-distance legality check:

```text
nearest_z = clamp(fielder.z, -10.06, +10.06)
distance_squared = fielder.x^2 + (fielder.z - nearest_z)^2
outside = distance_squared > 27.432^2
```

Nine ordinary outfielders are counted; keeper and bowler are excluded. Nearest excess outsiders move toward the nearest pitch-segment point to radius 26.5 m, preserving Y. Getter `0x42C500` and setter `0x5A34F0` use the native world-coordinate conversion path. Missing/invalid positions are not moved. Practice/special-field mode `0x462360` skips correction so parked actors are not pulled into play.

For valid finite positions, the cap holds at preparation-hook completion under this policy. It is not continuous movement policing and does not imply synchronization of frontend pegs and world positions. Native peg legality and new physical enforcement are separate.

## Passed automated checks

- Optimized x86 Windows C11 with `-Wall -Wextra -Werror`; test assertions enabled despite optimized-build NDEBUG defaults.
- **31,500 seeded schedules:** 30,000 short and 1,500 long, plus reproducibility. Independent exhaustive short-middle counts: T20 10,684; T10 60 for each supported third-bowler variant.
- **521 checkpoint resumes:** 245 short and 276 long, including completed innings. Invalid counts/last bowlers/fixed-phase histories preserve output on failure.
- Exact fixed phases, all six formats' quotas, random allocations, boundary adjacency and actual same-end long-spell runs, including all required odd single-turn remainders.
- Maximum long-scheduler search observed in its tests: 1,552 states versus a 250,000-state budget. This is observed coverage, not a universal worst-case proof.
- Complete 5/10/20/40/45/50 native-state fixtures under interactive CPU and simulation modes 1/2; stale interactive counters in simulation, packed bits, updates and UI refresh.
- Retained CPU-slot-zero regression on both configured sides/each absent-actor branch; human first/second device assignments and stale previous-innings actors protected.
- Human/unsupported forwarding, malformed/keeper/duplicate pools, last-delivery pointer handling, missing world, multiple innings and compatible/incompatible reloads.
- Field phases/families, retained bank/variant, 2/4/5 caps, original failures, four-outside editor/preset warning, adaptive final placement, preserved Y, practice exclusion and unchanged human physical fields.
- 10,000 repeated restriction-helper calls for supported and forwarded lengths.
- **Nine executable x86 detour/trampoline fixtures, 10,000 calls each**, covering cdecl/stdcall/thiscall arguments and cleanup. Shared-page RX restoration and final-signature conflict rollback pass.
- Actual EXE PE32 size/all nine entry prefixes match disk evidence; executable SHA-256 unchanged through validation.
- DLL load/export checks, unsupported-host rejection and idempotent initialization pass.

These are scheduler, native-state and executable-fixture tests, not a claim of a completed 50-over live match.

## Version 2.0 live verification

Observed in the user's fresh 50-over match with South Africa bowling. The installed ASI loaded all nine hooks. Player mapping: B1 S.Pollock (3152), B2 M.Ntini (4941), B3 A.Hall (25003), B4 N.Boje (29933), B5 J.Kallis (2390), B6 J.Rudolph (12388).

The captured plugin log verifies the exact fixed opening and normal sequences through over 20, then the random middle's first spell pair at overs 21–24: B2, B5, B2, B5. The next pair begins B4, B3, B4 at overs 25–27. A separate log check matched all 26 recorded end-of-over selections to the generated 50-over plan and confirmed its total allocations 10/10/10/10/6/4. No FALLBACK entry occurred in this long innings.

Independent Frida observation read actual selected preset IDs and actor positions after final field preparation. Open examples include FastOdi preset55 and SlowOdi preset20, each with two outsiders. At over11 the selected family changes to Normal; later recorded examples include FastOdi45/46 and SlowOdi11, with three or four outsiders under the new physical policy. Successful independent position snapshots extend through the start of over26. The captured plugin log reaches the start of over27 and validates caps at every recorded field preparation.

Evidence: [live_2.0.json](evidence/live_2.0.json), [observer source](evidence/live_2.0_observer.js), and [plugin log](evidence/live_2.0_plugin.log). The early observer read the x86 thiscall self value from the wrong location and reported caught read errors; it was corrected to ECX before valid snapshots. Those setup messages remain in the raw evidence. Observation read game state and called the native position getter; it did not set bowler/fielder values. The observer was detached after capture, leaving the installed ASI active and the match running.

This is partial live coverage, not a claim that the whole 50-over innings, death phase, every field variant, or 40/45-over matches were played. Complete schedule, phase3 and physical-correction cases are covered by the automated fixtures. Live samples did not require an excess-outside relocation; the relocation path is validated by fixtures using nine deep actors.

Compare scorecard/current bowlers with `PLAN` and B1–B6 mappings. Verify boundaries at the starts of overs 11/21/41, 2/4/5 final field counts after adaptation and the four-outside editor warning. Further checks should include 40/45 rounding, a second innings, both 15/20 settings, human/manual-field protection, reload behaviour and unexpected `FALLBACK` entries.
