#pragma once
#include "People.h"
#include <algorithm>
namespace dialogue {
enum class Stage { Introduction, Topics, Tension, Done };
enum class Reaction : unsigned { None, Friendly, Indifferent, Angry, Fight };
struct Session {
    unsigned id = 0;
    Stage stage = Stage::Introduction;
    Reaction reaction = Reaction::None;
    unsigned asked = 0;
    std::string reply = "Seni fark etti. Nasil yaklasacaksin?";
};
inline std::string displayName(const people::Family &f) {
    return (f.social.known & people::Name) ? people::generate(f)[0].name : "Yabanci";
}
inline unsigned temperament(const people::Family &f) {
    return people::mix(f.id ^ 0xAC731u) % 4;
}
inline bool knows(const people::Family &f, unsigned field) {
    return (f.social.known & field) != 0;
}
inline const char *relation(const people::Family &f) {
    return f.social.trust >= 35
               ? "Sana isiniyor"
               : (f.social.trust <= -20 ? "Sana mesafeli" : "Henuz birbirinizi taniyorsunuz");
}
inline std::array<std::string, 3> choices(Stage stage) {
    if (stage == Stage::Introduction)
        return {"Selam, ben CJ. Nasilsin?", "Bir sey sorabilir miyim?", "Hey! Sana diyorum!"};
    if (stage == Stage::Topics)
        return {"Ne isle ugrasiyorsun?", "Ailenden bahseder misin?", "Gorusuruz. Kendine iyi bak."};
    if (stage == Stage::Tension)
        return {"Tamam, yanlis basladik. Kusura bakma.", "Sorun yok. Ben gideyim.",
                "Ne oldu? Bir derdin mi var?"};
    return {"", "", ""};
}
inline void apply(people::Family &f, Session &s, unsigned choice, bool combatAllowed) {
    if (choice > 2 || s.stage == Stage::Done)
        return;
    auto &state = f.social;
    auto adjust = [&](int trust, int anger) {
        state.trust = std::clamp(state.trust + trust, -100, 100);
        state.anger =
            static_cast<unsigned>(std::clamp(static_cast<int>(state.anger) + anger, 0, 100));
    };
    auto p = people::generate(f);
    if (s.stage == Stage::Introduction) {
        if (state.attempts < 1000000)
            ++state.attempts;
        const int disposition[] = {22, 4, -10, -22};
        int noise = static_cast<int>(people::mix(f.id * 31 + state.attempts) % 41) - 20;
        int score = (choice == 0   ? 32
                     : choice == 1 ? 12
                                   : -35) +
                    disposition[temperament(f)] + state.trust / 2 -
                    static_cast<int>(state.anger) / 2 + noise;
        if (score >= 25) {
            bool known = knows(f, people::Name);
            state.known |= people::Name;
            adjust(12, -12);
            if (f.meetings < 1000000)
                ++f.meetings;
            s.stage = Stage::Topics;
            s.reaction = Reaction::Friendly;
            s.reply = known ? "CJ! Seni hatirliyorum. Nasil gidiyor?"
                            : "Selam CJ. Ben " + p[0].name + ". Nasil gidiyor?";
        } else if (score >= -12) {
            adjust(-1, 4);
            s.stage = Stage::Done;
            s.reaction = Reaction::Indifferent;
            s.reply = "Simdi vaktim yok. Sonra konusuruz.";
        } else {
            adjust(-12, 24);
            s.stage = Stage::Tension;
            s.reaction = Reaction::Angry;
            s.reply = "Boyle mi konusulur? Biraz mesafeni koru.";
            if (choice == 2 && combatAllowed && (temperament(f) == 3 || state.anger >= 65) &&
                score < -50) {
                s.stage = Stage::Done;
                s.reaction = Reaction::Fight;
                s.reply = "Sen kasindin!";
                adjust(-15, 30);
            }
        }
    } else if (s.stage == Stage::Tension) {
        s.stage = Stage::Done;
        if (choice == 0) {
            adjust(4, -25);
            s.reaction = Reaction::Indifferent;
            s.reply = "Peki. Bir daha dikkat et.";
        } else if (choice == 1) {
            adjust(0, -8);
            s.reaction = Reaction::Indifferent;
            s.reply = "Iyi olur. Yoluna bak.";
        } else {
            adjust(-25, 35);
            s.reaction = combatAllowed ? Reaction::Fight : Reaction::Angry;
            s.reply = combatAllowed ? "Yeter! Gel bakalim!" : "Git basimdan. Son kez soyluyorum.";
        }
    } else if (s.stage == Stage::Topics) {
        if (choice == 2) {
            s.stage = Stage::Done;
            s.reply = "Gorusuruz CJ. Kendine iyi bak.";
        } else if (s.asked & (1u << choice)) {
            s.reply = "Bunu az once konustuk. Baska zaman devam edelim.";
        } else {
            s.asked |= 1u << choice;
            if (choice == 0) {
                bool newInfo = !knows(f, people::Job);
                state.known |= people::Job;
                if (newInfo)
                    adjust(6, -3);
                s.reply = p[0].job + " olarak calisiyorum. Gecinip gidiyoruz.";
                if (state.trust >= 25) {
                    state.known |= people::Age | people::Background;
                    s.reply +=
                        " " + std::to_string(p[0].age) + " yasindayim. San Andreas'ta buyudum.";
                }
            } else if (state.trust < 16 || state.anger > 25)
                s.reply = "Daha yeni tanistik. Ozel hayatimi anlatmak istemiyorum.";
            else {
                const unsigned fields[] = {people::Child, people::Spouse, people::Sibling,
                                           people::Mother, people::Father};
                const unsigned members[] = {5, 3, 4, 1, 2};
                const char *descriptions[] = {"Cocugumun adi ", "Esimin adi ", "Kardesimin adi ",
                                              "Annemin adi ", "Babamin adi "};
                bool shared = false;
                for (unsigned i = 0; i < 5; ++i)
                    if (!knows(f, fields[i]) && (fields[i] != people::Spouse || p[0].spouse) &&
                        (fields[i] != people::Child || !p[0].children.empty())) {
                        state.known |= fields[i];
                        adjust(4, -3);
                        s.reply = std::string(descriptions[i]) + p[members[i]].name +
                                  ". Aile benim icin onemli.";
                        shared = true;
                        break;
                    }
                if (!shared)
                    s.reply = "Ailemi sana anlatmistim. Bir ara yine konusuruz.";
            }
        }
    }
    state.lastReaction = static_cast<unsigned>(s.reaction);
}
} // namespace dialogue
