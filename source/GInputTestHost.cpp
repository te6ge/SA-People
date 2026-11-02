#define WIN32_LEAN_AND_MEAN
#include "GInputBridge.h"
namespace {
int version=0x10b00;bool connected=true,inHands=true,ps=false;
class MockPad:public ginput::Pad {
public:
    bool IsPadConnected()const override{return connected;}
    bool HasPadInHands()const override{return inHands;}
    int GetVersion()const override{return version;}
    void *SendEvent(ginput::Event e,void *p)override{return SendConstEvent(e,p);}
    void *SendConstEvent(ginput::Event e,void *p)const override {
        if(e==ginput::FetchGeneralSettings && p) {
            auto &settings=*static_cast<ginput::GeneralSettings *>(p);
            if(settings.cbSize==sizeof(settings)){settings.PlayStationButtons=ps;return p;}
        }
        return reinterpret_cast<void *>(-1);
    }
} pad;
}
extern "C" __declspec(dllexport) ginput::Pad *GetGInputInterface(){return &pad;}
extern "C" __declspec(dllexport) void TestVersion(int v){version=v;}
extern "C" __declspec(dllexport) void TestState(bool c,bool hands,bool playStation){connected=c;inHands=hands;ps=playStation;}
