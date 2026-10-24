// Stock Options page extension, GTA SA 1.0 US.
#pragma pack(push, 1)
struct FrontendEntry {
    Byte action;
    char name[8];
    Byte type;
    char target;
    Byte pad;
    unsigned short x, y;
    Byte align, padding;
};
#pragma pack(pop)
static_assert(sizeof(FrontendEntry) == 18, "Frontend entry ABI");
FrontendEntry *modLanguageEntry = nullptr;
bool menuLanguageInstalled = false;
std::uintptr_t callDestination(std::uintptr_t address) {
    auto p = reinterpret_cast<Byte *>(address);
    return *p == 0xE8 ? address + 5 + *reinterpret_cast<int *>(p + 1) : 0;
}
bool replaceFrontendCall(std::uintptr_t address, std::uintptr_t target, std::uintptr_t expected) {
    if (callDestination(address) != expected)
        return false;
    auto call = reinterpret_cast<Byte *>(address);
    DWORD old = 0;
    if (!VirtualProtect(call + 1, 4, PAGE_EXECUTE_READWRITE, &old))
        return false;
    *reinterpret_cast<int *>(call + 1) = static_cast<int>(target - address - 5);
    DWORD unused = 0;
    VirtualProtect(call + 1, 4, old, &unused);
    FlushInstructionCache(GetCurrentProcess(), call, 5);
    return true;
}
void __cdecl vitalStatsHook() {
    if (disabled || storyActive()) {
        reinterpret_cast<void(__cdecl *)()>(0x589650)();
        return;
    }
    at<bool>(0xBAB1DE) = controls::down(controls::Vitals);
}
bool drawingMeetingBlip = false;
unsigned meetingRomance = 0;
void __cdecl meetingCoordHook(int index, unsigned char sprite) {
    bool previous = drawingMeetingBlip;
    unsigned previousRomance = meetingRomance;
    meetingRomance = 0;
    drawingMeetingBlip = false;
    if (!disabled && !storyActive())
        for (const auto &item : meetingRuns)
            if (item.second.blip != -1 &&
                reinterpret_cast<int(__cdecl *)(int)>(0x582870)(item.second.blip) == index) {
                drawingMeetingBlip = true;
                meetingRomance = registry.families[item.first - 1].social.romance;
            }
    reinterpret_cast<void(__cdecl *)(int, unsigned char)>(0x586D60)(index, sprite);
    drawingMeetingBlip = previous;
    meetingRomance = previousRomance;
}
void __fastcall meetingSpriteHook(void *sprite, void *, const GameRect &r,
                                  const panel::Color &color) {
    if (drawingMeetingBlip && meetingRomance) {
        auto &icon = meetingRomance >= 3 ? heartImage : interestImage;
        if (icon.draw(at<IDirect3DDevice9 *>(0xC97C28),
                      directory / L"assets" /
                          (meetingRomance >= 3 ? L"heart.png" : L"heartandqmark.png"),
                      std::min(r.left, r.right) - 2, std::min(r.top, r.bottom) - 2,
                      std::abs(r.right - r.left) + 4, std::abs(r.top - r.bottom) + 4, color.a))
            return;
    }
    if (drawingMeetingBlip &&
        drawMeetingImage(std::min(r.left, r.right) - 3, std::min(r.top, r.bottom) - 3,
                         std::abs(r.right - r.left) + 6, std::abs(r.top - r.bottom) + 6, color.a))
        return;
    reinterpret_cast<void(__thiscall *)(void *, const GameRect &, const panel::Color &)>(0x728350)(
        sprite, r, color);
}
bool installMeetingSprite() {
    struct Call {
        std::uintptr_t address, target, original;
    };
    const Call calls[] = {
        {0x588347, reinterpret_cast<std::uintptr_t>(meetingCoordHook), 0x586D60},
        {0x588411, reinterpret_cast<std::uintptr_t>(meetingCoordHook), 0x586D60},
        {0x5860F7, reinterpret_cast<std::uintptr_t>(meetingSpriteHook), 0x728350}};
    for (const auto &c : calls)
        if (callDestination(c.address) != c.original)
            return false;
    unsigned done = 0;
    for (const auto &c : calls) {
        if (!replaceFrontendCall(c.address, c.target, c.original)) {
            while (done) {
                const auto &old = calls[--done];
                replaceFrontendCall(old.address, old.original, old.target);
            }
            return false;
        }
        ++done;
    }
    return true;
}
void __cdecl modernBackground(const void *, const panel::Color &) {
    int width = at<int>(0xC17044), height = at<int>(0xC17048);
    if (width < 320 || height < 200)
        return;
    rectangle(0, 0, static_cast<float>(width), static_cast<float>(height), {6, 6, 5, 255});
    const bool art = gallery.draw(at<IDirect3DDevice9 *>(0xC97C28), static_cast<float>(width),
                                  static_cast<float>(height));
    auto scene = art ? frontend::overlay(static_cast<float>(width), static_cast<float>(height))
                     : frontend::background(static_cast<float>(width), static_cast<float>(height));
    for (const auto &r : scene.rectangles)
        rectangle(r.x, r.y, r.w, r.h, r.color);
    for (const auto &t : scene.texts)
        fontText(t.x, t.y, t.width, t.size, 1, t.value, t.color, t.align);
}
void __fastcall hideMenuArtwork(void *, void *, const void *, const panel::Color &) {}
void __cdecl modernMenuText(float x, float y, const char *text) {
    auto native = reinterpret_cast<void(__cdecl *)(float, float, const char *)>(0x71A700);
    if (!text || !*text)
        return;
    // Preserve native GXT extensions, inline controller icons and multiline wrapping.
    for (const unsigned char *c = reinterpret_cast<const unsigned char *>(text); *c; ++c)
        if (*c < 32 || *c >= 127 || *c == '~') {
            native(x, y, text);
            return;
        }
    try {
        float height = static_cast<float>(at<int>(0xC17048));
        float width = static_cast<float>(at<int>(0xC17044));
        float measured = reinterpret_cast<float(__cdecl *)(const char *, bool, bool)>(0x71A0E0)(
            text, true, false);
        auto align = at<bool>(0xC71A79)   ? panel::Align::Center
                     : at<bool>(0xC71A7A) ? panel::Align::Right
                                          : panel::Align::Left;
        float span = std::min(std::max(measured, 1.0f), width - 16);
        float left = align == panel::Align::Right    ? x - span
                     : align == panel::Align::Center ? x - span / 2
                                                     : x;
        auto color = at<panel::Color>(0xC71A60);
        // Western palette: bright native entries (selection, title) turn cream, the rest dusty.
        const bool bright = std::max({color.r, color.g, color.b}) >= 200;
        color =
            bright ? panel::Color{245, 238, 220, color.a} : panel::Color{158, 151, 135, color.a};
        float size = std::clamp(at<float>(0xC71A68) * 20, 9 * height / 448, 36 * height / 448);
        std::string shown = text;
        if (size >= 28) // Title: Georgia capitals, as on a saloon poster.
            for (auto &c : shown)
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        reinterpret_cast<void(__cdecl *)()>(0x71A210)();
        if (glyphRenderer.draw(at<IDirect3DDevice9 *>(0xC97C28), left, y, span, size, shown, color,
                               align, false, true))
            return;
    } catch (const std::exception &) {
    }
    native(x, y, text);
}
bool installModernMenu() {
    if (!GetPrivateProfileIntW(L"Interface", L"ModernMenu", 1,
                               (directory / L"settings.ini").c_str()))
        return false;
    struct Patch {
        std::uintptr_t call, target, original;
    };
    std::vector<Patch> patches{
        {0x57B9BB, reinterpret_cast<std::uintptr_t>(modernBackground), 0x727B60},
        {0x57B9FD, reinterpret_cast<std::uintptr_t>(hideMenuArtwork), 0x728350}};
    // Verified direct CFont::PrintString calls in the stock frontend, never the world HUD.
    for (auto call :
         {0x574001, 0x574313, 0x574DC1, 0x574E4D, 0x5750BA, 0x57511B, 0x575F89, 0x5764E7, 0x5791C4,
          0x579311, 0x579698, 0x579878, 0x57A1FA, 0x57A405, 0x57D9A1, 0x57DA52, 0x57DB00, 0x57EBAF,
          0x57EC9A, 0x57ED53, 0x57F737, 0x57F78C, 0x57F7CB, 0x57FAD1, 0x57FD5F})
        patches.push_back({static_cast<std::uintptr_t>(call),
                           reinterpret_cast<std::uintptr_t>(modernMenuText), 0x71A700});
    for (const auto &p : patches)
        if (callDestination(p.call) != p.original)
            return false;
    std::size_t count = 0;
    for (const auto &p : patches) {
        if (!replaceFrontendCall(p.call, p.target, p.original)) {
            while (count) {
                const auto &old = patches[--count];
                replaceFrontendCall(old.call, old.original, old.target);
            }
            return false;
        }
        ++count;
    }
    return true;
}
// Language guard: the game crashes when a language whose text file (text\*.gxt) is missing is
// chosen, which happens on patched copies that only ship american.gxt. Rows on the language page
// are recognised by their GXT key; a missing file swallows the choice and shows a note instead.
DWORD missingLanguageUntil = 0;
std::string missingLanguageFile;
const char *languageFile(const char *key) {
    static const char *const map[][2] = {{"FEL_ENG", "american.gxt"}, {"FEL_FRE", "french.gxt"},
                                         {"FEL_GER", "german.gxt"},   {"FEL_ITA", "italian.gxt"},
                                         {"FEL_SPA", "spanish.gxt"}};
    for (const auto &m : map)
        if (std::strncmp(key, m[0], 8) == 0)
            return m[1];
    return nullptr;
}
bool blockMissingLanguage(Byte *manager) {
    const int page = manager[0x15D], row = *reinterpret_cast<int *>(manager + 0x54);
    if (page < 0 || page > 44 || row < 0 || row > 11)
        return false;
    auto entry = reinterpret_cast<FrontendEntry *>(0x8CE008 + page * 0xE2 + 10) + row;
    char key[9] = {};
    std::memcpy(key, entry->name, 8);
    const char *file = languageFile(key);
    if (!file)
        return false;
    std::error_code error;
    if (std::filesystem::exists(directory.parent_path() / L"text" / file, error))
        return false;
    missingLanguageFile = file;
    missingLanguageUntil = GetTickCount() + 4500;
    log(std::string("MENU blocked language without text\\") + file);
    return true;
}
void __fastcall menuDrawHook(Byte *manager, void *, bool title) {
    bool ours = menuLanguageInstalled && manager[0x15D] == 33 && modLanguageEntry &&
                modLanguageEntry->action == 66;
    char first = 0;
    if (ours) {
        first = modLanguageEntry->name[0];
        modLanguageEntry->name[0] = 0;
    }
    reinterpret_cast<void(__thiscall *)(Byte *, bool)>(0x5794A0)(manager, title);
    int width = at<int>(0xC17044), height = at<int>(0xC17048);
    const bool warn = missingLanguageUntil && !due(GetTickCount(), missingLanguageUntil);
    if (!ours && !warn)
        return;
    if (ours)
        modLanguageEntry->name[0] = first;
    reinterpret_cast<void(__cdecl *)()>(0x71A210)();
    if (width < 320 || height < 200)
        return;
    float sx = width / 640.0f, sy = height / 448.0f, center = 210 * sx;
    if (warn) {
        std::string note = locale::format("MENU_LANG_MISSING", missingLanguageFile);
        if (!glyphRenderer.draw(at<IDirect3DDevice9 *>(0xC97C28), 24 * sx, 372 * sy,
                                width - 48 * sx, 15 * sy, note, panel::Color{235, 120, 105, 255},
                                panel::Align::Center, false, true))
            fontText(24 * sx, 372 * sy, width - 48 * sx, 15 * sy, 1, note,
                     panel::Color{235, 120, 105, 255}, panel::Align::Center, true);
    }
    if (!ours)
        return;
    // The mod entry takes the native language row (fourth line) and shares its look.
    float y = 242 * sy;
    bool selectedEntry = *reinterpret_cast<int *>(manager + 0x54) == 3;
    panel::Color color =
        selectedEntry ? panel::Color{245, 238, 220, 255} : panel::Color{158, 151, 135, 255};
    std::string label =
        locale::tr("MOD_LANGUAGE") + u8"  ‹ " + locale::nativeName(locale::language) + u8" ›";
    if (!glyphRenderer.draw(at<IDirect3DDevice9 *>(0xC97C28), std::max(8.0f, center - 175 * sx), y,
                            std::min(350 * sx, width - 16.0f), 24 * sy, label, color,
                            panel::Align::Center, false, true))
        fontText(std::max(8.0f, center - 175 * sx), y, std::min(350 * sx, width - 16.0f), 24 * sy,
                 1, label, color, panel::Align::Center, true);
}
void __fastcall menuInputHook(Byte *manager, void *, char direction, char *exit, char enter) {
    if (enter && blockMissingLanguage(manager)) {
        if (exit)
            *exit = 0;
        return;
    }
    if (menuLanguageInstalled && manager[0x15D] == 33 &&
        *reinterpret_cast<int *>(manager + 0x54) == 3) {
        if (direction || enter) {
            changeLanguage(direction < 0 ? -1 : 1);
            if (exit)
                *exit = 0;
        }
        return;
    }
    reinterpret_cast<void(__thiscall *)(Byte *, char, char *, char)>(0x576FE0)(manager, direction,
                                                                               exit, enter);
}
bool installLanguageMenu() {
    log(installMeetingSprite() ? "HUD meeting sprite installed."
                               : "HUD meeting sprite unavailable.");
    vitalsInstalled =
        replaceFrontendCall(0x58FC45, reinterpret_cast<std::uintptr_t>(vitalStatsHook), 0x589650);
    log(vitalsInstalled ? "HUD honor stats installed." : "HUD honor stats unavailable.");
    log(installModernMenu() ? "MENU modern theme installed."
                            : "MENU modern theme unavailable or disabled.");
    constexpr std::uintptr_t drawCall = 0x57BA58, inputA = 0x57B66F, inputB = 0x57B702;
    if (callDestination(drawCall) != 0x5794A0 || callDestination(inputA) != 0x576FE0 ||
        callDestination(inputB) != 0x576FE0) {
        log("MENU hooks unavailable; language guard off.");
        return false;
    }
    bool hooked =
        replaceFrontendCall(drawCall, reinterpret_cast<std::uintptr_t>(menuDrawHook), 0x5794A0) &&
        replaceFrontendCall(inputA, reinterpret_cast<std::uintptr_t>(menuInputHook), 0x576FE0) &&
        replaceFrontendCall(inputB, reinterpret_cast<std::uintptr_t>(menuInputHook), 0x576FE0);
    if (!hooked) {
        replaceFrontendCall(drawCall, 0x5794A0, reinterpret_cast<std::uintptr_t>(menuDrawHook));
        replaceFrontendCall(inputA, 0x576FE0, reinterpret_cast<std::uintptr_t>(menuInputHook));
        replaceFrontendCall(inputB, 0x576FE0, reinterpret_cast<std::uintptr_t>(menuInputHook));
        log("MENU hooks unavailable; language guard off.");
        return false;
    }
    log("MENU language guard installed.");
    // The game's own language row stays by default so the game text can still be switched;
    // the mod language lives in the phone settings. MenuLanguageRow=1 restores the old row.
    if (!GetPrivateProfileIntW(L"Interface", L"MenuLanguageRow", 0,
                               (directory / L"settings.ini").c_str()))
        return false;
    auto entries = reinterpret_cast<FrontendEntry *>(0x8CE008 + 33 * 0xE2 + 10);
    if (entries[4].action != 2)
        return false;
    // Replace the stock "Language" row in place; Back and the page layout stay untouched.
    DWORD old = 0;
    if (!VirtualProtect(entries + 3, sizeof(FrontendEntry), PAGE_READWRITE, &old))
        return false;
    entries[3].action = 66;
    entries[3].target = 0;
    modLanguageEntry = entries + 3;
    menuLanguageInstalled = true;
    return true;
}
