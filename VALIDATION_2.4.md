# Boundary/wicket field variants — version 2.4

CPU-controlled innings in the six supported formats start at XML variant 2. A rise in the cumulative batting XI fours, sixes or innings wickets selects one of the other four variants from 1–5. Each outcome maps from two PCG output bits without modulo bias. Selection uses separate random state from the bowling schedule. Co-occurring increases and skipped simulated batches cause one change per observed snapshot. Repeated calls with the same counters cause no extra changes.

The native bank and all 2.3 field-family thresholds, bowling plans, circle limits, ownership guards and hook locations are retained. No extra native hook is required. Human field selection and unsupported lengths remain native.

Static storage evidence:
- Match-local current batting XI pointer: match+D530, 11 records, stride 168 hex. The existing roster bridge and gameplay setup establish this pointer.
- Native 472E71–472EBF and 473B57–473BA5 inspect credited-boundary byte match+21219. Boundary value 4 increments record+A4 bits10..19; sixes increment bits0..9. The detector sums these independent fields, ignoring runs bits20..30.
- Wickets: signed 16-bit match+5910+2*innings. Native 472690/473430 update this counter. Declaration-style negative values are decoded as -1-value; invalid values above 10 do not enter the cache.
- Innings opening hook resets the cached state, including when match addresses are reused. A saved mid-innings load or lower counters establish a fresh baseline at variant 2 rather than replaying old events.

Validation passed:
- All six formats: initial variant 2 across all three banks, all incoming families and all ten incoming native variant numbers.
- Credited four on the previous striker after strike changes; six; wicket/run-out; simultaneous event-counter changes; new innings and reused match memory.
- Repeated calls (including 1,000 per event) keep the same variant and random state.
- Running four, score parity, change of strike, bowler/bank changes, family/over transitions alone do not consume variant RNG.
- 10,000 successive credited boundaries stay in range and never immediately repeat.
- For each current variant, 4,096 draws cover exactly the other four choices. Scheduling RNG remains unchanged.
- Resume with existing boundary/wicket history in simulation, null/rebuilt records, counter rollback, human ownership and unsupported formats.
- Existing inclusive T10/T20 limits 20/30, format cutoffs, long-format run independence, field-family/2-4-5 outside limits, placement, 31,500 seeded schedules, 521 resume cases and all nine detour/actual EXE-prefix checks.

Version 2.4 has not yet been verified in a live match.
ASI SHA-256: 679bf30791de073ca5eea483b1c9db9955402a8f84469fe6db6a8214f378d373.
Supported EXE SHA-256: d776dba0cc506b968f7d228e036cf42aeb4abe45a8d1ad0d7226cdebf91553a2.
Previous release remains in output/Cricket07BowlingPlan-2.3.zip. Executable, roster, XML and SAF files were not modified.