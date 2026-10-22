#pragma once
#include <windows.h>
#include <d3d9.h>
#include <wincodec.h>
#include <algorithm>
#include <cctype>
#include <cwctype>
#include <filesystem>
#include <string>
#include <vector>
// Full-screen frontend artwork. Files named gtasanandreas* are the common set (95%); every other
// picture in the folder is a rare surprise (5%). Decoding goes through the Windows imaging
// component so JPEG and PNG both work; an unreadable file is skipped for the session.
namespace backdrop {
class Gallery {
    std::vector<std::filesystem::path> common, rare;
    std::filesystem::path current;
    IDirect3DDevice9 *owner = nullptr;
    IDirect3DTexture9 *texture = nullptr;
    unsigned width = 0, height = 0, state = 0x9E3779B9u;
    DWORD lastDraw = 0, shownAt = 0;
    bool scanned = false;
    std::filesystem::path root;
    unsigned next() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }
    void release() {
        if (texture)
            texture->Release();
        texture = nullptr;
    }
    static bool common_name(const std::filesystem::path &p) {
        std::wstring s = p.stem().wstring();
        std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return std::towlower(c); });
        return s.rfind(L"gtasanandreas", 0) == 0;
    }
    void scan() {
        scanned = true;
        std::error_code ec;
        for (auto &entry : std::filesystem::directory_iterator(root, ec)) {
            if (!entry.is_regular_file())
                continue;
            auto ext = entry.path().extension().wstring();
            std::transform(ext.begin(), ext.end(), ext.begin(),
                           [](wchar_t c) { return std::towlower(c); });
            if (ext != L".jpg" && ext != L".jpeg" && ext != L".png" && ext != L".bmp")
                continue;
            (common_name(entry.path()) ? common : rare).push_back(entry.path());
        }
        std::sort(common.begin(), common.end());
        std::sort(rare.begin(), rare.end());
    }
    bool load(IDirect3DDevice9 *device, const std::filesystem::path &file) {
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        IWICImagingFactory *factory = nullptr;
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(&factory))))
            return false;
        IWICBitmapDecoder *decoder = nullptr;
        IWICBitmapFrameDecode *frame = nullptr;
        IWICBitmapScaler *scaler = nullptr;
        IWICFormatConverter *converter = nullptr;
        bool ok = false;
        UINT w = 0, h = 0;
        if (SUCCEEDED(factory->CreateDecoderFromFilename(file.c_str(), nullptr, GENERIC_READ,
                                                         WICDecodeMetadataCacheOnDemand,
                                                         &decoder)) &&
            SUCCEEDED(decoder->GetFrame(0, &frame)) && SUCCEEDED(frame->GetSize(&w, &h)) && w &&
            h && SUCCEEDED(factory->CreateFormatConverter(&converter))) {
            IWICBitmapSource *source = frame;
            UINT tw = w, th = h;
            const UINT limit = 2048;
            if (std::max(w, h) > limit) {
                float k = static_cast<float>(limit) / std::max(w, h);
                tw = std::max(1u, static_cast<UINT>(w * k));
                th = std::max(1u, static_cast<UINT>(h * k));
                if (SUCCEEDED(factory->CreateBitmapScaler(&scaler)) &&
                    SUCCEEDED(scaler->Initialize(frame, tw, th, WICBitmapInterpolationModeFant)))
                    source = scaler;
                else {
                    tw = w;
                    th = h;
                }
            }
            IDirect3DTexture9 *created = nullptr;
            if (SUCCEEDED(converter->Initialize(source, GUID_WICPixelFormat32bppBGRA,
                                                WICBitmapDitherTypeNone, nullptr, 0,
                                                WICBitmapPaletteTypeCustom)) &&
                SUCCEEDED(device->CreateTexture(tw, th, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                                &created, nullptr))) {
                D3DLOCKED_RECT locked{};
                if (SUCCEEDED(created->LockRect(0, &locked, nullptr, 0))) {
                    ok = SUCCEEDED(converter->CopyPixels(nullptr, locked.Pitch,
                                                         locked.Pitch * th,
                                                         static_cast<BYTE *>(locked.pBits)));
                    created->UnlockRect(0);
                }
                if (ok) {
                    release();
                    texture = created;
                    width = tw;
                    height = th;
                } else
                    created->Release();
            }
        }
        if (converter)
            converter->Release();
        if (scaler)
            scaler->Release();
        if (frame)
            frame->Release();
        if (decoder)
            decoder->Release();
        factory->Release();
        return ok;
    }
    void choose(IDirect3DDevice9 *device) {
        for (unsigned attempt = 0; attempt < 8; ++attempt) {
            const bool surprise = !rare.empty() && (common.empty() || next() % 100 < 5);
            auto &pool = surprise ? rare : common;
            if (pool.empty())
                return;
            const auto index = next() % pool.size();
            if (load(device, pool[index])) {
                current = pool[index];
                shownAt = GetTickCount();
                return;
            }
            pool.erase(pool.begin() + index);
        }
    }

  public:
    void initialize(const std::filesystem::path &directory) {
        root = directory / L"backgrounds";
        state ^= static_cast<unsigned>(GetTickCount64() * 2654435761u) | 1u;
    }
    // Draws the artwork cover-fitted; returns false if nothing could be loaded.
    bool draw(IDirect3DDevice9 *device, float sw, float sh) {
        if (!device || FAILED(device->TestCooperativeLevel()))
            return false;
        if (owner != device) {
            release();
            owner = device;
        }
        if (!scanned)
            scan();
        const DWORD now = GetTickCount();
        // A gap in drawing means the menu was left and entered again: roll a fresh picture.
        if (!texture || now - lastDraw > 1500)
            choose(device);
        lastDraw = now;
        if (!texture)
            return false;
        float fade = std::min(1.0f, (now - shownAt) / 700.0f);
        float imageAspect = static_cast<float>(width) / height, screenAspect = sw / sh;
        float u0 = 0, u1 = 1, v0 = 0, v1 = 1;
        if (imageAspect > screenAspect) {
            float keep = screenAspect / imageAspect;
            u0 = (1 - keep) * 0.5f;
            u1 = 1 - u0;
        } else {
            float keep = imageAspect / screenAspect;
            v0 = (1 - keep) * 0.5f;
            v1 = 1 - v0;
        }
        IDirect3DStateBlock9 *saved = nullptr;
        if (FAILED(device->CreateStateBlock(D3DSBT_ALL, &saved)))
            return false;
        saved->Capture();
        device->SetPixelShader(nullptr);
        device->SetVertexShader(nullptr);
        device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
        device->SetTexture(0, texture);
        device->SetRenderState(D3DRS_ZENABLE, FALSE);
        device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        device->SetRenderState(D3DRS_LIGHTING, FALSE);
        device->SetRenderState(D3DRS_FOGENABLE, FALSE);
        device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
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
        const D3DCOLOR tint = D3DCOLOR_ARGB(static_cast<int>(255 * fade), 255, 255, 255);
        Vertex quad[4] = {{-0.5f, -0.5f, 0, 1, tint, u0, v0},
                          {sw - 0.5f, -0.5f, 0, 1, tint, u1, v0},
                          {-0.5f, sh - 0.5f, 0, 1, tint, u0, v1},
                          {sw - 0.5f, sh - 0.5f, 0, 1, tint, u1, v1}};
        device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(Vertex));
        // Vignette as smooth vertex-colour gradients: no seams, whatever the picture covers.
        device->SetTexture(0, nullptr);
        device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
        device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
        auto shade = [&](float x0, float y0, float x1, float y1, int a0, int a1, bool horizontal) {
            const D3DCOLOR c0 = D3DCOLOR_ARGB(static_cast<int>(a0 * fade), 5, 5, 4),
                           c1 = D3DCOLOR_ARGB(static_cast<int>(a1 * fade), 5, 5, 4);
            Vertex v[4] = {{x0 - 0.5f, y0 - 0.5f, 0, 1, c0, 0, 0},
                           {x1 - 0.5f, y0 - 0.5f, 0, 1, horizontal ? c1 : c0, 0, 0},
                           {x0 - 0.5f, y1 - 0.5f, 0, 1, horizontal ? c0 : c1, 0, 0},
                           {x1 - 0.5f, y1 - 0.5f, 0, 1, c1, 0, 0}};
            device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(Vertex));
        };
        shade(0, 0, sw * 0.20f, sh, 215, 150, true);
        shade(sw * 0.20f, 0, sw * 0.66f, sh, 150, 0, true);
        shade(0, 0, sw, sh * 0.16f, 150, 0, false);
        shade(0, sh * 0.84f, sw, sh, 0, 160, false);
        saved->Apply();
        saved->Release();
        return true;
    }
};
} // namespace backdrop
