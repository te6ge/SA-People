#pragma once
#include "Network.h"
namespace stories {
enum News : unsigned { Robbed=1, Hurt, Helped, Peace, Confronted, Avoided,
    Sick, LostJob, NewWork, FamilyVisit, Recovered, Insulted, Remembered, Died };
enum Intent : unsigned { None, Avoid, Confront, Demand };
enum Life : unsigned { Normal, Ill, Unemployed, Working, Visiting };
inline std::uint64_t later(std::uint64_t now,unsigned minutes) {
    return std::min<std::uint64_t>(1000000000,now+minutes);
}
inline bool visibleWeapon(int weapon){return weapon>=2 && weapon<=39;}
inline unsigned temper(unsigned id) {return people::mix(id^0xac731u)%4;}
inline bool contact(const people::Family &f) {return f.social.journal && (f.social.known&people::Name);}
inline bool publish(people::Registry &r,unsigned person,unsigned kind,unsigned source=0) {
    if(!person || person>r.families.size() || source>r.families.size() || kind<1 || kind>14 ||
       r.nextNews>=1000000000)return false;
    for(auto it=r.news.rbegin();it!=r.news.rend();++it)
        if(it->person==person && it->kind==kind && r.worldMinutes>=it->minute && r.worldMinutes-it->minute<60) {
            if(!it->source && source)it->source=source;return false;
        }
    if(r.news.size()>=192)r.news.erase(r.news.begin());
    r.news.push_back({r.nextNews++,person,source,kind,r.worldMinutes});return true;
}
inline unsigned chooseIntent(const people::Family &f) {
    if(f.story.grievance<30 || !f.presence.alive)return None;
    if(temper(f.id)==0 || f.story.fear>=65)return Avoid;
    return f.story.grievance>=55 && temper(f.id)==3 ? Demand : Confront;
}
inline bool harm(people::Registry &r,unsigned id,unsigned kind,bool weapon=false) {
    if(!id || id>r.families.size() || (kind!=Robbed && kind!=Hurt && kind!=Helped && kind!=Insulted))return false;
    auto &f=r.families[id-1];auto &s=f.story;
    if(!f.presence.alive)return false;
    if(kind==Helped) {
        // Kindness is remembered, but it does not buy instant forgiveness for abuse.
        s.grievance=s.grievance>4?s.grievance-4:0;
        if(s.grievance<20)s.intent=None;
        if(contact(f))publish(r,id,Helped);
        return true;
    }
    // One blow or insult cannot generate a fresh story every frame.
    if(s.lastHarm && r.worldMinutes+1>=s.lastHarm && r.worldMinutes+1-s.lastHarm<15 &&
       (kind==Insulted || (kind==Hurt && s.lastDeed!=Insulted) || (kind==Robbed && s.lastDeed==Robbed)))return false;
    s.grievance=std::min(100u,s.grievance+(kind==Robbed?45u:kind==Hurt?35u:12u));
    s.fear=std::min(100u,s.fear+(weapon?25u:kind==Hurt?10u:0u));
    s.lastHarm=r.worldMinutes+1;s.lastDeed=kind;s.intent=chooseIntent(f);
    s.nextAction=later(r.worldMinutes,temper(id)==2?180u:temper(id)==3?1440u:480u);
    s.confrontations=0;publish(r,id,kind);return true;
}
inline bool peaceReady(const people::Registry &r,const people::Family &f) {
    return f.presence.alive && f.story.grievance>=20 && !f.social.feud &&
        f.story.grievance<85 && (!f.story.lastHarm || r.worldMinutes+1>=f.story.lastHarm+180) &&
        (!f.story.lastPeace || r.worldMinutes+1>=f.story.lastPeace+1440);
}
inline const char *makePeace(people::Registry &r,unsigned id,int &cash,bool compensate,bool confirmed) {
    if(!id || id>r.families.size())return "ST_PEACE_NO";
    auto &f=r.families[id-1];if(!peaceReady(r,f))return "ST_PEACE_NO";
    if(!confirmed)return "ST_CONFIRM_REQUIRED";
    if(compensate && cash<100)return "REL_NO_CASH";
    if(compensate)cash-=100;
    auto &s=f.story;s.lastPeace=r.worldMinutes+1;
    const unsigned relief=compensate?25u:12u;
    s.grievance=s.grievance>relief?s.grievance-relief:0;
    s.intent=chooseIntent(f);s.nextAction=later(r.worldMinutes,1440);
    f.social.anger=f.social.anger>12?f.social.anger-12:0;
    f.social.resentment=f.social.resentment>5?f.social.resentment-5:0;
    // No automatic contact, trust reward, romance restoration or feud removal.
    publish(r,id,Peace);return s.grievance>=20?"ST_PEACE_PARTIAL":"ST_PEACE_ACCEPTED";
}
inline bool due(const people::Registry &r,const people::Family &f) {
    return f.presence.alive && f.story.intent && f.story.grievance>=30 &&
        f.story.confrontations<3 && r.worldMinutes>=f.story.nextAction;
}
inline void acted(people::Registry &r,unsigned id,bool avoided) {
    auto &s=r.families.at(id-1).story;
    s.confrontations=std::min(3u,s.confrontations+1);
    s.nextAction=later(r.worldMinutes,1440+people::mix(id+s.confrontations)%1440);
    publish(r,id,avoided?Avoided:Confronted);
}
inline bool unavailable(const people::Registry &r,const people::Family &f) {
    return f.story.until>r.worldMinutes && (f.story.life==Ill || f.story.life==Visiting);
}
inline bool applyLife(people::Registry &r,unsigned id,unsigned kind) {
    if(!id || id>r.families.size() || kind<Ill || kind>Visiting)return false;
    auto &f=r.families[id-1];
    const auto type=f.presence.type;
    if(type!=4 && type!=5 && type!=21 && type!=22)return false;
    if(kind==Unemployed && type!=4 && type!=5)kind=Visiting;
    if(!contact(f) || !f.presence.alive || f.social.feud || f.social.cutOff || f.social.trust<15 || f.story.grievance>=30)return false;
    auto &s=f.story;s.life=kind;
    s.until=later(r.worldMinutes,kind==Ill?1440u:kind==Unemployed?2880u:kind==Working?1440u:360u);
    s.nextLife=later(r.worldMinutes,5760+people::mix(id+static_cast<unsigned>(r.worldMinutes))%5760);
    publish(r,id,kind==Ill?Sick:kind==Unemployed?LostJob:kind==Working?NewWork:FamilyVisit);
    // News is available in the phone feed; only close contacts send a separate text.
    if(f.social.trust>=35 && !f.social.cutOff)
        r.post(id,kind==Ill?"ST_MAIL_SICK":kind==Unemployed?"ST_MAIL_JOB":kind==Working?"ST_MAIL_WORK":"ST_MAIL_FAMILY");
    return true;
}
inline bool tick(people::Registry &r,unsigned id,bool occupied) {
    auto &f=r.families.at(id-1);auto &s=f.story;
    if(!contact(f) || !f.presence.alive)return false;
    if(!s.lastHarm && !s.lastPeace && !s.grievance && (f.social.robbed || f.social.resentment>=40)) {
        s.grievance=std::min(65u,std::max(30u,f.social.resentment));s.fear=f.social.robbed?15u:0u;
        s.lastHarm=r.worldMinutes+1;s.nextAction=later(r.worldMinutes,1440+people::mix(id)%1440);
        s.intent=chooseIntent(f);publish(r,id,Remembered);return true;
    }
    // Initialize from now: updating does not invent years of missed events.
    if(!s.nextLife){s.nextLife=later(r.worldMinutes,180+people::mix(id+23)%1260);return true;}
    if(occupied)return false;
    if(s.life && r.worldMinutes>=s.until) {
        const auto old=s.life;s.life=Normal;s.until=0;
        if(old==Unemployed){applyLife(r,id,Working);return true;}
        if(old==Ill){if(!f.social.cutOff && f.social.trust>=15)publish(r,id,Recovered);if(f.social.trust>=35 && !f.social.cutOff)r.post(id,"ST_MAIL_RECOVERED");}
        return true;
    }
    if(!s.life && r.worldMinutes>=s.nextLife) {
        const auto seed=people::mix(id+static_cast<unsigned>(s.nextLife));
        const unsigned kind=seed%3==0?Ill:seed%3==1?Unemployed:Visiting;
        if(applyLife(r,id,kind))return true;
        s.nextLife=later(r.worldMinutes,1440);return true;
    }
    return false;
}
inline const char *lifeStatus(const people::Registry &r,const people::Family &f) {
    return f.story.until<=r.worldMinutes?"ST_LIFE_NORMAL":f.story.life==Ill?"ST_LIFE_SICK":
        f.story.life==Unemployed?"ST_LIFE_JOB":f.story.life==Working?"ST_LIFE_WORK":"ST_LIFE_FAMILY";
}
inline const char *intentStatus(const people::Family &f) {
    return f.story.intent==Avoid?"ST_AVOID":f.story.intent==Confront?"ST_CONFRONT":
        f.story.intent==Demand?"ST_DEMAND":"ST_NO_GRUDGE";
}
inline const char *newsKey(unsigned kind) {
    static const char *keys[]={"ST_N_ROBBED","ST_N_HURT","ST_N_HELPED","ST_N_PEACE","ST_N_CONFRONT",
        "ST_N_AVOID","ST_N_SICK","ST_N_JOB","ST_N_WORK","ST_N_FAMILY","ST_N_RECOVERED","ST_N_INSULT","ST_N_MEMORY","ST_N_DIED"};
    return kind>=1 && kind<=14?keys[kind-1]:"ST_NEWS_EMPTY";
}
inline bool learned(const people::Registry &r,const people::StoryNews &n) {
    const auto &f=r.families[n.person-1];return contact(f) || (n.source && contact(r.families[n.source-1])) || ((f.social.known&people::Name) && n.kind<=6);
}
inline std::vector<const people::StoryNews *> news(const people::Registry &r,unsigned id=0) {
    std::vector<const people::StoryNews *> out;
    for(auto it=r.news.rbegin();it!=r.news.rend();++it)
        if((!id || it->person==id) && learned(r,*it))out.push_back(&*it);
    return out;
}
inline unsigned tie(const people::Registry &r,unsigned a,unsigned b) {
    for(const auto &e:r.connections)if((e.a==a&&e.b==b)||(e.a==b&&e.b==a))return e.kind+1;
    return 0;
}
inline bool seen(people::Registry &r,unsigned observer,unsigned person,people::Point point) {
    if(!observer || !person || observer==person || observer>r.families.size() || person>r.families.size() ||
        !people::valid(point) || !contact(r.families[observer-1]) || !contact(r.families[person-1]) ||
        !r.families[observer-1].presence.alive || !r.families[person-1].presence.alive ||
        !tie(r,observer,person) || tie(r,observer,person)==3)return false;
    for(auto &s:r.sightings)if(s.observer==observer && s.person==person) {
        if(r.worldMinutes==s.minute)return false;
        s.point=point;s.minute=r.worldMinutes;return true;
    }
    if(r.sightings.size()>=128) {
        const auto oldest=std::min_element(r.sightings.begin(),r.sightings.end(),[](const auto &a,const auto &b){return a.minute<b.minute;});
        r.sightings.erase(oldest);
    }
    r.sightings.push_back({observer,person,r.worldMinutes,point});return true;
}
enum class Lead { Unknown, Refused, Recent };
inline Lead inquire(const people::Registry &r,unsigned observer,unsigned person,people::Point &point) {
    if(!observer || !person || observer==person || observer>r.families.size() || person>r.families.size())return Lead::Unknown;
    const auto &f=r.families[observer-1];const auto &target=r.families[person-1];
    if(!contact(f) || !contact(target) || !f.presence.alive || f.social.cutOff || f.social.trust<20 ||
        f.social.anger>=25 || f.story.grievance>=30 || f.daily.busyUntil>r.worldMinutes || unavailable(r,f))return Lead::Refused;
    // Never answer from the target's archive position: the informant must actually have seen them.
    for(const auto &s:r.sightings)if(s.observer==observer && s.person==person &&
        s.minute<=r.worldMinutes && r.worldMinutes-s.minute<=1440) {
        if(!target.presence.alive)return Lead::Unknown;
        if(target.social.feud || target.social.cutOff || target.social.resentment>=40 || target.story.grievance>=55)return Lead::Refused; // Do not sell a friend's whereabouts to an enemy.
        point=s.point;return Lead::Recent;
    }
    return Lead::Unknown;
}
}
