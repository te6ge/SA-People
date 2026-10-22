// Single-file setup. The mod files are appended to this executable by MakeRelease.ps1:
// [file bytes...][index][u64 index offset]["SAPPKG01"]. Index: u32 count, then per file
// u16 path length, UTF-8 path, u8 mode (0 overwrite, 1 keep existing), u64 offset, u64 size.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
namespace fs = std::filesystem;
struct Entry {
    std::string path;
    unsigned char mode;
    std::uint64_t offset, size;
};
static const wchar_t *Title = L"SA People Setup";
static bool silent = false; // --silent "<game folder>": no dialogs, for scripted installs.
static int ask(const std::wstring &text, UINT flags) {
    if (silent)
        return (flags & MB_OKCANCEL) == MB_OKCANCEL ? IDOK : (flags & MB_YESNO) ? IDYES : IDOK;
    return MessageBoxW(nullptr, text.c_str(), Title, flags);
}
static std::wstring widen(const std::string &s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(static_cast<std::size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}
static bool readIndex(std::ifstream &in, std::vector<Entry> &entries) {
    in.seekg(0, std::ios::end);
    auto total = static_cast<std::uint64_t>(in.tellg());
    if (total < 16)
        return false;
    char magic[8];
    std::uint64_t indexAt = 0;
    in.seekg(static_cast<std::streamoff>(total - 16));
    in.read(reinterpret_cast<char *>(&indexAt), 8).read(magic, 8);
    if (!in || std::memcmp(magic, "SAPPKG01", 8) != 0 || indexAt >= total)
        return false;
    in.seekg(static_cast<std::streamoff>(indexAt));
    std::uint32_t count = 0;
    in.read(reinterpret_cast<char *>(&count), 4);
    if (!in || count > 10000)
        return false;
    for (std::uint32_t i = 0; i < count; ++i) {
        std::uint16_t length = 0;
        Entry e{};
        in.read(reinterpret_cast<char *>(&length), 2);
        e.path.resize(length);
        in.read(e.path.data(), length);
        in.read(reinterpret_cast<char *>(&e.mode), 1);
        in.read(reinterpret_cast<char *>(&e.offset), 8).read(reinterpret_cast<char *>(&e.size), 8);
        if (!in || e.path.find("..") != std::string::npos || e.offset + e.size > indexAt)
            return false;
        entries.push_back(e);
    }
    return true;
}
static std::wstring pickFolder() {
    std::wstring result;
    IFileOpenDialog *dialog = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&dialog))))
        return result;
    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
    dialog->SetTitle(L"GTA San Andreas klasörünü seç / Select your GTA San Andreas folder");
    if (SUCCEEDED(dialog->Show(nullptr))) {
        IShellItem *item = nullptr;
        if (SUCCEEDED(dialog->GetResult(&item))) {
            PWSTR path = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                result = path;
                CoTaskMemFree(path);
            }
            item->Release();
        }
    }
    dialog->Release();
    return result;
}
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    wchar_t self[MAX_PATH * 4] = {};
    GetModuleFileNameW(nullptr, self, MAX_PATH * 4);
    std::ifstream in(fs::path(self), std::ios::binary);
    std::vector<Entry> entries;
    if (!in || !readIndex(in, entries) || entries.empty()) {
        ask(L"Kurulum paketi bozuk.\nThe setup package is damaged.", MB_ICONERROR);
        return 1;
    }
    int argc = 0;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    fs::path given;
    if (argv && argc >= 3 && std::wstring(argv[1]) == L"--silent") {
        silent = true;
        given = argv[2];
    }
    if (!silent && ask(L"SA People, GTA San Andreas 1.0 US için bir sosyal etkileşim modudur.\n"
            L"Bir sonraki adımda oyunun klasörünü (gta_sa.exe olan) seçeceksin.\n\n"
            L"SA People is a social mod for GTA San Andreas 1.0 US.\n"
            L"Next, select your game folder (the one with gta_sa.exe).\n\nDevam? / Continue?",
            MB_ICONINFORMATION | MB_OKCANCEL) != IDOK)
        return 0;
    fs::path game;
    for (;;) {
        if (silent) {
            game = given;
            if (!fs::exists(game / L"gta_sa.exe"))
                return 3;
            break;
        }
        auto chosen = pickFolder();
        if (chosen.empty())
            return 0;
        game = chosen;
        if (fs::exists(game / L"gta_sa.exe"))
            break;
        if (ask(L"Bu klasörde gta_sa.exe yok. Başka bir klasör seç.\n"
                L"gta_sa.exe was not found in this folder. Choose another one.",
                MB_ICONWARNING | MB_RETRYCANCEL) != IDRETRY)
            return 0;
    }
    std::error_code error;
    if (fs::file_size(game / L"gta_sa.exe", error) != 14383616 &&
        ask(L"Bu gta_sa.exe 1.0 US sürümü gibi görünmüyor; mod büyük ihtimalle çalışmaz.\n"
            L"This gta_sa.exe does not look like version 1.0 US; the mod will likely not run.\n\n"
            L"Yine de kurulsun mu? / Install anyway?",
            MB_ICONWARNING | MB_YESNO) != IDYES)
        return 0;
    bool loader = fs::exists(game / L"vorbisFile.dll") && fs::file_size(game / L"vorbisFile.dll", error) > 100000;
    loader = loader || fs::exists(game / L"dinput8.dll");
    fs::path data = game / L"SA-People";
    if (fs::exists(game / L"SA-People.asi")) {
        wchar_t stamp[32];
        std::time_t now = std::time(nullptr);
        std::tm local{};
        localtime_s(&local, &now);
        wcsftime(stamp, 32, L"backup-%Y%m%d-%H%M%S", &local);
        fs::path backup = data / stamp;
        fs::create_directories(backup, error);
        for (const wchar_t *name : {L"people.dat", L"languages.tsv", L"settings.ini"})
            if (fs::exists(data / name))
                fs::copy_file(data / name, backup / name, fs::copy_options::overwrite_existing, error);
        fs::copy_file(game / L"SA-People.asi", backup / L"SA-People.asi",
                      fs::copy_options::overwrite_existing, error);
    }
    std::vector<char> buffer;
    for (const auto &e : entries) {
        fs::path target = game / fs::path(widen(e.path));
        if (e.mode == 1 && fs::exists(target))
            continue;
        fs::create_directories(target.parent_path(), error);
        buffer.resize(static_cast<std::size_t>(e.size));
        in.clear();
        in.seekg(static_cast<std::streamoff>(e.offset));
        in.read(buffer.data(), static_cast<std::streamsize>(e.size));
        std::ofstream out(target, std::ios::binary | std::ios::trunc);
        out.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        if (!in || !out) {
            if (silent)
                return 2;
            ask(L"Bir dosya yazılamadı: " + target.wstring() +
                    L"\nOyun açıksa kapatıp tekrar dene.\n\nA file could not be written. "
                    L"If the game is running, close it and try again.",
                MB_ICONERROR);
            return 2;
        }
    }
    std::wstring done = L"SA People kuruldu!\nSA People is installed!\n\n" + game.wstring();
    if (!loader)
        done += L"\n\nUYARI: ASI Loader bulunamadı (vorbisFile.dll / dinput8.dll). Mod onsuz çalışmaz.\n"
                L"WARNING: no ASI loader found. The mod does not start without one.";
    ask(done, MB_ICONINFORMATION);
    return 0;
}
