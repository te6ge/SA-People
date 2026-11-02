#pragma once
#include "SocialData.h"
namespace people {
// V18. These are personal consequences, not an omniscient simulation of every pedestrian.
struct Story {
    unsigned grievance=0, fear=0, intent=0, confrontations=0, life=0, lastDeed=0;
    std::uint64_t lastHarm=0, nextAction=0, lastPeace=0, nextLife=0, until=0, lastInquiry=0;
    bool empty() const {
        return !(grievance|fear|intent|confrontations|life|lastDeed) &&
            !lastHarm && !nextAction && !lastPeace && !nextLife && !until && !lastInquiry;
    }
    bool valid() const {
        return grievance<=100 && fear<=100 && intent<=3 && confrontations<=3 && life<=4 && (lastDeed==0 || lastDeed==1 || lastDeed==2 || lastDeed==12) &&
            lastHarm<=1000000001ULL && nextAction<=1000000000ULL && lastPeace<=1000000001ULL && lastInquiry<=1000000001ULL &&
            nextLife<=1000000000ULL && until<=1000000000ULL &&
            (!intent || grievance>=20) && (life ? until>0 : until==0);
    }
    void write(std::ostream &out) const {
        out<<grievance<<' '<<fear<<' '<<intent<<' '<<confrontations<<' '<<life<<' '
           <<lastHarm<<' '<<nextAction<<' '<<lastPeace<<' '<<nextLife<<' '<<until<<' '<<lastInquiry<<' '<<lastDeed;
    }
    bool read(std::istream &in) {
        return static_cast<bool>(in>>grievance>>fear>>intent>>confrontations>>life>>
            lastHarm>>nextAction>>lastPeace>>nextLife>>until>>lastInquiry>>lastDeed) && valid();
    }
};
struct StoryNews {
    unsigned id=0, person=0, source=0, kind=0;
    std::uint64_t minute=0;
};
struct Sighting {
    unsigned observer=0, person=0;
    std::uint64_t minute=0;
    Point point{};
};
}
