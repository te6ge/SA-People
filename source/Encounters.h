#pragma once
#include "Street.h"
namespace encounters {
enum class Outcome { Paid, Refused, Fight, Flee, Empty, Already };
struct Robbery {
    Outcome outcome = Outcome::Refused;
    unsigned amount = 0;
};
inline unsigned threat(int weapon, unsigned ammo, bool aiming) {
    unsigned strength = 5;
    if (weapon >= 2 && weapon <= 15)
        strength = weapon == 4 || weapon == 8 || weapon == 9 ? 42u : 28u;
    if (weapon >= 22 && weapon <= 34)
        strength = ammo ? 70u : 38u;
    if ((weapon >= 35 && weapon <= 38) || weapon == 16 || weapon == 17 || weapon == 18 ||
        weapon == 39)
        strength = ammo ? 85u : 30u;
    return std::min(95u, strength + (aiming ? 8u : 0u));
}
inline Robbery rob(people::Family &f, unsigned force, int wallet, bool armedVictim,
                   bool combatAllowed) {
    if (f.social.robbed)
        return {Outcome::Already, 0};
    f.social.robbed = 1;
    f.social.lastNote = 7;
    f.social.trust = std::max(-100, f.social.trust - 45);
    f.social.resentment = std::min(100u, f.social.resentment + 60);
    f.social.anger = std::min(100u, f.social.anger + 45);
    unsigned personality = dialogue::temperament(f),
             roll = people::mix(f.id * 719 + f.social.attempts * 13) % 100;
    int compliance = 25 + static_cast<int>(std::min(95u, force)) * 3 / 4 -
                     (personality == 3 ? 24 : 0) - (armedVictim ? 30 : 0);
    compliance = std::clamp(compliance, 8, 92);
    if (roll < static_cast<unsigned>(compliance))
        return wallet > 0 ? Robbery{Outcome::Paid, static_cast<unsigned>(std::min(wallet, 350))}
                          : Robbery{Outcome::Empty, 0};
    if (combatAllowed && (armedVictim || personality == 3 || roll > 88))
        return {Outcome::Fight, 0};
    return {roll % 2 ? Outcome::Flee : Outcome::Refused, 0};
}
inline unsigned transfer(int &player, int &wallet, unsigned requested) {
    if (player < 0 || player > 999999999 || wallet <= 0)
        return 0;
    auto amount = std::min(
        {requested, static_cast<unsigned>(wallet), static_cast<unsigned>(999999999 - player)});
    player += static_cast<int>(amount);
    wallet -= static_cast<int>(amount);
    return amount;
}
inline bool donate(int &player, int &wallet, unsigned amount) {
    if (player < 0 || wallet < 0 || amount > 32767u || static_cast<unsigned>(player) < amount ||
        static_cast<unsigned>(wallet) > 32767u - amount)
        return false;
    player -= static_cast<int>(amount);
    wallet += static_cast<int>(amount);
    return true;
}
enum class Kind { Battery = 0, Directions = 1, Fare = 2, Beggar = 3, Addict = 4, Ride = 5, Visit = 6 };
inline unsigned bit(Kind kind) {
    return 1u << static_cast<unsigned>(kind);
}
inline bool complete(people::Family &f, Kind kind) {
    unsigned mask = bit(kind);
    if (f.social.events & mask)
        return false;
    f.social.events |= mask;
    f.social.known |= people::Name;
    f.social.trust = std::min(100, f.social.trust + (kind == Kind::Fare ? 18 : 16));
    f.social.encounters = std::min(1000000u, f.social.encounters + 1);
    street::addMeaning(f, 3, kind == Kind::Fare ? 9u : 8u);
    return true;
}
// Gameplay milliseconds only: menus, background windows and missions do not age this timer.
struct Director {
    std::uint64_t clock = 0, next = 45000;
    unsigned attempts = 0, completed = 0;
    bool due() const {
        return clock >= next;
    }
    void step(unsigned elapsed, bool safe) {
        if (safe)
            clock += std::min(elapsed, 250u);
    }
    void retry() {
        next = clock + 8000;
        ++attempts;
    }
    void started(unsigned seed) {
        next = clock + 90000 + people::mix(seed) % 90001;
        ++completed;
        attempts = 0;
    }
    void initial(unsigned seconds) {
        clock = 0;
        next = std::clamp(seconds, 15u, 300u) * 1000;
        attempts = completed = 0;
    }
};
} // namespace encounters
