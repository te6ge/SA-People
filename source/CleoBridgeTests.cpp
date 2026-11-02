#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "CleoBridge.h"
#include "InstallPaths.h"
#include <cassert>
#include <iostream>
namespace {
unsigned starts = 0, frames = 0;
void WINAPI startup() { ++starts; }
void WINAPI process() { ++frames; }
}
int wmain(int argc,wchar_t **argv) {
    assert(!cleobridge::attach(nullptr, startup, process));
    assert(!cleobridge::attach(GetModuleHandleW(L"kernel32.dll"), startup, process));
    HMODULE host = LoadLibraryW(L"build\\CleoTestHost.dll");
    assert(host);
    assert(!GetProcAddress(host,"CLEO_GetVersion") && !GetProcAddress(host,"CLEO_RegisterCallback"));
    assert(GetProcAddress(host,"_CLEO_GetVersion@0") && GetProcAddress(host,"_CLEO_RegisterCallback@8"));
    auto version = reinterpret_cast<void (WINAPI *)(DWORD)>(GetProcAddress(host, "TestVersion"));
    auto count = reinterpret_cast<unsigned (WINAPI *)()>(GetProcAddress(host, "TestCount"));
    auto invoke = reinterpret_cast<void (WINAPI *)(DWORD)>(GetProcAddress(host, "TestInvoke"));
    assert(version && count && invoke);
    version(0x04040000);
    assert(!cleobridge::attach(host, startup, process) && count() == 0);
    version(0x06000000);
    assert(!cleobridge::attach(host, startup, process) && count() == 0);
    version(0x05040000);
    assert(cleobridge::attach(host, startup, process) && count() == 2);
    invoke(11); invoke(14); invoke(14);
    assert(starts == 1 && frames == 2);
    FreeLibrary(host);
    const std::filesystem::path game = L"D:\\GTA - San Andreas";
    auto paths = installpaths::resolve(game / L"gta_sa.exe", game / L"CLEO\\cleo_plugins\\SA-People.cleo");
    assert(paths.resources == game / L"SA-People" && paths.state == paths.resources);
    auto asi = installpaths::resolve(game / L"gta_sa.exe", game / L"SA-People.asi");
    assert(asi.state == paths.state && asi.resources == paths.resources);
    // A renamed ASI must not silently work as a CLEO plugin without the host.
    HMODULE plugin = LoadLibraryW(L"build\\SA-People.cleo");
    assert(!plugin);
    const auto hostDir = std::filesystem::absolute(L"build/cleo-test-host");
    std::filesystem::create_directories(hostDir);
    const auto hostPath = hostDir / L"CLEO.asi";
    std::filesystem::copy_file(L"build/CleoTestHost.dll", hostPath,
        std::filesystem::copy_options::overwrite_existing);
    host = LoadLibraryW(hostPath.c_str());
    assert(host);
    count = reinterpret_cast<unsigned (WINAPI *)()>(GetProcAddress(host, "TestCount"));
    invoke = reinterpret_cast<void (WINAPI *)(DWORD)>(GetProcAddress(host, "TestInvoke"));
    plugin = LoadLibraryW(L"build\\SA-People.cleo");
    assert(plugin && count() == 2);
    assert(!GetProcAddress(plugin, "InitializeASI"));
    // The real plugin must reject a non-GTA process without touching game addresses or saves.
    invoke(11); invoke(14); invoke(14);
    assert(count() == 2);
    FreeLibrary(plugin);
    FreeLibrary(host);
    assert(std::filesystem::remove(hostPath));
    if(argc>1) {
        // Inspect the real host's export table without running its DllMain outside GTA.
        auto actual=LoadLibraryExW(argv[1],nullptr,DONT_RESOLVE_DLL_REFERENCES);
        assert(actual);
        assert(cleobridge::api(actual,"_CLEO_GetVersion@0","CLEO_GetVersion"));
        assert(cleobridge::api(actual,"_CLEO_RegisterCallback@8","CLEO_RegisterCallback"));
        FreeLibrary(actual);
        std::cout<<"PASS: real CLEO library exposes both ABI entry points; no host code executed.\n";
    }
    std::cout << "PASS: CLEO callback ABI, old/missing/unknown host rejection, frame dispatch, shared ASI/CLEO save and resource paths.\n";
}
