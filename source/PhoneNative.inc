// Phone input and appointment actors. Only tasks and control bits owned here are released.
phone::State phoneState;
phone::LedgerSession ledgerSession;
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
                                    settingsFile.c_str()))
        log("Could not save language setting");
    panelInvalid = true;
}
void closePhone() {
    if (phoneOwnsControl && phoneControl)
        *phoneControl = static_cast<unsigned short>(*phoneControl & ~0x4000u);
    phoneOwnsControl = false;
    phoneControl = nullptr;
    panelOpen = false;
    ledgerSession.close(phoneState);
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
    if(auto reason=meetings::unavailable(registry,id)){phoneState.notice=reason;return;}
    DWORD now=GetTickCount();
    if(nextInvitation.count(id) && !due(now,nextInvitation[id])) {
        phoneState.notice="PHONE_INVITE_BUSY";return;
    }
    const bool mission=storyActive();
    const auto playerCar=drivenCar(player);
    const bool parkedDriver=playerCar && stoppedCar(playerCar);
    if(!meetings::safeToPlan(mission,player[0x2F]==0,busy(player),onFoot(player),parkedDriver)) {
        phoneState.notice="PHONE_INVITE_SAFE";return;
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
    if(auto reason=community::request(registry,id)){phoneState.notice=reason;dirty=true;return;}
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
            protectOutingActor(pool, actor, a.person);
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
        controls::save(settingsFile.c_str());
    }
    // Swallow the press so the new key does not also trigger its old meaning this frame.
    phoneUp = phoneDown = phoneLeft = phoneRight = phoneEnter = phoneBack = true;
}
// Phone → contact → Ask a favor (Favors.h). Effects apply at once; the friend texts back.
void askFavor(Byte *player, unsigned id) {
    if(!id || id>registry.families.size())return;
    const auto &f=registry.families[id-1];
    if(stories::unavailable(registry,f)){phoneState.notice=stories::lifeStatus(registry,f);return;}
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
bool groveAtHome(Byte *player) {
    if(!player)return false;
    const auto p=position(player);return grove::nearby(p.x,p.y,p.z,player[0x2F]);
}
bool groveSafe(Byte *player) {
    if(!player || storyActive() || *reinterpret_cast<float *>(player+0x540)<=0)return false;
    auto car=drivenCar(player);
    if(!onFoot(player) && !(car && stoppedCar(car)))return false;
    auto wanted=reinterpret_cast<Byte *(__cdecl *)(int)>(0x56E230)(0);
    return wanted && *reinterpret_cast<unsigned *>(wanted+0x2C)==0;
}
bool groveAtLedger(Byte *player) {
    if(!player)return false;const auto p=position(player);
    return grove::atLedger(p.x,p.y,p.z,player[0x2F]);
}
ledgerpointer::State grovePointer;
void openGroveLedger(Byte *player) {
    if(panelOpen && phone::ledgerPage(phoneState.page)){closePhone();return;}
    if(!player || !onFoot(player) || !groveAtLedger(player)){help("GROVE_LEDGER_VISIT");return;}
    if(!groveSafe(player)){help("GROVE_UNSAFE");return;}
    if(panelOpen)closePhone();
    // Acquire the modal control bit directly, without opening or navigating the phone.
    auto pad=reinterpret_cast<Byte *(__cdecl *)(int)>(0x53FB70)(0);
    if(!pad)return;
    auto flags=reinterpret_cast<unsigned short *>(pad+0x10E);
    if(*flags)return;
    phoneControl=flags;*flags|=0x4000;phoneOwnsControl=true;panelOpen=true;
    ledgerSession.open(phoneState);
    grovePointer.open((GetAsyncKeyState(VK_LBUTTON)&0x8000)!=0,(GetAsyncKeyState(VK_RBUTTON)&0x8000)!=0);
    panelTouched=GetTickCount();panelInvalid=true;
}
void groveSupply(Byte *player,unsigned project) {
    grove::Visit visit;visit.here=groveAtHome(player);visit.safe=groveSafe(player);
    if(!player){phoneState.notice="GROVE_UNSAFE";return;}
    visit.onFoot=onFoot(player);
    visit.health=*reinterpret_cast<float *>(player+0x540);
    visit.maxHealth=*reinterpret_cast<float *>(player+0x544);
    visit.armor=*reinterpret_cast<float *>(player+0x548);
    auto car=drivenCar(player);visit.driver=car!=nullptr;visit.parked=car && stoppedCar(car);
    if(car)visit.carHealth=*reinterpret_cast<float *>(car+0x4C0);
    phoneState.notice=grove::use(registry,project,visit);
    if(phoneState.notice=="GROVE_FOOD_DONE")*reinterpret_cast<float *>(player+0x540)=visit.health;
    else if(phoneState.notice=="GROVE_ARMOR_DONE")*reinterpret_cast<float *>(player+0x548)=visit.armor;
    else if(phoneState.notice=="GROVE_REPAIR_DONE" && car)*reinterpret_cast<float *>(car+0x4C0)=visit.carHealth;
    else return;
    dirty=panelInvalid=true;save();
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
        if(controls::padActions[controls::PhoneBack] || controls::padActions[controls::PhoneClose]) {
            phoneState.capturing=false;panelInvalid=true;
            phoneBack=controls::padActions[controls::PhoneBack]; // Cancel once; a held B must not also leave the page.
            if(controls::padActions[controls::PhoneClose])closePhone();
            return;
        }
        captureControl();
        return;
    }
    bool up=false,down=false,left=false,right=false,enter=false,back=false;
    if(!ledgerSession.active) {
        up=pressAction(controls::PhoneUp,phoneUp);down=pressAction(controls::PhoneDown,phoneDown);
        left=pressAction(controls::PhoneLeft,phoneLeft);right=pressAction(controls::PhoneRight,phoneRight);
        enter=pressAction(controls::PhoneSelect,phoneEnter);back=pressAction(controls::PhoneBack,phoneBack);
    }
    deliverReplies();
    if (!panelOpen)
        return;
    if(phone::ledgerPage(phoneState.page)) {
        DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
        const bool active=foreground==GetCurrentProcessId();
        const unsigned page=phoneState.page==phone::Page::Grove?0u:phoneState.page==phone::Page::GroveDonate?1u:
            phoneState.page==phone::Page::GroveUpgrades?2u:phoneState.page==phone::Page::GroveSupplies?3u:4u;
        const float scale=phone::ledgerLayout(static_cast<float>(at<int>(0xC17044)),static_cast<float>(at<int>(0xC17048)),panelScale).scale;
        const float dx=at<float>(0xB73424),dy=at<float>(0xB73428); // CPad::NewMouseControllerState movement.
        const int action=grovePointer.update(page,dx/std::max(.1f,scale),dy/std::max(.1f,scale),
            (GetAsyncKeyState(VK_LBUTTON)&0x8000)!=0,(GetAsyncKeyState(VK_RBUTTON)&0x8000)!=0,active,!phoneState.notice.empty());
        phoneState.ledgerX=grovePointer.x;phoneState.ledgerY=grovePointer.y;
        panelInvalid=true;
        auto key=[](int code){return (GetAsyncKeyState(code)&0x8000)!=0;};
        const unsigned keys=grovePointer.keyboard({active&&(key('W')||key(VK_UP)),
            active&&(key('S')||key(VK_DOWN)),active&&(key('E')||key('D')||key(VK_RETURN)||key(VK_RIGHT)),
            active&&(key('Q')||key('A')||key(VK_LEFT)||key(VK_BACK))});
        up=(keys&1)!=0;down=(keys&2)!=0;enter=(keys&4)!=0;back=(keys&8)!=0;
        if(action==-2){closePhone();return;}
        if(action==-3)back=true;
        if(action==-4){phoneState.notice.clear();return;}
        if(action>=0){phoneState.cursor=static_cast<unsigned>(action);enter=true;}
    }

    phoneState.typing = replyPending(phoneState.person);
    phoneState.legacyImport=canImportSharedProfile();
    phoneState.groveHere=groveAtLedger(player);
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
        if(phone::ledgerPage(page))phoneState.page=phone::ledgerBack(page,phoneState.groveMode);
        else if(page==phone::Page::ImportShared)phoneState.page=phone::Page::Settings;
        else if(page==phone::Page::History || page==phone::Page::Peace)phoneState.page=phone::Page::Stories;
        else if(page==phone::Page::PeaceConfirm)phoneState.page=phone::Page::Peace;
        else if(page==phone::Page::Stories || page==phone::Page::AskAbout || page==phone::Page::Care)phoneState.page=phone::Page::Actions;
        else if(page==phone::Page::RelationshipConfirm)phoneState.page=phone::Page::Relationship;
        else if(page==phone::Page::Relationship)phoneState.page=phone::Page::Actions;
        else if (page == phone::Page::Meeting)
            phoneState.page = phone::Page::Meetings;
        else if (page == phone::Page::Compose)
            openThread(phoneState.person);
        else if (page == phone::Page::Messages || page == phone::Page::Activities ||
                 page == phone::Page::Outing || page == phone::Page::Gift || page == phone::Page::Request)
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
    if(page==phone::Page::ImportShared) {
        phoneState.cursor%=2;
        if(up || down)phoneState.cursor=1-phoneState.cursor;
        if(enter) {
            if(!phoneState.cursor)importSharedProfile();
            else {phoneState.page=phone::Page::Settings;phoneState.cursor=0;}
        }
        return;
    }
    if(page==phone::Page::Grove || page==phone::Page::GroveDonate || page==phone::Page::GroveUpgrades ||
       page==phone::Page::GroveSupplies || page==phone::Page::GroveConfirm) {
        const unsigned count=page==phone::Page::Grove?4u:page==phone::Page::GroveConfirm?2u:page==phone::Page::GroveDonate?3u:grove::Count;
        phoneState.cursor%=count;
        if(up)phoneState.cursor=(phoneState.cursor+count-1)%count;
        if(down)phoneState.cursor=(phoneState.cursor+1)%count;
        if(enter) {
            const auto choice=phoneState.cursor;
            if(page==phone::Page::Grove) {
                if(choice==3){showRoutineRoute({2487.5f,-1686.0f,13.4f});phoneState.notice="GROVE_ROUTE_READY";return;}
                phoneState.page=choice==0?phone::Page::GroveDonate:choice==1?phone::Page::GroveUpgrades:phone::Page::GroveSupplies;
                phoneState.cursor=0;return;
            }
            if(page==phone::Page::GroveConfirm && choice) {
                phoneState.page=phoneState.groveMode?phone::Page::GroveUpgrades:phone::Page::GroveDonate;
                phoneState.cursor=0;return;
            }
            if(page!=phone::Page::GroveSupplies && (!groveAtLedger(player) || !onFoot(player))){phoneState.notice="GROVE_LEDGER_VISIT";return;}
            if(!groveSafe(player)){phoneState.notice="GROVE_UNSAFE";return;}
            if(page==phone::Page::GroveSupplies){groveSupply(player,choice);return;}
            if(page==phone::Page::GroveConfirm) {
                auto money=playerMoney(player);
                phoneState.notice=phoneState.groveMode?grove::upgrade(registry,phoneState.groveProject,phoneState.groveAmount):
                    money?grove::donate(registry,*money,phoneState.groveAmount):"GROVE_NO_CASH";
                if(phoneState.notice=="GROVE_BUILT" || phoneState.notice=="GROVE_DONATED") {dirty=true;save();}
                phoneState.page=phoneState.groveMode?phone::Page::GroveUpgrades:phone::Page::GroveDonate;
                phoneState.cursor=0;return;
            }
            phoneState.groveMode=page==phone::Page::GroveUpgrades?1u:0u;
            phoneState.groveProject=choice;
            const unsigned amounts[]={100,1000,10000};
            phoneState.groveAmount=phoneState.groveMode?people::GroveFund::cost(choice,registry.grove.level[choice]):amounts[choice];
            if(!phoneState.groveAmount){phoneState.notice="GROVE_MAX";return;}
            phoneState.page=phone::Page::GroveConfirm;phoneState.cursor=1;return;
        }
        return; // The ledger never enters the phone tab cycle.
    }
    if(page==phone::Page::Care) {
        const unsigned count=contactcare::Count;
        phoneState.cursor%=count;
        if(up)phoneState.cursor=(phoneState.cursor+count-1)%count;
        if(down)phoneState.cursor=(phoneState.cursor+1)%count;
        if(enter && player && !storyActive()) {
            auto money=playerMoney(player);
            if(!money){phoneState.notice="CARE_REFUSED";return;}
            phoneState.notice=contactcare::apply(registry,phoneState.person,phoneState.cursor,*money);
            if(phoneState.notice=="CARE_SENT") {dirty=true;save();}
            panelInvalid=true;
        }
        return;
    }
    if(page==phone::Page::Stories || page==phone::Page::Peace || page==phone::Page::PeaceConfirm) {
        const unsigned count=page==phone::Page::PeaceConfirm?2u:3u;
        phoneState.cursor%=count;
        if(up)phoneState.cursor=(phoneState.cursor+count-1)%count;
        if(down)phoneState.cursor=(phoneState.cursor+1)%count;
        if(enter) {
            const auto choice=phoneState.cursor;
            if(page==phone::Page::Stories)phoneState.page=choice==0?phone::Page::History:choice==1?phone::Page::Peace:phone::Page::AskAbout;
            else if(page==phone::Page::PeaceConfirm) {
                if(!choice)storyPeace(player,true,true);
                phoneState.page=phone::Page::Peace;
            } else if(choice==0)storyPeace(player,false,true);
            else if(choice==1)phoneState.page=phone::Page::PeaceConfirm;
            else phoneState.page=phone::Page::Stories;
            phoneState.cursor=0;
        }
        return;
    }
    if(page==phone::Page::AskAbout) {
        auto ids=phone::contacts(registry);ids.erase(std::remove(ids.begin(),ids.end(),phoneState.person),ids.end());
        const auto count=static_cast<unsigned>(ids.size());
        if(count){phoneState.cursor%=count;if(up)phoneState.cursor=(phoneState.cursor+count-1)%count;if(down)phoneState.cursor=(phoneState.cursor+1)%count;}
        if(enter && count)storyInquiry(player);
        return;
    }
    if(page==phone::Page::News || page==phone::Page::History) {
        const auto events=stories::news(registry,page==phone::Page::History?phoneState.person:0);
        const auto count=static_cast<unsigned>(events.size());
        if(count){phoneState.cursor%=count;if(up)phoneState.cursor=(phoneState.cursor+count-1)%count;if(down)phoneState.cursor=(phoneState.cursor+1)%count;}
        if(enter && count) {
            const unsigned person=events[phoneState.cursor]->person;
            if(!stories::contact(registry.families[person-1])){phoneState.notice="ST_UNKNOWN_PERSON";return;}
            phoneState.person=person;phoneState.page=phone::Page::Stories;phoneState.cursor=0;return;
        }
        if(page==phone::Page::History)return;
        if(!left && !right)return;
    }
    if(page==phone::Page::Relationship || page==phone::Page::RelationshipConfirm) {
        const unsigned count=page==phone::Page::Relationship?3u:2u;
        if(up)phoneState.cursor=(phoneState.cursor+count-1)%count;
        if(down)phoneState.cursor=(phoneState.cursor+1)%count;
        if(enter) {
            if(page==phone::Page::RelationshipConfirm) {
                if(!phoneState.cursor)relationshipChoice(player);
                if(panelOpen)phoneState.page=phone::Page::Relationship;
            } else if(phoneState.cursor==0) {
                phoneState.notice=commitment::talk(registry,phoneState.person);dirty=true;save();
            } else if(phoneState.cursor==1) {
                if(phoneState.person && phoneState.person<=registry.families.size())
                    phoneState.relationshipStage=registry.families[phoneState.person-1].commitment.stage;
                phoneState.page=phone::Page::RelationshipConfirm;
            }
            else phoneState.page=phone::Page::Actions;
            phoneState.cursor=0;
        }
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
        unsigned count = page == phone::Page::Actions ? static_cast<unsigned>(phone::actionKeys(registry, phoneState).size())
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
            const std::string actionKey = phone::actionKeys(registry, phoneState)[action];
            if(actionKey=="CARE_TITLE") {phoneState.page=phone::Page::Care;phoneState.cursor=0;return;}
            if(actionKey=="ST_PERSONAL" || actionKey=="ST_ASK") {
                phoneState.page=actionKey=="ST_PERSONAL"?phone::Page::Stories:phone::Page::AskAbout;phoneState.cursor=0;return;
            }
            if(actionKey=="REL_ACTION") {phoneState.page=phone::Page::Relationship;phoneState.cursor=0;return;}
            if (actionKey == "BDAY_GIVE" || actionKey == "PHONE_REQUEST") {
                phoneState.page = actionKey == "BDAY_GIVE" ? phone::Page::Gift : phone::Page::Request;
                phoneState.cursor = 0;
                return;
            }
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
    if (page == phone::Page::Gift || page == phone::Page::Request) {
        if (up || down) phoneState.cursor = (phoneState.cursor + 1) % 2;
        if (enter) {
            if (page == phone::Page::Gift) {
                if (!phoneState.cursor) giveBirthdayGift(player);
                else { phoneState.page = phone::Page::Actions; phoneState.cursor = 0; }
            } else {
                if (!phoneState.cursor) acceptRequest(player);
                else if (bonds::decline(registry, phoneState.person)) {
                    dirty = true; save(); phoneState.notice = "REQ_DECLINED";
                } else phoneState.notice = "REQ_UNAVAILABLE";
                phoneState.page = phone::Page::Messages;
                phoneState.cursor = 0;
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
        bool invite = !messages.empty() && phonedialogue::planOffer(*messages.back());
        bool request = bonds::open(registry, phoneState.person);
        if (enter && request && !phoneState.textOffset) {
            phoneState.page = phone::Page::Request;
            phoneState.cursor = 0;
            return;
        }
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
    if(page==phone::Page::News) {
        // Navigation continues to the top-level tab switch below.
    } else if (page == phone::Page::Inbox) {
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
                controls::save(settingsFile.c_str());
                phoneState.notice = "CTRL_RESET_DONE";
            } else {
                phoneState.capturing = true;
                captureFresh = true;
            }
        }
        return;
    } else if (page == phone::Page::Life) {
        // Read-only summary; left/right switch pages below.
    } else if (page == phone::Page::Settings) {
        if (up)
            phoneState.cursor = (phoneState.cursor + 4) % 5;
        if (down)
            phoneState.cursor = (phoneState.cursor + 1) % 5;
        if(enter && phoneState.cursor%5==4) {
            if(phoneState.legacyImport){phoneState.page=phone::Page::ImportShared;phoneState.cursor=0;}
            else phoneState.notice="PROFILE_IMPORT_UNAVAILABLE";
            return;
        }
        if (enter && phoneState.cursor % 5 == 3) {
            phoneState.page = phone::Page::Controls;
            phoneState.cursor = 0;
            return;
        }
        if (enter) {
            auto ini = settingsFile.wstring();
            if (phoneState.cursor % 5 == 0)
                changeLanguage(1);
            else if (phoneState.cursor % 5 == 1) {
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
                       : phoneState.page == phone::Page::Life     ? 3u
                       : phoneState.page == phone::Page::News     ? 4u
                       : phoneState.page == phone::Page::Settings ? 5u
                                                                  : 0u;
        tab = (tab + (left ? 5u : 1u)) % 6;
        phoneState.page = tab == 0   ? phone::Page::Contacts
                          : tab == 1 ? phone::Page::Inbox
                          : tab == 2 ? phone::Page::Meetings
                          : tab == 3 ? phone::Page::Life
                          : tab == 4 ? phone::Page::News
                                     : phone::Page::Settings;
        phoneState.cursor = 0;
    }
}
