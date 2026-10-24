# 0.13.3 beta — dealer hotfix

- Corrected the native pedestrian-type mapping used by the routine-task exemption:
  DEALER is 17 and CRIMINAL is 20. Dealers no longer receive an automatic busy refusal
  just because their ambient routine occupies the primary slot. Combat and recruited
  group-member checks still apply.
- Handle scripted CConversationForPed dealer offers as well as the separate
  CPedToPlayerConversations system. Free-roam dealer conversations relinquish the Y/N
  input prompt to SA People's interaction controls. Their script nodes remain intact.
- Restore only the enable flags owned by this mod when entering a story mission,
  resetting, disabling native-talk replacement, dying or handling an adapter error.
  Actor-generation checks prevent restoring flags to a reused conversation/ped slot.
- Corrected the first-attacker criminal honor check to use CRIMINAL=20.
- Added regression tests for type mapping, scripted waiting-for-input cancellation,
  setting/mission restoration and unrelated/recycled conversation-slot safety.

# 0.13.2 beta

- Repeat message reactions now require consecutive copies of the same preset within the
  current conversation. A different preset, a new event message or a stale conversation
  breaks the streak. Repeating an acknowledgement after a later outing is safe.
- Automatic outing conversation begins after four seconds and continues roughly every
  6.5–10.5 seconds, waiting for existing subtitles. Topic order changes across outings.
  Manual conversation no longer has an eight-second per-choice lock; a 2.8-second
  input guard prevents overlapping rapid inputs. Repeated choices give no extra mood reward.
- Completed outings give the contact three in-game hours of rest. Active appointments
  and partners cannot be selected for unrelated roadside requests. Known contacts use
  familiar farewells after rides and private visits rather than introducing themselves again.
- Greeting three different receptive pedestrians earns four honor points. Provoking three
  different people costs two. Each person counts once per direction per ten real minutes.
  Existing help and relationship milestone rewards remain separate.
- Stopping an active pickpocket, mugger or hothead earns a limited defense reward, and
  their defeat no longer receives the innocent-killing penalty. A CRIMINAL-type pedestrian
  who attacks CJ first can qualify too. Appearance or occupation alone grants no reward.
- Honor now has a stronger influence on willingness to talk. Stranger openings and ambient
  remarks rotate through alternatives; weather remarks are mixed with general remarks.
- Honor sounds wait in a bounded queue rather than being discarded during a cooldown.
  They retain the configured quiet volume. Both directions have separate playback logging.
- Type IMARTHURMORGAN during free roam to set honor to 100. Recognition is disabled in
  the phone, menus and missions, and only the short cheat prefix is held in memory.
- Removed oversized ID3 artwork from one CJ MP3 that Windows could not decode. The
  compressed speech frames are unchanged. All 80 audio files passed the muted playback probe.
- Added four-language strings and regression coverage. Existing archives and settings are
  preserved. This is a beta build; back up player data before updating.

# 0.13.1 beta

- Contextual preset replies for thank-you messages, outings, family plans and private visits;
  known contacts no longer receive CJ's first-time introduction. Expanded reply pools.
- Driving meetups require CJ to arrive as the driver of a stopped automobile with a free seat.
  Passenger slots are translated to actual door IDs. Contacts are retained during appointments
  and use controlled waiting/boarding tasks instead of returning to ambient wandering.
- Automatic outing conversation with non-repeating topics, repeatable manual questions with
  cooldowns, explicit end/stop notices and safe passenger exit after the car stops.
- Nearby companions react to witnessed violence, provocations and help. Violence or harassment
  ends the meetup; help earns a limited positive reaction. Honor affects social openness and tone.
- Occasional unsolicited street remarks: Grove recognition, changed outfits, current rain,
  fog, sun and wind. Separate global and per-person cooldowns keep them infrequent.
- Native in-car solicitation is suppressed in free roam, with the original routine restored
  for story missions. Optional suppression leaves an incompatible third-party entry patch alone.
- Private visits can schedule a later, non-explicit follow-up text. Archive format V13 retains
  V1–V12 import support. Successful outings no longer trigger another same-day invitation.

Validation: build, simulations and archive round-trips. Native passenger animations, world
remarks and witness behavior still require live gameplay verification before publication.

# 0.13.0

- Follow-up fixes: repeat appointments clear their actor/route/activity state; driving dates
  can start from a parked car and continue tracking with the normal passenger task active.
- Help conversations continue immediately instead of treating CJ's next greeting as a reunion.
  Aborted arguments silence ordinary texts, including replies queued before the argument.
- Repeated phone messages get reminder/annoyance replies, then silence. Persona and phone
  response pools expanded in all four mod languages. Recording event keys removed.
- Native dealer conversations end through the game's cleanup routine, releasing reserved
  speech state and removing the Y/N help prompt before mod input is processed.
- Build.cmd locates Visual Studio without nested command quoting or inherited VSROOT values.
- Delayed consequences: what CJ does comes back hours later in game time. People he helped
  text a thank-you a few hours later; a contact who already trusts him may send money too.
  Robbing someone can send a relative or friend after CJ later (a hothead marked in red).
  Nothing comes from someone CJ has wronged since. Saved in people.dat (format V12).
- Rival gangs: walking into a gang that hates CJ starts a short standoff instead of an instant
  attack. Hold ALT and press G to talk it down (odds depend on honor, their numbers and whether
  CJ aims at them); success keeps that gang calm for two minutes, failure or silence and they
  attack. [World] GangTalk=0 turns it off.
- The mod stands down during gang wars (attack waves and defending territory), like on missions.
- Friends' favors: phone → contact → Ask a favor. A close contact (trust 50+, job known) helps
  once a game day by trade: mechanic/electrician repair your car, cook sends food (health),
  guard lends armor, driver shows back roads (one wanted star less), others lend cash.
- New sudden incident: a lost wallet. Return it to the owner (yellow marker) for a reward and
  honor, or keep the money and lose a little honor.
- Side events come more often (about every 1-2 minutes instead of 1.5-3).
- Night offers only come from street workers, at most every 8 minutes; ordinary women no longer
  proposition CJ. If the far door does not load, CJ wakes on the same street instead of the
  scene failing with "no safe house".
- Install.cmd copies sound files with non-ASCII names (robocopy instead of xcopy). The 0.12.5
  zip installed only part of the voice clips through Install.cmd; Setup.exe was not affected.

# 0.12.5

- Honor: good deeds count double (help +4), slips count less (provoke -2, killing -8); the TAB
  panel shows the number next to the bar.
- Dealer Y/N prompt really disabled: the game's 30-second chat cooldown is kept armed while
  ModTalkOverNative=1, so native ped-to-player chats never start.
- SA People Control Options (phone → Settings): every mod key can be rebound in game or in
  settings.ini [Controls]; secondary defaults P (phone) and W/A/S/D/E/Q (phone navigation)
  for keyboards without arrow or function keys.
- Provoking someone you have a meeting with cancels the meeting.
- Meal dates: one greeting starts a short scene (eating, fade, one hour passes); the contact
  leaves and the relationship improves.
- Language guard: choosing a game language whose text\*.gxt file is missing is blocked.
- Single-file SA-People-Setup.exe that asks for the game folder, backs up and installs.
# 0.12.4

- Street dealers and other pedestrians with an identity talk through the mod instead of the
  game's Y/N prompt (Interface/ModTalkOverNative=0 restores the native prompt).
- Help encounters (directions, bus fare, dead battery) no longer vanish when the ped briefly
  reacts to CJ; a lost wait task is re-issued and only a lasting interruption ends the event.
- Sudden incidents: pickpocket (catch the thief to get the money back), mugging in progress
  (stop the attacker for a reward and honor) and hotheads picking a fight. Red radar markers.
- The game's own language option is back in the main menu; the mod language, random events and
  interface size are in the phone's Settings page.
- Phone calls removed; messages show as a chat thread with bubbles, scrolling and a short
  "typing" delay before replies.
- Meeting arrivals fall back to a farther visible point when no hidden one exists.
- Public package with installer, uninstaller, English README, license and test checklist.
# 0.12.3

- Night offers: 21:00–05:00, on foot only, within 25 m; explicit accept/decline and cooldowns. Adult female ambient candidates only; no new ped spawning.
- Accepted visits fade out and resume at 08:00 beside a random nearby residential entrance, using a collision-checked pedestrian landing. No fee on invalid landing, once-only payment on success.
- Fixed StandStill tasks ending as soon as idle animation blended in: useAnimIdleStance is now false for roadside/help/visit/meeting waits. The original setting caused task-identity checks to cancel help encounters early.
- Active help responses bypass ordinary greeting budgets. Menus/background pause encounter deadlines instead of cancelling roadside and directions/fare events.
- Existing passenger boarding, destination and payment flow retained; partner home visits retain their separate conditions.

# 0.12.2

- Suitable ambient adult sex workers can offer a fade-to-black home visit when greeted directly on foot; no random event rotation required.
- Prioritize scarce visit candidates before generic ambient encounters. Existing task, mission, fee and entrance restrictions remain.

# 0.12.1

- Removed the local model, external helper, text entry and speech synthesis dependency.
- Messages and calls now use six translated dialogue choices with relationship, memory and availability checks.
- Added a persistent per-contact message cooldown; retained all identity and relationship data.

# 0.12

- Persistent daily home/work/hangout anchors with free-roam pedestrian routines.
  Natural returns use the current routine target, with collision, visibility,
  group-membership, mission and task-ownership guards. Known job is required before
  the phone shows a routine location.
- Phone inbox, per-contact conversation history, Unicode composition, paste,
  calls and action menus. Spontaneous invitations and family-plan messages have
  a per-contact day budget and a global 90-second notification budget.
- Social-network reactions become messages; battery help, robbery, arguments and
  outings can be recalled later. Previously harmed known people may flee on re-encounter.
- Meal breaks, walks and real passenger drives have completion requirements and
  three conversation choices. Positive completion is awarded once; bad behavior
  can end the activity. Pool outings hand off to vanilla pool and observe actual
  wins/losses; the vanilla opponent is not replaced with the contact in this version.
- Optional local qwen3:4b replies through a separate Python worker, limited CPU
  usage, bounded history and a local-only HTTP endpoint. Game-state changes use
  controlled intent rules; no model-driven money/item/script commands. Calls can
  use a matching installed Windows voice; unavailable voice falls back to text.
- V11 archive migration and regression cases for routine phases, mailbox limits,
  UTF-8, invitation budgets, remote-reaction cooldowns, social consequences and
  one-time outing completion. Existing model/voice/menu/family features retained.

# 0.11

- Native pedestrian conversations temporarily own input and speech; mod focus, events
  and phone no longer compete with the dealer's Y/N interaction. Native prompts remain native.
- Meetings resolve onto streamed outdoor pedestrian nodes, validate collision and
  elevation, and retry stalled off-camera approaches. Corrected Jizzy's roof elevation
  and moved the horse-track rendezvous to its pedestrian approach.
- Radar-only meeting markers use a real sprite slot: handshake for friends, supplied
  heart/question art for romantic interest, heart for partners. Removed the large
  world-space icon and venue label. Stale radar handles are recreated.
- Contextual partner farewells, single/married eligibility, exclusive relationships,
  partner home invitations, once-per-date progress and explicit family planning.
- V10 household/family persistence, V1-V9 migration, preservation of named relatives,
  Johnson child records and Turkish/Russian/Spanish/English text for new flows.

# Meeting icon update

- Transparent handshake art for the meeting radar/map marker, world marker and phone header.
- Only meeting handles use the new radar art; stock coordinate transforms, clipping and
  unrelated sprites retain native behavior. Missing art falls back to the stock marker.
- PNG upload preserves alpha; Direct3D state is restored after each icon draw.

# 0.10

- Appointment expiry across restarts, shorter invitation lead-in, nearby hidden arrivals,
  distant actor relocation, persistent map markers and a world-space meeting label.
- Phone meeting details show distance, arrival status and explicit location/cancel actions.
  Contact details have separate information, family, memory and connection pages.
- Persistent honor with a replacement TAB stats panel; vanilla respect stays intact.
- Five named adult relatives can receive separate persistent population identities.
  Reciprocal family links, street friendships/rivalries and shared relationship memories.
- Donations, drug-money requests with honor loss, real passenger rides with randomized
  one-time fares, and fade-to-black house invitations with a six-hour clock advance.
- More frequent varied encounters, while retaining a single active event and per-person rewards.
- Early suspension during story missions and cutscenes; owned actor tasks and UI are released.
- Archive soft limit (12000): untouched ambient strangers are recycled in place, contacts,
  relatives, appointments and graph members never. An unreadable people.dat falls back to
  people.dat.bak (the bad file is kept as people.corrupt-*.dat). A failed invitation no longer
  starts the 60 s cooldown. House visits restore CJ's health. Control hint text restored.
- V7 persistence, V1-V6 migration, 242 localized text rows, archive migration validation and
  regression checks for family identity, graph news, offline expiry, honor and ride rewards.

# 0.9

- Protect recruited player-group members from social task assignment, targeting, robbery,
  invitation movement and side-event selection; vanilla group controls remain in charge.
- Read installed peds.ide metadata for female models, including PROSTITUTE models and
  female animation groups. Correct archived model labels on load.
- Reattach a nearby, uniquely matching known contact after ambient respawn or save reload.
  Never merge simultaneously loaded lookalikes, ambiguous contacts, dead people or different areas.
- Remove ALT lettering from the street focus badge.
- Shared dark frontend background and sans-serif plain-text rendering across the native menu
  call sites. Map, sliders, save slots, control binding and navigation retain native behavior.
  Extended GXT text, inline control icons and multiline text keep the native renderer.
- Interface/ModernMenu=0 opts out of the frontend theme. Modified call sites are checked;
  a conflicting theme causes this skin to decline installation without partial patching.

# 0.8

- Greeting limits now apply to a whole encounter. Busy replies close greeting immediately;
  an uninterested reply permits one final farewell. Changing target does not reset this state.
- Provocation builds persistent resentment. An apology can work once per encounter;
  breaking that truce makes another apology ineffective. Forgiveness does not restore trust.
- F6 opens a period-style mobile phone with contacts, learned details, meetings and settings.
- Known contacts can accept or reject invitations. Accepted meetings receive map markers,
  timed arrival windows, physical NPC arrival and a one-time relationship result.
- Turkish, Russian, Spanish and English text, including subtitles and event responses.
- Language selection is shared between the phone and an entry in the game's Options page.
- Filled triangular focus marker and model-based gender labels.
- Unicode text rendering, formatted source, expanded logic and rendering checks.
