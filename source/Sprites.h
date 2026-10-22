#pragma once
#include <objidl.h>
#include <gdiplus.h>
#include <d3d9.h>
#include <filesystem>
#include <vector>
#include <cstring>
#include <algorithm>
namespace sprites {
class Image {
    IDirect3DTexture9 *texture = nullptr;
    IDirect3DDevice9 *owner = nullptr;
    std::vector<unsigned> pixels;
    bool attempted = false;
    bool load(const std::filesystem::path &path) {
        attempted = true;
        Gdiplus::GdiplusStartupInput input;
        ULONG_PTR token = 0;
        if (Gdiplus::GdiplusStartup(&token, &input, nullptr) != Gdiplus::Ok)
            return false;
        bool result = false;
        {
            Gdiplus::Bitmap original(path.c_str());
            if (original.GetLastStatus() == Gdiplus::Ok && original.GetWidth() &&
                original.GetHeight()) {
                Gdiplus::Bitmap scaled(128, 128, PixelFormat32bppARGB);
                {
                    Gdiplus::Graphics graphics(&scaled);
                    graphics.Clear(Gdiplus::Color(0, 0, 0, 0));
                    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
                    float width = static_cast<float>(original.GetWidth()),
                          height = static_cast<float>(original.GetHeight());
                    float factor = std::min(128.0f / width, 128.0f / height);
                    float w = width * factor, h = height * factor;
                    graphics.DrawImage(&original,
                                       Gdiplus::RectF((128 - w) * 0.5f, (128 - h) * 0.5f, w, h));
                }
                Gdiplus::Rect bounds(0, 0, 128, 128);
                Gdiplus::BitmapData data{};
                if (scaled.LockBits(&bounds, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB,
                                    &data) == Gdiplus::Ok) {
                    pixels.resize(128 * 128);
                    for (unsigned y = 0; y < 128; ++y)
                        std::memcpy(pixels.data() + y * 128,
                                    static_cast<const unsigned char *>(data.Scan0) +
                                        static_cast<int>(y) * data.Stride,
                                    128 * 4);
                    scaled.UnlockBits(&data);
                    result = true;
                }
            }
        }
        Gdiplus::GdiplusShutdown(token);
        return result;
    }

  public:
    ~Image() {
        if (texture)
            texture->Release();
    }
    bool draw(IDirect3DDevice9 *device, const std::filesystem::path &path, float x, float y,
              float w, float h, unsigned char alpha = 255) {
        if (!device || FAILED(device->TestCooperativeLevel()) || w <= 0 || h <= 0)
            return false;
        if (!attempted && !load(path))
            return false;
        if (pixels.empty())
            return false;
        if (owner != device) {
            if (texture)
                texture->Release();
            texture = nullptr;
            owner = device;
        }
        if (!texture) {
            if (FAILED(device->CreateTexture(128, 128, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                             &texture, nullptr)))
                return false;
            D3DLOCKED_RECT locked{};
            if (FAILED(texture->LockRect(0, &locked, nullptr, 0))) {
                texture->Release();
                texture = nullptr;
                return false;
            }
            for (unsigned row = 0; row < 128; ++row)
                std::memcpy(static_cast<unsigned char *>(locked.pBits) + row * locked.Pitch,
                            pixels.data() + row * 128, 128 * 4);
            texture->UnlockRect(0);
        }
        IDirect3DStateBlock9 *state = nullptr;
        if (FAILED(device->CreateStateBlock(D3DSBT_ALL, &state)))
            return false;
        state->Capture();
        device->SetVertexShader(nullptr);
        device->SetPixelShader(nullptr);
        device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
        device->SetTexture(0, texture);
        device->SetRenderState(D3DRS_ZENABLE, FALSE);
        device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        device->SetRenderState(D3DRS_LIGHTING, FALSE);
        device->SetRenderState(D3DRS_FOGENABLE, FALSE);
        device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
        device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
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
        D3DCOLOR color = D3DCOLOR_ARGB(alpha, 255, 255, 255);
        Vertex vertices[4] = {{x - 0.5f, y - 0.5f, 0, 1, color, 0, 0},
                              {x + w - 0.5f, y - 0.5f, 0, 1, color, 1, 0},
                              {x - 0.5f, y + h - 0.5f, 0, 1, color, 0, 1},
                              {x + w - 0.5f, y + h - 0.5f, 0, 1, color, 1, 1}};
        auto status = device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));
        state->Apply();
        state->Release();
        return SUCCEEDED(status);
    }
};
} // namespace sprites
