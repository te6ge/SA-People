// Story consequences use people already in the local pool. No cross-map spawning,
// added weapons, forced attacks or changes to the game's gang-war ownership.
DWORD nextStoryTick=0,nextStoryRemark=0,nextStoryRoster=0;
std::vector<unsigned> storyRoster;
unsigned storyCursor=0,sightingCursor=0;
void resetStories(){nextStoryTick=nextStoryRemark=nextStoryRoster=storyCursor=sightingCursor=0;storyRoster.clear();}
bool storyStreet(people::Family &f,street::Action action) {
    if(f.story.grievance<20 || action==street::Action::Provoke)return false;
    if(action==street::Action::Defuse) {
        auto player=reinterpret_cast<Byte *(__cdecl *)(int)>(0x56e210)(0);
        if(!player || !onFoot(player) || currentWeapon(player)>1){say(f,"ST_PEACE_NEAR");return true;}
        int cash=0;const auto reply=stories::makePeace(registry,f.id,cash,false,true);
        say(f,reply);return true;
    }
    say(f,f.story.intent==stories::Avoid?"ST_STREET_AVOID":f.story.intent==stories::Demand?
        "ST_STREET_DEMAND":"ST_STREET_REMEMBER");return true;
}
void storyPeace(Byte *player,bool compensate,bool confirmed) {
    const auto id=phoneState.person;auto pool=at<Pool *>(0xb74490);people::Handle actor{};
    if(!storyLife || !player || storyActive() || !validPool(pool) || !onFoot(player) || busy(player) ||
       currentWeapon(player)>1 || !bindings.locate(id,actor) || !exists(pool,actor)) {
        phoneState.notice="ST_PEACE_NEAR";return;
    }
    auto ped=pedAt(pool,actor.slot);
    if(!eligible(ped,player) || *reinterpret_cast<float *>(ped+0x540)<=0 || !onFoot(ped) || busy(ped) || nativeConversation(ped) ||
       ped[0x2f]!=player[0x2f] || distance2(position(player),position(ped))>25 || !visibleTarget(ped) ||
       std::abs(position(player).z-position(ped).z)>2.5f) {phoneState.notice="ST_PEACE_NEAR";return;}
    auto cash=playerMoney(player);if(!cash){phoneState.notice="PHONE_UNAVAILABLE";return;}
    phoneState.notice=stories::makePeace(registry,id,*cash,compensate,confirmed);
    dirty=panelInvalid=true;save();
}
void storyInquiry(Byte *) {
    const auto ids=phone::contacts(registry);
    std::vector<unsigned> targets;for(auto id:ids)if(id!=phoneState.person)targets.push_back(id);
    if(!storyLife || targets.empty()) {phoneState.notice="ST_ASK_UNKNOWN";return;}
    const unsigned source=phoneState.person,target=targets[phoneState.cursor%targets.size()];
    if(!source || source>registry.families.size())return;
    const auto &informant=registry.families[source-1];
    if(!informant.presence.alive || informant.social.cutOff || informant.social.feud) {
        phoneState.notice="PHONE_UNAVAILABLE";return;
    }
    auto &memory=registry.families[source-1].story;
    if(memory.lastInquiry && registry.worldMinutes+1<memory.lastInquiry+60) {
        phoneState.notice="ST_ASK_LATER";return;
    }
    memory.lastInquiry=registry.worldMinutes+1;
    people::Point goal;const auto lead=stories::inquire(registry,source,target,goal);
    const auto targetName=locale::tr(dialogue::displayName(registry.families[target-1]));
    registry.post(source,locale::format("ST_ASK_TEXT",targetName),true);
    if(lead==stories::Lead::Recent) {
        const auto district=locale::tr(bonds::districtName(bonds::district(goal.x,goal.y)));
        const auto body=locale::format("ST_ASK_REPLY",targetName)+" "+district+". "+locale::tr("ST_CLUE_OLD");
        registry.post(source,body);
        // Deliberately approximate, with a three-minute map marker. It is not a live tracker.
        goal.x=std::floor(goal.x/80.0f)*80.0f+40;goal.y=std::floor(goal.y/80.0f)*80.0f+40;
        showRoutineRoute(goal);phoneState.notice="ST_CLUE_READY";
    } else {
        const char *key=lead==stories::Lead::Refused?"ST_ASK_REFUSED":"ST_ASK_UNKNOWN";
        registry.post(source,key);phoneState.notice=key;
    }
    dirty=panelInvalid=true;save();
}
void updateStories(Pool *pool,Byte *player) {
    const auto now=GetTickCount();
    if(!storyLife || !due(now,nextStoryTick) || storyActive() || !validPool(pool) || !player)return;
    nextStoryTick=now+1000;
    // A rotating budget avoids daily work growing with every archived stranger.
    if(due(now,nextStoryRoster)){storyRoster=phone::contacts(registry);nextStoryRoster=now+15000;}
    const unsigned count=static_cast<unsigned>(storyRoster.size());
    for(unsigned n=0;n<std::min(64u,count);++n) {
        const unsigned id=storyRoster[(storyCursor+n)%count];
        const bool occupied=hasAppointment(id) || companionOwns(id) || communityOwns(id) ||
            lifeEvent.person==id || roadside.person==id || smallEvent.person==id || focused==id ||
            recognitionOwns(id) || departureOwns(id) || registry.families[id-1].daily.busyUntil>registry.worldMinutes ||
            (registry.request.state==2 && registry.request.person==id);
        const unsigned before=registry.families[id-1].story.life;
        if(stories::tick(registry,id,occupied)) {
            dirty=panelInvalid=true;
            if(before!=registry.families[id-1].story.life)releaseRoutine(id);
        }
    }
    if(count)storyCursor=(storyCursor+64)%count;
    // Only friends/relatives who actually share the scene can acquire a sighting.
    const unsigned links=static_cast<unsigned>(registry.connections.size());unsigned rays=0;
    for(unsigned n=0;n<std::min(64u,links) && rays<8;++n) {
        const auto &edge=registry.connections[(sightingCursor+n)%links];
        if(edge.kind==2)continue;
        for(unsigned direction=0;direction<2 && rays<8;++direction) {
            const unsigned observer=direction?edge.b:edge.a,target=direction?edge.a:edge.b;
            const auto &f=registry.families[observer-1];const auto &other=registry.families[target-1];
            if(!stories::contact(f) || !stories::contact(other))continue;
            people::Handle first{},second{};
            if(!bindings.locate(observer,first) || !bindings.locate(target,second) ||
               !exists(pool,first) || !exists(pool,second))continue;
            auto ped=pedAt(pool,first.slot),seen=pedAt(pool,second.slot);
            if(ped[0x2f] || seen[0x2f] || *reinterpret_cast<float *>(ped+0x540)<=0 ||
               *reinterpret_cast<float *>(seen+0x540)<=0 ||
               distance2(position(ped),position(seen))>625 ||
               std::abs(position(ped).z-position(seen).z)>3)continue;
            ++rays;if(!npcSees(ped,position(seen)))continue;
            const auto point=position(seen);
            if(stories::seen(registry,observer,target,{point.x,point.y,point.z}))dirty=true;
        }
    }
    if(links)sightingCursor=(sightingCursor+64)%links;
    if(!onFoot(player) || player[0x2f] || panelOpen || focusHeld || eventActive() ||
       companion.person || communityBusy() || standoffActive() || nativeConversation(player) ||
       !due(now,nextSpeech) || !due(now,subtitleUntil) || !due(now,nextStoryRemark))return;
    for(unsigned slot=0;slot<static_cast<unsigned>(pool->size);++slot) {
        if(pool->flags[slot]&0x80)continue;
        auto ped=pedAt(pool,slot);const auto actor=handle(pool,slot);const unsigned id=bindings.find(actor);
        if(!id || !stories::due(registry,registry.families[id-1]) || !eligible(ped,player) ||
           ped[0x484]!=1 || !onFoot(ped) || ped[0x2f] || !socialIdle(ped) ||
           nativeConversation(ped) || hasAppointment(id) || communityOwns(id) || recognitionOwns(id) ||
           departureOwns(id) || leasedActor(ped) || distance2(position(player),position(ped))>64 ||
           std::abs(position(player).z-position(ped).z)>2.5f || !visibleTarget(ped))continue;
        const int type=*reinterpret_cast<int *>(ped+0x598);
        if(type!=4 && type!=5 && type!=21 && type!=22)continue;
        auto &f=registry.families[id-1];const bool avoid=f.story.intent==stories::Avoid || stories::visibleWeapon(currentWeapon(player));
        if(avoid) {
            releaseRoutine(id);
            // Only replace a verified ambient primary task; physical/combat responses were excluded.
            auto task=reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61a5a0)(0x40);
            if(!task)continue;
            reinterpret_cast<void(__thiscall *)(void *,Byte *,bool,float,int,int,float)>(0x65c430)(
                task,player,true,20.0f,7000,1000,1.0f);
            ownResponse(ped,task,7000);say(f,"ST_STREET_AVOID");
        } else {
            holdSocial(pool,id,actor,now+5000);
            say(f,f.story.intent==stories::Demand?"ST_STREET_DEMAND":"ST_STREET_REMEMBER");
        }
        stories::acted(registry,id,avoid);dirty=panelInvalid=true;
        nextSpeech=now+5000;nextStoryRemark=now+90000;return;
    }
}
