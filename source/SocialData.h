#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
namespace people {
struct Point {
    float x = 0, y = 0, z = 0;
};
inline bool valid(Point p) {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z) &&
           std::abs(p.x) < 30000 && std::abs(p.y) < 30000 && std::abs(p.z) < 10000;
}
struct Daily {
    bool ready = false;
    Point home{}, work{}, leisure{};
    unsigned lastInvitation = 0, lastRemote = 0, outings = 0, memories = 0;
    unsigned busyUntil = 0;
};
inline bool valid(const Daily &d) {
    return valid(d.home) && valid(d.work) && valid(d.leisure) && d.lastInvitation <= 1000000 &&
           d.lastRemote <= 1000000000 && d.outings <= 1000000 && d.memories <= 255 &&
           d.busyUntil <= 1000000000;
}
struct Letter {
    unsigned id = 0, person = 0, source = 0, kind = 0;
    std::uint64_t minute = 0;
    bool outgoing = false, unread = true, call = false;
    std::string body;
};
inline std::string cleanText(std::string s, std::size_t limit = 768) {
    for (auto &c : s)
        if (static_cast<unsigned char>(c) < 32)
            c = ' ';
    if (s.size() > limit) {
        std::size_t end = limit;
        while (end && (static_cast<unsigned char>(s[end]) & 0xC0) == 0x80)
            --end;
        s.resize(end);
    }
    return s;
}
} // namespace people
