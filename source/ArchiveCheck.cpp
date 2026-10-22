#include "Meetings.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ctime>
int wmain(int argc, wchar_t **argv) {
    if (argc != 2)
        return 2;
    try {
        std::ifstream input{std::filesystem::path(argv[1])};
        auto archive = people::Registry::read(input);
        std::stringstream memory;
        archive.write(memory);
        auto copied = people::Registry::read(memory);
        std::stringstream roundtrip;
        copied.write(roundtrip);
        if (roundtrip.str() != memory.str())
            return 5;
        if (archive.families.size() != copied.families.size())
            return 3;
        for (unsigned i = 0; i < archive.families.size(); ++i)
            if (people::generate(archive.families[i])[0].name !=
                    people::generate(copied.families[i])[0].name ||
                archive.families[i].social.known != copied.families[i].social.known)
                return 4;
        unsigned closed = meetings::resume(copied, static_cast<std::uint64_t>(std::time(nullptr)));
        std::cout << "Archive migration verified; identities=" << archive.families.size()
                  << "; stale appointments=" << closed << "; original file unchanged.\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
