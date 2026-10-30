#pragma once
#include <cstdint>
namespace people {
struct Commitment {
    // 0: dating/partner, 1: engaged, 2: ceremony booked, 3: married.
    unsigned stage = 0, venue = 0;
    std::uint64_t since = 0, lastTalk = 0, proposedAt = 0, marriedAt = 0;
    bool empty() const { return !stage && !venue && !since && !lastTalk && !proposedAt && !marriedAt; }
    bool valid() const {
        return stage <= 3 && venue <= 3 && since <= 1000000001ULL && lastTalk <= 1000000001ULL &&
            proposedAt <= 1000000001ULL && marriedAt <= 1000000001ULL &&
            (stage == 2 ? venue > 0 : !venue) &&
            (!stage || (since && proposedAt >= since)) &&
            (stage == 3 ? marriedAt >= proposedAt && marriedAt > 0 : !marriedAt);
    }
};
}
