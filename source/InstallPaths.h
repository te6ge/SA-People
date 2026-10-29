#pragma once
#include <algorithm>
#include <cwctype>
#include <filesystem>
namespace installpaths {
struct Layout {
    std::filesystem::path game, resources, state;
};
inline bool sameDirectory(const std::filesystem::path &a, const std::filesystem::path &b) {
    auto left = a.lexically_normal().wstring(), right = b.lexically_normal().wstring();
    std::transform(left.begin(), left.end(), left.begin(), [](wchar_t c) { return std::towlower(c); });
    std::transform(right.begin(), right.end(), right.begin(), [](wchar_t c) { return std::towlower(c); });
    return left == right;
}
inline Layout resolve(const std::filesystem::path &executable, const std::filesystem::path &module) {
    Layout result;
    result.game = executable.parent_path();
    result.state = result.game / L"SA-People";
    auto beside = module.parent_path();
    result.resources = sameDirectory(beside, result.game) ? result.state
        : std::filesystem::is_regular_file(beside / L"languages.tsv") ? beside
        : beside / L"SA-People";
    return result;
}
inline bool modLoaderModule(const std::filesystem::path &executable, const std::filesystem::path &module) {
    const auto loader = executable.parent_path() / L"modloader";
    for(auto parent=module.parent_path(); !parent.empty() && parent!=parent.root_path(); parent=parent.parent_path())
        if(sameDirectory(parent,loader)) return true;
    return false;
}
// Explicit settings.ini beside a ModLoader module is an opt-in local configuration.
// Example files remain seeds only, and relationship saves always stay in the game root.
inline std::filesystem::path settings(const Layout &paths) {
    auto local = paths.resources / L"settings.ini";
    return std::filesystem::is_regular_file(local) ? local : paths.state / L"settings.ini";
}
inline bool seedSettings(const Layout &paths) {
    auto target = paths.state / L"settings.ini";
    if (std::filesystem::exists(target)) return false;
    for (const auto *name : {L"settings.example.ini", L"settings.ini"}) {
        auto seed = paths.resources / name;
        if (!std::filesystem::is_regular_file(seed)) continue;
        std::error_code ec;
        bool copied = std::filesystem::copy_file(seed, target, std::filesystem::copy_options::none, ec);
        if (ec && ec != std::errc::file_exists) throw std::filesystem::filesystem_error("Settings seed failed", seed, target, ec);
        return copied;
    }
    return false;
}
} // namespace installpaths
