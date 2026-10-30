#define NOMINMAX
#include <windows.h>
#include <gdiplus.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include "Phone.h"
#include "ModernMenu.h"
#include "Vitals.h"
using namespace Gdiplus;
Color color(panel::Color c) {
    return Color(c.a, c.r, c.g, c.b);
}
std::wstring wide(const std::string &text) {
    int count =
        MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring result(static_cast<std::size_t>(count), 0);
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(),
                        count);
    return result;
}
bool render(const panel::Scene &scene, panel::Layout layout, int width, int height,
            const std::filesystem::path &path, const CLSID &png) {
    Bitmap bitmap(width, height, PixelFormat32bppARGB);
    {
        Graphics g(&bitmap);
        g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
        g.Clear(Color(255, 42, 48, 42));
        for (const auto &r : scene.rectangles) {
            SolidBrush brush(color(r.color));
            g.FillRectangle(&brush, layout.x + r.x * layout.scale, layout.y + r.y * layout.scale,
                            r.w * layout.scale, r.h * layout.scale);
        }
        StringFormat format;
        format.SetFormatFlags(StringFormatFlagsNoWrap);
        format.SetTrimming(StringTrimmingEllipsisCharacter);
        for (const auto &t : scene.texts) {
            SolidBrush brush(color(t.color));
            auto value = wide(t.value);
            Font font(locale::language == locale::Language::Chinese ? L"Microsoft YaHei" : L"Tahoma", std::max(8.0f, t.size * layout.scale), FontStyleBold, UnitPixel);
            RectF rect(layout.x + t.x * layout.scale, layout.y + t.y * layout.scale,
                       t.width * layout.scale, std::max(12.0f, 40 * layout.scale));
            format.SetAlignment(t.align == panel::Align::Right    ? StringAlignmentFar
                                : t.align == panel::Align::Center ? StringAlignmentCenter
                                                                  : StringAlignmentNear);
            if (t.shadow) {
                SolidBrush shadow(Color(210, 0, 0, 0));
                auto shifted = rect;
                shifted.X += 1.5f * layout.scale;
                shifted.Y += 1.5f * layout.scale;
                g.DrawString(value.c_str(), -1, &font, shifted, &format, &shadow);
            }
            g.DrawString(value.c_str(), -1, &font, rect, &format, &brush);
        }
    }
    return bitmap.Save(path.c_str(), &png, nullptr) == Ok;
}
int main() {
    {
        std::ifstream in("languages.tsv", std::ios::binary);
        locale::catalog.read(in);
    }
    GdiplusStartupInput startup;
    ULONG_PTR token = 0;
    if (GdiplusStartup(&token, &startup, nullptr) != Ok)
        return 1;
    CLSID png = {0x557cf406, 0x1a04, 0x11d3, {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}};
    std::filesystem::create_directories("preview");
    people::Registry registry;
    for (unsigned i = 0; i < 7; ++i) {
        auto &f = registry.create(i % 2 != 0);
        f.social.known = people::Name | people::Job | people::Mother;
        f.social.journal = true;
        f.social.trust = 60;
        f.social.encounters = 4;
        f.social.meaningful = 9;
    }
    registry.appointments.push_back({1, 1, 45000, 300000, 0, 0, 0});
    registry.families[0].presence = {17, 4, 100, 100, 12, 0, 100, 0, true, true};
    network::discover(registry, 1);
    registry.post(1, "MAIL_INVITATION");
    const int dimensions[][2] = {{1280, 720}, {800, 600}, {1920, 1080}};
    for (const auto &size : dimensions) {
        float sx = size[0] / 640.0f, sy = size[1] / 448.0f;
        auto scene = frontend::background(static_cast<float>(size[0]), static_cast<float>(size[1]));
        const char *items[] = {"RESUME", "START GAME", "MAP",      "STATS",
                               "BRIEF",  "OPTIONS",    "QUIT GAME"};
        for (unsigned i = 0; i < 7; ++i)
            scene.texts.push_back({64 * sx, (104 + i * 36) * sy, 400 * sx, 20 * sy, items[i],
                                   i == 5 ? panel::Color{149, 221, 175, 255} : panel::Ink});
        scene.texts.push_back(
            {360 * sx, 32 * sy, 230 * sx, 30 * sy, "PAUSE", panel::Ink, panel::Align::Right});
        auto path =
            std::filesystem::path("preview") / (L"menu-" + std::to_wstring(size[0]) + L".png");
        if (!render(scene, {0, 0, 1}, size[0], size[1], path, png))
            return 4;
    }
    for (unsigned lang = 0; lang < locale::LanguageCount; ++lang) {
        locale::language = static_cast<locale::Language>(lang);
        std::wstring code(locale::code(locale::language), locale::code(locale::language) + 2);
        for (const auto &size : dimensions) {
            float width = static_cast<float>(size[0]), height = static_cast<float>(size[1]);
            for (unsigned page = 0; page < 24; ++page) {
                phone::State state;
                state.page = page == 0   ? phone::Page::Contacts
                             : page == 1 ? phone::Page::Contact
                             : page == 2 ? phone::Page::Meetings
                                         : phone::Page::Settings;
                state.person = 1;
                if (page == 5) {
                    state.page = phone::Page::Contact;
                    state.detail = 3;
                }
                if (page == 6) {
                    state.page = phone::Page::Meeting;
                    state.distance = 42;
                    state.arrival = 2;
                }
                if (page == 7) {
                    state.page = phone::Page::Contact;
                    state.detail = 1;
                }
                if (page == 8) {
                    state.page = phone::Page::Contact;
                    state.detail = 4;
                }
                if (page >= 9 && page <= 14) {
                    const phone::Page views[] = {phone::Page::Inbox,      phone::Page::Messages,
                                                 phone::Page::Compose,    phone::Page::Actions,
                                                 phone::Page::Activities, phone::Page::Outing};
                    state.page = views[page - 9];
                }
                if (page == 15) {
                    state.page = phone::Page::Contact;
                    state.detail = 7;
                }
                if (page == 4)
                    state.notice = "PHONE_INVITE_ACCEPT";
                auto viewRegistry = registry;
                if (page == 16) {
                    state.page = phone::Page::Gift;
                    viewRegistry.worldMinutes = bonds::birthday(viewRegistry.families[0]) * 1440ull;
                }
                if (page == 17) {
                    state.page = phone::Page::Request;
                    viewRegistry.request = {1,bonds::Lend,0,1,200,viewRegistry.worldMinutes+180};
                }
                if (page == 18) state.page = phone::Page::Life;
                if (page >= 19) {
                    state.page=(page==20 || page==21)?phone::Page::RelationshipConfirm:
                        page==23?phone::Page::Contact:phone::Page::Relationship;
                    auto &partnerView=viewRegistry.families[0];
                    partnerView.female=true;partnerView.household=1;partnerView.social.romance=3;
                    partnerView.social.trust=90;partnerView.social.dates=6;partnerView.commitment.since=1;
                    viewRegistry.worldMinutes=1440;
                    if(page>=21){partnerView.commitment.stage=page==21?1u:3u;partnerView.commitment.proposedAt=1441;}
                    if(page>=22)partnerView.commitment.marriedAt=1441;
                    if(page==23){state.detail=4;partnerView.social.childStage=2;partnerView.social.childMinutes=10080;}
                }
                auto scene = phone::compose(viewRegistry, state, "14:35");
                auto path = std::filesystem::path("preview") /
                            (L"phone-" + code + L"-" + std::to_wstring(size[0]) + L"-" +
                             std::to_wstring(page) + L".png");
                if (!render(scene, phone::layout(width, height), size[0], size[1], path, png))
                    return 2;
                if (page == 11) {
                    auto contextual = registry;
                    contextual.letters.clear();
                    contextual.post(1, "MAIL_LATER_THANKS1");
                    auto contextScene = phone::compose(contextual, state, "14:35");
                    auto contextPath = std::filesystem::path("preview") /
                        (L"phone-context-" + code + L"-" + std::to_wstring(size[0]) + L".png");
                    if (!render(contextScene, phone::layout(width, height), size[0], size[1], contextPath, png))
                        return 2;
                }
            }
            auto stats = vitals::compose(-24, {810, 650, 270, 500, 640}, width, height);
            auto statsPath = std::filesystem::path("preview") /
                             (L"stats-" + code + L"-" + std::to_wstring(size[0]) + L".png");
            if (!render(stats, {0, 0, 1}, size[0], size[1], statsPath, png))
                return 5;
            for (unsigned mode = 0; mode < 3; ++mode) {
                auto npc = registry.families[mode == 0 ? 1 : 0];
                if (mode == 0)
                    npc.social.known = 0;
                panel::Hints hints;
                hints.canRob = true;
                hints.angry = mode == 2;
                hints.canGreet = mode != 2;
                auto scene = panel::roaming(
                    &npc, mode != 0, dialogue::displayName(npc),
                    mode == 2 ? "Az once de aynisini soyledin. Artik sana inanmiyorum."
                              : "Sen de benim gibi mahallede tur atiyorsun galiba.",
                    mode != 0, false, 0, width, height, hints);
                auto path = std::filesystem::path("preview") /
                            (L"street-" + code + L"-" + std::to_wstring(size[0]) + L"-" +
                             std::to_wstring(mode) + L".png");
                if (!render(scene, {0, 0, 1}, size[0], size[1], path, png))
                    return 3;
            }
        }
    }
    GdiplusShutdown(token);
    std::cout << "Phone and street previews: 5 languages, 3 resolutions.\n";
}
