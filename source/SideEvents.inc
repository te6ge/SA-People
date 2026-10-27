// Included in the native adapter, after the roadside helpers.
struct SmallEvent {
    unsigned person = 0, stage = 0;
    people::Handle owner{};
    void *waitTask = nullptr;
    DWORD started = 0;
    encounters::Kind kind = encounters::Kind::Directions;
    bool sanFierro = false;
    DWORD troubled = 0; // When the ped first lost our wait task or got a reaction task.
} smallEvent;
bool ambushActive();
void endAmbush();
bool startAmbush(Pool *pool, Byte *player, unsigned kind);
void endStandoff(bool restore);

struct DelayedFlee {
    people::Handle owner{};
    DWORD at = 0;
};
std::vector<DelayedFlee> delayedFlees;
bool visibleTarget(Byte *ped);
void cancelSmallEvent() {
    if (!smallEvent.person)
        return;
    auto pool = at<Pool *>(0xB74490);
    if (validPool(pool) && exists(pool, smallEvent.owner)) {
        auto manager = taskManager(pedAt(pool, smallEvent.owner.slot));
        if (manager &&
            at<void *>(reinterpret_cast<std::uintptr_t>(manager) + 12) == smallEvent.waitTask)
            setTask(manager, nullptr, 3);
    }
    smallEvent = {};
}
void cancelEvents() {
    cancelCommunity();
    clearEventBlip();
    endAmbush();
    endStandoff(true);
    cancelLife();
    cancelRoadside();
    cancelSmallEvent();
}
bool eventActive() {
    return roadside.person || smallEvent.person || lifeActive() || ambushActive();
}
std::string eventAction(unsigned id) {
    if (roadside.person == id)
        return "AKUYA YARDIM ET";
    if (smallEvent.person == id)
        return smallEvent.kind == encounters::Kind::Fare ? "YOL PARASI VER ($20)"
               : smallEvent.stage                        ? "YOL TARIF ET"
                                                         : "NE OLDUGUNU SOR";
    return lifeAction(id);
}
int *playerMoney(Byte *player) {
    // CWorld::Players[0].m_pPed / CPlayerInfo::m_nMoney; SDK-validated offsets.
    if (at<Byte *>(0xB7CD98) != player)
        return nullptr;
    return reinterpret_cast<int *>(0xB7CD98 + 0xB8);
}
bool startFlee(Byte *ped, Byte *player) {
    if (busy(ped) || primaryOccupied(ped))
        return false;
    auto task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x40);
    if (!task)
        return false;
    // GTA reversed: CTaskComplexSmartFleeEntity, original six-argument constructor.
    reinterpret_cast<void(__thiscall *)(void *, Byte *, bool, float, int, int, float)>(0x65C430)(
        task, player, true, 45.0f, 15000, 1000, 1.0f);
    return ownResponse(ped, task, 15000);
}
bool handsUp(Byte *ped) {
    if (primaryOccupied(ped))
        return false;
    // CTaskSimpleHandsUp extends the 0x34-byte CTaskSimpleRunTimedAnim.
    auto task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x34);
    if (!task)
        return false;
    reinterpret_cast<void(__thiscall *)(void *, int)>(0x48E970)(task, 4000);
    return ownResponse(ped, task, 4000);
}
void updateDelayedFlees(Pool *pool, Byte *player) {
    for (auto it = delayedFlees.begin(); it != delayedFlees.end();) {
        if (!exists(pool, it->owner) || due(GetTickCount(), it->at)) {
            if (exists(pool, it->owner)) {
                auto ped = pedAt(pool, it->owner.slot);
                if (eligible(ped, player) && ped[0x2F] == player[0x2F] &&
                    *reinterpret_cast<float *>(ped + 0x540) > 0 && onFoot(ped))
                    startFlee(ped, player);
            }
            it = delayedFlees.erase(it);
        } else
            ++it;
    }
}
bool offerSmallEvent(Pool *pool, Byte *player, encounters::Kind preferred) {
    Vec origin = position(player);
    unsigned best = 0;
    float score = 2000;
    people::Handle chosen{};
    encounters::Kind kind = preferred;
    for (unsigned slot = 0; slot < static_cast<unsigned>(pool->size); ++slot) {
        if (pool->flags[slot] & 0x80)
            continue;
        auto ped = pedAt(pool, slot);
        auto h = handle(pool, slot);
        unsigned id = bindings.find(h);
        if (!id || !eligible(ped, player) || !onFoot(ped) || ped[0x2F] != 0 || busy(ped) ||
            primaryOccupied(ped) || armed(ped) || !canFight(ped))
            continue;
        const auto &eventPerson = registry.families[id - 1];
        if (!socialprogress::availableForEvent(eventPerson.daily.busyUntil, registry.worldMinutes,
                hasAppointment(id), false, eventPerson.social.romance)) continue;
        const auto &social = eventPerson.social;
        if (social.anger >= 30 || social.robbed || social.cutOff ||
            (social.events & encounters::bit(preferred)))
            continue;
        float distance = distance2(origin, position(ped));
        if (distance < 9 || distance > 625 || !visibleTarget(ped))
            continue;
        if (distance < score) {
            score = distance;
            best = id;
            chosen = h;
            kind = preferred;
        }
    }
    if (!best)
        return false;
    auto ped = pedAt(pool, chosen.slot);
    auto task = reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x20);
    if (!task)
        return false;
    reinterpret_cast<void(__thiscall *)(void *, int, bool, bool, float)>(0x62F310)(
        task, 150000, false, false, 4.0f);
    setTask(taskManager(ped), task, 3);
    Vec point = position(player);
    int city = reinterpret_cast<int(__cdecl *)(const Vec *)>(0x572300)(&point);
    smallEvent = {best, 0, chosen, task, GetTickCount(), kind, city == 2};
    static const char *fares[] = {
        "Dostum, cuzdanimi evde unutmusum. Otobus icin 20 dolar verebilir misin?",
        "Vardiyaya yetismem lazim ama yol param eksik. 20 dolar yardim eder misin?",
        "Eve donmem gerekiyor. Bilet icin 20 dolarim eksik, el atar misin?"};
    static const char *asks[] = {"Hey dostum, buraya yabanciyim. Bir yer sorabilir miyim?",
                                 "Bir dakikani alabilir miyim? Yolumu kaybettim galiba.",
                                 "Buralari biliyor musun? Bir yere nasil gidecegimi soracaktim."};
    say(registry.families[best - 1], kind == encounters::Kind::Fare ? fares[people::mix(best) % 3]
                                                                    : asks[people::mix(best) % 3], npcvoice::Cue::Help);
    help("Yakininda yardim isteyen biri var. ALT ile odaklan, G ile konus.");
    log("SIDE offered kind=" + std::to_string(static_cast<unsigned>(kind)) +
        " identity=" + std::to_string(best));
    return true;
}
void updateSmallEvent(Pool *pool, Byte *player, bool mission) {
    if (!smallEvent.person)
        return;
    if (mission || !exists(pool, smallEvent.owner) ||
        GetTickCount() - smallEvent.started > 150000) {
        cancelSmallEvent();
        return;
    }
    auto ped = pedAt(pool, smallEvent.owner.slot);
    if (!eligible(ped, player) || *reinterpret_cast<float *>(ped + 0x540) <= 0 ||
        ped[0x2F] != player[0x2F] || registry.families[smallEvent.person - 1].social.anger >= 40 ||
        distance2(position(ped), position(player)) > 10000 ||
        !keepWaiting(ped, smallEvent.waitTask, smallEvent.troubled, 150000))
        cancelSmallEvent();
}
bool converseSmallEvent(Byte *player) {
    if (!smallEvent.person || smallEvent.person != focused)
        return false;
    auto pool = at<Pool *>(0xB74490);
    auto ped = pedAt(pool, smallEvent.owner.slot);
    auto &f = registry.families[focused - 1];
    DWORD now = GetTickCount();
    nextSpeech = now + 2800;
    if (f.social.events & encounters::bit(smallEvent.kind)) {
        cancelSmallEvent();
        return true;
    }
    if (smallEvent.kind == encounters::Kind::Directions && !smallEvent.stage) {
        smallEvent.stage = 1;
        audio.speak(audioChoices.choose(smallEvent.sanFierro ? socialaudio::Context::SanFierro
                                                             : socialaudio::Context::Question,
                                        now));
        say(f, smallEvent.sanFierro
                   ? "San Fierro'ya yeni geldim. Tabelalari takip edersem ana yola cikar miyim?"
                   : "Ana yola cikmam gerekiyor. Su tabelalari takip etsem olur mu?");
        return true;
    }
    if (smallEvent.kind == encounters::Kind::Fare) {
        auto money = playerMoney(player);
        int wallet = std::max(0, static_cast<int>(*reinterpret_cast<short *>(ped + 0x756)));
        if (!money || !encounters::donate(*money, wallet, 20)) {
            audio.speak(audioChoices.choose(socialaudio::Context::RoughDay, now));
            say(f, "Sorun degil dostum. Belki baska biri yardimci olur.");
            cancelSmallEvent();
            return true;
        }
        *reinterpret_cast<short *>(ped + 0x756) = static_cast<short>(wallet);
        help("Yol parasi icin 20 dolar verdin.");
    }
    bool helped = encounters::complete(f, smallEvent.kind);
    audio.speak(audioChoices.choose(socialaudio::Context::Help, now));
    say(f, smallEvent.kind == encounters::Kind::Fare
               ? "Cok sag ol CJ! Ben " + people::generate(f)[0].name + ". Yetismemi sagladin."
               : "Tabelalari takip edecegim. Ben " + people::generate(f)[0].name +
                     ". Sag ol CJ, iyi ki sordum.");
    if (helped) {
        reputation(1);
        later::helped(registry, f.id);
    }
    justMet(f);
    dirty = true;
    panelInvalid = true;
    cancelSmallEvent();
    save();
    log("SIDE completed identity=" + std::to_string(f.id));
    return true;
}
bool safeForEvent(Byte *player, bool mission) {
    auto wanted = reinterpret_cast<Byte *(__cdecl *)(int)>(0x56E230)(0);
    return !nativeConversation(player) && !mission && player[0x2F] == 0 && !communityBusy() &&
           (onFoot(player) || drivenCar(player)) && !busy(player) && !panelOpen &&
           (!wanted || !*reinterpret_cast<unsigned *>(wanted + 0x2C));
}
bool trySideEvent(Pool *pool, Byte *player, bool manual) {
    if (eventActive()) {
        if (manual)
            help("Olay zaten aktif. Basinin uzerindeki isarete yaklasip ALT + G kullan.");
        return false;
    }
    // Two in five automatic events are sudden incidents rather than a request for help.
    if (!manual && onFoot(player) && people::mix(GetTickCount() * 31) % 5 < 2) {
        unsigned first = people::mix(GetTickCount()) % 4;
        for (unsigned i = 0; i < 4; ++i)
            if (startAmbush(pool, player, 1 + (first + i) % 4)) {
                eventDirector.started(GetTickCount());
                return true;
            }
    }
    unsigned rotation = eventDirector.completed % 7;
    bool offered = false;
    if (drivenCar(player))
        offered = offerLife(pool, player, encounters::Kind::Ride);
    // A scarce, suitable local actor should not lose every turn to generic civilians.
    if (!offered && onFoot(player))
        offered = offerLife(pool, player, encounters::Kind::Visit);
    if (manual && onFoot(player))
        for (auto nearby :
             {encounters::Kind::Visit, encounters::Kind::Beggar, encounters::Kind::Addict})
            if (!offered)
                offered = offerLife(pool, player, nearby);
    if (!offered && rotation >= 3)
        offered = offerLife(pool, player, static_cast<encounters::Kind>(rotation));
    if (!offered && (rotation == 0 || manual) && onFoot(player))
        offered = offerRoadside(pool, player);
    auto kind = rotation == 2 ? encounters::Kind::Fare : encounters::Kind::Directions;
    if (!offered && onFoot(player))
        offered = offerSmallEvent(pool, player, kind);
    if (!offered && onFoot(player))
        offered = offerSmallEvent(pool, player,
                                  kind == encounters::Kind::Fare ? encounters::Kind::Directions
                                                                 : encounters::Kind::Fare);
    for (auto candidate :
         {encounters::Kind::Beggar, encounters::Kind::Addict, encounters::Kind::Visit})
        if (!offered && onFoot(player))
            offered = offerLife(pool, player, candidate);
    if (offered) {
        eventDirector.started(GetTickCount());
        lastManualEvent = GetTickCount();
    } else {
        eventDirector.retry();
        if (manual)
            help("Su an uygun bir sivil yok. Sakin bir sokakta, yayalara yakin F5'i dene.");
    }
    return offered;
}
void robPerson(Pool *pool, Byte *player) {
    DWORD now = GetTickCount();
    if (!focused || !exists(pool, focusedHandle) || bindings.find(focusedHandle) != focused ||
        !due(now, nextSpeech))
        return;
    auto ped = pedAt(pool, focusedHandle.slot);
    releaseTalkingHold(focused);
    auto &f = registry.families[focused - 1];
    if (!eligible(ped, player) || !robbable(ped) || !onFoot(ped) ||
        *reinterpret_cast<float *>(ped + 0x540) <= 0)
        return;
    if (distance2(position(player), position(ped)) > 14.44f || !visibleTarget(ped)) {
        help("Soymak icin kisiye biraz daha yaklas (4 metreden az).");
        nextSpeech = now + 1500;
        return;
    }
    if (f.social.robbed) {
        say(f, "Sana verecek baska bir seyim yok. Uzak dur benden!");
        nextSpeech = now + 2800;
        return;
    }
    if (roadside.person == focused)
        cancelRoadside();
    if (smallEvent.person == focused)
        cancelSmallEvent();
    if (robberyBusy(ped)) {
        help("Bu kisi su an mesgul.");
        nextSpeech = now + 1800;
        return;
    }
    int wallet = std::max(0, static_cast<int>(*reinterpret_cast<short *>(ped + 0x756)));
    auto result = encounters::rob(f,
                                  encounters::threat(currentWeapon(player), currentAmmo(player),
                                                     (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0),
                                  wallet, armed(ped), robbable(ped));
    if (result.outcome == encounters::Outcome::Paid) {
        auto money = playerMoney(player);
        unsigned amount = money ? encounters::transfer(*money, wallet, result.amount) : 0;
        if (amount) {
            *reinterpret_cast<short *>(ped + 0x756) = static_cast<short>(wallet);
            say(f, "Tamam! Al " + std::to_string(amount) + " dolar. Yeter ki zarar verme!");
            help(std::to_string(amount) + " dolar aldin.");
        } else
            say(f, "Ustume gelme! Benden bir sey alamazsin.");
        audio.speak(audioChoices.choose(socialaudio::Context::RobberyPaid, now));
        handsUp(ped);
        delayedFlees.push_back({focusedHandle, now + 4200});
    } else if (result.outcome == encounters::Outcome::Empty) {
        say(f, "Bak, ceplerim bos! Param yok, ne olur birak beni.");
        audio.speak(audioChoices.choose(socialaudio::Context::RobberyRefused, now));
        handsUp(ped);
        delayedFlees.push_back({focusedHandle, now + 4200});
    } else if (result.outcome == encounters::Outcome::Fight) {
        bool fighting = startFight(ped, player, true);
        say(f, fighting ? "Yanlis kisiye bulastin! Simdi goreceksin!"
                        : "Beni rahat birak! Burada durmayacagim!");
        audio.speak(audioChoices.choose(
            fighting ? socialaudio::Context::Fight : socialaudio::Context::RobberyRefused, now));
    } else {
        say(f, result.outcome == encounters::Outcome::Flee ? "Imdat! Uzak dur benden!"
                                                           : "Sana tek kurus yok! Git isine!");
        audio.speak(audioChoices.choose(socialaudio::Context::Provoke, now));
        if (result.outcome == encounters::Outcome::Flee)
            startFlee(ped, player);
        else
            gesture(pool, focusedHandle, 144);
    }
    auto &live = liveTalks[focused];
    live.encounter.ended = true;
    live.lastHandle = focusedHandle;
    live.last = now;
    live.next = now + 60000;
    reputation(-2);
    companionWitness(f.id, socialcontext::Witness::Provoke);
    discoverNetwork(f.id);
    observeDeed(f.id,community::Deed::Robbery);
    later::robbed(registry, f.id);
    nextSpeech = now + 3000;
    dirty = true;
    panelInvalid = true;
    save();
    log("ROBBERY identity=" + std::to_string(f.id) +
        " outcome=" + std::to_string(static_cast<unsigned>(result.outcome)));
}
