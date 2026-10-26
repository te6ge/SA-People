#pragma once
#include "Persona.h"
namespace introductions {
inline bool rival(int type) { return type>=7 && type<=16 && type!=8; }
inline bool comfortable(const people::Family &f) {
    return !f.social.cutOff && !f.social.robbed && !f.social.feud &&
        f.social.anger<20 && f.social.resentment<20;
}
inline bool name(const people::Family &f,int type) {
    const unsigned temperament=dialogue::temperament(f);
    return comfortable(f) && f.social.encounters >= (temperament==0 ? 1u:2u)+(rival(type)?1u:0u) &&
        f.social.trust >= 10+static_cast<int>(temperament)*4+(rival(type)?14:0);
}
inline bool job(const people::Family &f,int type) {
    return comfortable(f) && f.social.encounters >= 2u+(dialogue::temperament(f)>=2?1u:0u)+(rival(type)?1u:0u) &&
        f.social.trust >= (rival(type)?40:24);
}
inline bool contact(const people::Family &f) {
    return comfortable(f) && (f.social.known&people::Name) &&
        (f.social.helped || (f.social.encounters >= (dialogue::temperament(f)==0?2u:3u) &&
         f.social.meaningful>=5 && f.social.trust>=32));
}
inline bool personal(const people::Family &f) {
    return comfortable(f) && f.social.encounters>=3 && f.social.trust>=40;
}
inline bool gangRange(float squared,bool visible) {return visible && squared<=64.0f;}
} // namespace introductions
