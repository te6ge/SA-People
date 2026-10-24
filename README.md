# SA People — NPC Interactions · 0.13.3 beta

A single-player social mod for **GTA San Andreas PC 1.0 US**. Every pedestrian gets a
persistent identity: a name, a job, a family and memories of what you did. Meet people,
keep contacts in your phone, text them, arrange outings, help strangers and build relationships.

## Download and update

- **Existing installation:** download [the 0.13.3 manual update ZIP](downloads/SA-People-0.13.3-update.zip?raw=true).
  Close GTA SA and copy the ZIP contents into the folder containing `gta_sa.exe`, replacing
  matching mod files. No EXE is required. The update keeps your settings and relationships.
- **First installation:** install the full package from [Releases](../../releases) first,
  then apply the update ZIP above. The update ZIP contains the latest plugin, translations
  and one repaired voice file; it is not the full voice/artwork/ASI-loader package.
- Back up `SA-People/people.dat` and your game saves before updating.

## What's new

- Fixed dealer conversation conflicts: correct pedestrian type, scripted Y/N prompt handoff
  and restoration of native conversation permissions during story missions.
- Contextual phone replies and consecutive-message spam detection. A new event or a different
  message starts a fresh conversation instead of triggering a mistaken repeat complaint.
- Revised passenger boarding, more automatic outing conversation, familiar farewells and a
  rest period after outings so contacts do not immediately ask for another ride.
- Three distinct greetings earn honor; three distinct provocations reduce it. Honor affects
  willingness to talk. Active street offenders are distinguished from innocent pedestrians.
- Honor audio queue fixes, more varied street remarks and the `IMARTHURMORGAN` honor cheat.

See the English [changelog](CHANGELOG.md) and [validation notes](docs/VALIDATION-0.13.3.md).

## Features

- Persistent identities, families, learned details, relationships, friends and enemies.
- Conversations shaped by personality, trust, patience and pedestrian role.
- SA CELL phone with contacts, messages, meetings, family connections and rebindable controls.
- Outings at San Andreas venues, meals, drives, walks, dates and partners.
- Street help requests, paid passenger rides, pickpockets, muggings and other incidents.
- Honor panel, street interaction prompts, notifications and a themed frontend.
- Turkish, Russian, Spanish and English mod text.
- Social gameplay suspends during story missions and cutscenes.

## Controls

Hold **Left Alt** to focus a pedestrian. **G** greets, **H** provokes, **J** calms down,
**R** robs. **F6** opens the phone; hold **TAB** for the honor panel. Keys can be rebound
in phone settings. Player guide: [Turkish](docs/OYUNCU-REHBERI-TR.md).

## Compatibility

The supported target is **classic PC 1.0 US**. Steam/Launcher executables, 1.01, 3.0,
Definitive Edition, SA-MP and open.mp are not supported targets. An ASI loader is required;
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) is bundled with full releases.
This remains a beta; native task behavior can vary with other installed mods.

## Build and contribute

On Windows with Visual Studio's x86 C++ toolset and Windows SDK, run `Build.cmd`.
It builds the ASI, runs logic tests and generates interface previews. See
[BUILDING.md](BUILDING.md) for engine references and [MakeRelease.ps1](MakeRelease.ps1)
for packaging. Voice files and menu backgrounds are distributed separately from the source.

Translation corrections are welcome: `languages.tsv` is UTF-8 with columns
`key`, Turkish, Russian, Spanish and English. For bugs, include your game executable
version, other mods, reproduction steps and `SA-People/SA-People.log`.

## License and credits

Code, text and documentation: MIT, see [LICENSE](LICENSE). Optional voices and artwork
have separate provenance. Some voices were created with AI; AI tools also assisted development.
Not affiliated with Rockstar Games or Take-Two Interactive.
