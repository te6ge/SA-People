// GTA SA 1.0 US adapter. Plugin-SDK reference: 15f15b60bbf74c106e1b496ff92c98764abf4605.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <unordered_set>
#include <cstring>
#include <ctime>
#include "Network.h"
#include "World.h"
#include "ModelTraits.h"
#include "Panel.h"
#include "Street.h"
#include "Audio.h"
#include "AudioPolicy.h"
#include "Encounters.h"
#include "Locale.h"
#include "Glyphs.h"
#include "Sprites.h"
#include "Backdrop.h"
#include "Phone.h"
#include "ModernMenu.h"
#include "Life.h"
#include "SocialWorld.h"
#include "Romance.h"
#include "Vitals.h"
#include "Bonds.h"
#include "Later.h"
#include "Favors.h"
#include "MeetingRuntime.h"
#include "SocialContext.h"
#include "SocialProgress.h"
#include "NativeTalk.h"
#include "InstallPaths.h"
#include "Neighborhood.h"
#include "GangCombat.h"
#include "Community.h"
#include "OutingSafety.h"
namespace {
using Byte = unsigned char;
template <class T> T &at(std::uintptr_t a) {
    return *reinterpret_cast<T *>(a);
}
struct Vec {
    float x, y, z;
};
struct Pool {
    Byte *objects;
    Byte *flags;
    int size;
};
bool validPool(Pool *);
using Process = void(__cdecl *)();
Process original = nullptr, originalRestart = nullptr, originalDraw = nullptr;
HMODULE moduleHandle = nullptr;
std::filesystem::path directory, resourceDirectory, settingsFile;
HANDLE instanceGuard = nullptr;
socialaudio::Player audio;
unsigned audioVariation = 0;
socialaudio::Chooser audioChoices;
npcvoice::Chooser npcVoices;
glyphs::Renderer glyphRenderer;
sprites::Image meetingImage, heartImage, interestImage;
backdrop::Gallery gallery;
std::unordered_map<unsigned, DWORD> returnAfter;
people::Registry registry;
people::ModelTraits modelTraits;
people::Bindings bindings;
people::SpatialIndex spatial;
std::unordered_set<unsigned> restored;
unsigned selected = 0, currentPage = 0, lastGameTime = 0, restoreCursor = 0;
DWORD nextScan = 0, nextSave = 0, nextRestore = 0, nextHeartbeat = 0;
bool disabled = false, dirty = false, gameplaySeen = false, saveErrorReported = false,
     autoRestore = true;
bool storySuspended = false;
bool vitalsInstalled = false;
unsigned expiredMeetings = 0;
bool recovered = false;
// CGangWars::State 0x96AB64 (waves of a war CJ started) and State2 0x96AB4C (a rival gang
// attacking CJ's territory); plugin-sdk 1.0 US addresses. The mod stands down like on a mission.
bool gangWarActive() {
    return at<int>(0x96AB64) != 0 || at<int>(0x96AB4C) != 0;
}
bool storyActive() {
    return at<bool>(0xB5F851) || reinterpret_cast<bool(__cdecl *)()>(0x464D50)() ||
           gangWarActive();
}
bool heldPage = false, heldArchive = false, heldReunion = false, heldSave = false;
bool heldPanel = false, panelOpen = false, panelInvalid = true;
bool developerReunion = false;
DWORD panelTouched = 0, sceneRefresh = 0, toastUntil = 0, toastStart = 0;
unsigned panelAutoClose = 0;
float panelScale = 1;
panel::Scene panelScene;
struct Pending {
    unsigned id = 0;
    Vec pos{};
    DWORD started = 0;
    bool manual = false;
    bool appointment = false;
} pending;
std::string message;
void log(const std::string &text) {
    std::ofstream out(directory / L"SA-People.log", std::ios::app);
    out << GetTickCount64() << " " << text << '\n';
}
void help(const std::string &text) {
    // Independent short toast; never use CHud's crowded help-message box.
    message = text;
    if (message.rfind("SA People: ", 0) == 0)
        message.erase(0, 11);
    toastStart = GetTickCount();
    toastUntil =
        toastStart + std::clamp<DWORD>(2600 + static_cast<DWORD>(message.size()) * 40, 3000, 7500);
}
unsigned playerDistrict();
void reputation(int cue) {
    if (!cue)
        return;
    // Good deeds weigh more than single slips, so honor can be earned back (help +4, provoke -2).
    const int before = registry.honor;
    network::honor(registry, cue > 0 ? cue * 4 : cue * 2);
    if (before == registry.honor) return;
    audio.reputation(cue > 0 ? 1 : -1);
    dirty = true;
}
void discoverNetwork(unsigned id) {
    auto count = registry.families.size();
    network::discover(registry, id);
    for (auto i = count; i < registry.families.size(); ++i)
        spatial.update(registry.families[i]);
    dirty = true;
}
bool drawMeetingImage(float x, float y, float w, float h, Byte alpha = 255) {
    return meetingImage.draw(at<IDirect3DDevice9 *>(0xC97C28),
                             resourceDirectory / L"assets" / L"meeting.png", x, y, w, h, alpha);
}
void openPanel() {
    panelOpen = true;
    panelInvalid = true;
    panelTouched = GetTickCount();
}
bool due(DWORD now, DWORD deadline) {
    return deadline == 0 || now - deadline < 0x80000000u;
}
bool pressAction(unsigned action, bool &held) {
    bool down = controls::down(action);
    bool result = down && !held;
    held = down;
    return result;
}
bool press(int key, bool &held) {
    bool down = (GetAsyncKeyState(key) & 0x8000) != 0;
    bool result = down && !held;
    held = down;
    return result;
}
Vec position(Byte *e);
unsigned playerDistrict() {
    auto player = reinterpret_cast<Byte *(__cdecl *)(int)>(0x56E210)(0);
    if (!player)
        return 0;
    Vec p = position(player);
    return bonds::district(p.x, p.y);
}
Vec position(Byte *e) {
    auto matrix = *reinterpret_cast<Byte **>(e + 0x14);
    return *reinterpret_cast<Vec *>(matrix ? matrix + 0x30 : e + 4);
}
float distance2(Vec a, Vec b) {
    return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z);
}
Byte *pedAt(Pool *p, unsigned slot) {
    return p->objects + slot * 0x7C4;
}
people::Handle handle(Pool *p, unsigned slot) {
    return {slot, p->flags[slot], *reinterpret_cast<short *>(pedAt(p, slot) + 0x22)};
}
bool exists(Pool *p, people::Handle h) {
    return h.slot < static_cast<unsigned>(p->size) && !(p->flags[h.slot] & 0x80) &&
           handle(p, h.slot) == h;
}
struct ActorLease { people::Handle handle; unsigned person; Byte createdBy; outingsafety::Disposition outing; };
std::unordered_map<unsigned, ActorLease> actorLeases;
bool leasedActor(Byte *ped) {
    auto pool = at<Pool *>(0xB74490);
    if (!pool || !pool->objects || ped < pool->objects) return false;
    const auto delta = ped - pool->objects;
    if (delta % 0x7C4 || delta / 0x7C4 >= pool->size) return false;
    const unsigned slot = static_cast<unsigned>(delta / 0x7C4);
    auto it = actorLeases.find(slot);
    return it != actorLeases.end() && exists(pool, it->second.handle);
}
void retainActor(people::Handle h, unsigned id) {
    auto pool = at<Pool *>(0xB74490);
    if (!exists(pool, h)) return;
    auto ped = pedAt(pool, h.slot);
    if (leasedActor(ped) || ped[0x484] != 1) return;
    actorLeases[h.slot] = {h, id, ped[0x484], {}};
    // Native setter maintains population accounting and installs the mission decision maker.
    reinterpret_cast<void(__thiscall *)(Byte *, int)>(0x5E47E0)(ped, 2);
}
void releaseActor(unsigned id) {
    auto pool = at<Pool *>(0xB74490);
    for (auto it = actorLeases.begin(); it != actorLeases.end();) {
        if (it->second.person != id) { ++it; continue; }
        if (pool && exists(pool, it->second.handle)) {
            auto ped = pedAt(pool, it->first);
            it->second.outing.restore(ped);
            if (ped[0x484] == 2)
                reinterpret_cast<void(__thiscall *)(Byte *, int)>(0x5E47E0)(ped, it->second.createdBy);
        }
        it = actorLeases.erase(it);
    }
}
void releaseActors() {
    while (!actorLeases.empty()) releaseActor(actorLeases.begin()->second.person);
}
void companionWitness(unsigned, socialcontext::Witness);
void observeDeed(unsigned,community::Deed);
bool activeOffender(unsigned);
socialprogress::HonorBatch honorBatch;
socialprogress::Cheat honorCheat;
bool cheatHeld[26]{};
std::unordered_set<unsigned> defendedOffenders;
std::unordered_map<unsigned, DWORD> offenderUntil;
float observedPlayerHealth = -1;
bool recruited(Byte *ped) {
    return *reinterpret_cast<int *>(ped + 0x598) >= 4 &&
           reinterpret_cast<char(__cdecl *)(Byte *)>(0x5F7F10)(ped) != 0;
}
bool femaleModel(int model) {
    return modelTraits.female(model, reinterpret_cast<bool(__cdecl *)(int)>(0x611490)(model));
}
bool eligible(Byte *ped, Byte *player) {
    int type = *reinterpret_cast<int *>(ped + 0x598),
        model = *reinterpret_cast<short *>(ped + 0x22);
    return ped != player && !recruited(ped) && (ped[0x484] == 1 || leasedActor(ped)) && type >= 4 && type <= 23 &&
           model >= 0 && model <= 311;
}
void capture(people::Family &f, Byte *ped) {
    Vec v = position(ped);
    people::Presence p;
    p.model = *reinterpret_cast<short *>(ped + 0x22);
    p.type = *reinterpret_cast<int *>(ped + 0x598);
    p.x = v.x;
    p.y = v.y;
    p.z = v.z;
    p.heading = *reinterpret_cast<float *>(ped + 0x558);
    p.health = std::clamp(*reinterpret_cast<float *>(ped + 0x540), 0.0f, 10000.0f);
    p.area = ped[0x2F];
    p.located = true;
    p.alive = f.presence.alive && p.health > 0;
    if (!people::valid(p))
        return;
    f.female = femaleModel(p.model);
    bool died = f.presence.alive && !p.alive;
    const bool hurt = p.health + 0.5f < f.presence.health;
    f.presence = p;
    bool defense = activeOffender(f.id);
    if (hurt && !storyActive()) {
        auto attacker = *reinterpret_cast<Byte **>(ped + 0x764);
        auto cj = reinterpret_cast<Byte *(__cdecl *)(int)>(0x56E210)(0);
        auto vehicle = reinterpret_cast<Byte *(__cdecl *)(int, bool)>(0x56E0D0)(0, false);
        const bool byCJ = attacker && (attacker == cj || attacker == vehicle);
        auto vehicles=at<Pool *>(0xB74494);
        if(attacker && validPool(vehicles)) {
            const auto address=reinterpret_cast<std::uintptr_t>(attacker), base=reinterpret_cast<std::uintptr_t>(vehicles->objects);
            if(address>=base && (address-base)%0xA18==0 && (address-base)/0xA18<static_cast<unsigned>(vehicles->size) &&
               !(vehicles->flags[(address-base)/0xA18]&0x80))
                f.community.accidentAt=static_cast<unsigned>(registry.worldMinutes+1);
        }
        if (socialprogress::defended(defense, byCJ, p.health) &&
            defendedOffenders.insert(f.id).second) {
            reputation(1);
            companionWitness(f.id, socialcontext::Witness::Help);
            log("HONOR stopped active offender identity=" + std::to_string(f.id));
        } else if (byCJ && !defense)
            companionWitness(f.id, died ? socialcontext::Witness::Killing : socialcontext::Witness::Violence);
    }
    if (died) {
        auto attacker = *reinterpret_cast<Byte **>(ped + 0x764);
        auto player = reinterpret_cast<Byte *(__cdecl *)(int)>(0x56E210)(0);
        auto car = reinterpret_cast<Byte *(__cdecl *)(int, bool)>(0x56E0D0)(0, false);
        if (attacker && (attacker == player || attacker == car)) {
            if (defense) { spatial.update(f); dirty = true; return; }
            // Relatives learn through witnesses and local contacts, rather than globally.
            reputation(-4);
        }
    }
    spatial.update(f);
    dirty = true;
}
bool save() {
    try {
        auto tmp = directory / L"people.dat.tmp", target = directory / L"people.dat";
        {
            std::ofstream out(tmp, std::ios::trunc);
            registry.savedAt = static_cast<std::uint64_t>(std::time(nullptr));
            registry.write(out);
            out.flush();
            if (!out)
                throw std::runtime_error("Archive flush failed");
        }
        if (std::filesystem::exists(target) &&
            !CopyFileW(target.c_str(), (directory / L"people.dat.bak").c_str(), FALSE))
            throw std::runtime_error("Archive backup failed");
        if (!MoveFileExW(tmp.c_str(), target.c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Archive replacement failed");
        dirty = false;
        saveErrorReported = false;
        return true;
    } catch (const std::exception &e) {
        if (!saveErrorReported)
            log(e.what());
        saveErrorReported = true;
        return false;
    }
}
void initialize() {
    wchar_t path[32768] = {};
    // Resolve actual module/executable locations, independent of ModLoader's IAT path translation.
    auto modulePath = reinterpret_cast<DWORD(WINAPI *)(HMODULE, LPWSTR, DWORD)>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetModuleFileNameW"));
    wchar_t executable[32768] = {};
    if (!modulePath || !modulePath(moduleHandle, path, 32768) ||
        !modulePath(nullptr, executable, 32768))
        throw std::runtime_error("Module path unavailable");
    auto paths = installpaths::resolve(executable, path);
    directory = paths.state;
    resourceDirectory = paths.resources;
    std::filesystem::create_directories(directory);
    installpaths::seedSettings(paths);
    log("PATHS resources=" + resourceDirectory.u8string() + " state=" + directory.u8string());
    {
        std::ifstream peds(paths.game / L"data" / L"peds.ide");
        modelTraits.read(peds);
    }
    settingsFile = installpaths::settings(paths);
    auto ini = settingsFile;
    log("SETTINGS active=" + settingsFile.u8string());
    if (!std::filesystem::exists(ini)) {
        std::ofstream out(ini);
        out << "[World]\nAutoRestore=1\n; Set 0 to disable natural returns. F9 still allows a "
               "manual reunion.\n\n[Interface]\nScalePercent=100\nAutoCloseSeconds=0\n";
    }
    autoRestore = GetPrivateProfileIntW(L"World", L"AutoRestore", 1, ini.c_str()) != 0;
    panelScale = std::clamp(GetPrivateProfileIntW(L"Interface", L"ScalePercent", 100, ini.c_str()),
                            75u, 125u) /
                 100.0f;
    panelAutoClose =
        std::min(GetPrivateProfileIntW(L"Interface", L"AutoCloseSeconds", 0, ini.c_str()), 120u);
    developerReunion = GetPrivateProfileIntW(L"Development", L"EnableReunion", 0, ini.c_str()) != 0;
    {
        std::ifstream translations(resourceDirectory / L"languages.tsv", std::ios::binary);
        locale::catalog.read(translations);
    }
    wchar_t language[16] = {};
    GetPrivateProfileStringW(L"Interface", L"Language", L"tr", language, 16, ini.c_str());
    locale::language = (wcscmp(language, L"zh") == 0 || wcscmp(language, L"zh-CN") == 0) ? locale::Language::Chinese
                       : wcscmp(language, L"ru") == 0   ? locale::Language::Russian
                       : wcscmp(language, L"es") == 0 ? locale::Language::Spanish
                       : wcscmp(language, L"en") == 0 ? locale::Language::English
                                                      : locale::Language::Turkish;
    controls::load(ini.c_str());
    if (!controls::seedMissing(ini.c_str()))
        log("SETTINGS could not write missing control bindings; existing bindings remain active.");
    gallery.initialize(resourceDirectory);
    audio.initialize(directory, log, resourceDirectory, settingsFile);
    auto archive = directory / L"people.dat";
    if (std::filesystem::exists(archive)) {
        const auto beforeNeighborhood = directory / L"people.pre-0.15.dat";
        if (!std::filesystem::exists(beforeNeighborhood) &&
            !CopyFileW(archive.c_str(), beforeNeighborhood.c_str(), TRUE))
            throw std::runtime_error("Neighborhood upgrade backup failed");
        const auto beforeBonds = directory / L"people.pre-0.16.dat"; // 0.15 cannot read V16.
        if (!std::filesystem::exists(beforeBonds) &&
            !CopyFileW(archive.c_str(), beforeBonds.c_str(), TRUE))
            throw std::runtime_error("Bonds upgrade backup failed");
        try {
            std::ifstream in(archive);
            registry = people::Registry::read(in);
        } catch (const std::exception &e) {
            // Never overwrite an unreadable archive; fall back to the previous good save.
            auto spare = directory / L"people.dat.bak";
            log(std::string("Archive unreadable (") + e.what() + "); trying people.dat.bak");
            std::ifstream in(spare);
            if (!std::filesystem::exists(spare) || !in)
                throw;
            registry = people::Registry::read(in);
            CopyFileW(
                archive.c_str(),
                (directory / (L"people.corrupt-" + std::to_wstring(std::time(nullptr)) + L".dat"))
                    .c_str(),
                TRUE);
            recovered = true;
            dirty = true;
            log("Archive recovered from people.dat.bak");
        }
        auto backup = directory / L"people.pre-0.12.dat";
        if (!std::filesystem::exists(backup) && !CopyFileW(archive.c_str(), backup.c_str(), TRUE))
            throw std::runtime_error("Pre-upgrade backup failed");
    }
    expiredMeetings = meetings::resume(registry, static_cast<std::uint64_t>(std::time(nullptr)));
    for (auto &f : registry.families) {
        if (f.presence.located)
            f.female = modelTraits.female(f.presence.model, f.female);
        spatial.update(f);
    }
    const auto existingCount = registry.families.size();
    for (unsigned i = 0; i < existingCount; ++i)
        if (!registry.families[i].origin && (registry.families[i].social.known & people::Name))
            discoverNetwork(i + 1);
    log("SA People 0.16.2 initialized; persistent identities=" +
        std::to_string(registry.families.size()));
}
void scan(Pool *pool, Byte *player) {
    bindings.prune([&](people::Handle h) { return exists(pool, h); });
    for (auto it = restored.begin(); it != restored.end();) {
        if (!bindings.active(*it))
            it = restored.erase(it);
        else
            ++it;
    }
    unsigned added = 0;
    std::vector<people::Presence> appearances;
    for (unsigned slot = 0; slot < static_cast<unsigned>(pool->size); ++slot) {
        if (pool->flags[slot] & 0x80)
            continue;
        auto ped = pedAt(pool, slot);
        if (!eligible(ped, player) || bindings.find(handle(pool, slot)) ||
            *reinterpret_cast<float *>(ped + 0x540) <= 0)
            continue;
        auto pos = position(ped);
        people::Presence p;
        p.model = *reinterpret_cast<short *>(ped + 0x22);
        p.type = *reinterpret_cast<int *>(ped + 0x598);
        p.area = ped[0x2F];
        p.x = pos.x;
        p.y = pos.y;
        p.z = pos.z;
        appearances.push_back(p);
    }
    for (unsigned slot = 0; slot < static_cast<unsigned>(pool->size); ++slot) {
        if (pool->flags[slot] & 0x80)
            continue;
        Byte *ped = pedAt(pool, slot);
        auto h = handle(pool, slot);
        unsigned id = bindings.find(h);
        if (!id) {
            if (!eligible(ped, player) || *reinterpret_cast<float *>(ped + 0x540) <= 0)
                continue;
            auto pos = position(ped);
            people::Presence appearance;
            appearance.model = h.model;
            appearance.type = *reinterpret_cast<int *>(ped + 0x598);
            appearance.area = ped[0x2F];
            appearance.x = pos.x;
            appearance.y = pos.y;
            appearance.z = pos.z;
            id = people::returningContact(registry, bindings, appearance, &appearances);
            if (id)
                log("Reattached known contact id=" + std::to_string(id));
            else {
                const bool female = femaleModel(h.model);
                if (registry.families.size() >= people::SoftLimit) {
                    id = registry.recycle([&](unsigned x) { return bindings.active(x); });
                    if (id) {
                        registry.reset(id, female);
                        returnAfter.erase(id);
                        restored.erase(id);
                    }
                }
                if (!id) {
                    if (registry.families.size() >= people::MaxFamilies)
                        continue;
                    id = registry.create(female).id;
                    ++added;
                }
            }
            bindings.bind(h, id);
        }
        // A story script may adopt a formerly random actor; stop managing it then.
        if (ped[0x484] == 1 || leasedActor(ped))
            capture(registry.families[id - 1], ped);
    }
    if (added)
        log("Registered " + std::to_string(added) +
            " ambient people; total=" + std::to_string(registry.families.size()));
}
bool hasRoom(Pool *p, bool appointment = false) {
    int free = 0;
    for (int i = 0; i < p->size; ++i)
        if (p->flags[i] & 0x80)
            ++free;
    return free >= 16 && (appointment || restored.size() < 8);
}
bool streetPoint(Vec anchor, Byte *player, Vec &chosen, float radius = 65.0f) {
    const auto paths =
        at<std::uintptr_t>(0x40CA27); // SDK ThePaths reference, supports relocated pools.
    if (!paths)
        return false;
    float best = radius * radius;
    bool found = false;
    for (unsigned area = 0; area < 64; ++area) {
        auto nodes = at<Byte *>(paths + 0x804 + area * 4);
        unsigned total = at<unsigned>(paths + 0xFA4 + area * 4);
        unsigned vehicles = at<unsigned>(paths + 0x10C4 + area * 4);
        if (!nodes || total > 65535 || vehicles > total)
            continue;
        for (unsigned i = vehicles; i < total; ++i) {
            auto n = nodes + i * 0x1C;
            if (!(n[0x18] & 15) || (n[0x18] & 0xA0) || (n[0x19] & 0x0C))
                continue;
            Vec point{*reinterpret_cast<short *>(n + 8) / 8.0f,
                      *reinterpret_cast<short *>(n + 10) / 8.0f,
                      *reinterpret_cast<short *>(n + 12) / 8.0f + 1.0f};
            float d = distance2(anchor, point);
            if (d >= best || d < 9 || std::abs(point.z - anchor.z) > 4)
                continue;
            bool groundFound = false;
            float ground = reinterpret_cast<float(__cdecl *)(float, float, float, bool *, void **)>(
                0x5696C0)(point.x, point.y, point.z + 1, &groundFound, nullptr);
            if (!groundFound || !std::isfinite(ground) || std::abs(ground + 1 - point.z) > 1.5f)
                continue;
            point.z = ground + 1;
            if (reinterpret_cast<void *(__cdecl *)(Vec, float, void *, bool, bool, bool, bool, bool,
                                                   bool)>(0x569E20)(point, 0.5f, player, true, true,
                                                                    true, true, true, false))
                continue;
            best = d;
            chosen = point;
            found = true;
        }
    }
    return found;
}
bool clearGround(Vec &v, Byte *player, bool strict = false) {
    bool found = false;
    float ground = reinterpret_cast<float(__cdecl *)(float, float, float, bool *, void **)>(
        0x5696C0)(v.x, v.y, v.z + (strict ? 1.0f : 3.0f), &found, nullptr);
    if (!found || !std::isfinite(ground) ||
        (strict ? std::abs(ground + 1 - v.z) > 1.0f : std::abs(ground - v.z) > 4))
        return false;
    v.z = ground + 1;
    return !reinterpret_cast<void *(__cdecl *)(Vec, float, void *, bool, bool, bool, bool, bool,
                                               bool)>(0x569E20)(v, 0.45f, player, true, true, true,
                                                                true, true, false);
}
void requestReturn(unsigned id, Vec pos, bool manual, bool appointment = false) {
    if (!id || pending.id || bindings.active(id))
        return;
    if (!manual && !appointment && returnAfter.count(id) && !due(GetTickCount(), returnAfter[id]))
        return;
    const auto &p = registry.families[id - 1].presence;
    if (!p.located || !p.alive || p.area != 0)
        return;
    pending = {id, pos, GetTickCount(), manual, appointment};
    returnAfter[id] = GetTickCount() + 360000;
    // Request asynchronously without clearing flags owned by mission scripts.
    reinterpret_cast<void(__cdecl *)(int, int)>(0x4087E0)(p.model, 0);
    if (manual)
        help("SA People: Kisi yukleniyor...");
}
void advanceReturn(Pool *pool, Byte *player, bool mission) {
    if (!pending.id)
        return;
    auto &f = registry.families[pending.id - 1];
    auto p = f.presence;
    Vec origin = position(player);
    if (mission || player[0x2F] != 0 || !p.alive || bindings.active(f.id) ||
        GetTickCount() - pending.started > 10000 || distance2(origin, pending.pos) > 10000) {
        pending = {};
        return;
    }
    // CStreaming::ms_aInfoForModel is read from the executable reference, as in SDK.
    Byte *streaming = at<Byte *>(0x408B25);
    if (!streaming || streaming[p.model * 0x14 + 0x10] != 1)
        return;
    if (!hasRoom(pool, pending.appointment)) {
        pending = {};
        return;
    }
    Vec spawn = pending.pos;
    if (!clearGround(spawn, player, pending.appointment || f.daily.ready)) {
        pending = {};
        return;
    }
    Byte *ped = reinterpret_cast<Byte *(__cdecl *)(int, unsigned, const Vec &, bool)>(0x612710)(
        p.type, static_cast<unsigned>(p.model), spawn, true);
    if (!ped) {
        pending = {};
        return;
    }
    auto offset = ped - pool->objects;
    if (offset < 0 || offset % 0x7C4 != 0 || offset / 0x7C4 >= pool->size)
        throw std::runtime_error("Spawn returned invalid pool actor");
    auto slot = static_cast<unsigned>(offset / 0x7C4);
    if (!bindings.bind(handle(pool, slot), f.id))
        throw std::runtime_error("Duplicate identity blocked");
    if (pending.appointment)
        retainActor(handle(pool, slot), f.id);
    if (!pending.appointment)
        restored.insert(f.id);
    *reinterpret_cast<float *>(ped + 0x540) = std::clamp(p.health, 1.0f, 10000.0f);
    reinterpret_cast<void(__thiscall *)(Byte *, float)>(0x43E0C0)(ped, p.heading);
    *reinterpret_cast<float *>(ped + 0x558) = p.heading;
    *reinterpret_cast<float *>(ped + 0x55C) = p.heading;
    capture(f, ped);
    log("RESTORED identity=" + std::to_string(f.id) + " model=" + std::to_string(p.model) +
        (pending.manual ? " manual" : " natural"));
    if (pending.manual) {
        selected = f.id;
        currentPage = 0;
        openPanel();
        help("Bulusma hazir.");
    }
    pending = {};
}
#include "Roaming.inc"
struct GameRect {
    float left, bottom, right, top;
}; // CRect's memory ordering, not constructor ordering.
void rectangle(float x, float y, float w, float h, panel::Color color) {
    GameRect r{x, y + h, x + w, y};
    reinterpret_cast<void(__cdecl *)(const GameRect &, const panel::Color &)>(0x727B60)(r, color);
}
void fontText(float x, float y, float width, float size, float scale, const std::string &value,
              panel::Color color, panel::Align align = panel::Align::Left, bool shadow = false) {
    if (glyphRenderer.draw(at<IDirect3DDevice9 *>(0xC97C28), x, y, width, size * scale,
                           locale::tr(value), color, align, shadow))
        return;
    auto setScale = reinterpret_cast<void(__cdecl *)(float, float)>(0x719380);
    setScale(size / 40 * scale, size / 20 * scale);
    float measured = reinterpret_cast<float(__cdecl *)(const char *, bool, bool)>(0x71A0E0)(
        value.c_str(), true, false);
    if (measured > width && measured > 0)
        setScale(size / 40 * scale * width / measured, size / 20 * scale);
    float used = std::min(measured, width);
    if (align == panel::Align::Right)
        x += width - used;
    else if (align == panel::Align::Center)
        x += (width - used) * 0.5f;
    if (shadow) {
        reinterpret_cast<void(__cdecl *)(panel::Color)>(0x719430)({0, 0, 0, 210});
        reinterpret_cast<void(__cdecl *)(float, float, const char *)>(0x71A700)(
            x + 1.5f * scale, y + 1.5f * scale, value.c_str());
    }
    reinterpret_cast<void(__cdecl *)(panel::Color)>(0x719430)(color);
    reinterpret_cast<void(__cdecl *)(float, float, const char *)>(0x71A700)(x, y, value.c_str());
}
// Meeting icon: a fist bump for a man, a heart with a question mark for a woman.
void drawMeetingIcon(float cx, float top, float s, bool female) {
    static const char *const fist[] = {".XXXX...XXXX.", "XXXXX.X.XXXXX", "XXXXX...XXXXX",
                                       "XXXXX.X.XXXXX", ".XXXX...XXXX.", "..XX.....XX.."};
    static const char *const heart[] = {".XX...XX.", "XXXX.XXXX", "XXXXXXXXX", "XXXXXXXXX",
                                        ".XXXXXXX.", "..XXXXX..", "...XXX...", "....X...."};
    const char *const *rows = female ? heart : fist;
    const unsigned count = female ? 8 : 6, cols = female ? 9 : 13;
    const float px = 2.6f * s;
    const float left = cx - cols * px * 0.5f - (female ? 5 * s : 0);
    const panel::Color fill =
        female ? panel::Color{222, 64, 78, 255} : panel::Color{240, 233, 215, 255};
    for (unsigned pass = 0; pass < 2; ++pass)
        for (unsigned r = 0; r < count; ++r)
            for (unsigned c = 0; c < cols; ++c)
                if (rows[r][c] == 'X')
                    rectangle(left + c * px + (pass ? 0 : 1.4f * s),
                              top + r * px + (pass ? 0 : 1.4f * s), px, px,
                              pass ? fill : panel::Color{8, 7, 6, 230});
    if (female)
        fontText(left + cols * px + 3 * s, top - 3 * s, 24 * s, 20, s, "?", {240, 233, 215, 255},
                 panel::Align::Left, true);
}
void drawEventMarker(Pool *pool, float fw, float fh) {
    people::Handle owner = roadside.person     ? roadside.owner
                           : smallEvent.person ? smallEvent.owner
                                               : lifeEvent.actor;
    if (!eventActive() || !validPool(pool) || !exists(pool, owner))
        return;
    Vec head = position(pedAt(pool, owner.slot));
    head.z += 1.45f;
    Vec screen{};
    float w = 0, h = 0;
    if (!reinterpret_cast<bool(__cdecl *)(const Vec &, Vec *, float *, float *, bool, bool)>(
            0x70CE30)(head, &screen, &w, &h, true, true))
        return;
    if (screen.x < 20 || screen.y < 20 || screen.x > fw - 20 || screen.y > fh - 20)
        return;
    float s = std::min(fw / 1280, fh / 720);
    panel::Color gold{235, 213, 142, 255};
    fontText(screen.x - 60 * s, screen.y - 20 * s, 120 * s, 16, s, "YARDIM", gold,
             panel::Align::Center, true);
    rectangle(screen.x - 3 * s, screen.y + 3 * s, 6 * s, 6 * s, gold);
}
void drawStreetHud(float fw, float fh) {
    const people::Family *target = nullptr;
    bool continuing = false;
    auto pool = at<Pool *>(0xB74490);
    if (focusHeld && focused && validPool(pool) && exists(pool, focusedHandle)) {
        target = &registry.families[focused - 1];
        auto it = liveTalks.find(focused);
        continuing =
            it != liveTalks.end() && it->second.encounter.turns && !it->second.encounter.ended;
    }
    bool repairing = false;
    auto cars = at<Pool *>(0xB74494);
    auto player = reinterpret_cast<Byte *(__cdecl *)(int)>(0x56E210)(0);
    if (roadside.person && player && carExists(cars, roadside.vehicle))
        repairing = distance2(position(player), bonnet(carAt(cars, roadside.vehicle.slot))) < 16;
    panel::Hints hints;
    if (target) {
        hints.canGreet = !target->social.cutOff;
        hints.canRob = robbable(pedAt(pool, focusedHandle.slot));
        hints.robbed = target->social.robbed != 0;
        const auto live = liveTalks.find(focused);
        const street::Encounter noEncounter{focused};
        hints.angry = street::mayDefuse(*target,
            live == liveTalks.end() ? noEncounter : live->second.encounter,
            hasOwnedFight(pool, focusedHandle)) || gangNegotiationAvailable(pool, player, focusedHandle);
        hints.ready = due(GetTickCount(), nextSpeech) && !audio.talking();
        hints.relation = street::relationship(*target);
        {
            const int type = *reinterpret_cast<int *>(pedAt(pool, focusedHandle.slot) + 0x598);
            hints.role = type == 6                 ? "Polis"
                         : type >= 7 && type <= 16 ? "Cete uyesi"
                         : type == 17              ? "Satici"
                         : type == 18              ? "Saglik gorevlisi"
                         : type == 19              ? "Itfaiyeci"
                         : type == 20              ? "Suclu"
                         : type == 21              ? "Evsiz"
                         : type == 22              ? "Sokak kadini"
                                                   : "";
        }
        hints.trust = target->social.trust;
        hints.eventAction = eventAction(focused);
        const auto focusedPed=pedAt(pool,focusedHandle.slot);
        if(communityEnabled && target->community.accidentAt && *reinterpret_cast<float *>(focusedPed+0x540)>1 &&
           *reinterpret_cast<float *>(focusedPed+0x540)<=35 && community::mayAid(registry,focused)) {
            hints.eventAction="RESCUE_AID_OR_DRIVE";hints.defuseAction="RESCUE_CALL_AMBULANCE";
        }
        if (companionOwns(focused))
            hints.eventAction = "ACT_ASK";
        auto it = liveTalks.find(focused);
        if (it != liveTalks.end()) {
            hints.canGreet = street::mayGreet(it->second.encounter) && !target->social.cutOff;
        }
        hints.canDecline = nightOffer(focused);
        if (!hints.eventAction.empty())
            hints.canGreet = true;
    }
    auto scene = panel::roaming(target, continuing, speaker, spoken,
                                subtitleUntil && !due(GetTickCount(), subtitleUntil), repairing,
                                roadside.repair.progress, fw, fh, hints);
    for (const auto &r : scene.rectangles)
        rectangle(r.x, r.y, r.w, r.h, r.color);
    for (const auto &t : scene.texts)
        fontText(t.x, t.y, t.width, t.size, 1, t.value, t.color, t.align, t.shadow);
    drawEventMarker(pool, fw, fh);
    if (target) {
        Vec head = position(pedAt(pool, focusedHandle.slot));
        head.z += 1.0f;
        Vec screen{};
        float w = 0, h = 0;
        if (reinterpret_cast<bool(__cdecl *)(const Vec &, Vec *, float *, float *, bool, bool)>(
                0x70CE30)(head, &screen, &w, &h, true, true)) {
            float r = 8 * std::min(fw / 1280, fh / 720);
            float y = screen.y - 18 * std::min(fw / 1280, fh / 720);
            for (unsigned line = 0; line < 10; ++line) {
                float half = r * (1.0f - line / 10.0f);
                rectangle(screen.x - half, y + line * (r * 1.8f / 10), half * 2,
                          std::max(1.0f, r * 1.8f / 10), {240, 237, 219, 245});
            }
        }
    }
}
void renderInterface() {
    if (at<bool>(0xBA67A4) || at<bool>(0xB7CB48) || at<bool>(0xB7CB49)) {
        closePhone(); // Pause menu: never leave the phone holding CJ.
        return;
    }
    if (disabled || storyActive() || (lifeEvent.night && lifeEvent.stage >= 4))
        return;
    if (!reinterpret_cast<Byte *(__cdecl *)(int)>(0x56E210)(0))
        return;
    DWORD now = GetTickCount();
    bool showToast = toastUntil && !due(now, toastUntil);
    if (panelOpen && panelAutoClose && now - panelTouched >= panelAutoClose * 1000)
        closePhone();
    bool showPanel = panelOpen && !reinterpret_cast<bool(__cdecl *)()>(0x464D50)();
    bool showVitals = vitalsInstalled && controls::down(controls::Vitals);
    bool showMeeting = false;
    for (const auto &a : registry.appointments)
        if (meetings::active(a))
            showMeeting = true;
    if (!showPanel && !showVitals && !showMeeting && !lifeActive() && !showToast && !focusHeld &&
        !roadside.person && !smallEvent.person && !(subtitleUntil && !due(now, subtitleUntil)))
        return;
    int width = at<int>(0xC17044), height = at<int>(0xC17048); // RsGlobal.maximumWidth/Height.
    if (width < 320 || height < 200 || width > 16384 || height > 16384)
        return;
    float fw = static_cast<float>(width), fh = static_cast<float>(height);
    auto l = phone::layout(fw, fh, panelScale);
    // CFont global settings + RenderState; do not leak our style into the HUD.
    std::array<Byte, 0x7C> fontState{};
    std::memcpy(fontState.data(), reinterpret_cast<void *>(0xC71A54), fontState.size());
    struct RestoreFont {
        std::array<Byte, 0x7C> &data;
        ~RestoreFont() {
            std::memcpy(reinterpret_cast<void *>(0xC71A54), data.data(), data.size());
        }
    } restore{fontState};
    reinterpret_cast<void(__cdecl *)(short)>(0x719490)(2); // Menu font, proportionally spaced.
    reinterpret_cast<void(__cdecl *)(unsigned char)>(0x719610)(1); // Align left.
    reinterpret_cast<void(__cdecl *)(bool, bool)>(0x7195C0)(false, false);
    reinterpret_cast<void(__cdecl *)(bool)>(0x719600)(false);
    reinterpret_cast<void(__cdecl *)(bool)>(0x7195B0)(true);
    reinterpret_cast<void(__cdecl *)(short)>(0x719570)(0);
    reinterpret_cast<void(__cdecl *)(short)>(0x719590)(0);
    reinterpret_cast<void(__cdecl *)(float)>(0x719420)(0);
    reinterpret_cast<void(__cdecl *)(float)>(0x719500)(255);
    reinterpret_cast<void(__cdecl *)(float)>(0x7194D0)(fw - 8);
    if (showVitals && !showPanel) {
        std::array<float, 5> values{};
        const unsigned short stats[] = {22, 23, 21, 225, 25};
        for (unsigned i = 0; i < 5; ++i)
            values[i] = reinterpret_cast<float(__cdecl *)(unsigned short)>(0x558E40)(stats[i]);
        auto scene = vitals::compose(registry.honor, values, fw, fh);
        for (const auto &r : scene.rectangles)
            rectangle(r.x, r.y, r.w, r.h, r.color);
        for (const auto &t : scene.texts)
            fontText(t.x, t.y, t.width, t.size, 1, t.value, t.color, t.align);
    }
    if (showPanel) {
        if (panelInvalid || due(now, sceneRefresh)) {
            char clock[8] = {};
            sprintf_s(clock, "%02u:%02u", static_cast<unsigned>(at<Byte>(0xB70153)),
                      static_cast<unsigned>(at<Byte>(0xB70152)));
            panelScene = phone::compose(registry, phoneState, clock);
            panelInvalid = false;
            sceneRefresh = now + 250;
        }
        for (const auto &r : panelScene.rectangles)
            rectangle(l.x + r.x * l.scale, l.y + r.y * l.scale, r.w * l.scale, r.h * l.scale,
                      r.color);
        for (const auto &t : panelScene.texts)
            fontText(l.x + t.x * l.scale, l.y + t.y * l.scale, t.width * l.scale, t.size, l.scale,
                     t.value, t.color, t.align, t.shadow);
        if (phoneState.page == phone::Page::Meetings || phoneState.page == phone::Page::Meeting)
            drawMeetingImage(l.x + 40 * l.scale, l.y + 151 * l.scale, 28 * l.scale, 28 * l.scale);
    }
    if (!showPanel)
        drawStreetHud(fw, fh);
    if (showToast) {
        // Top-left notice: dark strip, hairlines and a red tab, fading in and out.
        float s = std::min(fw / 1280, fh / 720), left = 28 * s, top = 26 * s,
              boxW = std::min(560 * s, fw - 2 * left);
        auto lines = locale::wrap(locale::tr(message), 56);
        if (lines.size() > 3)
            lines.resize(3);
        float h = (18 + 25 * static_cast<float>(lines.size())) * s;
        float remain = static_cast<float>(toastUntil - now),
              elapsed = static_cast<float>(now - toastStart);
        float k = std::min({1.0f, elapsed / 160.0f, remain / 380.0f});
        auto alpha = [&](float base) {
            return static_cast<unsigned char>(std::clamp(base * k, 0.0f, 255.0f));
        };
        rectangle(left, top, boxW, h, {8, 8, 7, alpha(205)});
        rectangle(left, top, boxW, 1 * s, {240, 233, 215, alpha(110)});
        rectangle(left, top + h - 1 * s, boxW, 1 * s, {240, 233, 215, alpha(60)});
        rectangle(left, top, 3 * s, h, {196, 52, 42, alpha(255)});
        for (unsigned i = 0; i < lines.size(); ++i)
            fontText(left + 20 * s, top + (10 + 25 * i) * s, boxW - 34 * s, 18, s, lines[i],
                     {240, 233, 215, alpha(255)}, panel::Align::Left, true);
    }
    // Flush only our queued text; the original font buffer was drawn before this hook.
    reinterpret_cast<void(__cdecl *)()>(0x71A210)();
}
void __cdecl drawHook() {
    originalDraw();
    try {
        renderInterface();
    } catch (const std::exception &e) {
        closePhone();
        toastUntil = 0;
        log(std::string("UI: ") + e.what());
    }
}
void __cdecl processHook() {
    original();
    try {
        tick();
    } catch (const std::exception &e) {
        closePhone();
        audio.stop();
        cancelEvents();
        clearMeetingRuns();
        suppressCarSolicit(false);
        updateDealerTalks(at<Pool *>(0xB74490), nullptr, false);
        cancelCompanion(false);
        clearDepartures();
        releaseActors();
        disabled = true;
        log(e.what());
    } catch (...) {
        closePhone();
        audio.stop();
        cancelEvents();
        clearMeetingRuns();
        suppressCarSolicit(false);
        updateDealerTalks(at<Pool *>(0xB74490), nullptr, false);
        cancelCompanion(false);
        clearDepartures();
        releaseActors();
        disabled = true;
    }
}
void __cdecl restartHook() {
    if (dirty)
        save();
    resetRuntime();
    originalRestart();
}
bool patchCall(std::uintptr_t address, Process replacement, Process &previous) {
    auto call = reinterpret_cast<Byte *>(address);
    if (*call != 0xE8)
        return false;
    previous = reinterpret_cast<Process>(call + 5 + *reinterpret_cast<int *>(call + 1));
    DWORD old = 0;
    if (!VirtualProtect(call + 1, 4, PAGE_EXECUTE_READWRITE, &old))
        return false;
    *reinterpret_cast<int *>(call + 1) = static_cast<int>(
        reinterpret_cast<std::uintptr_t>(replacement) - reinterpret_cast<std::uintptr_t>(call + 5));
    DWORD ignored;
    VirtualProtect(call + 1, 4, old, &ignored);
    FlushInstructionCache(GetCurrentProcess(), call, 5);
    return true;
}
#include "MenuHooks.inc"
bool supportedExecutable() {
    if (reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr)) != 0x400000)
        return false;
    unsigned signature = at<unsigned>(0x401000);
    if (signature != 0x53EC8B55 && signature != 0x16197BE9)
        return false;
    if (GetModuleHandleW(L"samp.dll") || GetModuleHandleW(L"omp-client.dll") ||
        GetModuleHandleW(L"client.dll"))
        return false;
    return true;
}
bool install() {
    if (!supportedExecutable()) return false;
    if (!patchCall(0x53E981, processHook, original))
        return false;
    if (!patchCall(0x53EBB1, drawHook, originalDraw)) {
        disabled = true;
        log("UI hook unavailable; interface disabled.");
        return false;
    }
    log("UI HOOK OK: phone and street renderer installed.");
    log(installLanguageMenu()
            ? "MENU language row installed."
            : "MENU native language row kept; mod language is in phone settings.");
    if (!patchCall(0x748E48, restartHook, originalRestart))
        log("WARNING: restart hook unavailable; timer-reset fallback active.");
    return true;
}
} // namespace
// Available even when normal initialization is refused; never changes the game folder.
void startupDiagnostic(const wchar_t *diagnosticText) {
    wchar_t temp[MAX_PATH] = {};
    if (!GetTempPathW(MAX_PATH, temp)) return;
    const std::wstring path = std::wstring(temp) + L"SA-People-Startup.log";
    HANDLE file = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD written = 0;
    WriteFile(file, diagnosticText, static_cast<DWORD>(wcslen(diagnosticText) * sizeof(wchar_t)), &written, nullptr);
    CloseHandle(file);
}
extern "C" __declspec(dllexport) void __cdecl InitializeASI() {
    static volatile LONG initialized = 0;
    if (InterlockedCompareExchange(&initialized, 1, 0) != 0) return;
    if (!supportedExecutable()) {
        startupDiagnostic(L"Unsupported executable or multiplayer detected.\r\n");
        OutputDebugStringW(L"SA People: unsupported executable; initialization skipped.");
        return;
    }
    const auto guardName = L"Local\\SA-People-runtime-" + std::to_wstring(GetCurrentProcessId());
    instanceGuard = CreateMutexW(nullptr, FALSE, guardName.c_str());
    if (!instanceGuard || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (instanceGuard) CloseHandle(instanceGuard);
        instanceGuard = nullptr;
        OutputDebugStringW(L"SA People: another copy is already running; initialization skipped.");
        return;
    }
    startupDiagnostic(L"InitializeASI called; starting SA People 0.16.2.\r\n");
    try {
        initialize();
        if (install())
            log("HOOK OK: GTA SA 1.0 US persistent population adapter installed.");
        else {
            disabled = true;
            log("HOOK REFUSED: unsupported executable or multiplayer.");
        }
    } catch (const std::exception &e) {
        disabled = true;
        if (!directory.empty())
            log(e.what());
    }
}
BOOL WINAPI DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        moduleHandle = module;
        DisableThreadLibraryCalls(module);
        startupDiagnostic(L"SA-People library loaded; waiting for InitializeASI or ModLoader startup.\r\n");
        // ModLoader's std.asi calls LoadLibrary, but not the Ultimate ASI Loader export.
        // Initialization here does no COM/media playback and creates no worker threads.
        if (GetModuleHandleW(L"modloader.asi")) {
            // The root copy must still wait for Ultimate ASI Loader's export callback.
            // Merely having ModLoader installed is not evidence that it loaded this copy.
            wchar_t executable[32768]={}, modulePath[32768]={};
            if(GetModuleFileNameW(nullptr,executable,32768) && GetModuleFileNameW(module,modulePath,32768)) {
                try {
                    if(installpaths::modLoaderModule(executable,modulePath)) InitializeASI();
                } catch(const std::exception &) {
                    OutputDebugStringW(L"SA People: could not resolve loader startup path.");
                }
            }
        }
    }
    return TRUE;
}
