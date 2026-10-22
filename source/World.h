#pragma once
#include "People.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

namespace people {
// Runtime pool handles are never persisted. Generations and models must match.
struct Handle {
    unsigned slot, generation;
    int model;
    bool operator==(const Handle &b) const {
        return slot == b.slot && generation == b.generation && model == b.model;
    }
};
class Bindings {
    struct Link {
        Handle handle;
        unsigned id;
    };
    std::unordered_map<unsigned, Link> slots;
    std::unordered_map<unsigned, unsigned> identities;

  public:
    unsigned find(Handle h) const {
        auto it = slots.find(h.slot);
        return it != slots.end() && it->second.handle == h ? it->second.id : 0;
    }
    bool active(unsigned id) const {
        return identities.count(id) != 0;
    }
    bool locate(unsigned id, Handle &result) const {
        auto it = identities.find(id);
        if (it == identities.end())
            return false;
        auto slot = slots.find(it->second);
        if (slot == slots.end())
            return false;
        result = slot->second.handle;
        return true;
    }
    void unbind(unsigned slot) {
        auto it = slots.find(slot);
        if (it != slots.end()) {
            identities.erase(it->second.id);
            slots.erase(it);
        }
    }
    bool bind(Handle h, unsigned id) {
        if (!id || (active(id) && find(h) != id))
            return false;
        unbind(h.slot);
        slots.emplace(h.slot, Link{h, id});
        identities[id] = h.slot;
        return true;
    }
    template <class Exists> void prune(Exists exists) {
        std::vector<unsigned> gone;
        for (const auto &item : slots)
            if (!exists(item.second.handle))
                gone.push_back(item.first);
        for (auto slot : gone)
            unbind(slot);
    }
    void clear() {
        slots.clear();
        identities.clear();
    }
};
// Reattach a known contact only when its nearby appearance has one unambiguous match.
// A skin alone is never an identity; simultaneously loaded lookalikes remain separate.
inline unsigned returningContact(const Registry &registry, const Bindings &bindings,
                                 const Presence &appearance,
                                 const std::vector<Presence> *unbound = nullptr) {
    unsigned match = 0;
    for (const auto &f : registry.families) {
        const auto &p = f.presence;
        if (!(f.social.known & Name) || bindings.active(f.id) || !p.alive || !p.located ||
            p.model != appearance.model || p.type != appearance.type || p.area != appearance.area)
            continue;
        const float dx = p.x - appearance.x, dy = p.y - appearance.y, dz = p.z - appearance.z;
        if (dx * dx + dy * dy > 30 * 30 || std::abs(dz) > 4)
            continue;
        if (match)
            return 0;
        match = f.id;
    }
    if (match && unbound) {
        const auto &p = registry.families[match - 1].presence;
        unsigned candidates = 0;
        for (const auto &a : *unbound) {
            float dx = a.x - p.x, dy = a.y - p.y;
            if (a.model == p.model && a.type == p.type && a.area == p.area &&
                dx * dx + dy * dy <= 30 * 30 && std::abs(a.z - p.z) <= 4)
                ++candidates;
        }
        if (candidates != 1)
            return 0;
    }
    return match;
}
// Area and coordinates control residency. City names never form part of identity.
inline bool canReturn(const Family &f, const Bindings &b, float x, float y, float z,
                      unsigned area) {
    const auto &p = f.presence;
    if (!p.located || !p.alive || p.area != area || area != 0 || b.active(f.id))
        return false;
    const float dx = p.x - x, dy = p.y - y, dz = p.z - z;
    const float d = dx * dx + dy * dy;
    return d >= 35 * 35 && d <= 85 * 85 && std::abs(dz) < 12;
}
class SpatialIndex {
    std::unordered_map<std::int64_t, std::unordered_set<unsigned>> cells;
    std::unordered_map<unsigned, std::int64_t> locations;
    static int cell(float x) {
        return static_cast<int>(std::floor(x / 128.0f));
    }
    static std::int64_t key(int x, int y) {
        return static_cast<std::int64_t>(
            (static_cast<std::uint64_t>(static_cast<std::uint32_t>(x)) << 32) |
            static_cast<std::uint32_t>(y));
    }

  public:
    void update(const Family &f) {
        auto old = locations.find(f.id);
        if (!f.presence.located)
            return;
        auto next = key(cell(f.presence.x), cell(f.presence.y));
        if (old != locations.end() && old->second == next)
            return;
        if (old != locations.end()) {
            auto c = cells.find(old->second);
            if (c != cells.end()) {
                c->second.erase(f.id);
                if (c->second.empty())
                    cells.erase(c);
            }
        }
        locations[f.id] = next;
        cells[next].insert(f.id);
    }
    std::vector<unsigned> nearby(float x, float y) const {
        std::vector<unsigned> result;
        int cx = cell(x), cy = cell(y);
        for (int dx = -1; dx <= 1; ++dx)
            for (int dy = -1; dy <= 1; ++dy) {
                auto c = cells.find(key(cx + dx, cy + dy));
                if (c != cells.end())
                    result.insert(result.end(), c->second.begin(), c->second.end());
            }
        std::sort(result.begin(), result.end()); // Stable ordering for reproducibility.
        return result;
    }
};
} // namespace people
