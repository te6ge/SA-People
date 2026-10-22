#pragma once
#include <array>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <stdexcept>
namespace locale {
enum class Language : unsigned { Turkish, Russian, Spanish, English };
inline Language language = Language::Turkish;
inline const char *code(Language value) {
    static const char *names[] = {"tr", "ru", "es", "en"};
    return names[static_cast<unsigned>(value) % 4];
}
inline std::string nativeName(Language value) {
    static const char *names[] = {u8"Türkçe", u8"Русский", u8"Español", "English"};
    return names[static_cast<unsigned>(value) % 4];
}
inline void cycle(int direction) {
    language =
        static_cast<Language>((static_cast<unsigned>(language) + (direction < 0 ? 3 : 1)) % 4);
}
class Catalog {
    std::map<std::string, std::array<std::string, 4>> entries;
    struct Pattern {
        std::string key;
        std::array<std::string, 4> values;
    };
    std::vector<Pattern> patterns;
    static bool capture(const std::string &pattern, const std::string &value,
                        std::vector<std::string> &fields) {
        std::size_t p = 0, v = 0;
        for (unsigned index = 0;; ++index) {
            auto token = "{" + std::to_string(index) + "}";
            auto start = pattern.find(token, p);
            if (start == std::string::npos)
                return pattern.substr(p) == value.substr(v);
            auto literal = pattern.substr(p, start - p);
            if (value.compare(v, literal.size(), literal) != 0)
                return false;
            v += literal.size();
            p = start + token.size();
            auto next = pattern.find('{', p);
            auto tail = pattern.substr(p, next == std::string::npos ? std::string::npos : next - p);
            if (tail.empty() && next != std::string::npos)
                return false;
            auto end = tail.empty() ? value.size() : value.find(tail, v);
            if (end == std::string::npos)
                return false;
            fields.push_back(value.substr(v, end - v));
            v = end;
        }
    }

  public:
    void read(std::istream &in) {
        entries.clear();
        patterns.clear();
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (line.compare(0, 3, "\xEF\xBB\xBF") == 0)
                line.erase(0, 3);
            if (line.empty() || line[0] == '#')
                continue;
            std::array<std::string, 5> fields;
            std::size_t start = 0;
            for (unsigned i = 0; i < 5; ++i) {
                auto end = line.find('\t', start);
                if (i < 4 && end == std::string::npos)
                    throw std::runtime_error("Invalid language row");
                fields[i] =
                    line.substr(start, end == std::string::npos ? std::string::npos : end - start);
                start = end + 1;
            }
            if (fields[0].empty())
                throw std::runtime_error("Empty language key");
            std::array<std::string, 4> values = {fields[1], fields[2], fields[3], fields[4]};
            for (const auto &value : values)
                if (value.empty())
                    throw std::runtime_error("Empty translation");
            if (!entries.emplace(fields[0], values).second)
                throw std::runtime_error("Duplicate language key");
            if (fields[0].find("{0}") != std::string::npos)
                patterns.push_back({fields[0], values});
        }
        if (entries.size() < 30)
            throw std::runtime_error("Language catalog incomplete");
    }
    std::string translate(const std::string &source, Language lang, unsigned depth = 0) const {
        auto exact = entries.find(source);
        if (exact != entries.end())
            return exact->second[static_cast<unsigned>(lang) % 4];
        if (depth >= 3)
            return source;
        for (const auto &pattern : patterns) {
            std::vector<std::string> fields;
            if (!capture(pattern.key, source, fields))
                continue;
            std::string result = pattern.values[static_cast<unsigned>(lang) % 4];
            for (unsigned i = 0; i < fields.size(); ++i) {
                std::string token = "{" + std::to_string(i) + "}",
                            value = translate(fields[i], lang, depth + 1);
                std::size_t p = 0;
                while ((p = result.find(token, p)) != std::string::npos) {
                    result.replace(p, token.size(), value);
                    p += value.size();
                }
            }
            return result;
        }
        return source;
    }
    std::size_t size() const {
        return entries.size();
    }
};
inline Catalog catalog;
inline std::vector<std::string> wrap(std::string value, unsigned width) {
    std::vector<std::string> lines;
    while (!value.empty()) {
        std::size_t cut = 0, lastSpace = std::string::npos;
        unsigned count = 0;
        while (cut < value.size() && count < width) {
            if (value[cut] == ' ')
                lastSpace = cut;
            unsigned char c = static_cast<unsigned char>(value[cut]);
            cut += c < 0x80 ? 1 : (c & 0xE0) == 0xC0 ? 2 : (c & 0xF0) == 0xE0 ? 3 : 4;
            ++count;
        }
        cut = std::min(cut, value.size());
        if (cut < value.size() && lastSpace != std::string::npos && lastSpace > 0)
            cut = lastSpace;
        lines.push_back(value.substr(0, cut));
        if (cut < value.size() && value[cut] == ' ')
            ++cut;
        value.erase(0, cut);
    }
    return lines;
}
inline std::string tr(const std::string &source) {
    return catalog.translate(source, language);
}
inline std::string format(std::string value, const std::string &a) {
    value = tr(value);
    auto p = value.find("{0}");
    if (p != std::string::npos)
        value.replace(p, 3, tr(a));
    return value;
}
} // namespace locale
