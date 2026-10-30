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
    DWORD started = 0, nextMove = 0, nextChat = 0;
    DWORD boardingAt = 0, nextRestore = 0, nextHint = 0, interruptedAt = 0;
    unsigned autoChats = 0;
    socialcontext::TalkChoices dialogue;
    socialcontext::Topics topics;
    bool stopNotified = false;
    unsigned helpedWitness = 0;
    DWORD helpPraiseAt = 0;
    DWORD elapsed = 0, lastTick = 0, aboardElapsed = 0;
    socialworld::Travel travel;
    float poolPlayed = 0;
    DWORD sceneAt = 0; // Meal scene start (stage 10 = eating, 11 = faded out).
    community::Progress path;
    float carHealth=-1;
    DWORD lastDriveWarning=0;
} companion;
struct Departure { unsigned person; people::Handle actor; DWORD expires, retry; };
std::vector<Departure> departures;
bool departureOwns(unsigned id) {
    for (const auto &d : departures) if (d.person == id) return true;
    return false;
}
void clearDepartures() {
    for (const auto &d : departures) releaseActor(d.person);
    departures.clear();
}
void updateDepartures(Pool *pool) {
    const DWORD now = GetTickCount();
    for (auto it = departures.begin(); it != departures.end();) {
        if (!exists(pool, it->actor) || onFoot(pedAt(pool, it->actor.slot)) || due(now, it->expires)) {
            releaseActor(it->person); it = departures.erase(it); continue;
        }
        auto ped = pedAt(pool, it->actor.slot);
        auto car = *reinterpret_cast<Byte **>(ped + 0x58C);
        if (car && stoppedCar(car) && !busy(ped) && !recruited(ped) && due(now, it->retry)) {
            auto task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x34);
            if (task) {
                reinterpret_cast<void(__thiscall *)(void *, Byte *, int, int, bool, bool)>(0x63B8C0)(
                    task, car, 0, 0, true, false);
                ownResponse(ped, task, 15000);
            }
            it->retry = now + 15000;
        }
        ++it;
    }
}
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
    log("ACT released identity=" + std::to_string(companion.person) +
        " activity=" + std::to_string(companion.activity) + " failed=" + (fail ? "1" : "0"));
    if (companion.stage == 11)
        fadeVisit(false); // Never leave the screen black.
    releaseCompanionTask();
    auto run = meetingRuns.find(companion.person);
    if (run != meetingRuns.end())
        run->second.activityStarted = false;
    if (fail) {
        auto &f = registry.families[companion.person - 1];
        for (auto &a : registry.appointments)
            if (a.person == f.id && a.state == 2)
                socialworld::complete(registry, a, f, false, false);
        dirty = true;
    }
    auto pool = at<Pool *>(0xB74490);
    if (!storyActive() && validPool(pool) && exists(pool, companion.actor)) {
        auto ped = pedAt(pool, companion.actor.slot);
        if (!onFoot(ped) && !recruited(ped))
            departures.push_back({companion.person, companion.actor, GetTickCount() + 120000, 0});
    }
    const unsigned releasedPerson = companion.person;
    companion = {};
    if (!departureOwns(releasedPerson)) releaseActor(releasedPerson);
}
bool beginCompanion(people::Appointment &a, people::Family &f, people::Handle actor) {
    if (companion.person || lifeActive() || !socialworld::available(f))
        return false;
    releaseRoutine(f.id);
    f.community.drivingWarnings=0;
    companion = {};
    companion.person = f.id;
    companion.actor = actor;
    retainActor(actor, f.id);
    companion.activity = a.activity;
    companion.started = GetTickCount();
    companion.lastTick = companion.started;
    companion.nextChat = GetTickCount() + 4000;
    companion.poolPlayed = reinterpret_cast<float(__cdecl *)(unsigned short)>(0x558E40)(196) +
                           reinterpret_cast<float(__cdecl *)(unsigned short)>(0x558E40)(197);
    auto player = reinterpret_cast<Byte *(__cdecl *)(int)>(0x56E210)(0);
    const auto origin = position(player);
    companion.travel.sample({origin.x, origin.y, origin.z}, false, 20);
    phoneState.person = f.id;
    f.daily.lastInvitation = static_cast<unsigned>(registry.worldMinutes / 1440 + 1);
    companion.mood = socialcontext::honorTone(registry.honor);
    if (commitment::booked(registry,f)) {
        if(beginCeremony(f,actor)) return true;
        cancelCompanion(false); return false;
    }
    say(f, registry.honor <= -40 ? "ACT_HONOR_WARY" : a.activity == 0 ? "ACT_MEAL_START"
           : a.activity == 1 ? "ACT_POOL_START"
           : a.activity == 2 ? "ACT_DRIVE_START"
                             : "ACT_WALK_START");
    help(a.activity == 2 ? "ACT_DRIVE_BOARD_HINT" : "ACT_BEGIN_HINT");
    log("ACT started identity=" + std::to_string(f.id) +
        " activity=" + std::to_string(a.activity));
    return true;
}
void companionChoice(unsigned choice) {
    if (!companion.person || choice > 2)
        return;
    if (companion.stage >= 10) return;
    const DWORD now = GetTickCount();
    const unsigned count = companion.dialogue.use(choice, now);
    if (!count) {
        help("ACT_TALK_WAIT");
        return;
    }
    const bool first = count == 1;
    companion.choices |= 1u << choice;
    auto &f = registry.families[companion.person - 1];
    bool good = choice != 1 || people::mix(f.id + f.daily.outings) % 4 != 0;
    if (first) companion.mood += good ? 2 : -1;
    static const char *catchup[] = {"ACT_REPLY_CATCHUP", "ACT_CATCHUP_1", "ACT_CATCHUP_2", "ACT_CATCHUP_3"};
    static const char *listen[] = {"ACT_REPLY_LISTEN", "ACT_LISTEN_1", "ACT_LISTEN_2", "ACT_LISTEN_3"};
    static const char *joke[] = {"ACT_REPLY_JOKE", "ACT_JOKE_1", "ACT_JOKE_2"};
    unsigned variant = (count - 1) % 4;
    say(f, choice == 0 ? catchup[variant] : choice == 2 ? listen[variant]
            : good ? joke[(count - 1) % 3] : "ACT_REPLY_BAD_JOKE");
    nextSpeech = now + 4000;
    companion.nextChat = now + 6500;
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
        socialworld::meetingArgument(f);
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
                const int meal = static_cast<int>(bonds::price(registry, playerDistrict(), 20));
                if (!money || *money < meal) {
                    help("ACT_NO_MONEY");
                    return;
                }
                *money -= meal;
                auto &health = *reinterpret_cast<float *>(player + 0x540);
                health = std::min(*reinterpret_cast<float *>(player + 0x544), health + 10);
                auto pool = at<Pool *>(0xB74490);
                if (validPool(pool))
                    mealGesture(pool, actor);
            }
            socialworld::complete(registry, a, f, good, romance::eligible(registry, f));
            if (good)
                bonds::count(registry, bonds::Stat::Outing);
            log("ACT completed identity=" + std::to_string(f.id) +
                " activity=" + std::to_string(companion.activity));
            say(f, good ? "ACT_GOOD_END" : "ACT_BAD_END");
            help(good ? "ACT_FINISHED" : "ACT_CANCELLED");
            dirty = true;
            panelInvalid = true;
            cancelCompanion(false);
            justMet(f, actor);
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
                *money -= std::min(*money, static_cast<int>(bonds::price(registry, playerDistrict(), 20)));
            bonds::count(registry, bonds::Stat::Outing);
            auto &health = *reinterpret_cast<float *>(player + 0x540);
            health = std::min(*reinterpret_cast<float *>(player + 0x544), health + 25);
            socialworld::complete(registry, a, f, true, romance::eligible(registry, f));
            log("ACT completed identity=" + std::to_string(f.id) + " activity=0 scene=meal");
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
void holdCompanion(Byte *ped) {
    if (companion.task || busy(ped) || primaryOccupied(ped) || !onFoot(ped)) return;
    auto task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x20);
    if (!task) return;
    reinterpret_cast<void(__thiscall *)(void *, int, bool, bool, float)>(0x62F310)(
        task, 60000, false, false, 4.0f);
    setTask(taskManager(ped), task, 3);
    companion.task = task;
}
void autoCompanionChat(people::Family &f, DWORD now) {
    if (companion.helpPraiseAt && due(now, companion.helpPraiseAt) && now >= subtitleUntil && !panelOpen) {
        companion.helpPraiseAt = 0;
        say(f, "ACT_WITNESS_HELP");
        companion.nextChat = now + 6500;
        return;
    }
    if (!due(now, companion.nextChat) || panelOpen || now < subtitleUntil) return;
    companion.nextChat = now + 6500 + people::mix(f.id + companion.autoChats) % 4000;
    static const char *topics[] = {"ACT_CHAT_0", "ACT_CHAT_1", "ACT_CHAT_2", "ACT_CHAT_3",
                                  "ACT_CHAT_4", "ACT_CHAT_5", "ACT_CHAT_6", "ACT_CHAT_7",
                                  "ACT_CHAT_8", "ACT_CHAT_9", "ACT_CHAT_10", "ACT_CHAT_11",
        "ACT_CHAT_12","ACT_CHAT_13","ACT_CHAT_14","ACT_CHAT_15","ACT_CHAT_16","ACT_CHAT_17",
        "ACT_CHAT_18","ACT_CHAT_19","ACT_CHAT_20","ACT_CHAT_21","ACT_CHAT_22","ACT_CHAT_23"};
    auto topic = companion.topics.take(24, people::mix(f.id + f.daily.outings * 7));
    if (topic >= 24) return;
    if (companion.autoChats == 0 && registry.honor >= 40) say(f, "ACT_HONOR_WARM");
    else if (companion.autoChats == 0 && registry.honor <= -40) say(f, "ACT_HONOR_WARY");
    else if(f.commitment.stage==3 && companion.autoChats%4==0) {
        static const char *couple[]={"REL_CHAT_0","REL_CHAT_1","REL_CHAT_2","REL_CHAT_3"};
        say(f,couple[(companion.autoChats/4)%4]);
    }
    else say(f, topics[topic]);
    ++companion.autoChats;
    if (companion.autoChats <= 2) ++companion.mood;
}
void companionWitness(unsigned person, socialcontext::Witness event) {
    if(event!=socialcontext::Witness::Provoke)
        observeDeed(person,event==socialcontext::Witness::Help?community::Deed::Help:
            event==socialcontext::Witness::Killing?community::Deed::Killing:community::Deed::Violence);
    if (!companion.person || person == companion.person || !person ||
        person > registry.families.size() || storyActive()) return;
    auto pool = at<Pool *>(0xB74490);
    if (!validPool(pool) || !exists(pool, companion.actor)) return;
    auto witness = pedAt(pool, companion.actor.slot);
    const auto &target = registry.families[person - 1].presence;
    const Vec where{target.x, target.y, target.z};
    if (target.area != witness[0x2F] || distance2(position(witness), where) > 1225) return;
    Vec eyes = position(witness), head = where;
    eyes.z += 1; head.z += 1;
    if (!reinterpret_cast<bool(__cdecl *)(const Vec &, const Vec &, bool, bool, bool, bool, bool, bool, bool)>(
        0x56A490)(eyes, head, true, false, false, true, true, false, false)) return;
    auto &f = registry.families[companion.person - 1];
    if (!socialcontext::witnessCancels(event)) {
        if (companion.helpedWitness == person) return;
        companion.helpedWitness = person;
        companion.mood = std::min(8, companion.mood + 2);
        f.social.trust = std::min(100, f.social.trust + 2);
        f.community.strain=f.community.strain>3?f.community.strain-3:0;
        companion.helpPraiseAt = GetTickCount() + 5000;
    } else {
        f.community.strain=std::min(100u,f.community.strain+(community::tie(registry,f.id,person)?20u:8u));
        socialworld::meetingArgument(f);
        say(f, event == socialcontext::Witness::Killing ? "ACT_WITNESS_KILL"
               : event == socialcontext::Witness::Provoke ? "ACT_WITNESS_PROVOKE" : "ACT_WITNESS_HURT");
        help("ACT_WITNESS_CANCEL");
        cancelCompanion(true);
    }
    dirty = panelInvalid = true;
    save();
}
void updateCompanion(Pool *pool, Byte *player) {
    if (!companion.person) return;
    if (companion.stage >= 20) { runWeddingScene(pool, player); return; }
    if (companion.stage >= 10) { runMealScene(pool, player); return; }
    const DWORD now = GetTickCount();
    const DWORD frameElapsed = std::min<DWORD>(250, now - companion.lastTick);
    companion.elapsed += frameElapsed;
    companion.lastTick = now;
    if(communityPairPauses(companion.person)) return;
    auto &f = registry.families[companion.person - 1];
    bool appointment = false;
    for (const auto &a : registry.appointments)
        if (a.person == f.id && meetings::active(a)) appointment = true;
    if (!appointment || !socialworld::available(f) || f.community.strain>=60 ||
        companion.elapsed > (companion.activity == 1 ? 2700000u : 540000u)) {
        help("ACT_CANCELLED");
        cancelCompanion(appointment); dirty = true; return;
    }
    if (companion.activity == 1) {
        const float played = reinterpret_cast<float(__cdecl *)(unsigned short)>(0x558E40)(196) +
                             reinterpret_cast<float(__cdecl *)(unsigned short)>(0x558E40)(197);
        if (played > companion.poolPlayed) { finishCompanion(player); return; }
    }
    if (player[0x2F]) return;
    if (!exists(pool, companion.actor) || bindings.find(companion.actor) != f.id) {
        people::Handle actor{};
        if (bindings.locate(f.id, actor) && exists(pool, actor)) {
            companion.actor = actor;
            companion.task = nullptr;
            companion.boardingAt = 0;
            retainActor(actor, f.id);
        } else if (!pending.id && due(now, companion.nextRestore)) {
            companion.nextRestore = now + 5000;
            Vec spawn{};
            if (meetingSpawn(position(player), player, spawn, true)) requestReturn(f.id, spawn, false, true);
        }
        return;
    }
    auto ped = pedAt(pool, companion.actor.slot);
    retainActor(companion.actor, f.id);
    if (!eligible(ped, player) || recruited(ped)) { cancelCompanion(true); return; }
    protectOutingActor(pool, companion.actor, f.id);
    auto manager = taskManager(ped);
    if (!manager) return;
    // Ambient idle tasks can outlive the meeting wait or replace a finished route.
    // Let those yield so they cannot permanently block passenger boarding.
    auto primary = at<void *>(reinterpret_cast<std::uintptr_t>(manager) + 12);
    if (primary && primary != companion.task && gangcombat::ambientTask(gangTaskId(primary)))
        setTask(manager, nullptr, 3);
    const bool own = companion.task && at<void *>(reinterpret_cast<std::uintptr_t>(manager) + 12) == companion.task;
    if (!own) companion.task = nullptr;
    float apart = distance2(position(player), position(ped));
    if (apart > 22500) { cancelCompanion(true); help("ACT_LEFT_BEHIND"); return; }
    if (busy(ped) || (onFoot(ped) && primaryOccupied(ped) && !own)) {
        if (!companion.interruptedAt) {
            companion.interruptedAt = now;
            log("ACT interrupted identity=" + std::to_string(f.id) +
                " physical=" + std::to_string(gangTaskId(at<void *>(reinterpret_cast<std::uintptr_t>(manager)))) +
                " temporary=" + std::to_string(gangTaskId(at<void *>(reinterpret_cast<std::uintptr_t>(manager) + 4))) +
                " response=" + std::to_string(gangTaskId(at<void *>(reinterpret_cast<std::uintptr_t>(manager) + 8))) +
                " primary=" + std::to_string(gangTaskId(at<void *>(reinterpret_cast<std::uintptr_t>(manager) + 12))));
        }
        if (now - companion.interruptedAt > 20000) {
            help("ACT_CANCELLED"); cancelCompanion(true);
        }
        return;
    }
    companion.interruptedAt = 0;
    if (companion.activity == 2) {
        auto car = drivenCar(player);
        auto cars = at<Pool *>(0xB74494);
        if (!validPool(cars)) return;
        bool aboard = !onFoot(ped);
        if (aboard) {
            auto ridingCar=carExists(cars,companion.car)?carAt(cars,companion.car.slot):nullptr;
            if(ridingCar) {
                const float health=*reinterpret_cast<float *>(ridingCar+0x4C0);
                const float damage=companion.carHealth<0?0:std::max(0.0f,companion.carHealth-health);
                if(community::unsafeRide(f,damage,false,now-companion.lastDriveWarning)) {
                    companion.lastDriveWarning=now;companion.mood-=2;dirty=true;
                    say(f,f.community.drivingWarnings>=3?"BOUND_DRIVE_END":"BOUND_DRIVE_WARN");
                    if(f.community.drivingWarnings>=3){cancelCompanion(true);return;}
                }
                companion.carHealth=health;
            }
            if (!carExists(cars, companion.car) ||
                *reinterpret_cast<Byte **>(ped + 0x58C) != carAt(cars, companion.car.slot) ||
                *reinterpret_cast<Byte **>(carAt(cars, companion.car.slot) + 0x460) == ped) {
                help("ACT_CANCELLED"); cancelCompanion(true); return;
            }
            if (companion.stage != 2) {
                companion.stage = 2;
                log("ACT passenger aboard identity=" + std::to_string(f.id));
                help("ACT_DRIVE_RUNNING");
            }
            const auto pos = position(player);
            if (car == carAt(cars, companion.car.slot)) companion.aboardElapsed += frameElapsed;
            companion.travel.sample({pos.x, pos.y, pos.z}, car == carAt(cars, companion.car.slot), 20);
            if (companion.travel.distance >= 500 && companion.aboardElapsed >= 45000) {
                if (!companion.stopNotified) { help("ACT_DRIVE_STOP"); say(f, "LIFE_DROP_OFF"); companion.stopNotified = true; }
                if (stoppedCar(carAt(cars, companion.car.slot))) { finishCompanion(player); return; }
            }
            if (car) autoCompanionChat(f, now);
            return;
        }
        if (companion.stage == 2) { help("ACT_CANCELLED"); cancelCompanion(true); return; }
        if (!car || !stoppedCar(car) || freeSeat(car) < 0 || apart > 196) {
            if (due(now, companion.nextHint)) { help("ACT_DRIVE_BOARD_HINT"); companion.nextHint = now + 20000; }
            if (companion.stage == 1) {
                releaseCompanionTask(); companion.stage = 0; companion.boardingAt = 0;
            }
            holdCompanion(ped);
            return;
        }
        if (companion.stage == 1 && own) {
            if (now - companion.boardingAt < 30000) return;
            releaseCompanionTask();
            companion.stage = 0;
            help("ACT_DRIVE_DOOR_BLOCKED");
            companion.nextMove = now + 5000;
        }
        if (!due(now, companion.nextMove)) return;
        releaseCompanionTask(); // Replace only our waiting/boarding task.
        const int door = socialcontext::passengerDoor(reservePassenger(car,companion.actor));
        if (door < 0) { holdCompanion(ped); return; }
        auto task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x50);
        if (!task) return;
        reinterpret_cast<void(__thiscall *)(void *, Byte *, int, bool)>(0x640340)(task, car, door, false);
        setTask(manager, task, 3);
        companion.task = task;
        companion.car = carHandle(cars, static_cast<unsigned>((car - cars->objects) / 0xA18));
        companion.stage = 1; companion.boardingAt = now;
        const auto pos = position(player);
        companion.travel.sample({pos.x, pos.y, pos.z}, false, 20);
        return;
    }
    if (companion.activity == 3 && onFoot(player)) {
        const auto pos = position(player);
        companion.travel.sample({pos.x, pos.y, pos.z}, apart < 625, 5);
    }
    if (onFoot(player) && onFoot(ped) && apart > 16 && due(now, companion.nextMove)) {
        if(companion.path.stuck(apart,now) && companion.path.retries>=2) {
            help("SERVICE_PATH_BLOCKED");cancelCompanion(true);return;
        }
        releaseCompanionTask();
        Vec goal=position(player),sidewalk{};
        if(companion.path.stuck(apart,now)) {
            companion.path.retried(now);
            if(streetPoint(goal,player,sidewalk))goal=sidewalk;
        }
        companion.task = walkingTask(ped, goal);
        companion.nextMove = now + 3000;
    } else if (apart <= 16) holdCompanion(ped);
    const unsigned choices = ((companion.choices & 1) != 0) + ((companion.choices & 2) != 0) + ((companion.choices & 4) != 0);
    if (companion.activity == 3 && (choices >= 2 || companion.autoChats >= 2) &&
        companion.travel.distance >= 150 && apart < 100 && companion.elapsed >= 30000) {
        finishCompanion(player); return;
    }
    if (companion.activity == 0 && companion.elapsed >= 8000 && apart < 36) {
        companionAction(f.id, street::Action::Greet); // Meal scene also starts without hunting for a key.
        return;
    }
    if (apart < 625 && !nativeConversation(player)) autoCompanionChat(f, now);
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
    updateDepartures(pool);
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
        // Existing helpful relationships can enter the new chain after updating as well.
        if (f.social.journal && f.social.helped && socialworld::available(f) &&
            !(f.daily.memories & later::StoryStarted) && later::schedule(registry,f.id,later::FollowUp)) {
            f.daily.memories |= later::StoryStarted;
            dirty = true;
        }
        prepareRoutine(f, player);
        if (!companionOwns(f.id) && !hasAppointment(f.id) && !f.social.feud &&
            (f.social.robbed || f.community.opinion<=-15 || (f.social.trust < -10 && f.social.resentment >= 25)) &&
            (!avoidAfter.count(f.id) || due(GetTickCount(), avoidAfter[f.id]))) {
            people::Handle actor{};
            if (bindings.locate(f.id, actor) && exists(pool, actor)) {
                auto ped = pedAt(pool, actor.slot);
                if (eligible(ped, player) && onFoot(ped) && !busy(ped) && !primaryOccupied(ped) &&
                    distance2(position(ped), position(player)) < 64 && startFlee(ped, player))
                {
                    avoidAfter[f.id] = GetTickCount() + 300000;
                    if (due(GetTickCount(), nextSpeech) && due(GetTickCount(), subtitleUntil)) {
                        say(f, "RECOG_AVOID"); nextSpeech = GetTickCount() + 5000;
                    }
                }
            }
        }
        if (!socialworld::available(f) || !f.daily.ready || hasAppointment(f.id) ||
            companionOwns(f.id) || lifeEvent.person == f.id || roadside.person == f.id ||
            smallEvent.person == f.id || focused == f.id || recognitionOwns(f.id) || communityOwns(f.id)) {
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
