#pragma once
// Minimal CLEO 5 ABI, documented in cleolibrary/CLEO5 v5.4.0 cleo_sdk/CLEO.h.
// Resolve the host API rather than redistributing or replacing CLEO.asi.
#include <windows.h>
namespace cleobridge {
using Callback = void (WINAPI *)();
using Version = DWORD (WINAPI *)();
using Register = void (WINAPI *)(DWORD, void *);
inline FARPROC api(HMODULE host,const char *decorated,const char *plain) {
    auto proc=GetProcAddress(host,decorated);
    return proc?proc:GetProcAddress(host,plain);
}
inline bool attach(HMODULE host, Callback startup, Callback process) {
    if (!host || !startup || !process) return false;
    auto version = reinterpret_cast<Version>(api(host,"_CLEO_GetVersion@0","CLEO_GetVersion"));
    auto add = reinterpret_cast<Register>(api(host,"_CLEO_RegisterCallback@8","CLEO_RegisterCallback"));
    if (!version || !add || version() < 0x05000000 || version() >= 0x06000000) return false;
    add(11, reinterpret_cast<void *>(startup)); // DrawingFinished: frontend and game.
    add(14, reinterpret_cast<void *>(process)); // GameProcessAfter: one update per game frame.
    return true;
}
}
