#pragma once
#include "People.h"
#include <algorithm>
namespace socialcontext {
enum class Weather { Quiet, Rain, Fog, Sun, Wind };
inline Weather weather(float rain, float fog, float sun, float wind, unsigned hour) {
    if (rain > 0.15f) return Weather::Rain;
    if (fog > 0.3f) return Weather::Fog;
    if (wind > 0.65f) return Weather::Wind;
    if (hour >= 7 && hour < 20 && sun > 0.15f) return Weather::Sun;
    return Weather::Quiet;
}
inline int honorTone(int honor) { return honor >= 40 ? 1 : honor <= -40 ? -1 : 0; }
inline int passengerDoor(int slot, bool bike = false) {
    if (bike) return slot == 0 ? 11 : -1;
    const int doors[] = {8, 11, 9};
    return slot >= 0 && slot < 3 ? doors[slot] : -1;
}
inline bool driveReady(bool driver, bool automobile, bool stopped, bool seatFree) {
    return driver && automobile && stopped && seatFree;
}
struct TalkChoices {
    std::uint32_t last[3] = {}, lastAny = 0;
    unsigned count[3] = {}, total = 0;
    unsigned use(unsigned choice, std::uint32_t now) {
        if (choice > 2 || (total && now - lastAny < 2800)) return 0;
        last[choice] = lastAny = now;
        ++total;
        count[choice] = std::min(1000000u, count[choice] + 1);
        return count[choice];
    }
};
struct Topics {
    unsigned used = 0, cursor = 0;
    unsigned take(unsigned count, unsigned seed) {
        count = std::min(30u, count);
        if (!count) return 0;
        for (unsigned i = 0; i < count; ++i) {
            unsigned index = (seed + cursor + i) % count;
            if (!(used & (1u << index))) {
                used |= 1u << index;
                ++cursor;
                return index;
            }
        }
        return count; // Exhausted: silence beats repeating the same line.
    }
};
enum class Witness { Help, Provoke, Violence, Killing };
inline bool witnessCancels(Witness event) { return event != Witness::Help; }
} // namespace socialcontext
