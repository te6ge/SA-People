#pragma once
#include "Dialogue.h"
#include "Persona.h"
#include "SocialProgress.h"
#include "Introductions.h"
#include "Locale.h"
namespace street {
enum class Action { Greet, Provoke, Defuse };
enum class Cue {
    Neutral,
    Greeting,
    Depart,
    Joke,
    Surprise,
    RoughDay,
    SharedWalk,
    Question,
    Reassure,
    SanFierro,
    Apology
};
struct Encounter {
    unsigned id = 0, turns = 0, shared = 0, positive = 0, patience = 0;
    bool ended = false, busyRejected = false, refused = false, apologyUsed = false,
         brokenTruce = false;
    unsigned farewells = 0;
    bool disclosed = false;
    socialprogress::LineDeck topics;
};
struct Reply {
    std::string line;
    dialogue::Reaction reaction = dialogue::Reaction::None;
    bool leaves = false, newJournal = false, newFriend = false;
    Cue cue = Cue::Neutral;
    bool calmed = false;
    bool arrest = false, forgotten = false; // Police arrest; the contact deleted CJ's number.
};
inline bool mayGreet(const Encounter &e) {
    return !e.busyRejected && (!e.refused || e.farewells < 1);
}
inline bool mayDefuse(const people::Family &f, const Encounter &e, bool fighting = false) {
    return !f.social.cutOff && !f.social.feud && !e.apologyUsed && !e.brokenTruce &&
           f.social.resentment < 55 && (f.social.anger > 0 || fighting);
}
// A passing stranger can stage a random incident; someone CJ has already dealt with
// keeps that history even before their number is added to the phone.
inline bool strangerIncident(const people::Family &f) {
    const auto &s = f.social;
    return !s.known && !s.journal && !s.encounters && !s.attempts && !s.helped &&
           !s.romance && !s.feud && !s.robbed && !s.cutOff && !s.provokes &&
           !s.meaningful && !s.events && !s.anger && !s.resentment && !s.trust &&
           !f.meetings && !f.daily.outings;
}
inline void rejectBusy(Encounter &e) {
    e.busyRejected = true;
    e.refused = true;
    e.ended = true;
}
inline void continueAfterHelp(Encounter &e, unsigned id) {
    e.id = id;
    e.turns = std::max(e.turns, 1u);
    e.ended = e.busyRejected = e.refused = false;
    e.farewells = 0;
}
inline bool farewell(Encounter &e) {
    if (!mayGreet(e) || !e.refused)
        return false;
    ++e.farewells;
    return true;
}
inline bool isFriend(const people::Family &f) {
    return f.social.trust >= 55 && f.social.encounters >= 3 && f.social.meaningful >= 8 &&
           f.social.anger < 45 && f.social.resentment < 40;
}
inline bool journaled(const people::Family &f) {
    return f.social.journal;
}
inline const char *relationship(const people::Family &f) {
    return f.commitment.stage==3 ? (f.social.romance<3 || f.social.cutOff || f.social.trust<20 ? "REL_ESTRANGED" : "REL_MARRIED")
           : f.commitment.stage==2 ? "REL_BOOKED"
           : f.commitment.stage==1 ? "REL_ENGAGED"
           : f.social.romance >= 3   ? "Sevgili"
           : f.social.romance == 2 ? "Ilgi duyuyor"
           : isFriend(f)           ? "Arkadas"
           : f.social.trust < -20  ? "Araniz gergin"
           : journaled(f)          ? "Tanidik"
                                   : "Yabanci";
}
inline std::string departure(unsigned seed) {
    static const char *lines[] = {"Ocakta yemek var, yakmadan eve yetiseyim. Gorusuruz!",
                                  "Muhabbet guzel de ise gec kaldim. Sonra devam ederiz.",
                                  "Cocugu almaya gidecegim. Kendine iyi bak.",
                                  "Otobusu kacirmayayim. Bir ara yine konusuruz.",
                                  "Market kapanmadan yetismem lazim. Gorusuruz.",
                                  "Annem beni bekliyor. Simdilik musade isteyeyim.",
                                  "Vardiyam basliyor. Yine denk geliriz.",
                                  "Basim agriyor, bugun pek konusasim yok.",
                                  "Komsuya soz verdim, ona bir ugrayacagim.",
                                  "Karnim zil caliyor. Bir seyler yiyip geleyim.",
                                  "Eve misafir gelecek. Hazirlik yapmam lazim.",
                                  "Bir arkadasi bekletiyorum. Sonra gorusuruz.",
                                  "Faturalari yatirmam gerekiyor. Gec kalmayayim.",
                                  "Bugun biraz yalniz yurusem iyi olacak.",
                                  "Camasirlar makinede kaldi. Kosayim ben.",
                                  "Randevum var. Muhabbeti baska zaman tamamlayalim."};
    return lines[seed % 16];
}
inline void addMeaning(people::Family &f, unsigned amount, unsigned note) {
    auto &s = f.social;
    s.meaningful = std::min(1000000u, s.meaningful + amount);
    s.lastNote = note;
    if (introductions::contact(f))
        s.journal = true;
}
inline Reply exchange(people::Family &f, Encounter &e, Action action, bool combatAllowed,
                      int pedType = 4, int honor = 0, bool fighting = false) {
    Reply r;
    if ((e.ended && action == Action::Greet) || (action == Action::Greet && !mayGreet(e)))
        return r;
    const auto kind = persona::of(pedType);
    if (f.social.cutOff && action != Action::Provoke) {
        r.line = persona::ignored(kind, people::mix(f.id * 131 + f.social.attempts));
        r.reaction = dialogue::Reaction::Angry;
        return r;
    }
    bool hadJournal = f.social.journal, wasFriend = isFriend(f);
    auto &s = f.social;
    auto adjust = [&](int t, int a) {
        s.trust = std::clamp(s.trust + t, -100, 100);
        s.anger =
            std::max(s.resentment * 3 / 5,
                     static_cast<unsigned>(std::clamp(static_cast<int>(s.anger) + a, 0, 100)));
    };
    unsigned seed = people::mix(f.id * 131 + s.encounters * 23 + e.turns * 7 + s.attempts);
    auto p = people::generate(f);
    if (action == Action::Defuse) {
        r.cue = Cue::Apology;
        if (e.apologyUsed || e.brokenTruce || s.resentment >= 55 || s.feud) {
            r.line = "Az once de aynisini soyledin. Artik sana inanmiyorum.";
            r.reaction = dialogue::Reaction::Angry;
            return r;
        }
        bool angry = s.anger > 0 || fighting;
        if (angry) {
            e.apologyUsed = true;
            unsigned floor = s.resentment * 3 / 5;
            unsigned relief = 24 - s.resentment / 6;
            s.anger = std::max(floor, s.anger > relief ? s.anger - relief : 0);
            r.calmed = s.anger <= 45;
        }
        static const char *lines[] = {
            "Tamam, uzatmayalim. Sen yoluna, ben yoluma.", "Peki CJ. Bir daha boyle konusma.",
            "Anlastik. Biraz sakinlik iyi gelir.", "Tamam, meseleyi burada kapatalim."};
        r.line = angry ? lines[seed % 4] : "Bir sorun yok. Iyi gunler.";
        r.reaction = dialogue::Reaction::Indifferent;
        r.leaves = angry;
        e.ended = angry;
        s.lastReaction = static_cast<unsigned>(r.reaction);
        return r;
    }
    ++e.turns;
    if (s.attempts < 1000000)
        ++s.attempts;
    if (action == Action::Provoke) {
        const bool named = (s.known & people::Name) != 0, already = s.cutOff != 0;
        if (e.apologyUsed)
            e.brokenTruce = true;
        s.provokes = std::min(255u, s.provokes + 1);
        s.romance = 0; // Provoking ends any romance.
        s.resentment = std::min(100u, s.resentment + (e.brokenTruce ? 36u : 14u));
        adjust(-16 - static_cast<int>(s.resentment / 10),
               25 + static_cast<int>(s.resentment / 5) + (e.brokenTruce ? 15 : 0));
        s.lastNote = 6;
        e.refused = true;
        e.farewells = 1;
        bool fight = combatAllowed && (s.anger >= 65 || (e.brokenTruce && s.anger >= 50) ||
                                       (dialogue::temperament(f) == 3 && s.anger >= 25));
        // A known face first looks puzzled instead of picking a fight.
        if (named && !already && s.provokes == 1)
            fight = false;
        r.reaction = fight ? dialogue::Reaction::Fight : dialogue::Reaction::Angry;
        static const char *threats[] = {
            "Derdin ne senin? Yoluna baksana!", "Tanimadigin insanla boyle konusma.",
            "Baska eglencen yok mu senin?",     "Agzindan cikana dikkat et.",
            "Bugun benimle ugrasma!",           "Hadi, isine bak. Basimi agritma."};
        static const char *fights[] = {"Yeter artik! Sen kasindin!",
                                       "Gel buraya! Simdi hesaplasacagiz.",
                                       "O lafi sana yedirecegim!"};
        const bool cut = !already && s.provokes >= persona::patience(f, kind);
        if (kind == persona::Kind::Cop && cut) {
            r.arrest = true;
            fight = false;
            r.reaction = dialogue::Reaction::Angry;
        }
        if (cut) {
            s.cutOff = 1;
            if (named) {
                s.known = 0;
                s.journal = false;
                r.forgotten = true;
            }
        }
        auto warned = persona::warning(kind, seed);
        r.line = r.arrest  ? persona::arrest()
                 : already ? (fight ? fights[seed % 3] : persona::ignored(kind, seed))
                 : cut     ? persona::cutoff(kind, seed)
                 : fight   ? fights[seed % 3]
                 : named && s.provokes == 1 ? persona::confused(kind, seed)
                 : !warned.empty()          ? warned
                 : s.anger >= 50            ? "Uzak dur benden! Son kez soyluyorum."
                                            : threats[seed % 6];
        r.leaves = cut || already || r.arrest || s.anger >= 50;
        e.ended = r.leaves || fight;
    } else {
        if (!e.patience)
            e.patience = 2 + people::mix(f.id + s.encounters * 17) % 4 + (isFriend(f) ? 2u : 0u);
        unsigned patience = e.patience;
        if (e.turns >= patience || s.resentment >= 50) {
            r.line = f.social.romance >= 3 ? "ROMANCE_GOODBYE" : departure(seed);
            r.leaves = true;
            r.reaction = dialogue::Reaction::Indifferent;
            e.ended = true;
            e.refused = true;
        } else if (e.turns == 1) {
            int open[] = {25, 10, -6, -18};
            int score = 25 + open[dialogue::temperament(f)] + s.trust / 3 -
                        static_cast<int>(s.anger) / 2 + static_cast<int>(seed % 31) - 15 +
                        socialprogress::openness(honor) - static_cast<int>(f.community.strain)/3;
            if (score < 5) {
                r.line = score < -15             ? "Seninle konusmak istemiyorum."
                         : f.social.romance >= 3 ? "ROMANCE_GOODBYE"
                                                 : departure(seed);
                r.leaves = true;
                e.ended = true;
                e.refused = true;
                r.reaction =
                    score < -15 ? dialogue::Reaction::Angry : dialogue::Reaction::Indifferent;
            } else {
                adjust(7, -8);
                ++e.positive;
                s.encounters = std::min(1000000u, s.encounters + 1);
                f.meetings = std::min(1000000u, f.meetings + 1);
                addMeaning(f, 0, 1);
                r.reaction = dialogue::Reaction::Friendly;
                static const char *strangers[] = {"Selam dostum. Nasil gidiyor?",
                                                  "Iyi gunler! Mahallede bir tur mu atiyorsun?",
                                                  "Selam. Bugun sokaklar hareketli.",
                                                  "Hey, nasil gidiyor?",
                                                  "Merhaba. Bir sey mi soracaktin?",
                                                  "Selam dostum. Hava almaya mi ciktin?",
                                                  "STREET_HELLO_6", "STREET_HELLO_7", "STREET_HELLO_8",
                                                  "STREET_HELLO_9", "STREET_HELLO_10", "STREET_HELLO_11"};
                static const char *contacts[] = {
                    "CJ, yine denk geldik! Nasilsin?", "Hey CJ! Uzun zaman oldu.",
                    "CJ, mahallede haber var mi?", "Bak kim gelmis! Nasil gidiyor CJ?"};
                static const char *friends[] = {"CJ! Dostum, seni gormek iyi geldi!",
                                                "Kardesim CJ! Keyfin yerinde mi?",
                                                "CJ, iyi ki denk geldik. Neler yapiyorsun?",
                                                "Hey dostum! Bugun nasil gidiyor?"};
                static socialprogress::LineDeck openings[5];
                const unsigned opening = openings[static_cast<unsigned>(kind)].take(
                    kind == persona::Kind::Civilian ? 12u : kind == persona::Kind::Vulnerable ? 5u :
                    kind == persona::Kind::Service ? 4u : 5u, seed);
                auto manner = persona::greeting(kind, opening);
                static const char *partner[] = {"Askim! Seni gormek ne guzel.",
                                                "Hey yakisikli! Seni ozledim.",
                                                "CJ, bugun de gelmissin. Cok sevindim."};
                r.line = dialogue::knows(f, people::Name)
                             ? (f.social.romance >= 3 ? partner[seed % 3]
                                : isFriend(f)         ? friends[seed % 4]
                                                      : contacts[seed % 4])
                         : !manner.empty() ? manner
                                           : strangers[opening];
                r.cue = Cue::Greeting;
                if (pedType == 8 && !dialogue::knows(f, people::Name)) {
                    static socialprogress::LineDeck groveDeck;
                    static const char *grove[] = {"AMBIENT_GROVE_0", "AMBIENT_GROVE_1", "AMBIENT_GROVE_2",
                        "AMBIENT_GROVE_3", "AMBIENT_GROVE_4", "AMBIENT_GROVE_5", "AMBIENT_GROVE_6", "AMBIENT_GROVE_7"};
                    r.line = grove[groveDeck.take(8, seed)];
                }
            }
        } else {
            adjust(5, -5);
            ++e.positive;
            r.reaction = dialogue::Reaction::Friendly;
            if (!dialogue::knows(f, people::Name) && introductions::name(f,pedType) && !e.disclosed) {
                s.known |= people::Name;
                e.disclosed = true;
                static const char *introductions[] = {"INTRO_NAME_0","INTRO_NAME_1","INTRO_NAME_2"};
                r.line = locale::format(introductions[seed%3],p[0].name);
                addMeaning(f, 1, 1);
            } else if (!dialogue::knows(f, people::Name)) {
                static const char *boundaries[] = {"INTRO_BOUNDARY_0", "INTRO_BOUNDARY_1", "INTRO_BOUNDARY_2", "INTRO_BOUNDARY_3"};
                r.line = boundaries[e.topics.take(4,seed)];
                r.cue = Cue::Neutral;
                if(!(e.shared&2)) {e.shared|=2; addMeaning(f,1,1);}
            } else if (!dialogue::knows(f, people::Job) && introductions::job(f,pedType) && !e.disclosed) {
                s.known |= people::Job;
                e.disclosed = true;
                r.line = p[0].job + " olarak calisiyorum. Gecinip gidiyoruz.";
                addMeaning(f, 2, 2);
            } else if (!(e.shared & 1) && introductions::personal(f) && !e.disclosed) {
                e.disclosed = true;
                e.shared |= 1;
                unsigned fields[] = {people::Child, people::Spouse, people::Sibling, people::Mother,
                                     people::Father};
                unsigned members[] = {5, 3, 4, 1, 2};
                const char *labels[] = {"Cocugumun adi ", "Esimin adi ", "Kardesimin adi ",
                                        "Annemin adi ", "Babamin adi "};
                bool revealed = false;
                for (unsigned i = 0; i < 5; ++i)
                    if (!(s.known & fields[i]) && (fields[i] != people::Spouse || p[0].spouse) &&
                        (fields[i] != people::Child || !p[0].children.empty())) {
                        s.known |= fields[i];
                        r.line = std::string(labels[i]) + p[members[i]].name +
                                 ". Bir ara yine anlatirim.";
                        addMeaning(f, 2, 3);
                        revealed = true;
                        break;
                    }
                if (!revealed)
                    r.line = "Aileyi de tanidin artik! Bir gun daha uzun otururuz.";
            } else if (!dialogue::knows(f, people::Background) && s.encounters>=4 && s.trust>=45 && !e.disclosed) {
                e.disclosed = true;
                s.known |= people::Background | people::Age;
                r.line = "San Andreas'ta buyudum. " + std::to_string(p[0].age) +
                         " yasindayim; daha yapacak cok is var.";
                addMeaning(f, 2, 1);
            } else {
                static const char *smallTalk[] = {
                    "Sen de benim gibi mahallede tur atiyorsun galiba.",
                    "Dun komsu kedisini ariyordu. Kedi de adamin arabasinda uyuyormus!",
                    "Az once birinin arabasi kendi kendine kaydi. Adam pesinden kostu!",
                    "Is bitiyor, evde yine is cikiyor. Her gun ayni kosturmaca.",
                    "Buralarda her gun bir tuhaflik. Sen de alistin mi?",
                    "Bir sey soracak gibisin CJ. Ne oldu?",
                    "Benim gibi sakin bir gun isteyen cok kisi vardir.",
                    "Mahalledeki dukkan yine el degistirmis. Duydun mu?",
                    "Sen de hava almaya cikmissin. Ben de onu yapiyorum.",
                    "Otobuse yetisecegim diye kostum, yanlis yone gidiyormusum!",
                    "Bazen gune hic baslamasam diyorum. Isler birikiyor.",
                    "Bir gun isim duserse sana sorabilir miyim, dostum?"};
                unsigned topic = e.topics.take(12,seed);
                if (topic == 11 && !isFriend(f))
                    topic = 6;
                r.line = smallTalk[topic];
                auto manner = persona::smallTalk(kind, seed);
                if (!manner.empty() && topic % 2 == 0)
                    r.line = manner;
                Cue cues[] = {Cue::SharedWalk, Cue::Joke,     Cue::Surprise, Cue::RoughDay,
                              Cue::RoughDay,   Cue::Question, Cue::Neutral,  Cue::Surprise,
                              Cue::SharedWalk, Cue::Joke,     Cue::RoughDay, Cue::Reassure};
                r.cue = !manner.empty() && topic % 2 == 0 ? Cue::Neutral : cues[topic];
                if (!(e.shared & 2)) {
                    e.shared |= 2;
                    addMeaning(f, 1, 1);
                }
            }
        }
    }
    s.lastReaction = static_cast<unsigned>(r.reaction);
    if (r.leaves && action == Action::Greet)
        r.cue = Cue::Depart;
    // Keep a newly disclosed detail visible instead of replacing it with a contact offer.
    if(!hadJournal && e.disclosed) s.journal=false;
    r.newJournal = !hadJournal && s.journal;
    if(r.newJournal) {
        static const char *offers[]={"INTRO_CONTACT_OFFER","INTRO_CONTACT_OFFER_1","INTRO_CONTACT_OFFER_2"};
        r.line=offers[seed%3];
    }
    r.newFriend = !wasFriend && isFriend(f);
    return r;
}
inline Reply batteryHelp(people::Family &f) {
    bool journal = f.social.journal, friendBefore = isFriend(f);
    auto &s = f.social;
    if (s.helped)
        return {"Sag ol CJ, o isi zaten hallettik.", dialogue::Reaction::Friendly};
    s.helped = 1;
    s.events |= 1;
    s.known |= people::Name;
    s.trust = std::min(100, s.trust + 20);
    s.resentment = s.resentment > 8 ? s.resentment - 8 : 0;
    s.anger = std::max(s.resentment * 3 / 5, s.anger > 20 ? s.anger - 20 : 0);
    s.encounters = std::min(1000000u, s.encounters + 1);
    addMeaning(f, 3, 4);
    return {"Calisti! Cok sag ol CJ. Ben " + people::generate(f)[0].name + ". Bunu unutmam.",
            dialogue::Reaction::Friendly, false, !journal && s.journal,
            !friendBefore && isFriend(f)};
}
struct Candidate {
    unsigned id;
    float distanceSquared, alignment;
    bool visible;
};
inline unsigned focus(const std::vector<Candidate> &candidates, unsigned previous) {
    unsigned best = 0;
    float bestScore = 100000, oldScore = 100000;
    for (const auto &c : candidates) {
        if (!c.id || !c.visible || c.distanceSquared > 64 || c.distanceSquared < 0 ||
            c.alignment < 0.05f)
            continue;
        float score = c.distanceSquared + (1 - std::clamp(c.alignment, 0.0f, 1.0f)) * 6;
        if (c.id == previous)
            oldScore = score;
        if (score < bestScore) {
            best = c.id;
            bestScore = score;
        }
    }
    return previous && oldScore < 100000 && bestScore > oldScore * 0.72f ? previous : best;
}
struct Repair {
    unsigned progress = 0;
    bool complete = false;
};
inline bool repairStep(Repair &r, unsigned elapsed, bool holding, bool valid) {
    if (r.complete)
        return false;
    if (!holding || !valid) {
        r.progress = 0;
        return false;
    }
    r.progress = std::min(4000u, r.progress + std::min(elapsed, 250u));
    if (r.progress == 4000) {
        r.complete = true;
        return true;
    }
    return false;
}
} // namespace street
