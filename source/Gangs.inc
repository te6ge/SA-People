// Local rival gang standoffs ([World] GangTalk=1). PED.DAT is read for rivalry against
// PLAYER1 or Grove. Only the nearby members' copied CPed acquaintances are leased;
// the global type table is never changed. Personal leases expire or restore on mission,
// interior entry, failure and restart. Recycled pool slots are never written through.
bool visibleTarget(Byte *ped);
bool gangTalk = true;
struct GangMasks {
    unsigned hate = 0, dislike = 0;
    DWORD until = 0;
    bool lifted = false;
};
GangMasks gangState[32];
struct GangAttack {
    people::Handle actor{};
    int gang = 0;
    DWORD expires = 0, retry = 0;
};
std::vector<GangAttack> gangAttacks;
int gangTaskId(void *task) {
    if(!task) return -1;
    auto table=*reinterpret_cast<std::uintptr_t **>(task);
    return reinterpret_cast<int(__thiscall *)(void *)>(table[4])(task);
}
void updateGangAttacks(Pool *pool, Byte *player) {
    const DWORD now=GetTickCount();
    for(auto it=gangAttacks.begin();it!=gangAttacks.end();) {
        if(!exists(pool,it->actor) || due(now,it->expires)) {
            if(due(now,it->expires)) log("GANG attack expired slot="+std::to_string(it->actor.slot));
            it=gangAttacks.erase(it); continue;
        }
        auto ped=pedAt(pool,it->actor.slot);
        const int type=*reinterpret_cast<int *>(ped+0x598);
        if(type!=it->gang || !gangcombat::eligible(type,ped[0x484],recruited(ped),storyActive(),
            ped[0x2F]==player[0x2F] && !player[0x2F],
            *reinterpret_cast<float *>(ped+0x540)>0 && *reinterpret_cast<float *>(player+0x540)>0,
            distance2(position(player),position(ped))) || !onFoot(ped)) {
            it=gangAttacks.erase(it); continue;
        }
        if(!due(now,it->retry)) {++it; continue;}
        const bool firstAttempt=it->retry==0;
        it->retry=now+250;
        auto manager=taskManager(ped);
        if(!manager) {++it; continue;}
        bool fighting=false;
        for(int slot=0;slot<4;++slot)
            fighting=fighting || gangcombat::combatTask(gangTaskId(at<void *>(reinterpret_cast<std::uintptr_t>(manager)+slot*4)));
        if(fighting) {log("GANG native combat active slot="+std::to_string(it->actor.slot)); it=gangAttacks.erase(it); continue;}
        const int primary=gangTaskId(at<void *>(reinterpret_cast<std::uintptr_t>(manager)+12));
        bool blocked=nativeConversation(ped) || nativeConversation(player);
        for(int slot=0;slot<3;++slot) {
            auto response=at<void *>(reinterpret_cast<std::uintptr_t>(manager)+slot*4);
            blocked=blocked || (response && !gangcombat::ambientTask(gangTaskId(response)));
        }
        if(blocked || !gangcombat::ambientTask(primary)) {
            if(firstAttempt) log("GANG attack deferred slot="+std::to_string(it->actor.slot)+" primary="+std::to_string(primary));
            ++it; continue;
        }
        // A passive staring/chat event can also occupy an event-response slot. Release only
        // those allowlisted idles; never clear a fall, injury, scripted sequence or fight.
        for(int slot=0;slot<3;++slot)
            if(at<void *>(reinterpret_cast<std::uintptr_t>(manager)+slot*4)) setTask(manager,nullptr,slot);
        auto task=reinterpret_cast<void *(__cdecl *)(unsigned)>(0x61A5A0)(0x38);
        if(!task) {++it; continue;}
        // CTaskComplexKillPedOnFoot selects melee/firearms from the ped's actual equipment.
        reinterpret_cast<void(__thiscall *)(void *,Byte *,int,int,int,int,char)>(0x620E30)(
            task,player,45000,0,0,100,0);
        ownResponse(ped,task,45000,true);
        log("GANG attack assigned gang="+std::to_string(type)+" slot="+std::to_string(it->actor.slot)+
            " replacedAmbient="+std::to_string(primary));
        it=gangAttacks.erase(it);
    }
}
struct GangPedLease {
    people::Handle actor{};
    int type = 0;
    DWORD until = 0;
    neighborhood::TruceBits bits;
};
std::vector<GangPedLease> gangPedLeases;
void liftGangMember(Pool *pool, people::Handle actor, int type, DWORD until) {
    if (!exists(pool,actor)) return;
    auto found=std::find_if(gangPedLeases.begin(),gangPedLeases.end(),
        [&](const auto &lease){return lease.actor==actor;});
    if(found==gangPedLeases.end()) {
        if(gangPedLeases.size()>=64) return;
        gangPedLeases.push_back({actor,type,until,{}}); found=gangPedLeases.end()-1;
    }
    auto ped=pedAt(pool,actor.slot);
    // CPed::m_acquaintance is copied on spawn; updating the type table alone leaves it hostile.
    // Plugin-SDK validates this member at 0x4E0 (dislike +0xC, hate +0x10).
    found->bits.lift(*reinterpret_cast<unsigned *>(ped+0x4EC),*reinterpret_cast<unsigned *>(ped+0x4F0));
    found->until=until;
}
void restoreGangMembers(bool all) {
    auto pool=at<Pool *>(0xB74490);
    for(auto it=gangPedLeases.begin();it!=gangPedLeases.end();) {
        const bool valid=validPool(pool) && exists(pool,it->actor);
        if(!valid || all || due(GetTickCount(),it->until)) {
            if(valid) {
                auto ped=pedAt(pool,it->actor.slot);
                if(*reinterpret_cast<int *>(ped+0x598)==it->type)
                    it->bits.restore(*reinterpret_cast<unsigned *>(ped+0x4EC),*reinterpret_cast<unsigned *>(ped+0x4F0));
            }
            it=gangPedLeases.erase(it);
        } else ++it;
    }
}
struct Standoff {
    std::vector<people::Handle> members;
    int gang = 0;
    DWORD until = 0;
} standoff;
std::vector<people::Handle> gangRefused; // Members who already turned CJ down.
DWORD nextGangScan = 0, gangCooldown = 0;
Byte *typeAcquaintance(int type) {
    auto table = at<Byte *>(0xC0BBE8);
    return table && type >= 0 && type < 32 ? table + type * 0x14 : nullptr;
}
bool gangHatesPlayer(int type) {
    auto a = typeAcquaintance(type);
    return a && neighborhood::hostile(*reinterpret_cast<unsigned *>(a + 0x10),
                                        *reinterpret_cast<unsigned *>(a + 0xC));
}
bool rivalGang(int type) {
    return neighborhood::rival(type); // Every gang but CJ's own Grove Street.
}
const char *gangName(int type) {
    static const char *const names[] = {"GANG_BALLAS", "GANG_GROVE", "GANG_VAGOS",
                                        "GANG_RIFA",   "GANG_DANANG", "GANG_MAFIA",
                                        "GANG_TRIAD",  "GANG_AZTECAS", "GANG_OTHER",
                                        "GANG_OTHER"};
    return type >= 7 && type <= 16 ? names[type - 7] : "GANG_OTHER";
}
void liftGang(int type, DWORD until) {
    if (type < 0 || type >= 32) return;
    gangState[type].lifted = true;
    gangState[type].until = until;
}
void restoreGang(int type) { gangState[type] = {}; }
void restoreGangs() {
    restoreGangMembers(true);
    for (int type = 0; type < 32; ++type)
        restoreGang(type);
}
bool standoffActive() {
    return !standoff.members.empty();
}
void endStandoff(bool restore) {
    if(restore) gangAttacks.clear();
    if (restore)
        restoreGangs();
    standoff = {};
}
// Drops an attack already under way (CPedIntelligence::ClearTasks, plugin-sdk 0x601420).
void calmMember(Byte *ped) {
    auto m = taskManager(ped);
    auto intelligence = *reinterpret_cast<Byte **>(ped + 0x47C);
    if (!m || !intelligence)
        return;
    bool responding = false; // Physical/event response slots hold an attack or chase.
    for (int slot = 0; slot <= 2; ++slot)
        responding = responding || at<void *>(reinterpret_cast<std::uintptr_t>(m) + slot * 4);
    if (responding)
        reinterpret_cast<void(__thiscall *)(Byte *, bool, bool)>(0x601420)(intelligence, true,
                                                                           false);
}
void gangLine(int gang, const std::string &line) {
    speaker = locale::tr(gangName(gang));
    spoken = line;
    subtitleUntil = GetTickCount() + 5500;
}
void failStandoff(const char *toast) {
    int gang = standoff.gang;
    const auto members=standoff.members;
    for (const auto &h : standoff.members) {
        if (gangRefused.size() >= 64)
            gangRefused.erase(gangRefused.begin());
        gangRefused.push_back(h);
    }
    endStandoff(true);
    const DWORD now=GetTickCount();
    for(const auto &member:members) gangAttacks.push_back({member,gang,now+8000,0});
    auto pool=at<Pool *>(0xB74490);
    auto player=reinterpret_cast<Byte *(__cdecl *)(int)>(0x56E210)(0);
    if(player && validPool(pool)) updateGangAttacks(pool,player);
    gangCooldown = GetTickCount() + 120000;
    gangLine(gang, locale::tr("GANG_FAIL_LINE"));
    help(locale::format(toast, locale::tr(gangName(gang))));
    audio.speak(audioChoices.choose(socialaudio::Context::Fight, GetTickCount()));
    log("GANG standoff failed gang=" + std::to_string(gang));
}
void negotiateGang(Byte *player, bool provoke) {
    if (!standoffActive())
        return;
    if (provoke) {
        failStandoff("GANG_FAIL");
        return;
    }
    DWORD now = GetTickCount();
    int chance = 40 + registry.honor / 5 - static_cast<int>(standoff.members.size()) * 4;
    if (currentWeapon(player) <= 1)
        chance += 10; // Empty hands read as no threat.
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
        chance -= 25; // Aiming at them.
    chance = std::clamp(chance, 10, 85);
    if (people::mix(now * 17 + static_cast<unsigned>(standoff.gang)) % 100 >=
        static_cast<unsigned>(chance)) {
        failStandoff("GANG_FAIL");
        return;
    }
    int gang = standoff.gang;
    liftGang(gang, now + 120000);
    auto pool=at<Pool *>(0xB74490);
    for(const auto &member:standoff.members) liftGangMember(pool,member,gang,now+120000);
    standoff = {};
    gangLine(gang, locale::tr("GANG_PEACE_LINE"));
    help(locale::format("GANG_PEACE", locale::tr(gangName(gang))));
    reputation(1);
    audio.speak(audioChoices.choose(socialaudio::Context::Help, now));
    log("GANG standoff settled gang=" + std::to_string(gang) + " chance=" + std::to_string(chance));
}
bool beginGangTalk(Pool *pool, Byte *player, people::Handle target) {
    if(!exists(pool,target) || !onFoot(player) || player[0x2F]) return false;
    auto ped=pedAt(pool,target.slot);
    const int type=*reinterpret_cast<int *>(ped+0x598);
    if(!eligible(ped,player) || !rivalGang(type) || !onFoot(ped) || ped[0x2F] ||
       !introductions::gangRange(distance2(position(player),position(ped)),visibleTarget(ped))) return false;
    standoff.gang=type; standoff.until=GetTickCount()+20000;
    liftGang(type,standoff.until+1000);
    for(unsigned slot=0;slot<static_cast<unsigned>(pool->size);++slot) {
        if(pool->flags[slot]&0x80) continue;
        auto member=pedAt(pool,slot);
        if(!eligible(member,player) || !onFoot(member) || member[0x2F] ||
           *reinterpret_cast<int *>(member+0x598)!=type || *reinterpret_cast<float *>(member+0x540)<=0 ||
           distance2(position(player),position(member))>144 || !visibleTarget(member)) continue;
        if(standoff.members.size()>=8) break;
        auto h=handle(pool,slot); standoff.members.push_back(h);
        liftGangMember(pool,h,type,standoff.until+1000); calmMember(member);
    }
    if(standoff.members.empty()) {endStandoff(true); return false;}
    gangLine(type,locale::tr("GANG_CHALLENGE"));
    help(locale::format("GANG_STANDOFF",locale::tr(gangName(type)))+" ("+
        controls::describe(controls::Focus)+" + "+controls::describe(controls::Greet)+")");
    log("GANG close standoff gang="+std::to_string(type)+" members="+std::to_string(standoff.members.size()));
    return true;
}
bool talkRival(Pool *pool, Byte *player, people::Handle target, bool provoke) {
    if(!gangTalk || !exists(pool,target) || player[0x2F]) return false;
    auto ped=pedAt(pool,target.slot);
    int type=*reinterpret_cast<int *>(ped+0x598);
    if(!rivalGang(type) || !eligible(ped,player)) return false;
    if(!gangHatesPlayer(type) && !gangState[type].lifted) return false;
    const unsigned id=bindings.find(target);
    if(id) {
        const auto &person=registry.families[id-1];
        if(person.social.journal && (street::isFriend(person) ||
            (person.social.helped && person.social.trust>=25 && introductions::comfortable(person)))) return false;
    }
    const bool member=standoffActive() && std::find(standoff.members.begin(),standoff.members.end(),target)!=standoff.members.end();
    if(provoke) {if(member) {negotiateGang(player,true); return true;} return false;}
    const DWORD now=GetTickCount();
    if(!due(now,nextSpeech)) return true;
    if(member) negotiateGang(player,false);
    else if(!due(now,gangCooldown) || standoffActive()) gangLine(type,locale::tr("GANG_NOT_FRIEND"));
    else if(gangState[type].lifted && !due(now,gangState[type].until)) {
        static const char *lines[]={"GANG_TRUCE_0","GANG_TRUCE_1","GANG_TRUCE_2"};
        static socialprogress::LineDeck deck;
        gangLine(type,locale::tr(lines[deck.take(3,now)]));
    } else if(beginGangTalk(pool,player,target)) negotiateGang(player,false);
    else gangLine(type,locale::tr("GANG_NOT_FRIEND"));
    nextSpeech=now+3500;
    return true;
}
void updateGangs(Pool *pool, Byte *player) {
    updateGangAttacks(pool,player);
    restoreGangMembers(false);
    if (!gangTalk)
        return;
    DWORD now = GetTickCount();
    if (player[0x2F] != 0) { // Interiors (and safehouse saves) always see the real relationship.
        if (standoffActive())
            endStandoff(true);
        restoreGangs();
        return;
    }
    for (int type = 0; type < 32; ++type)
        if (gangState[type].lifted && !standoffActive() && due(now, gangState[type].until)) {
            restoreGang(type);
            log("GANG truce ended gang=" + std::to_string(type));
        }
    if (standoffActive()) {
        bool anyone = false;
        for (const auto &h : standoff.members)
            anyone = anyone || (exists(pool, h) && distance2(position(player),position(pedAt(pool,h.slot)))<=225);
        if (!anyone || !onFoot(player))
            endStandoff(true);
        else if (due(now, standoff.until))
            failStandoff("GANG_SILENT");
        return;
    }
    if (!due(now, nextGangScan) || !due(now, gangCooldown) || !onFoot(player))
        return;
    nextGangScan = now + 400;
    gangRefused.clear(); // A refusal lasts for the cooldown, not the lifetime of a pool slot.
    Vec origin = position(player);
    struct Found {
        people::Handle h;
        int type;
        float d;
    };
    std::vector<Found> found;
    for (unsigned slot = 0; slot < static_cast<unsigned>(pool->size); ++slot) {
        if (pool->flags[slot] & 0x80)
            continue;
        auto ped = pedAt(pool, slot);
        int type = *reinterpret_cast<int *>(ped + 0x598);
        if (!eligible(ped, player) || !rivalGang(type) || gangState[type].lifted || recruited(ped) ||
            !onFoot(ped) || ped[0x2F] != 0 || *reinterpret_cast<float *>(ped + 0x540) <= 0 ||
            !gangHatesPlayer(type))
            continue;
        float d = distance2(origin, position(ped));
        if (d > 144)
            continue;
        auto h = handle(pool, slot);
        if (std::find(gangRefused.begin(), gangRefused.end(), h) != gangRefused.end())
            continue;
        found.push_back({h, type, d});
    }
    const Found *nearest = nullptr;
    for (const auto &f : found)
        if (introductions::gangRange(f.d,visibleTarget(pedAt(pool,f.h.slot))) && (!nearest || f.d < nearest->d))
            nearest = &f;
    if (!nearest)
        return;
    beginGangTalk(pool,player,nearest->h);
}
