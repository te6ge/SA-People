#pragma once
#include <algorithm>
#include <cstdint>
#include <deque>
#include <map>
#include <string>
namespace socialprogress {
// Each person counts once in each direction per ten minutes of real play time.
struct HonorBatch {
    std::map<unsigned, std::uint32_t> positive, negative;
    unsigned greetings = 0, provocations = 0;
    int record(unsigned person, bool greet, std::uint32_t now) {
        if (!person) return 0;
        auto &seen = greet ? positive : negative;
        for (auto it = seen.begin(); it != seen.end();)
            if (now - it->second >= 600000) it = seen.erase(it); else ++it;
        if (seen.count(person)) return 0;
        seen[person] = now;
        auto &count = greet ? greetings : provocations;
        if (++count < 3) return 0;
        count = 0;
        return greet ? 1 : -1;
    }
};
struct HonorSounds {
    std::deque<int> pending;
    void push(int cue) {
        cue = cue > 0 ? 1 : cue < 0 ? -1 : 0;
        if (!cue || (!pending.empty() && pending.back() == cue)) return;
        if (pending.size() == 4) pending.pop_front();
        pending.push_back(cue);
    }
    int take(bool ready) {
        if (!ready || pending.empty()) return 0;
        int result = pending.front(); pending.pop_front(); return result;
    }
};
struct LineDeck {
    unsigned used = 0, last = 31;
    unsigned take(unsigned count, unsigned seed) {
        count = std::clamp(count, 1u, 30u);
        const unsigned mask = (1u << count) - 1;
        if ((used & mask) == mask) used = 0;
        for (unsigned i = 0; i < count; ++i) {
            const unsigned n = (seed + i) % count;
            if (!(used & (1u << n)) && (count == 1 || n != last)) {
                used |= 1u << n; last = n; return n;
            }
        }
        // The final unused item can equal the last item after a pool size change.
        used = 0; last = (last + 1) % count; used |= 1u << last; return last;
    }
};
inline int openness(int honor) { return std::clamp(honor, -100, 100) / 4; }
inline bool availableForEvent(std::uint64_t busyUntil, std::uint64_t now,
                             bool appointment, bool companion, unsigned romance) {
    return busyUntil <= now && !appointment && !companion && romance < 3;
}
// No reward for attacking an innocent pedestrian with a particular appearance.
inline bool defended(bool activeOffender, bool damagedByCJ, float health) {
    return activeOffender && damagedByCJ && health <= 25.0f;
}
struct Cheat {
    std::string typed;
    std::uint32_t last = 0;
    bool letter(char c, std::uint32_t now) {
        if (now - last > 15000) typed.clear();
        last = now;
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + 'a' - 'A');
        if (c < 'a' || c > 'z') { typed.clear(); return false; }
        const std::string code = "imarthurmorgan";
        typed += c;
        while (!typed.empty() && code.compare(0, typed.size(), typed) != 0)
            typed.erase(0, 1);
        if (typed != code) return false;
        typed.clear(); return true;
    }
};
} // namespace socialprogress
