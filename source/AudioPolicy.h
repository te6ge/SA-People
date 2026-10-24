#pragma once
#include "Street.h"
#include <map>
namespace socialaudio {
enum class Context {
    Hello,
    FriendHello,
    Flirt,
    Provoke,
    CopTaunt,
    Goodbye,
    Reply,
    Question,
    Surprise,
    Joke,
    RoughDay,
    SharedWalk,
    Reassure,
    ReassureFemale,
    SanFierro,
    Defuse,
    Fear,
    Repair,
    Help,
    RobberyPaid,
    RobberyRefused,
    Fight,
    Arrest,
    HelloFemale,
    GoodbyeFemale,
    Ride,
    Count
};
inline std::vector<const wchar_t *> clips(Context context) {
    switch (context) {
    case Context::Hello:
        return {L"cjHello.wav", L"cjAlrightHommie.wav", L"cjWhatsupPlayer.mp3", L"cjHeyYou!.wav"};
    case Context::HelloFemale:
        return {L"cjHeyMadam.wav", L"cjWassupLady.wav"};
    case Context::GoodbyeFemale:
        return {L"cjISeeYouAroundLady.wav", L"cjYoSeeYouSoonLady.wav", L"cjImmaSeeYouLater.wav"};
    case Context::Ride:
        return {L"cjComeinMyRide.wav", L"cjHeyHopOnMyWheels.wav", L"cjGonnaRideForYou.wav"};
    case Context::FriendHello:
        return {L"cjAlrightHommie.wav", L"cjHello.wav", L"cjYeaaahhhh.wav",
                L"cjWhatsupPlayer.mp3", L"cjYoHandsomeBro.wav"};
    case Context::Flirt:
        return {L"cjHeyBabyYouWantCompany.wav", L"cjHeyBeatiful.mp3",
                L"cjHeyBaby,OfCourseImGonnaComeWithYou.wav"};
    case Context::Provoke:
        return {L"cjSuchASSSSS(ragebaiting).wav",
                L"cjManImDisappointingYouIToughtYouWasaGangsta.wav", L"cjAaahhNotYourShitAgain.wav",
                L"cjLaughandsaysWhatever.wav", L"cjOhSoYouThinkIamAPunkDoYou.mp3",
                L"cjIBlameYouFoThisShit.mp3", L"cjYouHaventFunYetFool.mp3",
                L"cjEatMyAssBitch!.wav", L"cjGetLost,Bitch!.wav", L"cjNo,GetAwayFromMeBitch!.wav",
                L"cjYouAreUselessDog.wav", L"cjGimmeSomeSpaceWillYou.wav", L"cjYoStayAwayDog.wav"};
    case Context::Fight:
        return {L"cjAyThisAintNothingPersonal.mp3", L"cjComeonAssholeThenFight.mp3",
                L"cjYouHaventFunYetFool.mp3", L"cjImGonnaSendYouToYourGod.wav"};
    case Context::Arrest:
        return {L"cjOhhhNo.mp3", L"cjThisIsInsanity.mp3", L"cjHellNoooooo.wav"};
    case Context::CopTaunt:
        return {L"cjAyPigsAintSupoossedToFly.wav", L"cjSuchASSSSS(ragebaiting).wav"};
    case Context::Goodbye:
        return {L"cjOkayLaterThen.wav", L"cjImmaSeeYouLater.wav", L"cjISeeYouLaterHommie.wav",
                L"cjYeahForSureIseeYouAround.wav", L"cjYeahhYouTooHommie.wav", L"cjAyPeaceMan.wav",
                L"cjSeeYouAroundBlud.wav", L"cjSeeYouAroundHommie.wav", L"cjSeeYouLaterDog.wav",
                L"cjHey,TakeCareMan.wav"};
    case Context::Reply:
        return {L"cjOkay.wav", L"cjYeahForSure.wav", L"cjYeaaahhhh.wav"};
    case Context::Question:
        return {L"cjYeahWhat(questionlike).wav", L"cjWhatareyoutalkingabout.wav",
                L"cjIdontknowthatswhyimaskingyou.wav"};
    case Context::Surprise:
        return {L"cjFORREAL(surprised).wav", L"cjDamnWhatisThat.wav", L"cjOhman.wav",
                L"cjThisIsInsanity.mp3"};
    case Context::Joke:
        return {L"cjLaughs.wav", L"cjFORREAL(surprised).wav", L"cjLaughShorts.wav"};
    case Context::RoughDay:
        return {L"cjWasisalwayslikethis.wav", L"cjOhman.wav"};
    case Context::SharedWalk:
        return {L"cjThatwhatIwasdoing.wav", L"cjYeahForSure.wav"};
    case Context::Reassure:
        return {L"cjComeonManWeBrothersYouAndMe.wav", L"cjYouCanAskmeAnything.wav"};
    case Context::ReassureFemale:
        return {L"cjYouCanAskmeAnything.wav", L"cjYeahForSure.wav"};
    case Context::SanFierro:
        return {L"cjsaysSanfierroLikeaQuestion.wav"};
    case Context::Defuse:
        return {L"cjYouAretheBoss.wav", L"cjOkay.wav", L"cjYeahForSure.wav"};
    case Context::Fear:
        return {L"cjYeahh(fearful).wav", L"cjYouAretheBoss.wav", L"cjImScared.mp3"};
    case Context::Repair:
        return {L"cjFinally.wav", L"cjWeClean.wav", L"cjYeaaahhhh.wav"};
    case Context::Help:
        return {L"cjWhatstheproblem.wav", L"cjYeahWhat(questionlike).wav",
                L"cjYouCanAskmeAnything.wav"};
    case Context::RobberyPaid:
        return {L"cjYeahWhoNegativeNowBitch.wav", L"cjLaughandsaysWhatever.wav",
                L"cjYesIamAManiacWithaGun.mp3", L"cjNowWhoTheBitch,Bitch.mp3",
                L"cjGimmeYourWallet.wav", L"cjGiveMeTheMoney,Baby.wav",
                L"cjGiveMeYourMoney,Now!.wav"};
    case Context::RobberyRefused:
        return {L"cjAaahhNotYourShitAgain.wav", L"cjOhman.wav", L"cjLaughandsaysWhatever.wav"};
    default:
        return {L"cjOkay.wav"};
    }
}
inline Context context(street::Action action, unsigned turn, bool friendBefore,
                       const street::Reply &reply, int pedType = 4, bool female = false,
                       int trust = 0) {
    if (action == street::Action::Defuse)
        return Context::Defuse;
    if (action == street::Action::Provoke) {
        if (reply.arrest)
            return Context::Arrest;
        if (reply.reaction == dialogue::Reaction::Fight)
            return Context::Fight;
        return pedType == 6 ? Context::CopTaunt : Context::Provoke;
    }
    if (reply.leaves)
        return female ? Context::GoodbyeFemale : Context::Goodbye;
    if (reply.reaction != dialogue::Reaction::Friendly)
        return Context::Reply;
    if (turn == 1)
        return pedType == 22 && trust >= 25 ? Context::Flirt
               : friendBefore               ? Context::FriendHello
               : female                     ? Context::HelloFemale
                                            : Context::Hello;
    switch (reply.cue) {
    case street::Cue::Question:
        return Context::Question;
    case street::Cue::Joke:
        return Context::Joke;
    case street::Cue::Surprise:
        return Context::Surprise;
    case street::Cue::RoughDay:
        return Context::RoughDay;
    case street::Cue::SharedWalk:
        return Context::SharedWalk;
    case street::Cue::Reassure:
        return female ? Context::ReassureFemale : Context::Reassure;
    case street::Cue::SanFierro:
        return Context::SanFierro;
    default:
        return Context::Reply;
    }
}
// Language-specific easter eggs: a local curse for the interface language (none for English).
inline const wchar_t *localizedInsult(unsigned language) {
    switch (language) {
    case 0:
        return L"cjNeettinGülüm.wav";
    case 1:
        return L"cjOyBlyat.wav";
    case 2:
        return L"cjİspanyolcaKüfür.wav";
    default:
        return nullptr;
    }
}
inline const wchar_t *rickroll() {
    return L"cjRickroll(nevergonnagiveyouup).wav";
}
// Shuffle bags per context; exhaust alternatives before repeating, including across cycles.
class Chooser {
    struct Bag {
        std::vector<const wchar_t *> order;
        std::size_t index = 0;
        std::wstring last;
        unsigned cycles = 0;
    };
    std::map<Context, Bag> bags;

  public:
    const wchar_t *choose(Context context, unsigned seed) {
        auto &bag = bags[context];
        if (bag.index >= bag.order.size()) {
            bag.order = clips(context);
            bag.index = 0;
            for (std::size_t i = bag.order.size(); i > 1; --i) {
                seed = people::mix(seed + bag.cycles + static_cast<unsigned>(i));
                std::swap(bag.order[i - 1], bag.order[seed % i]);
            }
            if (bag.order.size() > 1 && bag.last == bag.order[0])
                std::swap(bag.order[0], bag.order[1]);
            ++bag.cycles;
        }
        auto result = bag.order[bag.index++];
        bag.last = result;
        return result;
    }
};
inline const wchar_t *line(street::Action action, unsigned turn, bool friendBefore,
                           const street::Reply &reply, unsigned variation) {
    auto pool = clips(context(action, turn, friendBefore, reply));
    return pool[variation % pool.size()];
}
inline int honorCue(street::Action action, int trustBefore, int trustAfter,
                    const street::Reply &reply) {
    (void)trustBefore;
    (void)trustAfter;
    // Provocations are accounted for by the three-person batch in the native adapter.
    return action == street::Action::Greet && (reply.newJournal || reply.newFriend) ? 1 : 0;
}
} // namespace socialaudio
