// Social world actions own their tasks, actors and newly created cars only.
struct SocialHold { unsigned person; people::Handle actor; void *task; DWORD until; };
std::vector<SocialHold> socialHolds;
struct StreetPair { unsigned a=0,b=0; people::Handle actorA{},actorB{}; DWORD until=0; bool replied=false; } streetPair;
struct WitnessRun { unsigned person; people::Handle actor; community::Response response; DWORD at,expires; bool warned=false; float victimHealth=0; };
std::vector<WitnessRun> witnessRuns;
std::unordered_map<unsigned,DWORD> deedAfter;
DWORD nextCommunityTick=0,nextStreetPair=0,nextRelative=0;
std::unordered_map<unsigned,DWORD> rescueOfferAfter;
unsigned relativeArrival=0;
DWORD relativeUntil=0;
bool communityEnabled=true;
struct FriendService {
    unsigned person=0,kind=0,stage=0;
    people::Handle actor{},car{};
    void *task=nullptr;
    DWORD started=0,retry=0,actionAt=0;
    Vec destination{};
    bool createdCar=false;
    bool boardHeld=false;
    void *playerTask=nullptr;
    community::Progress progress;
} friendService;
struct Rescue {
    unsigned person=0,stage=0;
    people::Handle actor{},car{};
    void *task=nullptr;
    DWORD started=0,actionAt=0,retry=0;
    Vec hospital{};
    bool ambulance=false;
    int blip=-1;
} rescue;
bool communityOwns(unsigned id) {
    if (!id) return false;
    return id==friendService.person || id==rescue.person || id==relativeArrival ||
        id==streetPair.a || id==streetPair.b ||
        std::any_of(socialHolds.begin(),socialHolds.end(),[&](const auto &h){return h.person==id;}) ||
        std::any_of(witnessRuns.begin(),witnessRuns.end(),[&](const auto &w){return w.person==id;});
}
bool communityBusy() {return friendService.person || rescue.person || streetPair.a;}
bool communityPairPauses(unsigned id) {return id && (streetPair.a==id || streetPair.b==id);}
bool socialIdle(Byte *ped) {
    auto manager=taskManager(ped);
    return manager && !busy(ped) && !nativeConversation(ped) &&
        gangcombat::ambientTask(gangTaskId(at<void *>(reinterpret_cast<std::uintptr_t>(manager)+12)));
}
bool pairIdle(Byte *ped,unsigned id) {
    if(socialIdle(ped))return true;
    auto manager=taskManager(ped);
    return companionOwns(id) && manager && !busy(ped) && companion.task &&
        at<void *>(reinterpret_cast<std::uintptr_t>(manager)+12)==companion.task;
}
void releaseSocialActor(unsigned id) {
    if (!communityOwns(id) && !companionOwns(id) && !hasAppointment(id) && !departureOwns(id) &&
        !recognitionOwns(id) && lifeEvent.person!=id && roadside.person!=id && smallEvent.person!=id)
        releaseActor(id);
}
void clearOwnedTask(Pool *pool,people::Handle actor,void *task) {
    if (!task || !exists(pool,actor)) return;
    auto manager=taskManager(pedAt(pool,actor.slot));
    if(manager && at<void *>(reinterpret_cast<std::uintptr_t>(manager)+12)==task) setTask(manager,nullptr,3);
}
bool talkingHold(unsigned id) {
    auto pool=at<Pool *>(0xB74490);
    for(const auto &h:socialHolds) if(h.person==id && exists(pool,h.actor)) {
        auto manager=taskManager(pedAt(pool,h.actor.slot));
        return manager && at<void *>(reinterpret_cast<std::uintptr_t>(manager)+12)==h.task;
    }
    return false;
}
void releaseTalkingHold(unsigned id) {
    auto pool=at<Pool *>(0xB74490);
    for(auto it=socialHolds.begin();it!=socialHolds.end();) {
        if(it->person==id) {clearOwnedTask(pool,it->actor,it->task);it=socialHolds.erase(it);}
        else ++it;
    }
    releaseSocialActor(id);
}
void holdSocial(Pool *pool,unsigned id,people::Handle actor,DWORD until) {
    if(!exists(pool,actor)) return;
    for(auto &h:socialHolds) if(h.person==id) {h.until=until;return;}
    auto ped=pedAt(pool,actor.slot);
    if(!socialIdle(ped) || recruited(ped)) return;
    auto task=reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x20);
    if(!task) return;
    reinterpret_cast<void(__thiscall *)(void *,int,bool,bool,float)>(0x62F310)(task,15000,false,false,4.0f);
    setTask(taskManager(ped),task,3);
    retainActor(actor,id);
    socialHolds.push_back({id,actor,task,until});
}
bool npcSees(Byte *observer,Vec point) {
    Vec eyes=position(observer); eyes.z+=1; point.z+=1;
    return reinterpret_cast<bool(__cdecl *)(const Vec &,const Vec &,bool,bool,bool,bool,bool,bool,bool)>(0x56A490)(
        eyes,point,true,false,false,true,true,false,false);
}
void observeDeed(unsigned victim,community::Deed deed) {
    if(!communityEnabled || storyActive() || !victim || victim>registry.families.size()) return;
    const DWORD now=GetTickCount();
    const unsigned key=victim*8+static_cast<unsigned>(deed);
    if(deedAfter.count(key) && !due(now,deedAfter[key])) return;
    deedAfter[key]=now+(deed==community::Deed::Help?300000:15000);
    if(deedAfter.size()>512) deedAfter.erase(deedAfter.begin());
    auto pool=at<Pool *>(0xB74490);
    auto player=reinterpret_cast<Byte *(__cdecl *)(int)>(0x56E210)(0);
    if(!player || !validPool(pool)) return;
    const auto token=community::incident(registry);
    if(!token) return;
    const auto &p=registry.families[victim-1].presence;
    const Vec scene{p.x,p.y,p.z};
    if(p.alive) community::remember(registry,victim,victim,deed,token);
    unsigned count=0;
    for(unsigned slot=0;slot<static_cast<unsigned>(pool->size) && count<6;++slot) {
        if(pool->flags[slot]&0x80) continue;
        auto actor=handle(pool,slot); auto ped=pedAt(pool,slot);
        const auto id=bindings.find(actor);
        if(!id || id==victim || !eligible(ped,player) || ped[0x2F]!=p.area ||
            *reinterpret_cast<float *>(ped+0x540)<=0 || distance2(position(ped),scene)>1225 ||
            !npcSees(ped,scene)) continue;
        if(!community::remember(registry,id,victim,deed,token)) continue;
        ++count;
        if(witnessRuns.size()<16 && !communityOwns(id) && !companionOwns(id) && !hasAppointment(id)) {
            const auto response=community::response(registry,id,victim,deed,armed(player));
            witnessRuns.push_back({id,actor,response,now+1200,now+12000});
        }
    }
    if (count) bonds::deed(registry, playerDistrict(), deed == community::Deed::Help ? 1 : -2);
    dirty=true;
    log("COMMUNITY observed kind="+std::to_string(static_cast<unsigned>(deed))+" victim="+
        std::to_string(victim)+" witnesses="+std::to_string(count));
}
Vec nearestHospital(Vec p) {
    const Vec places[]={{1177,-1323,14},{-2655,632,14.5f},{1607,1817,10.8f}};
    Vec best=places[0];
    for(auto place:places) if(distance2(p,place)<distance2(p,best)) best=place;
    return best;
}
void cancelRescue() {
    auto pool=at<Pool *>(0xB74490);
    if(validPool(pool)) clearOwnedTask(pool,rescue.actor,rescue.task);
    if(rescue.blip!=-1) reinterpret_cast<void(__cdecl *)(int)>(0x587CE0)(rescue.blip);
    const unsigned id=rescue.person; rescue={}; releaseSocialActor(id);
}
void cancelFriendService() {
    auto pool=at<Pool *>(0xB74490);
    if(validPool(pool)) clearOwnedTask(pool,friendService.actor,friendService.task);
    auto player=reinterpret_cast<Byte *(__cdecl *)(int)>(0x56E210)(0);
    if(player && friendService.playerTask) {
        auto manager=taskManager(player);
        if(manager && at<void *>(reinterpret_cast<std::uintptr_t>(manager)+12)==friendService.playerTask)
            setTask(manager,nullptr,3);
    }
    auto cars=at<Pool *>(0xB74494);
    if(friendService.createdCar && carExists(cars,friendService.car)) {
        auto car=carAt(cars,friendService.car.slot);
        // Native script cleanup maintains vehicle population counts and releases ownership.
        reinterpret_cast<void(__cdecl *)(Byte *)>(0x486670)(car);
    }
    const unsigned id=friendService.person; friendService={}; releaseSocialActor(id);
}
void cancelCommunity() {
    cancelRescue(); cancelFriendService();
    const auto pair=streetPair; streetPair={};
    const unsigned arriving=relativeArrival; relativeArrival=0;
    auto pool=at<Pool *>(0xB74490);
    if(validPool(pool)) for(const auto &h:socialHolds) clearOwnedTask(pool,h.actor,h.task);
    auto held=std::move(socialHolds); socialHolds.clear();
    auto witnesses=std::move(witnessRuns); witnessRuns.clear();
    for(const auto &h:held) releaseSocialActor(h.person);
    for(const auto &w:witnesses) releaseSocialActor(w.person);
    releaseSocialActor(pair.a); releaseSocialActor(pair.b); releaseSocialActor(arriving);
    deedAfter.clear(); nextCommunityTick=nextStreetPair=nextRelative=0;
    rescueOfferAfter.clear();
    passengerReservations.clear();
}
bool beginFriendService(Byte *player,unsigned id) {
    if(!communityEnabled || friendService.person || rescue.person || companion.person || eventActive() ||
        storyActive() || player[0x2F]) {phoneState.notice="SERVICE_BUSY";return false;}
    auto &f=registry.families[id-1];
    const auto perk=favors::perk(f);
    unsigned kind=perk==favors::Perk::Repair ? 1u : perk==favors::Perk::Heal ? 2u : 0u;
    if(people::generate(f)[0].job=="Sofor") kind=3;
    if(f.presence.type==18) kind=2;
    if(!kind) return false;
    auto car=drivenCar(player);
    if(kind==1 && !car) {phoneState.notice="FAVOR_NEED_CAR";return true;}
    if(kind==3 && !onFoot(player)) {phoneState.notice="SERVICE_ON_FOOT";return true;}
    releaseRoutine(id);
    friendService.person=id; friendService.kind=kind; friendService.started=GetTickCount();
    friendService.destination=nearestHospital(position(player));
    auto cars=at<Pool *>(0xB74494);
    if(car && validPool(cars)) friendService.car=carHandle(cars,static_cast<unsigned>((car-cars->objects)/0xA18));
    registry.post(id,kind==3?"SERVICE_PICKUP_COMING":"SERVICE_COMING");
    phoneState.notice="SERVICE_COMING";
    dirty=true; save(); return true;
}
bool serviceRoute(Byte *car,Vec target) {
    // Plugin-SDK CCarCtrl 0x42F870; CAutoPilot is CVehicle+0x390, mission+0x2A.
    const float distance=reinterpret_cast<float(__cdecl *)(Byte *,Vec *,int,bool)>(0x41CE30)(car,&target,0,true);
    return std::isfinite(distance);
}
void updateFriendService(Pool *pool,Byte *player,DWORD now) {
    if(!friendService.person) return;
    const auto id=friendService.person;
    auto &f=registry.families[id-1];
    if(storyActive() || !socialworld::available(f) || player[0x2F] || now-friendService.started>240000) {
        help("SERVICE_CANCELLED"); cancelFriendService(); return;
    }
    people::Handle actor{};
    if(!bindings.locate(id,actor) || !exists(pool,actor)) {
        if(!pending.id && due(now,friendService.retry)) {
            friendService.retry=now+5000;
            Vec spawn{}; if(meetingSpawn(position(player),player,spawn)) requestReturn(id,spawn,false,true);
        }
        return;
    }
    friendService.actor=actor;
    auto ped=pedAt(pool,actor.slot); auto manager=taskManager(ped);
    if(!eligible(ped,player) || !manager || recruited(ped)) {cancelFriendService();return;}
    retainActor(actor,id);
    const bool own=friendService.task && at<void *>(reinterpret_cast<std::uintptr_t>(manager)+12)==friendService.task;
    if(friendService.task && !own) friendService.task=nullptr;
    auto cars=at<Pool *>(0xB74494);
    if(friendService.kind==3) {
        if(!friendService.stage) {
            if(!onFoot(ped) || !socialIdle(ped)) return;
            // Build a friend's vehicle off screen, never take a pedestrian's occupied car.
            reinterpret_cast<void(__cdecl *)(int,int)>(0x4087E0)(445,0);
            auto info=at<Byte *>(0x408B25);
            Vec spawn{};
            if(!info || info[445*0x14+0x10]!=1 || !meetingSpawn(position(player),player,spawn)) return;
            auto car=reinterpret_cast<Byte *(__cdecl *)(int,Vec,unsigned char)>(0x431F80)(445,spawn,0);
            if(!car || !validPool(cars)) return;
            friendService.car=carHandle(cars,static_cast<unsigned>((car-cars->objects)/0xA18));
            friendService.createdCar=true;
            auto task=reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x50);
            if(!task){cancelFriendService();return;}
            reinterpret_cast<void(__thiscall *)(void *,Byte *)>(0x6402F0)(task,car);
            setTask(manager,task,3); friendService.task=task; friendService.stage=1; friendService.retry=now+1500;
            return;
        }
        if(!carExists(cars,friendService.car)) {cancelFriendService();return;}
        auto car=carAt(cars,friendService.car.slot);
        if(*reinterpret_cast<Byte **>(car+0x460)!=ped) return;
        if(friendService.stage==1) {
            if(!due(now,friendService.retry)) return;
            clearOwnedTask(pool,actor,friendService.task); friendService.task=nullptr;
            serviceRoute(car,position(player)); friendService.stage=2; friendService.retry=now+5000;
        }
        const bool aboard=*reinterpret_cast<Byte **>(player+0x58C)==car && !onFoot(player);
        if(friendService.stage==2) {
            if(distance2(position(car),position(player))<100) {
                car[0x3BA]=11; car[0x3D0]=0;
                if(!friendService.actionAt){say(f,"SERVICE_GET_IN");help("SERVICE_ENTER_PASSENGER");friendService.actionAt=now;}
                const bool board=controls::down(controls::Greet);
                if(board && !friendService.boardHeld && onFoot(player) && freeSeat(car)>=0 && !busy(player)) {
                    const auto delta=player-pool->objects;
                    if(delta<0 || delta%0x7C4 || delta/0x7C4>=pool->size)return;
                    const auto playerHandle=handle(pool,static_cast<unsigned>(delta/0x7C4));
                    const int door=socialcontext::passengerDoor(reservePassenger(car,playerHandle));
                    if(door<0)return;
                    auto task=reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x50);
                    if(task && door>=0) {
                        reinterpret_cast<void(__thiscall *)(void *,Byte *,int,bool)>(0x640340)(task,car,door,false);
                        setTask(taskManager(player),task,3);friendService.playerTask=task;
                    }
                }
                friendService.boardHeld=board;
                if(aboard) {
                    serviceRoute(car,friendService.destination); friendService.stage=3;
                    showRoutineRoute({friendService.destination.x,friendService.destination.y,friendService.destination.z});
                    say(f,"SERVICE_DRIVING_HOME");
                }
            } else if(due(now,friendService.retry)) {
                serviceRoute(car,position(player)); friendService.retry=now+5000;
            }
        } else if(friendService.stage==3) {
            if(!aboard) {cancelFriendService();return;}
            if(distance2(position(car),friendService.destination)<225) {
                car[0x3BA]=11; car[0x3D0]=0; say(f,"SERVICE_ARRIVED");
                f.community.serviceDay=static_cast<unsigned>(registry.worldMinutes/1440+1);
                favors::granted(registry,id,favors::Perk::Wanted);
                friendService.stage=4; friendService.retry=now+30000; dirty=true;
            }
        } else if(friendService.stage==4 && (!aboard || due(now,friendService.retry))) cancelFriendService();
        return;
    }
    Vec goal=position(player);
    Byte *car=nullptr;
    if(friendService.kind==1) {
        if(!carExists(cars,friendService.car)) {cancelFriendService();return;}
        car=carAt(cars,friendService.car.slot); goal=bonnet(car);
        if(!stoppedCar(car) || distance2(position(player),position(car))>225) return;
    }
    const float distance=distance2(position(ped),goal);
    if(distance>9) {
        if(friendService.progress.stuck(distance,now)) {
            if(friendService.progress.retries>=2) {help("SERVICE_PATH_BLOCKED");cancelFriendService();return;}
            clearOwnedTask(pool,actor,friendService.task); friendService.task=nullptr;
            friendService.progress.retried(now);
        }
        if(!friendService.task && socialIdle(ped)) friendService.task=walkingTask(ped,goal);
        return;
    }
    clearOwnedTask(pool,actor,friendService.task); friendService.task=nullptr;
    if(!friendService.actionAt){friendService.actionAt=now;gesture(pool,actor,12);say(f,"SERVICE_WORKING");return;}
    if(now-friendService.actionAt<4000) return;
    if(friendService.kind==1 && car) *reinterpret_cast<float *>(car+0x4C0)=1000;
    else *reinterpret_cast<float *>(player+0x540)=std::min(*reinterpret_cast<float *>(player+0x544),*reinterpret_cast<float *>(player+0x540)+40);
    favors::granted(registry,id,friendService.kind==1?favors::Perk::Repair:favors::Perk::Heal);
    f.community.serviceDay=static_cast<unsigned>(registry.worldMinutes/1440+1);
    say(f,"SERVICE_DONE");dirty=true;save();cancelFriendService();
}
bool communityAction(Pool *pool,Byte *player,unsigned id,street::Action action) {
    if(!communityEnabled || action==street::Action::Provoke || storyActive()) return false;
    people::Handle actor{};
    if(!bindings.locate(id,actor) || !exists(pool,actor)) return false;
    auto ped=pedAt(pool,actor.slot);
    const float health=*reinterpret_cast<float *>(ped+0x540);
    const auto &f=registry.families[id-1];
    if(!rescue.person && health>1 && health<=35 && f.community.accidentAt &&
       registry.worldMinutes+1>=f.community.accidentAt && registry.worldMinutes+1-f.community.accidentAt<=180 &&
       eligible(ped,player) && !hasAppointment(id) && !companionOwns(id) && !friendService.person) {
        rescue.person=id;rescue.actor=actor;rescue.started=GetTickCount();rescue.hospital=nearestHospital(position(player));
        retainActor(actor,id);
    }
    if(rescue.person!=id) return false;
    if(rescue.stage) return true;
    if(action==street::Action::Defuse) {
        const Vec point=position(ped);
        rescue.ambulance=reinterpret_cast<bool(__cdecl *)(unsigned,Vec)>(0x42FBC0)(416,point);
        if(rescue.ambulance){rescue.stage=2;say(f,"RESCUE_AMBULANCE_SENT");}
        else {help("RESCUE_AMBULANCE_BUSY");cancelRescue();}
        return true;
    }
    auto car=drivenCar(player);auto cars=at<Pool *>(0xB74494);
    if(car && stoppedCar(car) && freeSeat(car)>=0 && socialIdle(ped)) {
        const int door=socialcontext::passengerDoor(reservePassenger(car,rescue.actor));
        if(door<0)return true;
        auto task=reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x50);
        if(!task)return true;
        reinterpret_cast<void(__thiscall *)(void *,Byte *,int,bool)>(0x640340)(task,car,door,false);
        setTask(taskManager(ped),task,3);rescue.task=task;
        rescue.car=carHandle(cars,static_cast<unsigned>((car-cars->objects)/0xA18));rescue.stage=3;
        showRoutineRoute({rescue.hospital.x,rescue.hospital.y,rescue.hospital.z});say(f,"RESCUE_TO_HOSPITAL");
    } else if(onFoot(player) && distance2(position(player),position(ped))<9) {
        rescue.stage=1;rescue.actionAt=GetTickCount();gesture(pool,actor,12);say(f,"RESCUE_FIRST_AID");
    } else help("RESCUE_APPROACH");
    return true;
}
void updateRescue(Pool *pool,Byte *player,DWORD now) {
    if(!rescue.person)return;
    if(!exists(pool,rescue.actor) || !eligible(pedAt(pool,rescue.actor.slot),player) ||
       recruited(pedAt(pool,rescue.actor.slot)) || now-rescue.started>180000 || player[0x2F]) {
        if(now-rescue.started>180000)help("RESCUE_TIMEOUT");
        cancelRescue();return;
    }
    auto ped=pedAt(pool,rescue.actor.slot);
    auto &f=registry.families[rescue.person-1];
    if(*reinterpret_cast<float *>(ped+0x540)<=1){cancelRescue();return;}
    bool done=false;
    if(rescue.stage==1) {
        if(!onFoot(player) || distance2(position(player),position(ped))>16) {rescue.stage=0;help("RESCUE_INTERRUPTED");return;}
        done=now-rescue.actionAt>=4000;
    } else if(rescue.stage==2) {
        for(unsigned slot=0;slot<static_cast<unsigned>(pool->size);++slot) {
            if(pool->flags[slot]&0x80) continue;
            auto medic=pedAt(pool,slot);
            if(*reinterpret_cast<int *>(medic+0x598)==18 && medic[0x2F]==ped[0x2F] &&
               *reinterpret_cast<float *>(medic+0x540)>0 && distance2(position(medic),position(ped))<36) {done=true;break;}
        }
    } else if(rescue.stage==3) {
        auto cars=at<Pool *>(0xB74494);
        if(!carExists(cars,rescue.car)){cancelRescue();return;}
        auto car=carAt(cars,rescue.car.slot);
        if(onFoot(ped) && now-rescue.started>30000){help("RESCUE_BOARD_FAILED");cancelRescue();return;}
        done=!onFoot(ped) && *reinterpret_cast<Byte **>(ped+0x58C)==car &&
            *reinterpret_cast<Byte **>(car+0x460)==player && stoppedCar(car) && distance2(position(car),rescue.hospital)<225;
    }
    if(!done)return;
    auto attacker=*reinterpret_cast<Byte **>(ped+0x764);
    auto car=reinterpret_cast<Byte *(__cdecl *)(int,bool)>(0x56E0D0)(0,false);
    const bool caused=attacker && (attacker==player || attacker==car);
    *reinterpret_cast<float *>(ped+0x540)=std::max(*reinterpret_cast<float *>(ped+0x540),45.0f);
    f.community.accidentAt=0;
    if(community::aid(registry,f.id,caused)) {
        reputation(1);later::helped(registry,f.id);justMet(f,rescue.actor);observeDeed(f.id,community::Deed::Help);
    }
    say(f,"RESCUE_THANKS",npcvoice::Cue::Thank);dirty=true;save();cancelRescue();
}
void updateCommunity(Pool *pool,Byte *player) {
    if(!communityEnabled || storyActive())return;
    const DWORD now=GetTickCount();
    for(auto it=socialHolds.begin();it!=socialHolds.end();) {
        if(!exists(pool,it->actor) || due(now,it->until) || recruited(pedAt(pool,it->actor.slot)) ||
           distance2(position(player),position(pedAt(pool,it->actor.slot)))>900) {
            clearOwnedTask(pool,it->actor,it->task);const unsigned id=it->person;it=socialHolds.erase(it);releaseSocialActor(id);
        } else ++it;
    }
    updateFriendService(pool,player,now);updateRescue(pool,player,now);
    for(auto it=witnessRuns.begin();it!=witnessRuns.end();) {
        if(!exists(pool,it->actor) || due(now,it->expires)) {const auto id=it->person;it=witnessRuns.erase(it);releaseSocialActor(id);continue;}
        if(!due(now,it->at)){++it;continue;}
        auto ped=pedAt(pool,it->actor.slot);
        if(!eligible(ped,player) || recruited(ped) || !onFoot(ped) || !socialIdle(ped)) {++it;continue;}
        const auto response=it->response;
        if(response==community::Response::Flee) startFlee(ped,player);
        else if(response==community::Response::Defend) {
            const auto victim=registry.families[it->person-1].community.victim;
            if(!it->warned) {
                if(!due(now,subtitleUntil)){++it;continue;}
                say(registry.families[it->person-1],"WIT_DEFEND");it->warned=true;it->at=now+4000;
                it->victimHealth=victim && victim<=registry.families.size()?registry.families[victim-1].presence.health:0;
                ++it;continue;
            }
            if(victim && victim<=registry.families.size() && registry.families[victim-1].presence.health+0.5f<it->victimHealth &&
               !armed(player) && distance2(position(ped),position(player))<100 && npcSees(ped,position(player))) {
                setTask(taskManager(ped),nullptr,3);startFight(ped,player);
            }
        } else if(response==community::Response::Report) {
            auto wanted=reinterpret_cast<Byte *(__cdecl *)(int)>(0x56E230)(0);
            if(wanted && *reinterpret_cast<unsigned *>(wanted+0x2C)<2)
                reinterpret_cast<void(__thiscall *)(Byte *,int)>(0x562470)(wanted,2);
        }
        if(due(now,subtitleUntil) && !panelOpen) {
            say(registry.families[it->person-1],response==community::Response::Thanks?"WIT_HELP":
                response==community::Response::Report?"WIT_REPORT":response==community::Response::Defend?"WIT_DEFEND":"WIT_STOP");
        }
        const auto id=it->person;it=witnessRuns.erase(it);releaseSocialActor(id);
    }
    if(streetPair.a && due(now,streetPair.until)) {
        if(!streetPair.replied && exists(pool,streetPair.actorB) && !busy(pedAt(pool,streetPair.actorB.slot))) {
            streetPair.replied=true;streetPair.until=now+6000;
            say(registry.families[streetPair.b-1],community::tie(registry,streetPair.a,streetPair.b)==3?"PAIR_REPLY_RIVAL":"PAIR_REPLY",npcvoice::Cue::Hello);
            holdSocial(pool,streetPair.a,streetPair.actorA,now+6000);
            holdSocial(pool,streetPair.b,streetPair.actorB,now+6000);
        } else {
        const auto pair=streetPair;streetPair={};releaseSocialActor(pair.a);releaseSocialActor(pair.b);
        }
    }
    if(relativeArrival && due(now,relativeUntil)){const auto id=relativeArrival;relativeArrival=0;releaseSocialActor(id);}
    if(!due(now,nextCommunityTick))return;
    nextCommunityTick=now+2000;
    if(!rescue.person && !friendService.person && !companion.person && !eventActive() && !player[0x2F]) {
        for(unsigned slot=0;slot<static_cast<unsigned>(pool->size);++slot) {
            if(pool->flags[slot]&0x80)continue;
            const auto actor=handle(pool,slot);
            const unsigned id=bindings.find(actor);
            auto ped=pedAt(pool,slot);
            if(!id || !eligible(ped,player) || ped[0x2F] || hasAppointment(id) || communityOwns(id) ||
               (rescueOfferAfter.count(id) && !due(now,rescueOfferAfter[id])))continue;
            const auto &m=registry.families[id-1].community;
            const float health=*reinterpret_cast<float *>(ped+0x540);
            if(!m.accidentAt || !community::mayAid(registry,id) || health<=1 || health>35 ||
               distance2(position(player),position(ped))>625 || !visibleTarget(ped))continue;
            rescue.person=id;rescue.actor=actor;rescue.started=now;rescue.hospital=nearestHospital(position(player));
            retainActor(actor,id);rescueOfferAfter[id]=now+600000;
            if(rescueOfferAfter.size()>256)rescueOfferAfter.erase(rescueOfferAfter.begin());
            const int reference=static_cast<int>((actor.slot<<8)|(pool->flags[actor.slot]&0x7F));
            rescue.blip=reinterpret_cast<int(__cdecl *)(int,int,unsigned,int)>(0x5839A0)(2,reference,4,3);
            help("RESCUE_NEARBY");break;
        }
    }
    if(player[0x2F] || panelOpen || focusHeld || eventActive() || friendService.person || rescue.person || streetPair.a)return;
    // Nearby connected actors may pass a bounded, traceable rumor. No global broadcast.
    for(const auto &edge:registry.connections) {
        people::Handle a{},b{};
        if(!bindings.locate(edge.a,a)||!bindings.locate(edge.b,b)||!exists(pool,a)||!exists(pool,b))continue;
        auto pa=pedAt(pool,a.slot),pb=pedAt(pool,b.slot);
        if(!eligible(pa,player)||!eligible(pb,player)||pa[0x2F]!=pb[0x2F]||
           distance2(position(pa),position(pb))>64 || !npcSees(pa,position(pb)))continue;
        if(community::gossip(registry,edge.a,edge.b)||community::gossip(registry,edge.b,edge.a))dirty=true;
        if(!due(now,nextStreetPair) || !onFoot(pa)||!onFoot(pb)||!pairIdle(pa,edge.a)||!pairIdle(pb,edge.b)||
           (!companionOwns(edge.a)&&!companionOwns(edge.b)) || distance2(position(player),position(pa))>400) continue;
        releaseCompanionTask();releaseRoutine(edge.a);releaseRoutine(edge.b);
        streetPair={edge.a,edge.b,a,b,now+6500};
        holdSocial(pool,edge.a,a,now+6500);holdSocial(pool,edge.b,b,now+6500);
        const auto familiar=companionOwns(edge.a)?edge.a:edge.b, other=familiar==edge.a?edge.b:edge.a;
        auto &newPerson=registry.families[other-1];
        newPerson.social.known|=people::Name;
        const char *pairLine=edge.kind==2?"PAIR_RIVAL":edge.kind==1?"PAIR_SIBLING":edge.kind>=3?"PAIR_FAMILY":"PAIR_KNOWN";
        say(registry.families[familiar-1],locale::format(pairLine,dialogue::displayName(newPerson)));
        if(edge.kind==2) {companion.mood-=2;registry.families[familiar-1].community.strain=std::min(100u,registry.families[familiar-1].community.strain+5);}
        gesture(pool,a,12);gesture(pool,b,12);nextStreetPair=now+180000;dirty=true;break;
    }
    if(companion.person && companion.activity==3 && !relativeArrival && !pending.id && due(now,nextRelative)) {
        nextRelative=now+180000;
        for(const auto &edge:registry.connections) {
            const auto id=edge.a==companion.person?edge.b:edge.b==companion.person?edge.a:0;
            if(!id || edge.kind==0 || edge.kind==2 || bindings.active(id) || !registry.families[id-1].presence.alive)continue;
            const auto &p=registry.families[id-1].presence;
            if(p.area || distance2(position(player),{p.x,p.y,p.z})>250000)continue;
            Vec spawn{};if(meetingSpawn(position(player),player,spawn)) {
                requestReturn(id,spawn,false,true);relativeArrival=id;relativeUntil=now+30000;break;
            }
        }
    }
}
