// Street encounters own only their wait/boarding tasks and map marker.
struct LifeEvent {
    unsigned person = 0, stage = 0;
    encounters::Kind kind = encounters::Kind::Beggar;
    people::Handle actor{}, car{}, home{}, exit{};
    void *task = nullptr;
    int blip = -1;
    Vec target{};
    DWORD started = 0, fadeAt = 0;
    unsigned fare = 0;
    bool homeAccess = false, exitAccess = false, faded = false, partner = false, night = false;
    life::NightRest nightRest;
    life::Ride ride;
} lifeEvent;
std::vector<people::Handle> leasedExits;
DWORD nextNightOffer = 0;
DWORD nextNightProbe = 0;
std::unordered_map<unsigned, DWORD> nightOfferAfter;
bool nightOffer(unsigned id) {
    return lifeEvent.person == id && lifeEvent.night && lifeEvent.stage == 0;
}
bool lifeActive() {
    return lifeEvent.person != 0;
}
Byte *drivenCar(Byte *player) {
    auto car = reinterpret_cast<Byte *(__cdecl *)(int, bool)>(0x56E0D0)(0, false);
    return car && *reinterpret_cast<Byte **>(car + 0x460) == player &&
                   at<int>(reinterpret_cast<std::uintptr_t>(car) + 0x590) == 0
               ? car
               : nullptr;
}
bool stoppedCar(Byte *car) {
    auto speed = *reinterpret_cast<Vec *>(car + 0x44);
    return speed.x * speed.x + speed.y * speed.y + speed.z * speed.z < 0.0009f;
}
int freeSeat(Byte *car) {
    unsigned count = std::min(8u, static_cast<unsigned>(car[0x488]));
    for (unsigned i = 0; i < count; ++i)
        if (!*reinterpret_cast<Byte **>(car + 0x464 + i * 4))
            return static_cast<int>(i);
    return -1;
}
Pool *homePool() {
    return at<Pool *>(0x96A7D8);
}
bool validHome(people::Handle h) {
    auto pool = homePool();
    return validPool(pool) && h.slot < static_cast<unsigned>(pool->size) &&
           !(pool->flags[h.slot] & 0x80) && pool->flags[h.slot] == h.generation;
}
Byte *homeAt(people::Handle h) {
    return homePool()->objects + h.slot * 0x3C;
}
void releaseLeasedExits(Byte *player) {
    if (player[0x2F])
        return;
    for (auto door : leasedExits)
        if (validHome(door))
            *reinterpret_cast<unsigned short *>(homeAt(door) + 0x30) &=
                static_cast<unsigned short>(~0x4000u);
    leasedExits.clear();
}
bool findHome(Vec origin, people::Handle &chosen, Vec &door) {
    auto pool = homePool();
    if (!validPool(pool))
        return false;
    float best = 350 * 350;
    for (unsigned i = 0; i < static_cast<unsigned>(pool->size); ++i) {
        if (pool->flags[i] & 0x80)
            continue;
        auto e = pool->objects + i * 0x3C;
        unsigned flags = *reinterpret_cast<unsigned short *>(e + 0x30);
        if (!(flags & 0x1000) && std::memcmp(e, "BURHOUS", 7) != 0)
            continue;
        auto linked = *reinterpret_cast<Byte **>(e + 0x38);
        unsigned area = linked ? linked[0x32] : e[0x32];
        if (!area || area > 18)
            continue;
        Vec p{(*reinterpret_cast<float *>(e + 8) + *reinterpret_cast<float *>(e + 0x10)) * 0.5f,
              (*reinterpret_cast<float *>(e + 0xC) + *reinterpret_cast<float *>(e + 0x14)) * 0.5f,
              *reinterpret_cast<float *>(e + 0x18)};
        float d = distance2(origin, p);
        if (d < best && std::abs(p.z - origin.z) < 8) {
            best = d;
            chosen = {i, pool->flags[i], 0};
            door = p;
        }
    }
    return best < 350 * 350;
}
bool randomNightDoor(Vec origin, people::Handle &chosen, Vec &door) {
    auto pool = homePool();
    if (!validPool(pool))
        return false;
    std::vector<std::pair<people::Handle, Vec>> candidates;
    for (unsigned i = 0; i < static_cast<unsigned>(pool->size); ++i) {
        if (pool->flags[i] & 0x80)
            continue;
        auto entry = pool->objects + i * 0x3C;
        unsigned flags = *reinterpret_cast<unsigned short *>(entry + 0x30);
        if (!(flags & 0x1000) && std::memcmp(entry, "BURHOUS", 7) != 0)
            continue;
        auto linked = *reinterpret_cast<Byte **>(entry + 0x38);
        unsigned area = linked ? linked[0x32] : entry[0x32];
        if (!area || area > 18)
            continue;
        Vec p{(*reinterpret_cast<float *>(entry + 8) + *reinterpret_cast<float *>(entry + 0x10)) *
                  .5f,
              (*reinterpret_cast<float *>(entry + 0xC) + *reinterpret_cast<float *>(entry + 0x14)) *
                  .5f,
              *reinterpret_cast<float *>(entry + 0x18)};
        if (distance2(origin, p) > 1000000)
            continue;
        candidates.push_back({{i, pool->flags[i], 0}, p});
    }
    if (candidates.empty())
        return false;
    auto pick = people::mix(GetTickCount()) % candidates.size();
    chosen = candidates[pick].first;
    door = candidates[pick].second;
    return true;
}
void releaseLifeTask() {
    auto pool = at<Pool *>(0xB74490);
    if (validPool(pool) && exists(pool, lifeEvent.actor)) {
        auto manager = taskManager(pedAt(pool, lifeEvent.actor.slot));
        if (manager && lifeEvent.task &&
            at<void *>(reinterpret_cast<std::uintptr_t>(manager) + 12) == lifeEvent.task)
            setTask(manager, nullptr, 3);
    }
    lifeEvent.task = nullptr;
}
void fadeVisit(bool out) {
    reinterpret_cast<void(__thiscall *)(void *, float, short)>(0x50AC20)(
        reinterpret_cast<void *>(0xB6F028), 0.7f, out ? 0 : 1);
}
void cancelLife() {
    if (!lifeActive())
        return;
    releaseLifeTask();
    auto peds = at<Pool *>(0xB74490);
    if (lifeEvent.kind == encounters::Kind::Ride && lifeEvent.stage && validPool(peds) &&
        exists(peds, lifeEvent.actor)) {
        auto ped = pedAt(peds, lifeEvent.actor.slot);
        auto car = *reinterpret_cast<Byte **>(ped + 0x58C);
        if (!onFoot(ped) && car && ped[0x484] == 1 && !recruited(ped) && !busy(ped) &&
            !primaryOccupied(ped)) {
            auto task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x34);
            if (task) {
                reinterpret_cast<void(__thiscall *)(void *, Byte *, int, int, bool, bool)>(
                    0x63B8C0)(task, car, 0, 0, true, false);
                ownResponse(ped, task, 10000);
            }
        }
    }
    if (lifeEvent.blip != -1)
        reinterpret_cast<void(__cdecl *)(int)>(0x587CE0)(lifeEvent.blip);
    if (validHome(lifeEvent.home) && lifeEvent.homeAccess) {
        auto &flags = *reinterpret_cast<unsigned short *>(homeAt(lifeEvent.home) + 0x30);
        flags = static_cast<unsigned short>(flags & ~0x4000u);
    }
    if (lifeEvent.exitAccess && validHome(lifeEvent.exit))
        leasedExits.push_back(lifeEvent.exit);
    if (lifeEvent.faded)
        fadeVisit(false);
    lifeEvent = {};
}
void lifeMarker(Vec target, int sprite) {
    if (lifeEvent.blip != -1)
        reinterpret_cast<void(__cdecl *)(int)>(0x587CE0)(lifeEvent.blip);
    lifeEvent.blip = reinterpret_cast<int(__cdecl *)(int, Vec, unsigned, int, char *)>(0x583820)(
        4, target, 0, lifeEvent.partner ? 2 : 3, nullptr);
    if (lifeEvent.blip != -1) {
        reinterpret_cast<void(__cdecl *)(int, int)>(0x583D70)(lifeEvent.blip, sprite);
        reinterpret_cast<void(__cdecl *)(int, unsigned char)>(0x583DB0)(lifeEvent.blip, 1);
    }
}
std::string lifeAction(unsigned id) {
    if (lifeEvent.person != id)
        return {};
    switch (lifeEvent.kind) {
    case encounters::Kind::Beggar:
        return "LIFE_GIVE_BEGGAR";
    case encounters::Kind::Addict:
        return "LIFE_GIVE_ADDICT";
    case encounters::Kind::Ride:
        return "LIFE_ACCEPT_RIDE";
    case encounters::Kind::Visit:
        return lifeEvent.night     ? "NIGHT_ACCEPT"
               : lifeEvent.partner ? "ROMANCE_HOME_ACCEPT"
                                   : "LIFE_ACCEPT_VISIT";
    default:
        return {};
    }
}
bool offerLife(Pool *pool, Byte *player, encounters::Kind kind, unsigned preferred) {
    auto car = drivenCar(player);
    if (kind == encounters::Kind::Ride && (!car || freeSeat(car) < 0))
        return false;
    if (kind != encounters::Kind::Ride && !onFoot(player))
        return false;
    if (kind == encounters::Kind::Visit && !life::night(at<Byte>(0xB70153)))
        return false;
    if (kind == encounters::Kind::Visit && !due(GetTickCount(), nextNightOffer))
        return false;
    unsigned chosen = 0;
    people::Handle actor{};
    float best = 625;
    for (unsigned slot = 0; slot < static_cast<unsigned>(pool->size); ++slot) {
        if (pool->flags[slot] & 0x80)
            continue;
        auto ped = pedAt(pool, slot);
        auto h = handle(pool, slot);
        unsigned id = bindings.find(h);
        if (preferred && id != preferred)
            continue;
        if (!id || !eligible(ped, player) || !onFoot(ped) || ped[0x2F] != 0 || busy(ped) ||
            primaryOccupied(ped) || armed(ped))
            continue;
        auto &f = registry.families[id - 1];
        if (!socialprogress::availableForEvent(f.daily.busyUntil, registry.worldMinutes,
                hasAppointment(id), companionOwns(id), f.social.romance) ||
            f.social.events & encounters::bit(kind) || f.social.robbed || f.social.anger >= 30 ||
            f.social.cutOff)
            continue;
        int type = *reinterpret_cast<int *>(ped + 0x598);
        if (kind == encounters::Kind::Visit &&
            (!life::nightCandidate(f, type, at<Byte>(0xB70153), onFoot(player),
                                   distance2(position(player), position(ped))) ||
             hasAppointment(id) || companionOwns(id) ||
             (nightOfferAfter.count(id) && !due(GetTickCount(), nightOfferAfter[id]))))
            continue;
        if (kind == encounters::Kind::Beggar && type != 21)
            continue;
        if (kind == encounters::Kind::Addict && (type != 4 && type != 5 && type != 21))
            continue;
        if (kind == encounters::Kind::Addict && people::mix(id) % 4 != 0)
            continue;
        if (kind == encounters::Kind::Ride && (type != 4 && type != 5 && type != 21))
            continue;
        float d = distance2(position(player), position(ped));
        if (d <= best && (kind == encounters::Kind::Visit || (d >= 9 && visibleTarget(ped)))) {
            chosen = id;
            actor = h;
            best = d;
        }
    }
    if (!chosen)
        return false;
    Vec target{};
    people::Handle home{};
    if (kind == encounters::Kind::Visit && !randomNightDoor(position(player), home, target))
        return false;
    if (kind == encounters::Kind::Ride) {
        bool found = false;
        for (const auto &f : registry.families) {
            auto &p = f.presence;
            if (!p.alive || !p.located || p.area)
                continue;
            Vec point{p.x, p.y, p.z};
            float d = distance2(position(player), point);
            if (d > 40000 && d < 1440000 && std::abs(point.z - position(player).z) < 35) {
                target = point;
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }
    auto ped = pedAt(pool, actor.slot);
    auto task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x20);
    if (!task)
        return false;
    reinterpret_cast<void(__thiscall *)(void *, int, bool, bool, float)>(0x62F310)(
        task, 150000, false, false, 4.0f);
    setTask(taskManager(ped), task, 3);
    lifeEvent = {};
    lifeEvent.person = chosen;
    lifeEvent.actor = actor;
    lifeEvent.kind = kind;
    lifeEvent.task = task;
    lifeEvent.started = GetTickCount();
    lifeEvent.target = target;
    lifeEvent.home = home;
    lifeEvent.night = kind == encounters::Kind::Visit;
    if (lifeEvent.night) {
        if (nightOfferAfter.size() >= 256)
            nightOfferAfter.erase(nightOfferAfter.begin());
        nightOfferAfter[chosen] = GetTickCount() + 600000;
        nextNightOffer = GetTickCount() + 480000; // At most one night offer every 8 minutes.
    }
    lifeEvent.fare =
        life::rideFare(chosen + GetTickCount(), std::sqrt(distance2(position(player), target)));
    say(registry.families[chosen - 1], kind == encounters::Kind::Beggar   ? "LIFE_ASK_BEGGAR"
                                       : kind == encounters::Kind::Addict ? "LIFE_ASK_ADDICT"
                                       : kind == encounters::Kind::Ride   ? "LIFE_ASK_RIDE"
                                                                          : "NIGHT_CALL");
    help(lifeEvent.night ? "NIGHT_HINT" : "LIFE_NEARBY");
    return true;
}
// Wall-clock encounter deadlines pause together with the game/menu/background state.
void pauseEvents(DWORD now) {
    DWORD elapsed = lastDirectorTick ? now - lastDirectorTick : 0;
    if (roadside.person) {
        roadside.started += elapsed;
        roadside.last = now;
    }
    if (smallEvent.person)
        smallEvent.started += elapsed;
    if (lifeActive()) {
        lifeEvent.started += elapsed;
        if (lifeEvent.faded)
            lifeEvent.fadeAt += elapsed;
    }
    lastDirectorTick = now;
}
bool offerPartnerHome(Pool *pool, Byte *player, people::Family &f, people::Handle actor) {
    if (lifeActive() || !romance::homeReady(f) || !onFoot(player) || player[0x2F] ||
        !exists(pool, actor))
        return false;
    people::Handle home{};
    Vec target{};
    if (!findHome(position(player), home, target))
        return false;
    auto ped = pedAt(pool, actor.slot);
    if (busy(ped) || primaryOccupied(ped))
        return false;
    auto task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x20);
    if (!task)
        return false;
    reinterpret_cast<void(__thiscall *)(void *, int, bool, bool, float)>(0x62F310)(
        task, 150000, false, false, 4.0f);
    setTask(taskManager(ped), task, 3);
    lifeEvent = {};
    lifeEvent.person = f.id;
    lifeEvent.actor = actor;
    lifeEvent.home = home;
    lifeEvent.target = target;
    lifeEvent.task = task;
    lifeEvent.partner = true;
    lifeEvent.kind = encounters::Kind::Visit;
    lifeEvent.started = GetTickCount();
    say(f, "ROMANCE_HOME_OFFER");
    return true;
}
bool converseLife(Byte *player) {
    if (!lifeActive() || focused != lifeEvent.person || lifeEvent.stage)
        return false;
    auto pool = at<Pool *>(0xB74490);
    if (!validPool(pool) || !exists(pool, lifeEvent.actor)) {
        cancelLife();
        return false;
    }
    auto ped = pedAt(pool, lifeEvent.actor.slot);
    auto &f = registry.families[lifeEvent.person - 1];
    nextSpeech = GetTickCount() + 2800;
    auto kind = lifeEvent.kind;
    if (lifeEvent.night) {
        auto money = playerMoney(player);
        if (!onFoot(player) || player[0x2F] || !money || *money < 80) {
            help("NIGHT_NO_CASH");
            return true;
        }
        releaseLifeTask();
        lifeEvent.stage = 4;
        lifeEvent.fadeAt = GetTickCount();
        lifeEvent.faded = true;
        closePhone();
        fadeVisit(true);
        return true;
    }
    if (kind == encounters::Kind::Beggar || kind == encounters::Kind::Addict) {
        auto money = playerMoney(player);
        int wallet = std::max(0, static_cast<int>(*reinterpret_cast<short *>(ped + 0x756)));
        unsigned amount = kind == encounters::Kind::Beggar ? 10u : 20u;
        if (!money || !encounters::donate(*money, wallet, amount)) {
            say(f, "LIFE_NO_CASH");
            cancelLife();
            return true;
        }
        *reinterpret_cast<short *>(ped + 0x756) = static_cast<short>(wallet);
        const bool alreadyKnown = dialogue::knows(f, people::Name);
        bool first = life::complete(f, kind);
        say(f, locale::tr(kind == encounters::Kind::Beggar ? "LIFE_THANK_BEGGAR"
                                                           : "LIFE_THANK_ADDICT") +
                   (alreadyKnown ? "" : " " + locale::format("LIFE_MY_NAME", people::generate(f)[0].name)));
        if (first)
            reputation(kind == encounters::Kind::Beggar ? 1 : -1);
        if (first && kind == encounters::Kind::Beggar)
            later::schedule(registry, f.id, later::Thanks);
        justMet(f);
        discoverNetwork(f.id);
        if(kind!=encounters::Kind::Addict) observeDeed(f.id,community::Deed::Help);
        dirty = true;
        panelInvalid = true;
        cancelLife();
        save();
        return true;
    }
    if (kind == encounters::Kind::Ride) {
        auto car = drivenCar(player);
        auto cars = at<Pool *>(0xB74494);
        if (!car || !validPool(cars) || !stoppedCar(car) || freeSeat(car) < 0) {
            help("LIFE_NEED_CAR");
            return true;
        }
        releaseLifeTask();
        const int seat=reservePassenger(car,lifeEvent.actor);
        const int door=socialcontext::passengerDoor(seat);
        if(door<0){help("ACT_DRIVE_DOOR_BLOCKED");return true;}
        auto task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x50);
        if (!task) {
            cancelLife();
            return true;
        }
        reinterpret_cast<void(__thiscall *)(void *, Byte *, int, bool)>(0x640340)(task,car,door,false);
        setTask(taskManager(ped), task, 3);
        lifeEvent.task = task;
        auto slot = static_cast<unsigned>((car - cars->objects) / 0xA18);
        lifeEvent.car = carHandle(cars, slot);
        lifeEvent.stage = 1;
        lifeEvent.ride.stage = 1;
        lifeMarker(lifeEvent.target, 53);
        audio.speak(audioChoices.choose(socialaudio::Context::Ride, GetTickCount()));
        say(f, "LIFE_BOARDING");
        return true;
    }
    auto money = playerMoney(player);
    if ((!lifeEvent.partner && (!money || *money < 80)) || !validHome(lifeEvent.home)) {
        help("LIFE_VISIT_UNAVAILABLE");
        return true;
    }
    auto &flags = *reinterpret_cast<unsigned short *>(homeAt(lifeEvent.home) + 0x30);
    lifeEvent.homeAccess = !(flags & 0x4000);
    flags |= 0x4000;
    auto linked = *reinterpret_cast<Byte **>(homeAt(lifeEvent.home) + 0x38);
    auto entries = homePool();
    auto offset = linked ? linked - entries->objects : -1;
    if (offset >= 0 && offset % 0x3C == 0 && offset / 0x3C < entries->size) {
        unsigned slot = static_cast<unsigned>(offset / 0x3C);
        lifeEvent.exit = {slot, entries->flags[slot], 0};
        if (validHome(lifeEvent.exit)) {
            auto &exitFlags = *reinterpret_cast<unsigned short *>(linked + 0x30);
            lifeEvent.exitAccess = !(exitFlags & 0x4000);
            exitFlags |= 0x4000;
        }
    }
    releaseLifeTask();
    lifeEvent.stage = 1;
    lifeMarker(lifeEvent.target, 31);
    audio.speak(audioChoices.choose(socialaudio::Context::Flirt, GetTickCount()));
    say(f, "LIFE_WALK_HOME");
    help("LIFE_HOME_MARKER");
    return true;
}
void updateLife(Pool *pool, Byte *player, unsigned elapsed) {
    if (!lifeActive())
        return;
    auto &f = registry.families[lifeEvent.person - 1];
    if (lifeEvent.night && lifeEvent.stage >= 4) {
        if (lifeEvent.stage == 5) {
            if (GetTickCount() - lifeEvent.fadeAt > 700)
                cancelLife();
            return;
        }
        if (GetTickCount() - lifeEvent.fadeAt < 900)
            return;
        auto money = playerMoney(player);
        if (!onFoot(player) || player[0x2F] || !money || *money < 80 || !f.presence.alive) {
            cancelLife();
            return;
        }
        // Load the chosen exterior while the screen is black, then validate a pedestrian landing.
        Vec target = lifeEvent.target, landing{}, previousPosition = position(player);
        reinterpret_cast<void(__cdecl *)(const Vec *)>(0x40ED80)(&target);
        reinterpret_cast<void(__cdecl *)(const Vec *)>(0x40EB70)(&target);
        if (!streetPoint(target, player, landing, 12) || !clearGround(landing, player)) {
            // The far door did not stream in: CJ wakes up on the street where the night began.
            reinterpret_cast<void(__cdecl *)(const Vec *)>(0x40ED80)(&previousPosition);
            reinterpret_cast<void(__cdecl *)(const Vec *)>(0x40EB70)(&previousPosition);
            if (!streetPoint(previousPosition, player, landing, 12) || !clearGround(landing, player))
                landing = previousPosition;
        }
        if (!lifeEvent.nightRest.pay(*money)) {
            cancelLife();
            return;
        }
        unsigned hour = at<Byte>(0xB70153), minute = at<Byte>(0xB70152);
        unsigned skipped = life::untilMorning(hour, minute);
        if (hour >= 8)
            reinterpret_cast<void(__cdecl *)(unsigned)>(0x52D0B0)(1);
        reinterpret_cast<void(__cdecl *)(Byte, Byte, Byte)>(0x52D150)(8, 0, 0);
        auto table = *reinterpret_cast<std::uintptr_t **>(player);
        reinterpret_cast<void(__thiscall *)(Byte *, Vec, bool)>(table[14])(player, landing, false);
        reinterpret_cast<void(__thiscall *)(void *)>(0x50BAB0)(reinterpret_cast<void *>(0xB6F028));
        for (auto &parent : registry.families)
            romance::advance(parent, skipped);
        familyClock = 8 * 60;
        life::complete(f, encounters::Kind::Visit);
        f.social.known |= people::Name;
        f.social.journal = true;
        later::schedule(registry, f.id, later::Visit);
        auto &health = *reinterpret_cast<float *>(player + 0x540);
        health = std::max(health, std::min(100.0f, *reinterpret_cast<float *>(player + 0x544)));
        lifeEvent.stage = 5;
        lifeEvent.fadeAt = GetTickCount();
        dirty = true;
        panelInvalid = true;
        save();
        help("NIGHT_MORNING");
        log("NIGHT completed identity=" + std::to_string(f.id));
        return;
    }
    if (lifeEvent.night && (!onFoot(player) || player[0x2F])) {
        cancelLife();
        return;
    }
    if (GetTickCount() - lifeEvent.started > 600000 || !f.presence.alive) {
        cancelLife();
        return;
    }
    if (lifeEvent.partner && (f.social.romance < 3 || f.social.anger >= 30 || f.social.cutOff)) {
        cancelLife();
        return;
    }
    if (lifeEvent.kind == encounters::Kind::Visit && lifeEvent.stage) {
        bool atHome = false;
        if (validHome(lifeEvent.home)) {
            auto entry = homeAt(lifeEvent.home);
            auto linked = *reinterpret_cast<Byte **>(entry + 0x38);
            auto inside = linked ? linked : entry;
            atHome = player[0x2F] == inside[0x32] && player[0x2F] != 0 &&
                     distance2(position(player), *reinterpret_cast<Vec *>(inside + 0x20)) < 900;
        }
        if (atHome && lifeEvent.stage == 1 && at<int>(0x96A7CC) == 0) {
            auto money = playerMoney(player);
            if (!lifeEvent.partner && (!money || *money < 80)) {
                cancelLife();
                return;
            }
            lifeEvent.stage = 2;
            lifeEvent.fadeAt = GetTickCount();
            lifeEvent.faded = true;
            fadeVisit(true);
        } else if (lifeEvent.stage == 2 && GetTickCount() - lifeEvent.fadeAt > 900) {
            if (!atHome) {
                cancelLife();
                return;
            }
            auto money = playerMoney(player);
            if (lifeEvent.partner || (money && *money >= 80)) {
                if (!lifeEvent.partner)
                    *money -= 80;
                unsigned hours = at<Byte>(0xB70153) + 6u;
                if (hours >= 24)
                    reinterpret_cast<void(__cdecl *)(unsigned)>(0x52D0B0)(1);
                reinterpret_cast<void(__cdecl *)(Byte, Byte, Byte)>(0x52D150)(
                    static_cast<Byte>(hours % 24), at<Byte>(0xB70152), 0);
                if (lifeEvent.partner)
                    romance::homeCompleted(f);
                else
                    life::complete(f, encounters::Kind::Visit);
                for (auto &parent : registry.families)
                    romance::advance(parent, 360);
                // Rest like a safehouse save: top up CJ's health after the time skip.
                auto &health = *reinterpret_cast<float *>(player + 0x540);
                health =
                    std::max(health, std::min(100.0f, *reinterpret_cast<float *>(player + 0x544)));
                dirty = true;
                panelInvalid = true;
                save();
                help("LIFE_SIX_HOURS");
                say(f, lifeEvent.partner || f.social.journal || f.daily.outings
                    ? locale::tr("LIFE_VISIT_KNOWN")
                    : locale::format("LIFE_MY_NAME", people::generate(f)[0].name));
            }
            fadeVisit(false);
            lifeEvent.faded = false;
            lifeEvent.stage = 3;
        } else if (lifeEvent.stage == 3 && player[0x2F] == 0)
            cancelLife();
        if (lifeEvent.stage == 1 && player[0x2F] != 0 && !atHome) {
            cancelLife();
            return;
        }
        if (exists(pool, lifeEvent.actor) && lifeEvent.stage == 1) {
            auto ped = pedAt(pool, lifeEvent.actor.slot);
            if (!eligible(ped, player) || *reinterpret_cast<float *>(ped + 0x540) <= 0) {
                cancelLife();
                return;
            }
            if (!busy(ped) && !primaryOccupied(ped) &&
                distance2(position(ped), lifeEvent.target) > 9) {
                auto task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x60);
                if (task) {
                    reinterpret_cast<void(__thiscall *)(void *, int, const Vec &, float, float,
                                                        float, bool, int, bool)>(0x66EA30)(
                        task, 4, lifeEvent.target, 2, 3, 2, false, -1, true);
                    setTask(taskManager(ped), task, 3);
                    lifeEvent.task = task;
                }
            }
        }
        return;
    }
    if (!exists(pool, lifeEvent.actor)) {
        cancelLife();
        return;
    }
    auto ped = pedAt(pool, lifeEvent.actor.slot);
    if (!eligible(ped, player) || *reinterpret_cast<float *>(ped + 0x540) <= 0 ||
        f.social.anger >= 40) {
        cancelLife();
        return;
    }
    if (lifeEvent.kind != encounters::Kind::Ride || !lifeEvent.stage) {
        if (distance2(position(player), position(ped)) > 10000 ||
            GetTickCount() - lifeEvent.started > 150000)
            cancelLife();
        return;
    }
    auto cars = at<Pool *>(0xB74494);
    if (!carExists(cars, lifeEvent.car)) {
        cancelLife();
        return;
    }
    auto car = carAt(cars, lifeEvent.car.slot);
    if (drivenCar(player) != car) {
        help("LIFE_RIDE_CANCEL");
        cancelLife();
        return;
    }
    bool aboard = !onFoot(ped) && *reinterpret_cast<Byte **>(ped + 0x58C) == car;
    bool atDestination = distance2(position(car), lifeEvent.target) < 225;
    bool paid =
        life::rideStep(lifeEvent.ride, elapsed, true, aboard, atDestination, stoppedCar(car));
    if (lifeEvent.ride.stage == 3 && lifeEvent.stage != 3) {
        releaseLifeTask();
        auto task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x34);
        if (!task) {
            cancelLife();
            return;
        }
        reinterpret_cast<void(__thiscall *)(void *, Byte *, int, int, bool, bool)>(0x63B8C0)(
            task, car, 0, 0, true, false);
        setTask(taskManager(ped), task, 3);
        lifeEvent.task = task;
        lifeEvent.stage = 3;
    }
    if (paid) {
        auto money = playerMoney(player);
        const bool alreadyKnown = dialogue::knows(f, people::Name);
        if (money && *money >= 0 && *money < 999999999 &&
            life::complete(f, encounters::Kind::Ride)) {
            *money += static_cast<int>(
                std::min(lifeEvent.fare, static_cast<unsigned>(999999999 - *money)));
            reputation(1);
            discoverNetwork(f.id);
            observeDeed(f.id,community::Deed::Help);
            later::helped(registry, f.id);
            justMet(f);
            help(locale::format("LIFE_RIDE_PAID", std::to_string(lifeEvent.fare)));
            say(f, alreadyKnown ? locale::tr("LIFE_RIDE_THANK_KNOWN")
                : locale::format("LIFE_RIDE_THANK", people::generate(f)[0].name));
            dirty = true;
            panelInvalid = true;
            save();
        }
        cancelLife();
    } else if (lifeEvent.ride.stage == 4) {
        help("LIFE_RIDE_CANCEL");
        cancelLife();
    }
}
void updateEventBlip(Pool *pool) {
    people::Handle owner{};
    bool wanted = false;
    if (roadside.person) {
        owner = roadside.owner;
        wanted = true;
    } else if (smallEvent.person) {
        owner = smallEvent.owner;
        wanted = true;
    } else if (lifeActive() && !lifeEvent.stage) {
        owner = lifeEvent.actor;
        wanted = true;
    }
    if (!wanted || !validPool(pool) || !exists(pool, owner)) {
        clearEventBlip();
        return;
    }
    if (eventBlip != -1 && eventBlipOwner == owner)
        return;
    clearEventBlip();
    const int reference = static_cast<int>((owner.slot << 8) | (pool->flags[owner.slot] & 0x7F));
    eventBlip =
        reinterpret_cast<int(__cdecl *)(int, int, unsigned, int)>(0x5839A0)(2, reference, 0, 3);
    if (eventBlip != -1) {
        reinterpret_cast<void(__cdecl *)(int, unsigned)>(0x583AB0)(eventBlip, 4);
        reinterpret_cast<void(__cdecl *)(int, unsigned char)>(0x583DB0)(eventBlip, 1);
        eventBlipOwner = owner;
    }
}
// Blood feud: relatives of someone CJ killed attack on sight.
std::unordered_map<unsigned, DWORD> nextFeud;
void updateFeud(Pool *pool, Byte *player) {
    DWORD now = GetTickCount();
    for (unsigned slot = 0; slot < static_cast<unsigned>(pool->size); ++slot) {
        if (pool->flags[slot] & 0x80)
            continue;
        auto ped = pedAt(pool, slot);
        unsigned id = bindings.find(handle(pool, slot));
        if (!id || !registry.families[id - 1].social.feud || !eligible(ped, player) ||
            !onFoot(ped) || ped[0x2F] != player[0x2F] ||
            *reinterpret_cast<float *>(ped + 0x540) <= 0 ||
            distance2(position(player), position(ped)) > 2500)
            continue;
        auto &due_at = nextFeud[id];
        if (due_at && !due(now, due_at))
            continue;
        due_at = now + 12000;
        if (busy(ped) || primaryOccupied(ped))
            continue;
        if (startFight(ped, player, true)) {
            log("SOCIAL feud attack identity=" + std::to_string(id));
            say(registry.families[id - 1], "FEUD_LINE");
            audio.speak(audioChoices.choose(socialaudio::Context::Fight, now + id));
        }
    }
}
