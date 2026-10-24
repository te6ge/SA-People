// Included inside the adapter namespace. Walking interactions never touch CPad controls.
struct LiveTalk {
    street::Encounter encounter;
    DWORD last = 0, next = 0;
    people::Handle lastHandle{};
    bool left = false;
};
std::unordered_map<unsigned, LiveTalk> liveTalks;
void justMet(people::Family &f, people::Handle h = {});
struct Gesture {
    people::Handle handle;
    void *task;
    DWORD until;
};
std::vector<Gesture> gestures;
unsigned focused = 0;
people::Handle focusedHandle{};
DWORD nextFocus = 0, nextSpeech = 0, lastDirectorTick = 0, lastManualEvent = 0;
bool focusHeld = false, heldGreet = false, heldProvoke = false, heldDefuse = false, heldRob = false,
     heldEvent = false, batteryEvents = true, sideEvents = true;
encounters::Director eventDirector;
std::string speaker, spoken;
DWORD subtitleUntil = 0;
struct Roadside {
    unsigned person = 0;
    people::Handle owner{}, vehicle{};
    void *waitTask = nullptr;
    DWORD started = 0, last = 0;
    Byte originalEngine = 0, originalBroken = 0;
    street::Repair repair;
    DWORD troubled = 0;
} roadside;
std::unordered_set<unsigned> usedParkedCars;
bool visibleTarget(Byte *ped);
struct OwnedResponse {
    people::Handle handle;
    void *task;
    DWORD until;
    bool combat = false;
};
std::vector<OwnedResponse> responses;
bool onFoot(Byte *ped) {
    return !(ped[0x46D] & 1);
}
Byte *taskManager(Byte *ped) {
    auto intelligence = *reinterpret_cast<Byte **>(ped + 0x47C);
    return intelligence ? intelligence + 4 : nullptr;
}
void setTask(Byte *manager, void *task, int slot) {
    reinterpret_cast<void(__thiscall *)(Byte *, void *, int, bool)>(0x681AF0)(manager, task, slot,
                                                                              false);
}
void setSecondary(Byte *manager, void *task, int slot) {
    reinterpret_cast<void(__thiscall *)(Byte *, void *, int)>(0x681B60)(manager, task, slot);
}
bool validPool(Pool *p) {
    return p && p->objects && p->flags && p->size > 0 && p->size < 10000;
}
Byte *carAt(Pool *p, unsigned slot) {
    return p->objects + slot * 0xA18;
}
people::Handle carHandle(Pool *p, unsigned slot) {
    return {slot, p->flags[slot], *reinterpret_cast<short *>(carAt(p, slot) + 0x22)};
}
bool carExists(Pool *p, people::Handle h) {
    return validPool(p) && h.slot < static_cast<unsigned>(p->size) && !(p->flags[h.slot] & 0x80) &&
           carHandle(p, h.slot) == h;
}
bool nativeConversation(Byte *ped) {
    // CPedToPlayerConversations state and participant. m_pPed can outlive an ended talk.
    return ped && at<unsigned>(0x969A20) != 0 &&
           (ped == at<Byte *>(0x9691C0) || *reinterpret_cast<int *>(ped + 0x598) < 4);
}
// Native ped-to-player chats (dealer offers with the Y/N prompt) give way to the mod's own
// conversation for peds that have an identity. Use the native end routine so the audio
// conversation reservation, speech and look-at state are released as well.
bool modTalkOverNative = true;
nativetalk::ScriptGate dealerScriptGate;
void updateDealerTalks(Pool *pool, Byte *player, bool enabled) {
    bool interrupted = false;
    // CConversationForPed[14], size 0x1C. Separate from CPedToPlayerConversations.
    for (unsigned slot = 0; slot < 14; ++slot) {
        const auto address = 0x9691D8u + slot * 0x1Cu;
        auto ped = at<Byte *>(address + 8);
        people::Handle actor{};
        bool valid = false, mute = false;
        if (ped && validPool(pool)) {
            const auto base = reinterpret_cast<std::uintptr_t>(pool->objects);
            const auto pointer = reinterpret_cast<std::uintptr_t>(ped);
            if (pointer >= base && (pointer - base) % 0x7C4 == 0) {
                const auto index = (pointer - base) / 0x7C4;
                if (index < static_cast<unsigned>(pool->size) && !(pool->flags[index] & 0x80)) {
                    actor = handle(pool, static_cast<unsigned>(index));
                    valid = true;
                    mute = nativetalk::replaceDealer(enabled, false, bindings.find(actor) != 0,
                        ped != player && (ped[0x484] == 1 || leasedActor(ped)), recruited(ped),
                        *reinterpret_cast<int *>(ped + 0x598));
                }
            }
        }
        if (dealerScriptGate.apply(slot, actor, valid, mute, at<Byte>(address + 0x18),
                                   at<unsigned>(address + 0x14))) {
            interrupted = true;
            log("NATIVE dealer scripted Y/N interrupted identity=" + std::to_string(bindings.find(actor)));
        }
    }
    if (interrupted)
        reinterpret_cast<void(__cdecl *)(const char *, bool, bool, bool)>(0x588BE0)(
            nullptr, true, false, false);
}
void suppressNativeTalk(Pool *pool, Byte *player) {
    updateDealerTalks(pool, player, modTalkOverNative);
    auto &state = at<unsigned>(0x969A20);
    auto &other = at<Byte *>(0x9691C0);
    // The native update only starts a new chat 30 s after its last start time (0x9691B4).
    // Keeping that time at "now" means the dealer's Y/N prompt never opens.
    if (modTalkOverNative && !state)
        at<unsigned>(0x9691B4) = at<unsigned>(0xB7CB84);
    if (!modTalkOverNative || !state)
        return;
    if (other) {
        const auto address = reinterpret_cast<std::uintptr_t>(other);
        const auto base = reinterpret_cast<std::uintptr_t>(pool->objects);
        if (address < base)
            return;
        const auto delta = address - base;
        if (delta % 0x7C4 || delta / 0x7C4 >= static_cast<unsigned>(pool->size) ||
            (pool->flags[delta / 0x7C4] & 0x80) || !eligible(other, player) ||
            !bindings.find(handle(pool, static_cast<unsigned>(delta / 0x7C4))))
            return;
    }
    // CPedToPlayerConversations::EndConversation (gta-reversed, 0x43AB10).
    reinterpret_cast<void(__cdecl *)()>(0x43AB10)();
    reinterpret_cast<void(__cdecl *)(const char *, bool, bool, bool)>(0x588BE0)(
        nullptr, true, false, false);
    at<unsigned>(0x9691B4) = at<unsigned>(0xB7CB84);
}
bool busy(Byte *ped) {
    if (recruited(ped) || nativeConversation(ped))
        return true;
    auto m = taskManager(ped);
    return !m || at<void *>(reinterpret_cast<std::uintptr_t>(m)) ||
           at<void *>(reinterpret_cast<std::uintptr_t>(m) + 4) ||
           at<void *>(reinterpret_cast<std::uintptr_t>(m) + 8);
}
bool primaryOccupied(Byte *ped) {
    auto m = taskManager(ped);
    return !m || at<void *>(reinterpret_cast<std::uintptr_t>(m) + 12);
}
bool fearTask(void *task) {
    if (!task)
        return true;
    auto table = *reinterpret_cast<std::uintptr_t **>(task);
    int id = reinterpret_cast<int(__thiscall *)(void *)>(table[4])(task); // CTask::GetId.
    return id == 412 || id == 413 || id == 601 ||
           (id >= 908 && id <= 911); // Cower, hands up, gun threat, flee.
}
bool robberyBusy(Byte *ped) {
    auto manager = taskManager(ped);
    if (!manager || at<void *>(reinterpret_cast<std::uintptr_t>(manager)))
        return true;
    for (unsigned slot = 1; slot <= 3; ++slot)
        if (!fearTask(at<void *>(reinterpret_cast<std::uintptr_t>(manager) + slot * 4)))
            return true;
    return false;
}
int currentWeapon(Byte *ped) {
    unsigned slot = ped[0x718];
    return slot < 13 ? *reinterpret_cast<int *>(ped + 0x5A0 + slot * 0x1C) : 0;
}
unsigned currentAmmo(Byte *ped) {
    unsigned slot = ped[0x718];
    return slot < 13 ? *reinterpret_cast<unsigned *>(ped + 0x5A0 + slot * 0x1C + 0xC) : 0;
}
bool armed(Byte *ped) {
    for (unsigned slot = 0; slot < 13; ++slot) {
        int kind = *reinterpret_cast<int *>(ped + 0x5A0 + slot * 0x1C);
        if (kind >= 2 && kind <= 39)
            return true;
    }
    return false;
}
bool robbable(Byte *ped) {
    if (recruited(ped))
        return false;
    int type = *reinterpret_cast<int *>(ped + 0x598);
    return type == 4 || type == 5 || (type >= 7 && type <= 17) || type == 20 || type == 21 ||
           type == 22;
}
void cleanupResponses(Pool *pool, bool all = false) {
    for (auto it = responses.begin(); it != responses.end();) {
        Byte *manager = validPool(pool) && exists(pool, it->handle)
                            ? taskManager(pedAt(pool, it->handle.slot))
                            : nullptr;
        bool owned =
            manager && at<void *>(reinterpret_cast<std::uintptr_t>(manager) + 12) == it->task;
        if (!owned || all || due(GetTickCount(), it->until) ||
            (manager && recruited(pedAt(pool, it->handle.slot)))) {
            if (owned)
                setTask(manager, nullptr, 3);
            it = responses.erase(it);
        } else
            ++it;
    }
}
bool ownResponse(Byte *ped, void *task, unsigned duration, bool combat = false) {
    auto pool = at<Pool *>(0xB74490);
    auto delta = ped - pool->objects;
    if (delta < 0 || delta % 0x7C4 || delta / 0x7C4 >= pool->size)
        return false;
    auto h = handle(pool, static_cast<unsigned>(delta / 0x7C4));
    setTask(taskManager(ped), task, 3);
    responses.push_back({h, task, GetTickCount() + duration, combat});
    return true;
}
bool calmOwnedFight(Pool *pool, people::Handle h) {
    for (auto it = responses.begin(); it != responses.end(); ++it)
        if (it->handle == h && it->combat) {
            auto m = taskManager(pedAt(pool, h.slot));
            if (m && at<void *>(reinterpret_cast<std::uintptr_t>(m) + 12) == it->task)
                setTask(m, nullptr, 3);
            responses.erase(it);
            return true;
        }
    return false;
}
bool canFight(Byte *ped) {
    if (recruited(ped))
        return false;
    int type = *reinterpret_cast<int *>(ped + 0x598);
    unsigned slot = ped[0x718];
    if ((type != 4 && type != 5 && type != 21 && type != 22) || slot >= 13 ||
        *reinterpret_cast<int *>(ped + 0x5A0 + slot * 0x1C) > 1)
        return false;
    for (unsigned weapon = 1; weapon < 13; ++weapon) {
        int kind = *reinterpret_cast<int *>(ped + 0x5A0 + weapon * 0x1C);
        if (kind >= 2 && kind <= 46)
            return false;
    }
    return true;
}
bool startFight(Byte *ped, Byte *player, bool allowArmed = false) {
    if ((allowArmed ? !robbable(ped) : !canFight(ped)) || busy(ped) || primaryOccupied(ped))
        return false;
    auto manager = taskManager(ped);
    if (!manager)
        return false;
    void *task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x38);
    if (!task)
        return false;
    reinterpret_cast<void(__thiscall *)(void *, Byte *, int, int, int, int, char)>(0x620E30)(
        task, player, 15000, 0, 0, 100, 0);
    return ownResponse(ped, task, 15000, true);
}
void cleanupGestures(Pool *pool, bool all = false) {
    for (auto it = gestures.begin(); it != gestures.end();) {
        bool valid = validPool(pool) && exists(pool, it->handle);
        Byte *manager = valid ? taskManager(pedAt(pool, it->handle.slot)) : nullptr;
        auto slot = manager ? reinterpret_cast<std::uintptr_t>(manager) + 0x14 + 4 * 4 : 0;
        bool owned = slot && at<void *>(slot) == it->task;
        if (owned) {
            auto assoc = *reinterpret_cast<Byte **>(static_cast<Byte *>(it->task) + 8);
            if (assoc) {
                auto &flags = *reinterpret_cast<unsigned short *>(assoc + 0x2E);
                flags = static_cast<unsigned short>((flags | 0x18) & ~2u);
            }
        }
        if (!owned || all || due(GetTickCount(), it->until)) {
            if (owned)
                setSecondary(manager, nullptr, 4);
            it = gestures.erase(it);
        } else
            ++it;
    }
}
void gesture(Pool *pool, people::Handle h, unsigned animation) {
    if (!exists(pool, h) || gestures.size() >= 8)
        return;
    Byte *ped = pedAt(pool, h.slot), *manager = taskManager(ped);
    if (!manager || !*reinterpret_cast<void **>(ped + 0x18))
        return;
    if (at<void *>(reinterpret_cast<std::uintptr_t>(manager) + 0x14 + 4 * 4))
        return;
    void *task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x20);
    if (!task)
        return;
    reinterpret_cast<void(__thiscall *)(void *, unsigned, unsigned, float, bool)>(0x61A8B0)(
        task, 0, animation, 4.0f, false);
    setSecondary(manager, task, 4);
    gestures.push_back({h, task, GetTickCount() + 1300});
}
void say(const people::Family &f, const std::string &line) {
    speaker = dialogue::displayName(f);
    spoken = line;
    subtitleUntil = GetTickCount() + 5500;
}
void cancelRoadside(bool repaired = false) {
    if (!roadside.person)
        return;
    auto cars = at<Pool *>(0xB74494);
    if (carExists(cars, roadside.vehicle)) {
        auto car = carAt(cars, roadside.vehicle.slot);
        car[0x42D] = static_cast<Byte>((car[0x42D] & ~2u) | roadside.originalBroken);
        if (!*reinterpret_cast<void **>(car + 0x460))
            car[0x428] =
                static_cast<Byte>((car[0x428] & ~16u) | (repaired ? 16u : roadside.originalEngine));
    }
    auto pool = at<Pool *>(0xB74490);
    if (validPool(pool) && exists(pool, roadside.owner)) {
        auto manager = taskManager(pedAt(pool, roadside.owner.slot));
        if (manager &&
            at<void *>(reinterpret_cast<std::uintptr_t>(manager) + 3 * 4) == roadside.waitTask)
            setTask(manager, nullptr, 3);
    }
    roadside = {};
}
Vec bonnet(Byte *car) {
    Vec v = position(car);
    auto matrix = *reinterpret_cast<Byte **>(car + 0x14);
    if (matrix) {
        auto forward = *reinterpret_cast<Vec *>(matrix + 0x10);
        v.x += forward.x * 2.2f;
        v.y += forward.y * 2.2f;
    }
    return v;
}
bool hasAppointment(unsigned);
bool offerRoadside(Pool *pool, Byte *player) {
    if (!batteryEvents || roadside.person || player[0x2F] != 0 || !onFoot(player))
        return false;
    auto cars = at<Pool *>(0xB74494);
    if (!validPool(cars))
        return false;
    Vec origin = position(player);
    for (unsigned slot = 0; slot < static_cast<unsigned>(pool->size); ++slot) {
        if (pool->flags[slot] & 0x80)
            continue;
        auto ped = pedAt(pool, slot);
        auto h = handle(pool, slot);
        unsigned id = bindings.find(h);
        if (!id || !eligible(ped, player) || !onFoot(ped) || ped[0x2F] != 0 || busy(ped) ||
            armed(ped) || !visibleTarget(ped))
            continue;
        auto &f = registry.families[id - 1];
        if (f.daily.busyUntil > registry.worldMinutes || hasAppointment(id) || f.social.romance >= 3 ||
            f.social.helped || f.social.robbed || f.social.anger >= 40 || !canFight(ped))
            continue;
        float pd = distance2(origin, position(ped));
        if (pd < 16 || pd > 1024)
            continue;
        auto manager = taskManager(ped);
        if (at<void *>(reinterpret_cast<std::uintptr_t>(manager) + 3 * 4))
            continue;
        for (unsigned c = 0; c < static_cast<unsigned>(cars->size); ++c) {
            if (cars->flags[c] & 0x80)
                continue;
            auto car = carAt(cars, c);
            unsigned stamp = (c << 8) | cars->flags[c];
            if (usedParkedCars.count(stamp) || car[0x4A4] != 3 ||
                *reinterpret_cast<int *>(car + 0x590) != 0 ||
                *reinterpret_cast<void **>(car + 0x460) || car[0x484] || (car[0x42A] & 2) ||
                (car[0x42D] & 2) || (car[0x428] & 16) ||
                *reinterpret_cast<float *>(car + 0x4C0) < 800)
                continue;
            Vec velocity = *reinterpret_cast<Vec *>(car + 0x44);
            if (velocity.x * velocity.x + velocity.y * velocity.y > 0.0004f)
                continue;
            if (distance2(position(ped), position(car)) > 64 ||
                distance2(origin, position(car)) > 1600)
                continue;
            void *task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x20);
            if (!task)
                return false;
            reinterpret_cast<void(__thiscall *)(void *, int, bool, bool, float)>(0x62F310)(
                task, 120000, false, false, 4.0f);
            setTask(manager, task, 3);
            roadside = {id,
                        h,
                        carHandle(cars, c),
                        task,
                        GetTickCount(),
                        GetTickCount(),
                        static_cast<Byte>(car[0x428] & 16),
                        static_cast<Byte>(car[0x42D] & 2),
                        {}};
            car[0x42D] |= 2;
            car[0x428] &= static_cast<Byte>(~16u);
            usedParkedCars.insert(stamp);
            say(f, "Arabanin akusu bitti! Bir el atabilir misin?");
            help("Yakininda arabasina yardim bekleyen biri var. ALT + G ile konus.");
            log("BATTERY offered; identity=" + std::to_string(id));
            return true;
        }
    }
    return false;
}
// Peds react briefly to CJ walking up (look, sidestep). Only a lasting loss of our wait task
// ends an encounter; a lost task is re-issued as soon as the ped is free again.
bool keepWaiting(Byte *ped, void *&waitTask, DWORD &troubled, int duration) {
    auto manager = taskManager(ped);
    if (!manager)
        return false;
    bool owned = at<void *>(reinterpret_cast<std::uintptr_t>(manager) + 12) == waitTask;
    if (owned && !busy(ped)) {
        troubled = 0;
        return true;
    }
    if (!owned && !busy(ped) && !primaryOccupied(ped)) {
        if (auto task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x20)) {
            reinterpret_cast<void(__thiscall *)(void *, int, bool, bool, float)>(0x62F310)(
                task, duration, false, false, 4.0f);
            setTask(manager, task, 3);
            waitTask = task;
            troubled = 0;
            return true;
        }
    }
    if (!troubled)
        troubled = GetTickCount();
    return GetTickCount() - troubled < 6000;
}
void updateRoadside(Pool *pool, Byte *player, bool mission) {
    if (!roadside.person)
        return;
    auto cars = at<Pool *>(0xB74494);
    if (mission || !exists(pool, roadside.owner) || !carExists(cars, roadside.vehicle) ||
        GetTickCount() - roadside.started > 120000) {
        cancelRoadside();
        return;
    }
    auto ped = pedAt(pool, roadside.owner.slot), car = carAt(cars, roadside.vehicle.slot);
    auto &f = registry.families[roadside.person - 1];
    if (!eligible(ped, player) || !f.presence.alive ||
        *reinterpret_cast<float *>(ped + 0x540) <= 0 ||
        !keepWaiting(ped, roadside.waitTask, roadside.troubled, 120000) ||
        f.social.anger >= 45 || *reinterpret_cast<void **>(car + 0x460) ||
        *reinterpret_cast<float *>(car + 0x4C0) < 700 ||
        distance2(position(player), position(car)) > 6400) {
        cancelRoadside();
        return;
    }
    DWORD now = GetTickCount(), elapsed = now - roadside.last;
    roadside.last = now;
    bool closeEnough =
        onFoot(player) && player[0x2F] == 0 && distance2(position(player), bonnet(car)) < 6.25f;
    bool holding = controls::down(controls::Repair);
    if (street::repairStep(roadside.repair, elapsed, holding, closeEnough)) {
        bool first = !f.social.helped;
        auto reply = street::batteryHelp(f);
        if (first)
            later::helped(registry, f.id);
        justMet(f);
        say(f, reply.line);
        dirty = true;
        panelInvalid = true;
        audio.speak(audioChoices.choose(socialaudio::Context::Repair, GetTickCount()));
        reputation(1);
        cancelRoadside(true);
        save();
        help("Yardimlasman gunlugune eklendi.");
        log("BATTERY completed; identity=" + std::to_string(f.id));
    }
}
// A yellow radar blip follows whoever is asking CJ for help.
int eventBlip = -1;
people::Handle eventBlipOwner{};
void clearEventBlip() {
    if (eventBlip != -1)
        reinterpret_cast<void(__cdecl *)(int)>(0x587CE0)(eventBlip);
    eventBlip = -1;
    eventBlipOwner = {};
}
bool lifeActive();
void cancelLife();
std::string lifeAction(unsigned id);
bool offerLife(Pool *, Byte *, encounters::Kind, unsigned preferred = 0);
Byte *drivenCar(Byte *);
bool stoppedCar(Byte *);
bool hasAppointment(unsigned);
bool companionOwns(unsigned);
void closePhone();
int familyClock = -1;
#include "SideEvents.inc"
#include "LifeEvents.inc"
#include "Ambush.inc"
#include "Gangs.inc"
bool companionOwns(unsigned id);
void releaseRoutine(unsigned id);
void clearRoutines();
void cancelCompanion(bool fail);
void releaseCompanionTask();
bool beginCompanion(people::Appointment &, people::Family &, people::Handle);
bool companionAction(unsigned, street::Action);
void companionChoice(unsigned);
void updateSocialWorld(Pool *, Byte *);
void sendPhoneChoice(unsigned, unsigned, bool);
void showRoutineRoute(people::Point);
bool departureOwns(unsigned);
#include "PhoneNative.inc"
#include "PhoneMessages.inc"
#include "SocialNative.inc"
#include "Ambient.inc"
void observeCriminalAttack(Pool *pool, Byte *player) {
    const float health = *reinterpret_cast<float *>(player + 0x540);
    const bool hurt = observedPlayerHealth >= 0 && health + 0.5f < observedPlayerHealth;
    observedPlayerHealth = health;
    if (!hurt) return;
    auto attacker = *reinterpret_cast<Byte **>(player + 0x764);
    const auto start = reinterpret_cast<std::uintptr_t>(pool->objects);
    const auto address = reinterpret_cast<std::uintptr_t>(attacker);
    if (address < start || (address - start) % 0x7C4) return;
    const auto slot = (address - start) / 0x7C4;
    if (slot >= static_cast<unsigned>(pool->size) || (pool->flags[slot] & 0x80)) return;
    const unsigned id = bindings.find(handle(pool, static_cast<unsigned>(slot)));
    if (!id || *reinterpret_cast<int *>(attacker + 0x598) != nativetalk::Criminal) return;
    const auto &s = registry.families[id - 1].social;
    if (s.provokes || s.robbed || s.feud) return; // CJ cannot manufacture a self-defense bonus.
    offenderUntil[id] = GetTickCount() + 120000;
}
bool visibleTarget(Byte *ped) {
    Vec head = position(ped);
    head.z += 1.0f;
    Vec screen{};
    float w = 0, h = 0;
    if (!reinterpret_cast<bool(__cdecl *)(const Vec &, Vec *, float *, float *, bool, bool)>(
            0x70CE30)(head, &screen, &w, &h, true, true))
        return false;
    int width = at<int>(0xC17044), height = at<int>(0xC17048);
    if (screen.x < 0 || screen.y < 0 || screen.x > width || screen.y > height)
        return false;
    Vec camera = at<Vec>(0xB6F028 + 0x974 + 0x30);
    return reinterpret_cast<bool(__cdecl *)(const Vec &, const Vec &, bool, bool, bool, bool, bool,
                                            bool, bool)>(0x56A490)(camera, head, true, true, false,
                                                                   true, true, false, false);
}
void updateFocus(Pool *pool, Byte *player) {
    if (nativeConversation(player)) {
        focused = 0;
        focusedHandle = {};
        return;
    }
    DWORD now = GetTickCount();
    if (!due(now, nextFocus) && focused && exists(pool, focusedHandle))
        return;
    nextFocus = now + 100;
    std::vector<street::Candidate> candidates;
    std::unordered_map<unsigned, people::Handle> handles;
    Vec v = position(player);
    for (unsigned slot = 0; slot < static_cast<unsigned>(pool->size); ++slot) {
        if (pool->flags[slot] & 0x80)
            continue;
        auto ped = pedAt(pool, slot);
        if (nativeConversation(ped) || !eligible(ped, player) || !onFoot(ped) ||
            ped[0x2F] != player[0x2F] || *reinterpret_cast<float *>(ped + 0x540) <= 0)
            continue;
        float d = distance2(v, position(ped));
        if (d > 64)
            continue;
        auto h = handle(pool, slot);
        unsigned id = bindings.find(h);
        if (!id)
            continue;
        candidates.push_back({id, d, 1.0f, visibleTarget(ped)});
        handles[id] = h;
    }
    focused = street::focus(candidates, focused);
    if (focused)
        focusedHandle = handles[focused];
    else
        focusedHandle = {};
}
void updateTalkVisits(Pool *pool, Byte *player) {
    for (auto &item : liveTalks) {
        auto &live = item.second;
        if (!exists(pool, live.lastHandle) || bindings.find(live.lastHandle) != item.first ||
            distance2(position(player), position(pedAt(pool, live.lastHandle.slot))) > 225)
            live.left = true;
    }
}
// CJ just helped this person: the next greeting continues the same conversation instead of
// opening like a reunion ("the battery you fixed still works") seconds after the repair.
void justMet(people::Family &f, people::Handle h) {
    auto &live = liveTalks[f.id];
    street::continueAfterHelp(live.encounter, f.id);
    if (!h.model)
        bindings.locate(f.id, h);
    live.lastHandle = h;
    live.left = false;
    live.last = GetTickCount();
    f.daily.memories |= (f.social.helped ? 16u : 0u) | (f.daily.outings ? 128u : 0u);
    if (f.social.newsKind == 2)
        f.social.newsKind = 0;
    companionWitness(f.id, socialcontext::Witness::Help);
}
// Police have had enough: a wanted level brings the regular arrest chase, not a scripted busted.
void arrestByPolice() {
    auto wanted = reinterpret_cast<Byte *(__cdecl *)(int)>(0x56E230)(0);
    if (!wanted)
        return;
    reinterpret_cast<void(__thiscall *)(Byte *, int)>(0x562470)(wanted, 2);
    help("POLICE_ARREST");
}
void interact(Pool *pool, Byte *player, street::Action action) {
    if (!focused || !exists(pool, focusedHandle) || bindings.find(focusedHandle) != focused ||
        !due(GetTickCount(), nextSpeech))
        return;
    auto ped = pedAt(pool, focusedHandle.slot);
    if (nativeConversation(player) || nativeConversation(ped)) {
        focused = 0;
        return;
    }
    auto &f = registry.families[focused - 1];
    auto &live = liveTalks[focused];
    DWORD now = GetTickCount();
    if (!eligible(ped, player) || *reinterpret_cast<float *>(ped + 0x540) <= 0 ||
        distance2(position(player), position(ped)) > 64 || !visibleTarget(ped))
        return;
    if (!live.encounter.id || (live.left && now - live.last >= 300000)) {
        if (live.encounter.id && f.social.resentment)
            f.social.resentment = f.social.resentment > 5 ? f.social.resentment - 5 : 0;
        live.encounter = {focused};
        live.left = false;
    }
    live.lastHandle = focusedHandle;
    releaseRoutine(f.id);
    if (action == street::Action::Provoke && !companionOwns(f.id)) {
        // Provoking someone you are about to meet calls the meeting off on the spot.
        bool called = false;
        for (auto &a : registry.appointments)
            if (a.person == f.id && meetings::active(a)) {
                a.state = 4;
                called = true;
            }
        if (called) {
            dirty = true;
            // Stood up and insulted: they will not answer CJ's texts for a while.
            socialworld::meetingArgument(f);
            help(locale::format("MEET_CANCEL_PROVOKE", dialogue::displayName(f)));
        }
    }
    if (companionAction(f.id, action))
        return;
    if (nightOffer(f.id) && action == street::Action::Defuse) {
        say(f, "NIGHT_DECLINED");
        cancelLife();
        nextSpeech = now + 2800;
        return;
    }
    if (action == street::Action::Greet && converseLife(player))
        return;
    if (action == street::Action::Greet && converseSmallEvent(player))
        return;
    if (roadside.person == focused && action == street::Action::Greet) {
        say(f, "Kaputun altindaki aku baglantisina bakabilir misin? Burada kaldim.");
        nextSpeech = now + 2800;
        return;
    }
    if (action == street::Action::Greet && !street::mayGreet(live.encounter))
        return;
    if (action == street::Action::Defuse &&
        (live.encounter.apologyUsed || live.encounter.brokenTruce || f.social.resentment >= 55 ||
         !f.social.anger))
        return;
    if (action == street::Action::Greet && !lifeActive() &&
        offerPartnerHome(pool, player, f, focusedHandle))
        return;
    if (action == street::Action::Greet && f.social.newsKind && !live.encounter.turns) {
        auto source = f.social.newsFrom;
        std::string name = source && source <= registry.families.size() &&
                                   (registry.families[source - 1].social.known & people::Name)
                               ? dialogue::displayName(registry.families[source - 1])
                               : locale::tr("NET_SOMEONE");
        bool loss = source && source <= registry.families.size() &&
                    !registry.families[source - 1].presence.alive;
        say(f, locale::format(f.social.newsKind == 2 ? "NET_THANKS"
                              : loss                 ? "NET_LOSS"
                                                     : "NET_WARNING",
                              name));
        f.social.newsKind = 0;
        dirty = true;
        nextSpeech = now + 3000;
        save();
        return;
    }
    if (action == street::Action::Greet && !live.encounter.turns &&
        dialogue::knows(f, people::Name)) {
        const char *memory = socialworld::memory(f);
        unsigned recalled = f.social.robbed          ? 32u
                            : f.social.helped        ? 16u
                            : (f.daily.memories & 2) ? 64u
                            : f.daily.outings        ? 128u
                                                     : 0u;
        if (recalled && !(f.daily.memories & recalled)) {
            f.daily.memories |= recalled;
            say(f, memory);
            nextSpeech = now + 3500;
            dirty = true;
            save();
            return;
        }
    }
    if (action == street::Action::Greet && converseSmallEvent(player))
        return;
    if (roadside.person == focused && action == street::Action::Greet) {
        audio.speak(audioChoices.choose(socialaudio::Context::Help, now));
        say(f, "Kaputun altindaki aku baglantisina bakabilir misin? Burada kaldim.");
        nextSpeech = now + 2800;
        return;
    }
    if (action == street::Action::Greet && live.encounter.ended) {
        if (!street::farewell(live.encounter))
            return;
        static const char *lines[] = {
            "Simdi gitmem gerekiyor. Sonra konusuruz.", "Kendine iyi bak CJ. Ben gideyim.",
            "Sonra devam ederiz dostum.", "Bugunluk bu kadar. Gorusuruz!"};
        say(f, lines[people::mix(now + f.id) % 4]);
        audio.speak(audioChoices.choose(socialaudio::Context::Goodbye, now));
        live.last = now;
        live.next = now + 300000;
        nextSpeech = now + 4000;
        return;
    }
    if (action == street::Action::Provoke)
        live.encounter.ended = false;
    live.lastHandle = focusedHandle;
    // Street dealers (type 17) and gang members (7-16) run a hang-out/dealing routine as a
    // primary task; that is their idle state, not a reason to brush CJ off.
    const int pedType = *reinterpret_cast<int *>(ped + 0x598);
    const bool dealer = nativetalk::ambientRoutine(pedType);
    if (action == street::Action::Greet && (busy(ped) || (primaryOccupied(ped) && !dealer))) {
        street::rejectBusy(live.encounter);
        live.last = now;
        live.next = now + 300000;
        static const char *lines[] = {
            "Su an mesgulum, sonra konusalim.", "Biraz acelem var. Sonra olur mu?",
            "Simdi kafam dolu, musade et.", "Bir isi halletmem gerekiyor. Gorusuruz."};
        say(f, lines[people::mix(now + f.id) % 4]);
        nextSpeech = now + 3500;
        return;
    }
    if (action == street::Action::Provoke) {
        if (lifeEvent.person == focused)
            cancelLife();
        if (roadside.person == focused)
            cancelRoadside();
        if (smallEvent.person == focused)
            cancelSmallEvent();
    }
    int trustBefore = f.social.trust;
    bool friendBefore = street::isFriend(f);
    const std::string nameBefore = dialogue::displayName(f);
    auto reply = street::exchange(f, live.encounter, action, canFight(ped),
                                  *reinterpret_cast<int *>(ped + 0x598), registry.honor);
    auto context =
        socialaudio::context(action, live.encounter.turns, friendBefore, reply,
                             *reinterpret_cast<int *>(ped + 0x598), f.female, f.social.trust);
    if (action == street::Action::Defuse) {
        if (armed(ped))
            context = socialaudio::Context::Fear;
        if (reply.calmed)
            calmOwnedFight(pool, focusedHandle);
    }
    const wchar_t *clip = audioChoices.choose(context, now + f.id + audioVariation++);
    // Rare easter eggs: a local curse when provoking, the odd rickroll when greeting.
    if (action == street::Action::Greet && people::mix(now * 7 + f.id) % 150 == 0)
        clip = socialaudio::rickroll();
    else if (action == street::Action::Provoke && !reply.arrest &&
             people::mix(now + f.id * 3) % 4 == 0)
        if (auto local = socialaudio::localizedInsult(static_cast<unsigned>(locale::language)))
            clip = local;
    audio.speak(clip);
    int honorCue = 0;
    if (action == street::Action::Greet && live.encounter.turns == 1 &&
        reply.reaction == dialogue::Reaction::Friendly)
        honorCue = honorBatch.record(f.id, true, now);
    else if (action == street::Action::Provoke && f.social.trust < trustBefore)
        honorCue = honorBatch.record(f.id, false, now);
    if (!honorCue) honorCue = socialaudio::honorCue(action, trustBefore, f.social.trust, reply);
    reputation(honorCue);
    if (f.social.known & people::Name)
        discoverNetwork(f.id);
    if (honorCue)
        network::news(registry, f.id, honorCue < 0);
    live.last = now;
    live.next = now + (reply.leaves ? 300000 : 3000);
    nextSpeech = now + 2800;
    say(f, reply.line);
    dirty = true;
    panelInvalid = true;
    if (reply.forgotten)
        help(locale::format("PHONE_CONTACT_REMOVED", nameBefore));
    if (reply.arrest)
        arrestByPolice();
    reinterpret_cast<void(__thiscall *)(Byte *, Byte *, bool, bool)>(0x5DEE40)(ped, player, true,
                                                                               false);
    reinterpret_cast<void(__thiscall *)(Byte *, unsigned)>(0x5DF8D0)(ped, 1800);
    if (reply.reaction != dialogue::Reaction::Fight && !primaryOccupied(ped))
        gesture(pool, focusedHandle,
                reply.reaction == dialogue::Reaction::Angry         ? 144
                : reply.reaction == dialogue::Reaction::Indifferent ? 135
                                                                    : 12);
    if (reply.newFriend)
        help("Artik seni arkadasi olarak goruyor. F6 ile gunlugune bak.");
    else if (reply.newJournal)
        help("Gunlugune bir tanidik eklendi. F6 ile oku.");
    if (reply.reaction == dialogue::Reaction::Fight) {
        if (roadside.person == focused)
            cancelRoadside();
        startFight(ped, player);
    }
    save();
    log("WALK SOCIAL identity=" + std::to_string(f.id) +
        " journal=" + std::to_string(f.social.journal));
    if (action == street::Action::Provoke)
        companionWitness(f.id, socialcontext::Witness::Provoke);
}
void resetRuntime() {
    updateDealerTalks(at<Pool *>(0xB74490), nullptr, false);
    resetAmbient();
    honorBatch = {};
    honorCheat = {};
    defendedOffenders.clear();
    offenderUntil.clear();
    observedPlayerHealth = -1;
    cancelCompanion(false);
    clearRoutines();
    familyClock = -1;
    closePhone();
    clearMeetingRuns();
    clearDepartures();
    releaseActors();
    audio.stop();
    cancelEvents();
    auto pool = at<Pool *>(0xB74490);
    cleanupResponses(pool, true);
    cleanupGestures(pool, true);
    delayedFlees.clear();
    bindings.clear();
    restored.clear();
    pending = {};
    liveTalks.clear();
    pendingReplies.clear();
    nextInvitation.clear();
    gangRefused.clear();
    nextGangScan = gangCooldown = 0;
    nightOfferAfter.clear();
    nextNightOffer = nextNightProbe = 0;
    focused = 0;
    focusHeld = false;
    nextScan = nextRestore = nextFocus = nextSpeech = lastManualEvent = lastDirectorTick = 0;
    lastGameTime = 0;
    gameplaySeen = false;
    panelOpen = false;
    toastUntil = subtitleUntil = 0;
}
void tick() {
    if (disabled)
        return;
    if (panelOpen && !phoneState.capturing && controls::down(controls::PhoneClose))
        closePhone();
    if (at<bool>(0xB7CB48) || at<bool>(0xB7CB49) || at<bool>(0xBA67A4)) {
        closePhone();
        audio.stop();
        honorCheat = {};
        focusHeld = false;
        focused = 0;
        pauseEvents(GetTickCount());
        cleanupGestures(at<Pool *>(0xB74490), true);
        return;
    }
    auto player = reinterpret_cast<Byte *(__cdecl *)(int)>(0x56E210)(0);
    auto pool = at<Pool *>(0xB74490);
    if (!player || !validPool(pool))
        return;
    releaseLeasedExits(player);
    if (*reinterpret_cast<float *>(player + 0x540) <= 0) {
        updateDealerTalks(pool, player, false);
        closePhone();
        audio.stop();
        cancelEvents();
        cancelCompanion(true);
        clearMeetingRuns();
        clearDepartures();
        releaseActors();
        focused = 0;
        focusHeld = false;
        lastDirectorTick = GetTickCount();
        return;
    }
    if (!panelOpen) { // A stray control lock from an interrupted phone must never survive.
        auto pad = reinterpret_cast<Byte *(__cdecl *)(int)>(0x53FB70)(0);
        if (pad)
            *reinterpret_cast<unsigned short *>(pad + 0x10E) &=
                static_cast<unsigned short>(~0x4000u);
    }
    unsigned time = at<unsigned>(0xB7CB84);
    if (time < lastGameTime)
        resetRuntime();
    lastGameTime = time;
    DWORD now = GetTickCount();
    bool mission = storyActive();
    suppressCarSolicit(!mission);
    if (mission) {
        updateDealerTalks(pool, player, false);
        honorCheat = {};
        observedPlayerHealth = -1;
        if (!storySuspended) {
            storySuspended = true;
            closePhone();
            audio.stop();
            cleanupResponses(pool, true);
            cancelEvents();
            releaseCompanionTask();
            // The native pool game owns the screen during its minigame; preserve its result
            // observer.
            if (companion.person && companion.activity != 1)
                cancelCompanion(false);
            clearRoutines();
            clearMeetingRuns();
            clearDepartures();
            releaseActors();
            cleanupGestures(pool, true);
            delayedFlees.clear();
            pending = {};
            focused = 0;
            focusHeld = false;
            toastUntil = subtitleUntil = 0;
            if (dirty)
                save();
            log("STORY suspended all social gameplay.");
        }
        lastDirectorTick = now;
        return;
    }
    if (storySuspended) {
        storySuspended = false;
        lastDirectorTick = now;
        log("STORY resumed free roam.");
    }
    if (!gameplaySeen) {
        gameplaySeen = true;
        lastDirectorTick = now;
        auto ini = directory / L"settings.ini";
        batteryEvents = GetPrivateProfileIntW(L"World", L"BatteryEvents", 1, ini.c_str()) != 0;
        sideEvents = GetPrivateProfileIntW(L"World", L"SideEvents", 1, ini.c_str()) != 0;
        modTalkOverNative =
            GetPrivateProfileIntW(L"Interface", L"ModTalkOverNative", 1, ini.c_str()) != 0;
        gangTalk = GetPrivateProfileIntW(L"World", L"GangTalk", 1, ini.c_str()) != 0;
        eventDirector.initial(
            GetPrivateProfileIntW(L"World", L"FirstEventSeconds", 45, ini.c_str()));
        help("ALT: odaklan / G-H-J-R: etkilesim / F6: gunluk");
        log("GAMEPLAY active; social/robbery/side events enabled.");
        if (recovered)
            help("SAVE_RECOVERED");
        else if (expiredMeetings)
            help("PHONE_OLD_MEETINGS");
    }
    observeCriminalAttack(pool, player);
    if (due(now, nextScan)) {
        scan(pool, player);
        updateTalkVisits(pool, player);
        nextScan = now + 250;
    }
    cleanupGestures(pool);
    cleanupResponses(pool);
    updateDelayedFlees(pool, player);
    advanceReturn(pool, player, mission);
    if (autoRestore && !mission && !pending.id && due(now, nextRestore) && player[0x2F] == 0 &&
        hasRoom(pool)) {
        nextRestore = now + 15000;
        Vec v = position(player);
        auto candidates = spatial.nearby(v.x, v.y);
        if (!candidates.empty()) {
            for (unsigned n = 0; n < candidates.size(); ++n) {
                unsigned id = candidates[(restoreCursor + n) % candidates.size()];
                auto &f = registry.families[id - 1];
                if (!hasAppointment(id) && (!returnAfter.count(id) || due(now, returnAfter[id])) &&
                    people::canReturn(f, bindings, v.x, v.y, v.z, player[0x2F])) {
                    requestReturn(id, {f.presence.x, f.presence.y, f.presence.z}, false);
                    break;
                }
            }
            ++restoreCursor;
        }
    }
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    if (pid != GetCurrentProcessId()) {
        closePhone();
        audio.stop();
        honorCheat = {};
        focusHeld = false;
        focused = 0;
        pauseEvents(now);
        return;
    }
    // CMenuManager::m_nPrefsSfxVolume, verified SDK offset 0x4F (range 0..127).
    if (mission) {
        closePhone();
        audio.stop();
    }
    int clockMinute = static_cast<int>(at<Byte>(0xB70153)) * 60 + at<Byte>(0xB70152);
    if (familyClock >= 0) {
        unsigned minutes = static_cast<unsigned>((clockMinute - familyClock + 1440) % 1440);
        if (minutes && minutes <= 10) {
            registry.worldMinutes =
                std::min<std::uint64_t>(1000000000, registry.worldMinutes + minutes);
            dirty = true;
            for (auto &f : registry.families) {
                if (f.social.childStage == 1)
                    dirty = true;
                if (romance::advance(f, minutes))
                    help("ROMANCE_BIRTH");
            }
        }
    }
    familyClock = clockMinute;
    for (unsigned key = 0; key < 26; ++key) {
        const bool down = (GetAsyncKeyState('A' + static_cast<int>(key)) & 0x8000) != 0;
        if (!panelOpen && !nativeConversation(player) && down && !cheatHeld[key] &&
            honorCheat.letter(static_cast<char>('A' + key), now)) {
            registry.honor = 100;
            audio.reputation(1);
            help("HONOR_CHEAT");
            dirty = true;
            save();
            log("HONOR cheat activated");
        }
        cheatHeld[key] = down;
    }
    if (panelOpen) honorCheat = {};
    audio.update(at<Byte>(0xBA6748 + 0x4F));
    suppressNativeTalk(pool, player);
    if (nativeConversation(player)) {
        closePhone();
        audio.stop();
        subtitleUntil = 0;
        focused = 0;
    }
    updateRoadside(pool, player, mission);
    updateSmallEvent(pool, player, mission);
    bool eventSafe = safeForEvent(player, mission);
    unsigned elapsed = lastDirectorTick ? now - lastDirectorTick : 0;
    updateLife(pool, player, elapsed);
    if (lifeEvent.night && lifeEvent.stage >= 4) {
        focused = 0;
        focusHeld = false;
        lastDirectorTick = now;
        return;
    }
    updateEventBlip(pool);
    updateFeud(pool, player);
    updateAmbush(pool, player, mission);
    updateGangs(pool, player);
    updateLater(pool, player, mission,
                sideEvents && eventSafe && !eventActive() && !focusHeld);
    eventDirector.step(elapsed, eventSafe);
    lastDirectorTick = now;
    updateMeetings(pool, player, elapsed, mission);
    updateSocialWorld(pool, player);
    updateAmbient(pool, player);
    if (sideEvents && eventSafe && !eventActive() && !focusHeld && due(now, nextNightProbe) &&
        due(now, nextNightOffer) && life::night(at<Byte>(0xB70153))) {
        nextNightProbe = now + 1000;
        if (offerLife(pool, player, encounters::Kind::Visit))
            eventDirector.started(now);
    }
    if (sideEvents && eventSafe && !eventActive() && eventDirector.due() && !focusHeld &&
        (!nextSpeech || due(now, nextSpeech + 5000)))
        trySideEvent(pool, player, false);
    // Events come only from the director's own schedule; there is no manual trigger.
    if (dirty && due(now, nextSave)) {
        save();
        nextSave = now + 15000;
    }
    if (due(now, nextHeartbeat)) {
        nextHeartbeat = now + 30000;
        log("WORLD records=" + std::to_string(registry.families.size()));
    }
    bool toggle = pressAction(controls::Phone, heldPanel),
         page = pressAction(controls::Contacts, heldPage),
         archive = pressAction(controls::Meetings, heldArchive), reunion = press(VK_F9, heldReunion),
         flush = pressAction(controls::Save, heldSave);
    bool greet = pressAction(controls::Greet, heldGreet),
         provoke = pressAction(controls::Provoke, heldProvoke),
         defuse = pressAction(controls::Defuse, heldDefuse), rob = pressAction(controls::Rob, heldRob);
    auto car = drivenCar(player);
    focusHeld = !mission && !(panelOpen && phoneState.page == phone::Page::Compose) &&
                (onFoot(player) || (car && stoppedCar(car))) &&
                controls::down(controls::Focus);
    if (focusHeld) {
        closePhone();
        updateFocus(pool, player);
        if (standoffActive() && (greet || provoke))
            negotiateGang(player, provoke);
        else if (rob)
            robPerson(pool, player);
        else if (defuse)
            interact(pool, player, street::Action::Defuse);
        else if (greet)
            interact(pool, player, street::Action::Greet);
        else if (provoke)
            interact(pool, player, street::Action::Provoke);
    } else
        focused = 0;
    if (toggle && !controls::down(controls::Focus)) {
        togglePhone();
        return;
    }
    processPhone(player);
    if (mission)
        return;
    if (archive && panelOpen) {
        phoneState.page = phone::Page::Meetings;
        phoneState.cursor = 0;
        panelInvalid = true;
    }
    if (page && panelOpen) {
        phoneState.page = phone::Page::Contacts;
        phoneState.cursor = 0;
        panelInvalid = true;
    }
    if (flush)
        help(save() ? "Gunluk ve dunya kaydedildi." : "Kayit hatasi. Log dosyasina bak.");
    if (reunion && developerReunion && selected) {
        Vec v = position(player);
        v.x += 8;
        if (hasRoom(pool))
            requestReturn(selected, v, true);
    }
}
