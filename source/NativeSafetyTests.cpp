// Windows/x86 adapter checks against synthetic pools; never open or modify a game.
#include "Plugin.cpp"
#include <cassert>
#include <iostream>
namespace {
unsigned chainedCalls=0;
Process chainedPrevious=nullptr;
void __cdecl foreignCallback(){++chainedCalls;}
void __cdecl ourCallback(){++chainedCalls;chainedPrevious();}
}
int main() {
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
