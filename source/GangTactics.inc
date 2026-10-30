// A bounded director for native gang fights. It never replaces an existing native combat
// task, a recruit's group task, physical responses, or story scripts.
struct TacticalLease {
    people::Handle actor{};
    npcbrain::Posture posture;
    DWORD seen=0;
};
std::vector<TacticalLease> tacticalLeases;
DWORD nextTacticalScan=0;
unsigned tacticalCursor=0;
void restoreTacticalCombat(Pool *pool) {
    for(auto &lease:tacticalLeases)
        if(validPool(pool) && exists(pool,lease.actor)) lease.posture.restore(pedAt(pool,lease.actor.slot));
    tacticalLeases.clear();
}
Byte *verifiedCombatTarget(Pool *pool,Byte *ped,Byte *player) {
    auto manager=taskManager(ped);
    if(!manager) return nullptr;
    for(int slot=1;slot<4;++slot) {
        auto task=at<void *>(reinterpret_cast<std::uintptr_t>(manager)+slot*4);
        if(gangTaskId(task)!=1000) continue; // Only this constructor's target layout is verified.
        auto target=*reinterpret_cast<Byte **>(static_cast<Byte *>(task)+0x10);
        if(target==player) return target;
        const auto address=reinterpret_cast<std::uintptr_t>(target);
        const auto start=reinterpret_cast<std::uintptr_t>(pool->objects);
        if(address<start || (address-start)%0x7c4 || (address-start)/0x7c4>=static_cast<unsigned>(pool->size))continue;
        const auto index=static_cast<unsigned>((address-start)/0x7c4);
        if(!(pool->flags[index]&0x80)) {
            const int type=*reinterpret_cast<int *>(target+0x598);
            if(type>=7 && type<=16)return target;
        }
    }
    return nullptr;
}
void updateTacticalCombat(Pool *pool,Byte *player,bool mission,bool war) {
    if(!combatAI || mission || !validPool(pool) || !player || player[0x2f] ||
       *reinterpret_cast<float *>(player+0x540)<=0) {restoreTacticalCombat(pool);return;}
    const auto now=GetTickCount();
    if(!due(now,nextTacticalScan))return;
    nextTacticalScan=now+750;
    unsigned assigned=0, searches=0, raycasts=0;
    tacticalCursor=(tacticalCursor+37)%static_cast<unsigned>(pool->size);
    std::unordered_map<std::uintptr_t,unsigned> pressure;
    for(unsigned offset=0;offset<static_cast<unsigned>(pool->size);++offset) {
        const unsigned slot=(tacticalCursor+offset)%static_cast<unsigned>(pool->size);
        if(pool->flags[slot]&0x80)continue;
        auto ped=pedAt(pool,slot);
        const int type=*reinterpret_cast<int *>(ped+0x598);
        if(type<7 || type>16 || distance2(position(player),position(ped))>6400)continue;
        auto actor=handle(pool,slot);const unsigned id=bindings.find(actor);
        auto target=verifiedCombatTarget(pool,ped,player);
        const bool sociallyOwned=id && (companionOwns(id) || hasAppointment(id) || communityOwns(id));
        auto manager=taskManager(ped);
        if(!manager)continue;
        bool protectedResponse=(recruited(ped) && (!war || type!=8)) || nativeConversation(ped) ||
            at<void *>(reinterpret_cast<std::uintptr_t>(manager))!=nullptr;
        for(int n=1;n<3;++n) {
            auto response=at<void *>(reinterpret_cast<std::uintptr_t>(manager)+n*4);
            protectedResponse=protectedResponse || (response && !gangcombat::combatTask(gangTaskId(response)));
        }
        if(!npcbrain::mayAssist(type,ped[0x484],mission,ped[0x2f]!=player[0x2f],
            *reinterpret_cast<float *>(ped+0x540)>0,onFoot(ped),protectedResponse,sociallyOwned,war,target!=nullptr))continue;
        bool combat=target!=nullptr;
        for(int n=1;n<4;++n)combat=combat || gangcombat::combatTask(gangTaskId(at<void *>(reinterpret_cast<std::uintptr_t>(manager)+n*4)));
        const int weapon=currentWeapon(ped);
        // Recover passive ambient fighters only when a native gang war is already active.
        // Respect both local dislike/hate masks (including personal truces) and group tasks.
        if(!combat && weapon>=22 && weapon<=34 && war && !recruited(ped) && ped[0x484]==1 && !nativeConversation(ped) && assigned<3 && searches<6 && raycasts<24 &&
           gangcombat::ambientTask(gangTaskId(at<void *>(reinterpret_cast<std::uintptr_t>(manager)+12)))) {
            bool idle=true;
            for(int n=0;n<3;++n)idle=idle && !at<void *>(reinterpret_cast<std::uintptr_t>(manager)+n*4);
            const unsigned hate=*reinterpret_cast<unsigned *>(ped+0x4f0)|*reinterpret_cast<unsigned *>(ped+0x4ec);
            Byte *best=nullptr;float bestScore=2500;
            if(idle)++searches;
            if(idle) for(unsigned n=0;n<static_cast<unsigned>(pool->size);++n) {
                if(n==slot || (pool->flags[n]&0x80))continue;
                auto opponent=pedAt(pool,n);
                const int enemyType=*reinterpret_cast<int *>(opponent+0x598);
                if((opponent!=player && (enemyType<7 || enemyType>16)) || enemyType==type || enemyType<0 || enemyType>31 ||
                   !(hate&(1u<<enemyType)) || opponent[0x2f]!=ped[0x2f] || *reinterpret_cast<float *>(opponent+0x540)<=0)continue;
                const auto point=position(opponent);
                const float opponentDistance=distance2(position(ped),point);
                if(opponentDistance>2500 || std::abs(position(ped).z-point.z)>4)continue;
                if(raycasts>=24)break;
                ++raycasts;
                auto eye=position(ped),head=point;eye.z+=1;head.z+=1;
                if(std::abs(eye.z-head.z)>4 || !reinterpret_cast<bool(__cdecl *)(const Vec &,const Vec &,bool,bool,bool,bool,bool,bool,bool)>(
                    0x56a490)(eye,head,true,false,false,true,true,false,false))continue;
                const auto address=reinterpret_cast<std::uintptr_t>(opponent);
                const auto value=npcbrain::score(opponentDistance,pressure[address],true);
                if(value<bestScore){bestScore=value;best=opponent;}
            }
            if(best) {
                auto task=reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61a5a0)(0x38);
                if(task) {
                    reinterpret_cast<void(__thiscall *)(void *,Byte *,int,int,int,int,char)>(0x620e30)(
                        task,best,18000,0,350,80,0);
                    ownResponse(ped,task,18000,true);target=best;combat=true;++assigned;
                    ++pressure[reinterpret_cast<std::uintptr_t>(best)];
                }
            }
        }
        if(!combat || weapon<22 || weapon>34)continue; // Never change melee, throwables or explosives.
        auto found=std::find_if(tacticalLeases.begin(),tacticalLeases.end(),[&](const auto &x){return x.actor==actor;});
        if(found==tacticalLeases.end()) {
            if(tacticalLeases.size()>=32)continue;
            tacticalLeases.push_back({actor,{},now});found=tacticalLeases.end()-1;
        }
        const auto health=*reinterpret_cast<float *>(ped+0x540);
        found->posture.apply(ped,npcbrain::role(id?id:slot,health),target?distance2(position(ped),position(target)):225,weapon);
        found->seen=now;
    }
    for(auto it=tacticalLeases.begin();it!=tacticalLeases.end();) {
        if(!exists(pool,it->actor) || now-it->seen>=1500) {
            if(exists(pool,it->actor))it->posture.restore(pedAt(pool,it->actor.slot));
            it=tacticalLeases.erase(it);
        } else ++it;
    }
}
