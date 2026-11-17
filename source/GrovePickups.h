#pragma once
#include "Grove.h"
namespace grovestock {
struct Item {unsigned model,ammo;float x,y,z;};
inline Item item(unsigned slot) {
    static const Item items[]={{1240,0,2485.6f,-1687.2f,13.4f},{1242,0,2486.4f,-1687.2f,13.4f},
        {346,34,2488.5f,-1687.2f,13.4f},{349,12,2489.0f,-1687.2f,13.4f},{352,60,2489.5f,-1687.2f,13.4f}};
    return slot<5?items[slot]:Item{};
}
class Manager {
    std::array<int,5> handles{{-1,-1,-1,-1,-1}};
    std::array<bool,5> spent{};
public:
    template<class API> bool clear(people::Registry &r,API &api) {
        bool changed=false;
        for(unsigned slot=0;slot<5;++slot) {
            auto &handle=handles[slot];if(handle==-1)continue;
            const bool matches=api.matches(handle,slot);
            const bool taken=api.collected(handle);
            if(!spent[slot] && (taken || (matches && api.depleted(handle,slot))))
                changed=grove::collected(r,slot)||changed;
            if(matches)api.remove(handle);
            handle=-1;spent[slot]=false;
        }
        return changed;
    }
    template<class API> bool update(people::Registry &r,bool enabled,API &api) {
        if(!enabled)return clear(r,api);
        bool changed=false;
        for(unsigned slot=0;slot<5;++slot) {
            auto &handle=handles[slot];
            if(handle!=-1) {
                const bool matches=api.matches(handle,slot);
                const bool taken=api.collected(handle);
                if(!spent[slot] && (taken || (matches && api.depleted(handle,slot)))) {
                    changed=grove::collected(r,slot)||changed;spent[slot]=true;
                }
                if(taken || !matches || (spent[slot] && grove::ready(r,slot))) {
                    if(matches)api.remove(handle);
                    handle=-1;spent[slot]=false;
                }
            }
            if(!grove::ready(r,slot)) {
                // Leave an ammo-drained gun present so the native weapon-swap key still works.
                if(handle!=-1 && !spent[slot]){api.remove(handle);handle=-1;}
                continue;
            }
            if(handle==-1){handle=api.create(slot);spent[slot]=false;}
        }
        return changed;
    }
    int handle(unsigned slot)const{return slot<5?handles[slot]:-1;}
};
}
