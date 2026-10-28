#include "World.h"
#include "Panel.h"
#include "AudioPolicy.h"
#include "Encounters.h"
#include "Phone.h"
#include "ModelTraits.h"
#include "ModernMenu.h"
#include "Life.h"
#include "Vitals.h"
#include "Later.h"
#include "Favors.h"
#include "MeetingRuntime.h"
#include "SocialContext.h"
#include "NativeTalk.h"
#include "InstallPaths.h"
#include "SetupText.h"
#include "Neighborhood.h"
#include "GangCombat.h"
#include "NpcVoices.h"
#include "Community.h"
#include <filesystem>
#include <chrono>
#include <iostream>
#include <set>
#include <cassert>
int main() {
    for (unsigned language = 0; language < 4; ++language) {
        assert(setuptext::names[language][0]);
        for (unsigned key = 0; key < setuptext::Count; ++key)
            assert(setuptext::get(static_cast<setuptext::Language>(language),
                                 static_cast<setuptext::Key>(key))[0]);
    }
    assert(std::wstring(setuptext::get(setuptext::Language::Russian, setuptext::Install)) ==
           L"Установить SA People");
    assert(std::wstring(setuptext::get(setuptext::Language::Spanish, setuptext::Missing)).find(
           L"gta_sa.exe") != std::wstring::npos);
    std::cout << "PASS: complete setup catalog in four languages, including Cyrillic.\n";
    // Recordings are optional and not shipped; skip file checks when the folder is empty.
    const bool audioAvailable = std::filesystem::exists("audio/high.mp3");
    std::cout << std::unitbuf;
    {
        people::ModelTraits traits;
        std::istringstream ide("9, BFORI, BFORI, CIVFEMALE, STAT_COWARD, woman\n"
                               "63, WFYPRO, WFYPRO, PROSTITUTE, STAT_STREET_GIRL, sexywoman\n"
                               "105, FAM1, FAM1, GANG2, STAT_GANG1, gang1\n"
                               "190, test, test, CRIMINAL, STAT_COWARD, fatwoman\n");
        traits.read(ide);
        assert(traits.female(9, false) && traits.female(63, false) && traits.female(190, false));
        assert(!traits.female(105, true) && traits.female(311, true));
        people::Registry contacts;
        auto &contact = contacts.create(false);
        contact.social.known = people::Name;
        contact.presence = {105, 7, 10, 20, 3, 0, 100, 0, true, true};
        auto appearance = contact.presence;
        people::Bindings links;
        assert(people::returningContact(contacts, links, appearance) == 1);
        links.bind({0, 1, 105}, 1);
        assert(!people::returningContact(contacts, links, appearance));
        links.clear();
        std::vector<people::Presence> lookalikes{appearance, appearance};
        assert(!people::returningContact(contacts, links, appearance, &lookalikes));
        contacts.families[0].presence.alive = false;
        assert(!people::returningContact(contacts, links, appearance));
        contacts.families[0].presence.alive = true;
        appearance.area = 1;
        assert(!people::returningContact(contacts, links, appearance));
        appearance = contacts.families[0].presence;
        contacts.create(false).presence = appearance;
        contacts.families[1].social.known = people::Name;
        assert(!people::returningContact(contacts, links, appearance));
        for (auto dimensions :
             {std::pair<float, float>{640.0f, 480.0f}, {1280.0f, 720.0f}, {3440.0f, 1440.0f}}) {
            auto scene = frontend::background(dimensions.first, dimensions.second);
            for (const auto &r : scene.rectangles)
                assert(r.x >= 0 && r.y >= 0 && r.x + r.w <= dimensions.first + 0.01f &&
                       r.y + r.h <= dimensions.second + 0.01f);
        }
        auto scene = panel::roaming(&contacts.families[0], false, "", "", false, false, 0, 1280,
                                    720, panel::Hints{});
        for (const auto &text : scene.texts)
            assert(text.value != "ALT");
        std::cout << "PASS: model metadata; unique contact reattachment; lookalike ambiguity; "
                     "modern menu bounds; focus badge.\n";
    }
    {
        std::ifstream file("languages.tsv", std::ios::binary);
        locale::catalog.read(file);
    }
    street::Reply soundReply;
    soundReply.reaction = dialogue::Reaction::Friendly;
    assert(socialaudio::honorCue(street::Action::Greet, 0, 7, soundReply) == 0);
    assert(socialaudio::honorCue(street::Action::Provoke, 7, -9, soundReply) == 0);
    soundReply.newJournal = true;
    assert(socialaudio::honorCue(street::Action::Greet, 7, 20, soundReply) == 1);
    for (auto action : {street::Action::Greet, street::Action::Provoke})
        for (unsigned turn = 1; turn <= 5; ++turn)
            for (unsigned v = 0; v < 12; ++v)
                for (bool leaving : {false, true}) {
                    soundReply.leaves = leaving;
                    assert(!audioAvailable || std::filesystem::exists(
                        audiofiles::resolve(std::filesystem::path("audio") /
                        socialaudio::line(action, turn, v % 2 != 0, soundReply, v))));
                }
    assert(!audioAvailable || std::filesystem::exists("audio/high.mp3") && std::filesystem::exists("audio/low.mp3") &&
           std::filesystem::exists(audiofiles::resolve("audio/cjWeClean.wav")));
    std::cout << "PASS: contextual audio files; no honor farming from greetings; positive "
                 "milestones and negative provocation cues.\n";
    people::Registry r;
    for (unsigned i = 0; i < people::MaxFamilies; ++i) {
        auto &f = r.create(i % 2 != 0);
        f.meetings = i;
        auto p = people::generate(f);
        std::set<unsigned> ids;
        for (const auto &person : p) {
            assert(ids.insert(person.id).second);
            if (person.mother) {
                const auto &mother = p[person.mother - f.id * 8];
                assert(mother.age >= person.age + 18);
                bool found = false;
                for (auto child : mother.children)
                    if (child == person.id)
                        found = true;
                assert(found);
            }
            if (person.father)
                assert(p[person.father - f.id * 8].age >= person.age + 18);
            if (person.spouse)
                assert(p[person.spouse - f.id * 8].spouse == person.id);
            for (auto sibling : person.siblings) {
                const auto &other = p[sibling - f.id * 8];
                assert(other.siblings.size() == 1 && other.siblings[0] == person.id);
                assert(other.mother == person.mother && other.father == person.father);
            }
        }
        for (unsigned page = 0; page < 3; ++page)
            assert(people::page(f, page).size() < 400);
    }
    std::stringstream buffer;
    r.write(buffer);
    auto copy = people::Registry::read(buffer);
    assert(copy.families.size() == r.families.size());
    for (unsigned i = 0; i < r.families.size(); ++i)
        assert(people::page(r.families[i], 0) == people::page(copy.families[i], 0));
    for (auto bad :
         {"", "SAPEOPLE 7 0", "SAPEOPLE 1 32769", "SAPEOPLE 1 1 2 0 1", "SAPEOPLE 1 1 1 3 1",
          "SAPEOPLE 1 0 garbage", "SAPEOPLE 2 1 1 0 1 7 4 999999 0 0 0 100 0 1 1",
          "SAPEOPLE 2 1 1 0 1 7 4 0 0 0 0 -1 0 1 1", "SAPEOPLE 2 1 1 0 1 7 4 0 0 0 0 100 0 2 1"}) {
        bool rejected = false;
        try {
            std::stringstream b(bad);
            people::Registry::read(b);
        } catch (...) {
            rejected = true;
        }
        assert(rejected);
    }
    bool full = false;
    try {
        r.create(false);
    } catch (...) {
        full = true;
    }
    assert(full);
    std::stringstream legacy("SAPEOPLE 1\n2\n1 0 5\n2 1 7\n");
    auto migrated = people::Registry::read(legacy);
    assert(migrated.families[1].meetings == 7 && !migrated.families[1].presence.located);
    people::Bindings links;
    people::Handle a{4, 1, 7}, b{5, 2, 7}, recycled{4, 3, 7};
    assert(links.bind(a, 1) && links.bind(b, 2)); // Same face is NOT the same identity.
    assert(links.find(a) == 1 && links.find(b) == 2);
    assert(!links.bind({6, 2, 7}, 1)); // One identity cannot occupy two actors.
    assert(links.find(recycled) == 0);
    links.prune([&](people::Handle h) { return h == b; });
    assert(!links.active(1) && links.active(2));
    assert(links.bind(recycled, 3) && links.find(recycled) == 3);
    links.clear();
    assert(!links.active(2) && !links.active(3));
    auto &f = migrated.families[0];
    f.presence = {7, 4, 100, 100, 10, 1.25f, 80, 0, true, true};
    assert(people::canReturn(f, links, 50, 100, 10, 0));
    assert(!people::canReturn(f, links, 100, 100, 10, 0));
    assert(!people::canReturn(f, links, 50, 100, 10, 1));
    assert(!people::canReturn(f, links, 50, 100, 30, 0));
    links.bind(a, 1);
    assert(!people::canReturn(f, links, 50, 100, 10, 0));
    links.clear();
    people::SpatialIndex index;
    index.update(f);
    assert(index.nearby(100, 100).size() == 1);
    // Move from Los Santos to San Fierro: identity, model and family remain fixed.
    auto oldName = people::generate(f)[0].name;
    f.presence.x = -2000;
    f.presence.y = 500;
    index.update(f);
    assert(index.nearby(100, 100).empty() && index.nearby(-2000, 500).size() == 1);
    assert(people::generate(f)[0].name == oldName && f.id == 1 && f.presence.model == 7);
    f.presence.alive = false;
    assert(!people::canReturn(f, links, -2050, 500, 10, 0));
    std::stringstream world;
    migrated.write(world);
    auto restored = people::Registry::read(world);
    assert(restored.families[0].presence.x == -2000 && !restored.families[0].presence.alive);
    assert(restored.families[0].presence.heading == 1.25f &&
           restored.families[0].presence.model == 7);
    // Check geometry at 4:3, widescreen, ultrawide and the supported preference extremes.
    for (auto dimensions : {std::pair<float, float>{640.0f, 480.0f},
                            {800.0f, 600.0f},
                            {1280.0f, 720.0f},
                            {1920.0f, 1080.0f},
                            {3440.0f, 1440.0f}}) {
        for (float preference : {0.75f, 1.0f, 1.25f}) {
            auto l = panel::layout(dimensions.first, dimensions.second, preference);
            assert(l.x >= 0 && l.y >= 0);
            assert(l.x + (panel::Width + 5) * l.scale <= dimensions.first);
            assert(l.y + (panel::Height + 7) * l.scale <= dimensions.second);
        }
    }
    for (unsigned tab = 0; tab < 3; ++tab) {
        auto scene = panel::compose(&f, tab, false, 2);
        for (const auto &t : scene.texts) {
            assert(t.x >= 0 && t.y >= 0 && t.x + t.width <= panel::Width);
            assert(t.y + 40 < panel::Height && t.value.find('~') == std::string::npos);
        }
    }
    auto contains = [](const panel::Scene &scene, const std::string &value) {
        for (const auto &text : scene.texts)
            if (text.value.find(value) != std::string::npos)
                return true;
        return false;
    };
    // All notebook tabs must redact unknown personal information, including family names.
    people::Family stranger{10, false, 0, {}, {}};
    auto people = people::generate(stranger);
    for (unsigned tab = 0; tab < 3; ++tab) {
        auto scene = panel::compose(&stranger, tab, true, 999);
        for (const auto &person : people)
            assert(!contains(scene, person.name));
        assert(!contains(scene, people[0].job));
    }
    unsigned outcomes[5] = {};
    for (unsigned id = 1; id <= 256; ++id)
        for (unsigned choice = 0; choice < 3; ++choice) {
            people::Family npc{id, false, 0, {}, {}};
            dialogue::Session session;
            session.id = id;
            dialogue::apply(npc, session, choice, true);
            ++outcomes[static_cast<unsigned>(session.reaction)];
            assert(people::valid(npc.social));
            if (session.reaction == dialogue::Reaction::Friendly) {
                assert(npc.social.known == people::Name && npc.meetings == 1);
            } else
                assert(npc.social.known == 0 && npc.meetings == 0);
        }
    assert(outcomes[1] && outcomes[2] && outcomes[3] && outcomes[4]);
    // An armed/non-civilian actor may be angry, but this mod must not force combat.
    for (unsigned id = 1; id <= 256; ++id) {
        people::Family npc{id, false, 0, {}, {}};
        dialogue::Session session;
        session.id = id;
        dialogue::apply(npc, session, 2, false);
        assert(session.reaction != dialogue::Reaction::Fight);
        if (session.stage == dialogue::Stage::Tension) {
            dialogue::apply(npc, session, 2, false);
            assert(session.reaction != dialogue::Reaction::Fight);
        }
    }
    people::Family friendNpc{1, false, 1, {}, {people::Name, 30, 0, 1, 1}};
    dialogue::Session talk;
    talk.id = 1;
    talk.stage = dialogue::Stage::Topics;
    dialogue::apply(friendNpc, talk, 1, true);
    unsigned learned = friendNpc.social.known;
    assert(learned == (people::Name | people::Child));
    int trustAfterFirst = friendNpc.social.trust;
    dialogue::apply(friendNpc, talk, 1, true);
    assert(friendNpc.social.known == learned &&
           friendNpc.social.trust == trustAfterFirst); // No topic farming.
    auto notebook = panel::compose(&friendNpc, 1, true, 1);
    auto familyPeople = people::generate(friendNpc);
    assert(contains(notebook, familyPeople[5].name));
    for (unsigned member = 1; member < 5; ++member)
        assert(!contains(notebook, familyPeople[member].name));
    friendNpc.social.anger = 60;
    talk.stage = dialogue::Stage::Tension;
    dialogue::apply(friendNpc, talk, 0, true);
    assert(friendNpc.social.anger == 35 && talk.stage == dialogue::Stage::Done);
    people::Registry socialRegistry;
    socialRegistry.families.push_back(friendNpc);
    std::stringstream socialSave;
    socialRegistry.write(socialSave);
    auto socialLoad = people::Registry::read(socialSave);
    assert(socialLoad.families[0].social.known == learned &&
           socialLoad.families[0].social.anger == 35);
    for (auto bad : {"SAPEOPLE 3 1 1 0 0 -1 -1 0 0 0 0 100 0 0 1 512 0 0 0 0",
                     "SAPEOPLE 3 1 1 0 0 -1 -1 0 0 0 0 100 0 0 1 0 -101 0 0 0"}) {
        bool rejected = false;
        try {
            std::stringstream stream(bad);
            people::Registry::read(stream);
        } catch (...) {
            rejected = true;
        }
        assert(rejected);
    }
    dialogue::Session hidden;
    hidden.id = stranger.id;
    auto intro = panel::conversation(stranger, hidden);
    for (const auto &person : people)
        assert(!contains(intro, person.name));
    assert(contains(intro, "Selam, ben CJ.") && contains(intro, "Bir sey sorabilir") &&
           contains(intro, "Hey!"));
    for (auto stage : {dialogue::Stage::Introduction, dialogue::Stage::Topics,
                       dialogue::Stage::Tension, dialogue::Stage::Done}) {
        hidden.stage = stage;
        auto scene = panel::conversation(stranger, hidden);
        for (const auto &t : scene.texts)
            assert(t.x >= 0 && t.y >= 0 && t.x + t.width <= panel::Width &&
                   t.y + 40 < panel::Height);
    }
    std::cout << "PASS: RP choices and all four reactions; progressive knowledge; notebook "
                 "privacy; apology; no forced armed combat; v3 social persistence.\n";
    unsigned journalCount = 0, friends = 0;
    std::set<std::string> exits;
    for (unsigned id = 1; id <= 256; ++id) {
        people::Family npc{id, false, 0, {}, {}};
        street::Encounter first;
        first.id = id;
        street::exchange(npc, first, street::Action::Greet, true);
        assert(!npc.social.journal &&
               !(npc.social.known & people::Name)); // A passing hello is not a contact.
        for (unsigned meeting = 0; meeting < 14; ++meeting) {
            street::Encounter e;
            e.id = id;
            for (unsigned turn = 0; turn < 9 && !e.ended; ++turn) {
                auto reply = street::exchange(npc, e, street::Action::Greet, true);
                if (reply.leaves)
                    exits.insert(reply.line);
                assert(people::valid(npc.social));
                if (street::isFriend(npc))
                    assert(npc.social.encounters >= 3 && npc.social.meaningful >= 8);
            }
        }
        if (npc.social.journal)
            ++journalCount;
        if (street::isFriend(npc))
            ++friends;
    }
    assert(journalCount > 100 && friends > 20 && exits.size() >= 12);
    std::vector<street::Candidate> targets = {{1, 16, 1, true}, {2, 18, 1, true}};
    assert(street::focus(targets, 0) == 1);
    targets[1].distanceSquared = 15;
    assert(street::focus(targets, 1) == 1); // No flicker for tiny changes.
    targets[1].distanceSquared = 4;
    assert(street::focus(targets, 1) == 2); // Walking closer changes focus.
    targets[1].visible = false;
    assert(street::focus(targets, 2) == 1);
    targets[0].distanceSquared = 65;
    assert(street::focus(targets, 0) == 0); // Out of range / occluded.
    street::Repair repair;
    for (unsigned step = 0; step < 20; ++step)
        assert(!street::repairStep(repair, 100, true, true));
    assert(repair.progress == 2000);
    street::repairStep(repair, 100, false, true);
    assert(repair.progress == 0);
    for (unsigned step = 0; step < 39; ++step)
        assert(!street::repairStep(repair, 100, true, true));
    assert(street::repairStep(repair, 100, true, true) && repair.complete);
    assert(!street::repairStep(repair, 100, true, true)); // Reward cannot repeat.
    people::Family helped{1, false, 0, {}, {}};
    auto thanks = street::batteryHelp(helped);
    assert(helped.social.journal && helped.social.helped == 1 && thanks.newJournal);
    int helpedTrust = helped.social.trust;
    street::batteryHelp(helped);
    assert(helped.social.trust == helpedTrust);
    people::Registry savedStreet;
    savedStreet.families.push_back(helped);
    std::stringstream streetFile;
    savedStreet.write(streetFile);
    auto loadedStreet = people::Registry::read(streetFile);
    assert(loadedStreet.families[0].social.helped == 1 && loadedStreet.families[0].social.journal);
    for (auto dims :
         {std::pair<float, float>{640.0f, 480.0f}, {1280.0f, 720.0f}, {3440.0f, 1440.0f}}) {
        auto hud =
            panel::roaming(&helped, true, "Yabanci", "Arabanin akusu bitti! Bir el atabilir misin?",
                           true, true, 2000, dims.first, dims.second);
        for (const auto &box : hud.rectangles)
            assert(box.x >= 0 && box.y >= 0 && box.x + box.w <= dims.first &&
                   box.y + box.h <= dims.second);
        for (const auto &t : hud.texts)
            assert(t.x >= 0 && t.y >= 0 && t.x + t.width <= dims.first && t.y + 20 < dims.second);
    }
    std::stringstream oldContact("SAPEOPLE 3 1 1 0 1 -1 -1 0 0 0 0 100 0 0 1 1 12 0 1 1");
    auto oldHello = people::Registry::read(oldContact);
    assert(!oldHello.families[0].social.journal);
    std::stringstream oldConversation("SAPEOPLE 3 1 1 0 1 -1 -1 0 0 0 0 100 0 0 1 3 18 0 1 1");
    auto oldTalk = people::Registry::read(oldConversation);
    assert(oldTalk.families[0].social.journal);
    std::cout << "PASS: walking greetings; varied departures; earned journals; friendship across "
                 "encounters; focus hysteresis/occlusion; 4-second repair; one-time help reward; "
                 "v4 migration.\n";
    socialaudio::Chooser chooser;
    std::set<std::wstring> allClips;
    for (unsigned context = 0; context < static_cast<unsigned>(socialaudio::Context::Count);
         ++context) {
        auto type = static_cast<socialaudio::Context>(context);
        auto list = socialaudio::clips(type);
        std::set<std::wstring> cycle;
        std::wstring previous;
        for (unsigned i = 0; i < list.size() * 3; ++i) {
            std::wstring clip = chooser.choose(type, 719 + i);
            assert(!audioAvailable || std::filesystem::exists(audiofiles::resolve(std::filesystem::path("audio") / clip)));
            if (list.size() > 1)
                assert(clip != previous);
            if (i < list.size())
                assert(cycle.insert(clip).second);
            previous = clip;
            allClips.insert(clip);
        }
    }
    assert(allClips.size() == 73);
    for (unsigned lang = 0; lang < 3; ++lang)
        assert(!audioAvailable || std::filesystem::exists(audiofiles::resolve(std::filesystem::path("audio") / socialaudio::localizedInsult(lang))));
    assert(!audioAvailable || std::filesystem::exists(audiofiles::resolve(std::filesystem::path("audio") / socialaudio::rickroll())));
    unsigned fistPaid = 0, gunPaid = 0, armedFights = 0, unarmedFights = 0;
    for (unsigned id = 1; id <= 1000; ++id) {
        people::Family weak{id, false, 0, {}, {}}, strong = weak, armedNpc = weak;
        auto weakResult = encounters::rob(weak, encounters::threat(0, 0, false), 120, false, true);
        auto strongResult =
            encounters::rob(strong, encounters::threat(24, 50, true), 120, false, true);
        auto armedResult =
            encounters::rob(armedNpc, encounters::threat(24, 50, true), 120, true, true);
        if (weakResult.outcome == encounters::Outcome::Paid)
            ++fistPaid;
        if (strongResult.outcome == encounters::Outcome::Paid)
            ++gunPaid;
        if (armedResult.outcome == encounters::Outcome::Fight)
            ++armedFights;
        if (strongResult.outcome == encounters::Outcome::Fight)
            ++unarmedFights;
        assert(strongResult.amount <= 120 && strong.social.trust == -45 &&
               strong.social.robbed == 1);
        assert(encounters::rob(strong, 95, 500, false, true).outcome ==
               encounters::Outcome::Already);
        assert(!strong.social.journal && strong.social.known == 0);
        people::Family empty{id, false, 0, {}, {}};
        assert(encounters::rob(empty, 95, 0, false, true).amount == 0);
    }
    assert(gunPaid > fistPaid + 200 && armedFights > unarmedFights);
    assert(encounters::threat(24, 50, true) > encounters::threat(24, 0, true));
    assert(encounters::threat(4, 0, false) > encounters::threat(0, 0, false));
    int cash = 100, wallet = 50;
    assert(encounters::transfer(cash, wallet, 100) == 50 && cash == 150 && wallet == 0);
    assert(encounters::transfer(cash, wallet, 100) == 0);
    cash = 999999990;
    wallet = 50;
    assert(encounters::transfer(cash, wallet, 50) == 9 && cash == 999999999 && wallet == 41);
    cash = 10;
    wallet = 5;
    assert(!encounters::donate(cash, wallet, 20) && cash == 10 && wallet == 5);
    cash = 100;
    assert(encounters::donate(cash, wallet, 20) && cash == 80 && wallet == 25);
    wallet = 32760;
    assert(!encounters::donate(cash, wallet, 20) && cash == 80);
    people::Family eventPerson{17, false, 0, {}, {}};
    assert(encounters::complete(eventPerson, encounters::Kind::Directions));
    int trust = eventPerson.social.trust;
    assert(!encounters::complete(eventPerson, encounters::Kind::Directions) &&
           eventPerson.social.trust == trust);
    assert(encounters::complete(eventPerson, encounters::Kind::Fare));
    eventPerson.social.robbed = 1;
    people::Registry eventRegistry;
    eventPerson.id = 1;
    eventRegistry.families.push_back(eventPerson);
    std::stringstream eventArchive;
    eventRegistry.write(eventArchive);
    auto savedEvent = people::Registry::read(eventArchive);
    assert(savedEvent.families[0].social.events == 6 && savedEvent.families[0].social.robbed == 1);
    std::stringstream previousVersion(
        "SAPEOPLE 4 1 1 0 0 -1 -1 0 0 0 0 100 0 0 1 1 20 0 0 1 3 1 1 4 1");
    auto v4 = people::Registry::read(previousVersion);
    assert(v4.families[0].social.events == 1 && v4.families[0].social.robbed == 0);
    encounters::Director director;
    for (unsigned i = 0; i < 180; ++i)
        director.step(250, false);
    assert(director.clock == 0 && !director.due());
    for (unsigned i = 0; i < 180; ++i)
        director.step(250, true);
    assert(director.due());
    director.retry();
    assert(!director.due());
    for (unsigned i = 0; i < 32; ++i)
        director.step(250, true);
    assert(director.due());
    director.started(17);
    auto wait = director.next - director.clock;
    assert(wait >= 55000 && wait <= 110000 && !director.due());
    people::Family tense{3, false, 0, {}, {}};
    tense.social.anger = 70;
    tense.social.trust = -30;
    street::Encounter argument{3};
    argument.ended = true;
    auto calm = street::exchange(tense, argument, street::Action::Defuse, true);
    assert(tense.social.anger == 46 && tense.social.trust == -30 &&
           calm.cue == street::Cue::Apology);
    std::cout << "PASS: all 73 CJ clips; shuffle bags; weapon-dependent robbery; real wallet "
                 "bounds; no duplicate payout; donations; once-only encounters; v4/v5 migration; "
                 "event cooldown and pause; defusing.\n";
    for (unsigned id = 1; id <= 512; ++id) {
        street::Encounter blocked{id};
        street::rejectBusy(blocked);
        assert(!street::mayGreet(blocked) && !street::farewell(blocked));
        street::Encounter rejectedVisit{id};
        rejectedVisit.refused = true;
        rejectedVisit.ended = true;
        assert(street::mayGreet(rejectedVisit) && street::farewell(rejectedVisit));
        for (unsigned attempt = 0; attempt < 10; ++attempt)
            assert(!street::mayGreet(rejectedVisit) && !street::farewell(rejectedVisit));
        people::Family insulted{id, false, 0, {}, {}};
        street::Encounter conflict{id};
        street::exchange(insulted, conflict, street::Action::Provoke, false);
        auto firstApology = street::exchange(insulted, conflict, street::Action::Defuse, false);
        assert(firstApology.calmed && conflict.apologyUsed &&
               insulted.social.anger >= insulted.social.resentment * 3 / 5);
        street::exchange(insulted, conflict, street::Action::Provoke, false);
        assert(conflict.brokenTruce);
        unsigned angerBefore = insulted.social.anger, grudgeBefore = insulted.social.resentment;
        for (unsigned attempt = 0; attempt < 10; ++attempt) {
            auto rejectedApology =
                street::exchange(insulted, conflict, street::Action::Defuse, false);
            assert(!rejectedApology.calmed && insulted.social.anger == angerBefore &&
                   insulted.social.resentment == grudgeBefore);
        }
        assert(!street::mayGreet(conflict) && insulted.social.trust < 0);
    }
    people::Registry appointmentRegistry;
    auto &contact = appointmentRegistry.create(false);
    contact.presence = {7, 4, 10, 20, 3, 0, 100, 0, true, true};
    contact.social.known = people::Name;
    contact.social.journal = true;
    contact.social.trust = 40;
    assert(meetings::schedule(appointmentRegistry, 1, 10, 20, 3));
    assert(!meetings::schedule(appointmentRegistry, 1, 40, 50, 3));
    auto &planned = appointmentRegistry.appointments[0];
    for (unsigned i = 0; i < 240; ++i)
        assert(!meetings::step(planned, 250, false));
    assert(planned.remaining == 60000);
    for (unsigned i = 0; i < 239; ++i)
        assert(!meetings::step(planned, 250, true));
    assert(meetings::step(planned, 250, true) && planned.state == 2);
    assert(!meetings::arrive(planned, contact, false));
    assert(meetings::arrive(planned, contact, true) && contact.social.lastNote == 10);
    assert(!meetings::arrive(planned, contact, true));
    contact.social.resentment = 37;
    std::stringstream appointmentFile;
    appointmentRegistry.write(appointmentFile);
    auto restoredAppointment = people::Registry::read(appointmentFile);
    assert(restoredAppointment.appointments[0].person == 1 &&
           restoredAppointment.appointments[0].state == 3 &&
           restoredAppointment.families[0].social.resentment == 37);
    assert(!meetings::canInvite(contact));
    people::Appointment missed{1, 2, 0, 250, 1, 2, 3};
    assert(meetings::step(missed, 250, true) && missed.state == 4);
    assert(locale::catalog.size() > 180);
    for (unsigned lang = 0; lang < 4; ++lang) {
        locale::language = static_cast<locale::Language>(lang);
        auto translated = locale::tr("Bu arada, ben Miguel Garcia. Memnun oldum CJ.");
        assert(translated.find("Miguel Garcia") != std::string::npos &&
               translated.find("{0}") == std::string::npos);
        assert(locale::tr("PHONE_CONTACTS") != "PHONE_CONTACTS");
        auto scene = phone::compose(appointmentRegistry, phone::State{}, "14:35");
        for (auto dimensions :
             {std::pair<float, float>{640.0f, 480.0f}, {1280.0f, 720.0f}, {3440.0f, 1440.0f}}) {
            auto lay = phone::layout(dimensions.first, dimensions.second);
            for (const auto &shape : scene.rectangles)
                assert(lay.x + shape.x * lay.scale >= 0 && lay.y + shape.y * lay.scale >= 0 &&
                       lay.x + (shape.x + shape.w) * lay.scale <= dimensions.first &&
                       lay.y + (shape.y + shape.h) * lay.scale <= dimensions.second);
        }
        for (const auto &t : scene.texts)
            assert(t.value.find("PHONE_") == std::string::npos);
    }
    locale::language = locale::Language::English;
    assert(locale::tr("Tamirci olarak calisiyorum. Gecinip gidiyoruz.").find("Mechanic") !=
           std::string::npos);
    auto russian = locale::catalog.translate("PHONE_INVITE_ACCEPT", locale::Language::Russian);
    auto wrapped = locale::wrap(russian, 30);
    std::string joined;
    for (const auto &line : wrapped) {
        if (!joined.empty())
            joined += ' ';
        joined += line;
    }
    assert(joined == russian);
    locale::language = locale::Language::Turkish;
    {
        people::Registry socialWorld;
        auto &root = socialWorld.create(false);
        root.household = 2;
        root.presence = {17, 4, 100, 100, 12, 0, 100, 0, true, true};
        root.social.known = people::Name | people::Mother;
        root.social.journal = true;
        auto family = people::generate(root);
        network::discover(socialWorld, 1);
        assert(socialWorld.families.size() == 6 && socialWorld.connections.size() == 9);
        for (unsigned role = 1; role <= 5; ++role) {
            const auto &relative = socialWorld.families[role];
            assert(relative.origin == 1 && relative.role == role);
            assert(people::generate(relative)[0].name == family[role].name);
            assert(people::generate(relative)[0].id == family[role].id);
            assert(!relative.social.known && !relative.social.journal);
        }
        assert(people::generate(socialWorld.families[1])[5].name == family[0].name);
        assert(people::generate(socialWorld.families[4])[4].name == family[0].name);
        network::discover(socialWorld, 1);
        assert(socialWorld.families.size() == 6);
        network::news(socialWorld, 1, true, true);
        assert(socialWorld.families[1].social.newsFrom == 1 &&
               socialWorld.families[1].social.trust < 0);
        network::honor(socialWorld, 300);
        assert(socialWorld.honor == 100);
        network::honor(socialWorld, -500);
        assert(socialWorld.honor == -100);
        socialWorld.savedAt = 1000;
        std::stringstream file;
        socialWorld.write(file);
        auto restoredWorld = people::Registry::read(file);
        assert(restoredWorld.honor == -100 && restoredWorld.savedAt == 1000 &&
               restoredWorld.connections.size() == 9);
        assert(people::generate(restoredWorld.families[1])[0].name == family[1].name);
        restoredWorld.appointments.push_back({1, 1, 60000, 300000, 100, 100, 12});
        assert(meetings::resume(restoredWorld, 1040) == 0 &&
               restoredWorld.appointments[0].remaining == 20000);
        assert(meetings::resume(restoredWorld, 1080) == 0 &&
               restoredWorld.appointments[0].state == 2 &&
               restoredWorld.appointments[0].window == 280000);
        assert(meetings::resume(restoredWorld, 1500) == 1 &&
               restoredWorld.appointments[0].state == 4);
        restoredWorld.appointments[0] = {1, 1, 60000, 300000, 100, 100, 12};
        restoredWorld.savedAt = 0;
        assert(meetings::resume(restoredWorld, 1500) == 1 &&
               restoredWorld.appointments[0].state == 4);
        life::Ride ride;
        ride.stage = 1;
        assert(!life::rideStep(ride, 250, false, true, true, true) && ride.stage == 1 &&
               ride.elapsed == 0);
        assert(!life::rideStep(ride, 250, true, true, false, true) && ride.stage == 2);
        assert(!life::rideStep(ride, 250, true, true, true, false) && ride.stage == 2);
        assert(!life::rideStep(ride, 250, true, true, true, true) && ride.stage == 3);
        assert(life::rideStep(ride, 250, true, false, true, true));
        assert(!life::rideStep(ride, 250, true, false, true, true));
        life::Ride abandoned;
        abandoned.stage = 2;
        assert(!life::rideStep(abandoned, 250, true, false, false, true) && abandoned.stage == 4 &&
               !abandoned.paid);
        for (unsigned i = 0; i < 1000; ++i)
            assert(life::rideFare(i, 500) >= 35 && life::rideFare(i, 500) <= 225);
        for (auto kind : {encounters::Kind::Beggar, encounters::Kind::Addict,
                          encounters::Kind::Ride, encounters::Kind::Visit}) {
            assert(life::complete(root, kind));
            assert(!life::complete(root, kind));
        }
        assert(people::valid(root.social));
        for (auto size :
             {std::pair<float, float>{640.0f, 480.0f}, {1280.0f, 720.0f}, {3440.0f, 1440.0f}}) {
            auto scene = vitals::compose(-100, {1000, 500, 0, 300, 900}, size.first, size.second);
            for (const auto &shape : scene.rectangles)
                assert(shape.x >= 0 && shape.y >= 0 && shape.x + shape.w <= size.first + 0.01f &&
                       shape.y + shape.h <= size.second + 0.01f);
        }
        std::cout << "PASS: physical family identity, reciprocal relatives, network news, V7 "
                     "migration, honor, offline expiry, ride boarding/exit/payment, event reward "
                     "limits, TAB layout.\n";
    }
    std::cout << "PASS: greeting budgets; persistent resentment; broken-truce apology rejection; "
                 "appointments and V6 persistence; four languages; UTF-8 wrapping; phone bounds "
                 "and privacy.\n";
    std::cout << "PASS: 32768 families; reciprocal ages/relationships; v1 migration; v2 world "
                 "roundtrip; duplicate prevention; pool recycling; city move; death; spatial "
                 "relocation; corrupt input and limits\n";
    {
        // Disposable-stranger recycling keeps contacts, relatives, appointments and graph ends.
        people::Registry pool;
        for (unsigned i = 0; i < 10; ++i)
            pool.create(false);
        pool.families[0].social.known = people::Name; // named contact
        pool.families[1].social.trust = 5;            // touched relationship
        pool.connections.push_back({3, 4, 0});        // graph endpoints
        pool.appointments.push_back({5, 1, 1000, 1000, 0, 0, 0});
        pool.families[5].origin = 1; // relative of contact 1
        pool.families[5].role = 1;
        std::unordered_set<unsigned> live{7};
        std::vector<unsigned> taken;
        for (unsigned n = 0; n < 4; ++n) {
            unsigned id = pool.recycle([&](unsigned x) { return live.count(x) != 0; });
            assert(id);
            assert(id != 1 && id != 2 && id != 3 && id != 4 && id != 5 && id != 6 && id != 7);
            taken.push_back(id);
            pool.reset(id, true);
            assert(pool.families[id - 1].id == id && pool.families[id - 1].female);
        }
        assert(taken[0] == 8 && taken[1] == 9 && taken[2] == 10 && taken[3] == 8);
        std::stringstream out;
        pool.write(out);
        assert(people::Registry::read(out).families.size() == 10);
        std::cout << "PASS: archive soft limit recycles only untouched strangers.\n";
    }
    {
        // Provocation ladder: puzzlement, then severed-off with the number deleted; police arrest.
        people::Family neighbor{5, false, 3, {}, {}};
        neighbor.social.known = people::Name | people::Job;
        neighbor.social.journal = true;
        neighbor.social.trust = 40;
        street::Encounter chat{5};
        auto opening = street::exchange(neighbor, chat, street::Action::Provoke, true, 4);
        assert(opening.reaction != dialogue::Reaction::Fight && !opening.forgotten &&
               !opening.arrest);
        assert(neighbor.social.provokes == 1 && !neighbor.social.cutOff && neighbor.social.journal);
        bool severed = false;
        for (unsigned i = 0; i < 5 && !severed; ++i) {
            auto follow = street::exchange(neighbor, chat, street::Action::Provoke, false, 4);
            severed = follow.forgotten;
        }
        assert(severed && neighbor.social.cutOff && !neighbor.social.journal &&
               !neighbor.social.known);
        street::Encounter retry{5};
        auto refused = street::exchange(neighbor, retry, street::Action::Greet, true, 4);
        assert(!refused.line.empty() && neighbor.social.cutOff);
        std::stringstream stored;
        people::Registry book;
        book.create(false).social = neighbor.social;
        book.write(stored);
        auto reread = people::Registry::read(stored);
        assert(reread.families[0].social.cutOff == 1 && reread.families[0].social.provokes > 1);
        people::Family cop{9, false, 0, {}, {}};
        street::Encounter halt{9};
        street::Reply final;
        for (unsigned i = 0; i < 3; ++i)
            final = street::exchange(cop, halt, street::Action::Provoke, false, 6);
        assert(final.arrest && cop.social.cutOff);
        assert(socialaudio::context(street::Action::Provoke, 1, false, final, 6) ==
               socialaudio::Context::Arrest);
        people::Registry feudBook;
        feudBook.create(false).social.known = people::Name;
        feudBook.create(true);
        assert(network::link(feudBook, 1, 2, 3));
        assert(network::feud(feudBook, 1) == 1 && feudBook.families[1].social.feud == 1);
        auto &date = feudBook.families[1];
        date.social.feud = 0;
        date.social.trust = 60;
        date.household = 1;
        for (unsigned i = 0; i < 4; ++i) {
            people::Appointment visit{2, 2, 0, 1000, 0, 0, 0};
            meetings::arrive(visit, date, true);
        }
        assert(date.social.romance == 3 && street::relationship(date) == std::string("Sevgili"));
        std::stringstream feudFile;
        feudBook.write(feudFile);
        assert(people::Registry::read(feudFile).families[1].social.romance == 3);
        std::set<std::string> uniqueNames;
        for (unsigned id = 1; id <= 300; ++id)
            uniqueNames.insert(
                people::generate(people::Family{id, id % 2 == 0, 0, {}, {}})[0].name);
        assert(uniqueNames.size() >= 295);
        {
            people::Registry romanceBook;
            auto &partnerNpc = romanceBook.create(true);
            partnerNpc.household = 1;
            partnerNpc.presence = {41, 5, 0, 0, 13, 0, 100, 0, true, true};
            partnerNpc.social.trust = 80;
            partnerNpc.social.known = people::Name;
            partnerNpc.social.journal = true;
            auto originalName = people::generate(partnerNpc)[0].name;
            assert(!people::hasSpouse(partnerNpc) && !people::hasChild(partnerNpc));
            network::discover(romanceBook, partnerNpc.id);
            for (auto &relative : romanceBook.families)
                assert(relative.role != 3 && relative.role != 5);
            auto &married = romanceBook.create(true);
            married.household = 2;
            assert(!romance::eligible(romanceBook, married));
            married.social.romance = 3;
            married.social.dates = 5;
            married.social.homeVisits = 2;
            married.social.trust = 90;
            assert(!romance::homeReady(married) && !romance::plan(married));
            married.social.romance = 0;
            assert(!romance::plan(partnerNpc) && !romance::homeReady(partnerNpc));
            for (unsigned i = 0; i < 5; ++i) {
                people::Appointment dateMeeting{1, 2, 0, 1000, 0, 0, 13};
                bool allowed = romance::eligible(romanceBook, partnerNpc);
                assert(meetings::arrive(dateMeeting, partnerNpc, true, allowed));
                auto dates = partnerNpc.social.dates;
                assert(!meetings::arrive(dateMeeting, partnerNpc, true, allowed) &&
                       partnerNpc.social.dates == dates);
                if (i >= 3) {
                    assert(romance::homeReady(partnerNpc));
                    assert(romance::homeCompleted(partnerNpc));
                    assert(!romance::homeCompleted(partnerNpc));
                }
            }
            assert(partnerNpc.social.homeVisits == 2 && partnerNpc.social.dates == 5);
            auto &other = romanceBook.create(true);
            other.household = 1;
            assert(!romance::eligible(romanceBook, other));
            assert(romance::plan(partnerNpc) && !romance::plan(partnerNpc));
            assert(!romance::advance(partnerNpc, 10079) && partnerNpc.social.childStage == 1);
            std::stringstream saveFamily;
            romanceBook.write(saveFamily);
            auto loadedFamily = people::Registry::read(saveFamily);
            auto &mom = loadedFamily.families[0];
            assert(mom.social.homeVisits == 2 && mom.social.childMinutes == 10079);
            assert(romance::advance(mom, 1) && !romance::advance(mom, 10080));
            assert(mom.social.childStage == 2 &&
                   romance::childName(mom).find(" Johnson") != std::string::npos);
            assert(people::generate(mom)[0].name == originalName);
            street::Encounter tired{mom.id};
            tired.turns = 50;
            tired.patience = 2;
            assert(street::exchange(mom, tired, street::Action::Greet, false).line ==
                   "ROMANCE_GOODBYE");
            mom.social.anger = 40;
            assert(!romance::homeReady(mom));
            people::Registry legacyBook;
            auto &legacyPartner = legacyBook.create(true);
            legacyPartner.household = 0;
            legacyPartner.social.romance = 3;
            legacyPartner.social.trust = 75;
            legacyPartner.social.known = people::Name | people::Spouse | people::Child;
            auto beforeMigration = people::generate(legacyPartner);
            std::stringstream v10file;
            legacyBook.write(v10file);
            std::string v9header, v9count, v9person;
            std::getline(v10file, v9header);
            std::getline(v10file, v9count);
            std::getline(v10file, v9person);
            for (unsigned field = 0; field < 6; ++field)
                v9person.erase(v9person.rfind(' '));
            std::stringstream legacyFile;
            std::stringstream legacyTail;
            legacyTail << v10file.rdbuf();
            std::string legacyWorld = legacyTail.str();
            auto newSection = legacyWorld.find("LIFE ");
            if (newSection != std::string::npos)
                legacyWorld.resize(newSection);
            legacyFile << "SAPEOPLE 9\n" << v9count << '\n' << v9person << '\n' << legacyWorld;
            auto migratedBook = people::Registry::read(legacyFile);
            auto &migratedPartner = migratedBook.families[0];
            assert(migratedPartner.household == 3 && migratedPartner.social.dates == 3);
            auto afterMigration = people::generate(migratedPartner);
            assert(!afterMigration[0].spouse &&
                   afterMigration[0].children == beforeMigration[0].children);
            for (unsigned member = 0; member < 6; ++member)
                assert(afterMigration[member].name == beforeMigration[member].name);
            for (unsigned lang = 0; lang < 4; ++lang) {
                locale::language = static_cast<locale::Language>(lang);
                phone::State familyView;
                familyView.page = phone::Page::Contact;
                familyView.person = 1;
                familyView.detail = 4;
                auto familyScene = phone::compose(loadedFamily, familyView, "14:35");
                for (const auto &line : familyScene.texts)
                    assert(line.x >= 0 && line.y >= 0 && line.x + line.width <= 332 &&
                           line.y <= 650);
            }
            locale::language = locale::Language::Turkish;
            std::cout << "PASS: single/married eligibility, exclusivity, no invented relatives, "
                         "once-per-date home visits, explicit family planning, birth timer and "
                         "Johnson persistence.\n";
        }
        std::cout
            << "PASS: persona patience, puzzlement then severed-off, deleted neighbor, police "
               "arrest on the third provocation, V8 persistence.\n";
    }
#include "SocialTests.inc"
    {
        people::Registry book;
        auto &helper = book.create(false);
        helper.social.known = people::Name;
        helper.social.journal = true;
        helper.social.trust = 50;
        auto &victim = book.create(true);
        book.worldMinutes = 100;
        assert(later::schedule(book, helper.id, later::Gift) &&
               !later::schedule(book, helper.id, later::Gift) && book.later.size() == 1);
        assert(!later::schedule(book, 9, later::Thanks) && !later::schedule(book, 1, 4));
        assert(later::schedule(book, victim.id, later::Revenge, victim.id));
        people::Later item;
        assert(!later::next(book, true, item)); // Nothing is due yet.
        std::stringstream saved;
        book.write(saved);
        auto loaded = people::Registry::read(saved);
        assert(loaded.later.size() == 2 && loaded.later[0].kind == later::Gift &&
               loaded.later[1].source == victim.id && loaded.later[1].due == book.later[1].due);
        loaded.worldMinutes += 1300; // Past both delays.
        assert(later::next(loaded, false, item) && item.kind == later::Gift &&
               loaded.later.size() == 1);
        unsigned gift = 0;
        assert(later::deliver(loaded, item, gift) && gift >= 40 && gift <= 150 &&
               loaded.letters.back().body == "MAIL_LATER_GIFT" &&
               loaded.families[0].social.trust == 54);
        assert(!later::next(loaded, false, item)); // Revenge waits for a safe moment.
        assert(later::next(loaded, true, item) && item.kind == later::Revenge);
        loaded.later.push_back(item);
        loaded.worldMinutes = item.due + later::RevengeExpiry + 1;
        assert(!later::next(loaded, true, item) && loaded.later.empty()); // Expired.
        loaded.families[0].social.robbed = 1; // Wronged since: no thank-you arrives.
        assert(!later::deliver(loaded, {1, later::Thanks, 0, 0}, gift));
        std::stringstream v11("SAPEOPLE 11\n0\nMEETINGS 0\nWORLD 0 0 0\nLIFE 0 1 0\nMAIL 0\n");
        assert(people::Registry::read(v11).later.empty());
        std::cout << "PASS: delayed thanks, gifts and revenge; V12 persistence; expiry.\n";
    }
    {
        people::Registry book;
        auto &mate = book.create(false);
        assert(std::string(favors::refusal(book, mate.id)) == "FAVOR_UNKNOWN_JOB");
        mate.social.known = people::Name | people::Job;
        mate.social.trust = 20;
        assert(std::string(favors::refusal(book, mate.id)) == "FAVOR_LOW_TRUST");
        mate.social.trust = 60;
        assert(!favors::refusal(book, mate.id));
        auto perk = favors::perk(mate);
        favors::granted(book, mate.id, perk);
        assert(std::string(favors::refusal(book, mate.id)) == "FAVOR_COOLDOWN" &&
               book.letters.back().body == std::string(favors::label(perk)) + "_MSG");
        assert(later::pending(book) == 0 && book.later.size() == 1);
        std::stringstream saved;
        book.write(saved);
        auto loaded = people::Registry::read(saved);
        assert(later::cooling(loaded, mate.id));
        people::Later item;
        loaded.worldMinutes += 1441;
        assert(!later::next(loaded, true, item) && loaded.later.empty() &&
               !favors::refusal(loaded, mate.id)); // A day later the cooldown is gone.
        unsigned perks[5] = {};
        for (unsigned i = 0; i < 40; ++i)
            ++perks[static_cast<unsigned>(favors::perk(book.create(i % 2 != 0)))];
        assert(perks[0] && perks[4]);
        auto amount = favors::cash(book, 1);
        assert(amount >= 50 && amount <= 150);
        loaded.families[0].social.robbed = 1;
        assert(std::string(favors::refusal(loaded, 1)) == "FAVOR_NO");
        auto &woman = loaded.create(true);
        woman.presence.alive = true;
        for (unsigned id = 0; id < 12; ++id) // Ordinary women never proposition CJ.
            assert(!life::nightCandidate(loaded.create(true), 5, 23, true, 25));
        assert(life::nightCandidate(woman, 22, 23, true, 25));
        std::cout << "PASS: job favors, trust gate, day cooldown persisted in V12.\n";
    }
    {
        people::Registry book;
        auto &pal = book.create(false);
        pal.presence.alive = true;
        pal.social.known = people::Name;
        pal.social.journal = true;
        pal.social.trust = 40;
        const char *reply = nullptr;
        std::vector<std::string> replies;
        for (unsigned i = 0; i < 7; ++i) {
            book.worldMinutes += 6;
            reply = nullptr;
            assert(phonedialogue::send(book, pal.id, 0, false, 600, &reply) == std::string());
            replies.push_back(reply ? reply : "");
        }
        assert(replies[0].rfind("TXT_REPLY_HELLO_", 0) == 0 &&
               replies[1].rfind("TXT_REPLY_REPEAT_", 0) == 0 &&
               replies[3].rfind("TXT_REPLY_SPAM_", 0) == 0 && replies[6].empty() &&
               pal.social.anger > 0); // Repeats, then annoyance, then silence.
        auto &stood = book.create(true);
        stood.presence.alive = true;
        stood.social.known = people::Name;
        stood.social.journal = true;
        stood.social.anger = 30; // Provoked at the meeting.
        book.worldMinutes += 6;
        reply = nullptr;
        assert(phonedialogue::send(book, stood.id, 1, false, 600, &reply) == std::string() &&
               !reply && book.letters.back().outgoing);
        book.worldMinutes += 6;
        assert(phonedialogue::send(book, stood.id, 3, false, 600, &reply) == std::string() &&
               reply); // An apology still gets an answer.
        reply = "stale reply";
        assert(std::string(phonedialogue::send(book, stood.id, 0, false, 600, &reply)) ==
                   "TXT_WAIT" && !reply);
        std::cout << "PASS: repeated texts, spam silence, provoked contacts leave texts unread.\n";
    }
    {
        people::Registry book;
        auto &pal = book.create(false);
        pal.presence = {7, 4, 10, 20, 3, 0, 100, 0, true, true};
        pal.social.journal = true;
        pal.social.known = people::Name;
        pal.social.trust = 70;
        std::unordered_map<unsigned, meetings::Run> runs;
        unsigned releases = 0;
        // Same contact, four successive activities: fresh route and start state each time.
        for (unsigned activity = 0; activity < 4; ++activity) {
            assert(meetings::schedule(book, pal.id, 10, 20, 3, 0));
            meetings::forgetRun(runs, pal.id, [&](meetings::Run &) { ++releases; });
            auto &visit = runs[pal.id];
            assert(!visit.activityStarted && !visit.spotResolved && !visit.waiting &&
                   !visit.spawnFails && visit.blip == -1 && visit.task == nullptr);
            auto &appointment = book.appointments[0];
            appointment.activity = activity;
            assert(meetings::step(appointment, 250, true));
            visit.activityStarted = visit.spotResolved = visit.waiting = true;
            visit.spawnFails = 9;
            visit.blip = 42;
            assert(socialworld::complete(book, appointment, pal, true, false));
            assert(!socialworld::complete(book, appointment, pal, true, false));
        }
        assert(releases == 3 && pal.daily.outings == 4 && pal.meetings == 4);
        meetings::forgetRun(runs, pal.id, [&](meetings::Run &) { ++releases; });
        meetings::forgetRun(runs, pal.id, [&](meetings::Run &) { ++releases; });
        assert(runs.empty() && releases == 4);

        street::Encounter helpTalk{pal.id};
        street::rejectBusy(helpTalk);
        street::continueAfterHelp(helpTalk, pal.id);
        assert(street::mayGreet(helpTalk) && !helpTalk.ended && helpTalk.turns == 1);
        const auto continued = street::exchange(pal, helpTalk, street::Action::Greet, false);
        assert(!continued.line.empty() && continued.cue != street::Cue::Greeting);

        assert(phonedialogue::answers(pal, 0));
        socialworld::meetingArgument(pal);
        assert(!phonedialogue::answers(pal, 0) && !phonedialogue::answers(pal, 4) &&
               phonedialogue::answers(pal, 3));
        pal.social.cutOff = 1;
        assert(!phonedialogue::answers(pal, 3));
        pal.social.cutOff = 0;
        pal.presence.alive = false;
        assert(!phonedialogue::answers(pal, 3));

        socialworld::Travel drive;
        drive.sample({0, 0, 0}, false, 20);
        drive.sample({100, 0, 0}, true, 20);
        for (unsigned x = 110; x <= 600; x += 10)
            drive.sample({static_cast<float>(x), 0, 0}, true, 20);
        assert(drive.distance == 500);
        drive.sample({2000, 0, 0}, true, 20); // A teleport is not a drive.
        assert(drive.distance == 500);
        drive.sample({2010, 0, 0}, false, 20);
        drive.sample({2020, 0, 0}, true, 20);
        assert(drive.distance == 500); // Neither alone nor rejoining earns distance.
        socialworld::Travel walk;
        walk.sample({0, 0, 0}, true, 5);
        for (unsigned x = 1; x <= 150; ++x)
            walk.sample({static_cast<float>(x), 0, 0}, true, 5);
        assert(walk.distance == 150);
        std::cout << "PASS: repeated appointment cleanup, four activity rewards, help continuation, "
                     "queued-reply eligibility, measured drive/walk distance and teleport rejection.\n";
    }
    {
        people::Registry contextBook;
        auto &contactPerson = contextBook.create(true);
        contactPerson.social.known = people::Name;
        contactPerson.social.journal = true;
        contactPerson.social.trust = 60;
        contactPerson.presence = {93, 5, 0, 0, 13, 0, 100, 0, true, true};
        assert(std::string(phonedialogue::label(contextBook, 1, 0)) == "TXT_HELLO_AGAIN");
        const char *incoming[] = {"MAIL_LATER_THANKS1", "MAIL_GOOD_OUTING_1", "MAIL_FAMILY_PLANS", "MAIL_VISIT_0"};
        const unsigned expected[] = {6, 7, 8, 10};
        for (unsigned i = 0; i < 4; ++i) {
            contextBook.worldMinutes += 10;
            contextBook.post(1, incoming[i]);
            auto opts = phonedialogue::options(contextBook, 1);
            assert(opts.front() == expected[i]);
            std::set<unsigned> unique(opts.begin(), opts.end());
            assert(unique.size() == opts.size());
            const char *answer = nullptr;
            assert(std::string(phonedialogue::send(contextBook, 1, opts.front(), false, 900, &answer)).empty());
            assert(answer && std::string(answer).rfind("TXT_ACK_", 0) == 0);
            assert(contextBook.letters.back().outgoing &&
                   contextBook.letters.back().body == phonedialogue::choices[expected[i]]);
        }
        contextBook.post(1, "MAIL_INVITATION", false, 2);
        auto inviteOpts = phonedialogue::options(contextBook, 1);
        assert(inviteOpts.front() == 9 && std::set<unsigned>(inviteOpts.begin(), inviteOpts.end()).size() == inviteOpts.size());
        assert(meetings::schedule(contextBook, 1, 20, 20, 13, 0));
        auto &date = contextBook.appointments.back();
        date.state = 2;
        assert(socialworld::complete(contextBook, date, contactPerson, true, false));
        assert(!socialworld::spontaneous(contextBook, contactPerson, 900));
        assert(later::schedule(contextBook, 1, later::Visit));
        std::stringstream v13;
        contextBook.write(v13);
        assert(v13.str().rfind("SAPEOPLE 16", 0) == 0);
        auto reopened = people::Registry::read(v13);
        assert(reopened.later.size() == 1 && reopened.later[0].kind == later::Visit);
        reopened.worldMinutes = reopened.later[0].due;
        people::Later visitMessage;
        assert(later::next(reopened, false, visitMessage));
        unsigned noCash = 0;
        assert(later::deliver(reopened, visitMessage, noCash) && !noCash &&
               reopened.letters.back().body.rfind("MAIL_VISIT_", 0) == 0);
        std::stringstream legacyData;
        reopened.write(legacyData);
        auto legacyString = legacyData.str();
        legacyString.replace(0, 11, "SAPEOPLE 12");
        legacyString.resize(legacyString.find("COMMUNITY "));
        std::stringstream legacyInput(legacyString);
        auto legacyRead = people::Registry::read(legacyInput);
        assert(legacyRead.families[0].social.known == contactPerson.social.known);
        using namespace socialcontext;
        assert(passengerDoor(0) == 8 && passengerDoor(1) == 11 && passengerDoor(2) == 9);
        assert(passengerDoor(-1) == -1 && passengerDoor(3) == -1 && passengerDoor(0, true) == 11);
        assert(driveReady(true, true, true, true));
        assert(!driveReady(false, true, true, true) && !driveReady(true, false, true, true) &&
               !driveReady(true, true, false, true) && !driveReady(true, true, true, false));
        Topics topicBag;
        std::set<unsigned> topicsSeen;
        for (unsigned i = 0; i < 12; ++i) topicsSeen.insert(topicBag.take(12, 31));
        assert(topicsSeen.size() == 12 && topicBag.take(12, 31) == 12);
        TalkChoices questions;
        assert(questions.use(0, 10000) == 1 && questions.use(0, 11000) == 0);
        assert(questions.use(1, 11000) == 0 && questions.use(1, 14000) == 1);
        assert(questions.use(0, 18000) == 2 && questions.use(0, 26000) == 3);
        assert(weather(0.8f, 0.9f, 1, 0, 12) == Weather::Rain);
        assert(weather(0, 0.8f, 1, 0, 12) == Weather::Fog);
        assert(weather(0, 0, 1, 0, 12) == Weather::Sun);
        assert(weather(0, 0, 1, 0, 23) == Weather::Quiet);
        assert(!witnessCancels(Witness::Help) && witnessCancels(Witness::Provoke) &&
               witnessCancels(Witness::Violence) && witnessCancels(Witness::Killing));
        assert(honorTone(60) == 1 && honorTone(-60) == -1 && honorTone(0) == 0);
        std::cout << "PASS: contextual response menus, known-contact greeting, outing invitation suppression, "
                     "V13 delayed visit text and V12 import, passenger door safety, drive gates, "
                     "non-repeating topics, repeatable talk cooldowns, weather and witness policies.\n";
    }

#include "HonorTests.inc"
#include "NativeTalkTests.inc"
#include "InstallPathsTests.inc"
#include "NeighborhoodTests.inc"
#include "IntroductionTests.inc"
#include "ConflictTests.inc"
#include "NpcVoiceTests.inc"
#include "CommunityTests.inc"
#include "BondsTests.inc"
    {
        assert(gangcombat::eligible(7,1,false,false,true,true,25));
        assert(gangcombat::eligible(9,1,false,false,true,true,625));
        assert(!gangcombat::eligible(8,1,false,false,true,true,25));
        assert(!gangcombat::eligible(7,2,false,false,true,true,25));
        assert(!gangcombat::eligible(7,1,true,false,true,true,25));
        assert(!gangcombat::eligible(7,1,false,true,true,true,25));
        assert(!gangcombat::eligible(7,1,false,false,false,true,25));
        assert(!gangcombat::eligible(7,1,false,false,true,false,25));
        assert(!gangcombat::eligible(7,1,false,false,true,true,626));
        assert(gangcombat::ambientTask(1201) && gangcombat::ambientTask(1207) && gangcombat::ambientTask(1212));
        assert(gangcombat::ambientTask(-1) && !gangcombat::ambientTask(244));
        assert(!gangcombat::ambientTask(205) && !gangcombat::ambientTask(1000));
        assert(gangcombat::combatTask(1000) && gangcombat::combatTask(1002));
        std::cout << "PASS: rejected gang combat accepts armed gang idles, excludes missions, "
                     "recruits, dead/recycled actors, other interiors, injuries and scripted tasks.\n";
    }
}
