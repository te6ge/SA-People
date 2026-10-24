// Rival gang standoff ([World] GangTalk=1). Gangs attack CJ because their ped type hates
// PED_TYPE_PLAYER1 (bit 0) in the ped type acquaintance table: CPedType::ms_apPedTypes, a pointer
// at 0xC0BBE8 (read by CPedType::Save 0x5D3CD0), 0x14 bytes per type: respect, like, ignore,
// dislike +0xC, hate +0x10. When CJ walks on foot into such a gang, that bit is lifted for a few
// seconds and they size him up. Holding the focus key and greeting tries to talk them down; the
// odds depend on honor, how many they are and whether CJ aims at them. Success keeps the gang calm
// for two minutes; failure, a provocation or silence restores the bit and they attack as usual.
// Every lifted bit is put back when the truce ends and whenever the mod stands down (missions,
// gang wars, interiors), so a savegame never keeps a changed relationship for long.
bool gangTalk = true;
struct GangMasks {
    unsigned hate = 0, dislike = 0;
    DWORD until = 0;
    bool lifted = false;
};
GangMasks gangState[32];
struct Standoff {
    std::vector<people::Handle> members;
    int gang = 0;
    DWORD until = 0;
} standoff;
std::vector<people::Handle> gangRefused; // Members who already turned CJ down.
DWORD nextGangScan = 0, gangCooldown = 0;
Byte *typeAcquaintance(int type) {
    auto table = at<Byte *>(0xC0BBE8);
    return table && type >= 0 && type < 32 ? table + type * 0x14 : nullptr;
}
bool gangHatesPlayer(int type) {
    auto a = typeAcquaintance(type);
    return a && (*reinterpret_cast<unsigned *>(a + 0x10) & 1u);
}
bool rivalGang(int type) {
    return type >= 7 && type <= 16 && type != 8; // Every gang but CJ's own Grove Street.
}
const char *gangName(int type) {
    static const char *const names[] = {"GANG_BALLAS", "GANG_GROVE", "GANG_VAGOS",
                                        "GANG_RIFA",   "GANG_DANANG", "GANG_MAFIA",
                                        "GANG_TRIAD",  "GANG_AZTECAS", "GANG_OTHER",
                                        "GANG_OTHER"};
    return type >= 7 && type <= 16 ? names[type - 7] : "GANG_OTHER";
}
void liftGang(int type, DWORD until) {
    auto a = typeAcquaintance(type);
    if (!a)
        return;
    auto &s = gangState[type];
    auto &dislike = *reinterpret_cast<unsigned *>(a + 0xC);
    auto &hate = *reinterpret_cast<unsigned *>(a + 0x10);
    if (!s.lifted) {
        s.hate = hate & 1u;
        s.dislike = dislike & 1u;
        s.lifted = true;
    }
    s.until = until;
    hate &= ~1u;
    dislike &= ~1u;
}
void restoreGang(int type) {
    auto &s = gangState[type];
    if (!s.lifted)
        return;
    if (auto a = typeAcquaintance(type)) {
        *reinterpret_cast<unsigned *>(a + 0xC) |= s.dislike;
        *reinterpret_cast<unsigned *>(a + 0x10) |= s.hate;
    }
    s = {};
}
void restoreGangs() {
    for (int type = 0; type < 32; ++type)
        restoreGang(type);
}
bool standoffActive() {
    return !standoff.members.empty();
}
void endStandoff(bool restore) {
    if (restore)
        restoreGangs();
    standoff = {};
}
// Drops an attack already under way (CPedIntelligence::ClearTasks, plugin-sdk 0x601420).
void calmMember(Byte *ped) {
    auto m = taskManager(ped);
    auto intelligence = *reinterpret_cast<Byte **>(ped + 0x47C);
    if (!m || !intelligence)
        return;
    bool responding = false; // Physical/event response slots hold an attack or chase.
    for (int slot = 0; slot <= 2; ++slot)
        responding = responding || at<void *>(reinterpret_cast<std::uintptr_t>(m) + slot * 4);
    if (responding)
        reinterpret_cast<void(__thiscall *)(Byte *, bool, bool)>(0x601420)(intelligence, true,
                                                                           false);
}
void gangLine(int gang, const std::string &line) {
    speaker = locale::tr(gangName(gang));
    spoken = line;
    subtitleUntil = GetTickCount() + 5500;
}
void failStandoff(const char *toast) {
    int gang = standoff.gang;
    for (const auto &h : standoff.members) {
        if (gangRefused.size() >= 64)
            gangRefused.erase(gangRefused.begin());
        gangRefused.push_back(h);
    }
    endStandoff(true);
    gangCooldown = GetTickCount() + 30000;
    gangLine(gang, locale::tr("GANG_FAIL_LINE"));
    help(locale::format(toast, locale::tr(gangName(gang))));
    audio.speak(audioChoices.choose(socialaudio::Context::Fight, GetTickCount()));
    log("GANG standoff failed gang=" + std::to_string(gang));
}
void negotiateGang(Byte *player, bool provoke) {
    if (!standoffActive())
        return;
    if (provoke) {
        failStandoff("GANG_FAIL");
        return;
    }
    DWORD now = GetTickCount();
    int chance = 40 + registry.honor / 5 - static_cast<int>(standoff.members.size()) * 4;
    if (currentWeapon(player) <= 1)
        chance += 10; // Empty hands read as no threat.
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
        chance -= 25; // Aiming at them.
    chance = std::clamp(chance, 10, 85);
    if (people::mix(now * 17 + static_cast<unsigned>(standoff.gang)) % 100 >=
        static_cast<unsigned>(chance)) {
        failStandoff("GANG_FAIL");
        return;
    }
    int gang = standoff.gang;
    liftGang(gang, now + 120000);
    standoff = {};
    gangLine(gang, locale::tr("GANG_PEACE_LINE"));
    help(locale::format("GANG_PEACE", locale::tr(gangName(gang))));
    reputation(1);
    audio.speak(audioChoices.choose(socialaudio::Context::Help, now));
    log("GANG standoff settled gang=" + std::to_string(gang) + " chance=" + std::to_string(chance));
}
void updateGangs(Pool *pool, Byte *player) {
    if (!gangTalk)
        return;
    DWORD now = GetTickCount();
    if (player[0x2F] != 0) { // Interiors (and safehouse saves) always see the real relationship.
        if (standoffActive())
            endStandoff(true);
        restoreGangs();
        return;
    }
    for (int type = 0; type < 32; ++type)
        if (gangState[type].lifted && !standoffActive() && due(now, gangState[type].until)) {
            restoreGang(type);
            log("GANG truce ended gang=" + std::to_string(type));
        }
    if (standoffActive()) {
        bool anyone = false;
        for (const auto &h : standoff.members)
            anyone = anyone || exists(pool, h);
        if (!anyone || !onFoot(player))
            endStandoff(true);
        else if (due(now, standoff.until))
            failStandoff("GANG_SILENT");
        return;
    }
    if (!due(now, nextGangScan) || !due(now, gangCooldown) || !onFoot(player))
        return;
    nextGangScan = now + 400;
    Vec origin = position(player);
    struct Found {
        people::Handle h;
        int type;
        float d;
    };
    std::vector<Found> found;
    for (unsigned slot = 0; slot < static_cast<unsigned>(pool->size); ++slot) {
        if (pool->flags[slot] & 0x80)
            continue;
        auto ped = pedAt(pool, slot);
        int type = *reinterpret_cast<int *>(ped + 0x598);
        if (ped == player || !rivalGang(type) || gangState[type].lifted || recruited(ped) ||
            !onFoot(ped) || ped[0x2F] != 0 || *reinterpret_cast<float *>(ped + 0x540) <= 0 ||
            !gangHatesPlayer(type))
            continue;
        float d = distance2(origin, position(ped));
        if (d > 900)
            continue;
        auto h = handle(pool, slot);
        if (std::find(gangRefused.begin(), gangRefused.end(), h) != gangRefused.end())
            continue;
        found.push_back({h, type, d});
    }
    const Found *nearest = nullptr;
    for (const auto &f : found)
        if (f.d <= 484 && (!nearest || f.d < nearest->d))
            nearest = &f;
    if (!nearest)
        return;
    standoff.gang = nearest->type;
    standoff.until = now + 8000;
    liftGang(standoff.gang, standoff.until + 1000);
    for (const auto &f : found)
        if (f.type == standoff.gang && standoff.members.size() < 8) {
            standoff.members.push_back(f.h);
            calmMember(pedAt(pool, f.h.slot));
        }
    gangLine(standoff.gang, locale::tr("GANG_CHALLENGE"));
    help(locale::format("GANG_STANDOFF", locale::tr(gangName(standoff.gang))) + " (" +
         controls::describe(controls::Focus) + " + " + controls::describe(controls::Greet) + ")");
    log("GANG standoff gang=" + std::to_string(standoff.gang) +
        " members=" + std::to_string(standoff.members.size()));
}
