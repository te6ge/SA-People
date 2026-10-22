{
    people::Family nightPerson{1, true, 0, {}, {}};
    nightPerson.presence.alive = true;
    assert(life::nightCandidate(nightPerson, 22, 21, true, 625));
    assert(life::nightCandidate(nightPerson, 22, 4, true, 0));
    assert(!life::nightCandidate(nightPerson, 22, 5, true, 625));
    assert(!life::nightCandidate(nightPerson, 22, 20, true, 625));
    assert(!life::nightCandidate(nightPerson, 22, 23, false, 25));
    assert(!life::nightCandidate(nightPerson, 22, 23, true, 626));
    nightPerson.female = false;
    assert(!life::nightCandidate(nightPerson, 22, 23, true, 25));
    nightPerson.female = true;
    nightPerson.origin = 4;
    assert(!life::nightCandidate(nightPerson, 22, 23, true, 25));
    nightPerson.origin = 0;
    nightPerson.social.events = encounters::bit(encounters::Kind::Visit);
    assert(!life::nightCandidate(nightPerson, 22, 23, true, 25));
    assert(life::untilMorning(21, 0) == 660 && life::untilMorning(4, 30) == 210);
    life::NightRest nightRest;
    int nightCash = 79;
    assert(!nightRest.pay(nightCash) && nightCash == 79 && !nightRest.paid);
    nightCash = 100;
    assert(nightRest.pay(nightCash) && nightCash == 20 && nightRest.paid);
    assert(!nightRest.pay(nightCash) && nightCash == 20);
    std::cout << "PASS: night offer time/25m/adult/gender/vehicle/relative/completion gates, "
                 "morning clock and once-only fee.\n";
    people::Registry canned;
    auto &cannedContact = canned.create(false);
    cannedContact.social.journal = true;
    cannedContact.social.known = people::Name;
    cannedContact.presence = {17, 4, 10, 20, 13, 0, 100, 0, true, true};
    cannedContact.social.trust = 60;
    assert(std::string(phonedialogue::send(canned, 1, 0, false, 1200)).empty());
    assert(canned.letters.size() == 2 && canned.letters[0].outgoing);
    assert(std::string(phonedialogue::send(canned, 1, 4, false, 1200)) == "TXT_WAIT");
    assert(cannedContact.social.anger == 0 && canned.letters.size() == 2);
    canned.worldMinutes = 40;
    assert(std::string(phonedialogue::send(canned, 1, 4, false, 1200)).empty());
    assert(cannedContact.social.anger == 12 && cannedContact.social.trust == 54);
    canned.worldMinutes = 80;
    assert(std::string(phonedialogue::send(canned, 1, 3, false, 1200)).empty());
    assert(cannedContact.social.anger < 12 && cannedContact.social.trust == 54);
    canned.worldMinutes = 120;
    cannedContact.social.helped = 1;
    assert(std::string(phonedialogue::send(canned, 1, 2, true, 1200)).empty());
    assert(canned.letters.back().body == "MEM_BATTERY" && canned.letters.back().call);
    canned.worldMinutes = 160;
    cannedContact.daily.busyUntil = 200;
    auto count = canned.letters.size();
    assert(std::string(phonedialogue::send(canned, 1, 0, true, 1200)) == "MAIL_CALL_BUSY");
    assert(canned.letters.size() == count);
    cannedContact.social.cutOff = 1;
    assert(std::string(phonedialogue::send(canned, 1, 0, false, 1200)) == "MAIL_BLOCKED");
    assert(std::string(phonedialogue::send(canned, 0, 0, false, 1200)) == "PHONE_UNAVAILABLE");
    std::stringstream cannedSave;
    canned.write(cannedSave);
    auto reloadedCanned = people::Registry::read(cannedSave);
    assert(reloadedCanned.letters.size() == count);
    std::cout << "PASS: canned phone dialogue, persisted spam budget, insult/apology effects, "
                 "memory replies and busy calls.\n";
    people::Registry socialBook;
    auto &resident = socialBook.create(false);
    resident.presence = {17, 4, 10, 20, 13, 0, 100, 0, true, true};
    resident.social.known = people::Name;
    resident.social.journal = true;
    resident.social.trust = 60;
    resident.daily.ready = true;
    resident.daily.home = {10, 20, 13};
    resident.daily.work = {40, 50, 13};
    resident.daily.leisure = {60, 70, 13};
    assert(socialworld::phase(7 * 60 + 59) == 0 && socialworld::phase(8 * 60) == 1);
    assert(socialworld::phase(17 * 60) == 2 && socialworld::phase(22 * 60) == 0);
    assert(socialworld::destination(resident, 9 * 60).x == 40);
    assert(socialworld::destination(resident, 18 * 60).x == 60);
    assert(socialworld::destination(resident, 23 * 60).x == 10);
    auto original = people::generate(resident)[0].name;
    assert(socialworld::spontaneous(socialBook, resident, 10 * 60));
    assert(!socialworld::spontaneous(socialBook, resident, 11 * 60));
    socialBook.worldMinutes += 1440;
    assert(socialworld::spontaneous(socialBook, resident, 10 * 60));
    socialBook.post(1, u8"Tırnak: \"CJ\" — Привет, ¿qué tal?", true);
    std::stringstream savedSocial;
    socialBook.write(savedSocial);
    auto loadedSocial = people::Registry::read(savedSocial);
    assert(loadedSocial.letters.size() == 3 &&
           loadedSocial.letters.back().body == socialBook.letters.back().body);
    assert(loadedSocial.families[0].daily.work.x == 40 && loadedSocial.worldMinutes == 1440);
    assert(people::generate(loadedSocial.families[0])[0].name == original);
    assert(socialworld::unread(loadedSocial) == 2);
    for (unsigned i = 0; i < 300; ++i)
        socialBook.post(1, "message " + std::to_string(i), i % 2 == 0);
    assert(socialBook.letters.size() == 256 &&
           socialBook.letters.front().id < socialBook.letters.back().id);
    assert(socialBook.post(99, "bad") == 0 && socialBook.post(1, "") == 0);
    people::Registry pinnedMail;
    pinnedMail.create(false);
    pinnedMail.post(1, "Remember this person");
    assert(!pinnedMail.recycle([](unsigned) { return false; }));
    assert(people::cleanText("a\nb\tc") == "a b c");
    assert(people::cleanText(u8"ççç", 3) == u8"ç");
    auto &nearby = socialBook.create(false);
    nearby.presence = {17, 4, 14, 22, 13, 0, 100, 0, true, true};
    nearby.social.journal = true;
    nearby.social.known = people::Name;
    nearby.social.trust = 50;
    assert(network::link(socialBook, 1, 2, 0));
    int beforeTrust = resident.social.trust;
    assert(!socialworld::remoteEffect(socialBook, 1, socialworld::Intent::Neutral));
    assert(!socialworld::remoteEffect(socialBook, 1, socialworld::Intent::Friendly));
    assert(resident.social.trust == beforeTrust);
    assert(socialworld::remoteEffect(socialBook, 1, socialworld::Intent::Insult));
    assert(resident.social.trust == beforeTrust - 6 && resident.social.provokes == 1);
    assert(nearby.social.trust < 50 && nearby.social.newsFrom == 1);
    assert(socialBook.letters.back().person == 2 && socialBook.letters.back().source == 1);
    assert(!socialworld::remoteEffect(socialBook, 1, socialworld::Intent::Insult));
    socialBook.worldMinutes += 32;
    assert(socialworld::remoteEffect(socialBook, 1, socialworld::Intent::Threat));
    socialBook.worldMinutes += 32;
    assert(socialworld::remoteEffect(socialBook, 1, socialworld::Intent::Insult));
    assert(resident.social.cutOff && !socialworld::canHangOut(resident));
    people::Registry outingBook;
    auto &buddy = outingBook.create(false);
    buddy.presence = {17, 4, 0, 0, 13, 0, 100, 0, true, true};
    buddy.social.known = people::Name;
    buddy.social.journal = true;
    buddy.social.trust = 60;
    assert(meetings::schedule(outingBook, 1, 10, 10, 13));
    auto &trip = outingBook.appointments[0];
    trip.state = 2;
    trip.activity = 2;
    assert(socialworld::complete(outingBook, trip, buddy, true, false));
    assert(buddy.daily.outings == 1 && trip.state == 3 && outingBook.letters.size() == 1);
    assert(!socialworld::complete(outingBook, trip, buddy, true, false) &&
           buddy.daily.outings == 1);
    assert(meetings::schedule(outingBook, 1, 10, 10, 13));
    trip.state = 2;
    assert(socialworld::complete(outingBook, trip, buddy, false, false) &&
           buddy.daily.outings == 1 && trip.state == 4);
    auto poolVenue = meetings::activityVenue(-2200, 200, 1, 1);
    assert(poolVenue && std::string(poolVenue->name) == "VENUE_BOTTLES");
    for (unsigned language = 0; language < 4; ++language) {
        locale::language = static_cast<locale::Language>(language);
        for (auto page : {phone::Page::Inbox, phone::Page::Messages, phone::Page::Compose,
                          phone::Page::Actions, phone::Page::Activities, phone::Page::Outing}) {
            phone::State state;
            state.page = page;
            state.person = 1;
            auto scene = phone::compose(outingBook, state, "14:30");
            for (const auto &line : scene.texts)
                assert(line.x >= 0 && line.y >= 0 && line.x + line.width <= 332 && line.y <= 650);
        }
    }
    locale::language = locale::Language::Turkish;
    std::stringstream outingSave;
    outingBook.write(outingSave);
    auto outingReload = people::Registry::read(outingSave);
    assert(outingReload.families[0].daily.outings == 1 &&
           outingReload.appointments[0].activity == 0);
    std::cout
        << "PASS: V11 routine anchors and daily phases, bounded UTF-8 mailbox, mail identity "
           "retention, "
           "invitation budget, remote intent cooldown and no praise farming, network consequences, "
           "once-only activity completion, native pool venue and four-language phone scenes.\n";
}
