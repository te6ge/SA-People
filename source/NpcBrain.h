#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <deque>
#include <map>
#include <string>
#include <vector>
namespace npcbrain {
enum class Role { Assault, Support, Cautious };
inline Role role(unsigned identity, float health) {
    return health < 35 ? Role::Cautious : identity % 3 == 0 ? Role::Support : Role::Assault;
}
inline bool mayAssist(int type,int createdBy,bool mission,bool interior,bool alive,bool onFoot,
                      bool physical,bool ownSocial,bool war,bool nativeGangTarget) {
    return type>=7 && type<=16 && !mission && !interior && alive && onFoot &&
        !physical && !ownSocial && (createdBy==1 || (war && createdBy==2 && nativeGangTarget));
}
inline float effectiveRange(int weapon) {
    return weapon>=25 && weapon<=27 ? 20.0f : weapon==30 || weapon==31 ? 55.0f
        : weapon==33 || weapon==34 ? 75.0f : weapon==28 || weapon==29 || weapon==32 ? 35.0f : 30.0f;
}
inline float score(float distanceSquared,unsigned alreadyAssigned,bool clearShot) {
    return distanceSquared + alreadyAssigned*144.0f + (clearShot ? 0.0f : 225.0f);
}
struct Posture {
    // CPed layout verified against plugin-sdk: flags 0x46E/0x46F, rate/accuracy 0x719/0x71A.
    bool active=false;
    unsigned char oldRange=0,oldDuck=0,oldRate=0,oldAccuracy=0,lastRange=0,lastDuck=0,lastRate=0,lastAccuracy=0;
    void apply(unsigned char *ped,Role choice,float distanceSquared,int weapon=30) {
        if(!active) {
            oldRange=ped[0x46e]&0x48;oldDuck=ped[0x46f]&2;
            oldRate=ped[0x719];oldAccuracy=ped[0x71a];active=true;
        } else {
            if(ped[0x719]!=lastRate) oldRate=ped[0x719];
            if(ped[0x71a]!=lastAccuracy) oldAccuracy=ped[0x71a];
        }
        const float range=effectiveRange(weapon);
        const bool hold=(choice!=Role::Assault && distanceSquared>81 && distanceSquared<=range*range);
        lastRange=hold ? 0x48 : oldRange;
        lastDuck=((ped[0x46f]&1) || distanceSquared<16) ? oldDuck : 2; // Respect the native "not allowed to duck" flag.
        lastRate=std::max(oldRate,static_cast<unsigned char>(choice==Role::Support?45:35));
        lastAccuracy=std::max(oldAccuracy,static_cast<unsigned char>(choice==Role::Support?45:35));
        ped[0x46e]=(ped[0x46e]&~0x48)|lastRange;
        ped[0x46f]=(ped[0x46f]&~2)|lastDuck;
        ped[0x719]=lastRate;ped[0x71a]=lastAccuracy;
    }
    void restore(unsigned char *ped) {
        if(!active) return;
        // Restore only fields still holding our value; preserve another mod's later changes.
        if((ped[0x46e]&0x48)==lastRange) ped[0x46e]=(ped[0x46e]&~0x48)|oldRange;
        if((ped[0x46f]&2)==lastDuck) ped[0x46f]=(ped[0x46f]&~2)|oldDuck;
        if(ped[0x719]==lastRate) ped[0x719]=oldRate;
        if(ped[0x71a]==lastAccuracy) ped[0x71a]=oldAccuracy;
        active=false;
    }
};
struct BarkMemory {
    struct Entry {std::deque<std::string> lines;std::uint32_t at=0;};
    std::map<unsigned,Entry> people;
    std::deque<std::string> recent;
    std::string choose(unsigned id,const std::vector<std::string> &lines,unsigned seed,std::uint32_t now) {
        if(!id || lines.empty()) return {};
        if(!people.count(id) && people.size()>=512) {
            auto oldest=people.begin();
            for(auto it=people.begin();it!=people.end();++it) if(now-it->second.at>now-oldest->second.at)oldest=it;
            people.erase(oldest);
        }
        auto &entry=people[id];
        if(!entry.lines.empty() && now-entry.at<240000) return {};
        for(unsigned i=0;i<lines.size();++i) {
            const auto &line=lines[(seed+i)%lines.size()];
            if(std::find(entry.lines.begin(),entry.lines.end(),line)!=entry.lines.end() ||
               std::find(recent.begin(),recent.end(),line)!=recent.end()) continue;
            entry.lines.push_back(line);if(entry.lines.size()>4)entry.lines.pop_front();entry.at=now;
            recent.push_back(line);if(recent.size()>3)recent.pop_front();
            if(people.size()>512) {
                auto oldest=people.begin();
                for(auto it=people.begin();it!=people.end();++it) if(now-it->second.at>now-oldest->second.at)oldest=it;
                people.erase(oldest);
            }
            return line;
        }
        return {}; // A quiet pedestrian is preferable to another immediate repeat.
    }
};
}
