#pragma once
#include "Panel.h"
#include "Locale.h"
#include "Meetings.h"
#include "Network.h"
#include "Romance.h"
#include "SocialWorld.h"
#include "PhoneDialogue.h"
namespace phone {
enum class Page {
    Contacts,
    Meetings,
    Settings,
    Contact,
    Meeting,
    Inbox,
    Messages,
    Compose,
    Actions,
    Activities,
    Outing,
    Controls
};
struct State {
    Page page = Page::Contacts;
    unsigned cursor = 0, person = 0, detail = 0;
    unsigned distance = 0;
    unsigned networkOffset = 0;
    unsigned arrival = 0;
    std::string notice;
    bool call = false;
    unsigned activity = 0, outing = 0;
    unsigned textOffset = 0;
    bool typing = false, events = true; // Reply on its way; random events enabled.
    unsigned scale = 100;
    bool capturing = false; // Waiting for a key on the controls page.
};
inline std::vector<const people::Letter *> thread(const people::Registry &r, unsigned id) {
    std::vector<const people::Letter *> result;
    for (const auto &m : r.letters)
        if (m.person == id)
            result.push_back(&m);
    return result;
}
inline std::vector<const people::Letter *> inbox(const people::Registry &r) {
    std::vector<const people::Letter *> result;
    for (auto it = r.letters.rbegin(); it != r.letters.rend(); ++it)
        if (!it->outgoing)
            result.push_back(&*it);
    return result;
}
inline std::string letterText(const people::Registry &r, const people::Letter &m) {
    bool preset = false;
    for (const char *choice : phonedialogue::choices)
        if (m.body == choice)
            preset = true;
    preset = preset || m.body.rfind("TXT_", 0) == 0;
    std::string text = m.outgoing && !preset ? m.body : locale::tr(m.body);
    if (m.source && m.source <= r.families.size()) {
        auto pos = text.find("{0}");
        if (pos != std::string::npos)
            text.replace(pos, 3, locale::tr(dialogue::displayName(r.families[m.source - 1])));
    }
    return text;
}
// Chat view: every message wrapped into bubble lines, oldest first, blank line between messages.
struct ChatLine {
    std::string text;
    bool outgoing = false, spacer = false;
};
inline std::vector<ChatLine> chatLines(const people::Registry &r, unsigned id, bool typing) {
    std::vector<ChatLine> lines;
    for (const auto *m : thread(r, id)) {
        if (!lines.empty())
            lines.push_back({"", false, true});
        for (auto &piece : locale::wrap(letterText(r, *m), 26))
            lines.push_back({piece, m->outgoing, false});
    }
    if (typing) {
        if (!lines.empty())
            lines.push_back({"", false, true});
        lines.push_back({locale::tr("MAIL_TYPING"), false, false});
    }
    return lines;
}
inline std::vector<unsigned> contacts(const people::Registry &registry) {
    std::vector<unsigned> ids;
    for (const auto &f : registry.families)
        if (f.social.journal)
            ids.push_back(f.id);
    return ids;
}
inline panel::Layout layout(float width, float height, float preference = 1) {
    float s =
        std::min(std::min(width / 1280.0f, height / 720.0f) * std::clamp(preference, 0.75f, 1.25f),
                 height / 680.0f);
    return {width - 365 * s, (height - 650 * s) * 0.5f, s};
}
inline panel::Scene compose(const people::Registry &registry, const State &state,
                            const std::string &clock) {
    panel::Scene scene;
    using panel::Align;
    using panel::Color;
    Color body{40, 43, 46, 255}, edge{15, 17, 19, 255}, plastic{67, 71, 73, 255},
        screen{174, 190, 139, 255}, ink{30, 45, 29, 255}, dark{58, 78, 44, 255};
    auto box = [&](float x, float y, float w, float h, Color c) {
        scene.rectangles.push_back({x, y, w, h, c});
    };
    auto text = [&](float x, float y, float w, float size, const std::string &value,
                    Align align = Align::Left, Color c = Color{30, 45, 29, 255}) {
        scene.texts.push_back({x, y, w, size, locale::tr(value), c, align, false});
    };
    box(274, 0, 14, 67, edge);
    box(269, 7, 24, 11, plastic);
    box(6, 41, 320, 597, edge);
    box(0, 48, 332, 577, body);
    box(12, 47, 308, 584, plastic);
    box(18, 52, 296, 573, body);
    for (unsigned i = 0; i < 5; ++i)
        box(124 + i * 17.0f, 70, 10, 3, plastic);
    text(30, 87, 272, 15, "SA CELL", Align::Center, Color{190, 193, 185, 255});
    box(23, 111, 286, 274, edge);
    box(29, 117, 274, 262, screen);
    for (unsigned i = 0; i < 4; ++i)
        box(37 + i * 5.0f, 136 - i * 3.0f, 3, 3 + i * 3.0f, ink);
    text(70, 125, 188, 12, clock, Align::Center);
    box(274, 127, 19, 9, ink);
    box(293, 130, 2, 3, ink);
    box(276, 129, 14, 5, screen);
    box(37, 148, 258, 1, dark);
    std::string title = state.page == Page::Meeting    ? "PHONE_MEETINGS"
                        : state.page == Page::Meetings ? "PHONE_MEETINGS"
                        : state.page == Page::Settings ? "PHONE_SETTINGS"
                        : state.page == Page::Contact  ? "PHONE_CONTACT"
                                                       : "PHONE_CONTACTS";
    if (state.page == Page::Inbox)
        title = "PHONE_MESSAGES";
    if (state.page == Page::Messages)
        title = state.call ? "PHONE_CALL" : "PHONE_MESSAGES";
    if (state.page == Page::Compose)
        title = state.call ? "PHONE_CALL" : "PHONE_WRITE";
    if (state.page == Page::Actions)
        title = "PHONE_ACTIONS";
    if (state.page == Page::Controls)
        title = "PHONE_SET_CONTROLS";
    if (state.page == Page::Activities || state.page == Page::Outing)
        title = "PHONE_ACTIVITIES";
    if (state.page == Page::Inbox && socialworld::unread(registry))
        title = locale::tr(title) + " (" + std::to_string(socialworld::unread(registry)) + ")";
    const bool topLevel = state.page == Page::Contacts || state.page == Page::Inbox ||
                          state.page == Page::Meetings || state.page == Page::Settings;
    if (topLevel) // Arrows show that left/right switch between the main pages.
        title = u8"‹ " + locale::tr(title) + u8" ›";
    text(38, 157, 256, 18, title, Align::Center);
    box(37, 185, 258, 1, dark);
    auto ids = contacts(registry);
    if (!state.notice.empty()) {
        auto lines = locale::wrap(locale::tr(state.notice), 30);
        for (unsigned row = 0; row < 6 && row < lines.size(); ++row)
            text(41, 198 + row * 23.0f, 248, 15, lines[row]);
    } else if (state.page == Page::Inbox) {
        auto messages = inbox(registry);
        if (messages.empty())
            text(41, 227, 248, 15, "MAIL_EMPTY", Align::Center);
        unsigned start = state.cursor >= 3 ? state.cursor - 2 : 0;
        for (unsigned row = 0; row < 3 && start + row < messages.size(); ++row) {
            auto m = messages[start + row];
            float y = 198 + row * 45.0f;
            if (start + row == state.cursor)
                box(36, y - 1, 260, 43, dark);
            auto color = start + row == state.cursor ? screen : ink;
            text(44, y, 240, 14,
                 (m->unread ? "* " : "") + dialogue::displayName(registry.families[m->person - 1]),
                 Align::Left, color);
            auto wrapped = locale::wrap(letterText(registry, *m), 30);
            text(44, y + 21, 240, 11, wrapped.empty() ? "" : wrapped.front(), Align::Left, color);
        }
    } else if (state.page == Page::Messages) {
        auto messages = thread(registry, state.person);
        auto lines = chatLines(registry, state.person, state.typing);
        if (lines.empty())
            text(41, 227, 248, 15, "MAIL_EMPTY", Align::Center);
        // Newest at the bottom; Up scrolls back through older lines.
        const std::size_t rows = 8, shift = std::min<std::size_t>(state.textOffset, lines.size());
        std::size_t end = lines.size() - shift, start = end > rows ? end - rows : 0;
        for (std::size_t i = start; i < end; ++i) {
            const auto &line = lines[i];
            float y = 194 + (i - start) * 15.5f;
            if (line.spacer)
                continue;
            if (line.outgoing) {
                box(96, y - 1, 200, 15, dark);
                text(100, y, 192, 12, line.text, Align::Right, screen);
            } else {
                box(36, y - 1, 200, 15, Color{150, 166, 118, 255});
                text(40, y, 192, 12, line.text, Align::Left, ink);
            }
        }
        bool invite = !messages.empty() && !messages.back()->outgoing && messages.back()->kind == 2;
        text(41, 331, 248, 11, invite ? "MAIL_ACCEPT_INVITE" : "MAIL_REPLY_HINT", Align::Center);
    } else if (state.page == Page::Compose) {
        text(41, 197, 248, 13,
             state.person && state.person <= registry.families.size()
                 ? dialogue::displayName(registry.families[state.person - 1])
                 : "PHONE_UNKNOWN");
        unsigned start = state.cursor >= 3 ? state.cursor - 2 : 0;
        const auto available = phonedialogue::options(registry, state.person);
        for (unsigned row = 0; row < 3 && start + row < available.size(); ++row) {
            unsigned index = start + row, choice = available[index];
            float y = 222 + row * 31.0f;
            if (index == state.cursor)
                box(36, y - 1, 260, 30, dark);
            text(44, y, 244, 13, phonedialogue::menuLabel(registry, state.person, choice), Align::Left,
                 index == state.cursor ? screen : ink);
        }
        text(41, 331, 248, 11, "MAIL_SEND_HINT", Align::Center);
    } else if (state.page == Page::Actions || state.page == Page::Activities ||
               state.page == Page::Outing) {
        const char *actions[] = {"PHONE_MESSAGE", "PHONE_ACTIVITIES", "PHONE_INVITE",
                                 "PHONE_FAVOR", "ACT_STOP"};
        const char *activities[] = {"ACT_MEAL", "ACT_POOL", "ACT_DRIVE", "ACT_WALK"};
        const char *choices[] = {"ACT_ASK", "ACT_JOKE", "ACT_LISTEN"};
        unsigned count = state.page == Page::Actions      ? (state.outing == state.person ? 5u : 4u)
                         : state.page == Page::Activities ? 4u
                                                          : 3u;
        if (state.person && state.person <= registry.families.size())
            text(41, 195, 248, 12, dialogue::displayName(registry.families[state.person - 1]));
        for (unsigned row = 0; row < count; ++row) {
            float y = 219 + row * 24.0f;
            if (row == state.cursor)
                box(36, y - 1, 260, 25, dark);
            text(44, y, 244, 14,
                 state.page == Page::Actions      ? actions[row]
                 : state.page == Page::Activities ? activities[row]
                                                  : choices[row],
                 Align::Left, row == state.cursor ? screen : ink);
        }
    } else if (state.page == Page::Contacts) {
        if (ids.empty())
            text(41, 227, 248, 17, "PHONE_EMPTY", Align::Center);
        unsigned start = state.cursor >= 5 ? state.cursor - 4 : 0;
        for (unsigned row = 0; row < 5 && start + row < ids.size(); ++row) {
            unsigned index = start + row;
            const auto &f = registry.families[ids[index] - 1];
            float y = 197 + row * 28.0f;
            if (index == state.cursor)
                box(36, y - 1, 260, 26, dark);
            text(44, y, 244, 16, dialogue::displayName(f), Align::Left,
                 index == state.cursor ? screen : ink);
        }
    } else if (state.page == Page::Meetings) {
        if (registry.appointments.empty())
            text(41, 227, 248, 17, "PHONE_NO_MEETINGS", Align::Center);
        unsigned start = state.cursor >= 3 ? state.cursor - 2 : 0;
        for (unsigned row = 0; row < 3 && start + row < registry.appointments.size(); ++row) {
            unsigned index = start + row;
            const auto &a = registry.appointments[index];
            float y = 197 + row * 45.0f;
            if (index == state.cursor)
                box(36, y - 1, 260, 43, dark);
            auto color = index == state.cursor ? screen : ink;
            text(44, y, 244, 15, dialogue::displayName(registry.families[a.person - 1]),
                 Align::Left, color);
            std::string status =
                a.state == 1
                    ? locale::format("PHONE_IN_SECONDS", std::to_string((a.remaining + 999) / 1000))
                    : locale::tr(a.state == 2   ? "PHONE_ON_WAY"
                                 : a.state == 3 ? "PHONE_MET"
                                                : "PHONE_CANCELLED");
            text(44, y + 20, 244, 12, status, Align::Left, color);
        }
    } else if (state.page == Page::Meeting && state.cursor < registry.appointments.size()) {
        const auto &a = registry.appointments[state.cursor];
        text(41, 197, 248, 16, dialogue::displayName(registry.families[a.person - 1]));
        std::string place = meetings::venueAt(a.x, a.y);
        if (!place.empty())
            text(41, 210, 248, 12, place);
        text(41, 224, 248, 13, locale::format("PHONE_DISTANCE", std::to_string(state.distance)));
        text(41, 247, 248, 13,
             a.state == 1
                 ? locale::format("PHONE_IN_SECONDS", std::to_string((a.remaining + 999) / 1000))
             : a.state == 2
                 ? locale::format("PHONE_WAIT_LEFT", std::to_string((a.window + 999) / 1000))
                 : locale::tr(a.state == 3 ? "PHONE_MET" : "PHONE_CANCELLED"));
        if (meetings::active(a))
            text(41, 267, 248, 11, "PHONE_MEET_OUTSIDE");
        box(37, 279, 258, 1, dark);
        for (unsigned i = 0; i < 2; ++i) {
            float y = 289 + i * 27.0f;
            if (state.detail == i)
                box(37, y - 1, 258, 25, dark);
            text(44, y, 244, 14, i == 0 ? "PHONE_ROUTE" : "PHONE_CANCEL", Align::Left,
                 state.detail == i ? screen : ink);
        }
    } else if (state.page == Page::Controls) {
        // Six rows visible; the last row restores every default key.
        const unsigned rows = controls::Count + 1, start = state.cursor >= 6 ? state.cursor - 5 : 0;
        for (unsigned row = 0; row < 6 && start + row < rows; ++row) {
            unsigned index = start + row;
            float y = 196 + row * 21.0f;
            bool chosen = index == state.cursor;
            if (chosen)
                box(36, y - 2, 260, 20, dark);
            if (index == controls::Count) {
                text(44, y, 244, 12, "CTRL_RESET", Align::Left, chosen ? screen : ink);
                continue;
            }
            text(44, y, 150, 11, controls::label(index), Align::Left, chosen ? screen : ink);
            text(150, y, 140, 11,
                 chosen && state.capturing ? locale::tr("CTRL_PRESS") : controls::describe(index),
                 Align::Right, chosen ? screen : ink);
        }
        text(41, 331, 248, 11, state.capturing ? "CTRL_PRESS_HINT" : "CTRL_HINT", Align::Center);
    } else if (state.page == Page::Settings) {
        const std::string values[] = {locale::nativeName(locale::language),
                                      locale::tr(state.events ? "PHONE_ON" : "PHONE_OFF"),
                                      std::to_string(state.scale) + "%", u8"›"};
        const char *labels[] = {"PHONE_LANGUAGE", "PHONE_SET_EVENTS", "PHONE_SET_SCALE",
                                "PHONE_SET_CONTROLS"};
        for (unsigned row = 0; row < 4; ++row) {
            float y = 196 + row * 31.0f;
            bool chosen = state.cursor % 4 == row;
            if (chosen)
                box(36, y - 2, 260, 29, dark);
            text(44, y, 244, 11, labels[row], Align::Left, chosen ? screen : ink);
            text(44, y + 13, 244, 12, values[row], Align::Right, chosen ? screen : ink);
        }
        text(41, 331, 248, 11, "PHONE_SET_HINT", Align::Center);
    } else if (state.person && state.person <= registry.families.size()) {
        const auto &f = registry.families[state.person - 1];
        auto family = people::generate(f);
        text(41, 196, 248, 16, dialogue::displayName(f));
        const char *tabs[] = {"PHONE_INFO",     "PHONE_FAMILY",  "PHONE_MEMORIES", "PHONE_NETWORK",
                              "ROMANCE_FAMILY", "PHONE_ACTIONS", "PHONE_MESSAGES", "PHONE_ROUTINE"};
        text(41, 216, 248, 11, u8"‹ " + locale::tr(tabs[state.detail % 8]) + u8" ›", Align::Center);
        if (state.detail == 0) {
            text(41, 238, 248, 14,
                 dialogue::knows(f, people::Job) ? family[0].job : "PHONE_UNKNOWN_JOB");
            text(41, 262, 248, 14, f.female ? "PHONE_FEMALE" : "PHONE_MALE");
            text(41, 286, 248, 14, street::relationship(f));
        } else if (state.detail == 1) {
            const unsigned fields[] = {people::Mother, people::Father, people::Sibling,
                                       people::Child};
            const unsigned members[] = {1, 2, 4, 5};
            const char *labels[] = {"PHONE_MOTHER", "PHONE_FATHER", "PHONE_SIBLING", "PHONE_CHILD"};
            for (unsigned i = 0; i < 4; ++i)
                text(41, 235 + i * 20.0f, 248, 12,
                     locale::tr(labels[i]) + ": " +
                         (fields[i] == people::Child && !people::hasChild(f) &&
                                  f.social.childStage == 2
                              ? romance::childName(f)
                          : fields[i] == people::Child && !people::hasChild(f) &&
                                  dialogue::knows(f, fields[i])
                              ? locale::tr("PHONE_NO_CHILD")
                          : dialogue::knows(f, fields[i]) ? family[members[i]].name
                                                          : locale::tr("PHONE_UNKNOWN")));
        } else if (state.detail == 2) {
            text(41, 242, 248, 14,
                 locale::format("PHONE_ENCOUNTERS", std::to_string(f.social.encounters)));
            text(41, 270, 248, 13,
                 f.social.lastNote == 7    ? "PHONE_BAD_MEMORY"
                 : f.social.helped         ? "PHONE_BATTERY_MEMORY"
                 : f.social.lastNote == 10 ? "PHONE_MEETING_MEMORY"
                                           : "PHONE_TALK_MEMORY");
        } else if (state.detail == 4) {
            text(41, 239, 248, 13,
                 f.social.childStage == 2   ? romance::childName(f)
                 : f.social.childStage == 1 ? locale::tr("ROMANCE_EXPECTING")
                                            : locale::tr("ROMANCE_NO_CHILD"));
            text(41, 268, 248, 12,
                 locale::format("ROMANCE_HOME_COUNT", std::to_string(f.social.homeVisits)));
            text(41, 293, 248, 12,
                 f.social.childStage ? "ROMANCE_FAMILY_SAVED" : "ROMANCE_PLAN_HINT");
        } else if (state.detail == 5) {
            text(41, 245, 248, 14, "PHONE_ACTIONS_HINT", Align::Center);
        } else if (state.detail == 6) {
            text(41, 245, 248, 14, "PHONE_MESSAGES_HINT", Align::Center);
        } else if (state.detail == 7) {
            text(41, 238, 248, 12, "ROUTINE_WORK_HOURS");
            text(41, 264, 248, 12, "ROUTINE_SOCIAL_HOURS");
            text(41, 290, 248, 12, "ROUTINE_HOME_HOURS");
        } else {
            unsigned row = 0, skipped = 0;
            for (const auto &e : registry.connections) {
                unsigned id = e.a == f.id ? e.b : e.b == f.id ? e.a : 0;
                if (!id || row >= 3)
                    continue;
                if (skipped++ < state.networkOffset)
                    continue;
                const auto &other = registry.families[id - 1];
                float y = 239 + row * 25.0f;
                box(44, y + 3, 3, 15, dark);
                box(47, y + 9, 8, 1, dark);
                bool learned =
                    dialogue::knows(other, people::Name) ||
                    (other.origin == f.id && dialogue::knows(f, e.kind == 3   ? people::Mother
                                                                : e.kind == 4 ? people::Father
                                                                : e.kind == 5 ? people::Child
                                                                : e.kind == 6 ? people::Spouse
                                                                              : people::Sibling));
                std::string name =
                    learned ? people::generate(other)[0].name : locale::tr("PHONE_UNKNOWN");
                text(59, y, 230, 12,
                     locale::tr(e.kind == 6 && ((registry.families[e.a - 1].household == 3) ||
                                                (registry.families[e.b - 1].household == 3))
                                    ? "ROMANCE_EX"
                                    : network::label(e, f.id)) +
                         ": " + name);
                ++row;
            }
            if (!row)
                text(41, 252, 248, 13, "NET_EMPTY", Align::Center);
        }
        box(37, 323, 258, 1, dark);
        text(41, 331, 248, 14,
             state.detail == 3   ? "NET_SCROLL"
             : state.detail == 4 ? "ROMANCE_PLAN"
             : state.detail == 5 ? "PHONE_OPEN_ACTIONS"
             : state.detail == 6 ? "PHONE_MESSAGES"
             : state.detail == 7 ? "PHONE_ROUTINE_ROUTE"
                                 : "PHONE_OPEN_ACTIONS",
             Align::Center);
    }
    box(37, 352, 258, 1, dark);
    text(40, 358, 120, 13, state.notice.empty() ? "PHONE_SELECT" : "PHONE_OK");
    text(163, 358, 130, 13, "PHONE_BACK", Align::Right);
    box(36, 401, 86, 29, plastic);
    box(211, 401, 86, 29, plastic);
    text(38, 405, 82, 15, u8"—", Align::Center, Color{205, 209, 200, 255});
    text(213, 405, 82, 15, u8"—", Align::Center, Color{205, 209, 200, 255});
    box(135, 394, 63, 50, edge);
    text(139, 404, 55, 16, "OK", Align::Center, Color{210, 213, 203, 255});
    const char *digits[] = {"1",      "2 abc", "3 def",  "4 ghi", "5 jkl", "6 mno",
                            "7 pqrs", "8 tuv", "9 wxyz", "*",     "0",     "#"};
    for (unsigned i = 0; i < 12; ++i) {
        float x = 38 + (i % 3) * 88.0f, y = 457 + (i / 3) * 36.0f;
        box(x, y, 78, 28, plastic);
        text(x + 3, y + 5, 72, 15, digits[i], Align::Center, Color{221, 222, 209, 255});
    }
    box(126, 615, 80, 2, plastic);
    box(286, 93, 6, 4, Color{108, 204, 119, 255});
    return scene;
}
} // namespace phone
