// Synthetic callback host for adapter tests. Never distribute as CLEO.asi.
#include <windows.h>
namespace { DWORD version = 0x05040000; void *callbacks[16] = {}; unsigned count = 0; }
extern "C" DWORD WINAPI CLEO_GetVersion() { return version; }
extern "C" void WINAPI CLEO_RegisterCallback(DWORD id, void *func) {
    if (id < 16) { callbacks[id] = func; ++count; }
}
extern "C" void WINAPI TestVersion(DWORD value) { version = value; }
extern "C" unsigned WINAPI TestCount() { return count; }
extern "C" void WINAPI TestInvoke(DWORD id) {
    if (id < 16 && callbacks[id]) reinterpret_cast<void (WINAPI *)()>(callbacks[id])();
}
