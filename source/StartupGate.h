#pragma once
namespace startup {
// Frames may nest through another mod's callback chain. Enter startup only once,
// including when initialization refuses an executable or throws an exception.
struct Gate {
    bool attempted=false;
    bool begin() {
        if(attempted) return false;
        attempted=true;
        return true;
    }
};
}
