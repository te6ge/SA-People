    {
        npcvoice::Chooser voices;
        std::set<unsigned> profiles;
        for (unsigned voiceId=1;voiceId<=128;++voiceId) {
            for (bool woman : {false,true}) {
                people::Family voicePerson{voiceId,woman,0};
                const auto profile=npcvoice::bank(voicePerson);
                const auto number=static_cast<unsigned>(profile);
                assert(npcvoice::femaleBank(profile) == woman);
                profiles.insert(number);
                voicePerson.presence.model=woman?41:50;
                voicePerson.social.trust=70;
                voicePerson.social.known=people::Name;
                voicePerson.social.journal=true;
                voicePerson.social.romance=3;
                assert(npcvoice::bank(voicePerson)==profile);
                auto copyVoice=voicePerson; copyVoice.presence.model=woman?13:21;
                assert(npcvoice::bank(copyVoice)==profile);
                assert(voices.choose(voicePerson,npcvoice::Cue::Hello,voiceId));
                assert(!voices.choose(voicePerson,npcvoice::Cue::None,voiceId));
                assert(!voices.choose(voicePerson,npcvoice::Cue::Auto,voiceId));
            }
        }
        assert(profiles.size()==8);
        std::set<std::wstring> recordings;
        unsigned voicedCount=0;
        for (const auto &recording : npcvoice::catalog) {
            assert(recordings.insert(recording.file).second);
            assert(recording.durationMs>100 && recording.durationMs<15000);
            if (audioAvailable) assert(std::filesystem::is_regular_file(audiofiles::resolve(std::filesystem::path("audio")/recording.file)));
            people::Family recordingPerson{1,npcvoice::femaleBank(recording.bank),0};
            while(npcvoice::bank(recordingPerson)!=recording.bank) ++recordingPerson.id;
            recordingPerson.social.known=people::Name;
            recordingPerson.social.romance=3;
            bool reached=false;
            for(unsigned attempt=0;attempt<20;++attempt) {
                const auto *chosen=voices.choose(recordingPerson,recording.cue,attempt);
                assert(chosen);
                reached=reached || std::wcscmp(chosen,recording.file)==0;
                assert(npcvoice::duration(chosen)>0);
            }
            assert(reached); ++voicedCount;
        }
        assert(voicedCount==118);
        people::Family unknownVoice{900,true,0};
        assert(!npcvoice::allowed(unknownVoice,npcvoice::Cue::FriendHello));
        assert(!npcvoice::allowed(unknownVoice,npcvoice::Cue::PartnerHello));
        assert(!npcvoice::allowed(unknownVoice,npcvoice::Cue::RomanceHurt));
        assert(!voices.choose(unknownVoice,npcvoice::Cue::RobPaid,1));
        assert(npcvoice::forLine("MAIL_GOOD_OUTING")==npcvoice::Cue::None);
        assert(npcvoice::forLine("AMBIENT_RAIN_0")==npcvoice::Cue::None);
        assert(npcvoice::forLine("By the way, my name is Ana.")==npcvoice::Cue::None);
        assert(npcvoice::forLine("LIFE_ASK_RIDE")==npcvoice::Cue::RideRequest);
        assert(npcvoice::forLine("AMBIENT_HONOR_HIGH")==npcvoice::Cue::HonorHigh);
        street::Reply disclosure; disclosure.reaction=dialogue::Reaction::Friendly;
        unknownVoice.social.known=people::Name; // Learning their name now must not imply an old friendship.
        assert(npcvoice::streetReply(unknownVoice,street::Action::Greet,disclosure,1,false)==npcvoice::Cue::Hello);
        assert(npcvoice::streetReply(unknownVoice,street::Action::Greet,disclosure,1,true)==npcvoice::Cue::FriendHello);
        assert(npcvoice::streetReply(unknownVoice,street::Action::Greet,disclosure,3,false)==npcvoice::Cue::None);
        disclosure.newJournal=true;
        assert(npcvoice::streetReply(unknownVoice,street::Action::Greet,disclosure,3,false)==npcvoice::Cue::Contact);

        assert(npcvoice::forLine("BOUND_DRIVE_WARN")==npcvoice::Cue::DriveWarn);
        assert(npcvoice::forLine("BOUND_DRIVE_END")==npcvoice::Cue::DriveEnd);
        assert(npcvoice::forLine("LIFE_DROP_OFF")==npcvoice::Cue::DropOff);
        assert(npcvoice::forLine("COMMUNITY_WARM")==npcvoice::Cue::HonorHigh);
        npcvoice::PendingReply pendingVoice;
        pendingVoice.queue(L"first.wav",1,100);
        assert(pendingVoice.take(200,false).empty());
        assert(pendingVoice.take(500,true).empty()); // CJ is still speaking.
        assert(pendingVoice.take(1000,false)==L"first.wav");
        assert(pendingVoice.take(1001,false).empty());
        pendingVoice.queue(L"stale.wav",1,100);
        assert(pendingVoice.take(5600,false).empty() && pendingVoice.file.empty());
        pendingVoice.queue(L"old.wav",1,100);
        pendingVoice.queue(L"new.wav",2,200);
        assert(pendingVoice.owner==2 && pendingVoice.take(500,false)==L"new.wav");
        pendingVoice.queue(L"old.wav",1,100);
        pendingVoice.queue(nullptr,2,200);
        assert(pendingVoice.take(500,false).empty());
        pendingVoice.queue(L"wrap.wav",1,0xffffff00u);
        assert(pendingVoice.take(0xfffffff0u,false).empty());
        assert(pendingVoice.take(50,false)==L"wrap.wav");
        std::cout << "PASS: 118 NPC recordings, eight stable gendered voices, no invented disclosures, "
                     "partial-bank subtitle fallback, non-overlapping reply scheduling and expiry.\n";
    }
