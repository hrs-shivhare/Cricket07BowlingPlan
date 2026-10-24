# Cricket 07 BowlingPlan & Field Selections

**Version 2.5** — a 32-bit Windows ASI plugin for EA Sports Cricket 07 that adds planned CPU bowling rotations, format-specific powerplays, field-selection rules and reactive field variants.

Supports **5, 10, 20, 40, 45 and 50-over matches**. Other match lengths and Test matches retain their native bowling selection and powerplay behavior.

## Features

- Prescribed opening and death-over bowling sequences, with randomized middle-over plans.
- Bowler quotas and protection against the same bowler bowling consecutive team overs.
- Two-turn spells from the same end in the randomized phase of 40/45/50-over matches.
- Powerplays of 1/3/6/8/9/10 overs for the six supported lengths, irrespective of the game's 15/20-over powerplay setting.
- CPU Open, Normal and Defend fields selected by match length, over and, in T10/T20, the current striker's runs.
- Each CPU innings starts with field variant 2. A credited four, six or wicket selects a different variant from 1–5.
- Two-, four- and five-outside-circle phases for 40/45/50-over matches, including final CPU placement enforcement.

The plugin changes process memory while the game is running. It does **not** rewrite `Cricket07.exe`, player databases, rosters, XMLs, SAFs or other plugins. There is no INI/configuration file; the rules are compiled into the ASI.

## Requirements

- EA Sports Cricket 07, using original **32-bit x86 executable**.
- Ultimate ASI Loader

## Installation

1.  Extract the release archive.
2. Copy **`Cricket07BowlingPlan.asi`** into the game directory, beside **`Cricket07.exe`**. Replace an older BowlingPlan ASI if present.
3. Keep your ASI loader installed. Keep only one active copy of this plugin.
4. Launch the game and start a **new match** with the CPU bowling.
5. Check `Cricket07BowlingPlan.log` beside the ASI for the latest version's initialization line and an `APPLIED` entry.

Example location:
```text
Cricket07/
├── Cricket07.exe
├── Cricket07BowlingPlan.asi
├── Cricket07BowlingPlan.log    # Created when the plugin initializes
└── ...                        # Game files and ASI loader
```
To uninstall, close the game, remove `Cricket07BowlingPlan.asi` and restart. No roster or executable restoration is required.

## Bowling rotations

**B1–B6 are positions in the team's existing bowling order.** These plans control CPU bowling and simulation. 

### 5 overs
```text
Over:   1  2  3  4  5
Bowler: B1 B3 B5 B4 B2
```
Each of the five bowlers bowls one over.

### 10 overs

| Overs | Bowling plan |
|---|---|
| 1–3 | B1 B2 B3 |
| 4–8 | Random order: B3 once, B6 once, and B4/B5 one or two times each, with one of B4/B5 receiving two overs |
| 9–10 | B2 B1 |

Middle-phase allocations exclude the opening overs. B3 therefore bowls two overs in total. 

### 20 overs

| Overs | Bowling plan |
|---|---|
| 1–6 | B1 B2 B1 B2 B3 B4 |
| 7–16 | Random order: B3/B4/B5 each receive 2–3 overs; B6 receives 1–2 overs, totaling ten overs |
| 17–20 | B2 B1 B2 B1 |

Middle-phase allocations exclude opening overs. B1/B2 finish with four overs each.

### 40 overs

| Overs | Bowling plan |
|---|---|
| 1–8 | B1 B2 B1 B2 B1 B2 B3 B4 |
| 9–16 | B3 B4 B5 B3 B5 B4 B5 B4 |
| 17–32 | Randomized spells using the allocations below |
| 33–40 | B3 B1 B3 B2 B1 B2 B1 B2 |

### 45 overs

| Overs | Bowling plan |
|---|---|
| 1–9 | B1 B2 B1 B2 B1 B2 B1 B3 B4 |
| 10–18 | B3 B4 B5 B3 B5 B3 B4 B5 B4 |
| 19–36 | Randomized spells using the allocations below |
| 37–45 | B3 B1 B3 B2 B1 B2 B1 B2 B1 |

### 50 overs

| Overs | Bowling plan |
|---|---|
| 1–10 | B1 B2 B1 B2 B1 B2 B1 B2 B3 B4 |
| 11–20 | B3 B4 B5 B3 B5 B3 B5 B4 B5 B4 |
| 21–40 | Randomized spells using the allocations below |
| 41–50 | B3 B1 B3 B2 B1 B2 B1 B2 B1 B2 |

## Powerplays and CPU field families

`Open`, `Normal` and `Defend` are the game's field-resource family names. Selection is evaluated during delivery preparation.

| Match | Open-only powerplay | Normal after powerplay | Defend |
|---|---|---|---|
| 5 overs | Over 1 | Never | Overs 2–5 |
| 10 overs | Overs 1–3 | Overs 4–6 while striker runs are **≤20** | Overs 4–6 if striker runs are **≥21**; always from over 7 |
| 20 overs | Overs 1–6 | Overs 7–12 while striker runs are **≤30** | Overs 7–12 if striker runs are **≥31**; always from over 13 |
| 40 overs | Overs 1–8 | Overs 9–32 | Overs 33–40 |
| 45 overs | Overs 1–9 | Overs 10–36 | Overs 37–45 |
| 50 overs | Overs 1–10 | Overs 11–40 | Overs 41–50 |

## Reactive field variants

The field family and variant are separate choices. The native FastOdi, MediumOdi or SlowOdi bank is retained.

| Event | Variant behavior |
|---|---|
| Start of a CPU innings | Variant **2** |
| Credited batting four | Randomly choose a different variant from **1–5** |
| Credited batting six | Randomly choose a different variant from **1–5** |
| Wicket | Randomly choose a different variant from **1–5** |
| Dot, single, two, three or running four | Keep the variant |
| Change of strike, bowler, over or field family alone | Keep the variant number |

Each of the other four variants has a 25% selection probability. An immediate repeat is excluded; an older variant can return after an intervening selection. Variants 6–10 are not used by this policy.

## Outside-circle limits

For 40/45/50 overs, the plugin uses these limits irrespective of the game's 15/20-over powerplay setting:

| Match | Maximum 2 outside | Maximum 4 outside | Maximum 5 outside |
|---|---|---|---|
| 40 overs | Overs 1–8 | Overs 9–32 | Overs 33–40 |
| 45 overs | Overs 1–9 | Overs 10–36 | Overs 37–45 |
| 50 overs | Overs 1–10 | Overs 11–40 | Overs 41–50 |

## Logging and troubleshooting

The log is **`Cricket07BowlingPlan.log`**, beside the ASI. It is appended across runs: inspect the latest version/initialization entry and the lines after it.

| Log entry | Meaning / next step |
|---|---|
| `APPLIED` | All nine hooks were installed |
| SHA-256 `WARNING` followed by `APPLIED` | Hash differs or could not be read; checks passed and the plugin is active |
| `NOT APPLIED: unsupported executable PE image/layout.` | Architecture, PE format or expected image size did not match |
| `NOT APPLIED: entry signature conflict or hook installation failure.` | A function prefix differed, another hook conflicted, or memory-hook setup failed |
| `PLAN`, `B1`–`B6` | Planned overs and the player IDs/names assigned to each bowling-order position |
| `OPEN APPLIED` / `NEXT APPLIED` | Opening / next-over bowler selection applied |
| `FIELD SELECT` | Family, over, striker-run policy and native-to-selected preset IDs |
| `FIELD VARIANT` | Boundary/wicket counters that caused a variant change |
| `FIELD APPLIED` | Long-format outside-circle counts and repositioning |
| `FALLBACK` | A bowling plan or field-bank request could not be handled; inspect the reason |

If there is no new initialization entry, check the ASI location, whether the loader calls `InitializeASI`, and whether the game can write the log directory.

For a problem report, include the plugin version, the latest relevant log section, match length, CPU/human bowling ownership, whether the innings was loaded from a save, and reproduction steps. Include the executable SHA-256 if investigating compatibility:

```powershell
Get-FileHash .\Cricket07.exe -Algorithm SHA256
```
## Build from source

The supplied build script targets Windows x86 and was tested with **Zig 0.15.2**. From the directory containing `build.ps1`, supply your compiler and game paths:

```powershell
.\build.ps1 -Zig 'C:\Tools\Zig\zig.exe' -GameDirectory 'C:\Games\Cricket07'
```

A local development-workspace default exists, but it is not bundled with the source archive; use explicit paths on another computer.

The build script:

1. Builds `dist/Cricket07BowlingPlan.asi`.
2. Builds and runs short-format, long-format and native integration tests with assertions enabled.
3. Checks the supplied game's PE shape and all nine hook prefixes.
4. Warns about a reference-hash mismatch, and verifies the executable did not change during validation.

It **does not install the ASI automatically**. Copy the built ASI to the game directory after closing the game.

Source layout:

```text
src/                  Plugin, scheduling, field-policy and hook code
tests/                Scheduler and native integration tests
build.ps1             Windows x86 build and validation
README.md             Features and usage
VALIDATION_2.5.md     Current revision's validation notes
evidence/             Supporting reverse-engineering notes and resource data
```
