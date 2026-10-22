#pragma once
#include "SocialWorld.h"
namespace phonedialogue {
inline constexpr const char *choices[] = {"TXT_HELLO", "TXT_CHECK",  "TXT_MEMORY",
                                          "TXT_SORRY", "TXT_INSULT", "TXT_BYE"};
// Choices carry explicit meaning; no text classification or external service is involved.
inline const char *send(people::Registry &r, unsigned id, unsigned choice, bool call,
                        unsigned minute, const char **deferred = nullptr) {
    if (!id || id > r.families.size() || choice >= 6)
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
    const char *reply = "TXT_REPLY_BYE";
    unsigned variant = people::mix(id + r.nextLetter) % 3;
    bool cold = f.social.anger >= 25 || f.social.resentment >= 35 || f.social.robbed;
    if (choice == 0) {
        const char *lines[] = {"TXT_REPLY_HELLO_0", "TXT_REPLY_HELLO_1", "TXT_REPLY_HELLO_2"};
        reply = cold ? "TXT_REPLY_COLD" : lines[variant];
    } else if (choice == 1) {
        const char *lines[] = {"TXT_REPLY_CHECK_0", "TXT_REPLY_CHECK_1", "TXT_REPLY_CHECK_2"};
        reply = cold                                 ? "TXT_REPLY_COLD"
                : f.daily.busyUntil > r.worldMinutes ? "MAIL_FAMILY_PLANS"
                                                     : lines[variant];
    } else if (choice == 2) {
        reply = socialworld::memory(f);
        if (!*reply)
            reply = "TXT_REPLY_NO_MEMORY";
    } else if (choice == 3) {
        reply = f.social.resentment >= 55 ? "TXT_REPLY_NO_FORGIVE"
                : cold                    ? "TXT_REPLY_SORRY"
                                          : "TXT_REPLY_FINE";
    } else if (choice == 4)
        reply = "TXT_REPLY_INSULT";
    r.post(id, choices[choice], true, 0, 0, call);
    socialworld::remoteEffect(r, id,
                              choice == 4   ? socialworld::Intent::Insult
                              : choice == 3 ? socialworld::Intent::Apology
                                            : socialworld::Intent::Neutral);
    if (deferred) // The caller delivers the reply after a short, realistic delay.
        *deferred = reply;
    else
        r.post(id, reply, false, 0, 0, call);
    return "";
}
} // namespace phonedialogue
