// Owned display tables: SDK CObject (0x17C), object-pool generations and CWorld ABI.
// Removed with our pickups before native saves/loads/missions. Never delete other props.
struct GroveStand {people::Handle ref{};bool present=false;float top=0;};
std::array<GroveStand,2> groveStands;
bool groveStandExists(unsigned n) {
    if(n>=2 || !groveStands[n].present)return false;
    auto pool=at<Pool *>(0xB7449C);const auto h=groveStands[n].ref;
    if(!validPool(pool) || h.slot>=static_cast<unsigned>(pool->size) ||
       (pool->flags[h.slot]&0x80) || pool->flags[h.slot]!=h.generation)return false;
    auto object=pool->objects+h.slot*0x17C;
    return *reinterpret_cast<short *>(object+0x22)==2115 && object[0x13C]==2;
}
void clearGroveStands() {
    for(unsigned n=0;n<2;++n) {
        if(groveStandExists(n)) {
            auto object=at<Pool *>(0xB7449C)->objects+groveStands[n].ref.slot*0x17C;
            reinterpret_cast<void(__cdecl *)(Byte *)>(0x563280)(object);
            reinterpret_cast<void(__thiscall *)(Byte *)>(0x59F660)(object);
            reinterpret_cast<void(__cdecl *)(Byte *)>(0x5A1F20)(object);
        }
        groveStands[n]={};
    }
}
void updateGroveStands(bool enabled,bool retry) {
    if(!enabled){clearGroveStands();return;}
    if(!retry)return;
    auto streaming=at<Byte *>(0x408B25);
    if(!streaming || streaming[2115*0x14+0x10]!=1) {
        reinterpret_cast<void(__cdecl *)(int,int)>(0x4087E0)(2115,0);return;
    }
    auto pool=at<Pool *>(0xB7449C);if(!validPool(pool))return;
    unsigned free=0;for(int n=0;n<pool->size;++n)if(pool->flags[n]&0x80)++free;
    if(free<16)return;
    for(unsigned n=0;n<2;++n) {
        if(groveStandExists(n))continue;
        groveStands[n]={};const float x=n?2489.0f:2486.0f,y=-1687.2f;
        bool found=false;
        const float ground=reinterpret_cast<float(__cdecl *)(float,float,float,bool *,void **)>(0x5696C0)(x,y,15.4f,&found,nullptr);
        if(!found || !std::isfinite(ground) || std::abs(ground-13.4f)>2)continue;
        auto object=reinterpret_cast<Byte *(__cdecl *)(unsigned)>(0x5A1EE0)(0x17C);
        if(!object)continue;
        reinterpret_cast<void(__thiscall *)(Byte *,int,bool)>(0x5A1D70)(object,2115,true);
        auto col=reinterpret_cast<Byte *(__thiscall *)(Byte *)>(0x535300)(object);
        const auto minimum=col?*reinterpret_cast<Vec *>(col):Vec{};
        const auto maximum=col?*reinterpret_cast<Vec *>(col+12):Vec{};
        const float height=maximum.z-minimum.z;
        if(!col || !std::isfinite(height) || height<.35f || height>1.8f) {
            reinterpret_cast<void(__thiscall *)(Byte *)>(0x59F660)(object);
            reinterpret_cast<void(__cdecl *)(Byte *)>(0x5A1F20)(object);continue;
        }
        object[0x13C]=2;
        reinterpret_cast<void(__thiscall *)(Byte *,bool)>(0x5A0760)(object,true);
        const Vec pos{x-(minimum.x+maximum.x)/2,y-(minimum.y+maximum.y)/2,ground-minimum.z};
        reinterpret_cast<void(__thiscall *)(Byte *,const Vec &)>(0x4241C0)(object,pos);
        reinterpret_cast<void(__thiscall *)(Byte *)>(0x446F90)(object);
        reinterpret_cast<void(__thiscall *)(Byte *)>(0x532B00)(object);
        reinterpret_cast<void(__cdecl *)(Byte *)>(0x563220)(object);
        const auto slot=static_cast<unsigned>((object-pool->objects)/0x17C);
        groveStands[n].ref={slot,pool->flags[slot],2115};groveStands[n].present=true;
        groveStands[n].top=ground+height;
    }
}
