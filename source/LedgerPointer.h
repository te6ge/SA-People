#pragma once
#include "Grove.h"
namespace ledgerpointer {
// Logical notebook coordinates shared by the renderer and mouse hit tests.
inline int hit(unsigned page,float x,float y) {
    if(x>=872 && x<=910 && y>=28 && y<=65)return -2;
    if(x>=500 && x<=885 && y>=495 && y<=545)return -3;
    if(x<530 || x>885)return -1;
    const unsigned count=page==0?4u:page==1?3u:page==4?2u:grove::Count;
    for(unsigned n=0;n<count;++n) {
        const float top=page==4?365+n*46.0f:page>=2?137+n*76.0f:151+n*65.0f;
        const float height=page>=2 && page!=4?70.0f:page==4?42.0f:55.0f;
        if(y>=top-4 && y<=top+height)return static_cast<int>(n);
    }
    return -1;
}
struct State {
    float x=700,y=300;
    bool leftHeld=false,rightHeld=false;
    std::array<bool,4> keys{};
    unsigned keyboard(const std::array<bool,4> &down) {
        unsigned edges=0;
        for(unsigned n=0;n<4;++n){if(down[n]&&!keys[n])edges|=1u<<n;keys[n]=down[n];}
        return edges;
    }
    void open(bool left,bool right){x=700;y=300;leftHeld=left;rightHeld=right;keys.fill(true);}
    int update(unsigned page,float dx,float dy,bool left,bool right,bool active,bool notice) {
        if(!active){leftHeld=left;rightHeld=right;return -1;}
        if(std::isfinite(dx) && std::abs(dx)<3000)x=std::clamp(x+dx,20.0f,912.0f);
        if(std::isfinite(dy) && std::abs(dy)<3000)y=std::clamp(y+dy,20.0f,545.0f);
        const bool click=left&&!leftHeld,back=right&&!rightHeld;
        leftHeld=left;rightHeld=right;
        if(back)return -3;
        if(!click)return -1;
        const int target=hit(page,x,y);
        if(target==-2 || target==-3)return target;
        return notice?-4:target;
    }
};
}
