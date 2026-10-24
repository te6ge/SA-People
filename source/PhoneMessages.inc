// Replies arrive a few seconds after sending, with a "typing" line in the chat meanwhile.
struct PendingReply {
    unsigned person;
    const char *reply;
    DWORD due;
    unsigned choice;
};
std::vector<PendingReply> pendingReplies;
bool replyPending(unsigned id) {
    for (const auto &p : pendingReplies)
        if (p.person == id && id && id <= registry.families.size() &&
            phonedialogue::answers(registry.families[id - 1], p.choice))
            return true;
    return false;
}
void deliverReplies() {
    DWORD now = GetTickCount();
    for (auto it = pendingReplies.begin(); it != pendingReplies.end();) {
        const unsigned id = it->person;
        // A friendly reply queued before a fight must not arrive after the argument.
        if (!id || id > registry.families.size() ||
            !phonedialogue::answers(registry.families[id - 1], it->choice)) {
            it = pendingReplies.erase(it);
            panelInvalid = true;
            continue;
        }
        if (!due(now, it->due)) {
            ++it;
            continue;
        }
        registry.post(id, it->reply, false, 0, 0, false);
        bool reading = panelOpen && phoneState.page == phone::Page::Messages && phoneState.person == id;
        if (reading) {
            for (auto &m : registry.letters)
                if (m.person == id && !m.outgoing)
                    m.unread = false;
        } else if (id <= registry.families.size()) {
            help(locale::format("MAIL_FROM", dialogue::displayName(registry.families[id - 1])));
        }
        it = pendingReplies.erase(it);
        dirty = true;
        panelInvalid = true;
        save();
    }
}
void sendPhoneChoice(unsigned id, unsigned choice, bool) {
    unsigned minute = at<Byte>(0xB70153) * 60u + at<Byte>(0xB70152);
    const char *reply = nullptr;
    phoneState.notice = phonedialogue::send(registry, id, choice, false, minute, &reply);
    if (phoneState.notice.empty() && reply)
        pendingReplies.push_back({id, reply, GetTickCount() + 2500 + people::mix(id + minute) % 3500, choice});
    phoneState.textOffset = 0;
    for (auto &m : registry.letters)
        if (m.person == id && !m.outgoing)
            m.unread = false;
    dirty = true;
    panelInvalid = true;
    save();
}
