# SA People — NPC Interactions

**Version 0.19.0 beta · GTA San Andreas PC 1.0 US · Single-player**

Every pedestrian can become someone you know. Meet people, learn about their lives,
build friendships or rivalries, exchange messages and arrange outings. Their persistent
identities and relationships carry across game sessions.

[Download the latest release](https://github.com/te6ge/SA-People/releases/latest)

- [Normal ASI download](https://github.com/te6ge/SA-People/releases/download/v0.19.0-beta/SA-People-0.19.0-beta-normal.zip)
- [CLEO 5 plugin download](https://github.com/te6ge/SA-People/releases/download/v0.19.0-beta/SA-People-0.19.0-beta-CLEO.zip)

Choose one edition. Both use the same gameplay source; do not install them together.

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
- English, Turkish, Russian, Spanish, Simplified Chinese, Japanese and Polish. Story missions and cutscenes suspend social
  gameplay; scripted actors and recruited group members are excluded from ordinary chats.

## What's new in 0.19.0 — Friends Look Out for Each Other

- **Polish setup fix:** choosing Polish now applies the installer translation.
- **Paper Grove cashbook:** walk to the outdoor stand in front of CJ's house and press **L** (rebindable as
  `[Controls] Ledger`). A two-page paper ledger handles contributions and upgrades.
  Press **L** again to close. W/S or Up/Down select a row; E/Enter confirms; Q/A/Left goes back.
  Mouse controls also work. F6/P always opens the separate phone. Access requires being outdoors on foot near
  the stand, with no wanted level or active story mission. Contributions are $100,
  $1,000 or $10,000, with confirmation before spending.
- **Collectible home supplies:** purchased projects unlock native health and armor
  pickups on two tables outside CJ's house. Level I restocks 24 tracked game hours after collection;
  level II restocks after 12 hours. Native health/armor pickup behavior applies. Items
  stay available until needed; opening the ledger does not grant the rewards directly.
- **Expensive permanent upgrades:** food supplies cost $2,500/$6,500, engine workshop
  $5,000/$10,000, protective equipment $6,000/$12,000, weapon cache $20,000/$45,000.
  Second-level prices are additional. The workshop retains parked-driver repairs
  (+250/+500 engine health, capped at 1,000) with 24/12-hour cooldowns.
- **Weapon cache:** level I unlocks a pistol with 34 rounds; level II additionally
  unlocks a shotgun with 12 shells and Micro SMG with 60 rounds. Each weapon has its
  own 24-hour restock timer. The game's usual weapon-slot exchange controls apply.
- **Save-safe stock ownership:** runtime pickups are removed before native GTA saves,
  loads, restarts and during story missions. Only the mod's generation-checked handles
  are removed. Unlocks and collection clocks live in per-save SA People data, not as
  permanent additions to the GTA save's pickup pool. Engine ammo-only collection is
  treated as a stock claim too, so a revisit cannot refill the same ammo endlessly.
- **Lossless Grove migration:** archive V20 reads V19/older files. Existing V19 fund
  balances and purchased levels are kept; a legacy credit bridges the increased prices.
  Back up the entire SA-People folder before testing, as older plugins cannot read V20.
- **Polish language:** phone, dialogue, subtitles and setup support Polski (`pl`). The initial dialogue translation is machine-assisted and welcomes native-speaker corrections.
- **Keep in touch:** open a contact's actions and choose Keep in touch. Ask how they
  are doing and receive a reply matching illness, job hunting, work, family time or
  everyday life. Repeated check-ins have a two-game-hour cooldown.
- **Help a friend recover:** send $25 for medicine to a sick contact. Help shortens
  their remaining rest by up to six game hours and slightly builds trust; at least
  one hour of rest remains. It never buys honor or instantly cures them.
- **Job leads:** help an unemployed contact with their search. The remaining wait
  shortens by up to twelve game hours, with at least three hours remaining unless
  they were already closer to finishing. Assistance has a separate one-day cooldown.
- **Mutual friends:** trusted contacts can recommend someone they actually know.
  You learn the person's name, but still need to meet them and earn a phone-number
  exchange. Recommendations exclude enemies, deceased people and hostile contacts.
- **ASI startup compatibility:** configuration/resource loading and remaining game
  hooks now wait for the first game callback. ModLoader and loaders that do not call
  InitializeASI use the same deferred path. Repeated loader calls cannot self-chain.
  This targets an early-startup risk; the reported community crash is not yet proven fixed.
- All new phone actions and replies are available in all seven languages. The existing
  archive migrates existing per-save honor, contacts and relationships.
- Normal ASI and CLEO builds use the same gameplay source. Choose one edition.

The new gameplay and startup paths pass automated tests. Live-game testing of this
update and physical-controller testing are still pending.

## Previous update — 0.18.0


- **Personal grudges and motives:** robbery, violence and repeated insults can leave a
  persistent grievance. Temperament and fear influence whether a civilian keeps away,
  confronts CJ or asks him to make amends. Reactions use the actual person when encountered
  locally, with day-scale cooldowns and a three-encounter limit until a new harmful deed.
  New robberies no longer schedule the old random revenge ambush. Existing delayed revenge
  entries and family feuds remain compatible. This does not add automatic gang leadership
  changes or force every enemy to attack.
- **Making peace:** meet someone nearby on foot, unarmed, and use Defuse or the contact's
  Personal story → Make peace page. Apologies and an explicitly confirmed **$100** offer can
  reduce a grievance once per game day, after a cooling-off period. Compensation may only
  partly help; it does not restore friendship, romance, honor or erase a blood feud.
- **Lives outside CJ:** civilian contacts can fall ill, look for another job, start work
  again or spend time with family. Current circumstances affect availability and routine
  destinations; close contacts can send news by text. The first event is scheduled 3–24
  tracked game hours after initialization, with later events 4–8 game days apart. Active
  meetings, accepted favors and other owned interactions delay changes. These are abstract
  circumstances and route targets, not new hospital interiors, job minigames or illness animations.
- **Witnessed whereabouts:** choose a contact's Ask about someone action. A friendly
  informant needs a recent, actual nearby sighting of a socially connected contact before
  sharing a last-seen area. Information expires after a day. The temporary map marker is
  approximate and never spawns or tracks the person. Distrust, danger and current commitments
  can prevent an answer; repeated questions have a one-hour cooldown.
- **Neighborhood news and personal history:** a new phone tab and each contact's Personal
  story page show dated events. Direct experiences and news passed through actual social
  connections are recorded, with the informant identified when applicable. Unknown identities
  remain unknown. The feed is bounded to 192 entries and sightings to 128 records.
- New UI, messages and subtitles in all six languages. New unrecorded dialogue uses
  subtitles; the existing voice pack remains included. `[World] PersonalStories=0` disables
  the native personal-story director; it does not remove existing history.

- **GInput controller support in both editions:** bumper chords for NPC focus, phone and honor;
  gamepad navigation and Xbox/PlayStation interaction prompts. Focus keeps walking and camera
  input available while consuming its face-button actions, with release protection after closing
  the phone. Story missions and loss of focus suspend controller capture.

- **Meeting invitations:** an available contact's text now opens activity selection. Availability
  replies use the same trust, commitments and personal-boundary checks as scheduling. Any
  activity can be arranged while safely parked outside; moving vehicles remain excluded.
- **Conversation audio:** ordinary greetings no longer randomly play the Rickroll recording.

- **Separate worlds for GTA saves:** honor, contacts, relationships, messages, appointments and
  personal history now follow the GTA save you load. Loading a save restores its mod checkpoint;
  unsaved actions in another session do not carry into it. Save GTA after social progress to keep it.
- **Japanese:** phone, subtitles and setup now support Japanese (`ja`), with a Japanese font.
  The initial translation is machine-assisted and can be improved through community feedback.

**Existing players:** older versions used one shared `SA-People/people.dat` for every GTA save.
That file is preserved. Load the GTA save which should receive it, then choose phone → Settings →
Import shared archive and confirm. This replaces the current save's social data, not other saves.
It cannot undo cross-save crimes already mixed into the old archive. Other saves start with a
fresh social world unless you explicitly import into them too. The shared input and a
`people.pre-save-profiles.dat` backup remain available.

**Storage:** this version reads older archives and writes V20. Checkpoints are stored in
`SA-People/worlds/<save-fingerprint>/checkpoint.dat`, with matching backups. Each checkpoint is
associated with the save filename and contents; replacing a GTA save with a different file starts
another world. Copy the `worlds` folder when transferring your GTA saves to another installation.
The separate `live.dat` is a recovery copy; loading GTA uses its checkpoint, not unsaved live changes.
Keep independent backups of GTA saves and mod data before updating or downgrading.

**Validation:** automated logic, persistence, native safety, installation and package checks,
with six-language phone previews. Full live-game save transitions and story encounters still
require testing. Controller integration is beta and has not yet been tested with a physical pad.

## What's new in 0.17.1

- **Street traffic awareness:** eligible free-roam civilians can briefly flee from a
  fast player-driven vehicle predicted to pass dangerously close, with occasional warnings.
  Stationary/receding vehicles, safe clearance, different floors, gang members, scripted
  actors and active social participants are excluded. Existing combat and physical responses
  retain priority. Scans and reactions are bounded; this is additional avoidance, not a
  guarantee that pedestrians cannot be hit. Set `[World] StreetAwareness=0` to disable it.
- **More world context:** pedestrians can notice CJ's visible injuries and comment on
  late-night streets. Ten new subtitle entries are included in all five languages; no new
  voice recordings are bundled. These additions do not grant healing or automatic honor.
- **Relationship fix:** breaking up before engagement clears the old togetherness clock.
  Reuniting with the same person starts a new period instead of bypassing proposal readiness.
  A deceased spouse now has the correct status in the interaction guide too.
- **Compatibility fix:** combat posture refreshes preserve externally changed posture flags,
  as well as accuracy and shooting rate, when releasing temporary adjustments.

**Validation:** automated logic/native checks cover the new behavior and exclusions.
Live-game traffic scenarios remain unverified. The archive format remains **V17**; keep
backups and test these beta features in your game.

## What's new in 0.17.0

- **Relationship stages and marriage:** contacts now have a Relationship page in their
  phone actions. A committed partner needs at least six successful dates, a day of
  togetherness tracked from this update, high trust and a calm relationship before accepting
  a proposal. Stand nearby on foot with your weapon put away. The ring costs **$250**, only
  after explicit confirmation and acceptance. An accepted proposal is saved as an engagement.
- **Small ceremonies:** engaged partners can confirm a **$500 non-refundable** booking.
  Follow the normal meeting marker to the outdoor venue (Grove Street, Zero's area in San
  Fierro, or Come-A-Lot in Las Venturas). Arrive on foot, stay together and keep weapons put
  away for a brief exchange of vows. Marriage is recorded only when the sequence completes.
  Interrupted or expired ceremonies can be arranged again; the couple remains engaged.
- **Shared life:** married contacts have spouse-specific street remarks, outing lines and
  messages. Arguments still have consequences. An argument can end the romance without
  silently ending the marriage; separation requires a separate confirmed action. Existing
  children and memories are preserved. The family page identifies the spouse as CJ Johnson.
- **Experimental native combat assistance:** firearm-using gang fighters get modest,
  reversible shooting/posture preferences for assault, support and cautious roles. During
  native gang wars, a bounded scan helps eligible idle ambient gang members acquire visible
  hostile targets and spread their attacks. Existing combat, native group commands, injuries
  and story tasks are respected. No weapons, ammunition or health are granted. This is a
  limited combat director, not a replacement for GTA's whole AI. Set `[World] CombatAI=0`
  in the active settings.ini to disable it. Social gameplay still pauses during gang wars;
  the combat director operates separately when no story mission/cutscene is active.
- **Less repetitive dialogue:** 96 new UI/dialogue entries in all five languages, additional
  Grove/stranger/familiar-person remarks, weapon awareness, help memories and more outing
  topics. Recent lines are remembered per NPC and across nearby speakers. New unrecorded
  dialogue remains subtitled. Russian, Spanish and Chinese additions are machine-assisted.
  Chinese wrapping also accounts for wider characters so confirmation text stays readable.

**Save format:** this version imports V16 and older mod archives and writes **V17**.
`people.pre-0.17.dat` is kept before migration. To return to a 0.16 plugin, restore that
backup as people.dat while the game is closed. Keep an independent backup before testing.
**Validation:** logic, persistence, configuration and installer checks are automated;
new relationship pages also have rendered previews. Full live-game gang-war and wedding
scenarios remain unverified. Treat 0.17.0 as a beta test build.

## What's new in 0.16.2

- Simplified Chinese (`zh`) in the phone, subtitles and setup. The initial translation is
  machine-assisted and still needs native-speaker review; spoken recordings stay in English.
- ModLoader users can create/edit `settings.ini` beside `SA-People.asi`. When present,
  this explicit local configuration is used for language, keys, audio and interface options,
  and phone changes are saved there. Otherwise the game's `SA-People/settings.ini` is used.
  Example files remain defaults only. Relationship data always stays in the game root.
- Setup accepts `gta_sa.exe`, `gta-sa.exe` and `gta san andreas.exe`. The EXE setup checks
  the x86 PE header, image base and supported startup fingerprint instead of file size alone.
  This does not add support for other game versions or guarantee compatibility with repacks.
- Startup diagnostics: `%TEMP%/SA-People-Startup.log` records loader entry/initialization;
  `%TEMP%/SA-People-Setup.log` records setup checks and caught installation errors. Include
  these when the normal mod log is missing. Chinese font rendering requires Windows CJK fonts.

## What's new in 0.16.1

The main/pause menu redesign is now opt-in. The setup has a **With new menu** checkbox;
fresh installs leave it unchecked. Selecting an existing game folder loads its current
menu setting. Manual and ModLoader installs default to the original GTA menu.
The phone, interaction prompts and honor panel remain available with either menu style.

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
Close the target game before installing. Back up your game saves and the entire `SA-People` folder.

## CLEO edition installation

Requires CLEO 5 and an existing ASI Loader. These dependencies are not bundled in the CLEO ZIP.
With the game closed, copy the **contents** of `Copy to game folder` beside `gta_sa.exe`.
The plugin must end up at `CLEO/cleo_plugins/SA-People.cleo`. Remove any `SA-People.asi`
from the game root or ModLoader first. Keep the game's `SA-People` data folder and settings.
To remove this edition, delete only `CLEO/cleo_plugins/SA-People.cleo` while the game is closed.
The normal install methods below apply to the ASI download.

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

## Optional main/pause menu redesign

In the setup, tick **With new menu** to use SA People's menu theme. Leave it unticked to
keep the original GTA menu. When selecting an existing installation, the setup displays
its saved preference. Your explicit checkbox choice updates only this menu setting;
other settings and relationships are kept.

For manual or ModLoader installation, close the game and edit **SA-People/settings.ini**
in the game folder. Under `[Interface]`, choose:

```ini
[Interface]
ModernMenu=1
```

Use `ModernMenu=0` for the original menu. Edit the existing section instead of adding a
duplicate. Restart the game after changing it. On a fresh copy installation, you can edit
`SA-People/settings.example.ini` before the first launch instead. Existing settings take
priority over example files during updates; Install.cmd also keeps existing preferences.

For automated setup, `--silent "<game folder>" --new-menu` enables it and
`--silent "<game folder>" --original-menu` disables it. Omitting either option preserves
an existing menu setting; a fresh silent installation defaults to the original menu.

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
For ModLoader installs, an explicit `settings.ini` beside the mod's ASI takes precedence.
Without it, edit the game's `SA-People/settings.ini`. Never edit the example after the first launch.
The active file path is recorded as `SETTINGS active=` in the mod log. Updates preserve settings.

To negotiate with a hostile rival, approach a visible member within eight metres, hold
focus and greet or calm down. Honor, group size and whether you are aiming affect the
result. Do not expect a rejected gang member to become an ordinary new contact.

## Updating, saves and removal

Settings, relationships, backups and logs live in the game's `SA-People` folder, regardless
of edition or installation method. Version 0.19 writes archive format V20 and imports earlier
archives through the confirmed legacy-import option. The original shared `people.dat` is
preserved. New per-save checkpoints are stored under `worlds`; back up the complete folder.
Do not expect an older plugin to read V20 data. Restore its matching pre-update backup if
you downgrade. Previously mixed events in a shared archive cannot be assigned retroactively
to their original GTA saves.

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
`BuildAll.cmd`, release scripts and installer tests. Install Visual Studio with the desktop
C++ workload and a Windows SDK, then run `BuildAll.cmd` to build and test both x86 editions.
Outputs are `build/SA-People.asi` and `build/SA-People.cleo`.

Release media is distributed in the download, not tracked in Git. To reproduce the full
package, supply the release's audio files in `audio/`, its three menu pictures in
`backgrounds/` and the upstream loader in `loader/dinput8.dll`, then run `MakeAllReleases.ps1`.
Run `python tests/InstallerTests.py` to exercise the EXE/CMD installers in disposable folders.
The public release has two downloads: `*-normal.zip` (the complete ASI package) and
`*-CLEO.zip` (the CLEO plugin with a direct `Copy to game folder` layout). Other generated
archives are build and test artifacts.

## Credits and license

Source code: MIT; see `LICENSE.txt`. Bundled Ultimate ASI Loader: ThirteenAG, MIT; its
license notices are retained with the loader. Supplied audio and artwork have separate
rights and are not automatically covered by the source license. NPC recordings were supplied
by the author. Developed with AI assistance. Not affiliated with Rockstar Games.

## GInput controls (normal ASI and CLEO)

Install [GInputSA from Silent's site](https://silentsblog.com/mods/gta-sa/#ginput) separately.
It is optional and is not bundled. Without it, the existing keyboard controls remain available.

| Action | Xbox | PlayStation |
| --- | --- | --- |
| Focus NPC while walking or parked | Hold LB + RB | Hold L1 + R1 |
| Greet / provoke / defuse / rob | Focus + A / B / X / Y | Focus + Cross / Circle / Square / Triangle |
| Repair nearby event vehicle | Focus + D-pad Right | Focus + D-pad Right |
| Open phone | LB + D-pad Up | L1 + D-pad Up |
| Honor panel | Hold LB + D-pad Down | Hold L1 + D-pad Down |
| Phone navigation | D-pad | D-pad |
| Select / back / close phone | A / B / Y | Cross / Circle / Triangle |

Prompts follow GInput's button style. Gamepad chords are fixed; in-phone rebinding changes
keyboard keys. Existing keyboard bindings continue to work. To disable controller integration
set `[Controller] Enabled=0` in settings.ini. `Glyphs=-1` follows GInput; `0` forces Xbox and
`1` forces PlayStation. Restart after changing these settings. Only the single-player primary
pad is used. Synthetic controller/ABI tests do not replace live testing with a physical pad.

For maintainers: `BuildAll.cmd` builds and tests both editions from the same gameplay source;
`MakeAllReleases.ps1` produces the two separate downloads. The CLEO ZIP has `Copy to game folder`
at its top level and contains no ASI or loader DLL.
