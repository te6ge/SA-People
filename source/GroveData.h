#pragma once
#include <array>
#include <cstdint>
namespace people {
struct GroveFund {
    unsigned balance=0,contributed=0,legacyCredit=0;
    std::array<unsigned,4> level{};
    // Last claim minute + 1; zero means never collected. Health, armor, three guns, workshop.
    std::array<std::uint64_t,6> usedAt{};
    static unsigned cost(unsigned project,unsigned current) {
        static const unsigned prices[4][2]={{2500,6500},{5000,10000},{6000,12000},{20000,45000}};
        return project<4 && current<2?prices[project][current]:0;
    }
    bool valid(std::uint64_t minute) const {
        if(balance>1000000 || contributed>1000000000 || legacyCredit>37450)return false;
        unsigned invested=0;
        for(unsigned n=0;n<4;++n) {
            if(level[n]>2)return false;
            for(unsigned tier=0;tier<level[n];++tier)invested+=cost(n,tier);
        }
        if(std::uint64_t(contributed)+legacyCredit!=std::uint64_t(balance)+invested)return false;
        for(unsigned n=0;n<6;++n) {
            const unsigned project=n==0?0u:n==1?2u:n==5?1u:3u;
            if(usedAt[n]>minute+1 || (usedAt[n] && !level[project]) ||
               (usedAt[n] && (n==3 || n==4) && level[3]<2))return false;
        }
        return true;
    }
    bool importV19(unsigned oldBalance,unsigned oldContributed,const std::array<unsigned,3> &oldLevel,
                   const std::array<std::uint64_t,3> &oldDays,std::uint64_t minute) {
        static const unsigned oldPrices[3][2]={{300,800},{600,1400},{450,1000}};
        if(oldBalance>1000000 || oldContributed>1000000000 || oldBalance>oldContributed)return false;
        unsigned oldValue=0,newValue=0;
        for(unsigned p=0;p<3;++p) {
            if(oldLevel[p]>2 || oldDays[p]>minute/1440+1 || (oldDays[p] && !oldLevel[p]))return false;
            for(unsigned tier=0;tier<oldLevel[p];++tier){oldValue+=oldPrices[p][tier];newValue+=cost(p,tier);}
        }
        if(oldContributed-oldBalance!=oldValue)return false;
        *this={};balance=oldBalance;contributed=oldContributed;legacyCredit=newValue-oldValue;
        for(unsigned p=0;p<3;++p) {
            level[p]=oldLevel[p];
            const unsigned slot=p==0?0u:p==1?5u:1u;
            if(oldDays[p]==minute/1440+1)usedAt[slot]=minute+1;
        }
        return valid(minute);
    }
};
}
