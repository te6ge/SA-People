#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
// Rebindable keys. Every action has a primary and a secondary key; the secondaries give
// keyboards without arrow or function keys a full set (P opens the phone, WASD/E/Q drive it while
// it is open and CJ's controls are locked). Stored in settings.ini, [Controls] Name=KEY,KEY.
namespace controls {
enum Action : unsigned {
    Focus,
    Greet,
    Provoke,
    Defuse,
    Rob,
    Repair,
    Phone,
    PhoneUp,
    PhoneDown,
    PhoneLeft,
    PhoneRight,
    PhoneSelect,
    PhoneBack,
    PhoneClose,
    Vitals,
    Contacts,
    Meetings,
    Save,
    Count
};
struct Binding {
    int primary = 0, secondary = 0;
};
inline const Binding defaults[Count] = {
    {VK_LMENU, 0},  {'G', 0},       {'H', 0},          {'J', 0},         {'R', 0},
    {'E', 0},       {VK_F6, 'P'},   {VK_UP, 'W'},      {VK_DOWN, 'S'},   {VK_LEFT, 'A'},
    {VK_RIGHT, 'D'}, {VK_RETURN, 'E'}, {VK_BACK, 'Q'}, {VK_ESCAPE, 0},   {VK_TAB, 0},
    {VK_F7, 0},     {VK_F8, 0},     {VK_F10, 0}};
inline Binding keys[Count] = {
    {VK_LMENU, 0},  {'G', 0},       {'H', 0},          {'J', 0},         {'R', 0},
    {'E', 0},       {VK_F6, 'P'},   {VK_UP, 'W'},      {VK_DOWN, 'S'},   {VK_LEFT, 'A'},
    {VK_RIGHT, 'D'}, {VK_RETURN, 'E'}, {VK_BACK, 'Q'}, {VK_ESCAPE, 0},   {VK_TAB, 0},
    {VK_F7, 0},     {VK_F8, 0},     {VK_F10, 0}};
inline const char *iniName(unsigned a) {
    static const char *const names[Count] = {
        "Focus",      "Greet",      "Provoke",     "Defuse",    "Rob",        "Repair",
        "Phone",      "PhoneUp",    "PhoneDown",   "PhoneLeft", "PhoneRight", "PhoneSelect",
        "PhoneBack",  "PhoneClose", "Vitals",      "Contacts",  "Meetings",   "Save"};
    return a < Count ? names[a] : "";
}
inline const char *label(unsigned a) {
    static const char *const names[Count] = {
        "CTRL_FOCUS",      "CTRL_GREET",      "CTRL_PROVOKE",      "CTRL_DEFUSE",
        "CTRL_ROB",        "CTRL_REPAIR",     "CTRL_PHONE",        "CTRL_PHONE_UP",
        "CTRL_PHONE_DOWN", "CTRL_PHONE_LEFT", "CTRL_PHONE_RIGHT",  "CTRL_PHONE_SELECT",
        "CTRL_PHONE_BACK", "CTRL_PHONE_CLOSE", "CTRL_VITALS",      "CTRL_CONTACTS",
        "CTRL_MEETINGS",   "CTRL_SAVE"};
    return a < Count ? names[a] : "";
}
struct Named {
    int vk;
    const char *name;
};
inline const Named specials[] = {
    {VK_LMENU, "LALT"},     {VK_RMENU, "RALT"},     {VK_LSHIFT, "LSHIFT"}, {VK_RSHIFT, "RSHIFT"},
    {VK_LCONTROL, "LCTRL"}, {VK_RCONTROL, "RCTRL"}, {VK_TAB, "TAB"},       {VK_RETURN, "ENTER"},
    {VK_BACK, "BACKSPACE"}, {VK_ESCAPE, "ESC"},     {VK_SPACE, "SPACE"},   {VK_UP, "UP"},
    {VK_DOWN, "DOWN"},      {VK_LEFT, "LEFT"},      {VK_RIGHT, "RIGHT"},   {VK_INSERT, "INSERT"},
    {VK_DELETE, "DELETE"},  {VK_HOME, "HOME"},      {VK_END, "END"},       {VK_PRIOR, "PAGEUP"},
    {VK_NEXT, "PAGEDOWN"},  {VK_CAPITAL, "CAPSLOCK"}, {VK_LBUTTON, "MOUSE1"}, {VK_RBUTTON, "MOUSE2"},
    {VK_MBUTTON, "MOUSE3"}, {VK_XBUTTON1, "MOUSE4"}, {VK_XBUTTON2, "MOUSE5"}, {VK_OEM_COMMA, ","},
    {VK_OEM_PERIOD, "."},   {VK_OEM_MINUS, "-"},    {VK_OEM_PLUS, "+"}};
inline std::string keyName(int vk) {
    if (!vk)
        return "-";
    if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z'))
        return std::string(1, static_cast<char>(vk));
    if (vk >= VK_F1 && vk <= VK_F24)
        return "F" + std::to_string(vk - VK_F1 + 1);
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9)
        return "NUM" + std::to_string(vk - VK_NUMPAD0);
    for (const auto &s : specials)
        if (s.vk == vk)
            return s.name;
    char hex[8];
    std::snprintf(hex, sizeof hex, "0x%02X", vk);
    return hex;
}
inline int parseKey(std::string text) {
    while (!text.empty() && text.front() == ' ')
        text.erase(text.begin());
    while (!text.empty() && text.back() == ' ')
        text.pop_back();
    for (auto &c : text)
        if (c >= 'a' && c <= 'z')
            c = static_cast<char>(c - 'a' + 'A');
    if (text.empty() || text == "-" || text == "NONE")
        return 0;
    if (text.size() == 1 && ((text[0] >= '0' && text[0] <= '9') || (text[0] >= 'A' && text[0] <= 'Z')))
        return text[0];
    if (text.size() > 1 && text[0] == 'F' && text[1] >= '1' && text[1] <= '9') {
        int n = std::atoi(text.c_str() + 1);
        if (n >= 1 && n <= 24)
            return VK_F1 + n - 1;
    }
    if (text.rfind("NUM", 0) == 0 && text.size() == 4 && text[3] >= '0' && text[3] <= '9')
        return VK_NUMPAD0 + (text[3] - '0');
    if (text.rfind("0X", 0) == 0) {
        int v = static_cast<int>(std::strtol(text.c_str() + 2, nullptr, 16));
        return v > 0 && v < 256 ? v : 0;
    }
    for (const auto &s : specials)
        if (text == s.name)
            return s.vk;
    return -1; // Unknown name: keep the default.
}
inline bool held(int vk) {
    return vk > 0 && (GetAsyncKeyState(vk) & 0x8000) != 0;
}
inline bool down(unsigned a) {
    return a < Count && (held(keys[a].primary) || held(keys[a].secondary));
}
inline std::string describe(unsigned a) {
    if (a >= Count)
        return "";
    std::string s = keyName(keys[a].primary);
    if (keys[a].secondary)
        s += " / " + keyName(keys[a].secondary);
    return s;
}
inline void load(const wchar_t *ini) {
    for (unsigned a = 0; a < Count; ++a) {
        keys[a] = defaults[a];
        std::wstring name(iniName(a), iniName(a) + std::strlen(iniName(a)));
        wchar_t value[64] = {};
        if (!GetPrivateProfileStringW(L"Controls", name.c_str(), L"", value, 64, ini))
            continue;
        std::string text;
        for (const wchar_t *c = value; *c; ++c)
            text.push_back(*c < 128 ? static_cast<char>(*c) : '?');
        auto comma = text.find(',');
        int first = parseKey(text.substr(0, comma));
        int second = comma == std::string::npos ? 0 : parseKey(text.substr(comma + 1));
        if (first >= 0)
            keys[a].primary = first;
        if (second >= 0)
            keys[a].secondary = second;
    }
}
inline void save(const wchar_t *ini) {
    for (unsigned a = 0; a < Count; ++a) {
        std::string text = keyName(keys[a].primary) + "," + keyName(keys[a].secondary);
        std::wstring name(iniName(a), iniName(a) + std::strlen(iniName(a)));
        std::wstring value(text.begin(), text.end());
        WritePrivateProfileStringW(L"Controls", name.c_str(), value.c_str(), ini);
    }
}
inline void reset() {
    for (unsigned a = 0; a < Count; ++a)
        keys[a] = defaults[a];
}
// First key that went down since the previous call (for in-game rebinding).
inline int capture(bool (&previous)[256]) {
    int found = 0;
    for (int vk = 1; vk < 256; ++vk) {
        if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU)
            continue; // Prefer the left/right specific codes.
        bool now = (GetAsyncKeyState(vk) & 0x8000) != 0;
        if (now && !previous[vk] && !found)
            found = vk;
        previous[vk] = now;
    }
    return found;
}
} // namespace controls
