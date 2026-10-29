#pragma once
#include <windows.h>
#include <d3d9.h>
#include <map>
#include <string>
#include <algorithm>
#include <cstring>
#include "Panel.h"
namespace glyphs {
inline std::wstring wide(const std::string &value) {
    if (value.empty())
        return {};
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                                    static_cast<int>(value.size()), nullptr, 0);
    if (!count)
        return std::wstring(value.begin(), value.end());
    std::wstring result(static_cast<std::size_t>(count), 0);
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(),
                        count);
    return result;
}
class Renderer {
    struct Label {
        IDirect3DTexture9 *texture = nullptr;
        unsigned width = 0, height = 0;
    };
    std::map<std::wstring, Label> cache;
    IDirect3DDevice9 *owner = nullptr;
    void clear() {
        for (auto &pair : cache)
            if (pair.second.texture)
                pair.second.texture->Release();
        cache.clear();
    }
    Label make(IDirect3DDevice9 *device, const std::wstring &value, int size) {
        Label result;
        HDC dc = CreateCompatibleDC(nullptr);
        if (!dc)
            return result;
        HFONT font = CreateFontW(size, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                                 DEFAULT_PITCH, std::any_of(value.begin(), value.end(), [](wchar_t c) { return c >= 0x3000 && c <= 0x9fff; })
                                     ? L"Microsoft YaHei" : size >= 28 ? L"Georgia" : L"Tahoma");
        auto oldFont = SelectObject(dc, font);
        SIZE extent{};
        GetTextExtentPoint32W(dc, value.c_str(), static_cast<int>(value.size()), &extent);
        result.width = static_cast<unsigned>(std::clamp(extent.cx + 4, 4L, 2048L));
        result.height = static_cast<unsigned>(std::clamp(extent.cy + 4, 4L, 160L));
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = static_cast<LONG>(result.width);
        info.bmiHeader.biHeight = -static_cast<LONG>(result.height);
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        void *pixels = nullptr;
        HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
        auto oldBitmap = SelectObject(dc, bitmap);
        if (pixels) {
            std::memset(pixels, 0, result.width * result.height * 4);
            SetTextColor(dc, RGB(255, 255, 255));
            SetBkColor(dc, RGB(0, 0, 0));
            TextOutW(dc, 1, 1, value.c_str(), static_cast<int>(value.size()));
            GdiFlush();
            if (SUCCEEDED(device->CreateTexture(result.width, result.height, 1, 0, D3DFMT_A8R8G8B8,
                                                D3DPOOL_MANAGED, &result.texture, nullptr))) {
                D3DLOCKED_RECT locked{};
                if (SUCCEEDED(result.texture->LockRect(0, &locked, nullptr, 0))) {
                    auto source = static_cast<unsigned *>(pixels);
                    for (unsigned y = 0; y < result.height; ++y) {
                        auto dest = reinterpret_cast<unsigned *>(
                            static_cast<unsigned char *>(locked.pBits) + y * locked.Pitch);
                        for (unsigned x = 0; x < result.width; ++x) {
                            unsigned c = source[y * result.width + x];
                            unsigned alpha =
                                std::max({c & 255u, (c >> 8) & 255u, (c >> 16) & 255u});
                            dest[x] = (alpha << 24) | 0xFFFFFF;
                        }
                    }
                    result.texture->UnlockRect(0);
                } else {
                    result.texture->Release();
                    result.texture = nullptr;
                }
            }
        }
        SelectObject(dc, oldBitmap);
        if (bitmap)
            DeleteObject(bitmap);
        SelectObject(dc, oldFont);
        if (font)
            DeleteObject(font);
        DeleteDC(dc);
        return result;
    }

  public:
    bool draw(IDirect3DDevice9 *device, float x, float y, float width, float size,
              const std::string &text, panel::Color color, panel::Align align, bool shadow,
              bool stroke = false) {
        if (!device || FAILED(device->TestCooperativeLevel()) || text.empty())
            return false;
        if (owner != device) {
            clear();
            owner = device;
        }
        if (cache.size() >= 384)
            clear();
        auto value = wide(text);
        int pixels = std::clamp(static_cast<int>(size + 0.5f), 8, 96);
        auto key = std::to_wstring(pixels) + L":" + value;
        auto entry = cache.find(key);
        if (entry == cache.end())
            entry = cache.emplace(key, make(device, value, pixels)).first;
        const auto &label = entry->second;
        if (!label.texture)
            return false;
        float w = std::min(width, static_cast<float>(label.width)),
              h = static_cast<float>(label.height);
        if (align == panel::Align::Right)
            x += width - w;
        else if (align == panel::Align::Center)
            x += (width - w) * 0.5f;
        IDirect3DStateBlock9 *state = nullptr;
        if (FAILED(device->CreateStateBlock(D3DSBT_ALL, &state)))
            return false;
        state->Capture();
        device->SetPixelShader(nullptr);
        device->SetVertexShader(nullptr);
        device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
        device->SetTexture(0, label.texture);
        device->SetRenderState(D3DRS_ZENABLE, FALSE);
        device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        device->SetRenderState(D3DRS_LIGHTING, FALSE);
        device->SetRenderState(D3DRS_FOGENABLE, FALSE);
        device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
        device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        device->SetRenderState(D3DRS_COLORWRITEENABLE, 15);
        device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
        device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
        device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
        struct Vertex {
            float x, y, z, rhw;
            D3DCOLOR color;
            float u, v;
        };
        auto quad = [&](float dx, float dy, D3DCOLOR tint) {
            Vertex v[4] = {{x + dx - 0.5f, y + dy - 0.5f, 0, 1, tint, 0, 0},
                           {x + w + dx - 0.5f, y + dy - 0.5f, 0, 1, tint, 1, 0},
                           {x + dx - 0.5f, y + h + dy - 0.5f, 0, 1, tint, 0, 1},
                           {x + w + dx - 0.5f, y + h + dy - 0.5f, 0, 1, tint, 1, 1}};
            device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(Vertex));
        };
        if (stroke) { // Dark outline: eight offset copies keep light text readable on any art.
            const float o[8][2] = {{-1.6f, 0}, {1.6f, 0}, {0, -1.6f}, {0, 1.6f},
                                   {-1.2f, -1.2f}, {1.2f, -1.2f}, {-1.2f, 1.2f}, {1.2f, 1.2f}};
            for (auto &d : o)
                quad(d[0], d[1], D3DCOLOR_ARGB(color.a, 8, 7, 6));
        } else if (shadow)
            quad(1.5f, 1.5f, D3DCOLOR_ARGB(210, 0, 0, 0));
        quad(0, 0, D3DCOLOR_ARGB(color.a, color.r, color.g, color.b));
        state->Apply();
        state->Release();
        return true;
    }
};
} // namespace glyphs
