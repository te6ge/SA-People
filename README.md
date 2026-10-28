# SA People — NPC Interactions

**Version 0.16.0 beta · GTA San Andreas PC 1.0 US · Single-player**

Every pedestrian can become someone you know. Meet people, learn about their lives,
build friendships or rivalries, exchange messages and arrange outings. Their persistent
identities and relationships carry across game sessions.

[Download the latest release](https://github.com/te6ge/SA-People/releases/latest)

## Features

- Connected pedestrians can meet during walks; witnesses remember deeds and pass news
  through nearby social connections. Friends can visit to repair a car, give first aid or offer a pickup.
- Repeated requests and unsafe driving affect social boundaries. Accident victims can receive
  first aid, a requested ambulance or a ride to hospital. These native flows still require wider live testing.
- Conversation holds, passenger-seat reservations and bounded route retries protect active interactions.
- Persistent names, personalities, family connections and memories.
- Partial NPC voice pack: four female and four male voices, assigned consistently by identity.
  Recorded lines cover greetings, refusals, thanks, ride requests and selected relationship
  moments. Missing lines keep their subtitles. English recordings retain translated subtitles.
- Introductions shaped by temperament, trust, honor and previous encounters. People do
  not automatically share their job, private life or number during a first conversation.
- SA CELL phone with contacts, contextual messages, meetings and rebindable controls.
- NPC-proposed meals, pool games, drives and walks, with meeting markers.
- Personal follow-ups after helping someone, sometimes leading to a new introduction.
- Occasional recognition, greetings and approaches from familiar people.
- Street help requests, paid rides, robberies, friends, partners and enemies.
- Close-range rival gang negotiations. A truce does not make you friends; a failed
  negotiation can lead to an attack using the gang members' existing weapons.
- Honor on the TAB panel, contextual street remarks and interior patron dialogue.
- English, Turkish, Russian and Spanish. Story missions and cutscenes suspend social
  gameplay; scripted actors and recruited group members are excluded from ordinary chats.

## What's new in 0.16.0

- **Neighborhood reputation**: witnessed deeds contribute to a district summary (Grove,
  Idlewood, Downtown, Vinewood, San Fierro, Las Venturas and more). Street reactions require
  the NPC to have witnessed an incident or heard about it through a social connection.
  Personal friendship still matters. Meals and night visits can get cheaper where CJ is liked;
  district opinion softens over time.
- **Friends' requests**: a close friend sometimes texts you for a favor: a ride from their home,
  a word with someone they fell out with, or a small loan. Press Enter in the chat to review it.
  Review and accept or decline requests even after newer messages arrive. Ignoring or declining
  costs no trust; breaking an accepted ride/talk promise hurts the friendship. A loan keeps its
  principal in persistent data and returns the exact amount after 8–20 game hours. One friend
  request or pending repayment is tracked at a time; a full event queue cannot lose that debt.
- **My life**: a new phone page with this week's new acquaintances, good deeds, outings and
  robberies, your closest friend, your enemy, your partner, how a district sees you and the next
  birthday coming up.
- **Birthdays**: friends text you on their birthday. Stand near them on foot, open their
  phone contact's actions, choose **Give birthday gift** and confirm the $50 cost. A normal
  greeting never spends money. Only one gift can be given per birthday.
- The download has a single manual folder (no duplicate copy-ready tree) and ships
  `settings.example.ini` so copying it never overwrites your settings.

## What's new in 0.15.2

- Two additional NPC voice profiles with 53 newly connected recordings (118 NPC clips total).
- Compact 64 kbps mono MP3 speech assets; original recordings are preserved outside the
  download. Existing WAV-only installations remain supported. Volume settings are unchanged.


- Retained meeting contacts temporarily stop ambient hostility and friend-defense reactions,
  including spontaneous attacks on police. Their previous disposition is restored on release.
- Existing offensive attack responses yield to the outing; injuries, falls, arrests and
  unknown scripted tasks retain priority.
- Known ambient idle tasks no longer block passenger boarding after a meeting starts.
- Interrupted outings log the task types involved for diagnosis.

The reported police/boarding scenario still requires a live reproduction test.

## What's new in 0.15.1

Control bindings are now listed in the supplied settings files and automatically added
when missing from an existing installation. Custom bindings and other settings are kept.
If F6 opens another mod's save menu, use P or change the Phone entry as described below.
This does not disable or override other mods' hotkeys.

## What's new in 0.15.0

- Connected NPCs can acknowledge relatives, friends and rivals during outings.
- Witnesses react to nearby incidents and share memories through local social connections.
- Useful friends can visit to repair a car, provide first aid or arrange a pickup.
- Repeated requests and dangerous driving affect trust and willingness to help.
- Accident assistance includes first aid, ambulance requests and hospital transport.
- 65 new NPC voice clips across three female and three male voice profiles; missing
  recordings remain subtitled. NPC voice volume can be adjusted separately.
- Conversation holds, reserved passenger seats and bounded navigation retries.
- Familiar people are excluded from random hostile encounters; calm-down availability revised.
- Manual installation includes a ready-to-copy game-folder layout.

These are beta features. Automated tests and an isolated startup check passed; complete
live scenarios and SilentPatch coexistence have not yet been verified.

## Requirements

This build targets **classic GTA San Andreas PC 1.0 US**. Other executable versions,
Steam/Rockstar builds, Definitive Edition and multiplayer clients are not supported.
Close the target game before installing. Back up your game saves and `SA-People/people.dat`.

## Installation — choose ONE method

The complete ZIP contains `SA-People-Setup.exe`, `Manual`, `ModLoader`, this README and
license notices. Do not install both copies of `SA-People.asi` at the same time.

### ModLoader

1. Have an ASI Loader and [Mod Loader](https://github.com/thelink2012/modloader/releases)
   installed in your game.
2. Copy `ModLoader/SA-People` from this ZIP into the game's `modloader` folder.
3. The result must be `modloader/SA-People/SA-People.asi`, with its resource folders beside it.

When switching from a root installation, remove or disable the old `SA-People.asi` beside
`gta_sa.exe`. **Keep the game's `SA-People` data folder**: existing relationships and settings
are reused. Restart the game after enabling, disabling or updating this ASI; hot unloading
is not supported. The example settings in the ModLoader folder do not replace your settings.

### Manual installation — no setup EXE required

1. Open the `Manual` folder in the ZIP.
2. Copy `SA-People.asi` and the `SA-People` folder into the game folder beside `gta_sa.exe`.
   If you have no ASI Loader yet, also copy `loader/dinput8.dll` there:

   ```text
   GTA San Andreas/
     gta_sa.exe
     dinput8.dll
     SA-People.asi
     SA-People/
       languages.tsv
       settings.example.ini
       audio/
       assets/
       backgrounds/
       licenses/
   ```

Do not copy the outer `Manual` folder itself. Merge the `SA-People` folder
when updating. This layout contains no `settings.ini` or relationship archive, so your
existing settings and people are kept. On a new installation, settings are created on launch.
**Keep your existing ASI Loader if you already have one; skip the bundled `dinput8.dll`
instead of replacing an existing loader DLL.** Disable an old ModLoader copy first.

Alternatively, run `Manual/Install.cmd` and select the folder containing `gta_sa.exe`.
Never delete your `people.dat` files when updating.

### Optional Windows installer

Run `SA-People-Setup.exe`, choose the game folder and click Install. Its language selector
is in the bottom-right corner. The installer is unsigned; the manual and ModLoader methods
are available if you prefer not to run it. Installer language and in-game mod language are
separate settings. Disable any previous ModLoader copy before using this root installer.

## Controls

- Hold **Left Alt**: focus a nearby pedestrian.
- **G**: greet / negotiate · **H**: provoke · **J**: calm down · **R**: rob.
- **F6** or **P**: phone. Navigate with arrows / W-A-S-D, Enter / E and Backspace / Q.
- Hold **TAB**: honor panel.
- Change language and bindings in phone → Settings.
- Type **IMARTHURMORGAN** during free roam, with the phone closed, to set honor to 100.

### Change the phone key without opening the phone

Close the game, then open `SA-People/settings.ini` in the game folder with a text editor.
Under `[Controls]`, change `Phone=F6,P` to:

```ini
[Controls]
Phone=F2,P
```

Restart the game. F2 and P now open the phone; SA People no longer uses F6 for the phone.
Choose another unused key if F2 is also assigned by another mod. Use `Phone=F2,NONE` if
you want only F2. Existing settings gain missing control entries after the first launch
of 0.15.1; you can also add `[Controls]` and `Phone=F2,P` yourself before launching.
For ModLoader installations, edit the game's persistent `SA-People/settings.ini`, not
`settings.example.ini` inside the mod folder. Updating does not replace your settings.

To negotiate with a hostile rival, approach a visible member within eight metres, hold
focus and greet or calm down. Honor, group size and whether you are aiming affect the
result. Do not expect a rejected gang member to become an ordinary new contact.

## Updating, saves and removal

Settings, relationships, backups and logs live in the game's `SA-People` folder, regardless
of installation method. Version 0.16 uses archive format V16 and imports earlier archives.
The first upgrade keeps `people.pre-0.16.dat` (and `people.pre-0.15.dat` from earlier upgrades). Older plugins cannot read V16: restore the
pre-update archive as well as the older plugin if you downgrade.

To uninstall a root installation, run `Manual/Uninstall.cmd` or remove `SA-People.asi` from
the game root. For ModLoader, remove `modloader/SA-People` while the game is closed. Keep
the persistent data folder if you want your relationships available for a later reinstall.

## Support and beta status

Report bugs at [GitHub Issues](https://github.com/te6ge/SA-People/issues). Include the mod
version, executable version, installation method, other mods, steps to reproduce and
`SA-People/SA-People.log`. Do not post your entire game folder or personal save data.

The source build, logic regressions, exact-principal loan persistence/full-queue regressions,
explicit gift confirmation, pending-request UI checks and disposable-folder installer checks passed.
Native gameplay paths, including friend services, accident assistance and witness reactions,
still need wider in-game testing. SilentPatch coexistence has not yet been verified. This is a beta, not a compatibility guarantee for every mod combination.

## Source and building

The [source repository](https://github.com/te6ge/SA-People) contains the C++ source,
`Build.cmd`, `MakeRelease.ps1` and installer tests. Install Visual Studio with the desktop
C++ workload and a Windows SDK, then run `Build.cmd` to build the x86 plugin, helpers and
simulations. The plugin is written to `build/SA-People.asi`.

Release media is distributed in the download, not tracked in Git. To reproduce the full
package, supply the release's audio files in `audio/`, its three menu pictures in
`backgrounds/` and the upstream loader in `loader/dinput8.dll`, then run `MakeRelease.ps1`.
Run `python tests/InstallerTests.py` to exercise the EXE/CMD installers in disposable folders.
The only public release asset is the `*-complete.zip`; other generated archives are build
and test artifacts.

## Credits and license

Source code: MIT; see `LICENSE.txt`. Bundled Ultimate ASI Loader: ThirteenAG, MIT; its
license notices are retained with the loader. Supplied audio and artwork have separate
rights and are not automatically covered by the source license. NPC recordings were supplied
by the author. Developed with AI assistance. Not affiliated with Rockstar Games.
