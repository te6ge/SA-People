#pragma once
#include <windows.h>
namespace setupoptions {
enum class Menu { Keep = -1, Original = 0, New = 1 };
inline bool newMenu(const wchar_t *ini) {
    return GetPrivateProfileIntW(L"Interface", L"ModernMenu", 0, ini) != 0;
}
inline bool apply(const wchar_t *ini, Menu choice) {
    return choice == Menu::Keep || WritePrivateProfileStringW(L"Interface", L"ModernMenu",
        choice == Menu::New ? L"1" : L"0", ini) != FALSE;
}
} // namespace setupoptions
