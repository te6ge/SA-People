#include "GroveStandNative.inc"
// 1.0 US CPickups ABI/0x20-byte layout from plugin-sdk CPickups.cpp/CPickup.h.
// Own only generation-checked handles. Never remove pickups by radius or model alone.
struct GroveStockAPI {
    bool allowCreate=false;
    bool collected(int handle) {return reinterpret_cast<bool(__cdecl *)(int)>(0x454B40)(handle);}
    bool matches(int handle,unsigned slot) {
        const unsigned index=static_cast<unsigned>(handle)&0xffffu;
        if(handle==-1 || index>=620 || slot>=5)return false;
        const auto p=reinterpret_cast<const Byte *>(0x9788C0)+index*0x20;
        const auto spec=grovestock::item(slot);
        return *reinterpret_cast<const unsigned short *>(p+0x1A)==static_cast<unsigned short>(static_cast<unsigned>(handle)>>16) &&
            p[0x1C]==3 && static_cast<unsigned>(*reinterpret_cast<const unsigned short *>(p+0x18))==spec.model &&
            std::abs(*reinterpret_cast<const short *>(p+0x10)/8.0f-spec.x)<0.4f &&
            std::abs(*reinterpret_cast<const short *>(p+0x12)/8.0f-spec.y)<0.4f;
    }
    bool depleted(int handle,unsigned slot) {
        if(slot<2 || !matches(handle,slot))return false;
        const auto p=reinterpret_cast<const Byte *>(0x9788C0)+(static_cast<unsigned>(handle)&0xffffu)*0x20;
        return (p[0x1D]&2)!=0 || *reinterpret_cast<const unsigned *>(p+8)<grovestock::item(slot).ammo;
    }
    void remove(int handle){reinterpret_cast<void(__cdecl *)(int)>(0x4573D0)(handle);}
    int create(unsigned slot) {
        if(!allowCreate || !groveStandExists(slot<2?0u:1u))return -1;
        const auto spec=grovestock::item(slot);
        auto streaming=at<Byte *>(0x408B25);
        if(!streaming || streaming[spec.model*0x14+0x10]!=1) {
            reinterpret_cast<void(__cdecl *)(int,int)>(0x4087E0)(static_cast<int>(spec.model),0);
            return -1;
        }
        unsigned free=0;
        for(unsigned n=0;n<620;++n)if(reinterpret_cast<const Byte *>(0x9788C0)[n*0x20+0x1C]==0)++free;
        if(free<24)return -1; // Keep headroom for missions and the game's ordinary drops.
        const float shelf=groveStands[slot<2?0u:1u].top;
        return reinterpret_cast<int(__cdecl *)(Vec,unsigned,unsigned char,unsigned,unsigned,bool,char *)>(0x456F20)(
            Vec{spec.x,spec.y,shelf+0.25f},spec.model,3,spec.ammo,0,false,nullptr);
    }
};
grovestock::Manager groveStocks;
DWORD nextGroveStock=0;
void clearGroveStock() {
    GroveStockAPI api;
    if(groveStocks.clear(registry,api))dirty=true;
    clearGroveStands();
    nextGroveStock=0;
}
void updateGroveStock(Byte *player) {
    const auto p=player?position(player):Vec{};
    auto wanted=player?reinterpret_cast<Byte *(__cdecl *)(int)>(0x56E230)(0):nullptr;
    const float dx=p.x-2487.5f,dy=p.y+1687.2f;
    const bool enabled=profileReady && player && !storyActive() && !player[0x2F] &&
        *reinterpret_cast<float *>(player+0x540)>0 && wanted && *reinterpret_cast<unsigned *>(wanted+0x2C)==0 &&
        dx*dx+dy*dy<10000 && std::abs(p.z-13.4f)<12;
    const auto now=GetTickCount();GroveStockAPI api;api.allowCreate=due(now,nextGroveStock);
    if(api.allowCreate)nextGroveStock=now+1000;
    if(!enabled) {
        if(groveStocks.clear(registry,api))dirty=panelInvalid=true;
        clearGroveStands();return;
    }
    updateGroveStands(true,api.allowCreate);
    if(groveStocks.update(registry,true,api)){dirty=panelInvalid=true;}
}
