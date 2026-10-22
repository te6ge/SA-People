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


