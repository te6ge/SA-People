// Free-roam civilians only. Native collision avoidance remains in charge; this is
// a short additional flee response, never a replacement for combat or story tasks.
bool streetAwareness=true;
DWORD nextAwareness=0,nextTrafficRemark=0;
awareness::Cooldowns trafficCooldowns;
void resetAwareness() {nextAwareness=nextTrafficRemark=0;trafficCooldowns={};}
void updateStreetAwareness(Pool *pool,Byte *player) {
    const auto now=GetTickCount();
    if(!streetAwareness || !due(now,nextAwareness))return;
    nextAwareness=now+250;
    if(storyActive() || player[0x2f] || panelOpen || focusHeld || eventActive() ||
       standoffActive() || companion.person || communityBusy() || nativeConversation(player))return;
    auto car=drivenCar(player);if(!car)return;
    const auto origin=position(car),velocity=*reinterpret_cast<Vec *>(car+0x44);
    const float vx=velocity.x*50.0f,vy=velocity.y*50.0f;
    if(vx*vx+vy*vy<36)return;
    unsigned rays=0;
    for(unsigned slot=0;slot<static_cast<unsigned>(pool->size);++slot) {
        if(pool->flags[slot]&0x80)continue;
        const auto actor=handle(pool,slot);const unsigned id=bindings.find(actor);
        if(!trafficCooldowns.ready(id,now))continue;
        auto ped=pedAt(pool,slot);const auto point=position(ped);
        if(!awareness::nearMiss(point.x-origin.x,point.y-origin.y,point.z-origin.z,vx,vy))continue;
        const bool owned=hasAppointment(id) || communityOwns(id) || companionOwns(id) ||
            recognitionOwns(id) || departureOwns(id) || leasedActor(ped);
        if(!awareness::civilian(*reinterpret_cast<int *>(ped+0x598),ped[0x484],ped[0x2f]!=0,
            *reinterpret_cast<float *>(ped+0x540)>0,onFoot(ped),
            !socialIdle(ped) || nativeConversation(ped),owned,recruited(ped)))continue;
        if(rays++>=4)break;
        if(!npcSees(ped,origin))continue;
        auto task=reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61a5a0)(0x40);
        if(!task)return;
        reinterpret_cast<void(__thiscall *)(void *,Byte *,bool,float,int,int,float)>(0x65c430)(
            task,player,true,12.0f,3500,500,1.0f);
        releaseRoutine(id);ownResponse(ped,task,3500);trafficCooldowns.record(id,now);
        if(due(now,nextTrafficRemark) && due(now,nextSpeech) && due(now,subtitleUntil)) {
            const auto key="WORLD_TRAFFIC_"+std::to_string(people::mix(id+now/1000)%4);
            say(registry.families[id-1],key);nextSpeech=now+4500;nextTrafficRemark=now+20000;
        }
        // No automatic honor penalty: native impacts and actual witnesses own consequences.
        return;
    }
}
