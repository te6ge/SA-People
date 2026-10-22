// Sudden street incidents in the style of open-world westerns: a pickpocket, a mugging in
// progress and a hothead looking for a fight. They start without being asked for, use only
// ambient pedestrians that are already there and own only the tasks they give.
struct Ambush {
    unsigned kind = 0; // 1 pickpocket, 2 mugging, 3 hothead
    people::Handle actor{}, victim{};
    DWORD started = 0;
    int stolen = 0;
    int blip = -1;
} ambush;
bool ambushActive() {
    return ambush.kind != 0;
}
void endAmbush() {
    if (ambush.blip != -1)
        reinterpret_cast<void(__cdecl *)(int)>(0x587CE0)(ambush.blip);
    ambush = {};
}
void markAmbush(Pool *pool, people::Handle h) {
    const int reference = static_cast<int>((h.slot << 8) | (pool->flags[h.slot] & 0x7F));
    ambush.blip = reinterpret_cast<int(__cdecl *)(int, int, unsigned, int)>(0x5839A0)(2, reference, 0, 3);
    if (ambush.blip != -1)
        reinterpret_cast<void(__cdecl *)(int, unsigned)>(0x583AB0)(ambush.blip, 0); // Red.
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
        if (id && (registry.families[id - 1].social.journal || registry.families[id - 1].social.feud))
            continue; // Never turn a contact into a thief.
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
    auto list = ambushCandidates(pool, player, 4.0f, 15.0f, true);
    for (const auto &h : list) {
        auto ped = pedAt(pool, h.slot);
        if (!startFight(ped, player))
            continue;
        ambush = {3, h, {}, now, 0, -1};
        markAmbush(pool, h);
        if (unsigned id = bindings.find(h))
            say(registry.families[id - 1], "AMB_HOTHEAD_LINE");
        help("AMB_HOTHEAD");
        audio.speak(audioChoices.choose(socialaudio::Context::Fight, now));
        log("AMBUSH hothead started");
        return true;
    }
    return false;
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
            if (!actorDown)
                reputation(1);
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
                dirty = true;
            }
            reputation(2);
            audio.speak(audioChoices.choose(socialaudio::Context::Help, now));
            help(locale::format("AMB_MUGGING_THANKS", std::to_string(reward)));
            endAmbush();
        } else if (!actorHere || !victimHere || apart > 14400 || age > 60000)
            endAmbush();
        return;
    }
    if (!actorHere || actorDown || apart > 10000 || age > 45000)
        endAmbush();
}
