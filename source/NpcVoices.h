#pragma once
#include "Street.h"
#include <map>
#include <utility>
namespace npcvoice {
enum class Bank : unsigned { F1, F2, F3, M1, M2, M3, Count };
enum class Cue : unsigned {
    Auto, None, Hello, FriendHello, Flirt, PartnerHello, Busy, Reject, Angry, Fight,
    Thank, Help, RideRequest, Contact, HonorHigh, HonorLow, Gossip, MeetingStart,
    RomanceHurt, Goodbye, Calm, Count
};
struct Clip { Bank bank; Cue cue; const wchar_t *file; unsigned durationMs; };
#include "NpcVoiceCatalog.inc"
inline unsigned duration(const wchar_t *file) {
    if (file) for (const auto &clip : catalog)
        if (std::wcscmp(file,clip.file)==0) return clip.durationMs;
    return 0;
}
inline Bank bank(const people::Family &person) {
    // Identity-based, not skin or nationality-based. No save-format change is needed.
    return static_cast<Bank>((person.female ? 0u : 3u) + people::mix(person.id ^ 0x564F4943u) % 3);
}
inline bool knowsCJ(const people::Family &person) {
    return (person.social.known & people::Name) || person.social.journal ||
           person.social.helped || person.daily.outings || person.presence.type == 8;
}
inline bool allowed(const people::Family &person, Cue cue) {
    if (cue == Cue::PartnerHello || cue == Cue::Flirt || cue == Cue::RomanceHurt)
        return person.social.romance >= 2;
    if (cue == Cue::FriendHello || cue == Cue::MeetingStart || cue == Cue::Contact)
        return knowsCJ(person);
    return cue != Cue::Auto && cue != Cue::None;
}
inline Cue forLine(const std::string &line) {
    if (line.rfind("AMBIENT_HELLO",0)==0 || line.rfind("INSIDE_HELLO_",0)==0) return Cue::Hello;
    if (line.rfind("AMBIENT_KNOWN",0)==0 || line.rfind("RECOG_APPROACH_",0)==0 ||
        line.rfind("AMBIENT_GROVE_",0)==0) return line == "AMBIENT_GROVE_WARY" ? Cue::HonorLow : Cue::FriendHello;
    if (line == "AMBIENT_HONOR_HIGH" || line == "ACT_HONOR_WARM") return Cue::HonorHigh;
    if (line == "AMBIENT_HONOR_LOW" || line == "ACT_HONOR_WARY") return Cue::HonorLow;
    if (line == "RECOG_AVOID") return Cue::Reject;
    if (line == "LIFE_ASK_RIDE") return Cue::RideRequest;
    if (line == "LIFE_ASK_BEGGAR" || line == "LIFE_ASK_ADDICT") return Cue::Help;
    if (line.rfind("LIFE_THANK_",0)==0 || line == "LIFE_RIDE_THANK_KNOWN") return Cue::Thank;
    if (line == "ACT_MEAL_START" || line == "ACT_POOL_START" || line == "ACT_DRIVE_START" ||
        line == "ACT_WALK_START") return Cue::MeetingStart;
    if (line == "ACT_ABORT_ARGUMENT" || line == "ACT_REPLY_BAD_JOKE") return Cue::Angry;
    if (line == "ACT_GOOD_END") return Cue::Goodbye;
    if (line == "ACT_BAD_END") return Cue::RomanceHurt;
    if (line == "ACT_CHAT_8") return Cue::Gossip;
    if (line == "FEUD_LINE") return Cue::Fight;
    // Do not put generic greetings over disclosures, weather, named relatives or phone texts.
    return Cue::None;
}
inline Cue streetReply(const people::Family &person, street::Action action,
                       const street::Reply &reply, unsigned turn, bool friendBefore) {
    if (action == street::Action::Provoke)
        return reply.reaction == dialogue::Reaction::Fight ? Cue::Fight : Cue::Angry;
    if (action == street::Action::Defuse) return reply.calmed ? Cue::Calm : Cue::Reject;
    if (reply.leaves)
        return reply.reaction == dialogue::Reaction::Angry ? Cue::Reject : Cue::Busy;
    if (reply.newJournal) return Cue::Contact;
    if (reply.reaction == dialogue::Reaction::Friendly && turn == 1)
        return person.social.romance >= 2 ? Cue::PartnerHello :
               friendBefore ? Cue::FriendHello : Cue::Hello;
    return Cue::None;
}
class Chooser {
    std::map<std::pair<Bank,Cue>, socialprogress::LineDeck> decks;
  public:
    const wchar_t *choose(const people::Family &person, Cue cue, unsigned seed) {
        if (!allowed(person,cue)) return nullptr;
        const auto profile = bank(person);
        std::vector<const wchar_t *> options;
        for (const auto &clip : catalog)
            if (clip.bank == profile && clip.cue == cue) options.push_back(clip.file);
        if (options.empty() && cue == Cue::PartnerHello)
            return choose(person,Cue::Flirt,seed);
        if (options.empty() && (cue == Cue::Fight || cue == Cue::Angry))
            return choose(person,Cue::Reject,seed);
        // A partial voice pack falls back to subtitles, never a different actor.
        if (options.empty()) return nullptr;
        return options[decks[{profile,cue}].take(static_cast<unsigned>(options.size()),seed)];
    }
};
struct PendingReply {
    std::wstring file;
    unsigned owner = 0;
    std::uint32_t ready = 0, expires = 0;
    static bool due(std::uint32_t now, std::uint32_t time) { return now-time < 0x80000000u; }
    void queue(const wchar_t *name, unsigned person, std::uint32_t now) {
        file = name ? name : L"";
        owner = person; ready = now+300; expires = now+5500;
    }
    std::wstring take(std::uint32_t now, bool busy) {
        if (file.empty()) return {};
        if (due(now,expires)) { *this={}; return {}; }
        if (busy || !due(now,ready)) return {};
        auto result=std::move(file); *this={}; return result;
    }
};
} // namespace npcvoice
