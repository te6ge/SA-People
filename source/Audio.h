#pragma once
#include <windows.h>
#include <dshow.h>
#include <filesystem>
#include <string>
#include <algorithm>
#include <functional>
#include <cmath>
#include <set>
#include "SocialProgress.h"

// Private DirectShow graphs; never change the game's mixer or device volume.
namespace socialaudio {
class Player {
    struct Channel {
        IGraphBuilder *graph = nullptr;
        IMediaControl *control = nullptr;
        IBasicAudio *audio = nullptr;
        DWORD until = 0;
        unsigned level = 0;
    };
    Channel voice, honor;
    std::filesystem::path root;
    std::function<void(const std::string &)> logger;
    unsigned voiceLevel = 45, honorLevel = 10, master = 127;
    DWORD lastHonor = 0;
    socialprogress::HonorSounds queued;
    std::set<std::wstring> missing;
    bool enabled = true, comReady = false;
    static bool due(DWORD now, DWORD end) {
        return now - end < 0x80000000u;
    }
    void close(Channel &c) {
        if (c.control) {
            c.control->Stop();
            c.control->Release();
        }
        if (c.audio)
            c.audio->Release();
        if (c.graph)
            c.graph->Release();
        c = {};
    }
    bool volume(Channel &c) {
        if (!c.audio)
            return false;
        double gain = static_cast<double>(c.level) * master / (100.0 * 127.0);
        long db = gain > 0 ? static_cast<long>(2000.0 * std::log10(gain)) : -10000;
        return SUCCEEDED(c.audio->put_Volume(std::clamp(db, -10000L, 0L)));
    }
    bool play(Channel &c, const wchar_t *name, unsigned level) {
        if (!enabled || !level || !master)
            return false;
        if (!comReady) {
            auto result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
            comReady = SUCCEEDED(result) || result == RPC_E_CHANGED_MODE;
            if (!comReady)
                return false;
        }
        auto file = root / name;
        if (!std::filesystem::exists(file)) {
            if (logger && missing.insert(file.filename().wstring()).second) // Report each file once.
                logger("AUDIO missing: " + file.filename().string());
            return false;
        }
        close(c);
        HRESULT result = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER,
                                          IID_IGraphBuilder, reinterpret_cast<void **>(&c.graph));
        if (SUCCEEDED(result))
            result = c.graph->RenderFile(file.c_str(), nullptr);
        if (SUCCEEDED(result))
            result =
                c.graph->QueryInterface(IID_IMediaControl, reinterpret_cast<void **>(&c.control));
        if (SUCCEEDED(result))
            result = c.graph->QueryInterface(IID_IBasicAudio, reinterpret_cast<void **>(&c.audio));
        if (FAILED(result)) {
            if (logger)
                logger("AUDIO open failed: " + file.filename().string() +
                       " HRESULT=" + std::to_string(result));
            close(c);
            return false;
        }
        IMediaSeeking *seek = nullptr;
        LONGLONG duration = 0;
        if (SUCCEEDED(
                c.graph->QueryInterface(IID_IMediaSeeking, reinterpret_cast<void **>(&seek)))) {
            seek->GetDuration(&duration);
            seek->Release();
        }
        c.level = level;
        if (!volume(c)) {
            if (logger)
                logger("AUDIO volume control unavailable; playback refused.");
            close(c);
            return false;
        }
        if (FAILED(c.control->Run())) {
            if (logger)
                logger("AUDIO playback failed: " + file.filename().string());
            close(c);
            return false;
        }
        c.until =
            GetTickCount() + static_cast<DWORD>(std::clamp<LONGLONG>(duration / 10000, 100, 30000));
        return true;
    }

  public:
    void initialize(const std::filesystem::path &directory,
                    std::function<void(const std::string &)> log) {
        root = directory / L"audio";
        logger = std::move(log);
        auto ini = directory / L"settings.ini";
        enabled = GetPrivateProfileIntW(L"Audio", L"Enabled", 1, ini.c_str()) != 0;
        voiceLevel =
            std::min(100u, GetPrivateProfileIntW(L"Audio", L"VoicePercent", 45, ini.c_str()));
        honorLevel =
            std::min(100u, GetPrivateProfileIntW(L"Audio", L"HonorPercent", 10, ini.c_str()));
        if (logger)
            logger("AUDIO ready; voice=" + std::to_string(voiceLevel) +
                   " honor=" + std::to_string(honorLevel));
    }
    void stop() {
        close(voice);
        close(honor);
        queued.pending.clear();
    }
    void update(unsigned gameVolume) {
        unsigned next = std::min(127u, gameVolume);
        if (next != master) {
            master = next;
            if (voice.graph && !volume(voice))
                close(voice);
            if (honor.graph && !volume(honor))
                close(honor);
        }
        DWORD now = GetTickCount();
        if (voice.graph && due(now, voice.until))
            close(voice);
        if (honor.graph && due(now, honor.until))
            close(honor);
        const int cue = queued.take(!voice.graph && !honor.graph &&
            (!lastHonor || now - lastHonor >= 2500));
        if (cue && play(honor, cue > 0 ? L"high.mp3" : L"low.mp3", honorLevel)) {
            lastHonor = now;
            if (logger) logger(cue > 0 ? "AUDIO honor up" : "AUDIO honor down");
        }
    }

    void speak(const wchar_t *name) {
        if (!voice.graph)
            play(voice, name, voiceLevel);
    }
    void reputation(int direction) {
        if (!direction)
            return;
        queued.push(direction);
    }
};
} // namespace socialaudio
