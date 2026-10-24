# Native field-legality audit for long formats

Read-only native analysis; only this new evidence report was written. Target: E:/Cricket07/Cricket07.exe, known SHA-256 d776dba0cc506b968f7d228e036cf42aeb4abe45a8d1ad0d7226cdebf91553a2.

## Central validators and safe entry detours

All three return the Boolean result in AL, not a guaranteed fully cleared EAX. Use an unsigned-char return type when calling originals.

| Address | ABI | Safe prologue | Original bytes |
|---|---|---|---|
| 0x491330 | thiscall unsigned char(self, int proposed_position, unsigned char* reason), RET 8 | 8 bytes | 83 EC 10 53 8B 5C 24 18 |
| 0x4914E0 | thiscall unsigned char(self, unsigned char* reason), RET 4 | 6 bytes | 51 53 55 56 33 DB |
| 0x491280 | thiscall unsigned char(self, int preset_index, int limited, int powerplay), RET 12 | 7 bytes | 83 EC 08 8D 44 24 04 |

All copied prologue instructions are whole instructions and contain no relative operands. Bytes were read from the actual installed executable.

## Field object and category ranges

self is the frontend field-configuration object, normally the static object returned by 0x48E9C0 (0x7EA6B8).

88 rows, stride 0x50. For row i:

- assigned player ID: unsigned byte self + 0x10 + 0x50*i; 0xFF means empty.
- position category: byte self + 0x11 + 0x50*i. Native uses MOVSX, so read as int8_t then convert to uint32_t for native unsigned range comparisons.
- selected player ID: unsigned byte self + 0x1B80.
- all range starts/ends below are uint32_t fields, not byte fields.

range_start[0..4] = read_u32(self + 0x2A78 + 4*i)
range_end[0..4]   = read_u32(self + 0x2A8C + 4*i)

0x491BA0 initializes starts to {0,31,38,68,82}, ends to {30,37,67,81,87}.

Native restricted categories, using unsigned comparisons:

outside(c) = in(c,start[0],end[0]) || in(c,start[1],end[1])
legside(c) = in(c,start[0],15) || in(c,start[1],34) || in(c,start[2],52) || in(c,start[3],73) || in(c,start[4],84)
behind_leg(c) = in(c,start[0],7) || in(c,start[1],32) || in(c,start[2],45) || in(c,start[3],71) || c == start[4]

Here in(c,a,b) means uint32_t(c) >= a && uint32_t(c) <= b. Thus a category byte 0xFF sign-extends to UINT32_MAX and fails ordinary finite range matches.

These are UI position categories, not calculations of actual distance from a circle. Actual world-coordinate enforcement requires a separate gameplay-placement hook; changing only these validators affects field editor/move/save legality and preset availability.

## 0x491330: projected move validator

uint32_t noutside=0,nleg=0,nbehind=0;
bool candidate_affects_restricted=false;
for (int i=0;i<88;i++) {
    uint8_t assigned=row[i].assigned;
    if (i != proposed_position && assigned == 0xFF) continue;
    // Native MOVSX of assigned, compared with unsigned selected byte:
    if ((int)(int8_t)assigned == (int)selected_player) continue;
    uint32_t c=(uint32_t)(int32_t)(int8_t)row[i].category;
    if (outside(c)) { noutside++; if(i==proposed_position) candidate_affects_restricted=true; }
    if (legside(c)) { nleg++; if(i==proposed_position) candidate_affects_restricted=true; }
    if (behind_leg(c)) { nbehind++; if(i==proposed_position) candidate_affects_restricted=true; }
}
if (!candidate_affects_restricted) { *reason=1; return 1; }
if (scheduled_overs != -1) {
    if (native_powerplay_active && noutside>2) { *reason=5; return 0; }
    if (!native_powerplay_active && noutside>5) { *reason=2; return 0; }
    if (nleg>5) { *reason=3; return 0; }
}
if (nbehind>2) { *reason=4; return 0; }
*reason=1; return 1;

It excludes the selected player's old position but force-includes an empty proposed row. Calling it with an occupied candidate is not the normal UI path (caller checks empty before invocation).

For a minimal phase-two-only tightening wrapper, first call original, preserve a failure, and when original succeeds reject if candidate_affects_restricted is true and projected outside count exceeds four. Return reason=2. This preserves the native ability to move a player out of restricted categories even when another part of the field is already illegal.

## 0x4914E0: complete current-field validator

Same three category tests, evaluated independently, counting every nonempty row. No selected-player exclusion. Reads global match via 0x4EC220 and active powerplay via 0x4AF4E0. Restrictions and reason codes are the same as above. There is no candidate_affects_restricted early success.

Minimal phase-two wrapper: original failure remains failure; if original succeeds and outside count exceeds four, set reason=2 and return zero.

## 0x491180 and 0x491280: saved-preset path

0x491180 is thiscall void(self, uint8_t* nine_positions, uint32_t* outside, uint32_t* legside, uint32_t* behind_leg), RET 16.

0x491280 gets the nine-byte array at:

    self + 0x282D + 9*preset_index

Its Boolean limited and powerplay arguments are actually tested only as their low bytes. It calls 0x491180, then enforces outside <=2 if limited&&powerplay, outside<=5 if limited&&!powerplay, legside<=5 if limited, and behind_leg<=2. It does not expose a reason output.

IMPORTANT native quirk: 0x491180 counts using ELSE-IF, unlike 491330/4914E0:

for each of nine position bytes c (MOVZX, unsigned):
    if (outside(c)) outside_count++;
    else if (legside(c)) leg_count++;
    else if (behind_leg(c)) behind_count++;

Therefore outside leg-side positions are not included in its leg_count; with default ranges the behind_leg categories are subsets of earlier categories and behind_count cannot increase. Preserve the original validator's other behavior rather than accidentally correcting unrelated native quirks. A middle-only tightening wrapper can call original and separately count outside directly in the nine bytes, rejecting >4.

## Reason codes and consumers

1 = valid
2 = too many outside the circle, non-powerplay (native maximum five)
3 = too many on the leg side (five)
4 = too many behind square on the leg side (two)
5 = too many outside the circle during powerplay (two)

Direct consumers:

- 0x491330 is called twice by 0x491E50 (find valid alternate/free position) and once by 0x491FC0 (move selected fielder). Thus detouring it covers the editor's candidate finding and actual move approval.
- 0x4914E0 is called by 0x4AD070 (field editor Done/save path).
- 0x491280 is called by 0x4B4AF0 (field preset list filtering). The shared call site passes actual limited and PP flags; in the PP branch it pushes actual PP=1, in normal branch it pushes zero.
- 0x491180's only direct caller is 0x491280.

Warnings in 0x491FC0 and 0x4AD070 use localized text IDs:

- reason 2 -> 0x20E
- reason 3 -> 0x20D
- reason 4 -> 0x20F
- reason 5 -> 0x20C formatted with 0x48EEF0's powerplay-overs result.

The reason-2 native message still describes five; returning reason=2 for a new maximum of four needs a phase-aware message override or the warning text is misleading. The localization lookup is 0x6E3610, thiscall char*(self, uint32_t text_id), RET4; accessor 0x6E3660 returns its singleton, cdecl no arguments. A narrowly filtered lookup override for ID0x20E in the new phase-two formats can supply a persistent English four-fielders message without changing other text, if this additional hook is desired.

## Phase source and existing powerplay consumers

Native 0x4AF4E0 compares global completed overs match+0x58C6 against 0x48EEF0(scheduled_overs). The scheduled total is signed word +0x58C4. Current innings is byte+0x58C9.

For gameplay and quick simulation consistently use the verified plugin completed_overs helper: mode0 uses 58C6; nonzero mode uses 58E4+2*innings. Native quick-simulation path does not advance 58C6.

0x48EEF0 has five direct callers:
491FC0,4AD070 warnings;4AF4E0 active flag;4B4AF0 preset list phase;4BEEE0 status/label.
0x4AF4E0 has three direct callers:
491330,4914E0,59F840 CPU preset selection.

The new long-format phase boundaries should be centralized by the parent implementation; this audit establishes limits/consumers and does not assume unprovided boundaries. 48EEF0 should report first-phase length irrespective of 15/20 setting. It alone cannot encode the second phase's maximum four; all three validators need the new tightening.

## Scope limitation

No direct calls to these three frontend legality validators were found in gameplay preset loading or adaptive CPU positioning. CPU preset selection/load needs its own phase-aware valid-position enforcement, owned by the separate field-selector investigation. Otherwise a CPU stock preset or adaptive move can place five outside during phase two despite a correct field editor. UI position indices and gameplay XML/world coordinates are separate representations.

Verified actual jump tables: 0x49222C reason1..5 -> {4921A0,4920A2,492067,4920DD,492118}; 0x4AD4BC -> {4AD1B9,4AD158,4AD121,4AD163,4AD16E}. Thus circle warning is ID0x20E, leg-side warning ID0x20D. Localization entry safe7 bytes:55 8B EC 51 89 4D FC.

Existing category-only helpers also match these formulas:0x48FEA0 outside,0x48FEE0 legside,0x48FF40 behind-leg. Each is thiscall int(self,int category),RET4. No direct callers were found in generated code; they contain no limit constants and need no detour.
