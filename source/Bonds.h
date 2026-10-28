#pragma once
#include "Later.h"
#include <algorithm>
#include <array>
// What CJ's life looks like beyond single encounters: how each part of town thinks of him, what
// his week was like, the favor a friend asks of him and friends' birthdays. Pure logic; the
// game-side wiring lives in Bonds.inc.
namespace bonds {
// ---- Districts ---------------------------------------------------------------------------
// Approximate map boxes (world X/Y). District 0 is everywhere else (countryside, small towns).
struct Box {
    unsigned district;
    float x0, y0, x1, y1;
};
inline const Box boxes[] = {
    {1, 2222, -1852, 2632, -1628},  // Ganton / Grove Street
    {2, 1812, -1852, 2222, -1577},  // Idlewood
    {3, 1812, -1577, 2222, -1100},  // Jefferson, Glen Park
    {4, 2222, -1628, 2997, -1000},  // East Los Santos, Las Colinas
    {9, 1300, -2892, 2200, -2150},  // Los Santos airport
    {5, 1812, -2150, 2997, -1852},  // El Corona, Willowfield
    {5, 2200, -2892, 2997, -2150},  // Ocean Docks
    {6, 1300, -1852, 1812, -1000},  // Downtown, Commerce, Pershing Square
    {7, 787, -1100, 1812, -500},    // Vinewood, Mulholland
    {8, 44, -2150, 1300, -1100},    // Rodeo, Marina, Verona and Santa Maria Beach
    {10, -2997, -1250, -1213, 1700}, // San Fierro
    {11, 869, 596, 2997, 2993},     // Las Venturas
};
inline unsigned district(float x, float y) {
    for (const auto &b : boxes)
        if (x >= b.x0 && x < b.x1 && y >= b.y0 && y < b.y1)
            return b.district;
    return 0;
}
inline const char *districtName(unsigned d) {
    static const char *const keys[people::DistrictCount] = {
        "DIST_0", "DIST_1", "DIST_2", "DIST_3", "DIST_4",  "DIST_5",
        "DIST_6", "DIST_7", "DIST_8", "DIST_9", "DIST_10", "DIST_11"};
    return keys[d < people::DistrictCount ? d : 0];
}
constexpr int Loved = 40, Feared = -40;
// Only incidents with an actual witness contribute to the district summary.
inline void deed(people::Registry &r, unsigned d, int delta) {
    if (d >= people::DistrictCount || !delta)
        return;
    r.districts[d] = std::clamp(r.districts[d] + delta * 3, -100, 100);
}
// Opinions soften a little every game day.
inline void fade(people::Registry &r) {
    for (auto &d : r.districts)
        d += d > 0 ? -1 : d < 0 ? 1 : 0;
}
// Locals in a district that likes CJ give him a friend's price.
inline unsigned price(const people::Registry &r, unsigned d, unsigned full) {
    int rep = d < people::DistrictCount ? r.districts[d] : 0;
    return rep >= 70 ? full / 2 : rep >= Loved ? full * 3 / 4 : full;
}
inline const char *mood(int rep) {
    return rep >= 70        ? "DIST_MOOD_HERO"
           : rep >= Loved   ? "DIST_MOOD_LIKED"
           : rep <= -70     ? "DIST_MOOD_HATED"
           : rep <= Feared  ? "DIST_MOOD_FEARED"
                            : "DIST_MOOD_NEUTRAL";
}
inline bool localReaction(const people::Family &f, int districtOpinion) {
    if (!f.community.incident) return false;
    return districtOpinion >= Loved ? f.community.opinion >= 10 :
        districtOpinion <= Feared && f.community.opinion <= -10 && f.social.trust < 30;
}
// ---- Week --------------------------------------------------------------------------------
enum class Stat { Met, Helped, Robbed, Outing, Gift };
inline std::uint64_t weekOf(std::uint64_t minutes) {
    return minutes / 10080;
}
inline people::WeekStats current(const people::Registry &r) {
    return r.week.week == weekOf(r.worldMinutes) ? r.week : people::WeekStats{weekOf(r.worldMinutes)};
}
inline void count(people::Registry &r, Stat s) {
    r.week = current(r);
    auto &w = r.week;
    unsigned &field = s == Stat::Met       ? w.met
                      : s == Stat::Helped  ? w.helped
                      : s == Stat::Robbed  ? w.robbed
                      : s == Stat::Outing  ? w.outings
                                           : w.gifts;
    field = std::min(1000000u, field + 1);
}
inline bool contact(const people::Family &f) {
    return f.social.journal && f.presence.alive && !f.social.cutOff;
}
inline unsigned bestFriend(const people::Registry &r) {
    unsigned best = 0;
    for (const auto &f : r.families)
        if (contact(f) && f.social.trust > 0 &&
            (!best || f.social.trust > r.families[best - 1].social.trust))
            best = f.id;
    return best;
}
inline unsigned enemy(const people::Registry &r) {
    unsigned worst = 0;
    for (const auto &f : r.families) {
        if (!f.presence.alive || !(f.social.known & people::Name))
            continue;
        int score = f.social.feud ? -200 : f.social.trust - static_cast<int>(f.social.resentment);
        if (score > -30)
            continue;
        int previous = worst ? (r.families[worst - 1].social.feud
                                    ? -200
                                    : r.families[worst - 1].social.trust -
                                          static_cast<int>(r.families[worst - 1].social.resentment))
                             : 0;
        if (!worst || score < previous)
            worst = f.id;
    }
    return worst;
}
inline unsigned partner(const people::Registry &r) {
    for (const auto &f : r.families)
        if (f.social.romance >= 3 && f.presence.alive)
            return f.id;
    return 0;
}
inline unsigned contacts(const people::Registry &r) {
    unsigned n = 0;
    for (const auto &f : r.families)
        n += contact(f);
    return n;
}
// ---- Birthdays ---------------------------------------------------------------------------
// San Andreas years are short here: 60 game days (one real day of play), so friends' birthdays
// actually come around. The date never changes and is the same on every machine.
constexpr unsigned Year = 60;
constexpr unsigned GiftKind = 8, NoticeKind = 9; // Day markers in the Later queue.
constexpr unsigned GiftPrice = 50;
inline unsigned birthday(const people::Family &f) {
    return people::mix(f.id * 13 + 7) % Year;
}
inline unsigned today(const people::Registry &r) {
    return static_cast<unsigned>((r.worldMinutes / 1440) % Year);
}
inline bool birthdayToday(const people::Registry &r, const people::Family &f) {
    return birthday(f) == today(r);
}
inline unsigned daysUntil(const people::Registry &r, const people::Family &f) {
    return (birthday(f) + Year - today(r)) % Year;
}
inline bool marked(const people::Registry &r, unsigned id, unsigned kind) {
    for (const auto &l : r.later)
        if (l.person == id && l.kind == kind && l.due > r.worldMinutes)
            return true;
    return false;
}
inline bool mark(people::Registry &r, unsigned id, unsigned kind) {
    if (r.later.size() >= later::Total)
        return false;
    r.later.push_back({id, kind, 0, (r.worldMinutes / 1440 + 1) * 1440});
    return true;
}
inline bool friendly(const people::Family &f, int trust) {
    const auto &s = f.social;
    return contact(f) && !s.feud && !s.robbed && s.anger < 30 && s.trust >= trust;
}
// The friend texts CJ on the morning of their birthday, once.
inline bool invite(people::Registry &r, people::Family &f) {
    if (!birthdayToday(r, f) || !friendly(f, 20) || marked(r, f.id, NoticeKind) ||
        !mark(r, f.id, NoticeKind))
        return false;
    r.post(f.id, "MAIL_BDAY_" + std::to_string(people::mix(f.id + today(r)) % 3));
    return true;
}
// An explicit in-person gift confirmation is required before spending $50.
inline bool canGift(const people::Registry &r, unsigned id) {
    return id && id <= r.families.size() && birthdayToday(r, r.families[id-1]) &&
           friendly(r.families[id-1], 0) && !marked(r, id, GiftKind);
}
inline bool gift(people::Registry &r, people::Family &f, int &money, bool confirmed = false) {
    if (!confirmed || !canGift(r, f.id) || money < 0 ||
        static_cast<unsigned>(money) < GiftPrice || !mark(r, f.id, GiftKind))
        return false;
    money -= static_cast<int>(GiftPrice);
    f.social.trust = std::min(100, f.social.trust + 12);
    count(r, Stat::Gift);
    r.post(f.id, "MAIL_BDAY_THANKS");
    return true;
}
// ---- Friend requests ---------------------------------------------------------------------
// A close friend asks CJ for something. Saying no (or ignoring it) costs nothing; accepting and
// then letting the deadline pass costs trust. Only one request is open at a time.
enum Kind : unsigned { None, Ride, TalkTo, Lend };
constexpr unsigned AnswerMinutes = 180, DoMinutes = 720;
inline unsigned talkTarget(const people::Registry &r, unsigned id) {
    for (const auto &e : r.connections) {
        unsigned other = e.a == id ? e.b : e.b == id ? e.a : 0;
        if (other && e.kind != 2 && r.families[other - 1].presence.alive &&
            r.families[other - 1].presence.located && !r.families[other - 1].presence.area)
            return other;
    }
    return 0;
}
inline const char *letter(Kind k) {
    return k == Ride ? "MAIL_REQ_RIDE" : k == TalkTo ? "MAIL_REQ_TALK" : "MAIL_REQ_LEND";
}
inline bool ask(people::Registry &r, unsigned id, unsigned seed) {
    if (r.request.state || !id || id > r.families.size())
        return false;
    auto &f = r.families[id - 1];
    if (!friendly(f, 35) || f.origin)
        return false;
    unsigned target = talkTarget(r, id);
    Kind kinds[3] = {Ride, TalkTo, Lend};
    Kind kind = Lend;
    for (unsigned i = 0; i < 3; ++i) {
        Kind k = kinds[(seed + i) % 3];
        if ((k == Ride && !f.daily.ready) || (k == TalkTo && !target))
            continue;
        kind = k;
        break;
    }
    r.request = {id, kind, kind == TalkTo ? target : 0u, 1, kind == Lend ? 50 + seed % 151 : 0u,
                 std::min<std::uint64_t>(1000000000, r.worldMinutes + AnswerMinutes)};
    r.post(id, letter(kind), false, 5, r.request.target);
    return true;
}
inline bool open(const people::Registry &r, unsigned id) {
    return id && id <= r.families.size() && r.request.person == id && r.request.state == 1 &&
           r.request.due > r.worldMinutes && friendly(r.families[id-1], 0);
}
// Accepting: a loan is paid on the spot (and comes back later); a ride or a talk starts a clock.
inline const char *accept(people::Registry &r, int &money) {
    auto &q = r.request;
    if (!open(r, q.person))
        return nullptr;
    auto &f = r.families[q.person - 1];
    if (q.kind == Lend) {
        if (money < 0 || static_cast<unsigned>(money) < q.amount)
            return "REQ_NO_MONEY";
        money -= static_cast<int>(q.amount);
        f.social.trust = std::min(100, f.social.trust + 6);
        // The principal stays in the existing persistent request record. This does not
        // consume a Later queue slot and cannot silently lose a full-queue repayment.
        q.state = 3;
        q.due = std::min<std::uint64_t>(1000000000, r.worldMinutes + 480 +
            people::mix(f.id + static_cast<unsigned>(r.worldMinutes)) % 720);
        count(r, Stat::Helped);
        r.post(f.id, "MAIL_REQ_LEND_THANKS");
        return "REQ_LENT";
    }
    q.state = 2;
    q.due = std::min<std::uint64_t>(1000000000, r.worldMinutes + DoMinutes);
    r.post(f.id, "MAIL_REQ_ACCEPTED");
    return q.kind == Ride ? "REQ_RIDE_ACCEPTED" : "REQ_TALK_ACCEPTED";
}
// Return the exact principal once; debt survives save/load and a full event queue.
inline unsigned repay(people::Registry &r, int &money) {
    auto &q = r.request;
    if (q.state != 3 || q.kind != Lend || q.due > r.worldMinutes || money < 0 || money > 999999999 ||
        q.amount > static_cast<unsigned>(999999999 - money)) return 0;
    const unsigned amount = q.amount;
    money += static_cast<int>(amount);
    r.post(q.person, "MAIL_REQ_REPAID");
    q = {};
    return amount;
}
inline bool decline(people::Registry &r, unsigned id) {
    if (!open(r, id)) return false;
    r.post(id, "MAIL_REQ_DECLINED");
    r.request = {};
    return true;
}
// Done: the ride ended at its destination, or CJ greeted the person he was asked to see.
inline bool complete(people::Registry &r, Kind kind, unsigned who) {
    auto &q = r.request;
    if (q.state != 2 || q.kind != kind || (kind == Ride ? q.person : q.target) != who)
        return false;
    auto &f = r.families[q.person - 1];
    f.social.trust = std::min(100, f.social.trust + 8);
    count(r, Stat::Helped);
    r.post(f.id, kind == Ride ? "MAIL_REQ_RIDE_THANKS" : "MAIL_REQ_TALK_THANKS", false, 0,
           q.target);
    q = {};
    return true;
}
// Unanswered requests lapse quietly; promises CJ did not keep hurt.
inline bool expire(people::Registry &r) {
    auto &q = r.request;
    if (!q.state || q.state == 3 || q.due > r.worldMinutes)
        return false;
    bool broken = q.state == 2;
    if (broken && q.person <= r.families.size()) {
        auto &s = r.families[q.person - 1].social;
        s.trust = std::max(-100, s.trust - 8);
        s.resentment = std::min(100u, s.resentment + 6);
        r.post(q.person, "MAIL_REQ_LET_DOWN");
    }
    q = {};
    return broken;
}
} // namespace bonds
