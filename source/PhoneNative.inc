// Phone input and appointment actors. Only tasks and control bits owned here are released.
phone::State phoneState;
void deliverReplies();
bool replyPending(unsigned id);
bool hasAppointment(unsigned id) {
    for (const auto &a : registry.appointments)
        if (a.person == id && meetings::active(a))
            return true;
    return false;
}
bool phoneUp = false, phoneDown = false, phoneLeft = false, phoneRight = false, phoneEnter = false,
     phoneBack = false;
unsigned short *phoneControl = nullptr;
bool phoneOwnsControl = false;
using MeetingRun = meetings::Run;
std::unordered_map<unsigned, MeetingRun> meetingRuns;
std::unordered_map<unsigned, DWORD> nextInvitation;
void changeLanguage(int direction) {
    locale::cycle(direction);
    std::string code = locale::code(locale::language);
    std::wstring value(code.begin(), code.end());
    if (!WritePrivateProfileStringW(L"Interface", L"Language", value.c_str(),
                                    (directory / L"settings.ini").c_str()))
        log("Could not save language setting");
    panelInvalid = true;
}
void closePhone() {
    if (phoneOwnsControl && phoneControl)
        *phoneControl = static_cast<unsigned short>(*phoneControl & ~0x4000u);
    phoneOwnsControl = false;
    phoneControl = nullptr;
    panelOpen = false;
}
void togglePhone() {
    if (panelOpen) {
        closePhone();
        return;
    }
    if (at<unsigned>(0x969A20) != 0)
        return;
    auto pad = reinterpret_cast<Byte *(__cdecl *)(int)>(0x53FB70)(0);
    if (!pad)
        return;
    auto flags = reinterpret_cast<unsigned short *>(pad + 0x10E);
    if (*flags)
        return;
    phoneControl = flags;
    *flags |= 0x4000;
    phoneOwnsControl = true;
    panelOpen = true;
    panelInvalid = true;
    panelTouched = GetTickCount();
    phoneState.notice.clear();
}
void releaseMeetingActor(MeetingRun &run) {
    auto pool = at<Pool *>(0xB74490);
    if (validPool(pool) && exists(pool, run.actor)) {
        auto manager = taskManager(pedAt(pool, run.actor.slot));
        if (manager && run.task &&
            at<void *>(reinterpret_cast<std::uintptr_t>(manager) + 12) == run.task)
            setTask(manager, nullptr, 3);
    }
    run.task = nullptr;
}
void clearMeetingRuns() {
    for (auto &item : meetingRuns) {
        releaseMeetingActor(item.second);
        if (item.second.blip != -1)
            reinterpret_cast<void(__cdecl *)(int)>(0x587CE0)(item.second.blip);
        if (!companionOwns(item.first) && !departureOwns(item.first)) releaseActor(item.first);
    }
    meetingRuns.clear();
}
void resetMeetingRun(unsigned id) {
    meetings::forgetRun(meetingRuns, id, [](MeetingRun &run) {
        releaseMeetingActor(run);
        if (run.blip != -1)
            reinterpret_cast<void(__cdecl *)(int)>(0x587CE0)(run.blip);
    });
    if (!companionOwns(id) && !departureOwns(id)) releaseActor(id);
}
bool visiblePoint(Vec point) {
    Vec screen{};
    float w = 0, h = 0;
    if (!reinterpret_cast<bool(__cdecl *)(const Vec &, Vec *, float *, float *, bool, bool)>(
            0x70CE30)(point, &screen, &w, &h, true, true))
        return false;
    return screen.x >= 0 && screen.y >= 0 && screen.x < at<int>(0xC17044) &&
           screen.y < at<int>(0xC17048);
}
void inviteContact(Byte *player) {
    unsigned id = phoneState.person;
    if (!id || id > registry.families.size())
        return;
    auto &f = registry.families[id - 1];
    if(auto reason=community::request(registry,id)){phoneState.notice=reason;dirty=true;return;}
    if (f.daily.busyUntil > registry.worldMinutes) {
        phoneState.notice = "PHONE_INVITE_BUSY";
        return;
    }
    DWORD now = GetTickCount();
    for (const auto &a : registry.appointments)
        if (a.person == id && meetings::active(a)) {
            phoneState.notice = "PHONE_INVITE_ACTIVE";
            return;
        }
    if (nextInvitation.count(id) && !due(now, nextInvitation[id])) {
        phoneState.notice = "PHONE_INVITE_BUSY";
        return;
    }
    if (!f.presence.alive || !f.presence.located) {
        phoneState.notice = "PHONE_UNAVAILABLE";
        return;
    }
    if (f.social.resentment >= 30 || f.social.anger >= 25 || f.social.robbed) {
        phoneState.notice = "PHONE_INVITE_REFUSED";
        return;
    }
    if (!meetings::canInvite(f)) {
        phoneState.notice = "PHONE_INVITE_REJECT";
        return;
    }
    releaseRoutine(id);
    {
        people::Handle actor{};
        auto pool = at<Pool *>(0xB74490);
        if (bindings.locate(id, actor) && validPool(pool) && exists(pool, actor)) {
            auto ped = pedAt(pool, actor.slot);
            if (!eligible(ped, player) || !onFoot(ped) || busy(ped) || primaryOccupied(ped)) {
                phoneState.notice = "PHONE_INVITE_BUSY";
                return;
            }
        }
    }
    bool mission = reinterpret_cast<bool(__cdecl *)()>(0x464D50)();
    const bool driveFromCar = phoneState.activity == 2 && drivenCar(player) &&
                              stoppedCar(drivenCar(player));
    if (mission || (!onFoot(player) && !driveFromCar) || player[0x2F] != 0 || busy(player)) {
        phoneState.notice = "PHONE_INVITE_SAFE";
        return;
    }
    Vec origin = position(player), place{};
    bool found = false;
    // Prefer a real venue (bar, diner, gym, casino, horse track) a short drive away.
    const auto *venue =
        meetings::activityVenue(origin.x, origin.y, id * 977 + now, phoneState.activity);
    unsigned delay = 15000, window = 300000;
    if (venue) {
        place = {venue->x, venue->y, venue->z};
        // Distant collision is not streamed yet; resolve onto a pedestrian node on arrival.
        Vec sidewalk{};
        if (streetPoint(place, player, sidewalk))
            place = sidewalk;
        found = true;
        delay = 20000;
        window = 480000;
    }
    const float offsets[][2] = {{18, 0}, {0, 18}, {-18, 0}, {0, -18}, {26, 12}, {-26, -12}};
    for (const auto &offset : offsets) {
        if (found)
            break;
        place = {origin.x + offset[0], origin.y + offset[1], origin.z};
        if (clearGround(place, player)) {
            found = true;
            break;
        }
    }
    if (!found) {
        phoneState.notice = "PHONE_INVITE_NO_SPOT";
        return;
    }
    if (!meetings::schedule(registry, id, place.x, place.y, place.z, delay, window)) {
        phoneState.notice = "PHONE_INVITE_FULL";
        return;
    }
    // A new appointment must not inherit activityStarted, route or spawn failures from the last.
    resetMeetingRun(id);
    for (auto &a : registry.appointments)
        if (a.person == id && meetings::active(a))
            a.activity = phoneState.activity;
    nextInvitation[id] = now + 60000;
    phoneState.notice =
        venue ? locale::format("PHONE_INVITE_VENUE", venue->name) : "PHONE_INVITE_ACCEPT";
    dirty = true;
    save();
    log("MEETING scheduled identity=" + std::to_string(id));
}
// Strict mode needs a point the player cannot see. When CJ stands at the venue every nearby
// point is visible, so after a few failures `relaxed` accepts a farther, visible point (30 m+)
// and lets the contact walk in.
bool meetingSpawn(Vec place, Byte *player, Vec &spawn, bool relaxed = false) {
    const float closeSet[][2] = {{12, 0},  {0, 12},   {-12, 0},  {0, -12},
                             {22, 10}, {-22, 10}, {22, -10}, {-22, -10}};
    const float farSet[][2] = {{40, 0},  {0, 40},   {-40, 0}, {0, -40},
                            {30, 30}, {-30, 30}, {30, -30}, {-30, -30},
                            {55, 0},  {0, 55},   {-55, 0}, {0, -55}};
    const size_t count = relaxed ? sizeof(farSet) / sizeof(farSet[0]) : sizeof(closeSet) / sizeof(closeSet[0]);
    for (size_t i = 0; i < count; ++i) {
        const float *o = relaxed ? farSet[i] : closeSet[i];
        Vec anchor{place.x + o[0], place.y + o[1], place.z};
        if (!streetPoint(anchor, player, spawn, relaxed ? 20.0f : 12.0f) ||
            std::abs(spawn.z - place.z) > 1.5f)
            continue;
        // Require a clear, short approach to the meeting point, not a route through a wall.
        if (!relaxed &&
            !reinterpret_cast<bool(__cdecl *)(const Vec &, const Vec &, bool, bool, bool, bool,
                                              bool, bool, bool)>(0x56A490)(
                spawn, place, true, false, false, true, true, false, false))
            continue;
        if (distance2(position(player), spawn) < (relaxed ? 900.0f : 64.0f) ||
            (!relaxed && visiblePoint(spawn)) || !clearGround(spawn, player, true))
            continue;
        return true;
    }
    return false;
}
void teleportActor(Byte *ped, Vec position) {
    auto table = *reinterpret_cast<std::uintptr_t **>(ped);
    reinterpret_cast<void(__thiscall *)(Byte *, Vec, bool)>(table[14])(ped, position, false);
}
void updateMeetings(Pool *pool, Byte *player, unsigned elapsed, bool mission) {
    bool safe = !mission;
    for (auto &a : registry.appointments) {
        if (!a.person || a.person > registry.families.size())
            continue;
        auto &f = registry.families[a.person - 1];
        if (!f.presence.alive && meetings::active(a)) {
            a.state = 4;
            dirty = true;
        }
        if (meetings::step(a, elapsed, safe && !companionOwns(a.person))) {
            dirty = true;
            if (a.state == 4)
                help(locale::format("PHONE_MEETING_MISSED", dialogue::displayName(f)));
        }
        if (!meetings::active(a)) {
            resetMeetingRun(a.person);
            continue;
        }
        auto &run = meetingRuns[a.person];
        Vec place{a.x, a.y, a.z};
        if (!run.spotResolved && safe && player[0x2F] == 0 &&
            distance2(position(player), place) < 14400 && due(GetTickCount(), run.nextResolve)) {
            run.nextResolve = GetTickCount() + 1000;
            Vec anchor = place;
            // Repair old roof-height appointments using the venue's street elevation.
            float nearest = 22500;
            for (const auto &venue : meetings::venues()) {
                float dx = place.x - venue.x, dy = place.y - venue.y;
                if (dx * dx + dy * dy < nearest) {
                    anchor = {venue.x, venue.y, venue.z};
                    nearest = dx * dx + dy * dy;
                }
            }
            Vec sidewalk{};
            if (streetPoint(anchor, player, sidewalk)) {
                releaseMeetingActor(run);
                if (run.blip != -1)
                    reinterpret_cast<void(__cdecl *)(int)>(0x587CE0)(run.blip);
                run.blip = -1;
                place = sidewalk;
                a.x = place.x;
                a.y = place.y;
                a.z = place.z;
                run.spotResolved = true;
                dirty = true;
                log("MEETING sidewalk resolved identity=" + std::to_string(a.person) +
                    " x=" + std::to_string(a.x) + " y=" + std::to_string(a.y) +
                    " z=" + std::to_string(a.z));
            }
        }
        if (run.blip != -1 && reinterpret_cast<int(__cdecl *)(int)>(0x582870)(run.blip) == -1)
            run.blip = -1;
        if (run.blip == -1)
            run.blip = reinterpret_cast<int(__cdecl *)(int, Vec, unsigned, int, char *)>(0x583820)(
                4, place, 0, 2, nullptr);
        if (run.blip != -1)
            reinterpret_cast<void(__cdecl *)(int, unsigned)>(0x583AB0)(run.blip, 2);
        if (run.blip != -1) {
            reinterpret_cast<void(__cdecl *)(int, unsigned char)>(0x583DB0)(run.blip, 1);
            reinterpret_cast<void(__cdecl *)(int, int)>(0x583D70)(run.blip,
                                                                  f.social.romance ? 21 : 37);
        }
        if (!safe || player[0x2F] != 0 || !run.spotResolved || nativeConversation(player))
            continue;
        if (companionOwns(a.person))
            continue;
        people::Handle actor{};
        if (bindings.locate(a.person, actor) && exists(pool, actor)) {
            auto ped = pedAt(pool, actor.slot);
            if (!eligible(ped, player) || !onFoot(ped) || ped[0x2F] != 0 ||
                *reinterpret_cast<float *>(ped + 0x540) <= 0) {
                releaseMeetingActor(run);
                continue;
            }
            auto manager = taskManager(ped);
            if (!manager)
                continue;
            retainActor(actor, a.person);
            if (!(run.actor == actor)) {
                releaseMeetingActor(run);
                run.actor = actor;
                run.waiting = false;
                run.progressAt = 0;
                run.bestDistance = 1000000000.0f;
            }
            const auto actorPosition = position(ped);
            const float currentDistance = distance2(actorPosition, place);
            const DWORD clock = GetTickCount();
            if (!run.progressAt || currentDistance + 4 < run.bestDistance) {
                run.bestDistance = currentDistance;
                run.progressAt = clock;
            }
            bool stuck = currentDistance > 16 && clock - run.progressAt > 20000;
            bool wrongHeight = std::abs(actorPosition.z - place.z) > 3;
            bool ownsRoute =
                run.task && at<void *>(reinterpret_cast<std::uintptr_t>(manager) + 12) == run.task;
            if (distance2(position(player), place) < 6400 &&
                (currentDistance > 1600 || wrongHeight || stuck) && !visibleTarget(ped) &&
                !busy(ped) && (!primaryOccupied(ped) || ownsRoute)) {
                Vec spawn{};
                if (meetingSpawn(place, player, spawn)) {
                    releaseMeetingActor(run);
                    teleportActor(ped, spawn);
                    run.progressAt = clock;
                    run.bestDistance = distance2(spawn, place);
                    capture(f, ped);
                    log("MEETING moved distant contact identity=" + std::to_string(f.id));
                }
            }
            if (run.task && at<void *>(reinterpret_cast<std::uintptr_t>(manager) + 12) != run.task)
                run.task = nullptr;
            // Once CJ is at the venue the contact walks up to him, not to a fixed spot he may
            // never stand on (a contact waiting a few metres away used to stall the meeting).
            auto car = drivenCar(player);
            const bool fromCar = a.activity == 2 && car && stoppedCar(car) &&
                                 freeSeat(car) >= 0;
            const bool playerAtVenue = ((a.activity != 2 && onFoot(player)) || fromCar) &&
                                       distance2(position(player), place) <= 225;
            const Vec goal = playerAtVenue ? position(player) : place;
            float distance = distance2(position(ped), goal);
            if(run.path.stuck(distance,clock)) {
                releaseMeetingActor(run);
                run.path.retried(clock);
                if(run.path.retries>=3) {
                    help("MEET_ROUTE_BLOCKED");a.state=4;dirty=true;continue;
                }
                Vec sidewalk{};
                if(streetPoint(goal,player,sidewalk)) {a.x=sidewalk.x;a.y=sidewalk.y;a.z=sidewalk.z;}
                log("MEETING path retry identity="+std::to_string(f.id));
            }
            if (run.task && run.waiting && distance > 25) {
                releaseMeetingActor(run);
                run.waiting = false;
            }
            if (!busy(ped) && !primaryOccupied(ped) && !run.task &&
                std::abs(position(ped).z - goal.z) <= 2.5f) {
                if (distance > 9) {
                    auto task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x60);
                    if (task) {
                        reinterpret_cast<void(__thiscall *)(void *, int, const Vec &, float, float,
                                                            float, bool, int, bool)>(0x66EA30)(
                            task, 4, goal, 2.0f, 3.0f, 2.0f, false, -1, true);
                        setTask(manager, task, 3);
                        run.task = task;
                        run.waiting = false;
                    }
                } else {
                    auto task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x20);
                    if (task) {
                        reinterpret_cast<void(__thiscall *)(void *, int, bool, bool, float)>(
                            0x62F310)(task, static_cast<int>(a.window), false, false, 4.0f);
                        setTask(manager, task, 3);
                        run.task = task;
                        run.waiting = true;
                    }
                }
            }
            if (distance2(position(ped), place) <= 16 && !run.notified) {
                run.notified = true;
                help(locale::format("PHONE_ARRIVED", dialogue::displayName(f)));
            }
            // A drive date may start with CJ waiting in a stopped car beside the contact.
            const float apart = distance2(position(player), position(ped));
            const bool validDrive = a.activity != 2 || socialcontext::driveReady(
                car != nullptr, car && at<int>(reinterpret_cast<std::uintptr_t>(car) + 0x590) == 0,
                fromCar, car && freeSeat(car) >= 0);
            if (a.activity == 2 && !validDrive && distance2(position(player), place) < 1600 &&
                due(clock, run.nextHint)) {
                help("ACT_DRIVE_CAR_REQUIRED");
                run.nextHint = clock + 20000;
            }
            bool together = validDrive && (onFoot(player) || fromCar) &&
                            std::abs(position(ped).z - position(player).z) <= 2.5f &&
                            distance2(position(player), place) <= (fromCar ? 1600.0f : 400.0f) &&
                            apart <= (fromCar ? 144.0f : 25.0f) && visibleTarget(ped);
            if (a.state == 2 && together && !run.activityStarted) {
                releaseMeetingActor(run);
                run.activityStarted = beginCompanion(a, f, actor);
                if (run.activityStarted) {
                    dirty = true;
                    panelInvalid = true;
                    save();
                }
            }
        } else if (!pending.id && distance2(position(player), place) < 3600 &&
                   due(GetTickCount(), run.nextSpawn)) {
            run.nextSpawn = GetTickCount() + 3000;
            Vec spawn{};
            if (meetingSpawn(place, player, spawn, run.spawnFails >= 3)) {
                if (run.spawnFails >= 3)
                    log("MEETING arrival point relaxed identity=" + std::to_string(a.person));
                requestReturn(a.person, spawn, false, true);
                run.spawnFails = 0;
            } else if (++run.spawnFails == 1 || run.spawnFails % 20 == 0)
                log("MEETING no safe arrival point identity=" + std::to_string(a.person));
        }
    }
}

// In-game rebinding: the first key pressed after Enter becomes the primary key; Esc cancels.
bool captureFresh = false;
bool capturePrevious[256] = {};
void captureControl() {
    if (captureFresh) {
        controls::capture(capturePrevious); // Ignore keys already held (the Enter itself).
        captureFresh = false;
        return;
    }
    int vk = controls::capture(capturePrevious);
    if (!vk)
        return;
    phoneState.capturing = false;
    panelInvalid = true;
    if (vk == VK_ESCAPE)
        return;
    unsigned action = phoneState.cursor % (controls::Count + 1);
    if (action < controls::Count) {
        controls::keys[action].primary = vk;
        controls::save((directory / L"settings.ini").c_str());
    }
    // Swallow the press so the new key does not also trigger its old meaning this frame.
    phoneUp = phoneDown = phoneLeft = phoneRight = phoneEnter = phoneBack = true;
}
// Phone → contact → Ask a favor (Favors.h). Effects apply at once; the friend texts back.
void askFavor(Byte *player, unsigned id) {
    if(auto reason=community::request(registry,id)){phoneState.notice=reason;dirty=true;return;}
    if (auto reason = favors::refusal(registry, id)) {
        phoneState.notice = reason;
        return;
    }
    auto perk = favors::perk(registry.families[id - 1]);
    if(perk==favors::Perk::Repair || perk==favors::Perk::Heal || perk==favors::Perk::Wanted ||
        registry.families[id-1].presence.type==18) {
        beginFriendService(player,id);return;
    }
    std::string done = std::string(favors::label(perk)) + "_DONE";
    if (perk == favors::Perk::Repair) {
        auto car = drivenCar(player);
        if (!car) {
            phoneState.notice = "FAVOR_NEED_CAR";
            return;
        }
        auto &health = *reinterpret_cast<float *>(car + 0x4C0);
        health = std::max(health, 1000.0f);
    } else if (perk == favors::Perk::Heal) {
        auto &health = *reinterpret_cast<float *>(player + 0x540);
        health = std::min(std::max(health, *reinterpret_cast<float *>(player + 0x544)),
                          health + 40.0f);
    } else if (perk == favors::Perk::Armor) {
        auto &armor = *reinterpret_cast<float *>(player + 0x548);
        armor = std::max(armor, 50.0f);
    } else if (perk == favors::Perk::Wanted) {
        auto wanted = reinterpret_cast<Byte *(__cdecl *)(int)>(0x56E230)(0);
        unsigned level = wanted ? *reinterpret_cast<unsigned *>(wanted + 0x2C) : 0;
        if (!level || level > 6) {
            phoneState.notice = "FAVOR_NO_WANTED";
            return;
        }
        reinterpret_cast<void(__thiscall *)(Byte *, int)>(0x562470)(wanted,
                                                                   static_cast<int>(level) - 1);
    } else {
        auto money = playerMoney(player);
        unsigned amount = favors::cash(registry, id);
        if (!money || *money < 0 || *money > 999999999 - static_cast<int>(amount)) {
            phoneState.notice = "FAVOR_NO";
            return;
        }
        *money += static_cast<int>(amount);
        done = locale::format("FAVOR_CASH_DONE", std::to_string(amount));
    }
    favors::granted(registry, id, perk);
    phoneState.notice = done;
    dirty = true;
    panelInvalid = true;
    save();
    log("FAVOR " + std::string(favors::label(perk)) + " identity=" + std::to_string(id));
}
void openCompose(bool call) {
    phoneState.page = phone::Page::Compose;
    phoneState.call = call;
    phoneState.cursor = 0;
}
void openThread(unsigned id) {
    phoneState.person = id;
    phoneState.page = phone::Page::Messages;
    phoneState.textOffset = 0;
    auto messages = phone::thread(registry, id);
    phoneState.cursor = messages.empty() ? 0u : static_cast<unsigned>(messages.size() - 1);
    for (auto &m : registry.letters)
        if (m.person == id && !m.outgoing)
            m.unread = false;
    dirty = true;
    panelInvalid = true;
}
void processPhone(Byte *player) {
    if (panelOpen && phoneState.page == phone::Page::Controls && phoneState.capturing) {
        captureControl();
        return;
    }
    bool up = pressAction(controls::PhoneUp, phoneUp), down = pressAction(controls::PhoneDown, phoneDown),
         left = pressAction(controls::PhoneLeft, phoneLeft),
         right = pressAction(controls::PhoneRight, phoneRight),
         enter = pressAction(controls::PhoneSelect, phoneEnter),
         back = pressAction(controls::PhoneBack, phoneBack);
    deliverReplies();
    if (!panelOpen)
        return;
    phoneState.typing = replyPending(phoneState.person);
    phoneState.events = sideEvents;
    phoneState.scale = static_cast<unsigned>(panelScale * 100 + 0.5f);
    phoneState.outing = companionOwns(phoneState.person) ? phoneState.person : 0;
    if (!phoneState.notice.empty()) {
        if (enter || back)
            phoneState.notice.clear();
        panelInvalid = true;
        return;
    }
    if (up || down || left || right || enter || back) {
        panelInvalid = true;
        panelTouched = GetTickCount();
    }
    auto page = phoneState.page;
    if (back) {
        if (page == phone::Page::Meeting)
            phoneState.page = phone::Page::Meetings;
        else if (page == phone::Page::Compose)
            openThread(phoneState.person);
        else if (page == phone::Page::Messages || page == phone::Page::Activities ||
                 page == phone::Page::Outing)
            phoneState.page = phone::Page::Actions;
        else if (page == phone::Page::Actions)
            phoneState.page = phone::Page::Contact;
        else if (page == phone::Page::Controls)
            phoneState.page = phone::Page::Settings;
        else if (page == phone::Page::Contacts)
            closePhone();
        else
            phoneState.page = phone::Page::Contacts;
        phoneState.cursor = 0;
        return;
    }
    if (page == phone::Page::Compose) {
        const auto available = phonedialogue::options(registry, phoneState.person);
        const auto count = static_cast<unsigned>(available.size());
        phoneState.cursor %= count;
        if (up)
            phoneState.cursor = (phoneState.cursor + count - 1) % count;
        if (down)
            phoneState.cursor = (phoneState.cursor + 1) % count;
        if (enter) {
            unsigned choice = available[phoneState.cursor], id = phoneState.person;
            openThread(id);
            sendPhoneChoice(id, choice, false);
        }
        return;
    }
    if (page == phone::Page::Actions || page == phone::Page::Activities ||
        page == phone::Page::Outing) {
        unsigned count = page == phone::Page::Actions ? (companionOwns(phoneState.person) ? 5u : 4u)
                         : page == phone::Page::Activities ? 4u
                                                           : 3u;
        phoneState.cursor %= count;
        if (up)
            phoneState.cursor = (phoneState.cursor + count - 1) % count;
        if (down)
            phoneState.cursor = (phoneState.cursor + 1) % count;
        if (enter) {
            if (page == phone::Page::Outing) {
                closePhone(); // Let the spoken answer appear in the world, not behind the phone.
                companionChoice(phoneState.cursor);
                return;
            }
            if (page == phone::Page::Activities) {
                phoneState.activity = phoneState.cursor;
                inviteContact(player);
                return;
            }
            unsigned action = phoneState.cursor;
            if (action == 0) {
                openCompose(false);
                return;
            }
            if (action == 1) {
                if (companionOwns(phoneState.person))
                    phoneState.page = phone::Page::Outing;
                else
                    phoneState.page = phone::Page::Activities;
                phoneState.cursor = 0;
                return;
            }
            if (action == 2) {
                phoneState.activity = 0;
                inviteContact(player);
                return;
            }
            if (action == 3) {
                askFavor(player, phoneState.person);
                return;
            }
            if (action == 4) {
                cancelCompanion(true);
                save();
                phoneState.notice = "ACT_CANCELLED";
                return;
            }
        }
        return;
    }
    if (page == phone::Page::Messages) {
        auto messages = phone::thread(registry, phoneState.person);
        auto lines = phone::chatLines(registry, phoneState.person, phoneState.typing);
        unsigned maxShift = lines.size() > 8 ? static_cast<unsigned>(lines.size() - 8) : 0u;
        if (up && phoneState.textOffset < maxShift)
            ++phoneState.textOffset;
        if (down && phoneState.textOffset)
            --phoneState.textOffset;
        bool invite = !messages.empty() && !messages.back()->outgoing && messages.back()->kind == 2;
        if (enter && invite && !phoneState.textOffset) {
            const int proposed = neighborhood::plannedActivity(messages.back()->body);
            if (proposed >= 0) {
                phoneState.activity = static_cast<unsigned>(proposed);
                inviteContact(player);
                panelInvalid = true;
            } else {
                phoneState.page = phone::Page::Activities;
                phoneState.cursor = 0;
            }
            return;
        }
        if (enter || right)
            openCompose(false);
        return;
    }
    if (page == phone::Page::Inbox) {
        auto messages = phone::inbox(registry);
        if (!messages.empty()) {
            unsigned count = static_cast<unsigned>(messages.size());
            phoneState.cursor %= count;
            if (up)
                phoneState.cursor = (phoneState.cursor + count - 1) % count;
            if (down)
                phoneState.cursor = (phoneState.cursor + 1) % count;
            if (enter) {
                openThread(messages[phoneState.cursor]->person);
                phoneState.call = false;
                return;
            }
        }
    } else if (page == phone::Page::Meeting && phoneState.cursor < registry.appointments.size()) {
        auto &a = registry.appointments[phoneState.cursor];
        phoneState.distance =
            static_cast<unsigned>(std::sqrt(distance2(position(player), {a.x, a.y, a.z})));
        if (up || down || left || right)
            phoneState.detail = (phoneState.detail + 1) % 2;
        if (enter) {
            if (phoneState.detail == 1 && meetings::active(a)) {
                a.state = 4;
                if (companionOwns(a.person))
                    cancelCompanion(false);
                dirty = true;
                save();
                phoneState.notice = "PHONE_INVITE_CANCEL";
            } else
                phoneState.notice = meetings::active(a) ? "PHONE_ROUTE_READY" : "PHONE_CANCELLED";
        }
        return;
    } else if (page == phone::Page::Controls) {
        const unsigned rows = controls::Count + 1;
        if (up)
            phoneState.cursor = (phoneState.cursor + rows - 1) % rows;
        if (down)
            phoneState.cursor = (phoneState.cursor + 1) % rows;
        if (enter) {
            if (phoneState.cursor % rows == controls::Count) {
                controls::reset();
                controls::save((directory / L"settings.ini").c_str());
                phoneState.notice = "CTRL_RESET_DONE";
            } else {
                phoneState.capturing = true;
                captureFresh = true;
            }
        }
        return;
    } else if (page == phone::Page::Settings) {
        if (up)
            phoneState.cursor = (phoneState.cursor + 3) % 4;
        if (down)
            phoneState.cursor = (phoneState.cursor + 1) % 4;
        if (enter && phoneState.cursor % 4 == 3) {
            phoneState.page = phone::Page::Controls;
            phoneState.cursor = 0;
            return;
        }
        if (enter) {
            auto ini = (directory / L"settings.ini").wstring();
            if (phoneState.cursor % 4 == 0)
                changeLanguage(1);
            else if (phoneState.cursor % 4 == 1) {
                sideEvents = !sideEvents;
                if (!sideEvents)
                    cancelEvents();
                WritePrivateProfileStringW(L"World", L"SideEvents", sideEvents ? L"1" : L"0",
                                           ini.c_str());
            } else {
                static const unsigned steps[] = {75, 90, 100, 110, 125};
                unsigned now = static_cast<unsigned>(panelScale * 100 + 0.5f), next = 75;
                for (unsigned s : steps)
                    if (s > now) {
                        next = s;
                        break;
                    }
                panelScale = next / 100.0f;
                WritePrivateProfileStringW(L"Interface", L"ScalePercent",
                                           std::to_wstring(next).c_str(), ini.c_str());
            }
            panelInvalid = true;
        }
        if (left || right)
            phoneState.cursor = 0;
    } else if (page == phone::Page::Contact) {
        if (!phoneState.person || phoneState.person > registry.families.size())
            return;
        auto &f = registry.families[phoneState.person - 1];
        auto tab = [&](int direction) {
            do
                phoneState.detail = (phoneState.detail + (direction < 0 ? 7u : 1u)) % 8;
            while (phoneState.detail == 4 && f.social.romance < 3 && !f.social.childStage);
        };
        if (left || (up && phoneState.detail != 3))
            tab(-1);
        if (right || (down && phoneState.detail != 3))
            tab(1);
        if (phoneState.detail == 3 && (up || down)) {
            unsigned count = network::degree(registry, f.id), pages = count > 3 ? count - 2 : 1;
            phoneState.networkOffset = (phoneState.networkOffset + (up ? pages - 1 : 1)) % pages;
        }
        if (enter) {
            if (phoneState.detail == 4) {
                if (romance::plan(f)) {
                    phoneState.notice = "ROMANCE_PLAN_ACCEPTED";
                    dirty = true;
                    save();
                } else
                    phoneState.notice =
                        f.social.childStage ? "ROMANCE_PLAN_EXISTS" : "ROMANCE_PLAN_WAIT";
            } else if (phoneState.detail == 5) {
                phoneState.page = phone::Page::Actions;
                phoneState.cursor = 0;
            } else if (phoneState.detail == 6) {
                openThread(f.id);
                phoneState.call = false;
            } else if (phoneState.detail == 7) {
                if (!f.daily.ready || !dialogue::knows(f, people::Job)) {
                    phoneState.notice = "ROUTINE_NOT_READY";
                    return;
                }
                auto goal = socialworld::destination(
                    f, at<Byte>(0xB70153) * 60u + at<Byte>(0xB70152), registry.worldMinutes);
                showRoutineRoute(goal);
                phoneState.notice = "ROUTINE_ROUTE_READY";
            } else if (phoneState.detail != 3) {
                phoneState.page = phone::Page::Actions;
                phoneState.cursor = 0;
            }
        }
        return;
    } else {
        auto ids = phone::contacts(registry);
        unsigned count = page == phone::Page::Meetings
                             ? static_cast<unsigned>(registry.appointments.size())
                             : static_cast<unsigned>(ids.size());
        if (count) {
            phoneState.cursor %= count;
            if (up)
                phoneState.cursor = (phoneState.cursor + count - 1) % count;
            if (down)
                phoneState.cursor = (phoneState.cursor + 1) % count;
        }
        if (enter && count) {
            if (page == phone::Page::Contacts) {
                phoneState.person = ids[phoneState.cursor];
                phoneState.page = phone::Page::Contact;
                phoneState.networkOffset = 0;
            } else
                phoneState.page = phone::Page::Meeting;
            phoneState.detail = 0;
            return;
        }
    }
    if (left || right) {
        unsigned tab = phoneState.page == phone::Page::Inbox      ? 1u
                       : phoneState.page == phone::Page::Meetings ? 2u
                       : phoneState.page == phone::Page::Settings ? 3u
                                                                  : 0u;
        tab = (tab + (left ? 3u : 1u)) % 4;
        phoneState.page = tab == 0   ? phone::Page::Contacts
                          : tab == 1 ? phone::Page::Inbox
                          : tab == 2 ? phone::Page::Meetings
                                     : phone::Page::Settings;
        phoneState.cursor = 0;
    }
}
