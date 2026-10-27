#pragma once
#include "People.h"
#include "Dialogue.h"
#include "Street.h"
#include "Locale.h"
#include "Controls.h"
#include <algorithm>

namespace panel {
struct Color {
    unsigned char r, g, b, a = 255;
};
struct Rect {
    float x, y, w, h;
    Color color;
};
enum class Align { Left, Right, Center };
struct Text {
    float x, y, width, size;
    std::string value;
    Color color;
    Align align = Align::Left;
    bool shadow = false;
};
struct Scene {
    std::vector<Rect> rectangles;
    std::vector<Text> texts;
};
constexpr Color Background{17, 23, 28, 244}, Card{27, 35, 42, 255}, Border{47, 59, 67, 255};
constexpr Color Ink{239, 239, 226, 255}, Muted{152, 171, 176, 255}, Accent{163, 213, 172, 255};
constexpr float Width = 600, Height = 486;
struct Layout {
    float x, y, scale;
};
inline Layout layout(float width, float height, float preference = 1) {
    float scale = std::min(width / 1280.0f, height / 720.0f) * std::clamp(preference, 0.75f, 1.25f);
    return {width - (Width + 36) * scale, (height - Height * scale) * 0.5f, scale};
}
inline void text(Scene &s, float x, float y, float w, float size, const std::string &value,
                 Color color = Ink) {
    s.texts.push_back({x, y, w, size, value, color});
}
inline void row(Scene &s, float y, const std::string &label, const std::string &value) {
    s.rectangles.push_back({24, y, 552, 48, Card});
    text(s, 38, y + 13, 150, 16, label, Muted);
    text(s, 202, y + 10, 356, 20, value);
}
inline Scene compose(const people::Family *family, unsigned tab, bool /*active*/,
                     unsigned /*count*/) {
    Scene s;
    s.rectangles.push_back({5, 7, Width, Height, {0, 0, 0, 100}});
    s.rectangles.push_back({0, 0, Width, Height, Background});
    s.rectangles.push_back({0, 0, Width, 3, Accent});
    text(s, 24, 18, 370, 15, "SA PEOPLE  /  GUNLUK", Accent);
    text(s, 470, 18, 105, 15, "F6  KAPAT", Muted);
    text(s, 24, 46, 550, 30, family ? dialogue::displayName(*family) : "San Andreas insanlari");
    std::string subtitle =
        family ? "Yalnizca konusarak ogrendiklerin." : "Sokaktaki herkesin bir hikayesi var.";
    text(s, 24, 87, 550, 17, subtitle, Muted);
    const char *tabs[] = {"TANISMA", "AILE", "ANILAR"};
    tab %= 3;
    for (unsigned i = 0; i < 3; ++i) {
        float x = 24 + i * 184.0f;
        s.rectangles.push_back({x, 124, 176, 38, i == tab ? Card : Background});
        if (i == tab)
            s.rectangles.push_back({x, 160, 176, 2, Accent});
        text(s, x + 14, 134, 150, 16, tabs[i], i == tab ? Accent : Muted);
    }
    if (!family) {
        text(s, 38, 195, 520, 23, "Henuz yazacak bir hikaye yok", Ink);
        text(s, 38, 238, 520, 19, "ALT ile odaklan, G ile selam ver.", Muted);
        text(s, 38, 270, 520, 19, "Muhabbetiniz ilerlerse gunluge eklenir.", Muted);
        text(s, 38, 332, 520, 17, "Her selam bir tanidik demek degil.", Accent);
    } else {
        auto p = people::generate(*family);
        if (tab == 0) {
            row(s, 180, "YAS",
                dialogue::knows(*family, people::Age) ? std::to_string(p[0].age)
                                                      : "Henuz soylemedi");
            row(s, 236, "MESLEK",
                dialogue::knows(*family, people::Job) ? p[0].job : "Henuz soylemedi");
            row(s, 292, "KARSILASMA", std::to_string(family->social.encounters) + " kez");
            row(s, 348, "BAGINIZ", street::relationship(*family));
        } else if (tab == 1) {
            const char *labels[] = {"ANNE", "BABA", "ES", "KARDES", "YETISKIN COCUK"};
            unsigned order[] = {1, 2, 3, 4, 5};
            unsigned fields[] = {people::Mother, people::Father, people::Spouse, people::Sibling,
                                 people::Child};
            for (unsigned i = 0; i < 5; ++i) {
                const auto &relative = p[order[i]];
                float y = 177 + i * 45.0f;
                s.rectangles.push_back({24, y, 552, 40, Card});
                text(s, 38, y + 12, 157, 14, labels[i], Muted);
                text(s, 202, y + 9, 356, 18,
                     dialogue::knows(*family, fields[i]) ? relative.name : "Henuz anlatmadi");
            }
        } else {
            text(s, 38, 187, 520, 22, "Sohbetten kalanlar", Accent);
            text(s, 38, 229, 520, 19,
                 dialogue::knows(*family, people::Background) ? "San Andreas'ta buyudugunu anlatti."
                                                              : "Henuz gecmisinden bahsetmedi.");
            text(s, 38, 270, 520, 19,
                 dialogue::knows(*family, people::Child) ? "Cocugunun adi " + p[5].name + "."
                                                         : "Ailesini tanimak zaman alir.");
            std::string memory = family->social.lastNote == 7 ? "Onu tehdit edip parasini istedim."
                                 : family->social.lastNote == 9 ? "Yol parasi icin 20 dolar verdim."
                                 : family->social.lastNote == 8 ? "Yolunu bulmasina yardim ettim."
                                 : family->social.helped ? "Akusu biten arabasina yardim ettim."
                                                         : "Yururken birkac kez muhabbet ettik.";
            text(s, 38, 324, 520, 17, memory, Muted);
            text(s, 38, 365, 520, 17,
                 street::isFriend(*family) ? "Artik beni arkadasi olarak goruyor."
                                           : "Bir dahaki karsilasmada daha iyi tanirim.",
                 Muted);
        }
    }
    s.rectangles.push_back({24, 420, 552, 1, Border});
    text(s, 24, 438, 552, 16, "ALT+G  Selamla    /    F7  Sekme    /    F8  Sonraki", Muted);
    return s;
}
inline Scene conversation(const people::Family &f, const dialogue::Session &session) {
    Scene s;
    s.rectangles.push_back({5, 7, Width, Height, {0, 0, 0, 100}});
    s.rectangles.push_back({0, 0, Width, Height, Background});
    s.rectangles.push_back({0, 0, Width, 3, Accent});
    text(s, 24, 18, 370, 15, "SA PEOPLE  /  SOKAK SOHBETI", Accent);
    text(s, 470, 18, 105, 15, "F6  AYRIL", Muted);
    text(s, 24, 46, 550, 30, dialogue::displayName(f));
    text(s, 24, 94, 550, 17,
         session.stage == dialogue::Stage::Introduction ? "Nasil tanismak istersin?"
                                                        : "Sozlerin bir iz birakir.",
         Muted);
    s.rectangles.push_back({24, 140, 552, 112, Card});
    text(s, 40, 154, 510, 14, "KARSINDAKI", Accent);
    std::string remaining = session.reply;
    for (unsigned line = 0; line < 2 && !remaining.empty(); ++line) {
        auto cut = remaining.size() > 57 ? remaining.rfind(' ', 57) : remaining.size();
        if (cut == std::string::npos || cut == 0)
            cut = std::min<std::size_t>(57, remaining.size());
        text(s, 40, 180 + line * 30.0f, 510, 19, remaining.substr(0, cut));
        remaining.erase(0, std::min(cut + 1, remaining.size()));
    }
    if (session.stage != dialogue::Stage::Done) {
        auto options = dialogue::choices(session.stage);
        for (unsigned i = 0; i < 3; ++i) {
            float y = 270 + i * 45.0f;
            s.rectangles.push_back({24, y, 552, 39, Card});
            text(s, 38, y + 9, 30, 18, std::to_string(i + 1), Accent);
            text(s, 74, y + 9, 482, 18, options[i]);
        }
    } else
        text(s, 38, 300, 520, 20, "Karsilasma sona erdi.", Muted);
    s.rectangles.push_back({24, 420, 552, 1, Border});
    text(s, 24, 438, 552, 16, "1 / 2 / 3  Cevap sec    /    F6  Yoluna devam et", Muted);
    return s;
}
// Walking interaction prompts.
struct Hints {
    bool canRob = false, robbed = false, angry = false, ready = true, canGreet = true;
    std::string eventAction, relation, role, defuseAction;
    bool canDecline = false;
    int trust = 0;
};
inline Scene roaming(const people::Family *focus, bool continuing, const std::string &speaker,
                     const std::string &spoken, bool subtitles, bool repair, unsigned progress,
                     float fw, float fh, Hints hints = {}) {
    Scene scene;
    float scale = std::min(fw / 1280, fh / 720);
    auto rect = [&](float x, float y, float w, float h, Color c) {
        scene.rectangles.push_back({x, y, w, h, c});
    };
    auto label = [&](float x, float y, float w, float size, const std::string &value, Color c,
                     Align align = Align::Left, bool shadow = false) {
        scene.texts.push_back({x, y, w, size * scale, locale::tr(value), c, align, shadow});
    };
    auto circle = [&](float cx, float cy, float radius, Color color) {
        for (int row = -12; row <= 12; ++row) {
            float yy = row * radius / 13.0f;
            float half = std::sqrt(std::max(0.0f, radius * radius - yy * yy));
            rect(cx - half, cy + yy, half * 2, radius / 12.0f + 0.3f, color);
        }
    };
    const Color cream{240, 233, 215, 255}, dust{160, 153, 138, 210}, dark{10, 10, 9, 232},
        gold{214, 184, 112, 255}, red{196, 52, 42, 255};
    if (focus) {
        // Interaction prompts: ring keys on the right edge, name plate above them.
        const float radius = 13 * scale, keyX = fw - 54 * scale, right = keyX - 26 * scale,
                    width = 250 * scale, y0 = fh - 236 * scale, step = 36 * scale;
        auto option = [&](unsigned row, const std::string &name, const std::string &key,
                          bool active) {
            float top = y0 + row * step;
            Color ink = active ? cream : dust;
            label(right - width, top + 3 * scale, width, 17, name, ink, Align::Right, true);
            circle(keyX, top + 12 * scale, radius, ink);
            circle(keyX, top + 12 * scale, radius - 2.2f * scale, dark);
            label(keyX - radius, top + 4 * scale, radius * 2, 15, key, ink, Align::Center);
        };
        option(0,
               !hints.canGreet             ? "KONUSMAK ISTEMIYOR"
               : hints.eventAction.empty() ? (continuing ? "KONUSMAYA DEVAM ET" : "SELAMLA")
                                           : hints.eventAction,
               controls::keyName(controls::keys[controls::Greet].primary),
               hints.canGreet && hints.ready);
        option(1, !hints.defuseAction.empty() ? hints.defuseAction : hints.canDecline ? "NIGHT_DECLINE" : "SAKINLESTIR",
               controls::keyName(controls::keys[controls::Defuse].primary),
               hints.ready && (!hints.defuseAction.empty() || hints.canDecline || hints.angry));
        option(2, "KISKIRT", controls::keyName(controls::keys[controls::Provoke].primary), hints.ready);
        option(3, hints.robbed ? "PARASINI ZATEN ISTEDIN" : "SOY",
               controls::keyName(controls::keys[controls::Rob].primary),
               hints.canRob && !hints.robbed && hints.ready);
        // Name plate: name, relationship and a small trust gauge (centre = neutral).
        const float plateRight = keyX + radius, plateWidth = 300 * scale, plateY = y0 - 74 * scale;
        label(plateRight - plateWidth, plateY, plateWidth, 22,
              dialogue::knows(*focus, people::Name) ? dialogue::displayName(*focus)
              : !hints.role.empty()                 ? hints.role
              : focus->female                       ? "Yabanci kadin"
                                                    : "Yabanci erkek",
              cream, Align::Right, true);
        if (!hints.relation.empty()) {
            const bool tagged = !hints.role.empty() && dialogue::knows(*focus, people::Name);
            label(plateRight - plateWidth, plateY + 28 * scale, plateWidth, 13,
                  tagged ? locale::tr(hints.role) + "  /  " + locale::tr(hints.relation)
                         : hints.relation,
                  gold, Align::Right, true);
        }
        const float gaugeW = 140 * scale, gaugeX = plateRight - gaugeW,
                    gaugeY = plateY + 50 * scale;
        rect(gaugeX, gaugeY, gaugeW, 3 * scale, {240, 233, 215, 60});
        float amount = std::clamp(hints.trust, -100, 100) / 100.0f;
        rect(amount < 0 ? gaugeX + gaugeW * 0.5f + gaugeW * 0.5f * amount : gaugeX + gaugeW * 0.5f,
             gaugeY, gaugeW * 0.5f * std::abs(amount), 3 * scale, amount < 0 ? red : cream);
        rect(gaugeX + gaugeW * 0.5f - 0.5f * scale, gaugeY - 2 * scale, 1 * scale, 7 * scale,
             cream);
    }
    if (subtitles) {
        // Film-style subtitle: soft shade at the bottom, gold speaker, cream centred line.
        const float shadeTop = fh - 190 * scale;
        for (unsigned i = 0; i < 10; ++i)
            rect(0, shadeTop + i * 19 * scale, fw, 19.0f * scale,
                 {5, 5, 4, static_cast<unsigned char>(18 + i * 11)});
        const float w = std::min(980 * scale, fw - 32), x = (fw - w) / 2, y = fh - 112 * scale;
        label(x, y, w, 15, speaker, gold, Align::Center, true);
        auto lines = locale::wrap(locale::tr(spoken), 74);
        for (unsigned row = 0; row < 2 && row < lines.size(); ++row)
            label(x, y + (26 + row * 26) * scale, w, 21, lines[row], cream, Align::Center, true);
    }
    if (repair) {
        float x = fw - 398 * scale, y = fh - 388 * scale;
        rect(x, y, 370 * scale, 58 * scale, dark);
        rect(x, y, 370 * scale, 1 * scale, {240, 233, 215, 90});
        label(x + 15 * scale, y + 10 * scale, 340 * scale, 17,
              "E basili tut  /  Aku baglantisini kontrol et", cream);
        rect(x + 15 * scale, y + 43 * scale, 340 * scale, 4 * scale, {240, 233, 215, 50});
        rect(x + 15 * scale, y + 43 * scale, 340 * scale * std::min(progress, 4000u) / 4000.0f,
             4 * scale, red);
    }
    return scene;
}
} // namespace panel
