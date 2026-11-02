#define WIN32_LEAN_AND_MEAN
#include "GInputBridge.h"
#include <cassert>
#include <iostream>
int main() {
    assert(!ginput::resolve(nullptr));
    auto host=LoadLibraryW(L"build\\GInputTestHost.dll");assert(host);
    auto version=reinterpret_cast<void(__cdecl *)(int)>(GetProcAddress(host,"TestVersion"));
    auto state=reinterpret_cast<void(__cdecl *)(bool,bool,bool)>(GetProcAddress(host,"TestState"));
    assert(version && state);
    version(-1);assert(!ginput::resolve(host));
    version(0x20000);assert(!ginput::resolve(host));
    version(0x10b00);auto pad=ginput::resolve(host);assert(pad && pad->IsPadConnected() && pad->HasPadInHands());
    state(true,true,true);ginput::GeneralSettings settings{};
    assert(pad->SendConstEvent(ginput::FetchGeneralSettings,&settings)==&settings && settings.PlayStationButtons);
    state(false,false,false);assert(!pad->IsPadConnected() && !pad->HasPadInHands());
    std::cout<<"PASS: GInput exported interface ABI, version rejection, connection/input detection and PlayStation settings.\n";
}
