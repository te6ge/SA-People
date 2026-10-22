#pragma once
#include "Panel.h"
namespace frontend {
inline panel::Scene background(float width, float height) {
    panel::Scene scene;
    const float sx = width / 640.0f, sy = height / 448.0f;
    auto box = [&](float x, float y, float w, float h, panel::Color color) {
        scene.rectangles.push_back({x * sx, y * sy, w * sx, h * sy, color});
    };
    box(0, 0, 640, 448, {12, 18, 22, 255});
    box(16, 16, 608, 414, {21, 29, 34, 255});
    box(16, 16, 3, 414, {113, 190, 149, 255});
    box(32, 78, 576, 1, {48, 64, 68, 255});
    box(32, 403, 576, 1, {48, 64, 68, 255});
    // Abstract city blocks stay behind the native entries and the interactive map.
    for (unsigned i = 0; i < 18; ++i) {
        float h = 12.0f + ((i * 37) % 47);
        box(350.0f + i * 14, 400 - h, 9, h, {28, 40, 43, 255});
    }
    return scene;
}
// Drawn over the artwork: a western-menu vignette that keeps the native entries readable.
inline panel::Scene overlay(float width, float height) {
    panel::Scene scene;
    const float sx = width / 640.0f, sy = height / 448.0f;
    auto box = [&](float x, float y, float w, float h, panel::Color color) {
        scene.rectangles.push_back({x * sx, y * sy, w * sx, h * sy, color});
    };
    // The vignette itself is drawn smoothly with the artwork (Backdrop.h).
    box(32, 78, 576, 1, {235, 226, 205, 70});
    box(32, 403, 576, 1, {235, 226, 205, 70});
    box(32, 78, 44, 2, {196, 52, 42, 230}); // Short red tab on the title rule.
    scene.texts.push_back({34 * sx, 413 * sy, 400 * sx, 10 * sy, "SAN ANDREAS  /  SA PEOPLE",
                           {200, 192, 172, 190}});
    return scene;
}
} // namespace frontend
