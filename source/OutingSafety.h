#pragma once
#include <cstdint>
#include <cstring>
namespace outingsafety {
// Per-actor CPed fields validated against plugin-sdk's CPed/CPedAcquaintance.
// No global PED.DAT relationships, weapons, health or collision flags are changed.
struct Disposition {
    bool active = false, alreadyPeaceful = false;
    std::uint32_t removed[4] = {};
    static constexpr unsigned offsets[4] = {0x4E0, 0x4E4, 0x4EC, 0x4F0};
    void apply(unsigned char *ped) {
        if (active) return;
        active = true;
        alreadyPeaceful = (ped[0x473] & 1) != 0; // CPed::bDontFight.
        ped[0x473] |= 1;
        for (unsigned i = 0; i < 4; ++i) {
            std::memcpy(&removed[i], ped + offsets[i], sizeof removed[i]);
            const std::uint32_t neutral = 0;
            std::memcpy(ped + offsets[i], &neutral, sizeof neutral);
        }
    }
    void restore(unsigned char *ped) {
        if (!active) return;
        if (!alreadyPeaceful) ped[0x473] &= static_cast<unsigned char>(~1u);
        for (unsigned i = 0; i < 4; ++i) {
            std::uint32_t current = 0;
            std::memcpy(&current, ped + offsets[i], sizeof current);
            current |= removed[i];
            std::memcpy(ped + offsets[i], &current, sizeof current);
        }
        *this = {};
    }
};
// Clear only offensive native event responses, never damage/fall/arrest/script tasks.
inline bool offensiveTask(int type) { return type >= 1000 && type <= 1005; }
} // namespace outingsafety
