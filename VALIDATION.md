# Validation and native evidence — version 1.1

Target: E:\Cricket07\Cricket07.exe, SHA-256 d776dba0cc506b968f7d228e036cf42aeb4abe45a8d1ad0d7226cdebf91553a2. Static evidence comes from the earlier REA native analysis and the corresponding recovered instruction listings in out_all. Original addresses use image base 0x00400000; the ASI uses RVAs relative to the actual loaded image.

## Hook boundary

| Address | Evidence and purpose | Entry signature |
|---|---|---|
| 0046F350 | cdecl opening-bowler initializer; sets D540/D544 pointers and bowler role flags. Original initialization is called, then the supported CPU innings' first pair is applied. | 53 56 8B 74 24 0C |
| 00471CC0 | cdecl end-over wrapper; native chooser call 004701B0 is inside this wrapper. Supported CPU innings bypass its rotation logic and retain its final update sequence. | 56 8B 74 24 08 |
| 0048EEF0 | stdcall restriction-length helper, ret 4. Native 15/20 setting is read inside this helper; the plugin returns fixed lengths for 5/10/20 and calls the original helper otherwise. | 8B 4C 24 04 8D 41 FB |

Each overwritten prologue contains complete instructions with no relative operand. The installer checks all three signatures and prepares all trampolines before changing any entry. Initialization runs before gameplay; this is not a hot-attach patcher for a running match. The DLL is pinned for the lifetime of its hooks.

Native 00471CC0 tail: 0046FED0(match), 004628A0(match); when the world exists and match+211F4 is 2, thiscall 00415330(00413B80()). If the scheduled next bowler replaces the other-end pointer, the world+65 change notification is set through 00403520/00403CE0, as in the native chooser branch.

004628A0 swaps D540/D544, sets their low six role bits to 1/2 and updates the displayed bowler. Opening and replacement code clear obsolete bowling roles while preserving the packed high bits. The batsman/end swap remains in 0046FED0.

## State evidence

| Match offset | Use |
|---|---|
| 58C4 | Signed 16-bit scheduled innings overs; -1 means Test. |
| 58C6 | Completed team overs. 00471F90 increments it before calling 00471CC0, so it directly indexes the next over in a zero-based plan. |
| 58C9 | Innings index. |
| 58E4 + 2*innings | Per-innings completed overs. The simulation path 00474400/00473430 updates this counter while leaving 58C6 stale, so nonzero simulation modes use this counter instead. |
| D540 / D544 | Current bowler / bowler at other end (next after swapping). |
| D548–D55C | Six bowling-order record pointers. |
| D560 | Previous/current delivery bowler; 00472690 assigns D540 here. **Not a keeper pointer and not excluded from the pool.** |
| D568 | Bowling team ID. |
| 211F4 / 2120D | Simulation mode / current bowling controller. Nonzero mode or FF selects the CPU path; a non-FF byte alone does not establish human control. |
| Player +00 / +04 | Player ID / display name. |
| Player +8C | Packed role; low six bits 1 current bowler, 2 other end, 4 keeper. |
| Player +A8 | Low 11 bits completed bowling overs. 00472A5A increments this field when legal-ball count reaches six, before end-over selection. |

The frontend CPU flag and controller actors can still describe the previous innings when the opening initializer runs. Version 1.1 uses 00487E10(side, controllerIndex), a cdecl getter of the team's current device IDs. Side 0 is the first configured team (match+1C20), side 1 the second (match+4858); match+D568 identifies the bowling team. Both assignment values -1 mean no human device. The mapping follows 00414230, 00403C10 and 004A6870, which route device markers H/A to the correct current-role manager. Simulation and the native FF controller sentinel also select the CPU opening path.

At end of over, version 1.1 follows the exact inverse of the native human guard in 00471CC0: CPU when mode is nonzero, slot is FF, 005A6C10(bowlingManager) returns zero, or 005A6BF0(bowlingManager) returns zero. The manager comes from 005A4AC0. A human side reaches the original wrapper when its mode is zero, slot is not FF and both controller getters return nonzero. Unsupported formats also retain the original wrapper.

## Live failure captured and corrected

On 24 October 2026 (Asia/Calcutta), version 1.0 was inspected in the user's running ten-over match. The CPU bowling side had mode 0 and controller-slot byte 0. Its manager's two actor slots both resolved to null pointers. The current bowling side's frontend assignments were both -1; the batting side had a human device. All three ASI entry detours were present. The plugin log contained APPLIED but no PLAN because its incomplete CPU predicate silently skipped rotation. Native end-of-over calls were observed at completed overs 1 and 2.

This is a confirmed CPU-classification bug in version 1.0. The original fixtures used FF for ordinary CPU sides and missed this state. Version 1.1 adds full-innings regression fixtures for slot 0 with absent actors, separately tests each absent actor branch and both configured team sides, and protects human openings against stale actors from the previous innings. Raw Frida observations, with their provider limitations, are retained in evidence/live_1.0_failure.json.

Restriction-helper callers include 004AF4E0 (active restrictions, consumed by CPU field selection), 00491FC0/004AD070 (legality/error text), 004B4AF0 (field UI state) and 004BEEE0 (field-status text). 004AF4E0 compares completed overs against the helper result directly. The five-over native restriction period is two overs; this plugin deliberately changes it to the requested one.

## Passed checks

- Optimized x86 build with C11 and -Wall -Wextra -Werror; assertions explicitly enabled in test executables despite optimized-build NDEBUG defaults.
- Independent exhaustive enumeration of feasible middle sequences: T20 10,684; T10/B3 60. The scheduler also supports B4 as a test variant, but the plugin always uses the user's chosen B3.
- 30,000 seeded schedule cases: fixed phases, middle allocations, quotas, no consecutive overs, reproducibility and errors.
- 245 checkpoint resumes, including complete innings and impossible histories, with unchanged output on failure.
- Native-state fixtures for complete T5/T10/T20 innings under ordinary CPU control and simulation modes 1/2. Simulation fixtures deliberately leave 58C6 stale. Tests retain packed role/stat bits and assert normal tail callbacks and simulation UI refresh.
- Human and unsupported-format forwarding, malformed/keeper/duplicate pools, previous-delivery D560 behavior, change notifications, missing-world handling, multi-innings cache separation, compatible reloads and incompatible-history fallback.
- Version 1.1 CPU-slot-zero regression: six complete ten-over fixture innings (both configured sides; both actors absent or either actor absent), plus human first/second device assignments and stale-actor opening states.
- 10,000 repeated powerplay calls for supported and forwarded lengths.
- Executable x86 detour/trampoline fixtures for all three actual prologues; 10,000 calls each through replacement and original trampolines, including stdcall stack cleanup. A conflict in the third signature leaves every entry unmodified.
- Actual EXE PE32 size and all three hook signatures read from disk and matched. SHA-256 unchanged before/after validation.
- The built DLL loads in a test process, exposes both expected undecorated exports, rejects that unsupported host with status -1, and initializes idempotently.
- Source-level comparison against the installed sibling plugin hook locations found no overlapping entry patch. This does not guarantee compatibility with future or uninspected third-party hooks.

## Version 1.1 live verification

Version 1.1 was installed over the known 1.0 build after the user closed the game; the original ASI was preserved in releases/1.0. The installed SHA-256 is 4d690393e089868602ea6779306abcd26dc9fd27d8c76fe41f0683f22ddc93c4.

In the user's fresh ten-over match, CPU slot 0 now produces OPEN APPLIED and a valid PLAN. Both the plugin log and independent passive Frida reads show over 1 S.Harmison (B1, player3892), over 2 M.Hoggard (B2, player3860), over 3 A.Flintoff (B3, player25679), then over 4 A.Giles (B4, player20526), matching the sampled middle. Completed-over statistics advance normally and no FALLBACK is logged. This verifies the CPU-detection fix and the first middle-over transition in the actual game rather than only in fixtures. Raw observations and the plugin log are retained in evidence/live_1.1_verified.json and evidence/live_1.1_plugin.log. The Frida observer was detached after the capture; it only read match state and did not write bowler values.

## Remaining live-game validation

Automated fixtures cover complete schedules for all three lengths. Live observation of this correction covers the starts of overs 1–4 in the ten-over match, not every format and condition.

After installing, play or simulate new 5-, 10- and 20-over matches. Compare the scorecard's bowler per over to the logged plan. Check that field restrictions are active for exactly 1/3/6 overs under both 15- and 20-over setting choices, and that the field UI and CPU field changes reflect the boundary. Verify a second innings, a compatible save/reload and one ordinary 50-over match. Confirm the log contains `APPLIED` and no unexpected `FALLBACK` entry. These are validation steps, not additional prerequisites for building the supplied ASI.
