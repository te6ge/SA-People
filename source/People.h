#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <unordered_set>
#include <functional>
#include "SocialData.h"

namespace people {
constexpr unsigned MaxFamilies = 32768;
constexpr unsigned SoftLimit = 12000;
struct Presence {
    int model = -1, type = -1;
    float x = 0, y = 0, z = 0, heading = 0, health = 100;
    unsigned area = 0;
    bool located = false, alive = true;
};
inline bool valid(const Presence &p) {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z) &&
           std::isfinite(p.heading) && std::isfinite(p.health) && std::abs(p.x) < 30000 &&
           std::abs(p.y) < 30000 && std::abs(p.z) < 10000 && p.health >= 0 && p.health <= 10000 &&
           p.area <= 255 &&
           ((!p.located && p.model == -1 && p.type == -1) ||
            (p.model >= 0 && p.model <= 311 && p.type >= 4 && p.type <= 23));
}
enum Knowledge : unsigned {
    Name = 1,
    Job = 2,
    Age = 4,
    Mother = 8,
    Father = 16,
    Spouse = 32,
    Sibling = 64,
    Child = 128,
    Background = 256
};
constexpr unsigned AllKnowledge = 511;
struct Social {
    unsigned known = 0;
    int trust = 0;
    unsigned anger = 0, attempts = 0, lastReaction = 0;
    unsigned meaningful = 0, encounters = 0, helped = 0, lastNote = 0;
    bool journal = false;
    unsigned robbed = 0, events = 0; // One robbery attempt and each small encounter per identity.
    unsigned resentment = 0;
    unsigned newsFrom = 0, newsKind = 0;
    unsigned provokes = 0, cutOff = 0; // Persistent provocation count; cut-off ends talking.
    unsigned dates = 0, homeVisits = 0, homeDate = 0, childStage = 0, childMinutes = 0;
    unsigned feud = 0, romance = 0; // Blood feud after a killing; completed dates (3 = partner).
};
inline bool valid(const Social &s) {
    return s.known <= AllKnowledge && s.trust >= -100 && s.trust <= 100 && s.anger <= 100 &&
           s.attempts <= 1000000 && s.lastReaction <= 4 && s.meaningful <= 1000000 &&
           s.encounters <= 1000000 && s.helped <= 1000000 && s.lastNote <= 10 && s.robbed <= 1 &&
           s.events <= 127 && s.resentment <= 100 && s.newsFrom <= MaxFamilies && s.newsKind <= 3 &&
           s.provokes <= 255 && s.cutOff <= 1 && s.feud <= 1 && s.romance <= 3 &&
           s.dates <= 1000000 && s.homeVisits <= 1000000 && s.homeDate <= s.dates &&
           s.childStage <= 2 && s.childMinutes <= 10080;
}
struct Family {
    unsigned id;
    bool female;
    unsigned meetings;
    Presence presence{};
    Social social{};
    unsigned origin = 0, role = 0;
    Daily daily{};
    bool originFemale = false;
    unsigned household =
        0; // 0 legacy married parent; 1 single; 2 married parent; 3 separated parent.
};
struct Person {
    unsigned id;
    std::string name;
    unsigned age;
    std::string job;
    unsigned mother = 0, father = 0, spouse = 0;
    std::vector<unsigned> siblings, children;
};
inline unsigned mix(unsigned x) {
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    return x ^ (x >> 16);
}
inline std::array<Person, 6> generateBase(const Family &f) {
    if (!f.id || f.id > MaxFamilies)
        throw std::runtime_error("Invalid family ID");
    static const char *men[] = {
        "Miguel", "Carlos", "Luis",   "David",  "Marcus",  "James",  "Daniel", "Robert", "Andre",
        "Victor", "Ray",    "Leon",   "Tyrone", "Darnell", "Marcel", "Jamal",  "Isaiah", "Roberto",
        "Eddie",  "Frank",  "Tony",   "Calvin", "Dante",   "Hector", "Ivan",   "Jerome", "Kevin",
        "Lamar",  "Mario",  "Nathan", "Omar",   "Pablo",   "Quincy", "Ruben",  "Samuel", "Trevor"};
    static const char *women[] = {"Elena",  "Maria",  "Rosa",   "Diana",   "Denise",  "Angela",
                                  "Sandra", "Linda",  "Ana",    "Grace",   "Carmen",  "Lisa",
                                  "Tanya",  "Monica", "Keisha", "Nicole",  "Olivia",  "Patrice",
                                  "Renee",  "Sonia",  "Tina",   "Valerie", "Yolanda", "Zoe",
                                  "Alicia", "Bianca", "Camila", "Dolores", "Erica",   "Felicia",
                                  "Gloria", "Hannah", "Irene",  "Jasmine", "Kayla",   "Lorena"};
    static const char *last[] = {
        "Herrera", "Williams", "Garcia", "Walker",  "Miller",  "Reyes",   "Davis",   "Torres",
        "Brooks",  "Bennett",  "Carter", "Morales", "Jackson", "Ramirez", "Johnson", "Flores",
        "Harris",  "Mendoza",  "Cooper", "Ortiz",   "Parker",  "Rivera",  "Stewart", "Vargas",
        "Wright",  "Castillo", "Fisher", "Gomez",   "Hayes",   "Jimenez", "Knight",  "Lopez",
        "Monroe",  "Navarro",  "Owens",  "Perez",   "Quinn",   "Rojas",   "Sanders", "Tucker"};
    static const char *jobs[] = {"Tamirci",    "Sofor",      "Asci",    "Tezgahtar",
                                 "Elektrikci", "Guvenlikci", "Postaci", "Berber"};
    const unsigned seed = mix(f.id), age = 42 + seed % 10, base = f.id * 8;
    const std::string surname = last[mix(seed + 17) % 40];
    const unsigned given = mix(seed + 5) % 36;
    std::array<Person, 6> p;
    const bool genders[] = {f.female, true, false, !f.female, f.female, (seed & 1) != 0};
    const unsigned ages[] = {age, age + 23, age + 25, age - 1, age - 3, 20 + seed % 4};
    for (unsigned i = 0; i < p.size(); ++i) {
        // Distinct given names within each gender, deterministic across restarts.
        const unsigned nameIndex = (given + i) % 36;
        p[i].id = base + i;
        const char initial = static_cast<char>('A' + mix(seed + i * 29 + 3) % 26);
        p[i].name = std::string(genders[i] ? women[nameIndex] : men[nameIndex]) + " " + initial +
                    ". " + surname;
        p[i].age = ages[i];
        p[i].job = i == 1 || i == 2 ? "Emekli" : jobs[mix(seed + i) % 8];
    }
    p[0].mother = p[4].mother = base + 1;
    p[0].father = p[4].father = base + 2;
    p[0].spouse = base + 3;
    p[3].spouse = base;
    p[0].siblings = {base + 4};
    p[4].siblings = {base};
    p[1].spouse = base + 2;
    p[2].spouse = base + 1;
    p[1].children = p[2].children = {base, base + 4};
    p[0].children = p[3].children = {base + 5};
    p[5].mother = f.female ? base : base + 3;
    p[5].father = f.female ? base + 3 : base;
    if (f.household == 1 || f.household == 3) {
        p[0].spouse = 0;
        p[3].spouse = 0;
    }
    if (f.household == 1) {
        p[0].children.clear();
        p[3].children.clear();
        p[5].mother = p[5].father = 0;
    }
    return p;
}
inline std::array<Person, 6> generate(const Family &f) {
    auto result = generateBase(f);
    if (!f.origin)
        return result;
    if (f.origin >= f.id || f.role < 1 || f.role > 5)
        throw std::runtime_error("Invalid family origin");
    Family root{f.origin, f.originFemale, 0, {}, {}};
    auto members = generateBase(root);
    result[0] = members[f.role];
    auto replace = [&](unsigned slot, unsigned id) {
        for (const auto &person : members)
            if (id && person.id == id) {
                result[slot] = person;
                return;
            }
        if (slot == 1 || slot == 2)
            result[slot].age = result[0].age + (slot == 1 ? 23u : 25u);
    };
    replace(1, result[0].mother);
    replace(2, result[0].father);
    replace(3, result[0].spouse);
    replace(4, result[0].siblings.empty() ? 0 : result[0].siblings.front());
    replace(5, result[0].children.empty() ? 0 : result[0].children.front());
    return result;
}
inline bool hasSpouse(const Family &f) {
    return generate(f)[0].spouse != 0;
}
inline bool hasChild(const Family &f) {
    return !generate(f)[0].children.empty();
}
struct Appointment {
    unsigned person = 0, state = 0, remaining = 60000, window = 300000;
    float x = 0, y = 0, z = 0;
    unsigned activity = 0;
};
inline bool valid(const Appointment &a, unsigned count) {
    return a.person > 0 && a.person <= count && a.state <= 4 && a.activity <= 3 &&
           a.remaining <= 3600000 && a.window <= 600000 && std::isfinite(a.x) &&
           std::isfinite(a.y) && std::isfinite(a.z) && std::abs(a.x) < 30000 &&
           std::abs(a.y) < 30000 && std::abs(a.z) < 10000;
}
struct Connection {
    unsigned a = 0, b = 0, kind = 0; // 0 friend, 1 sibling, 2 rival
};
struct Later {
    unsigned person = 0, kind = 0, source = 0; // 1 thanks, 2 gift, 3 revenge, 4 favor cooldown
    std::uint64_t due = 0;                     // World minutes.
};
struct Registry {
    std::vector<Family> families;
    std::vector<Appointment> appointments;
    std::vector<Connection> connections;
    std::vector<Letter> letters;
    std::vector<Later> later;
    std::uint64_t worldMinutes = 0;
    unsigned nextLetter = 1;
    int honor = 0;
    std::uint64_t savedAt = 0;
    Registry() {
        families.reserve(MaxFamilies);
    }
    unsigned post(unsigned person, std::string body, bool outgoing = false, unsigned kind = 0,
                  unsigned source = 0, bool call = false) {
        if (!person || person > families.size() || source > families.size() || kind > 6 ||
            nextLetter >= 1000000000 || body.empty())
            return 0;
        if (letters.size() >= 256)
            letters.erase(letters.begin());
        unsigned id = nextLetter++;
        letters.push_back(
            {id, person, source, kind, worldMinutes, outgoing, !outgoing, call, cleanText(body)});
        return id;
    }
    Family &create(bool female) {
        if (families.size() >= MaxFamilies)
            throw std::runtime_error("Archive full");
        families.push_back({static_cast<unsigned>(families.size() + 1), female, 0, {}, {}});
        families.back().household = mix(families.back().id + 811) % 4 == 0 ? 2u : 1u;
        return families.back();
    }
    // Ambient strangers who never became a contact hold no state worth keeping. Once the archive
    // reaches SoftLimit their slots are reused in place; IDs referenced elsewhere stay untouched.
    unsigned recycleCursor = 0;
    unsigned recycle(const std::function<bool(unsigned)> &active) {
        if (families.empty())
            return 0;
        std::unordered_set<unsigned> keep;
        for (const auto &e : connections) {
            keep.insert(e.a);
            keep.insert(e.b);
        }
        for (const auto &a : appointments)
            keep.insert(a.person);
        for (const auto &m : letters) {
            keep.insert(m.person);
            if (m.source)
                keep.insert(m.source);
        }
        for (const auto &l : later) {
            keep.insert(l.person);
            keep.insert(l.source);
        }
        for (const auto &f : families) {
            if (f.origin)
                keep.insert(f.origin);
            if (f.social.newsFrom)
                keep.insert(f.social.newsFrom);
        }
        for (unsigned n = 0; n < families.size(); ++n) {
            const unsigned i = (recycleCursor + n) % static_cast<unsigned>(families.size());
            const auto &f = families[i];
            const auto &s = f.social;
            if (keep.count(f.id) || active(f.id) || f.origin || f.role || f.meetings || s.known ||
                s.trust || s.anger || s.attempts || s.lastReaction || s.meaningful ||
                s.encounters || s.helped || s.lastNote || s.journal || s.robbed || s.events ||
                s.resentment || s.newsFrom || s.newsKind || s.provokes || s.cutOff || s.feud ||
                s.romance || s.dates || s.homeVisits || s.childStage)
                continue;
            recycleCursor = i + 1;
            return f.id;
        }
        return 0;
    }
    void reset(unsigned id, bool female) {
        families[id - 1] = Family{id, female, 0, {}, {}};
        families[id - 1].household = mix(id + 811) % 4 == 0 ? 2u : 1u;
    }
    void write(std::ostream &out) const {
        out << "SAPEOPLE 13\n"
            << families.size() << '\n'
            << std::setprecision(std::numeric_limits<float>::max_digits10);
        for (const auto &f : families) {
            const auto &p = f.presence;
            if (!valid(p))
                throw std::runtime_error("Invalid presence state");
            if (!valid(f.social) || f.household > 3 || f.social.newsFrom > families.size() ||
                (f.origin &&
                 (f.origin >= f.id || f.role < 1 || f.role > 5 || families[f.origin - 1].origin)) ||
                (!f.origin && f.role))
                throw std::runtime_error("Invalid social state");
            out << f.id << ' ' << f.female << ' ' << f.meetings << ' ' << p.model << ' ' << p.type
                << ' ' << p.x << ' ' << p.y << ' ' << p.z << ' ' << p.heading << ' ' << p.health
                << ' ' << p.area << ' ' << p.located << ' ' << p.alive << ' ' << f.social.known
                << ' ' << f.social.trust << ' ' << f.social.anger << ' ' << f.social.attempts << ' '
                << f.social.lastReaction << ' ' << f.social.meaningful << ' ' << f.social.encounters
                << ' ' << f.social.helped << ' ' << f.social.lastNote << ' ' << f.social.journal
                << ' ' << f.social.robbed << ' ' << f.social.events << ' ' << f.social.resentment
                << ' ' << f.social.newsFrom << ' ' << f.social.newsKind << ' ' << f.origin << ' '
                << f.role << ' ' << f.originFemale << ' ' << f.social.provokes << ' '
                << f.social.cutOff << ' ' << f.social.feud << ' ' << f.social.romance << ' '
                << f.household << ' ' << f.social.dates << ' ' << f.social.homeVisits << ' '
                << f.social.homeDate << ' ' << f.social.childStage << ' ' << f.social.childMinutes
                << '\n';
        }
        if (appointments.size() > 8)
            throw std::runtime_error("Too many appointments");
        out << "MEETINGS " << appointments.size() << '\n';
        for (const auto &a : appointments) {
            if (!valid(a, static_cast<unsigned>(families.size())))
                throw std::runtime_error("Invalid appointment");
            out << a.person << ' ' << a.state << ' ' << a.remaining << ' ' << a.window << ' ' << a.x
                << ' ' << a.y << ' ' << a.z << ' ' << a.activity << '\n';
        }
        if (honor < -100 || honor > 100 || connections.size() > 65536 || savedAt > 4102444800ULL)
            throw std::runtime_error("Invalid world state");
        out << "WORLD " << honor << ' ' << connections.size() << ' ' << savedAt << '\n';
        std::unordered_set<std::uint64_t> pairs;
        for (const auto &e : connections) {
            if (!e.a || e.a >= e.b || e.b > families.size() || e.kind > 6 ||
                !pairs.insert((std::uint64_t(e.a) << 32) | e.b).second)
                throw std::runtime_error("Invalid connection");
            out << e.a << ' ' << e.b << ' ' << e.kind << '\n';
        }
        unsigned routines = 0;
        for (const auto &f : families)
            if (f.daily.ready || f.daily.outings || f.daily.memories || f.daily.lastInvitation ||
                f.daily.lastRemote)
                ++routines;
        if (worldMinutes > 1000000000 || !nextLetter || nextLetter > 1000000000 ||
            letters.size() > 256)
            throw std::runtime_error("Invalid social world");
        out << "LIFE " << worldMinutes << ' ' << nextLetter << ' ' << routines << '\n';
        for (const auto &f : families) {
            const auto &d = f.daily;
            if (!(d.ready || d.outings || d.memories || d.lastInvitation || d.lastRemote))
                continue;
            if (!valid(d))
                throw std::runtime_error("Invalid routine");
            out << f.id << ' ' << d.ready << ' ' << d.home.x << ' ' << d.home.y << ' ' << d.home.z
                << ' ' << d.work.x << ' ' << d.work.y << ' ' << d.work.z << ' ' << d.leisure.x
                << ' ' << d.leisure.y << ' ' << d.leisure.z << ' ' << d.lastInvitation << ' '
                << d.lastRemote << ' ' << d.outings << ' ' << d.memories << ' ' << d.busyUntil
                << '\n';
        }
        out << "MAIL " << letters.size() << '\n';
        unsigned previous = 0;
        for (const auto &m : letters) {
            if (m.id <= previous || m.id >= nextLetter || !m.person || m.person > families.size() ||
                m.source > families.size() || m.kind > 6 || m.minute > 1000000000 ||
                m.body.empty() || m.body != cleanText(m.body))
                throw std::runtime_error("Invalid mail");
            previous = m.id;
            out << m.id << ' ' << m.person << ' ' << m.source << ' ' << m.kind << ' ' << m.minute
                << ' ' << m.outgoing << ' ' << m.unread << ' ' << m.call << ' '
                << std::quoted(m.body) << '\n';
        }
        if (later.size() > 64)
            throw std::runtime_error("Too many consequences");
        out << "LATER " << later.size() << '\n';
        for (const auto &l : later) {
            if (!l.person || l.person > families.size() || l.source > families.size() ||
                !l.kind || l.kind > 5 || l.due > 1000000000)
                throw std::runtime_error("Invalid consequence");
            out << l.person << ' ' << l.kind << ' ' << l.source << ' ' << l.due << '\n';
        }
        if (!out)
            throw std::runtime_error("Write failed");
    }
    static Registry read(std::istream &in) {
        Registry r;
        std::string magic;
        unsigned version, count;
        if (!(in >> magic >> version >> count) || magic != "SAPEOPLE" || version < 1 ||
            version > 13 || count > MaxFamilies)
            throw std::runtime_error("Unsupported or corrupt archive");
        std::unordered_set<unsigned> relatives;
        for (unsigned i = 0; i < count; ++i) {
            unsigned id, gender, meetings;
            if (!(in >> id >> gender >> meetings) || id != i + 1 || gender > 1 ||
                meetings > 1000000)
                throw std::runtime_error("Corrupt person record");
            Family f{id, gender != 0, meetings, {}, {}};
            if (version >= 2) {
                auto &p = f.presence;
                unsigned located, alive;
                if (!(in >> p.model >> p.type >> p.x >> p.y >> p.z >> p.heading >> p.health >>
                      p.area >> located >> alive) ||
                    located > 1 || alive > 1)
                    throw std::runtime_error("Corrupt presence record");
                p.located = located != 0;
                p.alive = alive != 0;
                if (!valid(p))
                    throw std::runtime_error("Invalid presence data");
            }
            if (version >= 3) {
                auto &s = f.social;
                if (!(in >> s.known >> s.trust >> s.anger >> s.attempts >> s.lastReaction) ||
                    !valid(s))
                    throw std::runtime_error("Corrupt social data");
            } else if (meetings)
                f.social.known =
                    Knowledge::Name; // Legacy contacts keep names, not private dossiers.
            if (version >= 4) {
                auto &s = f.social;
                unsigned journal;
                if (!(in >> s.meaningful >> s.encounters >> s.helped >> s.lastNote >> journal) ||
                    journal > 1 || !valid(s))
                    throw std::runtime_error("Corrupt journal data");
                s.journal = journal != 0;
                if (s.journal && !(s.known & Knowledge::Name))
                    throw std::runtime_error("Journal entry without a learned name");
            } else {
                auto &s = f.social;
                s.encounters = f.meetings;
                unsigned privateInfo = s.known & ~Knowledge::Name;
                while (privateInfo) {
                    s.meaningful += privateInfo & 1;
                    privateInfo >>= 1;
                }
                s.journal = (s.known & Knowledge::Name) && s.meaningful > 0;
            }
            if (version >= 5) {
                if (!(in >> f.social.robbed >> f.social.events) || !valid(f.social))
                    throw std::runtime_error("Corrupt encounter memory");
            } else if (f.social.helped)
                f.social.events |= 1;
            if (version >= 6) {
                if (!(in >> f.social.resentment) || !valid(f.social))
                    throw std::runtime_error("Corrupt relationship memory");
            }
            if (version >= 7 && (!(in >> f.social.newsFrom >> f.social.newsKind) ||
                                 !valid(f.social) || f.social.newsFrom > count))
                throw std::runtime_error("Corrupt social news");
            if (version >= 7) {
                unsigned female;
                if (!(in >> f.origin >> f.role >> female) || female > 1 ||
                    (f.origin && (f.origin >= f.id || f.role < 1 || f.role > 5 ||
                                  r.families[f.origin - 1].origin)) ||
                    (!f.origin && f.role))
                    throw std::runtime_error("Corrupt family origin");
                f.originFemale = female != 0;
                if (version >= 8 &&
                    (!(in >> f.social.provokes >> f.social.cutOff) || !valid(f.social)))
                    throw std::runtime_error("Corrupt provocation memory");
                if (version >= 9 &&
                    (!(in >> f.social.feud >> f.social.romance) || !valid(f.social)))
                    throw std::runtime_error("Corrupt feud memory");
                if (version >= 10) {
                    if (!(in >> f.household >> f.social.dates >> f.social.homeVisits >>
                          f.social.homeDate >> f.social.childStage >> f.social.childMinutes) ||
                        f.household > 3 || !valid(f.social))
                        throw std::runtime_error("Corrupt household state");
                } else if (f.social.romance) {
                    // Older builds made every adult married, even contacts already dating CJ.
                    // Keep named relatives and children as previous-family history.
                    f.household = 3;
                    f.social.dates = f.social.romance;
                }
                if (f.origin && !relatives.insert(f.origin * 8 + f.role).second)
                    throw std::runtime_error("Duplicate relative");
            }
            r.families.push_back(f);
        }
        if (version >= 6) {
            unsigned size;
            std::string marker;
            if (!(in >> marker >> size) || marker != "MEETINGS" || size > 8)
                throw std::runtime_error("Corrupt appointments");
            for (unsigned i = 0; i < size; ++i) {
                Appointment a;
                if (!(in >> a.person >> a.state >> a.remaining >> a.window >> a.x >> a.y >> a.z) ||
                    !valid(a, count))
                    throw std::runtime_error("Corrupt appointment");
                for (const auto &old : r.appointments)
                    if (old.person == a.person)
                        throw std::runtime_error("Duplicate appointment");
                if (version >= 11 && (!(in >> a.activity) || a.activity > 3))
                    throw std::runtime_error("Corrupt activity");
                r.appointments.push_back(a);
            }
        }
        if (version >= 7) {
            std::string marker;
            unsigned size;
            if (!(in >> marker >> r.honor >> size >> r.savedAt) || r.savedAt > 4102444800ULL ||
                marker != "WORLD" || r.honor < -100 || r.honor > 100 || size > 65536)
                throw std::runtime_error("Corrupt world state");
            std::unordered_set<std::uint64_t> pairs;
            for (unsigned i = 0; i < size; ++i) {
                Connection e;
                if (!(in >> e.a >> e.b >> e.kind) || !e.a || e.a >= e.b || e.b > count ||
                    e.kind > 6 || !pairs.insert((std::uint64_t(e.a) << 32) | e.b).second)
                    throw std::runtime_error("Corrupt connection");
                r.connections.push_back(e);
            }
        }
        if (version < 10) {
            unsigned partner = 0;
            for (const auto &f : r.families)
                if (f.social.romance >= 3 && f.presence.alive &&
                    (!partner || f.social.trust > r.families[partner - 1].social.trust))
                    partner = f.id;
            for (auto &f : r.families)
                if (f.social.romance >= 3 && f.id != partner)
                    f.social.romance = 2;
        }
        if (version >= 11) {
            std::string marker;
            unsigned routines;
            if (!(in >> marker >> r.worldMinutes >> r.nextLetter >> routines) || marker != "LIFE" ||
                r.worldMinutes > 1000000000 || !r.nextLetter || r.nextLetter > 1000000000 ||
                routines > count)
                throw std::runtime_error("Corrupt social world");
            std::unordered_set<unsigned> seen;
            for (unsigned i = 0; i < routines; ++i) {
                unsigned id, ready;
                Daily d;
                if (!(in >> id >> ready >> d.home.x >> d.home.y >> d.home.z >> d.work.x >>
                      d.work.y >> d.work.z >> d.leisure.x >> d.leisure.y >> d.leisure.z >>
                      d.lastInvitation >> d.lastRemote >> d.outings >> d.memories >> d.busyUntil) ||
                    !id || id > count || ready > 1 || !valid(d) || !seen.insert(id).second)
                    throw std::runtime_error("Corrupt routine");
                d.ready = ready != 0;
                r.families[id - 1].daily = d;
            }
            unsigned mails, previous = 0;
            if (!(in >> marker >> mails) || marker != "MAIL" || mails > 256)
                throw std::runtime_error("Corrupt mailbox");
            for (unsigned i = 0; i < mails; ++i) {
                Letter m;
                unsigned outgoing, unread, call;
                if (!(in >> m.id >> m.person >> m.source >> m.kind >> m.minute >> outgoing >>
                      unread >> call >> std::quoted(m.body)) ||
                    m.id <= previous || m.id >= r.nextLetter || !m.person || m.person > count ||
                    m.source > count || m.kind > 6 || m.minute > 1000000000 || outgoing > 1 ||
                    unread > 1 || call > 1 || m.body.empty() || m.body != cleanText(m.body))
                    throw std::runtime_error("Corrupt message");
                previous = m.id;
                m.outgoing = outgoing != 0;
                m.unread = unread != 0;
                m.call = call != 0;
                r.letters.push_back(m);
            }
        }
        if (version >= 12) {
            std::string marker;
            unsigned size;
            if (!(in >> marker >> size) || marker != "LATER" || size > 64)
                throw std::runtime_error("Corrupt consequences");
            for (unsigned i = 0; i < size; ++i) {
                Later l;
                if (!(in >> l.person >> l.kind >> l.source >> l.due) || !l.person ||
                    l.person > count || l.source > count || !l.kind || l.kind > (version >= 13 ? 5u : 4u) ||
                    l.due > 1000000000)
                    throw std::runtime_error("Corrupt consequence");
                r.later.push_back(l);
            }
        }
        in >> std::ws;
        if (!in.eof())
            throw std::runtime_error("Unexpected archive data");
        return r;
    }
};
inline std::string page(const Family &f, unsigned pageNumber) {
    auto p = generate(f);
    std::ostringstream s;
    if (pageNumber % 3 == 0) {
        s << "~g~" << p[0].name << "~w~ | " << p[0].age << " yas~n~" << p[0].job << " | Kisi #"
          << p[0].id << "~n~San Andreas'ta buyudu. ";
        if (!p[0].children.empty())
            s << p[5].name << " adinda yetiskin bir cocugu var.";
        s << "~n~Tanisma: " << f.meetings << " | " << (f.presence.alive ? "Hayatta" : "Vefat etti");
    } else if (pageNumber % 3 == 1) {
        s << "~g~AILE: " << p[0].name << "~w~~n~Anne: " << p[1].name << " (" << p[1].age
          << ")~n~Baba: " << p[2].name << " (" << p[2].age << ")~n~Kardes: " << p[4].name;
    } else {
        s << "~g~YAKINLARI~w~~n~Es: " << (p[0].spouse ? p[3].name : "Yok")
          << "~n~Cocuk: " << (p[0].children.empty() ? "Yok" : p[5].name)
          << "~n~Aile kayitlari karsilikli baglidir.";
    }
    s << "~n~~y~N: Tanis | F7: Sayfa | F8: Arsiv";
    return s.str();
}
} // namespace people
