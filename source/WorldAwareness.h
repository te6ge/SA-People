#pragma once
#include <cmath>
#include <cstdint>
#include <map>
namespace awareness {
// Relative motion in metres/second. Ignore stationary traffic, receding cars,
// separated floors and paths that miss the pedestrian by a comfortable margin.
inline bool nearMiss(float dx,float dy,float dz,float vx,float vy,float radius=2.3f) {
    if(!std::isfinite(dx) || !std::isfinite(dy) || !std::isfinite(dz) ||
       !std::isfinite(vx) || !std::isfinite(vy) || std::abs(dz)>2.0f)return false;
    const float speed=vx*vx+vy*vy;
    if(speed<36 || dx*dx+dy*dy>225)return false;
    const float t=(dx*vx+dy*vy)/speed;
    if(t<=0 || t>0.85f)return false;
    const float x=dx-vx*t,y=dy-vy*t;
    return x*x+y*y<radius*radius;
}
inline bool civilian(int type,int createdBy,bool interior,bool alive,bool walking,
                     bool occupied,bool owned,bool recruited) {
    return (type==4 || type==5 || type==21 || type==22) && createdBy==1 &&
        !interior && alive && walking && !occupied && !owned && !recruited;
}
struct Cooldowns {
    std::map<unsigned,std::uint32_t> people;
    std::uint32_t last=0;bool used=false;
    bool ready(unsigned id,std::uint32_t now) const {
        const auto found=people.find(id);
        return id && (!used || now-last>=3000) &&
            (found==people.end() || now-found->second>=90000);
    }
    void record(unsigned id,std::uint32_t now) {
        for(auto it=people.begin();it!=people.end();)
            if(now-it->second>=90000)it=people.erase(it);else ++it;
        if(people.size()>=128)people.erase(people.begin());
        people[id]=now;last=now;used=true;
    }
};
}
