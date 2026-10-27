#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <istream>
#include <ostream>
namespace people {
struct CommunityMemory {
    int opinion = 0;
    unsigned strain = 0, requests = 0, lastRequest = 0, serviceDay = 0, aidDay = 0;
    unsigned incident = 0, victim = 0, teller = 0, kind = 0, hops = 0, minute = 0, sharedAt = 0;
    unsigned drivingWarnings = 0, accidentAt = 0;
    bool empty() const {
        return !opinion && !(strain | requests | lastRequest | serviceDay | aidDay | incident |
                            victim | teller | kind | hops | minute | sharedAt | drivingWarnings | accidentAt);
    }
    bool valid(unsigned count) const {
        return opinion >= -100 && opinion <= 100 && strain <= 100 && requests <= 12 &&
            lastRequest <= 1000000001 && serviceDay <= 1000000 && aidDay <= 1000000 &&
            incident <= 1000000000 && victim <= count && teller <= count &&
            kind <= 4 && hops <= 2 && minute <= 1000000000 && sharedAt <= 1000000001 &&
            drivingWarnings <= 3 && accidentAt <= 1000000001 &&
            ((!incident && !kind && !victim && !teller && !hops) || (incident && kind && victim && teller));
    }
    void write(std::ostream &out) const {
        out << opinion << ' ' << strain << ' ' << requests << ' ' << lastRequest << ' '
            << serviceDay << ' ' << aidDay << ' ' << incident << ' ' << victim << ' ' << teller
            << ' ' << kind << ' ' << hops << ' ' << minute << ' ' << sharedAt << ' '
            << drivingWarnings << ' ' << accidentAt;
    }
    bool read(std::istream &in, unsigned count) {
        return static_cast<bool>(in >> opinion >> strain >> requests >> lastRequest >> serviceDay >> aidDay >>
            incident >> victim >> teller >> kind >> hops >> minute >> sharedAt >> drivingWarnings >> accidentAt) && valid(count);
    }
};
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
