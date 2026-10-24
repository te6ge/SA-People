#pragma once
#include "Network.h"
#include "Meetings.h"
namespace socialworld {
enum class Activity : unsigned { Meal, Pool, Drive, Walk };
enum class Intent : unsigned { Neutral, Friendly, Insult, Threat, Apology };
inline unsigned phase(unsigned minute) {
    unsigned hour = (minute / 60) % 24;
    return hour >= 8 && hour < 17 ? 1u : hour >= 17 && hour < 22 ? 2u : 0u;
}
inline people::Point destination(const people::Family &f, unsigned minute,
                                 std::uint64_t world = 0) {
    if (f.daily.busyUntil > world)
        return f.daily.home;
    return phase(minute) == 1 ? f.daily.work : phase(minute) == 2 ? f.daily.leisure : f.daily.home;
}
inline bool available(const people::Family &f) {
    return f.presence.alive && f.presence.located && !f.social.cutOff && !f.social.robbed &&
           f.social.anger < 30 && f.social.resentment < 40;
}
inline bool canHangOut(const people::Family &f) {
    return available(f) && f.social.journal && f.social.trust >= 35;
}
inline void meetingArgument(people::Family &f) {
    f.social.anger = std::max(30u, f.social.anger);
    f.social.resentment = std::max(15u, f.social.resentment);
    f.daily.memories |= 2;
}
struct Travel {
    people::Point previous{};
    float distance = 0;
    bool tracking = false;
    void sample(people::Point point, bool together, float maxSegment) {
        if (tracking && together) {
            const float dx = point.x - previous.x, dy = point.y - previous.y;
            const float segment = std::sqrt(dx * dx + dy * dy);
            if (segment < maxSegment)
                distance += segment;
        }
        previous = point;
        tracking = together;
    }
};
inline unsigned unread(const people::Registry &r) {
    unsigned count = 0;
    for (const auto &m : r.letters)
        if (m.unread && !m.outgoing)
            ++count;
    return count;
}
inline bool remoteEffect(people::Registry &r, unsigned id, Intent intent) {
    if (!id || id > r.families.size())
        return false;
    auto &f = r.families[id - 1];
    auto &s = f.social;
    if (!f.presence.alive || s.cutOff ||
        (f.daily.lastRemote && r.worldMinutes < f.daily.lastRemote + 30))
        return false;
    if (intent == Intent::Neutral || intent == Intent::Friendly)
        return false;
    f.daily.lastRemote =
        static_cast<unsigned>(std::min<std::uint64_t>(1000000000, r.worldMinutes + 1));
    if (intent == Intent::Insult || intent == Intent::Threat) {
        bool threat = intent == Intent::Threat;
        s.trust = std::max(-100, s.trust - (threat ? 12 : 6));
        s.anger = std::min(100u, s.anger + (threat ? 22 : 12));
        s.resentment = std::min(100u, s.resentment + (threat ? 14 : 8));
        s.provokes = std::min(255u, s.provokes + 1);
        f.daily.memories |= 2;
        if (s.provokes >= 3) {
            s.cutOff = 1;
            s.romance = 0;
        }
        network::news(r, id, true);
    } else if (intent == Intent::Apology && s.anger && s.resentment < 55) {
        s.anger = std::max(s.resentment * 3 / 5, s.anger > 8 ? s.anger - 8 : 0);
    }
    return true;
}
inline const char *memory(const people::Family &f) {
    return f.social.robbed          ? "MEM_ROBBERY"
           : f.social.helped        ? "MEM_BATTERY"
           : (f.daily.memories & 2) ? "MEM_ARGUMENT"
           : f.daily.outings        ? "MEM_OUTING"
                                    : "";
}
inline bool complete(people::Registry &r, people::Appointment &a, people::Family &f, bool good,
                     bool romanceAllowed) {
    if (a.state != 2)
        return false;
    if (!good) {
        a.state = 4;
        f.social.trust = std::max(-100, f.social.trust - 6);
        f.social.resentment = std::min(100u, f.social.resentment + 6);
        f.daily.memories |= 2;
        return true;
    }
    if (!meetings::arrive(a, f, true, romanceAllowed))
        return false;
    f.daily.outings = std::min(1000000u, f.daily.outings + 1);
    f.daily.memories |= 4;
    f.daily.busyUntil = static_cast<unsigned>(std::min<std::uint64_t>(1000000000, r.worldMinutes + 180));
    f.daily.lastInvitation = static_cast<unsigned>(r.worldMinutes / 1440 + 1);
    static const char *pool[] = {"MAIL_GOOD_OUTING", "MAIL_GOOD_OUTING_1", "MAIL_GOOD_OUTING_2", "MAIL_GOOD_OUTING_3"};
    r.post(f.id, pool[(f.daily.outings - 1) % 4], false, 4);
    return true;
}
inline bool spontaneous(people::Registry &r, people::Family &f, unsigned minute) {
    unsigned day = static_cast<unsigned>(r.worldMinutes / 1440 + 1);
    if (!canHangOut(f) || f.daily.busyUntil > r.worldMinutes || minute < 9 * 60 ||
        minute > 21 * 60 || f.daily.lastInvitation == day)
        return false;
    f.daily.lastInvitation = day;
    bool family = people::mix(f.id + day) % 5 == 0 && f.origin == 0 &&
                  (people::hasSpouse(f) || people::hasChild(f));
    if (family)
        f.daily.busyUntil =
            static_cast<unsigned>(std::min<std::uint64_t>(1000000000, r.worldMinutes + 180));
    r.post(f.id, family ? "MAIL_FAMILY_PLANS" : "MAIL_INVITATION", false, family ? 3u : 2u);
    return true;
}
} // namespace socialworld
