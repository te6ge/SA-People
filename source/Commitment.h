#pragma once
#include "Romance.h"
#include "Meetings.h"
namespace commitment {
inline const char *status(const people::Family &f) {
    if (!f.presence.alive && f.commitment.stage == 3) return "REL_BEREAVED";
    if (f.commitment.stage == 3)
        return f.social.cutOff || f.social.romance < 3 || f.social.trust < 20 ? "REL_ESTRANGED" : "REL_MARRIED";
    if (f.commitment.stage == 2) return "REL_BOOKED";
    if (f.commitment.stage == 1) return "REL_ENGAGED";
    return f.social.romance >= 3 ? "REL_PARTNER" : f.social.romance ? "REL_DATING" : "REL_KNOWN";
}
inline bool stable(const people::Registry &r, const people::Family &f) {
    return romance::eligible(r, f) && f.social.romance >= 3 && !f.social.feud &&
        f.social.trust >= 75 && f.social.anger < 15 && f.social.resentment < 15 &&
        f.community.strain < 40 && !f.social.robbed;
}
inline const char *proposalBlock(const people::Registry &r, const people::Family &f) {
    if (f.commitment.stage || !romance::eligible(r,f) || f.social.romance < 3) return "REL_NOT_PARTNER";
    if (!stable(r,f)) return "REL_NEED_TRUST";
    if (f.social.dates < 6 || !f.commitment.since || r.worldMinutes+1 < f.commitment.since+1440)
        return "REL_NEED_TIME";
    return nullptr;
}
inline bool propose(people::Registry &r, unsigned id, int &cash, bool confirmed) {
    if (!confirmed || !id || id > r.families.size() || cash < 250) return false;
    auto &f=r.families[id-1];
    if (proposalBlock(r,f)) return false;
    cash-=250; f.commitment.stage=1; f.commitment.proposedAt=r.worldMinutes+1;
    r.post(id,"REL_MAIL_ENGAGED"); return true;
}
inline const meetings::Venue &venue(unsigned index) {
    // Reuse outdoor meeting anchors already used by the normal arrival system.
    static const unsigned places[]={6,7,10};
    return meetings::venues()[places[(index ? index-1 : 0)%3]];
}
inline unsigned nearest(float x,float y) {
    unsigned best=1; float distance=1e30f;
    for(unsigned i=1;i<=3;++i) {
        const auto &v=venue(i); const float d=(v.x-x)*(v.x-x)+(v.y-y)*(v.y-y);
        if(d<distance){distance=d;best=i;}
    }
    return best;
}
inline bool booked(const people::Registry &r,const people::Family &f) {
    if(f.commitment.stage!=2) return false;
    const auto &v=venue(f.commitment.venue);
    for(const auto &a:r.appointments)
        if(a.person==f.id && meetings::active(a) && a.activity==3 &&
           (a.x-v.x)*(a.x-v.x)+(a.y-v.y)*(a.y-v.y)<=10000) return true;
    return false;
}
inline bool book(people::Registry &r,unsigned id,int &cash,unsigned location,bool confirmed) {
    if(!confirmed || !id || id>r.families.size() || cash<500 || location<1 || location>3) return false;
    auto &f=r.families[id-1];
    if(f.commitment.stage!=1 || !stable(r,f)) return false;
    const auto &v=venue(location);
    if(!meetings::schedule(r,id,v.x,v.y,v.z,30000,600000)) return false;
    for(auto &a:r.appointments) if(a.person==id && meetings::active(a)) a.activity=3;
    cash-=500; f.commitment.stage=2; f.commitment.venue=location;
    r.post(id,"REL_MAIL_CEREMONY"); return true;
}
inline bool cancel(people::Registry &r,unsigned id) {
    if(!id || id>r.families.size() || r.families[id-1].commitment.stage!=2) return false;
    auto &f=r.families[id-1];
    for(auto &a:r.appointments) if(a.person==id && meetings::active(a)) a.state=4;
    f.commitment.stage=1;f.commitment.venue=0; r.post(id,"REL_MAIL_POSTPONED");return true;
}
inline bool marry(people::Registry &r,unsigned id) {
    if(!id || id>r.families.size()) return false;
    auto &f=r.families[id-1];
    if(!booked(r,f) || !stable(r,f)) return false;
    for(const auto &other:r.families)
        if(other.id!=id && other.commitment.stage==3 && other.presence.alive) return false;
    f.commitment.stage=3;f.commitment.venue=0;f.commitment.marriedAt=r.worldMinutes+1;
    f.social.trust=std::min(100,f.social.trust+8);
    for(auto &a:r.appointments) if(a.person==id && meetings::active(a)) a.state=3;
    r.post(id,"REL_MAIL_MARRIED");return true;
}
inline bool separate(people::Registry &r,unsigned id,bool confirmed) {
    if(!confirmed || !id || id>r.families.size() || r.families[id-1].commitment.stage!=3) return false;
    auto &f=r.families[id-1];
    f.commitment={};f.social.romance=0;
    f.social.trust=std::max(-100,f.social.trust-15);
    f.social.resentment=std::min(100u,f.social.resentment+15);
    r.post(id,"REL_MAIL_SEPARATED");return true;
}
inline const char *talk(people::Registry &r,unsigned id) {
    if(!id || id>r.families.size()) return "PHONE_UNAVAILABLE";
    auto &f=r.families[id-1]; auto &c=f.commitment;
    if(!f.presence.alive || !f.social.romance || f.social.cutOff || f.social.feud) return "REL_NEED_SPACE";
    const auto now=r.worldMinutes+1;
    if(c.lastTalk && now<c.lastTalk+180) return "REL_TALK_LATER";
    c.lastTalk=now;
    const char *reply=f.social.resentment>=15 || f.community.strain>=40 ? "REL_TALK_HURT"
        : c.stage==3 ? "REL_TALK_MARRIED" : c.stage ? "REL_TALK_ENGAGED" : "REL_TALK_PARTNER";
    // Talking acknowledges problems; it never erases the consequences of violence.
    r.post(id,reply);return reply;
}
inline bool update(people::Registry &r,people::Family &f) {
    auto &c=f.commitment;
    if(!c.since && f.social.romance>=3 && f.presence.alive){c.since=r.worldMinutes+1;return true;}
    if(c.stage==1 && (f.social.romance<3 || f.social.cutOff || !f.presence.alive)) {c={};return true;}
    if(c.stage==2 && (!booked(r,f) || f.social.cutOff || !f.presence.alive)) {
        cancel(r,f.id);return true;
    }
    return false;
}
}
