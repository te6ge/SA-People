#pragma once
#include "SocialWorld.h"
#include "Introductions.h"
namespace neighborhood {
inline bool personalPeace(const people::Family &f) {
    return (f.social.known & people::Name) && f.social.trust >= 15 &&
           introductions::comfortable(f);
}
// PED.DAT expresses gang rivalry against Grove (type 8), not necessarily PLAYER1.
constexpr unsigned PlayerAndGrove = (1u << 0) | (1u << 8);
inline bool hostile(unsigned hate, unsigned dislike) { return ((hate | dislike) & PlayerAndGrove) != 0; }
inline bool rival(int type) { return type >= 7 && type <= 16 && type != 8; }
struct TruceBits {
    unsigned dislike = 0, hate = 0;
    bool owned = false;
    void lift(unsigned &currentDislike, unsigned &currentHate) {
        if (!owned) {
            dislike=currentDislike & PlayerAndGrove; hate=currentHate & PlayerAndGrove;
            owned=true;
        }
        currentDislike &= ~PlayerAndGrove; currentHate &= ~PlayerAndGrove;
    }
    void restore(unsigned &currentDislike, unsigned &currentHate) {
        if (!owned) return;
        currentDislike |= dislike; currentHate |= hate;
        *this={};
    }
};
inline bool canRecognize(const people::Family &f, bool mission, bool native, bool owned,
                         unsigned area, float distanceSquared) {
    return !mission && !native && !owned && f.social.journal &&
        (f.social.known & people::Name) && socialworld::available(f) && f.social.trust >= 15 &&
        f.presence.area == area && distanceSquared >= 16 && distanceSquared <= 225;
}
inline int plannedActivity(const std::string &body) {
    if (body == "MAIL_PLAN_MEAL") return 0;
    if (body == "MAIL_PLAN_POOL") return 1;
    if (body == "MAIL_PLAN_DRIVE") return 2;
    if (body == "MAIL_PLAN_WALK") return 3;
    return -1;
}
inline const char *plan(unsigned activity) {
    static const char *keys[] = {"MAIL_PLAN_MEAL", "MAIL_PLAN_POOL", "MAIL_PLAN_DRIVE", "MAIL_PLAN_WALK"};
    return keys[activity % 4];
}
inline const char *interiorLine(unsigned variant) {
    static const char *keys[] = {"INSIDE_HELLO_0", "INSIDE_HELLO_1", "INSIDE_HELLO_2", "INSIDE_HELLO_3"};
    return keys[variant % 4];
}
} // namespace neighborhood
