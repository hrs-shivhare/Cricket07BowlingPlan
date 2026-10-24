# Field thresholds — version 2.3

T10 now uses a striker-run limit of 20; T20 uses 30. The limits are inclusive: after powerplay and before the format cutoff, 20/30 runs gives Normal and 21/31 gives Defensive. Defensive remains unconditional from overs 7/13. Powerplay, bowling rotation, circle limits and all 40/45/50-over policies remain unchanged.

Build and all automated tests passed, including both new inclusive run boundaries, limit+1, powerplay priority, striker changes, over cutoffs, long-format score independence, 31,500 seeded schedules, 521 resume checks and native integration/detour checks. No live-match verification was performed for 2.3.

ASI SHA-256: 5fecfc51f8d583058f40d8bc314d9295d91c4c4d4851503e2fbd0d03d8e3b2e1.
The 2.2 package remains preserved in output/Cricket07BowlingPlan-2.2.zip. Game executable and player/configuration files were not edited.