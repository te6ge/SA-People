#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iostream>
#include "Glyphs.h"
#include "Sprites.h"
int main() {
    WNDCLASSW type{};
    type.lpfnWndProc = DefWindowProcW;
    type.hInstance = GetModuleHandleW(nullptr);
    type.lpszClassName = L"SAPeopleGlyphCheck";
    RegisterClassW(&type);
    HWND window = CreateWindowW(type.lpszClassName, L"Glyph check", WS_OVERLAPPED, 0, 0, 1024, 512,
                                nullptr, nullptr, type.hInstance, nullptr);
    if (!window)
        return 1;
    IDirect3D9 *api = Direct3DCreate9(D3D_SDK_VERSION);
    if (!api)
        return 2;
    D3DPRESENT_PARAMETERS present{};
    present.Windowed = TRUE;
    present.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present.hDeviceWindow = window;
    present.BackBufferWidth = 1024;
    present.BackBufferHeight = 512;
    present.BackBufferFormat = D3DFMT_A8R8G8B8;
    IDirect3DDevice9 *device = nullptr;
    HRESULT result = api->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
                                       D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present, &device);
    if (FAILED(result)) {
        std::cerr << "D3D device unavailable: " << std::hex << result << '\n';
        return 3;
    }
    device->SetRenderState(D3DRS_ZENABLE, TRUE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1, 0);
    device->BeginScene();
    glyphs::Renderer renderer;
    bool drawn = renderer.draw(device, 40, 50, 940, 36, u8"Türkçe — Русский — Español — English",
                               {240, 237, 219, 255}, panel::Align::Left, true);
    drawn = renderer.draw(device, 40, 140, 900, 30, u8"Контакты  Встречи  Настройки",
                          {174, 210, 139, 255}, panel::Align::Left, true) &&
            drawn;
    sprites::Image icon;
    drawn = icon.draw(device, L"assets/meeting.png", 40, 250, 32, 32) && drawn;
    drawn = icon.draw(device, L"assets/meeting.png", 110, 240, 48, 48) && drawn;
    drawn = icon.draw(device, L"assets/meeting.png", 210, 210, 128, 128) && drawn;
    sprites::Image heart, interest;
    drawn = heart.draw(device, L"assets/heart.png", 410, 250, 24, 24) && drawn;
    drawn = heart.draw(device, L"assets/heart.png", 460, 240, 48, 48) && drawn;
    drawn = interest.draw(device, L"assets/heartandqmark.png", 560, 250, 24, 24) && drawn;
    drawn = interest.draw(device, L"assets/heartandqmark.png", 610, 240, 48, 48) && drawn;
    device->EndScene();
    DWORD z = 0, alpha = 0;
    device->GetRenderState(D3DRS_ZENABLE, &z);
    device->GetRenderState(D3DRS_ALPHABLENDENABLE, &alpha);
    if (!drawn || z != TRUE || alpha != FALSE) {
        std::cerr << "Drawing/state restore failed\n";
        return 4;
    }
    IDirect3DSurface9 *target = nullptr;
    device->GetRenderTarget(0, &target);
    D3DSURFACE_DESC desc{};
    target->GetDesc(&desc);
    IDirect3DSurface9 *copy = nullptr;
    if (FAILED(device->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format,
                                                   D3DPOOL_SYSTEMMEM, &copy, nullptr)) ||
        FAILED(device->GetRenderTargetData(target, copy)))
        return 5;
    D3DLOCKED_RECT pixels{};
    if (FAILED(copy->LockRect(&pixels, nullptr, D3DLOCK_READONLY)))
        return 6;
    unsigned filled = 0;
    for (unsigned y = 0; y < desc.Height; ++y) {
        auto row = reinterpret_cast<const unsigned *>(
            static_cast<const unsigned char *>(pixels.pBits) + y * pixels.Pitch);
        for (unsigned x = 0; x < desc.Width; ++x)
            if (row[x] & 0xFFFFFF)
                ++filled;
    }
    std::filesystem::create_directories("preview");
    std::ofstream out("preview/glyph-check.bmp", std::ios::binary);
    BITMAPFILEHEADER file{};
    file.bfType = 0x4D42;
    file.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    file.bfSize = file.bfOffBits + desc.Width * desc.Height * 4;
    BITMAPINFOHEADER info{};
    info.biSize = sizeof(info);
    info.biWidth = static_cast<LONG>(desc.Width);
    info.biHeight = -static_cast<LONG>(desc.Height);
    info.biPlanes = 1;
    info.biBitCount = 32;
    info.biCompression = BI_RGB;
    out.write(reinterpret_cast<const char *>(&file), sizeof(file));
    out.write(reinterpret_cast<const char *>(&info), sizeof(info));
    for (unsigned y = 0; y < desc.Height; ++y)
        out.write(static_cast<const char *>(pixels.pBits) + y * pixels.Pitch, desc.Width * 4);
    out.close();
    copy->UnlockRect();
    copy->Release();
    target->Release();
    device->Release();
    api->Release();
    DestroyWindow(window);
    std::cout << "Glyphs rendered=" << filled << "; Direct3D state restored; four scripts drawn.\n";
    return filled > 1000 ? 0 : 7;
}
