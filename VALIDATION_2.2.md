# Field thresholds — version 2.2

Target EXE SHA-256 remains d776dba0cc506b968f7d228e036cf42aeb4abe45a8d1ad0d7226cdebf91553a2. ASI SHA-256: e1b72f49d03c3319b52ea97b64364151faf571de4ce9a5ada1f82e43d1afbb9d.

The user's final clarification removes striker-run checks from 40/45/50 overs. Forty-over matches have eight Open overs, Normal in 9–32 and Defensive from 33. Forty-five and fifty overs retain 9/10 Open overs and become Defensive from 37/41 respectively. Every post-powerplay over before that cutoff is Normal, regardless of striker runs. Five-over matches retain Open over1 and Defensive afterwards. T10/T20 retain limits10/20 and unconditional Defensive from7/13.

All bowling rotations, quotas, spell rules and outside-circle limits are unchanged. No hook locations or ABIs changed. The original selector still executes to obtain the native bowling bank and variant; the plugin's final family override is independent of runs for long formats. The plugin reads the striker's packed score only for T10/T20 and logs `runs=ignored` otherwise.

Validation passed:

- Long-format last-powerplay, first-normal, last-normal and first-Defensive boundaries at scores0/10/20/40/41/100/2047. All three native banks, incoming families and variant0/4/9 combinations retain bank/variant and select the requested family.
- Switching strike from a zero-score to a 2047-score batter leaves the long-format Normal selection unchanged.
- Simulation with a stale global completed-over counter still switches at the correct innings over.
- T10/T20 inclusive run limits and limit+1, powerplay priority, changes of striker and format cutoffs continue to pass.
- All prior 31,500 scheduler cases, 521 resume checks, circle/editor/physical-placement fixtures and nine executable detour/trampoline/actual-EXE-prefix checks passed.

Version 2.2 has not yet been verified in a live match. Older live captures apply to their recorded versions. The previous 2.1 ASI and sources are preserved in releases/2.1. The executable, roster and XML files were not edited.
