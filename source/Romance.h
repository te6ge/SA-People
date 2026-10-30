#pragma once
#include "People.h"
#include <algorithm>
namespace romance {
inline bool available(const people::Family &f) {
    return f.female && !f.origin && !people::hasSpouse(f) && f.presence.alive && !f.social.cutOff;
}
inline bool eligible(const people::Registry &r, const people::Family &f) {
    if (!available(f))
        return false;
    for (const auto &other : r.families)
        if (other.id != f.id && (other.social.romance >= 3 || other.commitment.stage) && other.presence.alive)
            return false;
    return true;
}
inline bool homeReady(const people::Family &f) {
    const auto &s = f.social;
    return available(f) && s.romance >= 3 && s.dates >= 4 && s.trust >= 60 && s.anger < 20 &&
           s.resentment < 20 && !s.cutOff && s.homeDate < s.dates;
}
inline bool homeCompleted(people::Family &f) {
    if (!homeReady(f))
        return false;
    f.social.homeDate = f.social.dates;
    f.social.homeVisits = std::min(1000000u, f.social.homeVisits + 1);
    return true;
}
inline bool plan(people::Family &f) {
    auto &s = f.social;
    if (!available(f) || s.romance < 3 || s.dates < 5 || s.homeVisits < 2 || s.trust < 70 ||
        s.anger >= 20 || s.resentment >= 20 || s.cutOff || s.childStage)
        return false;
    s.childStage = 1;
    s.childMinutes = 0;
    return true;
}
inline bool advance(people::Family &f, unsigned minutes) {
    auto &s = f.social;
    if (s.childStage != 1 || !f.presence.alive)
        return false;
    s.childMinutes += std::min(minutes, 10080u - s.childMinutes);
    if (s.childMinutes < 10080)
        return false;
    s.childStage = 2;
    return true;
}
inline std::string childName(const people::Family &f) {
    return people::mix(f.id + 1709) % 2 ? "Alex Johnson" : "Morgan Johnson";
}
} // namespace romance
