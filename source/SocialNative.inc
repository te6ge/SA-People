// Free-roam routines and one companion activity. No actor is added to CJ's gang group.
struct RoutineRun {
    people::Handle actor{};
    void *task = nullptr;
    unsigned phase = 99;
};
std::unordered_map<unsigned, RoutineRun> routineRuns;
DWORD nextRoutineTick = 0, nextIncoming = 0;
unsigned routineCursor = 0;
int routineBlip = -1;
DWORD routineBlipUntil = 0;
std::unordered_map<unsigned, DWORD> avoidAfter;
void showRoutineRoute(people::Point goal) {
    if (routineBlip != -1)
        reinterpret_cast<void(__cdecl *)(int)>(0x587CE0)(routineBlip);
    routineBlip = reinterpret_cast<int(__cdecl *)(int, Vec, unsigned, int, char *)>(0x583820)(
        4, {goal.x, goal.y, goal.z}, 0, 2, nullptr);
    if (routineBlip != -1)
        reinterpret_cast<void(__cdecl *)(int, int)>(0x583D70)(routineBlip, 37);
    routineBlipUntil = GetTickCount() + 180000;
}
struct Companion {
    unsigned person = 0, activity = 0, stage = 0, choices = 0;
    int mood = 0;
    people::Handle actor{}, car{};
    void *task = nullptr;
    DWORD started = 0, nextMove = 0, nextChat = 0, lastChoice = 0;
    DWORD elapsed = 0, lastTick = 0;
    Vec lastPosition{};
    float travelled = 0, poolPlayed = 0;
    DWORD sceneAt = 0; // Meal scene start (stage 10 = eating, 11 = faded out).
} companion;
bool companionOwns(unsigned id) {
    return companion.person == id;
}
void releaseRoutine(unsigned id) {
    auto found = routineRuns.find(id);
    if (found == routineRuns.end())
        return;
    auto pool = at<Pool *>(0xB74490);
    auto &r = found->second;
    if (validPool(pool) && exists(pool, r.actor)) {
        auto manager = taskManager(pedAt(pool, r.actor.slot));
        if (manager && r.task &&
            at<void *>(reinterpret_cast<std::uintptr_t>(manager) + 12) == r.task)
            setTask(manager, nullptr, 3);
    }
    routineRuns.erase(found);
}
void clearRoutines() {
    while (!routineRuns.empty())
        releaseRoutine(routineRuns.begin()->first);
    if (routineBlip != -1)
        reinterpret_cast<void(__cdecl *)(int)>(0x587CE0)(routineBlip);
    routineBlip = -1;
}
void *walkingTask(Byte *ped, Vec target) {
    if (!ped || busy(ped) || primaryOccupied(ped))
        return nullptr;
    auto task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x60);
    if (!task)
        return nullptr;
    reinterpret_cast<void(__thiscall *)(void *, int, const Vec &, float, float, float, bool, int,
                                        bool)>(0x66EA30)(task, 4, target, 2, 3, 2, false, -1, true);
    setTask(taskManager(ped), task, 3);
    return task;
}
void releaseCompanionTask() {
    auto pool = at<Pool *>(0xB74490);
    if (validPool(pool) && exists(pool, companion.actor)) {
        auto manager = taskManager(pedAt(pool, companion.actor.slot));
        if (manager && companion.task &&
            at<void *>(reinterpret_cast<std::uintptr_t>(manager) + 12) == companion.task)
            setTask(manager, nullptr, 3);
    }
    companion.task = nullptr;
}
void mealGesture(Pool *pool, people::Handle actor) {
    if (!exists(pool, actor) || gestures.size() >= 8)
        return;
    auto block = reinterpret_cast<Byte *(__cdecl *)(const char *)>(0x4D3940)("FOOD");
    if (!block || !block[0x10] ||
        !reinterpret_cast<void *(__cdecl *)(const char *, void *)>(0x4D42F0)("EAT_Burger", block))
        return;
    auto ped = pedAt(pool, actor.slot);
    auto manager = taskManager(ped);
    if (!manager || at<void *>(reinterpret_cast<std::uintptr_t>(manager) + 0x24))
        return;
    auto task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x64);
    if (!task)
        return;
    reinterpret_cast<void(__thiscall *)(void *, const char *, const char *, unsigned, float,
                                        unsigned, bool, bool, bool, bool)>(0x61A990)(
        task, "EAT_Burger", "FOOD", 0, 4.0f, 3500, false, false, false, false);
    setSecondary(manager, task, 4);
    gestures.push_back({actor, task, GetTickCount() + 4000});
}
void cancelCompanion(bool fail) {
    if (!companion.person)
        return;
    if (companion.stage == 11)
        fadeVisit(false); // Never leave the screen black.
    releaseCompanionTask();
    if (fail) {
        auto &f = registry.families[companion.person - 1];
        for (auto &a : registry.appointments)
            if (a.person == f.id && a.state == 2)
                socialworld::complete(registry, a, f, false, false);
        dirty = true;
    }
    auto pool = at<Pool *>(0xB74490);
    if (validPool(pool) && exists(pool, companion.actor)) {
        auto ped = pedAt(pool, companion.actor.slot);
        auto car = *reinterpret_cast<Byte **>(ped + 0x58C);
        if (!onFoot(ped) && car && !busy(ped) && !primaryOccupied(ped) && !recruited(ped)) {
            auto task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x34);
            if (task) {
                reinterpret_cast<void(__thiscall *)(void *, Byte *, int, int, bool, bool)>(
                    0x63B8C0)(task, car, 0, 0, true, false);
                ownResponse(ped, task, 10000);
            }
        }
    }
    companion = {};
}
bool beginCompanion(people::Appointment &a, people::Family &f, people::Handle actor) {
    if (companion.person || lifeActive() || !socialworld::available(f))
        return false;
    releaseRoutine(f.id);
    companion = {};
    companion.person = f.id;
    companion.actor = actor;
    companion.activity = a.activity;
    companion.started = GetTickCount();
    companion.lastTick = companion.started;
    companion.nextChat = GetTickCount() + 22000;
    companion.poolPlayed = reinterpret_cast<float(__cdecl *)(unsigned short)>(0x558E40)(196) +
                           reinterpret_cast<float(__cdecl *)(unsigned short)>(0x558E40)(197);
    auto player = reinterpret_cast<Byte *(__cdecl *)(int)>(0x56E210)(0);
    companion.lastPosition = position(player);
    phoneState.person = f.id;
    say(f, a.activity == 0   ? "ACT_MEAL_START"
           : a.activity == 1 ? "ACT_POOL_START"
           : a.activity == 2 ? "ACT_DRIVE_START"
                             : "ACT_WALK_START");
    help("ACT_CONTROLS");
    return true;
}
void companionChoice(unsigned choice) {
    if (!companion.person || choice > 2 || GetTickCount() - companion.lastChoice < 2800 ||
        (companion.choices & (1u << choice)))
        return;
    companion.lastChoice = GetTickCount();
    companion.choices |= 1u << choice;
    auto &f = registry.families[companion.person - 1];
    bool good = choice != 1 || people::mix(f.id + f.daily.outings) % 4 != 0;
    companion.mood += good ? 2 : -1;
    say(f, choice == 0   ? "ACT_REPLY_CATCHUP"
           : choice == 2 ? "ACT_REPLY_LISTEN"
           : good        ? "ACT_REPLY_JOKE"
                         : "ACT_REPLY_BAD_JOKE");
    auto pool = at<Pool *>(0xB74490);
    if (validPool(pool) && exists(pool, companion.actor) &&
        onFoot(pedAt(pool, companion.actor.slot)))
        gesture(pool, companion.actor, 12);
    audio.speak(audioChoices.choose(
        choice == 1 ? socialaudio::Context::Joke : socialaudio::Context::Reassure, GetTickCount()));
}
bool companionAction(unsigned id, street::Action action) {
    if (companion.person != id)
        return false;
    if (companion.stage >= 10)
        return true; // The meal scene is playing.
    if (companion.activity == 0 && action == street::Action::Greet) {
        // Meal date: one greeting sits you down together; the scene plays out by itself.
        companion.stage = 10;
        companion.sceneAt = GetTickCount();
        auto pool = at<Pool *>(0xB74490);
        auto player = reinterpret_cast<Byte *(__cdecl *)(int)>(0x56E210)(0);
        if (validPool(pool) && player) {
            mealGesture(pool, companion.actor);
            auto delta = player - pool->objects;
            if (delta >= 0 && delta % 0x7C4 == 0 && delta / 0x7C4 < pool->size)
                mealGesture(pool, handle(pool, static_cast<unsigned>(delta / 0x7C4)));
        }
        say(registry.families[id - 1], "ACT_MEAL_SCENE");
        audio.speak(audioChoices.choose(socialaudio::Context::FriendHello, GetTickCount()));
        return true;
    }
    if (action == street::Action::Provoke) {
        auto &f = registry.families[id - 1];
        say(f, "ACT_ABORT_ARGUMENT");
        cancelCompanion(true);
        save();
    } else
        companionChoice(action == street::Action::Defuse ? 2u : (companion.choices & 1u) ? 1u : 0u);
    return true;
}
void finishCompanion(Byte *player) {
    if (!companion.person)
        return;
    auto &f = registry.families[companion.person - 1];
    auto actor = companion.actor;
    for (auto &a : registry.appointments)
        if (a.person == f.id && a.state == 2) {
            bool good = companion.mood >= 0;
            if (good && companion.activity == 0) {
                auto money = playerMoney(player);
                if (!money || *money < 20) {
                    help("ACT_NO_MONEY");
                    return;
                }
                *money -= 20;
                auto &health = *reinterpret_cast<float *>(player + 0x540);
                health = std::min(*reinterpret_cast<float *>(player + 0x544), health + 10);
                auto pool = at<Pool *>(0xB74490);
                if (validPool(pool))
                    mealGesture(pool, actor);
            }
            socialworld::complete(registry, a, f, good, romance::eligible(registry, f));
            say(f, good ? "ACT_GOOD_END" : "ACT_BAD_END");
            dirty = true;
            panelInvalid = true;
            cancelCompanion(false);
            auto pool = at<Pool *>(0xB74490);
            if (good && romance::homeReady(f) && validPool(pool))
                offerPartnerHome(pool, player, f, actor);
            save();
            return;
        }
    cancelCompanion(false);
}
// Meal scene: both eat for a moment, the screen fades, an hour passes, the contact has left.
void runMealScene(Pool *pool, Byte *player) {
    DWORD age = GetTickCount() - companion.sceneAt;
    if (companion.stage == 10 && age > 3500) {
        companion.stage = 11;
        fadeVisit(true);
        return;
    }
    if (companion.stage != 11 || age < 4700)
        return;
    auto &f = registry.families[companion.person - 1];
    for (auto &a : registry.appointments)
        if (a.person == f.id && a.state == 2) {
            if (auto money = playerMoney(player))
                *money -= std::min(*money, 20);
            auto &health = *reinterpret_cast<float *>(player + 0x540);
            health = std::min(*reinterpret_cast<float *>(player + 0x544), health + 25);
            socialworld::complete(registry, a, f, true, romance::eligible(registry, f));
            f.social.trust = std::min(100, f.social.trust + 6);
        }
    unsigned hour = at<Byte>(0xB70153) + 1u;
    if (hour >= 24)
        reinterpret_cast<void(__cdecl *)(unsigned)>(0x52D0B0)(1);
    reinterpret_cast<void(__cdecl *)(Byte, Byte, Byte)>(0x52D150)(static_cast<Byte>(hour % 24),
                                                                at<Byte>(0xB70152), 0);
    auto actor = companion.actor;
    cancelCompanion(false);
    if (validPool(pool) && exists(pool, actor)) {
        auto ped = pedAt(pool, actor.slot);
        Vec away{};
        if (onFoot(ped) && !recruited(ped) && meetingSpawn(position(player), player, away, true))
            teleportActor(ped, away); // Gone by the time the screen comes back.
    }
    fadeVisit(false);
    dirty = true;
    panelInvalid = true;
    save();
    help(locale::format("ACT_MEAL_SCENE_DONE", dialogue::displayName(f)));
}
void updateCompanion(Pool *pool, Byte *player) {
    if (!companion.person)
        return;
    if (companion.stage >= 10) {
        runMealScene(pool, player);
        return;
    }
    DWORD now = GetTickCount();
    companion.elapsed += std::min<DWORD>(250, now - companion.lastTick);
    companion.lastTick = now;
    auto &f = registry.families[companion.person - 1];
    bool appointment = false;
    for (const auto &a : registry.appointments)
        if (a.person == f.id && meetings::active(a))
            appointment = true;
    if (!appointment || !socialworld::available(f) ||
        companion.elapsed > (companion.activity == 1 ? 2700000u : 540000u)) {
        cancelCompanion(false);
        return;
    }
    // Pool completion is tied to the real game's wins/losses, never a fake timer.
    if (companion.activity == 1) {
        float played = reinterpret_cast<float(__cdecl *)(unsigned short)>(0x558E40)(196) +
                       reinterpret_cast<float(__cdecl *)(unsigned short)>(0x558E40)(197);
        if (played > companion.poolPlayed) {
            finishCompanion(player);
            return;
        }
    }
    if (player[0x2F])
        return;
    if (!exists(pool, companion.actor)) {
        people::Handle actor{};
        if (bindings.locate(f.id, actor) && exists(pool, actor))
            companion.actor = actor;
        else if (!pending.id) {
            Vec spawn{};
            if (meetingSpawn(position(player), player, spawn))
                requestReturn(f.id, spawn, false, true);
        }
        return;
    }
    auto ped = pedAt(pool, companion.actor.slot);
    if (!eligible(ped, player) || recruited(ped)) {
        cancelCompanion(false);
        return;
    }
    auto manager = taskManager(ped);
    bool own = companion.task && manager &&
               at<void *>(reinterpret_cast<std::uintptr_t>(manager) + 12) == companion.task;
    if (!own)
        companion.task = nullptr;
    if (busy(ped) || (primaryOccupied(ped) && !own))
        return;
    float apart = distance2(position(player), position(ped));
    if (apart > 22500) {
        cancelCompanion(true);
        help("ACT_LEFT_BEHIND");
        return;
    }
    if (companion.activity == 2) {
        auto car = drivenCar(player);
        if (car && onFoot(ped) && stoppedCar(car) && freeSeat(car) >= 0 && !own) {
            auto cars = at<Pool *>(0xB74494);
            if (!validPool(cars))
                return;
            auto task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x50);
            if (!task)
                return;
            reinterpret_cast<void(__thiscall *)(void *, Byte *, int, bool)>(0x640340)(
                task, car, freeSeat(car), true);
            setTask(manager, task, 3);
            companion.task = task;
            companion.car = carHandle(cars, static_cast<unsigned>((car - cars->objects) / 0xA18));
            companion.stage = 1;
            companion.lastPosition = position(player);
        }
        if (companion.stage && !onFoot(ped)) {
            auto cars = at<Pool *>(0xB74494);
            if (!carExists(cars, companion.car) ||
                *reinterpret_cast<Byte **>(ped + 0x58C) != carAt(cars, companion.car.slot)) {
                cancelCompanion(true);
                return;
            }
            if (car == carAt(cars, companion.car.slot)) {
                Vec pos = position(player);
                float segment = std::sqrt(distance2(pos, companion.lastPosition));
                if (segment < 20)
                    companion.travelled += segment;
                companion.lastPosition = pos;
            }
            if (companion.travelled >= 500 && companion.elapsed > 45000 &&
                (!car || stoppedCar(carAt(cars, companion.car.slot)))) {
                finishCompanion(player);
                return;
            }
        }
    } else {
        if (companion.activity == 3 && onFoot(player)) {
            Vec pos = position(player);
            float segment = std::sqrt(distance2(pos, companion.lastPosition));
            if (segment < 5 && apart < 625)
                companion.travelled += segment;
            companion.lastPosition = pos;
        }
        if (onFoot(player) && onFoot(ped) && apart > 16 &&
            due(GetTickCount(), companion.nextMove)) {
            releaseCompanionTask();
            companion.task = walkingTask(ped, position(player));
            companion.nextMove = GetTickCount() + 3000;
        }
        unsigned choices = ((companion.choices & 1) != 0) + ((companion.choices & 2) != 0) +
                           ((companion.choices & 4) != 0);
        if (companion.activity == 0 && choices >= 2 && apart < 100 && companion.elapsed >= 30000) {
            finishCompanion(player);
            return;
        }
        if (companion.activity == 3 && choices >= 2 && companion.travelled >= 150 && apart < 100 &&
            companion.elapsed >= 30000) {
            finishCompanion(player);
            return;
        }
    }
    if (due(GetTickCount(), companion.nextChat) && apart < 625 && !nativeConversation(player)) {
        companion.nextChat = GetTickCount() + 25000;
        const char *memory = socialworld::memory(f);
        say(f, *memory ? memory : "ACT_COMPANION_CHAT");
    }
}
void prepareRoutine(people::Family &f, Byte *player) {
    if (f.daily.ready || !f.social.journal || !f.presence.located || f.presence.area)
        return;
    Vec home{f.presence.x, f.presence.y, f.presence.z};
    people::Handle door{};
    Vec house{};
    if (distance2(home, position(player)) < 122500 && findHome(home, door, house))
        home = house;
    f.daily.home = {home.x, home.y, home.z};
    const meetings::Venue *nearest = nullptr;
    float best = 1e12f;
    for (const auto &v : meetings::venues()) {
        float dx = v.x - home.x, dy = v.y - home.y, d = dx * dx + dy * dy;
        if (d < best) {
            best = d;
            nearest = &v;
        }
    }
    if (!nearest)
        return;
    f.daily.leisure = {nearest->x, nearest->y, nearest->z};
    f.daily.work = f.daily.leisure;
    if (people::generate(f)[0].job == "Tamirci") {
        const Vec garages[] = {{1041, -1025, 31.7f}, {-2025, 143, 28.8f}, {2386, 1048, 10.8f}};
        best = 1e12f;
        for (auto p : garages) {
            float d = distance2(home, p);
            if (d < best) {
                best = d;
                f.daily.work = {p.x, p.y, p.z};
            }
        }
    }
    f.daily.ready = true;
    dirty = true;
}
void updateSocialWorld(Pool *pool, Byte *player) {
    updateCompanion(pool, player);
    if (routineBlip != -1 && due(GetTickCount(), routineBlipUntil)) {
        reinterpret_cast<void(__cdecl *)(int)>(0x587CE0)(routineBlip);
        routineBlip = -1;
    }
    if (!due(GetTickCount(), nextRoutineTick))
        return;
    nextRoutineTick = GetTickCount() + 1500;
    if (player[0x2F] || nativeConversation(player)) {
        clearRoutines();
        return;
    }
    unsigned minute = at<Byte>(0xB70153) * 60u + at<Byte>(0xB70152);
    for (auto &f : registry.families) {
        if (!f.social.journal && !f.daily.ready)
            continue;
        prepareRoutine(f, player);
        if (!companionOwns(f.id) && !hasAppointment(f.id) && !f.social.feud &&
            (f.social.robbed || (f.social.trust < -10 && f.social.resentment >= 25)) &&
            (!avoidAfter.count(f.id) || due(GetTickCount(), avoidAfter[f.id]))) {
            people::Handle actor{};
            if (bindings.locate(f.id, actor) && exists(pool, actor)) {
                auto ped = pedAt(pool, actor.slot);
                if (eligible(ped, player) && onFoot(ped) && !busy(ped) && !primaryOccupied(ped) &&
                    distance2(position(ped), position(player)) < 64 && startFlee(ped, player))
                    avoidAfter[f.id] = GetTickCount() + 120000;
            }
        }
        if (!socialworld::available(f) || !f.daily.ready || hasAppointment(f.id) ||
            companionOwns(f.id) || lifeEvent.person == f.id || roadside.person == f.id ||
            smallEvent.person == f.id || focused == f.id) {
            releaseRoutine(f.id);
            continue;
        }
        auto target = socialworld::destination(f, minute, registry.worldMinutes);
        Vec destination{target.x, target.y, target.z};
        if (distance2(position(player), destination) > 14400) {
            releaseRoutine(f.id);
            continue;
        }
        Vec sidewalk{};
        if (!streetPoint(destination, player, sidewalk))
            continue;
        people::Handle actor{};
        if (!bindings.locate(f.id, actor) || !exists(pool, actor)) {
            Vec spawn{};
            if (autoRestore && !pending.id && hasRoom(pool) &&
                meetingSpawn(sidewalk, player, spawn))
                requestReturn(f.id, spawn, false, false);
            continue;
        }
        auto ped = pedAt(pool, actor.slot);
        if (!eligible(ped, player) || !onFoot(ped) || busy(ped) || ped[0x2F]) {
            releaseRoutine(f.id);
            continue;
        }
        auto found = routineRuns.find(f.id);
        if (found != routineRuns.end() && found->second.phase != socialworld::phase(minute))
            releaseRoutine(f.id);
        auto &run = routineRuns[f.id];
        run.actor = actor;
        run.phase = socialworld::phase(minute);
        auto manager = taskManager(ped);
        if (run.task && at<void *>(reinterpret_cast<std::uintptr_t>(manager) + 12) != run.task)
            run.task = nullptr;
        if (distance2(position(ped), sidewalk) > 6400 && !visibleTarget(ped) &&
            !primaryOccupied(ped)) {
            Vec spawn{};
            if (meetingSpawn(sidewalk, player, spawn)) {
                teleportActor(ped, spawn);
                capture(f, ped);
            }
        }
        if (!primaryOccupied(ped) && !run.task && distance2(position(ped), sidewalk) > 16 &&
            std::abs(position(ped).z - sidewalk.z) < 2)
            run.task = walkingTask(ped, sidewalk);
    }
    if (due(GetTickCount(), nextIncoming)) {
        auto contacts = phone::contacts(registry);
        if (!contacts.empty()) {
            for (unsigned i = 0; i < contacts.size(); ++i) {
                unsigned id = contacts[(routineCursor + i) % contacts.size()];
                if (!hasAppointment(id) && !companionOwns(id) &&
                    socialworld::spontaneous(registry, registry.families[id - 1], minute)) {
                    nextIncoming = GetTickCount() + 90000;
                    routineCursor += i + 1;
                    help("MAIL_NEW_MESSAGE");
                    dirty = true;
                    panelInvalid = true;
                    break;
                }
            }
        }
    }
}
