#pragma once
#include "Dialogue.h"
// Who CJ is talking to shapes the tone, the patience and what a provocation costs.
namespace persona {
enum class Kind { Civilian, Cop, Street, Vulnerable, Service };
inline Kind of(int pedType) {
    if (pedType == 6)
        return Kind::Cop;
    if ((pedType >= 7 && pedType <= 17) || pedType == 20)
        return Kind::Street; // Gang members, dealers, criminals.
    if (pedType == 21 || pedType == 22)
        return Kind::Vulnerable; // Bums and prostitutes.
    if (pedType == 18 || pedType == 19)
        return Kind::Service; // Paramedics and firefighters.
    return Kind::Civilian;
}
// Provocations an identity tolerates before cutting CJ off. Police arrest on the third.
inline unsigned patience(const people::Family &f, Kind kind) {
    if (kind == Kind::Cop)
        return 3;
    static const int base[] = {3, 3, 2, 4, 4};
    const unsigned t = dialogue::temperament(f);
    int value = base[static_cast<unsigned>(kind)] + (t == 0 ? 1 : 0) - (t == 3 ? 1 : 0) +
                static_cast<int>(people::mix(f.id * 7 + 3) % 2);
    return static_cast<unsigned>(std::clamp(value, 2, 5));
}
template <std::size_t N> inline const char *pick(const char *const (&pool)[N], unsigned seed) {
    return pool[seed % N];
}
inline std::string greeting(Kind kind, unsigned seed) {
    static const char *const cop[] = {"Gunaydin. Bir sorun mu var?",
                                      "Sokaklar bugun sakin degil. Dikkatli ol."};
    static const char *const street[] = {"Hey, kimsin sen? Burasi bizim mahalle.",
                                         "Ne ariyorsun burada?"};
    static const char *const weak[] = {"Merhaba. Bir sey mi lazim?", "Selam. Vaktin var mi?"};
    static const char *const service[] = {"Gorev basindayiz. Bir sorun mu var?",
                                          "Yardim lazimsa soyle, isimizi yapiyoruz."};
    switch (kind) {
    case Kind::Cop:
        return pick(cop, seed);
    case Kind::Street:
        return pick(street, seed);
    case Kind::Vulnerable:
        return pick(weak, seed);
    case Kind::Service:
        return pick(service, seed);
    default:
        return {};
    }
}
inline std::string smallTalk(Kind kind, unsigned seed) {
    static const char *const cop[] = {"Devriye uzun suruyor. Sen burada ne yapiyorsun?",
                                      "Bir sey gorursen bana soyle. Bu mahalle emanet."};
    static const char *const street[] = {"Mahallede gozum uzerinde. Bir sey olursa duyarim.",
                                         "Dikkat et, buralarda herkes birbirini tanir."};
    static const char *const weak[] = {"Sokakta gunler uzun. Insan yalniz kaliyor."};
    switch (kind) {
    case Kind::Cop:
        return pick(cop, seed);
    case Kind::Street:
        return pick(street, seed);
    case Kind::Vulnerable:
        return pick(weak, seed);
    default:
        return {};
    }
}
// First provocation by someone CJ already knows: puzzlement, not insults.
inline std::string confused(Kind kind, unsigned seed) {
    static const char *const civilian[] = {"CJ? Neyin var senin, ne oluyor boyle?",
                                           "Bu ne simdi? Seni anlayamiyorum."};
    static const char *const cop[] = {"CJ? Ciddi misin sen? Bunu bir daha yapma."};
    static const char *const street[] = {"Ne bu simdi? Dalga mi geciyorsun benimle?"};
    static const char *const weak[] = {"Neden boyle yapiyorsun? Sana bir sey yapmadim."};
    static const char *const service[] = {"Ciddi misin? Biz burada is yapiyoruz."};
    switch (kind) {
    case Kind::Cop:
        return pick(cop, seed);
    case Kind::Street:
        return pick(street, seed);
    case Kind::Vulnerable:
        return pick(weak, seed);
    case Kind::Service:
        return pick(service, seed);
    default:
        return pick(civilian, seed);
    }
}
// Mid-ladder warning; empty means the generic street lines apply.
inline std::string warning(Kind kind, unsigned seed) {
    static const char *const cop[] = {"Beni sinama. Bu son uyari.",
                                      "Polisle dalga gecilmez. Geri cekil."};
    static const char *const street[] = {"Sabrimi tasirma. Bir daha deneme.",
                                         "Beni test etme. Pisman olursun."};
    if (kind == Kind::Cop)
        return pick(cop, seed);
    if (kind == Kind::Street)
        return pick(street, seed);
    return {};
}
inline std::string cutoff(Kind kind, unsigned seed) {
    static const char *const civilian[] = {"Yeter artik. Seninle bir daha konusmak istemiyorum.",
                                           "Numarami sil, beni bir daha arama."};
    static const char *const street[] = {"Bitti. Seninle konusmuyorum artik.",
                                         "Bu mahallede sana yer yok. Uzak dur."};
    static const char *const weak[] = {"Yeter. Beni rahat birak."};
    static const char *const service[] = {"Bu kadar yeter. Bizi rahat birak."};
    static const char *const cop[] = {"Yeter! Seninle isim bitti."};
    switch (kind) {
    case Kind::Cop:
        return pick(cop, seed);
    case Kind::Street:
        return pick(street, seed);
    case Kind::Vulnerable:
        return pick(weak, seed);
    case Kind::Service:
        return pick(service, seed);
    default:
        return pick(civilian, seed);
    }
}
// After the cut-off CJ can only keep provoking; they do not answer back.
inline std::string ignored(Kind kind, unsigned seed) {
    static const char *const civilian[] = {"Seninle konusmuyorum.", "Beni rahat birak."};
    static const char *const street[] = {"Defol git.", "Seni duymuyorum bile."};
    static const char *const weak[] = {"Lutfen git."};
    static const char *const service[] = {"Isimize engel olma."};
    static const char *const cop[] = {"Seninle isim yok. Uzaklas."};
    switch (kind) {
    case Kind::Cop:
        return pick(cop, seed);
    case Kind::Street:
        return pick(street, seed);
    case Kind::Vulnerable:
        return pick(weak, seed);
    case Kind::Service:
        return pick(service, seed);
    default:
        return pick(civilian, seed);
    }
}
inline const char *arrest() {
    return "Yeter! Tutuklaniyorsun. Eller basinin ustune!";
}
} // namespace persona
