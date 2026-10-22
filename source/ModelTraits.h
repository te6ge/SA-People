#pragma once
#include <array>
#include <sstream>
#include <string>
#include <algorithm>
#include <cctype>
namespace people {
// Read the installed game's metadata, including replacement skins' animation groups.
class ModelTraits {
    std::array<signed char, 312> genders;

  public:
    ModelTraits() {
        genders.fill(-1);
    }
    void read(std::istream &input) {
        std::string line;
        while (std::getline(input, line)) {
            auto comment = line.find('#');
            if (comment != std::string::npos)
                line.resize(comment);
            std::replace(line.begin(), line.end(), ',', ' ');
            std::istringstream row(line);
            int id;
            std::string model, texture, type, stat, animation;
            if (!(row >> id >> model >> texture >> type >> stat >> animation) || id < 0 ||
                id >= 312)
                continue;
            std::transform(animation.begin(), animation.end(), animation.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            const bool female = type == "CIVFEMALE" || type == "PROSTITUTE" ||
                                animation.find("woman") != std::string::npos ||
                                animation == "female";
            genders[id] = female ? 1 : 0;
        }
    }
    bool female(int model, bool fallback) const {
        return model >= 0 && model < 312 && genders[model] >= 0 ? genders[model] != 0 : fallback;
    }
};
} // namespace people
