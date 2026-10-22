# SA People

A single-player social mod for **GTA San Andreas PC 1.0 US**. Every pedestrian gets a persistent
identity: a name, a job, a family and a memory of what you did. Meet people, keep a phone book,
text them, arrange meetings at real San Andreas venues, help strangers, make enemies, fall in love.

> Türkçe: GTA San Andreas için tek oyunculu sosyal etkileşim modu. Oyuncu rehberi:
> [docs/OYUNCU-REHBERI-TR.md](docs/OYUNCU-REHBERI-TR.md)

**Status:** beta. Download the ready-to-play build (setup, voices, menu pictures) from the
[Releases](../../releases) page. This repository contains the source code.

## Features

- Persistent identities for ambient pedestrians, deterministic names and families,
  continuity when the same person comes back.
- Persona-aware conversations (civilians, police, gangs, street people, medics), individual
  patience, puzzlement → number deleted → police arrest.
- SA CELL phone: contacts, family and connection pages, chat-style messages, meetings,
  settings and fully rebindable controls.
- Meetings at real venues, meal dates, relationships and partners, blood feuds.
- Random and sudden street events: help requests, passenger rides, pickpockets, muggings,
  hotheads.
- Honor system on the stats panel, RDR2-style menu theme with picture backgrounds.
- Four mod languages: Türkçe, English, Русский, Español.
- Story missions and cutscenes suspend the mod.

## Requirements

- GTA San Andreas PC **1.0 US** (`gta_sa.exe`, 14,383,616 bytes). Other executables are refused.
- [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader).

## Building

Requires Windows, Visual Studio (x86 C++ toolset) and the Windows SDK.

```bat
Build.cmd
```

`Build.cmd` builds `SA-People.asi`, runs the logic tests (`PeopleTests.exe`), builds the
checker tools, the layout preview generator and `Installer.exe`. Engine addresses and
references are documented in [BUILDING.md](BUILDING.md).

To make release packages (voice clips in `audio/` and pictures in `backgrounds/` are optional
and not part of this repository):

```powershell
powershell -ExecutionPolicy Bypass -File MakeRelease.ps1
```

## Layout

| Path | Contents |
| --- | --- |
| `source/Plugin.cpp` | ASI entry, hooks, persistence, rendering |
| `source/People.h`, `World.h` | Identity archive, bindings, spatial index |
| `source/Street.h`, `Persona.h`, `Dialogue.h` | Conversations and reactions |
| `source/Phone*.h/.inc` | Phone UI and input |
| `source/*Events.inc`, `Ambush.inc` | Encounters and sudden incidents |
| `source/Controls.h` | Rebindable keys |
| `source/Installer.cpp` | Single-file setup |
| `source/Tests.cpp` | Logic tests run by `Build.cmd` |
| `languages.tsv` | Translations (key, TR, RU, ES, EN) |

## Translations

`languages.tsv` is UTF-8 with five tab-separated columns: key, Turkish, Russian, Spanish,
English. Pull requests with corrections are welcome; the build fails on malformed rows.

## License

Code, text and documentation: MIT, see [LICENSE](LICENSE). Voice clips, menu pictures and icons
belong to their author. Not affiliated with Rockstar Games or Take-Two Interactive; no game
files are included.
