// Game-side wiring of Bonds.h: how each district treats CJ, friends' requests and birthdays.
unsigned lastDistrict = 99;
DWORD nextBondTick = 0, nextDistrictAct = 0, nextRequestRoll = 0;
std::uint64_t lastBondDay = ~0ull;
int requestBlip = -1;
unsigned requestBlipFor = 0;
void clearRequestBlip() {
    if (requestBlip != -1)
        reinterpret_cast<void(__cdecl *)(int)>(0x587CE0)(requestBlip);
    requestBlip = -1;
    requestBlipFor = 0;
}
void markRequest(Vec at, unsigned owner) {
    if (requestBlip != -1 && requestBlipFor == owner)
        return;
    clearRequestBlip();
    requestBlip = reinterpret_cast<int(__cdecl *)(int, Vec, unsigned, int, char *)>(0x583820)(
        4, at, 0, 2, nullptr);
    if (requestBlip != -1) {
        reinterpret_cast<void(__cdecl *)(int, unsigned)>(0x583AB0)(requestBlip, 4); // Yellow.
        reinterpret_cast<void(__cdecl *)(int, unsigned char)>(0x583DB0)(requestBlip, 1);
        requestBlipFor = owner;
    }
}
// Phone: Enter on a friend's request message.
void acceptRequest(Byte *player) {
    auto money = playerMoney(player);
    int cash = money ? *money : 0;
    const unsigned amount = registry.request.amount;
    const char *notice = bonds::accept(registry, cash);
    if (!notice) { phoneState.notice = "REQ_UNAVAILABLE"; return; }
    if (money)
        *money = cash;
    phoneState.notice = std::string(notice) == "REQ_LENT" || std::string(notice) == "REQ_NO_MONEY"
                            ? locale::format(notice, std::to_string(amount))
                            : notice;
    dirty = true;
    panelInvalid = true;
    save();
    log(std::string("REQUEST accepted ") + notice);
}
// The phone confirmation is explicit; a normal greeting never spends money.
void giveBirthdayGift(Byte *player) {
    const unsigned id = phoneState.person;
    if (!bonds::canGift(registry, id) || storyActive()) {
        phoneState.notice = "REQ_UNAVAILABLE"; return;
    }
    auto pool = at<Pool *>(0xB74490);
    people::Handle actor{};
    if (!validPool(pool) || !bindings.locate(id, actor) || !exists(pool, actor)) {
        phoneState.notice = "BDAY_NEARBY"; return;
    }
    auto ped = pedAt(pool, actor.slot);
    if (!eligible(ped, player) || !onFoot(player) || !onFoot(ped) ||
        ped[0x2F] != player[0x2F] || distance2(position(player), position(ped)) > 25 ||
        !visibleTarget(ped) || *reinterpret_cast<float *>(ped + 0x540) <= 0) {
        phoneState.notice = "BDAY_NEARBY"; return;
    }
    auto money = playerMoney(player);
    if (!money) { phoneState.notice = "BDAY_NO_MONEY"; return; }
    auto &f = registry.families[id-1];
    if (!bonds::gift(registry, f, *money, true)) {
        phoneState.notice = *money < 50 ? "BDAY_NO_MONEY" : "BDAY_UNAVAILABLE"; return;
    }
    closePhone();
    say(f, "BDAY_GIFT_LINE", npcvoice::Cue::Thank);
    gesture(pool, actor, 12);
    help(locale::format("BDAY_GIFT_GIVEN", dialogue::displayName(f)));
    reputation(1); nextSpeech = GetTickCount() + 3500;
    dirty = panelInvalid = true; save();
}
// Brings the person CJ has to reach into the world when he gets close to where they live.
void summon(Pool *pool, Byte *player, unsigned id, Vec place) {
    people::Handle actor{};
    if ((bindings.locate(id, actor) && exists(pool, actor)) || pending.id ||
        distance2(position(player), place) > 3600)
        return;
    Vec spawn{};
    if (meetingSpawn(place, player, spawn, true))
        requestReturn(id, spawn, false, true);
}
void updateRequest(Pool *pool, Byte *player) {
    auto &q = registry.request;
    if (q.state != 2 || !q.person || q.person > registry.families.size()) {
        clearRequestBlip();
        return;
    }
    if (q.kind == bonds::TalkTo && q.target && q.target <= registry.families.size()) {
        const auto &p = registry.families[q.target - 1].presence;
        Vec place{p.x, p.y, p.z};
        markRequest(place, q.target);
        if (player[0x2F] == 0 && p.located && !p.area)
            summon(pool, player, q.target, place);
        return;
    }
    if (q.kind != bonds::Ride)
        return;
    auto &f = registry.families[q.person - 1];
    const auto &h = f.daily.home;
    Vec home{h.x, h.y, h.z};
    markRequest(home, q.person);
    auto car = drivenCar(player);
    if (!car || lifeActive() || player[0x2F] != 0 || distance2(position(player), home) > 3600)
        return;
    summon(pool, player, q.person, home);
    people::Handle actor{};
    if (!stoppedCar(car) || !bindings.locate(q.person, actor) || !exists(pool, actor) ||
        distance2(position(player), position(pedAt(pool, actor.slot))) > 625)
        return;
    f.social.events &= ~encounters::bit(encounters::Kind::Ride); // Friends ride more than once.
    if (offerLife(pool, player, encounters::Kind::Ride, q.person)) {
        help("REQ_RIDE_BOARD");
        log("REQUEST ride offered identity=" + std::to_string(q.person));
    }
}
void districtLife(Pool *pool, Byte *player) {
    if (player[0x2F] != 0)
        return;
    Vec p = position(player);
    const unsigned d = bonds::district(p.x, p.y);
    const int rep = registry.districts[d];
    if (d != lastDistrict) {
        if (lastDistrict != 99 && (rep >= bonds::Loved || rep <= bonds::Feared))
            help(locale::tr(bonds::districtName(d)) + ": " + locale::tr(bonds::mood(rep)));
        lastDistrict = d;
    }
    DWORD now = GetTickCount();
    if (panelOpen || focusHeld || eventActive() || communityBusy() || companion.person ||
        !due(now, subtitleUntil) || !onFoot(player) || !due(now, nextDistrictAct) || nativeConversation(player) ||
        (rep > bonds::Feared && rep < bonds::Loved))
        return;
    nextDistrictAct = now + 20000 + people::mix(now) % 25000;
    // A district score is a summary, not telepathy. Only a witness or someone who
    // received a bounded social rumor may act on it, and personal friendship still matters.
    people::Handle chosen{}; unsigned identity = 0; float nearest = 225;
    for (unsigned slot=0; slot<static_cast<unsigned>(pool->size); ++slot) {
        if (pool->flags[slot] & 0x80) continue;
        const auto actor = handle(pool, slot);
        const unsigned id = bindings.find(actor);
        if (!id || companionOwns(id) || hasAppointment(id) || communityOwns(id) || recognitionOwns(id)) continue;
        auto ped = pedAt(pool, slot);
        const auto &f = registry.families[id-1];
        const bool informed = bonds::localReaction(f, rep);
        const float distance = distance2(p, position(ped));
        if (!informed || !eligible(ped,player) || !onFoot(ped) || ped[0x2F] ||
            *reinterpret_cast<float *>(ped+0x540)<=0 || !socialIdle(ped) ||
            distance<9 || distance>=nearest || !npcSees(ped,p)) continue;
        chosen=actor; identity=id; nearest=distance;
    }
    if (!identity) return;
    auto ped = pedAt(pool, chosen.slot);
    const auto &f = registry.families[identity-1];
    if (rep >= bonds::Loved) {
        gesture(pool, chosen, 12);
        say(f, "DIST_HELLO_" + std::to_string(people::mix(now + 3) % 4), npcvoice::Cue::HonorHigh);
    } else if (startFlee(ped, player)) {
        say(f, "DIST_FEAR_" + std::to_string(people::mix(now + 5) % 3), npcvoice::Cue::HonorLow);
    }
}
void updateBonds(Pool *pool, Byte *player, bool mission) {
    DWORD now = GetTickCount();
    if (mission || !due(now, nextBondTick))
        return;
    nextBondTick = now + 1000;
    const auto day = registry.worldMinutes / 1440;
    if (day != lastBondDay) {
        if (lastBondDay != ~0ull)
            bonds::fade(registry);
        lastBondDay = day;
        dirty = true;
    }
    if (auto money = playerMoney(player)) {
        const unsigned amount = bonds::repay(registry, *money);
        if (amount) {
            help(locale::format("REQ_REPAID", std::to_string(amount)));
            dirty = panelInvalid = true;
            save();
        }
    }
    const auto previousRequestState = registry.request.state;
    if (bonds::expire(registry)) {
        help("REQ_BROKEN");
        dirty = true;
    }
    if (registry.request.state != previousRequestState) dirty = panelInvalid = true;
    const unsigned hour = at<Byte>(0xB70153);
    if (hour >= 9 && hour < 22)
        for (auto &f : registry.families)
            if (f.social.journal && bonds::invite(registry, f)) {
                help(locale::format("BDAY_TOAST", dialogue::displayName(f)));
                dirty = true;
                panelInvalid = true;
                break;
            }
    if (!nextRequestRoll)
        nextRequestRoll = now + 180000;
    if (!registry.request.state && due(now, nextRequestRoll)) {
        nextRequestRoll = now + 240000;
        auto ids = phone::contacts(registry);
        if (!ids.empty() && people::mix(now) % 3 == 0) {
            unsigned start = people::mix(now + 11) % ids.size();
            for (unsigned i = 0; i < ids.size(); ++i) {
                unsigned id = ids[(start + i) % ids.size()];
                if (hasAppointment(id) || companionOwns(id) ||
                    !bonds::ask(registry, id, people::mix(now + id)))
                    continue;
                help(locale::format("MAIL_FROM", dialogue::displayName(registry.families[id - 1])));
                dirty = true;
                panelInvalid = true;
                log("REQUEST asked identity=" + std::to_string(id) +
                    " kind=" + std::to_string(registry.request.kind));
                break;
            }
        }
    }
    updateRequest(pool, player);
    districtLife(pool, player);
}
