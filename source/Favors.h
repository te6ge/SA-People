#pragma once
#include "Dialogue.h"
#include "Later.h"
// Friends' favors: a close contact helps CJ in their line of work, once per game day.
// Mechanic/electrician fix CJ's car, a cook sends food, a guard lends body armor, a driver knows
// the back roads (one wanted star less); shop assistants, postmen and barbers slip him some cash.
namespace favors {
enum class Perk : unsigned { Repair, Heal, Armor, Wanted, Cash };
inline Perk perk(const people::Family &f) {
    if(f.presence.type==18)return Perk::Heal;
    const auto job = people::generate(f)[0].job;
    if (job == "Tamirci" || job == "Elektrikci")
        return Perk::Repair;
    if (job == "Asci")
        return Perk::Heal;
    if (job == "Guvenlikci")
        return Perk::Armor;
    if (job == "Sofor")
        return Perk::Wanted;
    return Perk::Cash;
}
inline const char *label(Perk p) {
    switch (p) {
    case Perk::Repair:
        return "FAVOR_REPAIR";
    case Perk::Heal:
        return "FAVOR_HEAL";
    case Perk::Armor:
        return "FAVOR_ARMOR";
    case Perk::Wanted:
        return "SERVICE_PICKUP";
    default:
        return "FAVOR_CASH";
    }
}
constexpr int MinTrust = 50;
// Why this contact cannot help right now, or nullptr when the favor may go ahead.
inline const char *refusal(const people::Registry &r, unsigned id) {
    if (!id || id > r.families.size())
        return "FAVOR_NO";
    const auto &f = r.families[id - 1];
    const auto &s = f.social;
    if (!f.presence.alive || s.cutOff || s.robbed || s.feud || s.anger >= 30)
        return "FAVOR_NO";
    if (!dialogue::knows(f, people::Job))
        return "FAVOR_UNKNOWN_JOB";
    if (s.trust < MinTrust)
        return "FAVOR_LOW_TRUST";
    if (later::cooling(r, id))
        return "FAVOR_COOLDOWN";
    if(f.community.serviceDay==r.worldMinutes/1440+1)return "FAVOR_COOLDOWN";
    return nullptr;
}
// Cash amount for the Cash perk; deterministic per contact and day.
inline unsigned cash(const people::Registry &r, unsigned id) {
    return 50 + people::mix(id * 211 + static_cast<unsigned>(r.worldMinutes / 1440)) % 101;
}
// Records a granted favor: day-long cooldown and a message in the contact's thread.
inline void granted(people::Registry &r, unsigned id, Perk p) {
    later::cooldown(r, id);
    r.post(id, std::string(label(p)) + "_MSG");
}
} // namespace favors
