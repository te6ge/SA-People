#pragma once
#include "Network.h"
#include "SocialProgress.h"
namespace community {
enum class Deed : unsigned { Help=1, Robbery, Violence, Killing };
enum class Response { None, Thanks, Warn, Flee, Report, Defend };
inline int impact(Deed deed) { return deed==Deed::Help ? 5 : deed==Deed::Robbery ? -10 : deed==Deed::Violence ? -7 : -18; }
inline unsigned tie(const people::Registry &r,unsigned a,unsigned b) {
    for (const auto &e:r.connections) if ((e.a==a&&e.b==b)||(e.a==b&&e.b==a)) return e.kind+1;
    return 0;
}
inline unsigned incident(people::Registry &r) {
    if (r.nextIncident>=1000000000) return 0;
    return r.nextIncident++;
}
inline bool remember(people::Registry &r,unsigned witness,unsigned victim,Deed deed,unsigned token,
                     unsigned teller=0,unsigned hops=0) {
    if (!witness || !victim || witness>r.families.size() || victim>r.families.size() ||
        !token || token>=r.nextIncident || hops>2 || teller>r.families.size() ||
        static_cast<unsigned>(deed)<1 || static_cast<unsigned>(deed)>4) return false;
    auto &f=r.families[witness-1]; auto &m=f.community;
    if (!f.presence.alive || token<=m.incident) return false;
    int delta=impact(deed)/static_cast<int>(hops+1);
    const auto relation=tie(r,witness,victim);
    if (relation==3) delta=-delta/2;
    m.opinion=std::clamp(m.opinion+delta,-100,100);
    m.incident=token; m.victim=victim; m.teller=teller?teller:witness;
    m.kind=static_cast<unsigned>(deed); m.hops=hops; m.minute=static_cast<unsigned>(r.worldMinutes);
    m.sharedAt=0;
    if (f.social.journal) {
        f.social.trust=std::clamp(f.social.trust+delta/2,-100,100);
        if (delta<0) f.social.resentment=std::min(100u,f.social.resentment+static_cast<unsigned>(-delta/2));
    }
    if(deed==Deed::Killing && relation>=2 && relation!=3) {
        f.social.feud=1;f.social.cutOff=1;f.social.trust=-100;f.social.anger=100;f.social.resentment=100;
    }
    return true;
}
inline bool gossip(people::Registry &r,unsigned from,unsigned to) {
    if (!from || !to || from>r.families.size() || to>r.families.size() || from==to ||
        !tie(r,from,to) || tie(r,from,to)==3) return false;
    auto &m=r.families[from-1].community;
    if (!m.incident || m.hops>=2 || r.worldMinutes<m.minute || r.worldMinutes-m.minute>2880 ||
        (m.sharedAt && r.worldMinutes+1<m.sharedAt+60)) return false;
    const auto memory=m;
    if (!remember(r,to,memory.victim,static_cast<Deed>(memory.kind),memory.incident,from,memory.hops+1)) return false;
    m.sharedAt=static_cast<unsigned>(r.worldMinutes+1);
    if(r.families[to-1].social.journal)
        r.post(to,memory.kind==static_cast<unsigned>(Deed::Help)?"COMMUNITY_MAIL_GOOD":"COMMUNITY_MAIL_BAD",false,1,from);
    return true;
}
inline Response response(const people::Registry &r,unsigned id,unsigned victim,Deed deed,bool weapon) {
    if (!id || id>r.families.size()) return Response::None;
    const auto &f=r.families[id-1];
    if (deed==Deed::Help) return Response::Thanks;
    const unsigned courage=people::mix(id+317)%100;
    if (f.presence.type==6) return Response::Report;
    const auto relation=tie(r,id,victim);
    if (!weapon && relation && relation!=3 && f.social.trust<30 && courage>=70 && deed!=Deed::Killing)
        return Response::Defend;
    if (courage<45 || weapon) return Response::Flee;
    return courage<75 ? Response::Warn : Response::Report;
}
inline const char *request(people::Registry &r,unsigned id) {
    if (!id || id>r.families.size()) return "FAVOR_NO";
    auto &f=r.families[id-1]; auto &m=f.community;
    const auto now=static_cast<unsigned>(r.worldMinutes+1);
    const unsigned elapsed=m.lastRequest && now>=m.lastRequest ? now-m.lastRequest : 1440;
    if (elapsed>=60) {
        m.requests=0;
        const unsigned recovery=std::min(100u,elapsed/30);
        m.strain=m.strain>recovery ? m.strain-recovery : 0;
    }
    m.requests=std::min(12u,m.requests+1); m.lastRequest=now;
    if (m.requests>=3) {
        m.strain=std::min(100u,m.strain+10);
        f.social.trust=std::max(-100,f.social.trust-2);
        return "BOUND_TOO_MUCH";
    }
    if (m.strain>=60) return "BOUND_SPACE";
    if (f.daily.busyUntil>r.worldMinutes) return "BOUND_BUSY";
    const auto hour=static_cast<unsigned>(r.worldMinutes/60);
    if (hour%24>=8 && hour%24<17 && people::mix(id+hour/24)%5==0) return "BOUND_WORK";
    return nullptr;
}
inline bool unsafeRide(people::Family &f,float damage,bool collision,unsigned elapsed) {
    if (elapsed<4000 || (!collision && damage<30)) return false;
    auto &m=f.community;
    m.drivingWarnings=std::min(3u,m.drivingWarnings+1);
    m.strain=std::min(100u,m.strain+12);
    f.social.trust=std::max(-100,f.social.trust-3);
    return true;
}
inline bool mayAid(const people::Registry &r,unsigned id) {
    return id && id<=r.families.size() && r.families[id-1].presence.alive &&
        r.families[id-1].community.aidDay!=r.worldMinutes/1440+1;
}
inline bool aid(people::Registry &r,unsigned id,bool causedByCJ) {
    if (causedByCJ || !mayAid(r,id)) return false;
    auto &f=r.families[id-1];
    f.community.aidDay=static_cast<unsigned>(r.worldMinutes/1440+1);
    f.social.helped=std::min(1000000u,f.social.helped+1);
    f.social.trust=std::min(100,f.social.trust+10);
    f.social.meaningful=std::min(1000000u,f.social.meaningful+2);
    return true;
}
struct Progress {
    float best=1e12f;
    std::uint32_t changed=0;
    unsigned retries=0;
    bool stuck(float distance,std::uint32_t now) {
        if (!changed || distance+1<best) {best=distance;changed=now;return false;}
        return distance>16 && now-changed>=12000;
    }
    void retried(std::uint32_t now) {changed=now;best=1e12f;++retries;}
};
} // namespace community
