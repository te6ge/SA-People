#pragma once
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <cstring>
namespace gameexe {
inline std::filesystem::path find(const std::filesystem::path &folder) {
    for (auto name : {L"gta_sa.exe", L"gta-sa.exe", L"gta san andreas.exe"}) {
        auto file = folder / name;
        if (std::filesystem::is_regular_file(file)) return file;
    }
    return {};
}
// Match the adapter's x86/image-base/startup fingerprint; file size alone is not a version check.
inline bool supported(const std::filesystem::path &file) {
    std::ifstream in(file, std::ios::binary);
    auto read = [&](std::uint64_t offset, void *data, std::size_t size) {
        in.clear(); in.seekg(static_cast<std::streamoff>(offset));
        return static_cast<bool>(in.read(static_cast<char *>(data), static_cast<std::streamsize>(size)));
    };
    std::uint16_t magic = 0, machine = 0, sections = 0, optSize = 0, optMagic = 0;
    std::uint32_t pe = 0, signature = 0, imageBase = 0;
    if (!read(0, &magic, 2) || magic != 0x5a4d || !read(0x3c, &pe, 4) || pe > 0x100000 ||
        !read(pe, &signature, 4) || signature != 0x4550 || !read(pe+4, &machine, 2) || machine != 0x14c ||
        !read(pe+6, &sections, 2) || !sections || sections > 96 || !read(pe+20, &optSize, 2) || optSize < 64 ||
        !read(pe+24, &optMagic, 2) || optMagic != 0x10b || !read(pe+52, &imageBase, 4) || imageBase != 0x400000)
        return false;
    for (unsigned i = 0; i < sections; ++i) {
        std::uint32_t section[10] = {};
        if (!read(static_cast<std::uint64_t>(pe)+24+optSize+i*40, section, sizeof(section))) return false;
        const auto va = section[3], rawSize = section[4], raw = section[5];
        if (va <= 0x1000 && 0x1000u-va <= rawSize && rawSize-(0x1000u-va) >= 4) {
            if (!read(static_cast<std::uint64_t>(raw)+0x1000-va, &signature, 4)) return false;
            return signature == 0x53ec8b55 || signature == 0x16197be9;
        }
    }
    return false;
}
}
