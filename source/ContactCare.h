#pragma once
#include "Stories.h"
namespace contactcare {
enum Action : unsigned { CheckIn, Medicine, JobLead, Introduction, Count };
inline const char *label(unsigned action) {
    static const char *keys[]={"CARE_CHECK","CARE_MEDICINE","CARE_JOB","CARE_INTRO"};
    return action<Count?keys[action]:"CONFIRM_CANCEL";
}
inline bool recent(const people::Registry &r,unsigned id,unsigned action,unsigned minutes) {
    for(auto it=r.letters.rbegin();it!=r.letters.rend();++it)
        if(it->person==id && it->outgoing && it->body==label(action) &&
           (it->minute>r.worldMinutes || r.worldMinutes-it->minute<minutes))return true;
    return false;
}
inline unsigned recommendation(const people::Registry &r,unsigned id) {
    for(const auto &e:r.connections) {
        const auto other=e.a==id?e.b:e.b==id?e.a:0;
        if(!other || other==id || other>r.families.size() || e.kind==2)continue;
        const auto &f=r.families[other-1];
        const auto &s=f.social;
        if(!f.presence.alive || !f.presence.located || f.origin || s.journal ||
           (s.known&people::Name) || s.cutOff || s.feud || s.robbed || s.anger>=25 ||
           s.resentment>=30 || f.story.grievance>=20)continue;
        if(f.presence.type==4 || f.presence.type==5 || f.presence.type==21 || f.presence.type==22)return other;
    }
    return 0;
}
inline const char *apply(people::Registry &r,unsigned id,unsigned action,int &cash) {
    if(!id || id>r.families.size() || action>=Count)return "CARE_REFUSED";
    auto &f=r.families[id-1];auto &s=f.social;
    if(!stories::contact(f) || !f.presence.alive || s.trust<20 || s.cutOff || s.feud ||
       s.anger>=25 || s.resentment>=35 || f.story.grievance>=20)return "CARE_REFUSED";
    if(f.daily.busyUntil>r.worldMinutes)return "CARE_BUSY";
    if(recent(r,id,action,action==CheckIn?120u:1440u))return "CARE_WAIT";
    // Check every precondition before changing money, trust or another person's identity.
    const bool active=f.story.until>r.worldMinutes;
    if(action==Medicine && (!active || f.story.life!=stories::Ill))return "CARE_NOT_SICK";
    if(action==JobLead && (!active || f.story.life!=stories::Unemployed))return "CARE_NOT_JOB";
    if(action==Medicine && cash<25)return "CARE_NO_MONEY";
    if(action==Introduction && s.trust<40)return "CARE_REFUSED";
    const unsigned other=action==Introduction?recommendation(r,id):0;
    if(action==Introduction && !other)return "CARE_NO_INTRO";
    if(r.nextLetter>=999999998u)return "CARE_REFUSED";
    const char *reply="CARE_REPLY_NORMAL";
    if(action==Medicine || action==JobLead) {
        const auto minimum=action==Medicine?60u:180u;
        const auto reduction=action==Medicine?360u:720u;
        const auto remaining=f.story.until-r.worldMinutes;
        // Help saves time, but neither instantly cures illness nor guarantees immediate work.
        f.story.until=r.worldMinutes+std::min<std::uint64_t>(remaining,std::max<std::uint64_t>(minimum,remaining>reduction?remaining-reduction:0));
        s.trust=std::min(100,s.trust+2);
        if(action==Medicine)cash-=25;
        reply=action==Medicine?"CARE_REPLY_MEDICINE":"CARE_REPLY_JOB";
    } else if(action==Introduction) {
        r.families[other-1].social.known|=people::Name;
        // A name through a mutual friend is not consent to a phone-number exchange.
        reply="CARE_REPLY_INTRO";
    } else if(active) {
        reply=f.story.life==stories::Ill?"CARE_REPLY_SICK":f.story.life==stories::Unemployed?
            "CARE_REPLY_UNEMPLOYED":f.story.life==stories::Visiting?"CARE_REPLY_FAMILY":"CARE_REPLY_WORK";
    } else {
        static const char *replies[]={"CARE_REPLY_NORMAL","CARE_REPLY_NORMAL_2","CARE_REPLY_NORMAL_3"};
        reply=replies[people::mix(id+static_cast<unsigned>(r.worldMinutes/120))%3];
    }
    r.post(id,label(action),true);
    r.post(id,reply,false,0,other);
    return "CARE_SENT";
}
}
