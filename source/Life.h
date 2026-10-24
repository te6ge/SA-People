#pragma once
#include "Encounters.h"
namespace life {
inline bool night(unsigned hour) {
    return hour >= 21 || hour < 5;
}
inline unsigned untilMorning(unsigned hour, unsigned minute) {
    return (8 * 60 + 1440 - ((hour % 24) * 60 + minute % 60)) % 1440;
}
inline bool nightCandidate(const people::Family &f, int type, unsigned hour, bool walking,
                           float distanceSquared) {
    return walking && night(hour) && distanceSquared >= 0 && distanceSquared <= 625 && f.female &&
           !f.origin && f.presence.alive && f.social.romance < 3 && !f.social.robbed &&
           !f.social.cutOff && f.social.anger < 30 &&
           !(f.social.events & encounters::bit(encounters::Kind::Visit)) &&
           type == 22 && // Only street workers; ordinary women do not proposition CJ.
           people::generate(f)[0].age >= 18;
}
struct NightRest {
    bool paid = false;
    bool pay(int &cash) {
        if (paid || cash < 80)
            return false;
        cash -= 80;
        paid = true;
        return true;
    }
};
inline unsigned rideFare(unsigned seed, float metres) {
    return 35 + people::mix(seed) % 91 +
           std::min(100u, static_cast<unsigned>(std::max(0.0f, metres)) / 20);
}
struct Ride {
    unsigned stage = 0, elapsed = 0;
    bool paid = false;
};
inline bool rideStep(Ride &r, unsigned elapsed, bool safe, bool aboard, bool arrived,
                     bool stopped) {
    if (!safe || r.paid)
        return false;
    r.elapsed += std::min(elapsed, 250u);
    if (r.stage == 1 && aboard) {
        r.stage = 2;
        r.elapsed = 0;
    }
    if (r.stage == 2 && !aboard) {
        r.stage = 4;
        return false;
    }
    if (r.stage == 2 && arrived && stopped) {
        r.stage = 3;
        return false;
    }
    if (r.stage == 3 && !aboard && arrived) {
        r.paid = true;
        r.stage = 4;
        return true;
    }
    if ((r.stage == 1 && r.elapsed > 30000) || r.elapsed > 600000)
        r.stage = 4;
    return false;
}
inline bool complete(people::Family &f, encounters::Kind kind) {
    return encounters::complete(f, kind);
}
} // namespace life
