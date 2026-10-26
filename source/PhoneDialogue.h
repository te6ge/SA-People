#pragma once
#include "SocialWorld.h"
namespace phonedialogue {
inline constexpr const char *choices[] = {"TXT_HELLO", "TXT_CHECK",  "TXT_MEMORY",
                                          "TXT_SORRY", "TXT_INSULT", "TXT_BYE",
                                          "TXT_THANKS_ACK", "TXT_OUTING_ACK", "TXT_FAMILY_ACK",
                                          "TXT_AVAILABLE", "TXT_VISIT_AGAIN", "TXT_SUPPORT"};
inline constexpr unsigned ChoiceCount = static_cast<unsigned>(sizeof(choices) / sizeof(*choices));
enum class Context { General, Thanks, Outing, Family, Visit, Invitation };
inline Context context(const people::Registry &r, unsigned id) {
    for (auto it = r.letters.rbegin(); it != r.letters.rend(); ++it) {
        if (it->person != id) continue;
        if (it->outgoing) return Context::General;
        if (it->body.rfind("MAIL_VISIT_", 0) == 0) return Context::Visit;
        if (it->body.rfind("MAIL_LATER_", 0) == 0) return Context::Thanks;
        if (it->body.rfind("MAIL_STORY_", 0) == 0 && it->body != "MAIL_STORY_NEW_CONTACT") return Context::Thanks;
        if (it->body.rfind("MAIL_GOOD_OUTING", 0) == 0) return Context::Outing;
        if (it->body == "MAIL_FAMILY_PLANS") return Context::Family;
        if (it->kind == 2) return Context::Invitation;
        return Context::General;
    }
    return Context::General;
}
inline std::vector<unsigned> options(const people::Registry &r, unsigned id) {
    std::vector<unsigned> result;
    auto c = context(r, id);
    if (c == Context::Thanks) result.push_back(6);
    if (c == Context::Outing) result.push_back(7);
    if (c == Context::Family) result.push_back(8);
    if (c == Context::Visit) result.push_back(10);
    if (c == Context::Invitation) result.push_back(9);
    for (unsigned i = 0; i < 6; ++i) result.push_back(i);
    result.push_back(9);
    result.push_back(11);
    if (c == Context::Invitation) result.erase(result.end() - 2);
    return result;
}
inline const char *label(const people::Registry &r, unsigned id, unsigned choice) {
    if (choice >= ChoiceCount) return "";
    if (choice == 0 && id && id <= r.families.size() &&
        (r.families[id - 1].social.known & people::Name)) return "TXT_HELLO_AGAIN";
    return choices[choice];
}
inline const char *menuLabel(const people::Registry &r, unsigned id, unsigned choice) {
    static const char *shortLabels[] = {"TXT_MENU_THANKS", "TXT_MENU_OUTING", "TXT_MENU_FAMILY",
        "TXT_MENU_AVAILABLE", "TXT_MENU_VISIT", "TXT_MENU_SUPPORT"};
    return choice >= 6 && choice < ChoiceCount ? shortLabels[choice - 6] : label(r, id, choice);
}
inline constexpr const char *Hello[] = {"TXT_REPLY_HELLO_0", "TXT_REPLY_HELLO_1", "TXT_REPLY_HELLO_2",
                                        "TXT_REPLY_HELLO_3", "TXT_REPLY_HELLO_4", "TXT_REPLY_HELLO_5",
                                        "TXT_REPLY_HELLO_6", "TXT_REPLY_HELLO_7"};
inline constexpr const char *Check[] = {"TXT_REPLY_CHECK_0", "TXT_REPLY_CHECK_1", "TXT_REPLY_CHECK_2",
                                        "TXT_REPLY_CHECK_3", "TXT_REPLY_CHECK_4", "TXT_REPLY_CHECK_5",
                                        "TXT_REPLY_CHECK_6", "TXT_REPLY_CHECK_7"};
inline constexpr const char *Cold[] = {"TXT_REPLY_COLD", "TXT_REPLY_COLD_1", "TXT_REPLY_COLD_2",
                                       "TXT_REPLY_COLD_3"};
inline constexpr const char *Fine[] = {"TXT_REPLY_FINE", "TXT_REPLY_FINE_1", "TXT_REPLY_FINE_2",
                                       "TXT_REPLY_FINE_3"};
inline constexpr const char *Sorry[] = {"TXT_REPLY_SORRY", "TXT_REPLY_SORRY_1", "TXT_REPLY_SORRY_2"};
inline constexpr const char *Insult[] = {"TXT_REPLY_INSULT", "TXT_REPLY_INSULT_1",
                                         "TXT_REPLY_INSULT_2", "TXT_REPLY_INSULT_3"};
inline constexpr const char *Bye[] = {"TXT_REPLY_BYE", "TXT_REPLY_BYE_1", "TXT_REPLY_BYE_2",
                                      "TXT_REPLY_BYE_3"};
inline constexpr const char *NoMemory[] = {"TXT_REPLY_NO_MEMORY", "TXT_REPLY_NO_MEMORY_1",
                                           "TXT_REPLY_NO_MEMORY_2"};
inline constexpr const char *Repeat[] = {"TXT_REPLY_REPEAT_0", "TXT_REPLY_REPEAT_1",
                                         "TXT_REPLY_REPEAT_2", "TXT_REPLY_REPEAT_3"};
inline constexpr const char *Spam[] = {"TXT_REPLY_SPAM_0", "TXT_REPLY_SPAM_1", "TXT_REPLY_SPAM_2"};
template <std::size_t N> inline const char *pick(const char *const (&pool)[N], unsigned seed) {
    return pool[seed % N];
}
inline bool answers(const people::Family &f, unsigned choice) {
    if (!f.presence.alive || !f.social.journal || f.social.cutOff || f.social.anger >= 60)
        return false;
    const bool sulking = f.social.anger >= 20 ||
                         ((f.daily.memories & 2) && f.social.resentment >= 15);
    return !sulking || choice == 3;
}
// Choices carry explicit meaning; no text classification or external service is involved.
inline const char *send(people::Registry &r, unsigned id, unsigned choice, bool call,
                        unsigned minute, const char **deferred = nullptr) {
    if (deferred)
        *deferred = nullptr;
    if (!id || id > r.families.size() || choice >= ChoiceCount)
        return "PHONE_UNAVAILABLE";
    auto &f = r.families[id - 1];
    if (!f.presence.alive || !f.social.journal)
        return "PHONE_UNAVAILABLE";
    if (f.social.cutOff || f.social.anger >= 60)
        return "MAIL_BLOCKED";
    if (call && (f.daily.busyUntil > r.worldMinutes ||
                 (socialworld::phase(minute) == 1 && people::mix(id) % 3 != 0)))
        return "MAIL_CALL_BUSY";
    for (auto i = r.letters.rbegin(); i != r.letters.rend(); ++i)
        if (i->person == id && i->outgoing) {
            if (r.worldMinutes < i->minute + 5)
                return "TXT_WAIT";
            break;
        }
    if (r.nextLetter >= 999999999)
        return "PHONE_UNAVAILABLE";
    const unsigned variant = people::mix(id + r.nextLetter);
    const char *outgoing = label(r, id, choice);
    std::string lastIncoming;
    for (auto it = r.letters.rbegin(); it != r.letters.rend(); ++it)
        if (it->person == id && !it->outgoing) { lastIncoming = it->body; break; }
    auto choose = [&](const auto &pool) {
        const unsigned length = static_cast<unsigned>(sizeof(pool) / sizeof(pool[0]));
        unsigned index = variant % length;
        if (lastIncoming == pool[index]) index = (index + 1) % length;
        return pool[index];
    };
    bool cold = f.social.anger >= 25 || f.social.resentment >= 35 || f.social.robbed;
    // Someone CJ just provoked (or who is still fuming) reads the text and leaves it unanswered;
    // only an apology may get a cool reply.
    // The same text again and again: "you said that already", then annoyance, then silence.
    unsigned repeats = 0;
    for (auto i = r.letters.rbegin(); i != r.letters.rend() && repeats < 8; ++i) {
        if (i->person != id) continue;
        if (i->minute + 60 <= r.worldMinutes) break;
        // An unsolicited event text starts a fresh conversation. Automatic replies do not.
        if (!i->outgoing) {
            if (i->kind || (i->body.rfind("TXT_", 0) != 0 && i->body != "MAIL_FAMILY_PLANS")) break;
            continue;
        }
        if (i->body != outgoing && !(choice == 0 && i->body == choices[0])) break;
        ++repeats;
    }
    const char *reply = choose(Bye);
    if (choice == 0)
        reply = cold ? choose(Cold) : choose(Hello);
    else if (choice == 1)
        reply = cold                                 ? choose(Cold)
                : f.daily.busyUntil > r.worldMinutes ? "MAIL_FAMILY_PLANS"
                                                     : choose(Check);
    else if (choice == 2) {
        reply = socialworld::memory(f);
        if (!*reply)
            reply = choose(NoMemory);
    } else if (choice == 3)
        reply = f.social.resentment >= 55 ? "TXT_REPLY_NO_FORGIVE"
                : cold                    ? choose(Sorry)
                                          : choose(Fine);
    else if (choice == 4)
        reply = choose(Insult);
    else if (choice == 6) {
        static const char *pool[] = {"TXT_ACK_HELP_0", "TXT_ACK_HELP_1", "TXT_ACK_HELP_2", "TXT_ACK_HELP_3"};
        reply = choose(pool);
    } else if (choice == 7) {
        static const char *pool[] = {"TXT_ACK_OUTING_0", "TXT_ACK_OUTING_1", "TXT_ACK_OUTING_2", "TXT_ACK_OUTING_3"};
        reply = choose(pool);
    } else if (choice == 8) reply = "TXT_ACK_FAMILY";
    else if (choice == 9) reply = cold ? choose(Cold)
                                 : f.daily.busyUntil > r.worldMinutes ? "TXT_AVAIL_BUSY" : "TXT_AVAIL_FREE";
    else if (choice == 10) reply = cold ? choose(Cold) : "TXT_ACK_VISIT";
    else if (choice == 11) {
        static const char *pool[] = {"TXT_SUPPORT_0", "TXT_SUPPORT_1", "TXT_SUPPORT_2"};
        reply = choose(pool);
    }
    if (choice < 3 && !cold && r.honor <= -40) reply = "TXT_HONOR_WARY";
    if (choice == 0 && !cold && r.honor >= 40) reply = "TXT_HONOR_WARM";
    if (repeats >= 1 && choice != 4)
        reply = repeats >= 3 ? choose(Spam) : choose(Repeat);
    if (repeats >= 3 && choice != 4) {
        f.social.anger = std::min(100u, f.social.anger + 3);
        f.social.trust = std::max(-100, f.social.trust - 1);
    }
    if (!answers(f, choice) || (repeats >= 5 && choice != 4))
        reply = nullptr; // Seen, no answer.
    r.post(id, outgoing, true, 0, 0, call);
    socialworld::remoteEffect(r, id,
                              choice == 4   ? socialworld::Intent::Insult
                              : choice == 3 ? socialworld::Intent::Apology
                                            : socialworld::Intent::Neutral);
    if (deferred) // The caller delivers the reply after a short, realistic delay.
        *deferred = reply;
    else if (reply)
        r.post(id, reply, false, 0, 0, call);
    return "";
}
} // namespace phonedialogue
