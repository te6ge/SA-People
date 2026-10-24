# Building

Requirements: Windows, Visual Studio with the x86 C++ toolset, and Windows SDK.
Run Build.cmd from the project folder. It builds SA-People.asi, the pure logic tests,
layout previews, AudioCheck.exe and GlyphCheck.exe. Build.cmd runs the pure tests;
hardware-dependent audio and graphics checks are separate programs.

Build.cmd also builds the Windows installer stub with an embedded asInvoker manifest.
Run MakeRelease.ps1, then `python tests/InstallerTests.py` to verify both packaged
installers in disposable fake game folders. See INSTALLER-VALIDATION-TR.md for scope.

Audio assets are optional for a source-only build. The current distribution has 80 audio files, including high.mp3 and low.mp3.
Tests check their names. Audio assets are separate from the program source;
this project does not assign rights to the original game recordings.

At runtime the ASI lives beside gta_sa.exe. Its data directory is SA-People/ and
contains languages.tsv, audio/, assets/, backgrounds/ and settings.ini. people.dat is user state and must
not be included in a distribution. The loader must support InitializeASI.

## Data and localization

Current save format: SAPEOPLE 13, reading versions 1 through 12 as well. Relationships,
resentment, learned information, event memory and up to eight appointments persist.
Appointments use foreground gameplay time; menus and missions suspend their timers.
The upgrade creates people.pre-0.12.dat. Restoring an older ASI also requires restoring
its matching data backup; builds predating V13 cannot read format 13.

V11 adds a sparse LIFE section for stable home/work/leisure anchors, completed
outings, conversation memories and invitation/remote-reaction budgets, plus a MAIL
section containing at most 256 messages. Existing identity records stay in order.
Appointments store a selected activity; reaching the meeting point begins the
activity and only successful completion awards the meeting/romance progress.
Phone messages and calls use built-in, translated dialogue choices. No external runtime or model is required.
Private queue files, model weights, archive files and local runtime paths do not
belong in source/distribution packages. C++ code contains no HTTP/model inference.

V10 adds household status, good-date counts, once-per-date home visits and a family
record. Previously generated spouses of existing romantic contacts become former
spouses; their identity and existing children stay in the archive. New single people
do not acquire a generated spouse or adult child. Romance excludes married people and
generated relatives, and allows one living established partner. Family planning is
explicit: five good dates, two home visits, trust >=70 and no active conflict. Birth
uses a compressed seven-day gameplay clock, persists through restarts and records a
child with the surname Johnson. There is no child actor, pregnancy model or childcare
simulation in this version. Offline time does not advance pregnancy.

V7 stores honor, an archive timestamp, social news, family origins and a persistent
connection graph. A root's five existing named adult relatives can become separate native
actors; their canonical names and person IDs match the original family tree. Population
restoration still obeys the existing pool and off-camera limits, so family actors do not
all appear immediately. Connections propagate trust, resentment and dialogue memories.
Only damage attributed by the game to CJ or CJ's current vehicle produces kill penalties.

Appointment timers pause in menus and story gameplay. Across process restarts, real offline
time is reconciled from the saved timestamp. Legacy appointments without a timestamp are
closed once instead of leaving stale invitations active. ArchiveCheck.exe verifies an existing
archive using an in-memory round trip and never modifies its input file.

New encounters include donations, explicit requests for drug money, paid passenger rides
and house invitations. Passenger rewards require actual boarding and exit at the destination.
House invitations require a nearby loaded residential entry/exit; native transitions handle
the interior. The fee and six-hour time skip happen only inside the selected home. Exit
access is kept until the player returns outside. No explicit scene or game save is created.
Story missions and running cutscenes suspend population scanning and returns, interaction,
events, phone input and the custom TAB panel; only owned tasks are released.

TAB replaces the vital-stats presentation with an independent honor bar and native stamina,
muscle, fat, lung capacity and sex appeal. The game's respect value is never modified.

The optional frontend theme uses Interface/ModernMenu (default 1). It verifies 27 original
call destinations before patching; an incompatible frontend leaves the original theme intact.
Plain ASCII menu labels use the Unicode glyph renderer. GXT extensions and inline tokens
retain native rendering. Menu content and hitbox positions stay under the game's control.
The menu PNG previews demonstrate the palette and typography with illustrative labels;
native entry positions depend on the current game screen.

Model gender traits are read from the installed data/peds.ide, with native metadata as fallback.
Reattachment uses known identity, model, ped type, area and a 30-metre location window;
it rejects ambiguous archived or loaded lookalikes. This is a continuity heuristic, not proof
that the base game preserved a despawned actor. Replaced skins need accurate IDE metadata.

languages.tsv is UTF-8 with five tab-separated columns: key, Turkish, Russian,
Spanish, English. Source sentence templates use {0}, {1}, etc. Placeholder captures
are translated recursively for job names and other known fields. The phone and the
native Options extension share the Interface.Language setting: tr, ru, es, en.
Only the mod is localized; the original game's text and recorded CJ voices are retained.

Text is rasterized with the Windows Tahoma font and rendered through a private
Direct3D 9 texture cache. Render state is captured and restored around each draw.
This supports Cyrillic without changing the game's font textures or GXT files.

## Engine references

The adapter targets GTA SA 1.0 US and refuses known multiplayer clients.
Existing hook destinations and menu entries are checked before patching.
The Options extension adds its own action in a free menu slot; if patching fails,
it rolls back its menu changes. The phone remains a language-selection fallback.

Plugin-SDK reference commit: 15f15b60bbf74c106e1b496ff92c98764abf4605.
CPopulation::IsFemale reads model metadata at 0x611490. CMenuManager fields
m_nCurrentMenuEntry and m_nCurrentMenuPage are at 0x54 and 0x15D.
The Options table is aScreens[33], 0xE2 bytes per screen, 18 bytes per entry.

Additional ABI references:
- https://github.com/gta-reversed/gta-reversed/blob/master/source/game_sa/Frontend/MenuManager_Draw.cpp
- https://github.com/gta-reversed/gta-reversed/blob/master/source/game_sa/Tasks/TaskTypes/TaskComplexFollowNodeRoute.h
- https://github.com/gta-reversed/gta-reversed/blob/master/source/game_sa/Tasks/TaskTypes/TaskSimpleHandsUp.h
- https://github.com/gta-reversed/gta-reversed/blob/master/source/game_sa/Tasks/TaskTypes/TaskComplexSmartFleeEntity.h

## Scope

The ambient archive retains identities and restores actors when possible; it does
not simulate every offscreen character's daily routine. Appointments reuse the
saved identity and model. Loaded actors walk using the game's node routes; unloaded
actors are streamed outside the camera view before walking to the meeting point.
Mission-owned actors are not moved. Arrival is credited only when both participants
are physically close and visible; cancellation or expiry removes owned tasks and blips.
