#pragma once
#include "ControlActions.h"
#include <array>
#include <cstdint>
namespace controller {
// CControllerState field order from plugin-sdk's SA CPad.h (24 signed shorts, 0x30 bytes).
enum Field : unsigned { LX,LY,RX,RY,LB,LT,RB,RT,Up,Down,Left,Right,Start,Back,X,Y,A,B,LS,RS };
inline constexpr std::uint32_t bit(unsigned field){return 1u<<field;}
using State=std::array<short,24>;
static_assert(sizeof(State)==0x30,"SA controller-state layout");
inline std::uint32_t buttons(const State &state) {
    std::uint32_t result=0;
    for(unsigned i=LB;i<=RS;++i)if(state[i]>30)result|=bit(i);
    return result;
}
struct Result {
    std::array<bool,controls::Count> actions{};
    std::uint32_t consumed=0;
    bool modal=false;
};
class Policy {
    bool initialized=false,wasPhone=false;
    std::uint32_t quarantine=0,owned=0;
public:
    void reset(){initialized=wasPhone=false;quarantine=owned=0;}
    Result update(const State &state,bool connected,bool inHands,bool allowed,bool phone,bool canFocus) {
        Result result;
        if(!connected || !allowed){reset();return result;}
        const auto raw=buttons(state);
        if(!initialized){initialized=true;wasPhone=phone;quarantine=raw;}
        if(!inHands){owned=0;quarantine=raw;wasPhone=phone;return result;}
        if(phone!=wasPhone){quarantine|=raw;wasPhone=phone;}
        quarantine&=raw;owned&=raw;
        const auto input=raw&~quarantine;
        auto held=[&](unsigned field){return (input&bit(field))!=0;};
        auto &a=result.actions;
        if(phone) {
            result.modal=true;
            a[controls::PhoneUp]=held(Up);
            a[controls::PhoneDown]=held(Down);
            a[controls::PhoneLeft]=held(Left);
            a[controls::PhoneRight]=held(Right);
            a[controls::PhoneSelect]=held(A);a[controls::PhoneBack]=held(B);a[controls::PhoneClose]=held(Y);
            result.consumed=((1u<<20)-1)&~((1u<<4)-1)&~bit(Start);
        } else {
            const bool focus=canFocus && held(LB)&&held(RB);
            a[controls::Focus]=focus;
            a[controls::Greet]=focus&&held(A);a[controls::Provoke]=focus&&held(B);
            a[controls::Defuse]=focus&&held(X);a[controls::Rob]=focus&&held(Y);
            a[controls::Repair]=focus&&held(Right);
            a[controls::Phone]=!held(RB)&&held(LB)&&held(Up);
            a[controls::Vitals]=!held(RB)&&held(LB)&&held(Down);
            if(focus)result.consumed=bit(LB)|bit(RB)|bit(A)|bit(B)|bit(X)|bit(Y)|bit(Right);
            if(a[controls::Phone])result.consumed|=bit(LB)|bit(Up);
            if(a[controls::Vitals])result.consumed|=bit(LB)|bit(Down);
        }
        owned|=raw&result.consumed;
        result.consumed|=owned; // A button captured by the phone/focus stays captured until release.
        return result;
    }
};
inline void filter(State &state,const Result &result) {
    if(result.modal)for(unsigned i=0;i<4;++i)state[i]=0;
    for(unsigned i=LB;i<=RS;++i)if(result.consumed&bit(i))state[i]=0;
}
}
