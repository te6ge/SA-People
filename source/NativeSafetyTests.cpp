// Windows/x86 adapter checks against synthetic pools; never open or modify a game.
#include "Plugin.cpp"
#include "SetupOptions.h"
#include <cassert>
#include <iostream>
namespace {
unsigned chainedCalls=0;
Process chainedPrevious=nullptr;
void __cdecl foreignCallback(){++chainedCalls;}
void __cdecl ourCallback(){++chainedCalls;chainedPrevious();}
}
int main() {
    {
        wchar_t temp[MAX_PATH] = {}, ini[MAX_PATH] = {};
        assert(GetTempPathW(MAX_PATH, temp));
        assert(GetTempFileNameW(temp, L"sap", 0, ini));
        assert(WritePrivateProfileStringW(L"Controls", L"Phone", L"F2,P", ini));
        assert(WritePrivateProfileStringW(L"Controls", L"Greet", L"K,NONE", ini));
        assert(WritePrivateProfileStringW(L"Interface", L"Language", L"ru", ini));
        controls::load(ini);
        assert(controls::keys[controls::Phone].primary == VK_F2);
        assert(controls::keys[controls::Phone].secondary == 'P');
        assert(controls::keys[controls::Greet].primary == 'K');
        assert(controls::seedMissing(ini));
        controls::load(ini);
        assert(controls::keys[controls::Phone].primary == VK_F2);
        assert(controls::keys[controls::Greet].primary == 'K');
        wchar_t value[64] = {};
        GetPrivateProfileStringW(L"Interface", L"Language", L"", value, 64, ini);
        assert(std::wstring(value) == L"ru");
        for (unsigned a = 0; a < controls::Count; ++a) {
            const auto name = controls::iniName(a);
            const std::wstring wide(name, name + std::strlen(name));
            assert(GetPrivateProfileStringW(L"Controls", wide.c_str(), L"", value, 64, ini));
        }
        assert(WritePrivateProfileStringW(L"Controls", L"Phone", L"F2,NONE", ini));
        assert(controls::seedMissing(ini));
        controls::load(ini);
        assert(controls::keys[controls::Phone].primary == VK_F2 && !controls::keys[controls::Phone].secondary);
        controls::save(ini); controls::load(ini);
        assert(controls::keys[controls::Phone].primary == VK_F2 && !controls::keys[controls::Phone].secondary);
        assert(DeleteFileW(ini));
        controls::reset();
        std::cout << "PASS: INI phone rebinding, missing control migration, custom settings preservation and disabled secondary key roundtrip.\n";
    }

    {
        std::vector<Byte> ped(0x7C4, 0), savedPed;
        ped[0x473] = 0xA4;
        const unsigned masks[] = {1u, 1u << 8, 1u << 6, (1u << 6) | (1u << 7)};
        for (unsigned i = 0; i < 4; ++i)
            std::memcpy(ped.data() + outingsafety::Disposition::offsets[i], &masks[i], 4);
        savedPed = ped;
        outingsafety::Disposition guard;
        guard.apply(ped.data());
        assert(ped[0x473] == 0xA5);
        for (unsigned i = 0; i < 4; ++i) {
            unsigned value = 99;
            std::memcpy(&value, ped.data() + outingsafety::Disposition::offsets[i], 4);
            assert(value == 0);
        }
        guard.apply(ped.data()); // Refresh must not replace the original with zeroes.
        guard.restore(ped.data());
        assert(ped == savedPed && !guard.active);
        ped[0x473] |= 1; savedPed = ped;
        guard.apply(ped.data()); guard.restore(ped.data());
        assert(ped == savedPed); // A pre-existing don't-fight flag belongs to its owner.
        guard.apply(ped.data());
        ped[0x473] |= 0x10;
        const unsigned external = 1u << 14;
        std::memcpy(ped.data() + 0x4EC, &external, 4);
        guard.restore(ped.data());
        unsigned restoredMask = 0;
        std::memcpy(&restoredMask, ped.data() + 0x4EC, 4);
        assert(restoredMask == (masks[2] | external) && (ped[0x473] & 0x10));
        assert(outingsafety::offensiveTask(1000) && outingsafety::offensiveTask(1005));
        for (int task : {-1, 203, 1006, 1007, 1008, 1100, 1102, 244})
            assert(!outingsafety::offensiveTask(task));
        std::cout << "PASS: outing-only hostility suppression, idempotent leases, complete restoration, unrelated flag preservation and injury/arrest task exclusions.\n";
    }
    {
        const auto probeRoot = std::filesystem::temp_directory_path() /
            ("sa-people-audio-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(GetTickCount()));
        assert(std::filesystem::create_directory(probeRoot));
        const auto wavPath = probeRoot / "line.wav", mp3Path = probeRoot / "line.mp3";
        { std::ofstream sample(wavPath); sample << "legacy"; }
        assert(audiofiles::resolve(wavPath) == wavPath);
        { std::ofstream sample(mp3Path); sample << "compact"; }
        assert(audiofiles::resolve(wavPath) == mp3Path);
        assert(audiofiles::resolve(mp3Path) == mp3Path);
        std::filesystem::remove(wavPath);
        assert(audiofiles::resolve(wavPath) == mp3Path);
        std::filesystem::remove(mp3Path); std::filesystem::remove(probeRoot);
        std::cout << "PASS: compressed speech preference, existing WAV-only installations and direct MP3 paths.\n";
    }
    {
        wchar_t menuTemp[MAX_PATH] = {}, menuIni[MAX_PATH] = {}, value[32] = {};
        assert(GetTempPathW(MAX_PATH,menuTemp));
        assert(GetTempFileNameW(menuTemp,L"sap",0,menuIni));
        assert(!setupoptions::newMenu(menuIni));
        assert(WritePrivateProfileStringW(L"Interface",L"Language",L"es",menuIni));
        assert(WritePrivateProfileStringW(L"Controls",L"Phone",L"F2,P",menuIni));
        assert(setupoptions::apply(menuIni,setupoptions::Menu::New));
        assert(setupoptions::newMenu(menuIni));
        assert(setupoptions::apply(menuIni,setupoptions::Menu::Keep) && setupoptions::newMenu(menuIni));
        assert(setupoptions::apply(menuIni,setupoptions::Menu::Original));
        assert(!setupoptions::newMenu(menuIni));
        GetPrivateProfileStringW(L"Interface",L"Language",L"",value,32,menuIni);
        assert(std::wstring(value)==L"es");
        GetPrivateProfileStringW(L"Controls",L"Phone",L"",value,32,menuIni);
        assert(std::wstring(value)==L"F2,P");
        assert(DeleteFileW(menuIni));
        std::cout << "PASS: menu opt-in defaults off, explicit enable/disable and preserving language/key bindings.\n";
    }
    auto page=static_cast<Byte *>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    assert(page);
    page[0]=0xE8;page[5]=0xC3;
    *reinterpret_cast<int *>(page+1)=static_cast<int>(reinterpret_cast<std::uintptr_t>(&foreignCallback)-reinterpret_cast<std::uintptr_t>(page+5));
    assert(patchCall(reinterpret_cast<std::uintptr_t>(page),ourCallback,chainedPrevious));
    assert(chainedPrevious==foreignCallback);
    reinterpret_cast<Process>(page)();assert(chainedCalls==2);
    page[0]=0xE9;Process untouched=nullptr;
    assert(!patchCall(reinterpret_cast<std::uintptr_t>(page),ourCallback,untouched)&&!untouched);
    VirtualFree(page,0,MEM_RELEASE);
    std::vector<Byte> actors(0x7C4*4),vehicles(0xA18*2);
    Byte actorFlags[4]={1,2,3,4},carFlags[2]={1,2};
    Pool peoplePool{actors.data(),actorFlags,4},vehiclePool{vehicles.data(),carFlags,2};
    for(unsigned i=0;i<4;++i)*reinterpret_cast<short *>(actors.data()+i*0x7C4+0x22)=50;
    auto car=vehicles.data();car[0x488]=3;*reinterpret_cast<short *>(car+0x22)=445;
    const auto first=handle(&peoplePool,0),second=handle(&peoplePool,1),third=handle(&peoplePool,2);
    auto seat=[&](people::Handle who){return reservePassengerInPools(&vehiclePool,&peoplePool,car,who);};
    assert(seat(first)==0);assert(seat(first)==0);
    assert(seat(second)==1);assert(seat(third)==2);
    assert(seat(handle(&peoplePool,3))==-1);
    passengerReservations.clear();
    *reinterpret_cast<Byte **>(car+0x464)=actors.data();
    assert(seat(second)==1);
    car[0x485]=1;assert(seat(third)==-1);car[0x485]=0;
    ++actorFlags[1];
    assert(seat(third)==1); // Recycled ped cannot keep its predecessor's seat.
    passengerReservations.clear();car[0x488]=8;
    for(unsigned slot=0;slot<3;++slot)*reinterpret_cast<Byte **>(car+0x464+slot*4)=actors.data();
    assert(seat(first)==-1); // Never invent a fourth automobile door.
    actorFlags[0]=0x80;assert(seat(first)==-1);
    passengerReservations.clear();
    std::cout<<"PASS: native call chaining preserves another plugin, unfamiliar hooks are refused, "
               "passenger reservations exclude driver/occupied/recycled slots and unsupported doors.\n";
}
