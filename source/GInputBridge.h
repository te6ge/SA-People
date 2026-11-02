#pragma once
#include <windows.h>
#include <tlhelp32.h>
namespace ginput {
// Read-only ABI subset of Silent's GInput 1.11 SDK, as distributed with v-hud:
// https://github.com/gennariarmando/v-hud/blob/master/vendor/GInputAPI/GInputAPI.h
// The interface is obtained only on the game thread, never in DllMain.
enum Event : int { FetchGeneralSettings=6 };
struct GeneralSettings {
    DWORD cbSize=sizeof(GeneralSettings);
    bool DisableOnFocusLost:1,Vibration:1,CheatsFromPad:1,GuideLaunchesOverlay:1,
         ApplyMissionSpecificFixes:1,ApplyGXTFixes:1,PlayStationButtons:1,MapPadOneToPadTwo:1,FreeAim:1;
};
static_assert(sizeof(GeneralSettings)==8,"GInput SDK settings ABI");
class Pad {
protected:virtual ~Pad()=default;
public:
    virtual bool IsPadConnected()const=0;
    virtual bool HasPadInHands()const=0;
    virtual int GetVersion()const=0;
    virtual void *SendEvent(Event,void *)=0;
    virtual void *SendConstEvent(Event,void *)const=0;
};
using GetInterface=Pad *(__cdecl *)();
inline Pad *resolve(HMODULE module) {
    if(!module)return nullptr;
    auto get=reinterpret_cast<GetInterface>(GetProcAddress(module,"GetGInputInterface"));
    if(!get)get=reinterpret_cast<GetInterface>(GetProcAddress(module,MAKEINTRESOURCEA(1)));
    if(!get)return nullptr;
    auto pad=get();
    if(!pad || pad->GetVersion()<0x10000 || pad->GetVersion()>=0x20000)return nullptr;
    HMODULE retained=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(get),&retained))return nullptr;
    return pad;
}
inline HMODULE findModule() {
    if(auto module=GetModuleHandleW(L"GInputSA.asi"))return module;
    auto snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,GetCurrentProcessId());
    if(snapshot==INVALID_HANDLE_VALUE)return nullptr;
    MODULEENTRY32W entry{};entry.dwSize=sizeof(entry);HMODULE found=nullptr;
    if(Module32FirstW(snapshot,&entry))do {
        if(_wcsnicmp(entry.szModule,L"GInputSA",8)==0 &&
           (entry.szModule[8]==L'.' || entry.szModule[8]==0)){found=entry.hModule;break;}
    }while(Module32NextW(snapshot,&entry));
    CloseHandle(snapshot);return found;
}
}
