#pragma once
#include "Panel.h"
namespace vitals {
inline panel::Scene compose(int honor, const std::array<float, 5> &stats, float width,
                            float height) {
    panel::Scene s;
    float scale = std::min(width / 1280, height / 720), x = 24 * scale, y = height - 390 * scale,
          w = 304 * scale;
    auto rect = [&](float a, float b, float c, float d, panel::Color color) {
        s.rectangles.push_back({a, b, c, d, color});
    };
    auto text = [&](float a, float b, float c, float size, const std::string &v,
                    panel::Color color) { s.texts.push_back({a, b, c, size, locale::tr(v), color}); };
    rect(x, y, w, 224 * scale, panel::Background);
    rect(x, y, 3 * scale, 224 * scale, panel::Accent);
    text(x + 16 * scale, y + 13 * scale, w - 32 * scale, 16 * scale, "VITAL_TITLE", panel::Accent);
    const char *names[] = {"VITAL_HONOR", "VITAL_STAMINA", "VITAL_MUSCLE",
                           "VITAL_FAT",   "VITAL_LUNG",    "VITAL_APPEAL"};
    for (unsigned i = 0; i < 6; ++i) {
        float top = y + (44 + i * 28.0f) * scale, bar = x + 160 * scale, bw = 126 * scale;
        text(x + 16 * scale, top, 140 * scale, 13 * scale,
             i ? std::string(names[i])
               : locale::tr(names[i]) + "  " + (honor > 0 ? "+" : "") + std::to_string(honor),
             panel::Ink);
        rect(bar, top + 4 * scale, bw, 7 * scale, panel::Border);
        if (!i) {
            float amount = std::clamp(honor, -100, 100) / 100.0f;
            rect(amount < 0 ? bar + bw * 0.5f + bw * 0.5f * amount : bar + bw * 0.5f,
                 top + 4 * scale, bw * 0.5f * std::abs(amount), 7 * scale,
                 amount < 0 ? panel::Color{210, 112, 102, 255} : panel::Accent);
            rect(bar + bw * 0.5f, top + 2 * scale, 1 * scale, 11 * scale, panel::Ink);
        } else
            rect(bar, top + 4 * scale, bw * std::clamp(stats[i - 1] / 1000.0f, 0.0f, 1.0f),
                 7 * scale, panel::Accent);
    }
    return s;
}
} // namespace vitals
