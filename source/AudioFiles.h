#pragma once
#include <filesystem>
namespace audiofiles {
// Prefer compact speech assets, but keep existing WAV-only installations usable.
inline std::filesystem::path resolve(const std::filesystem::path &requested) {
    if (requested.extension() == L".wav") {
        auto compressed = requested;
        compressed.replace_extension(L".mp3");
        if (std::filesystem::is_regular_file(compressed)) return compressed;
    }
    return requested;
}
} // namespace audiofiles
