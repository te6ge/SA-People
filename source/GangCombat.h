#pragma once
namespace gangcombat {
inline bool eligible(int type, int createdBy, bool recruited, bool mission,
                     bool sameArea, bool alive, float distanceSquared) {
    return type>=7 && type<=16 && type!=8 && createdBy==1 && !recruited && !mission &&
        sameArea && alive && distanceSquared<=625;
}
// Empty or known ambient gang/standing/chat tasks may yield to the rejected standoff.
// Do not overwrite scripted sequences, unfamiliar tasks, injuries or existing combat.
inline bool ambientTask(int type) {
    switch(type) {
    case -1: case 203: case 257: case 274: case 912: case 1201: case 1203:
    case 1204: case 1207: case 1209: case 1210: case 1212: case 1213: case 1218:
        return true;
    default: return false;
    }
}
inline bool combatTask(int type) {return type>=1000 && type<=1034;}
} // namespace gangcombat
