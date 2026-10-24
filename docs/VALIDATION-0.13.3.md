# Validation — 0.13.3 beta

- Windows x86 C++17 build passed with /W4 /WX. All logic simulations passed, including
  repeat preset boundaries, persisted outing rest, distinct-person honor batches, sound
  queue retention, cheat recognition, pedestrian types and scripted conversation leases.
- Interface previews were generated for four languages at three resolutions.
- Archive read/write/read checks preserved identity names and learned details. The checks
  use an in-memory copy and do not modify the player's actual archive.
- All 80 supplied audio files passed Windows DirectShow decoding, private volume control
  and muted playback. Oversized artwork metadata was removed from one MP3 without
  changing its compressed speech frames.
- DEALER=17 and CRIMINAL=20 were checked against plugin-sdk. Script conversation layout
  and update gating were compared with gta-reversed and the installed 1.0 US executable.
- Scripted dealer Y/N handling now covers CConversationForPed as well as the separate
  CPedToPlayerConversations system. Nodes remain intact; only owned flags are restored.

Native dealer interaction, mixed game-audio balance and compatibility with other mods
still need wider live playtesting. Automated simulations do not substitute for those checks.

References:

- https://github.com/DK22Pac/plugin-sdk/blob/master/plugin_sa/game_sa/enums/ePedType.h
- https://github.com/gta-reversed/gta-reversed/blob/master/source/game_sa/Conversations/ConversationForPed.h
- https://github.com/gta-reversed/gta-reversed/blob/master/source/game_sa/Conversations/Conversations.cpp

Full release packages: both CMD and EXE installers passed 61 disposable-folder checks, including updates, user-data preservation, Unicode assets, loader handling, read-only targets and malformed packages. The manual ZIP contains no EXE.
