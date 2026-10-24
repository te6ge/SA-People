#pragma once
#include "People.h"
#include <array>
namespace nativetalk {
constexpr int Dealer = 17, Criminal = 20;
inline bool ambientRoutine(int type) { return type >= 7 && type <= Dealer; }
inline bool replaceDealer(bool enabled, bool mission, bool known, bool ambient,
                          bool recruited, int type) {
    return enabled && !mission && known && ambient && !recruited && type == Dealer;
}
// Own only the enable flag of a particular live conversation slot/actor generation.
// Keep its nodes intact so game scripts can still find their conversation.
struct ScriptGate {
    struct Lease { people::Handle actor{}; bool owned = false; unsigned char original = 0; };
    std::array<Lease, 14> slots{};
    bool apply(unsigned slot, people::Handle actor, bool valid, bool mute,
               unsigned char &enabled, unsigned &state) {
        auto &lease = slots.at(slot);
        if (!valid || (lease.owned && !(actor == lease.actor))) lease = {};
        if (!valid) return false;
        if (!mute) {
            if (lease.owned) { enabled = lease.original; lease = {}; }
            return false;
        }
        if (!lease.owned) lease = {actor, true, enabled};
        const bool interrupted = state != 0;
        enabled = 0;
        state = 0;
        return interrupted;
    }
};
} // namespace nativetalk
