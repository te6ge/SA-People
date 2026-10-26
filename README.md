# SA People — NPC Interactions

**Version 0.14.2 beta · GTA San Andreas PC 1.0 US · Single-player**

Every pedestrian can become someone you know. Meet people, learn about their lives,
build friendships or rivalries, exchange messages and arrange outings. Their persistent
identities and relationships carry across game sessions.

[Download the latest release](https://github.com/te6ge/SA-People/releases/latest)

## Features

- Persistent names, personalities, family connections and memories.
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

## What's new since 0.13.3

NPC activity proposals, multi-day personal follow-ups, street recognition, gradual
introductions, interior dialogue and more Grove greetings. Rival gang detection,
negotiation routing and combat after failed negotiations have been revised. The optional
installer has a new interface and four selectable languages. A ModLoader layout is included.

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

Run `Manual/Install.cmd` and select the game folder containing `gta_sa.exe`.

Alternatively, copy `Manual/SA-People.asi` and the contents of `Manual/SA-People` into the
game folder. **Do not replace an existing `SA-People/settings.ini` when updating.** Never
delete your `people.dat` files. If the game has no ASI Loader, the supplied
`Manual/loader/dinput8.dll` is available; do not overwrite an existing loader DLL.

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

To negotiate with a hostile rival, approach a visible member within eight metres, hold
focus and greet or calm down. Honor, group size and whether you are aiming affect the
result. Do not expect a rejected gang member to become an ordinary new contact.

## Updating, saves and removal

Settings, relationships, backups and logs live in the game's `SA-People` folder, regardless
of installation method. Versions 0.14.x use archive format V14 and import earlier archives.
The first upgrade keeps `people.pre-0.14.dat`. Older plugins cannot read V14: restore the
pre-update archive as well as the older plugin if you downgrade.

To uninstall a root installation, run `Manual/Uninstall.cmd` or remove `SA-People.asi` from
the game root. For ModLoader, remove `modloader/SA-People` while the game is closed. Keep
the persistent data folder if you want your relationships available for a later reinstall.

## Support and beta status

Report bugs at [GitHub Issues](https://github.com/te6ge/SA-People/issues). Include the mod
version, executable version, installation method, other mods, steps to reproduce and
`SA-People/SA-People.log`. Do not post your entire game folder or personal save data.

The source build, logic regressions and 61 disposable-folder installer checks passed.
Native gameplay paths, including the latest gang combat changes, still need wider in-game
testing. This is a beta, not a compatibility guarantee for every mod combination.

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
rights and are not automatically covered by the source license. No new NPC voice pack is
included in 0.14.2. Developed with AI assistance. Not affiliated with Rockstar Games.
