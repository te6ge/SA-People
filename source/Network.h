#pragma once
#include "People.h"
#include <algorithm>
namespace network {
inline unsigned degree(const people::Registry &r, unsigned id) {
    unsigned n = 0;
    for (const auto &e : r.connections)
        if (e.a == id || e.b == id)
            ++n;
    return n;
}
inline bool link(people::Registry &r, unsigned a, unsigned b, unsigned kind) {
    if (!a || !b || a == b || a > r.families.size() || b > r.families.size() || kind > 6 ||
        degree(r, a) >= 8 || degree(r, b) >= 8 || r.connections.size() >= 65536)
        return false;
    if (a > b)
        std::swap(a, b);
    for (const auto &e : r.connections)
        if (e.a == a && e.b == b)
            return false;
    r.connections.push_back({a, b, kind});
    return true;
}
inline void discover(people::Registry &r, unsigned id) {
    if (!id || id > r.families.size())
        return;
    const auto root = r.families[id - 1];
    if (!root.origin && (root.social.known & people::Name) && root.presence.located &&
        !root.presence.area) {
        for (unsigned role = 1; role <= 5 && r.families.size() < people::MaxFamilies; ++role) {
            if ((role == 3 && !people::hasSpouse(root)) || (role == 5 && !people::hasChild(root)))
                continue;
            bool exists = false;
            for (const auto &f : r.families)
                if (f.origin == id && f.role == role) {
                    exists = true;
                    break;
                }
            if (exists)
                continue;
            const bool female = role == 1 || (role == 3   ? !root.female
                                              : role == 4 ? root.female
                                              : role == 5 ? (people::mix(id) & 1) != 0
                                                          : false);
            auto &relative = r.create(female);
            relative.origin = id;
            relative.role = role;
            relative.originFemale = root.female;
            relative.presence = root.presence;
            relative.presence.alive = true;
            relative.presence.health = 100;
            relative.presence.model = role == 1 ? 10 : role == 2 ? 14 : female ? 41 : 17;
            relative.presence.type = female ? 5 : 4;
            const float angle = role * 1.256637f;
            relative.presence.x += std::cos(angle) * 52;
            relative.presence.y += std::sin(angle) * 52;
            link(r, id, relative.id,
                 role == 1   ? 3u
                 : role == 2 ? 4u
                 : role == 3 ? 6u
                 : role == 4 ? 1u
                             : 5u);
        }
    }
    if (!root.origin) {
        unsigned ids[6] = {id, 0, 0, 0, 0, 0};
        for (const auto &f : r.families)
            if (f.origin == id && f.role <= 5)
                ids[f.role] = f.id;
        if (ids[1] && ids[2])
            link(r, ids[1], ids[2], 6);
        if (ids[1] && ids[4])
            link(r, ids[1], ids[4], 5);
        if (ids[2] && ids[4])
            link(r, ids[2], ids[4], 5);
        if (ids[3] && ids[5])
            link(r, ids[3], ids[5], 5);
    }
    for (const auto &e : r.connections)
        if ((e.a == id || e.b == id) && (e.kind == 0 || e.kind == 2))
            return;
    const auto &p = r.families[id - 1].presence;
    unsigned best = 0;
    float distance = 180 * 180;
    for (const auto &f : r.families) {
        if (f.id == id || f.origin == id || f.origin || !f.presence.alive || !f.presence.located ||
            f.presence.area != p.area)
            continue;
        float dx = f.presence.x - p.x, dy = f.presence.y - p.y;
        float d = dx * dx + dy * dy;
        if (d < distance && degree(r, f.id) < 8) {
            distance = d;
            best = f.id;
        }
    }
    if (best) {
        // Named family ties remain in the existing family tree. Street links join actual actors.
        unsigned kind = people::mix(id ^ (best * 317)) % 5 == 0 ? 2u : 0u;
        link(r, id, best, kind);
    }
}
inline const char *label(const people::Connection &e, unsigned person) {
    if (e.kind == 0)
        return "NET_FRIEND";
    if (e.kind == 1)
        return "NET_SIBLING";
    if (e.kind == 2)
        return "NET_RIVAL";
    if (e.kind == 6)
        return "PHONE_SPOUSE";
    if (person == e.a)
        return e.kind == 3 ? "PHONE_MOTHER" : e.kind == 4 ? "PHONE_FATHER" : "PHONE_CHILD";
    return e.kind == 5 ? "NET_PARENT" : "PHONE_CHILD";
}
inline void news(people::Registry &r, unsigned source, bool harm, bool severe = false) {
    for (const auto &e : r.connections) {
        unsigned id = e.a == source ? e.b : e.b == source ? e.a : 0;
        if (!id || !r.families[id - 1].presence.alive)
            continue;
        auto &s = r.families[id - 1].social;
        int change = harm ? (severe ? -25 : -8) : 6;
        if (e.kind == 2)
            change = -change / 2;
        s.trust = std::clamp(s.trust + change, -100, 100);
        if (change < 0)
            s.resentment = std::min(100u, s.resentment + static_cast<unsigned>(-change));
        if (change < 0)
            s.anger = std::min(100u, s.anger + (severe ? 18u : 4u));
        s.newsFrom = source;
        s.newsKind = change < 0 ? (severe ? 3u : 1u) : 2u;
        if (s.journal &&
            (r.letters.empty() || r.letters.back().person != id ||
             r.letters.back().source != source || r.letters.back().minute + 30 < r.worldMinutes))
            r.post(id, change < 0 ? "MAIL_NETWORK_WARNING" : "MAIL_NETWORK_THANKS", false, 1,
                   source);
    }
}
// Killing someone CJ had met: their close family declares a blood feud.
inline unsigned feud(people::Registry &r, unsigned victim) {
    unsigned count = 0;
    for (const auto &e : r.connections) {
        if (e.kind == 0 || e.kind == 2)
            continue;
        unsigned id = e.a == victim ? e.b : e.b == victim ? e.a : 0;
        if (!id || !r.families[id - 1].presence.alive)
            continue;
        auto &s = r.families[id - 1].social;
        s.feud = 1;
        s.cutOff = 1;
        s.trust = -100;
        s.resentment = 100;
        s.anger = 100;
        ++count;
    }
    return count;
}
inline void honor(people::Registry &r, int delta) {
    r.honor = std::clamp(r.honor + delta, -100, 100);
}
} // namespace network
