#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dshow.h>
#include <filesystem>
#include <iostream>
#include <fstream>
int main() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    unsigned count = 0;
    std::ofstream out("audio-inventory.tsv");
    out << "File\tDurationMs\tDirectShow\n";
    for (const auto &e : std::filesystem::directory_iterator("audio")) {
        if (e.path().extension() != L".wav" && e.path().extension() != L".mp3")
            continue;
        IGraphBuilder *g = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER,
                                      IID_IGraphBuilder, reinterpret_cast<void **>(&g));
        if (FAILED(hr)) {
            std::cerr << "create " << std::hex << hr;
            return 1;
        }
        hr = g->RenderFile(std::filesystem::absolute(e.path()).c_str(), nullptr);
        if (FAILED(hr)) {
            std::cerr << e.path() << " render " << std::hex << hr;
            g->Release();
            return 2;
        }
        IBasicAudio *a = nullptr;
        IMediaControl *c = nullptr;
        IMediaSeeking *s = nullptr;
        g->QueryInterface(IID_IBasicAudio, reinterpret_cast<void **>(&a));
        g->QueryInterface(IID_IMediaControl, reinterpret_cast<void **>(&c));
        g->QueryInterface(IID_IMediaSeeking, reinterpret_cast<void **>(&s));
        if (!a || !c || !s || FAILED(a->put_Volume(-10000))) {
            std::cerr << "interfaces";
            return 3;
        }
        LONGLONG duration = 0;
        s->GetDuration(&duration);
        if (FAILED(c->Run()))
            return 4;
        c->Stop();
        out << e.path().filename().string() << '\t' << duration / 10000
            << "\tdecode/volume/muted-play OK\n";
        s->Release();
        c->Release();
        a->Release();
        g->Release();
        ++count;
    }
    CoUninitialize();
    std::cout << "PASS: " << count
              << " audio files decoded and privately volume-controlled (muted playback probe).\n";
    return count == 36 ? 0 : 5;
}
