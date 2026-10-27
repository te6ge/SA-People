// Sudden street incidents in the style of open-world westerns: a pickpocket, a mugging in
// progress and a hothead looking for a fight. They start without being asked for, use only
// ambient pedestrians that are already there and own only the tasks they give.
struct Ambush {
    unsigned kind = 0; // 1 pickpocket, 2 mugging, 3 hothead, 4 lost wallet
    people::Handle actor{}, victim{};
    DWORD started = 0;
    int stolen = 0;
    int blip = -1;
} ambush;
constexpr unsigned AmbushKinds = 4;
bool activeOffender(unsigned id) {
    auto it = offenderUntil.find(id);
    return it != offenderUntil.end() && !due(GetTickCount(), it->second);
}
bool ambushActive() {
    return ambush.kind != 0;
}
void endAmbush() {
    if (ambush.blip != -1)
        reinterpret_cast<void(__cdecl *)(int)>(0x587CE0)(ambush.blip);
    ambush = {};
}
void markAmbush(Pool *pool, people::Handle h, unsigned colour = 0) {
    if (ambush.kind >= 1 && ambush.kind <= 3) {
        const unsigned id = bindings.find(h);
        if (id) offenderUntil[id] = GetTickCount() + 120000;
    }
    const int reference = static_cast<int>((h.slot << 8) | (pool->flags[h.slot] & 0x7F));
    ambush.blip = reinterpret_cast<int(__cdecl *)(int, int, unsigned, int)>(0x5839A0)(2, reference, 0, 3);
    if (ambush.blip != -1)
        reinterpret_cast<void(__cdecl *)(int, unsigned)>(0x583AB0)(ambush.blip, colour); // 0 red, 4 yellow.
}
bool attackPed(Byte *attacker, Byte *target, unsigned duration) {
    if (busy(attacker) || primaryOccupied(attacker) || !taskManager(attacker))
        return false;
    void *task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x38);
    if (!task)
        return false;
    reinterpret_cast<void(__thiscall *)(void *, Byte *, int, int, int, int, char)>(0x620E30)(
        task, target, static_cast<int>(duration), 0, 0, 100, 0);
    return ownResponse(attacker, task, duration, true);
}
// Free ambient pedestrians at a distance band, nearest first.
std::vector<people::Handle> ambushCandidates(Pool *pool, Byte *player, float nearest,
                                             float farthest, bool needFight) {
    std::vector<std::pair<float, people::Handle>> found;
    Vec origin = position(player);
    for (unsigned slot = 0; slot < static_cast<unsigned>(pool->size); ++slot) {
        if (pool->flags[slot] & 0x80)
            continue;
        auto ped = pedAt(pool, slot);
        int type = *reinterpret_cast<int *>(ped + 0x598);
        if (!eligible(ped, player) || !onFoot(ped) || ped[0x2F] != 0 || busy(ped) ||
            primaryOccupied(ped) || armed(ped) || (type != 4 && type != 5 && type != 20 && type != 21) ||
            (needFight && !canFight(ped)) || *reinterpret_cast<float *>(ped + 0x540) <= 0)
            continue;
        unsigned id = bindings.find(handle(pool, slot));
        if (id && (!street::strangerIncident(registry.families[id - 1]) || hasAppointment(id) ||
                   registry.families[id - 1].daily.busyUntil > registry.worldMinutes))
            continue; // Never recast a familiar face or an outing participant as a random offender.
        float d = distance2(origin, position(ped));
        if (d < nearest * nearest || d > farthest * farthest || !visibleTarget(ped))
            continue;
        found.push_back({d, handle(pool, slot)});
    }
    std::sort(found.begin(), found.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });
    std::vector<people::Handle> result;
    for (const auto &item : found)
        result.push_back(item.second);
    return result;
}
unsigned revengeFor = 0; // Set while a hothead is staged as someone's revenge (see Later.h).
bool startAmbush(Pool *pool, Byte *player, unsigned kind) {
    if (ambushActive() || !onFoot(player) || player[0x2F] != 0)
        return false;
    DWORD now = GetTickCount();
    if (kind == 1) {
        auto money = playerMoney(player);
        if (!money || *money < 10)
            return false;
        auto list = ambushCandidates(pool, player, 1.5f, 9.0f, false);
        if (list.empty())
            return false;
        auto ped = pedAt(pool, list[0].slot);
        if (!startFlee(ped, player))
            return false;
        ambush = {1, list[0], {}, now, std::min(*money, 10 + static_cast<int>(people::mix(now) % 51)), -1};
        *money -= ambush.stolen;
        markAmbush(pool, list[0]);
        audio.speak(audioChoices.choose(socialaudio::Context::Surprise, now));
        help(locale::format("AMB_PICKPOCKET", std::to_string(ambush.stolen)));
        log("AMBUSH pickpocket stole=" + std::to_string(ambush.stolen));
        return true;
    }
    if (kind == 2) {
        auto list = ambushCandidates(pool, player, 10.0f, 40.0f, false);
        for (std::size_t i = 0; i < list.size(); ++i)
            for (std::size_t j = 0; j < list.size(); ++j) {
                if (i == j)
                    continue;
                auto attacker = pedAt(pool, list[i].slot), victim = pedAt(pool, list[j].slot);
                if (!canFight(attacker) || distance2(position(attacker), position(victim)) > 64)
                    continue;
                if (!attackPed(attacker, victim, 45000))
                    continue;
                ambush = {2, list[i], list[j], now, 0, -1};
                markAmbush(pool, list[i]);
                help("AMB_MUGGING");
                log("AMBUSH mugging started");
                return true;
            }
        return false;
    }
    if (kind == 4) {
        // A pedestrian dropped a wallet; the owner waits nearby, marked in yellow.
        auto list = ambushCandidates(pool, player, 15.0f, 45.0f, false);
        for (const auto &h : list) {
            auto ped = pedAt(pool, h.slot);
            void *task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x20);
            if (!task)
                return false;
            reinterpret_cast<void(__thiscall *)(void *, int, bool, bool, float)>(0x62F310)(
                task, 90000, false, false, 4.0f);
            if (!ownResponse(ped, task, 90000))
                continue;
            ambush = {4, h, {}, now, 40 + static_cast<int>(people::mix(now) % 111), -1};
            markAmbush(pool, h, 4);
            help(locale::format("AMB_WALLET", std::to_string(ambush.stolen)));
            log("AMBUSH wallet started");
            return true;
        }
        return false;
    }
    auto list = ambushCandidates(pool, player, 4.0f, 15.0f, true);
    for (const auto &h : list) {
        auto ped = pedAt(pool, h.slot);
        if (!startFight(ped, player))
            continue;
        ambush = {3, h, {}, now, 0, -1};
        markAmbush(pool, h);
        std::string victim = revengeFor && revengeFor <= registry.families.size()
                                 ? locale::tr(dialogue::displayName(registry.families[revengeFor - 1]))
                                 : std::string();
        if (unsigned id = bindings.find(h))
            say(registry.families[id - 1], victim.empty()
                                               ? locale::tr("AMB_HOTHEAD_LINE")
                                               : locale::format("AMB_REVENGE_LINE", victim));
        help(victim.empty() ? locale::tr("AMB_HOTHEAD") : locale::format("LATER_REVENGE", victim));
        audio.speak(audioChoices.choose(socialaudio::Context::Fight, now));
        log("AMBUSH hothead started identity=" + std::to_string(bindings.find(h)));
        return true;
    }
    return false;
}
// Delayed consequences come due in world minutes; checked every few seconds of play.
DWORD nextLaterCheck = 0;
void updateLater(Pool *pool, Byte *player, bool mission, bool revengeReady) {
    DWORD now = GetTickCount();
    if (mission || registry.later.empty() || !due(now, nextLaterCheck))
        return;
    nextLaterCheck = now + 5000;
    people::Later item;
    if (!later::next(registry, revengeReady && onFoot(player), item))
        return;
    dirty = true;
    if (item.kind == later::Revenge) {
        revengeFor = item.source;
        bool started = startAmbush(pool, player, 3);
        revengeFor = 0;
        if (started) {
            eventDirector.started(now);
            log("LATER revenge for identity=" + std::to_string(item.source));
        } else
            registry.later.push_back(item); // No one suitable nearby yet; try again later.
        return;
    }
    unsigned gift = 0;
    if (!later::deliver(registry, item, gift))
        return;
    auto &f = registry.families[item.person - 1];
    if (gift)
        if (auto money = playerMoney(player))
            if (*money >= 0 && *money < 999999999 - static_cast<int>(gift))
                *money += static_cast<int>(gift);
    help(gift ? locale::format("LATER_GIFT", locale::tr(dialogue::displayName(f))) + " +$" + std::to_string(gift)
              : locale::format("MAIL_FROM", locale::tr(dialogue::displayName(f))));
    panelInvalid = true;
    save();
    log("LATER " + std::to_string(item.kind) + " identity=" + std::to_string(item.person));
}
void updateAmbush(Pool *pool, Byte *player, bool mission) {
    if (!ambushActive())
        return;
    DWORD now = GetTickCount(), age = now - ambush.started;
    if (mission) {
        if (ambush.kind == 1)
            if (auto money = playerMoney(player))
                *money += ambush.stolen; // Story missions are never charged for our incident.
        endAmbush();
        return;
    }
    bool actorHere = exists(pool, ambush.actor);
    Byte *actor = actorHere ? pedAt(pool, ambush.actor.slot) : nullptr;
    bool actorDown = actorHere && *reinterpret_cast<float *>(actor + 0x540) <= 0;
    float apart = actorHere ? distance2(position(player), position(actor)) : 1e9f;
    if (ambush.kind == 1) {
        if (actorDown || (actorHere && apart < 6.25f && onFoot(player))) {
            if (auto money = playerMoney(player))
                *money += ambush.stolen;
            help(locale::format("AMB_PICKPOCKET_CAUGHT", std::to_string(ambush.stolen)));
            if (unsigned id = bindings.find(ambush.actor))
                if (defendedOffenders.insert(id).second) reputation(1);
            endAmbush();
        } else if (!actorHere || apart > 22500 || age > 35000) {
            help("AMB_PICKPOCKET_LOST");
            endAmbush();
        }
        return;
    }
    if (ambush.kind == 2) {
        bool victimHere = exists(pool, ambush.victim);
        Byte *victim = victimHere ? pedAt(pool, ambush.victim.slot) : nullptr;
        if (victimHere && *reinterpret_cast<float *>(victim + 0x540) <= 0) {
            help("AMB_MUGGING_LATE");
            endAmbush();
        } else if (actorDown && victimHere) {
            int reward = 20 + static_cast<int>(people::mix(now) % 61);
            if (auto money = playerMoney(player))
                *money += reward;
            if (unsigned id = bindings.find(ambush.victim)) {
                auto &s = registry.families[id - 1].social;
                s.trust = std::min(100, s.trust + 15);
                s.known |= people::Name;
                later::helped(registry, id);
                justMet(registry.families[id - 1], ambush.victim);
                dirty = true;
            }
            if (unsigned id = bindings.find(ambush.actor))
                if (defendedOffenders.insert(id).second) reputation(1);
            audio.speak(audioChoices.choose(socialaudio::Context::Help, now));
            help(locale::format("AMB_MUGGING_THANKS", std::to_string(reward)));
            endAmbush();
        } else if (!actorHere || !victimHere || apart > 14400 || age > 60000)
            endAmbush();
        return;
    }
    if (ambush.kind == 4) {
        auto money = playerMoney(player);
        if (actorHere && !actorDown && apart < 6.25f && onFoot(player)) {
            int reward = 20 + static_cast<int>(people::mix(now + 7) % 31);
            if (money && *money >= 0 && *money < 999999999 - reward)
                *money += reward;
            if (unsigned id = bindings.find(ambush.actor)) {
                auto &f = registry.families[id - 1];
                f.social.trust = std::min(100, f.social.trust + 10);
                f.social.known |= people::Name;
                say(f, "AMB_WALLET_THANKS_LINE");
                later::helped(registry, id);
                justMet(f, ambush.actor);
                dirty = true;
            }
            reputation(2);
            audio.speak(audioChoices.choose(socialaudio::Context::Help, now));
            help(locale::format("AMB_WALLET_RETURNED", std::to_string(reward)));
            endAmbush();
        } else if (!actorHere || actorDown || age > 90000) {
            if (money && *money >= 0 && *money < 999999999 - ambush.stolen)
                *money += ambush.stolen;
            reputation(-1);
            help(locale::format("AMB_WALLET_KEPT", std::to_string(ambush.stolen)));
            endAmbush();
        }
        return;
    }
    if (!actorHere || actorDown || apart > 10000 || age > 45000)
        endAmbush();
}
