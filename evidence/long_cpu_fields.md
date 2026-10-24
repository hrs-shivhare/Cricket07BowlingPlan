# CPU field selection and physical-placement evidence

Scope: installed Cricket 07 Windows executable and shipped field XMLs. This report and odi_field_counts.json are the only files written by this audit.

Executable: E:/Cricket07/Cricket07.exe, SHA-256 d776dba0cc506b968f7d228e036cf42aeb4abe45a8d1ad0d7226cdebf91553a2. Data archive: E:/Cricket07/Data/data.gob, SHA-256 77ccf07c6d60ad75f692187d0df22216bc934d0240756398b6b715625378f83f.

## Confirmed hooks and ABI

| Address | Recovered behaviour / ABI | Whole-instruction entry bytes |
|---|---|---|
| 0x59F840 | Limited-over selector: cdecl int(void), preset ID in EAX | 51 53 55 56 57 (5 bytes, five PUSH instructions) |
| 0x5B2350 | Field preparation: thiscall void(field_manager), ECX=self, no stack arguments | 81 EC 38 02 00 00 (6 bytes, SUB ESP,0x238) |
| 0x5B25F6 | Sole direct CALL of 0x59F840 inside preparation | E8 45 D2 FE FF (5-byte relative CALL) |

Bytes were read from the installed executable. The selector additionally saves/restores ECX, along with EBX/EBP/ESI/EDI; EDX is scratch. An entry hook must copy whole instructions; the five-byte selector prologue has no relative operands. A call-site replacement must not copy its relative CALL into a trampoline without relocation.

The preparation wrapper should call its original first, then apply physical enforcement. This places enforcement after template loading and the existing adaptive move. Standard x86 detour form: void __fastcall Hook(void* self, void* unused_edx), with an original thiscall pointer.

## State, banks and family IDs

0x4EC220 is a cdecl no-argument match getter. Scheduled overs are int16 at match+0x58C4 (-1 is unlimited); completed overs are uint16 at +0x58C6. Thus completed=10 describes the first delivery of over 11. Only 40/45/50 are in this plugin policy.

| Bank | Base | Native bowler group from 0x569590 |
|---|---:|---|
| SlowOdi | 0 | 2 or 3 |
| FastOdi | 35 | 1 or 5 |
| MediumOdi | 140 | 4 |

Each bank has Defend1..10 at base+0..9, Normal1..10 at base+10..19, Open1..10 at base+20..29. Defend is the actual resource label. Built-in path is Fielding/{bank}/{family}{one_based_variant}.xml. The loader uses case-insensitive hashed archive lookup. All 90 ODI resources were matched and decoded; the JSON records member identities and decoded hashes.

0x5B2350 calls 0x59ED70 for unlimited matches and 0x59F840 otherwise. After return, it checks ownership and applies the returned preset on the CPU/automatic path: owner byte self+5 is 0xFF, or the controller slot/pointer is absent. A present human controller takes the separate stored-field path. This check is at 0x5B25FB..0x5B261D.

0x6F7780 is a cdecl no-argument RNG getter; 0x6F7B40 is thiscall int(rng,lower,upper). Upper bound is inclusive: helper 0x6F7A90 takes raw random modulo (range+1). The selector's existing 0..9 draw is overwritten before its final decision, but advances RNG state. Calling the original before remapping retains that consumption.

## Built-in positions are separate from frontend peg validators

0x48E9C0 is cdecl void*(void), returning static frontend field-config object 0x7EA6B8. Validator 0x491280 passes nine peg IDs from self+0x282D+9*index to counter 0x491180; it is not a general validator for all global built-in preset IDs. Native UI outside restrictions use discrete peg-category ranges, documented separately in long_field_legality.md.

Built-in gameplay positions follow 0x5B2350 -> 0x5B1960 -> 0x5B91A0. The last function loads the XML; 0x5B1960 interprets startpos and sequence, mirrors for hand/end, converts XML coordinates with float constant 0x7F1B78=9.00900936 and wicket offset 0x7F1B88=10.06, then places actors.

## NEW plugin geometry policy; not recovered native legality

The conventional 30-yard oval used in field_policy.c and this JSON is an explicit NEW plugin policy. It is not a recovered native world-coordinate legality calculation. Native frontend legality uses pegs, and this audit did not establish a native equivalent of the world-distance formula.

Policy geometry: radius 27.432 metres about the segment between (0,-10.06) and (0,+10.06) in the ground plane. Outside means hypot(x,z-clamp(z,-10.06,10.06)) > 27.432; Y is ignored. Counts in odi_field_counts.json are base XML counts under this policy, before adaptive relocation or later movement. Reflections for hand/end preserve these counts.

Examples from actual decoded resources:

| Resource | Policy outside count |
|---|---:|
| SlowOdi/Normal2.xml | 4 |
| FastOdi/Normal2.xml | 3 |
| SlowOdi/Normal6.xml | 5 |
| FastOdi/Normal7.xml | 6 |
| SlowOdi/Open5.xml | 3 |

Across all three ODI banks, Open variants 1..4 have two outside; Normal variants 1..4 have at most four; Defend variants 1..5 have at most five. This is a useful initial template filter but does not prove the final placement remains legal. The JSON contains all 90 counts and raw XML positions, making those checks reproducible.

## Why the post-preparation hook is necessary

0x5B1960 and the tail of 0x5B2350 contain adaptive movement gated by the striker's fours field (+0xA4 bits10..19). The template family does not constrain that destination. The preferred candidate pass searches ordinary deep outfielders; a fallback permits a broader candidate set. Therefore a family-only remap or frontend validator hook cannot guarantee the final physical outside count.

After original 0x5B2350 returns, inspect nine outfielders at pointer slots self+0x80+4*i for i=1..9. Slot10 is keeper and slot11 bowler; omit them. Native helpers:

- 0x42C500: thiscall Vec3*(actor,Vec3* out), RET4. Reads actor transform and converts to world metres with factor 0.01905.
- 0x5A34F0: thiscall void(actor,const Vec3* world_metres), RET4. Calls 0x53E0C0(position,1) and 0x53E080(position), the same immediate position-setting path used by native field placement. These helpers handle the inverse animation/world conversion.

The plugin post-hook can count under its explicit oval policy and pull only nearest excess outsiders inside with a margin. Preserve Y. Current policy implementation uses radius26.5 for moved positions. Apply only to CPU ownership and supported formats. This guarantees the count at completion of preparation under the NEW policy; it does not claim that native peg counters are synchronized or that later gameplay movement is continuously constrained.

## Refresh timing and callers

Unique direct preparation call sites: 0x4A4069 (4A3F60), 0x4A7296 (4A7200), 0x4A83E8 (4A83C0), 0x4C5810 (4C56D0 and overlapping generated fragments), 0x4C5943 (4C5850), 0x59648B (596390).

0x596390 is normal delivery preparation/reset and calls 0x5AAC50 then 0x5B2350. Next-delivery continuation passes through 0x5965E0. End-over routine 0x597030 calls 0x471F90, which increments completed overs at match+0x58C6. Next preparation reads the updated counter, so no independent phase-cache refresh hook is required.

## Saved source references

- out_all/recomp_043.c:8012 — limited selector.
- out_all/recomp_046.c:13648 — XML loader, positions and adaptive handling.
- out_all/recomp_046.c:14383 — complete field preparation and ownership paths.
- out_all/recomp_017.c:7969,8194,8846,12359 — config getter, bank/family mapping, saved-peg validator.
- out_all/recomp_007.c:6425 — position getter.
- out_all/recomp_044.c:6271 — immediate field position setter.
- out_all/recomp_042.c:1089,2245 — preparation and end-over paths.
- out_all/recomp_014.c:28373 — end-over counter update.
- out_all/recomp_077.c:5040,5135 — inclusive integer RNG.
