// Optional native car solicitation gives way to the mod's on-foot encounters. Only this
// three-byte entry patch is owned here; mission/cutscene code sees the original routine.
struct SolicitPatch {
    Byte original[3]{};
    bool initialized = false, applied = false, unavailable = false;
} solicitPatch;
void suppressCarSolicit(bool enable) {
    auto target = reinterpret_cast<Byte *>(0x611790); // CPopulation::CanSolicitPlayerInCar(int).
    const Byte replacement[3] = {0x31, 0xC0, 0xC3};
    if (solicitPatch.unavailable) return;
    if (!solicitPatch.initialized) {
        // Stock 1.0 US starts by loading the model argument. Leave any other hook alone.
        const Byte expected[3] = {0x8B, 0x44, 0x24};
        if (std::memcmp(target, expected, 3) != 0) {
            solicitPatch.unavailable = true;
            log("NATIVE solicitation guard skipped: entry differs from stock 1.0 US.");
            return;
        }
        std::memcpy(solicitPatch.original, target, 3);
        solicitPatch.initialized = true;
    }
    if (solicitPatch.applied == enable) return;
    const auto current = solicitPatch.applied ? replacement : solicitPatch.original;
    if (std::memcmp(target, current, 3) != 0) { solicitPatch.unavailable = true; return; }
    DWORD old = 0;
    if (!VirtualProtect(target, 3, PAGE_EXECUTE_READWRITE, &old)) return;
    std::memcpy(target, enable ? replacement : solicitPatch.original, 3);
    FlushInstructionCache(GetCurrentProcess(), target, 3);
    DWORD ignored = 0;
    VirtualProtect(target, 3, old, &ignored);
    solicitPatch.applied = enable;
}
unsigned outfitFingerprint(Byte *player) {
    auto data = *reinterpret_cast<Byte **>(player + 0x480);
    auto clothes = data ? *reinterpret_cast<Byte **>(data + 4) : nullptr;
    if (!clothes) return 0;
    unsigned hash = 2166136261u;
    for (unsigned i = 0; i < 28; ++i) // CPedClothesDesc: 10 model + 18 texture keys.
        hash = (hash ^ reinterpret_cast<unsigned *>(clothes)[i]) * 16777619u;
    return hash;
}
DWORD nextAmbient = 0, outfitChangedAt = 0;
unsigned lastOutfit = 0;
bool outfitInitialized = false;
std::unordered_map<unsigned, DWORD> ambientAfter;
socialcontext::Topics ambientTopics;
npcbrain::BarkMemory barkMemory;
struct RecognitionRun {
    unsigned person = 0;
    people::Handle actor{};
    void *task = nullptr;
    DWORD until = 0;
    unsigned area = 0;
} recognition;
bool recognitionOwns(unsigned id) { return recognition.person == id; }
void clearRecognition() {
    auto pool = at<Pool *>(0xB74490);
    if (recognition.person && validPool(pool) && exists(pool, recognition.actor)) {
        auto ped = pedAt(pool, recognition.actor.slot);
        auto manager = taskManager(ped);
        if (manager && recognition.task &&
            at<void *>(reinterpret_cast<std::uintptr_t>(manager) + 12) == recognition.task)
            setTask(manager, nullptr, 3);
    }
    recognition = {};
}
void updateRecognition(Pool *pool, Byte *player) {
    if (!recognition.person) return;
    if (!exists(pool,recognition.actor)) {clearRecognition(); return;}
    auto ped=pedAt(pool,recognition.actor.slot);
    if (!eligible(ped,player) || player[0x2F] != recognition.area || ped[0x2F] != player[0x2F] ||
        !onFoot(player) || busy(ped) || nativeConversation(player) || nativeConversation(ped) ||
        panelOpen || focusHeld || eventActive() || hasAppointment(recognition.person) ||
        *reinterpret_cast<float *>(ped+0x540)<=0 ||
        !socialworld::available(registry.families[recognition.person-1]) ||
        due(GetTickCount(),recognition.until) || distance2(position(player),position(ped)) < 9)
        clearRecognition();
}
void resetAmbient() {
    clearRecognition();
    nextAmbient = outfitChangedAt = lastOutfit = 0;
    outfitInitialized = false; ambientAfter.clear(); ambientTopics = {}; barkMemory = {};
    suppressCarSolicit(false);
}
void clearOldCarSolicit(Pool *pool, Byte *player) {
    if (!solicitPatch.applied) return;
    for (unsigned slot = 0; slot < static_cast<unsigned>(pool->size); ++slot) {
        if (pool->flags[slot] & 0x80) continue;
        auto ped = pedAt(pool, slot);
        if (ped[0x484] != 1 || *reinterpret_cast<int *>(ped + 0x598) != 22 || recruited(ped)) continue;
        auto manager = taskManager(ped);
        if (!manager) continue;
        for (int index = 0; index < 5; ++index) {
            auto task = at<void *>(reinterpret_cast<std::uintptr_t>(manager) + index * 4);
            if (!task) continue;
            auto table = *reinterpret_cast<std::uintptr_t **>(task);
            if (reinterpret_cast<int(__thiscall *)(void *)>(table[4])(task) == 1309 &&
                *reinterpret_cast<Byte **>(static_cast<Byte *>(task) + 0xC) == player) {
                setTask(manager, nullptr, index);
                const char *text = reinterpret_cast<const char *(__thiscall *)(void *, const char *)>(
                    0x6A0050)(reinterpret_cast<void *>(0xC1B340), "PROS_04");
                if (text) reinterpret_cast<void(__cdecl *)(const char *)>(0x69EA30)(text);
            }
        }
    }
}
void updateAmbient(Pool *pool, Byte *player) {
    const DWORD now = GetTickCount();
    updateRecognition(pool, player);
    const unsigned outfit = outfitFingerprint(player);
    clearOldCarSolicit(pool, player);
    if (outfit) {
        if (outfitInitialized && lastOutfit != outfit) outfitChangedAt = now;
        lastOutfit = outfit; outfitInitialized = true;
    }
    if (!onFoot(player) || panelOpen || focusHeld || companion.person || recognition.person ||
        eventActive() || standoffActive() || now < subtitleUntil || !due(now, nextSpeech) ||
        !due(now, nextAmbient)) return;
    nextAmbient = now + 12000;
    people::Handle candidate{};
    unsigned identity = 0;
    float best = 225;
    for (unsigned slot = 0; slot < static_cast<unsigned>(pool->size); ++slot) {
        if (pool->flags[slot] & 0x80) continue;
        auto ped = pedAt(pool, slot);
        const auto h = handle(pool, slot);
        const auto id = bindings.find(h);
        if (!id || !eligible(ped, player) || !onFoot(ped) || ped[0x2F] != player[0x2F] || busy(ped) ||
            nativeConversation(ped) ||
            *reinterpret_cast<float *>(ped + 0x540) <= 0 || hasAppointment(id) ||
            (ambientAfter.count(id) && !due(now, ambientAfter[id])) || !visibleTarget(ped)) continue;
        float distance = distance2(position(player), position(ped));
        if (distance < best) { best = distance; candidate = h; identity = id; }
    }
    if (!identity) return;
    auto ped = pedAt(pool, candidate.slot);
    auto &f = registry.families[identity - 1];
    if (!socialworld::available(f) || f.social.anger >= 20) return;
    const bool changed = outfitChangedAt && now - outfitChangedAt < 240000;
    const bool grove = *reinterpret_cast<int *>(ped + 0x598) == 8;
    const auto weather = socialcontext::weather(at<float>(0xC81324), at<float>(0xC81300),
        std::max(at<float>(0xC812E0), at<float>(0xC812F8)), at<float>(0xC812F0), at<Byte>(0xB70153));
    static const char *groveLines[] = {"AMBIENT_GROVE_0", "AMBIENT_GROVE_1", "AMBIENT_GROVE_2",
        "AMBIENT_GROVE_3", "AMBIENT_GROVE_4", "AMBIENT_GROVE_5", "AMBIENT_GROVE_6", "AMBIENT_GROVE_7", "AMBIENT_GROVE_8", "AMBIENT_GROVE_9", "AMBIENT_GROVE_10", "AMBIENT_GROVE_11",
        "AMBIENT_GROVE_12", "AMBIENT_GROVE_13", "AMBIENT_GROVE_14", "AMBIENT_GROVE_15"};
    static const char *rain[] = {"AMBIENT_RAIN_0", "AMBIENT_RAIN_1", "AMBIENT_RAIN_2"};
    static const char *fog[] = {"AMBIENT_FOG_0", "AMBIENT_FOG_1"};
    static const char *sun[] = {"AMBIENT_SUN_0", "AMBIENT_SUN_1"};
    static socialprogress::LineDeck greetingDeck, knownDeck, groveDeck, rainDeck, fogDeck, sunDeck;
    const unsigned seed = people::mix(identity + now / 1000);
    const char *line = "AMBIENT_HELLO";
    if (neighborhood::canRecognize(f,false,false,false,player[0x2F],best) &&
        f.daily.busyUntil <= registry.worldMinutes && seed % 3 == 0) {
        releaseRoutine(f.id);
        if (auto task=walkingTask(ped,position(player))) {
            recognition={identity,candidate,task,now+10000,player[0x2F]};
            line=seed%2 ? "RECOG_APPROACH_0" : "RECOG_APPROACH_1";
        } else line="AMBIENT_KNOWN";
    }
    else if (player[0x2F]) line=neighborhood::interiorLine(greetingDeck.take(4,seed));
    else if (changed) { line = registry.honor <= -40 ? "AMBIENT_CLOTHES_WARY" : "AMBIENT_CLOTHES"; outfitChangedAt = 0; }
    else if (f.commitment.stage==3 && f.social.romance>=3 && f.social.trust>=40 && f.social.resentment<25 && !f.social.cutOff) {
        static const char *spouse[]={"REL_AMBIENT_0","REL_AMBIENT_1","REL_AMBIENT_2","REL_AMBIENT_3"};
        line=spouse[seed%4];
    }
    else if (currentWeapon(player)>=22 && currentWeapon(player)<=38 && *reinterpret_cast<int *>(ped+0x598)>=4 &&
             *reinterpret_cast<int *>(ped+0x598)<=6) {
        static const char *armed[]={"BRAIN_ARMED_0","BRAIN_ARMED_1","BRAIN_ARMED_2"};line=armed[seed%3];
    }
    else if (f.social.helped && seed%3==0) {
        static const char *helped[]={"BRAIN_HELPED_0","BRAIN_HELPED_1","BRAIN_HELPED_2"};line=helped[seed%3];
    }
    else if (grove) line = registry.honor <= -40 ? "AMBIENT_GROVE_WARY" : groveLines[groveDeck.take(16, seed)];
    else if (f.community.opinion<=-10) line="COMMUNITY_WARY";
    else if (f.community.opinion>=10) line="COMMUNITY_WARM";
    else if (registry.honor <= -40 && seed % 3 == 0) line = "AMBIENT_HONOR_LOW";
    else if (registry.honor >= 40 && seed % 3 == 0) line = "AMBIENT_HONOR_HIGH";
    else if (weather == socialcontext::Weather::Rain && seed % 3 == 0) line = rain[rainDeck.take(3, seed)];
    else if (weather == socialcontext::Weather::Fog && seed % 3 == 0) line = fog[fogDeck.take(2, seed)];
    else if (weather == socialcontext::Weather::Sun && seed % 3 == 0) line = sun[sunDeck.take(2, seed)];
    else if (weather == socialcontext::Weather::Wind && seed % 3 == 0) line = "AMBIENT_WIND";
    else if (f.social.known & people::Name) {
        static const char *known[] = {"AMBIENT_KNOWN", "AMBIENT_KNOWN_1", "AMBIENT_KNOWN_2", "AMBIENT_KNOWN_3", "AMBIENT_KNOWN_4", "AMBIENT_KNOWN_5", "AMBIENT_KNOWN_6",
            "AMBIENT_KNOWN_7", "AMBIENT_KNOWN_8", "AMBIENT_KNOWN_9"};
        line = known[knownDeck.take(10, seed)];
    } else {
        if (seed % 3) return;
        static const char *greetings[] = {"AMBIENT_HELLO", "AMBIENT_HELLO_1", "AMBIENT_HELLO_2", "AMBIENT_HELLO_3", "AMBIENT_HELLO_4", "AMBIENT_HELLO_5", "AMBIENT_HELLO_6",
            "AMBIENT_HELLO_7", "AMBIENT_HELLO_8", "AMBIENT_HELLO_9"};
        line = greetings[greetingDeck.take(10, seed)];
    } // Ordinary strangers often just carry on.
    const auto selectedBark=barkMemory.choose(identity,{line},seed,now);
    if(selectedBark.empty())return;
    say(f, selectedBark);
    ambientAfter[identity] = now + 300000;
    if (ambientAfter.size() > 512)
        for (auto it = ambientAfter.begin(); it != ambientAfter.end();)
            if (due(now, it->second)) it = ambientAfter.erase(it); else ++it;
    nextAmbient = now + 45000 + seed % 30000;
    nextSpeech = now + 4500;
}
