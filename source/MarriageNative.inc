bool commitmentNear(Byte *player,unsigned id,people::Handle &actor) {
    auto pool=at<Pool *>(0xb74490);
    if(!player || storyActive() || !validPool(pool) || !onFoot(player) || busy(player) ||
       currentWeapon(player)>1 || !bindings.locate(id,actor) || !exists(pool,actor))return false;
    auto ped=pedAt(pool,actor.slot);
    return eligible(ped,player) && onFoot(ped) && !busy(ped) && !recruited(ped) &&
        ped[0x2f]==player[0x2f] && distance2(position(player),position(ped))<=25 &&
        std::abs(position(player).z-position(ped).z)<2.5f && visibleTarget(ped);
}
void relationshipChoice(Byte *player) {
    const auto id=phoneState.person;
    if(!id || id>registry.families.size())return;
    auto &f=registry.families[id-1];people::Handle actor{};
    if(f.commitment.stage!=phoneState.relationshipStage){phoneState.notice="PHONE_UNAVAILABLE";return;}
    if(!commitmentNear(player,id,actor)){phoneState.notice="REL_NEARBY";return;}
    auto cash=playerMoney(player);
    if(!cash){phoneState.notice="PHONE_UNAVAILABLE";return;}
    if(f.commitment.stage==0) {
        if(auto reason=commitment::proposalBlock(registry,f)){phoneState.notice=reason;return;}
        if(*cash<250){phoneState.notice="REL_NO_CASH";return;}
        if(!commitment::propose(registry,id,*cash,true))return;
        closePhone();gesture(at<Pool *>(0xb74490),actor,12);say(f,"REL_PROPOSAL_YES");
    } else if(f.commitment.stage==1) {
        if(*cash<500){phoneState.notice="REL_NO_CASH";return;}
        const auto where=position(player);
        if(!commitment::book(registry,id,*cash,commitment::nearest(where.x,where.y),true)) {
            phoneState.notice="REL_FINISH_PLANS";return;
        }
        releaseRoutine(id);closePhone();say(f,"REL_CEREMONY_READY");help("REL_CEREMONY_MAP");
    } else if(f.commitment.stage==2) {
        commitment::cancel(registry,id);if(companionOwns(id))cancelCompanion(false);phoneState.notice="REL_CANCELLED";
    } else {
        // Explicit confirmed separation keeps children and the existing identity/history.
        if(companionOwns(id))cancelCompanion(false);
        commitment::separate(registry,id,true);phoneState.notice="REL_SEPARATED";
    }
    dirty=panelInvalid=true;save();
}
bool beginCeremony(people::Family &f,people::Handle actor) {
    if(!commitment::booked(registry,f))return false;
    auto player=reinterpret_cast<Byte *(__cdecl *)(int)>(0x56e210)(0);
    people::Handle nearby{};
    if(!commitmentNear(player,f.id,nearby) || !commitment::stable(registry,f)) {
        help("REL_NEARBY");return false;
    }
    const auto now=GetTickCount();
    companion.stage=20;companion.sceneAt=now;companion.lastTick=now;
    auto ped=pedAt(at<Pool *>(0xb74490),actor.slot);
    auto task=reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61a5a0)(0x20);
    if(task) {
        reinterpret_cast<void(__thiscall *)(void *,int,bool,bool,float)>(0x62f310)(task,22000,false,false,4.0f);
        setTask(taskManager(ped),task,3);companion.task=task;
    }
    gesture(at<Pool *>(0xb74490),actor,12);
    say(f,"REL_VOW_START");help("REL_CEREMONY_STAY");
    log("REL ceremony started identity="+std::to_string(f.id));return true;
}
void runWeddingScene(Pool *pool,Byte *player) {
    if(!companion.person || companion.person>registry.families.size())return;
    auto &f=registry.families[companion.person-1];
    const auto id=f.id;const auto now=GetTickCount();const auto age=now-companion.sceneAt;
    if(!exists(pool,companion.actor) || !commitment::stable(registry,f) || !commitment::booked(registry,f) ||
       !onFoot(player) || player[0x2f] || busy(player) || currentWeapon(player)>1) {
        commitment::cancel(registry,id);cancelCompanion(false);help("REL_CEREMONY_INTERRUPTED");dirty=true;save();return;
    }
    auto ped=pedAt(pool,companion.actor.slot);
    if(recruited(ped) || busy(ped) || !onFoot(ped) || ped[0x2f] ||
       distance2(position(player),position(ped))>64 || *reinterpret_cast<float *>(ped+0x540)<=0) {
        commitment::cancel(registry,id);cancelCompanion(false);help("REL_CEREMONY_INTERRUPTED");dirty=true;save();return;
    }
    protectOutingActor(pool,companion.actor,id);
    if(companion.stage==20 && age>=4500) {
        companion.stage=21;speaker="CJ";spoken=locale::tr("REL_VOW_CJ");subtitleUntil=now+4500;
    } else if(companion.stage==21 && age>=9500) {
        companion.stage=22;say(f,"REL_VOW_PARTNER");
    } else if(companion.stage==22 && age>=15000) {
        if(commitment::marry(registry,id)) {
            bonds::count(registry,bonds::Stat::Outing);
            say(f,"REL_WEDDING_DONE");log("REL married identity="+std::to_string(id));
        } else {commitment::cancel(registry,id);help("REL_CEREMONY_INTERRUPTED");}
        cancelCompanion(false);dirty=panelInvalid=true;save();
    }
}
DWORD nextCommitmentTick=0;
void updateCommitments() {
    const auto now=GetTickCount();if(!due(now,nextCommitmentTick))return;nextCommitmentTick=now+1000;
    for(auto &f:registry.families)
        if((f.social.romance>=3 || !f.commitment.empty()) && commitment::update(registry,f))dirty=true;
}
