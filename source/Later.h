#pragma once
#include "People.h"
#include "Network.h"
#include <algorithm>
// Delayed consequences: what CJ did comes back hours later in game time. Someone he helped
// texts a thank-you (a close contact may send money); someone he robbed sends people after him.
namespace later {
enum Kind : unsigned { Thanks = 1, Gift = 2, Revenge = 3, Cooldown = 4, Visit = 5,
    FollowUp = 6, Introduction = 7 };
constexpr unsigned StoryStarted = 1u, StoryFinished = 8u;
constexpr std::size_t Limit = 32;    // Pending thanks, gifts and revenges.
constexpr std::size_t Total = 64;    // Including favor cooldowns.
constexpr std::uint64_t RevengeExpiry = 1440; // A revenge that never found CJ fades after a day.
inline std::size_t pending(const people::Registry &r) {
    std::size_t n = 0;
    for (const auto &l : r.later)
        n += l.kind != Cooldown;
    return n;
}
// A contact who did CJ a favor (Favors.h) cannot be asked again for a game day.
inline bool cooling(const people::Registry &r, unsigned person) {
    for (const auto &l : r.later)
        if (l.kind == Cooldown && l.person == person && l.due > r.worldMinutes)
            return true;
    return false;
}
inline bool cooldown(people::Registry &r, unsigned person, std::uint64_t minutes = 1440) {
    if (!person || person > r.families.size() || r.later.size() >= Total)
        return false;
    r.later.push_back({person, Cooldown, 0, std::min<std::uint64_t>(1000000000, r.worldMinutes + minutes)});
    return true;
}
inline bool schedule(people::Registry &r, unsigned person, unsigned kind, unsigned source = 0) {
    if (!person || person > r.families.size() || source > r.families.size() || kind < Thanks ||
        (kind > Revenge && kind != Visit && kind != FollowUp && kind != Introduction) ||
        pending(r) >= Limit || r.later.size() >= Total)
        return false;
    for (const auto &l : r.later)
        if (l.person == person && l.kind == kind)
            return false;
    unsigned seed = people::mix(person * 131 + kind * 7 + static_cast<unsigned>(r.worldMinutes));
    unsigned delay = kind == FollowUp || kind == Introduction ? 1440 + seed % 720
                     : kind == Visit ? 180 + seed % 360
                     : kind == Revenge ? 120 + seed % 360   // 2-8 hours
                     : kind == Gift  ? 480 + seed % 720   // 8-20 hours
                                     : 240 + seed % 480;  // 4-12 hours
    r.later.push_back({person, kind, source, std::min<std::uint64_t>(1000000000, r.worldMinutes + delay)});
    return true;
}
// After a good deed: a contact who already trusts CJ sometimes sends money with the thanks.
inline unsigned helped(people::Registry &r, unsigned person) {
    if (!person || person > r.families.size())
        return 0;
    const auto &s = r.families[person - 1].social;
    unsigned kind = s.trust >= 30 && people::mix(person * 977 + s.encounters) % 3 == 0 ? Gift : Thanks;
    const unsigned result = schedule(r, person, kind) ? kind : 0;
    auto &f = r.families[person - 1];
    if (!(f.daily.memories & StoryStarted) && schedule(r, person, FollowUp))
        f.daily.memories |= StoryStarted;
    return result;
}
// After a robbery: a little over half of the victims send someone to settle it.
inline bool robbed(people::Registry &r, unsigned victim) {
    if (!victim || victim > r.families.size() ||
        people::mix(victim * 389 + static_cast<unsigned>(r.worldMinutes)) % 100 >= 55)
        return false;
    return schedule(r, victim, Revenge, victim);
}
// Takes the next due consequence. Revenge waits until the caller can stage it (or expires).
inline bool next(people::Registry &r, bool revengeReady, people::Later &out) {
    for (std::size_t i = 0; i < r.later.size();) {
        const auto &l = r.later[i];
        if (l.due > r.worldMinutes) {
            ++i;
            continue;
        }
        if (l.kind == Cooldown || (l.kind == Revenge && r.worldMinutes > l.due + RevengeExpiry)) {
            r.later.erase(r.later.begin() + static_cast<std::ptrdiff_t>(i));
            continue;
        }
        if (l.kind == Revenge && !revengeReady) {
            ++i;
            continue;
        }
        out = l;
        r.later.erase(r.later.begin() + static_cast<std::ptrdiff_t>(i));
        return true;
    }
    return false;
}
// Thanks and gifts arrive as phone messages; nothing comes from someone CJ wronged since.
inline bool deliver(people::Registry &r, const people::Later &l, unsigned &gift) {
    gift = 0;
    if (l.kind == Revenge || l.kind == Cooldown || !l.person || l.person > r.families.size())
        return false;
    auto &f = r.families[l.person - 1];
    auto &s = f.social;
    if (!f.presence.alive || !s.journal || s.cutOff || s.robbed || s.feud || s.anger >= 40)
        return false;
    unsigned seed = people::mix(l.person * 53 + static_cast<unsigned>(l.due));
    if ((l.kind == FollowUp || l.kind == Introduction) &&
        (s.trust < 15 || s.resentment >= 40)) return false;
    if (l.kind == FollowUp) {
        if (f.daily.memories & StoryFinished) return false;
        if (people::mix(f.id) % 2 == 0 && s.trust >= 20 && schedule(r, f.id, Introduction))
            r.post(f.id, "MAIL_STORY_FRIEND");
        else {
            r.post(f.id, "MAIL_STORY_CLOSURE");
            f.daily.memories |= StoryFinished;
        }
    } else if (l.kind == Introduction) {
        if (f.daily.memories & StoryFinished) return false;
        f.daily.memories |= StoryFinished;
        unsigned friendId = 0;
        float nearest = 1000 * 1000;
        if (network::degree(r, f.id) < 8) for (const auto &candidate : r.families) {
            const auto &p = candidate.presence;
            if (candidate.id == f.id || candidate.origin || candidate.social.journal ||
                !p.alive || !p.located || p.area || (p.type != 4 && p.type != 5) ||
                candidate.social.trust < 0 || candidate.social.cutOff || candidate.social.feud ||
                candidate.social.robbed || candidate.social.anger || network::degree(r,candidate.id) >= 8) continue;
            const float dx=p.x-f.presence.x, dy=p.y-f.presence.y;
            if(dx*dx+dy*dy < nearest) {nearest=dx*dx+dy*dy; friendId=candidate.id;}
        }
        if (friendId && network::link(r, f.id, friendId, 0)) {
            auto &contact = r.families[friendId-1];
            contact.social.known |= people::Name;
            contact.social.journal = 1;
            contact.social.trust = std::max(20,contact.social.trust);
            contact.social.meaningful = std::max(3u,contact.social.meaningful);
            r.post(f.id,"MAIL_STORY_INTRODUCED",false,0,friendId);
            r.post(friendId,"MAIL_STORY_NEW_CONTACT",false,0,f.id);
        } else r.post(f.id,"MAIL_STORY_CLOSURE");
    } else if (l.kind == Visit) {
        static const char *pool[] = {"MAIL_VISIT_0", "MAIL_VISIT_1", "MAIL_VISIT_2"};
        r.post(l.person, pool[seed % 3]);
    } else if (l.kind == Gift && s.trust >= 30) {
        gift = 40 + seed % 111;
        r.post(l.person, "MAIL_LATER_GIFT");
    } else {
        static const char *thanks[] = {"MAIL_LATER_THANKS1", "MAIL_LATER_THANKS2",
                                       "MAIL_LATER_THANKS3"};
        r.post(l.person, thanks[seed % 3]);
    }
    s.trust = std::min(100, s.trust + 4);
    return true;
}
} // namespace later
