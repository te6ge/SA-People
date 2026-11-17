#pragma once
#include "People.h"
#include <algorithm>
namespace grove {
enum Project : unsigned { Pantry, Workshop, Equipment, Arsenal, Count };
inline const char *name(unsigned project) {
    static const char *keys[]={"GROVE_PANTRY","GROVE_WORKSHOP","GROVE_EQUIPMENT","GROVE_ARSENAL"};
    return project<Count?keys[project]:"GROVE_TITLE";
}
inline const char *description(unsigned project) {
    static const char *keys[]={"GROVE_PANTRY_DESC","GROVE_WORKSHOP_DESC","GROVE_EQUIPMENT_DESC","GROVE_ARSENAL_DESC"};
    return project<Count?keys[project]:"GROVE_VISIT";
}
inline bool nearby(float x,float y,float z,unsigned interior) {
    const float dx=x-2488.0f,dy=y+1668.0f;
    return !interior && std::isfinite(x) && std::isfinite(y) && std::isfinite(z) &&
        dx*dx+dy*dy<=45.0f*45.0f && std::abs(z-13.3f)<=8;
}
inline const char *donate(people::Registry &r,int &cash,unsigned amount) {
    if(amount!=100 && amount!=1000 && amount!=10000)return "GROVE_INVALID";
    if(!r.grove.valid(r.worldMinutes))return "GROVE_INVALID";
    if(cash<0 || static_cast<unsigned>(cash)<amount)return "GROVE_NO_CASH";
    if(r.grove.balance>1000000-amount || r.grove.contributed>1000000000-amount)return "GROVE_FULL";
    cash-=static_cast<int>(amount);r.grove.balance+=amount;r.grove.contributed+=amount;
    return "GROVE_DONATED";
}
inline const char *upgrade(people::Registry &r,unsigned project,unsigned quotedCost) {
    if(project>=Count || !r.grove.valid(r.worldMinutes))return "GROVE_INVALID";
    const auto price=people::GroveFund::cost(project,r.grove.level[project]);
    if(!price)return "GROVE_MAX";
    if(quotedCost!=price)return "GROVE_CHANGED";
    if(r.grove.balance<price)return "GROVE_NO_FUND";
    r.grove.balance-=price;++r.grove.level[project];
    return "GROVE_BUILT";
}
struct Visit {
    bool here=false,safe=false,onFoot=false,driver=false,parked=false;
    float health=0,maxHealth=0,armor=0,carHealth=0;
};
enum Stock : unsigned { Health, Armor, Pistol, Shotgun, MicroSmg, Engine, StockCount };
inline unsigned projectFor(unsigned stock){return stock==Health?Pantry:stock==Armor?Equipment:stock==Engine?Workshop:Arsenal;}
inline bool unlocked(const people::Registry &r,unsigned stock) {
    if(stock>=StockCount)return false;
    return r.grove.level[projectFor(stock)]>=(stock==Shotgun || stock==MicroSmg?2u:1u);
}
inline unsigned restock(const people::Registry &r,unsigned stock) {
    return stock<StockCount && r.grove.level[projectFor(stock)]>=2 && projectFor(stock)!=Arsenal?720u:1440u;
}
inline bool ready(const people::Registry &r,unsigned stock) {
    if(!unlocked(r,stock))return false;
    const auto last=r.grove.usedAt[stock];
    return !last || (last<=r.worldMinutes+1 && r.worldMinutes+1-last>=restock(r,stock));
}
inline bool collected(people::Registry &r,unsigned stock) {
    if(!ready(r,stock))return false;
    r.grove.usedAt[stock]=r.worldMinutes+1;return true;
}
inline bool atLedger(float x,float y,float z,unsigned interior) {
    const float dx=x-2487.5f,dy=y+1686.0f;
    return !interior && std::isfinite(x) && std::isfinite(y) && std::isfinite(z) &&
        dx*dx+dy*dy<=100 && y>=-1689.0f && std::abs(z-13.4f)<=3;
}
inline const char *use(people::Registry &r,unsigned project,Visit &visit) {
    if(project>=Count || !r.grove.valid(r.worldMinutes))return "GROVE_INVALID";
    if(!visit.here)return "GROVE_VISIT";
    if(!visit.safe)return "GROVE_UNSAFE";
    if(!r.grove.level[project])return "GROVE_LOCKED";
    if(project!=Workshop)return "GROVE_COLLECT_OUTSIDE";
    if(!ready(r,Engine))return "GROVE_RESTOCK";
    if(!visit.driver || !visit.parked)return "GROVE_PARK";
    if(!std::isfinite(visit.carHealth) || visit.carHealth<=250)return "GROVE_WRECK";
    if(visit.carHealth>=1000)return "GROVE_NOT_NEEDED";
    visit.carHealth=std::min(1000.0f,visit.carHealth+250.0f*r.grove.level[Workshop]);
    collected(r,Engine);return "GROVE_REPAIR_DONE";
}
}
