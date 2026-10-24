#pragma once
#include "World.h"

namespace meetings {
// Transient state belongs to one appointment, never to the contact's lifetime.
struct Run {
    people::Handle actor{};
    void *task = nullptr;
    int blip = -1;
    std::uint32_t nextSpawn = 0;
    bool notified = false, waiting = false, spotResolved = false, activityStarted = false;
    std::uint32_t progressAt = 0, nextResolve = 0;
    std::uint32_t nextHint = 0;
    unsigned spawnFails = 0;
    float bestDistance = 1000000000.0f;
};
template <class Cleanup>
inline void forgetRun(std::unordered_map<unsigned, Run> &runs, unsigned id, Cleanup cleanup) {
    auto it = runs.find(id);
    if (it == runs.end())
        return;
    cleanup(it->second);
    runs.erase(it);
}
} // namespace meetings
