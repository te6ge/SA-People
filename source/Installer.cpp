// Single-file setup. The mod files are appended to this executable by MakeRelease.ps1:
// [file bytes...][index][u64 index offset]["SAPPKG01"]. Index: u32 count, then per file
// u16 path length, UTF-8 path, u8 mode (0 overwrite, 1 keep existing, 2 ASI loader: written
// only when the game folder has no loader yet), u64 offset, u64 size.
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
#include "SetupText.h"
namespace setupui { inline const wchar_t *tr(setuptext::Key key); }
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
    if (!in || std::memcmp(magic, "SAPPKG01", 8) != 0 || indexAt > total - 20)
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
        fs::path relative = fs::u8path(e.path);
        bool safe = !e.path.empty() && e.path.find('\0') == std::string::npos &&
                    e.path.find(':') == std::string::npos && !relative.is_absolute() &&
                    !relative.has_root_path();
        for (const auto &part : relative)
            if (part == "." || part == "..")
                safe = false;
        if (!in || !safe || e.mode > 2 || e.offset > indexAt || e.size > indexAt - e.offset ||
            e.size > 256u * 1024u * 1024u || in.tellg() > static_cast<std::streamoff>(total - 16))
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
    dialog->SetTitle(setupui::tr(setuptext::Pick));
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
#include "SetupUi.h"
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    int argc = 0;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    fs::path given;
    if (argv && argc >= 3 && std::wstring(argv[1]) == L"--silent") {
        silent = true;
        given = argv[2];
    }
    if (argv)
        LocalFree(argv);
    try {
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        wchar_t self[MAX_PATH * 4] = {};
        GetModuleFileNameW(nullptr, self, MAX_PATH * 4);
        std::ifstream in(fs::path(self), std::ios::binary);
        std::vector<Entry> entries;
        if (!in || !readIndex(in, entries) || entries.empty()) {
            ask(setupui::tr(setuptext::Damaged), MB_ICONERROR);
            return 1;
        }
        fs::path game;
        if (silent) {
            game = given;
            if (!fs::exists(game / L"gta_sa.exe")) return 3;
        } else {
            game = setupui::choose();
            if (game.empty()) return 0;
        }
        std::error_code error;
        if (silent && fs::file_size(game / L"gta_sa.exe") != 14383616)
            return 4;
        if (fs::file_size(game / L"gta_sa.exe", error) != 14383616 &&
            ask(setupui::tr(setuptext::Unsupported), MB_ICONWARNING | MB_YESNO) != IDYES) {
            setupui::close();
            return 0;
        }
        bool loader = fs::exists(game / L"vorbisFile.dll") &&
                      fs::file_size(game / L"vorbisFile.dll", error) > 100000;
        loader = loader || fs::exists(game / L"dinput8.dll");
        fs::path data = game / L"SA-People";
        // Reject blocked directories/read-only destinations before replacing the old ASI.
        for (const auto &e : entries) {
            fs::path target = game / fs::u8path(e.path);
            if ((e.mode == 1 && fs::exists(target)) || (e.mode == 2 && loader))
                continue;
            if (fs::exists(target) &&
                (!fs::is_regular_file(target) ||
                 (GetFileAttributesW(target.c_str()) & FILE_ATTRIBUTE_READONLY)))
                throw std::runtime_error("Destination is not a writable file");
            for (auto parent = target.parent_path(); parent != game && !parent.empty();
                 parent = parent.parent_path())
                if (fs::exists(parent) && !fs::is_directory(parent))
                    throw std::runtime_error("A file blocks a destination directory");
        }
        if (fs::exists(game / L"SA-People.asi")) {
            wchar_t stamp[32];
            std::time_t now = std::time(nullptr);
            std::tm local{};
            localtime_s(&local, &now);
            wcsftime(stamp, 32, L"backup-%Y%m%d-%H%M%S", &local);
            fs::path backup = data / (std::wstring(stamp) + L"-" + std::to_wstring(GetTickCount()));
            fs::create_directories(backup);
            for (const wchar_t *name :
                 {L"people.dat", L"people.dat.bak", L"languages.tsv", L"settings.ini"})
                if (fs::exists(data / name))
                    fs::copy_file(data / name, backup / name, fs::copy_options::overwrite_existing);
            fs::copy_file(game / L"SA-People.asi", backup / L"SA-People.asi",
                          fs::copy_options::overwrite_existing);
        }
        std::vector<char> buffer;
        bool loaderInstalled = false;
        unsigned written = 0;
        for (const auto &e : entries) {
            setupui::progress(++written, static_cast<unsigned>(entries.size()), widen(e.path));
            fs::path target = game / fs::path(widen(e.path));
            if (e.mode == 1 && fs::exists(target))
                continue;
            if (e.mode == 2) {
                // Never replace an existing loader or another mod's dinput8.dll.
                if (loader || fs::exists(target))
                    continue;
                loaderInstalled = true;
            }
            fs::create_directories(target.parent_path());
            buffer.resize(static_cast<std::size_t>(e.size));
            in.clear();
            in.seekg(static_cast<std::streamoff>(e.offset));
            in.read(buffer.data(), static_cast<std::streamsize>(e.size));
            std::ofstream out(target, std::ios::binary | std::ios::trunc);
            out.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            out.close();
            if (!in || !out) {
                if (silent)
                    return 2;
                ask(std::wstring(setupui::tr(setuptext::WriteError)) + L"\n" + target.wstring(), MB_ICONERROR);
                setupui::close();
                return 2;
            }
        }
        setupui::addedLoader = loaderInstalled;
        setupui::finish();
        return 0;
    } catch (const std::exception &) {
        ask(std::wstring(setupui::tr(setuptext::Failed)) + L"\n\n" + setupui::tr(setuptext::Failure), MB_ICONERROR);
        setupui::close();
        return 2;
    }
}
