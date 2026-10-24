# SA People 0.13.3 (beta)

A single-player social mod for **GTA San Andreas PC, version 1.0 US** (`gta_sa.exe`, 14,383,616 bytes).
Meet the pedestrians of San Andreas, learn who they are, keep a phone book, arrange meetings,
message and call them, help strangers, make enemies, fall in love. People remember what you did.

> **Beta.** The logic is covered by automated tests, but most in-game flows have had limited play
> time. Keep a backup of your savegames. Back up `SA-People/people.dat` too.

## Requirements

- GTA San Andreas PC **1.0 US**. Other executables (1.01, 3.0, Steam, Rockstar Launcher, SA-MP, open.mp)
  are refused on purpose: the mod patches fixed addresses.
- An ASI loader. [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) (MIT) is included:
  the setup installs it as `dinput8.dll` only when the game folder has no loader yet.
- Windows 10/11. No other dependencies, no internet, no AI model.

## Install

1. Optional installer: download `SA-People-0.13.3-beta-Setup.exe` separately from GitHub Releases,
   run it and select your GTA San Andreas folder.
   (Windows may show a SmartScreen warning because the setup is not signed: More info → Run anyway.)
2. Or extract the archive and run `Install.cmd` with your GTA San Andreas folder.
   It copies `SA-People.asi` and the `SA-People/` data folder, and backs up anything it replaces.
3. Start the game.

Manual install: copy `SA-People.asi` beside `gta_sa.exe` and the `SA-People` folder next to it.
Existing `settings.ini` and `people.dat` are kept. Bundled sound and picture files are updated;
additional files with other names are kept. Existing `dinput8.dll` is never replaced.

Uninstall: run `Uninstall.cmd` (keeps your relationships file) or delete `SA-People.asi`.

## Voices and menu pictures

The full package includes CJ voice clips and AI-generated lines, and three original
menu pictures. You can replace or add files:

- **Voices**: `SA-People/audio/`. Names are listed in `SA-People/audio/README.txt`; a missing
  file is skipped silently.
- **Menu pictures**: `SA-People/backgrounds/` (JPG/PNG). Files starting with `gtasanandreas` are
  shown 95% of the time, every other picture 5% of the time.
## Controls

| Key | Action |
| --- | --- |
| Hold **Left Alt** | Focus the nearest pedestrian |
| **Alt + G** | Greet / keep talking / accept an offered event |
| **Alt + H** | Provoke |
| **Alt + J** | Calm down / apologise |
| **Alt + R** | Rob |
| **F6** | Phone (arrows, Enter, Backspace, Esc) |
| **Tab** (held) | Honor and fitness panel |
| **F7 / F8** | Phone: contacts / meetings |

No arrow or F keys? **P** opens the phone and **W/A/S/D**, **E** (select) and **Q** (back) drive it.
Every key can be changed in phone → Settings → SA People Control Options.

The mod pauses itself during story missions and cutscenes.

## What it does

- Every pedestrian gets a persistent identity (name, job, family, memories). Names are only learned by talking.
- Conversations depend on who you talk to: civilians, police, gangs, street people, medics.
  Everyone has a different patience; provoke a known person enough and they delete your number.
  Provoke police three times and they come for you.
- Phone with contacts, family and connection pages, text messages, settings, meetings at real
  San Andreas venues, radar markers, and a relationship system.
- Sudden incidents: pickpockets, muggings in progress, hotheads looking for a fight.
- Help encounters (dead battery, directions, bus fare), passenger rides with fares,
  begging and street requests, night offers, house visits.
- Delayed consequences: people you helped text you (or send money) hours later; someone you
  robbed may send people after you.
- Honor instead of respect on the stats panel. Killing someone you know can start a blood feud.
- Mod languages: Türkçe, English, Русский, Español (phone → Settings). The game's own language
  option in the main menu is untouched.
- Street dealers and other pedestrians talk through the mod instead of the game's Y/N prompt
  (ModTalkOverNative=0 in settings.ini restores the game's prompt).

See `CHANGELOG.md` for details and `OYUNCU-REHBERI-TR.md` for the Turkish player guide.

## Known limitations

- Only the 1.0 US executable is supported.
- Native hooks have not been verified against every ASI/graphics mod. If another mod patches the
  frontend menu, this mod declines to install its menu theme (set `ModernMenu=0` in `settings.ini`).
- Arrest by police means a wanted level, not the scripted "busted" screen.
- Data format: `SA-People/people.dat`. Newer versions read older ones, older ones cannot read newer.

## Reporting problems

Include `SA-People/SA-People.log` and what you were doing. Please do not post `people.dat`.

## License

Source code: see `LICENSE.txt`. Not affiliated with Rockstar Games. Grand Theft Auto and
San Andreas are trademarks of Take-Two Interactive. No files from the game are distributed.
