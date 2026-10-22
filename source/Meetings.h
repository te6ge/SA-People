#pragma once
#include "Street.h"
#include <algorithm>
namespace meetings {
// Real San Andreas places for dates and meetups. Names are translation keys.
struct Venue {
    const char *name;
    float x, y, z;
};
inline const std::vector<Venue> &venues() {
    static const std::vector<Venue> list = {
        {"VENUE_ALHAMBRA", 1832.0f, -1682.0f, 13.4f}, {"VENUE_PIGPEN", 2421.0f, -1220.0f, 25.4f},
        {"VENUE_BOTTLES", 2310.0f, -1643.0f, 14.5f},  {"VENUE_PIZZA", 2105.0f, -1806.0f, 13.5f},
        {"VENUE_GYM", 2229.0f, -1722.0f, 13.5f},      {"VENUE_CLUCKIN", 2397.0f, -1898.0f, 13.5f},
        {"VENUE_GROVE", 2495.0f, -1688.0f, 13.5f},    {"VENUE_ZERO", -2240.0f, 128.0f, 35.3f},
        {"VENUE_JIZZY", -2630.0f, 1452.0f, 7.1f},     {"VENUE_BURGER", -2336.0f, -166.0f, 35.5f},
        {"VENUE_COMEALOT", 2036.0f, 1008.0f, 10.7f},  {"VENUE_CALIGULA", 2196.0f, 1677.0f, 12.4f},
        {"VENUE_HORSES", 1134.0f, 2060.0f, 10.8f}};
    return list;
}
// One of the three nearest venues that is a real trip away (120 m to 4 km).
inline const Venue *chooseVenue(float px, float py, unsigned seed) {
    std::vector<std::pair<float, const Venue *>> candidates;
    for (const auto &v : venues()) {
        float dx = v.x - px, dy = v.y - py, d = dx * dx + dy * dy;
        if (d >= 120.0f * 120.0f && d <= 4000.0f * 4000.0f)
            candidates.push_back({d, &v});
    }
    if (candidates.empty())
        return nullptr;
    std::sort(candidates.begin(), candidates.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });
    return candidates[people::mix(seed) % std::min<std::size_t>(3, candidates.size())].second;
}
inline const char *venueAt(float x, float y) {
    for (const auto &v : venues())
        if ((v.x - x) * (v.x - x) + (v.y - y) * (v.y - y) < 60.0f * 60.0f)
            return v.name;
    return "";
}
inline const Venue *activityVenue(float x, float y, unsigned seed, unsigned activity) {
    if (activity == 1) {
        for (const auto &v : venues())
            if (std::string(v.name) == "VENUE_BOTTLES")
                return &v;
    }
    if (activity == 0) {
        const Venue *best = nullptr;
        float distance = 1e12f;
        for (const auto &v : venues()) {
            std::string name = v.name;
            if (name != "VENUE_PIZZA" && name != "VENUE_CLUCKIN" && name != "VENUE_BURGER")
                continue;
            float dx = v.x - x, dy = v.y - y, d = dx * dx + dy * dy;
            if (d < distance) {
                distance = d;
                best = &v;
            }
        }
        if (best)
            return best;
    }
    return chooseVenue(x, y, seed);
}
inline bool canInvite(const people::Family &f) {
    return f.presence.alive && f.presence.located && f.presence.area == 0 && f.social.journal &&
           f.social.trust >= 20 && f.social.anger < 25 && f.social.resentment < 30 &&
           !f.social.robbed;
}
inline bool active(const people::Appointment &a) {
    return a.state == 1 || a.state == 2;
}
inline bool schedule(people::Registry &registry, unsigned id, float x, float y, float z,
                     unsigned delay = 60000, unsigned window = 300000) {
    if (!id || id > registry.families.size() || !canInvite(registry.families[id - 1]))
        return false;
    for (auto &a : registry.appointments)
        if (a.person == id) {
            if (active(a))
                return false;
            a = {id, 1, delay, window, x, y, z};
            return true;
        }
    if (registry.appointments.size() >= 8) {
        auto it = std::find_if(registry.appointments.begin(), registry.appointments.end(),
                               [](const auto &a) { return !active(a); });
        if (it == registry.appointments.end())
            return false;
        registry.appointments.erase(it);
    }
    registry.appointments.push_back({id, 1, delay, window, x, y, z});
    return true;
}
inline unsigned resume(people::Registry &r, std::uint64_t now) {
    unsigned expired = 0;
    std::uint64_t offline =
        now >= r.savedAt ? std::min<std::uint64_t>(86400000, (now - r.savedAt) * 1000) : 0;
    for (auto &a : r.appointments) {
        if (!active(a))
            continue;
        if (!r.savedAt) {
            a.state = 4;
            ++expired;
            continue;
        }
        auto elapsed = offline;
        if (a.state == 1) {
            if (elapsed < a.remaining) {
                a.remaining -= static_cast<unsigned>(elapsed);
                continue;
            }
            elapsed -= a.remaining;
            a.remaining = 0;
            a.state = 2;
        }
        if (elapsed >= a.window) {
            a.window = 0;
            a.state = 4;
            ++expired;
        } else
            a.window -= static_cast<unsigned>(elapsed);
    }
    r.savedAt = now;
    return expired;
}
inline bool step(people::Appointment &a, unsigned elapsed, bool safe) {
    if (!safe || !active(a))
        return false;
    elapsed = std::min(elapsed, 250u);
    if (a.state == 1) {
        if (elapsed >= a.remaining) {
            a.remaining = 0;
            a.state = 2;
            return true;
        }
        a.remaining -= elapsed;
    } else {
        if (elapsed >= a.window) {
            a.window = 0;
            a.state = 4;
            return true;
        }
        a.window -= elapsed;
    }
    return false;
}
inline bool arrive(people::Appointment &a, people::Family &f, bool together,
                   bool romanceAllowed = true) {
    if (a.state != 2 || !together)
        return false;
    a.state = 3;
    f.social.trust = std::min(100, f.social.trust + 5);
    f.social.encounters = std::min(1000000u, f.social.encounters + 1);
    f.meetings = std::min(1000000u, f.meetings + 1);
    f.social.resentment = f.social.resentment > 5 ? f.social.resentment - 5 : 0;
    street::addMeaning(f, 2, 10);
    if (romanceAllowed && f.female && !f.origin && !people::hasSpouse(f) && f.social.trust >= 45) {
        f.social.dates = std::min(1000000u, f.social.dates + 1);
        f.social.romance = std::min(3u, f.social.romance + 1);
    }
    return true;
}
} // namespace meetings
