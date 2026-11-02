    {
        assert(stories::visibleWeapon(22) && stories::visibleWeapon(4) && !stories::visibleWeapon(0) && !stories::visibleWeapon(43));
        people::Registry storyTown;
        for(unsigned n=0;n<5;++n) {
            auto &person=storyTown.create(false);person.social.journal=true;
            person.social.known=people::Name;person.social.trust=60;
            person.presence={17,4,100,100,10,0,100,0,true,true};
        }
        assert(stories::harm(storyTown,1,stories::Robbed,true));
        auto &victimStory=storyTown.families[0];
        assert(victimStory.story.grievance==45 && victimStory.story.fear==25 &&
            victimStory.story.intent==stories::chooseIntent(victimStory));
        assert(!stories::due(storyTown,victimStory));
        assert(!stories::harm(storyTown,1,stories::Hurt) && victimStory.story.grievance==45);
        storyTown.worldMinutes=181;int peaceCash=500;
        assert(std::string(stories::makePeace(storyTown,1,peaceCash,true,false))=="ST_CONFIRM_REQUIRED" && peaceCash==500);
        assert(std::string(stories::makePeace(storyTown,1,peaceCash,true,true))=="ST_PEACE_PARTIAL" && peaceCash==400);
        assert(!stories::peaceReady(storyTown,victimStory));
        assert(std::string(stories::makePeace(storyTown,1,peaceCash,true,true))=="ST_PEACE_NO" && peaceCash==400);
        assert(storyTown.honor==0 && victimStory.social.trust==60); // No honor/trust farming.
        storyTown.worldMinutes+=1440;
        assert(std::string(stories::makePeace(storyTown,1,peaceCash,false,true))=="ST_PEACE_ACCEPTED");
        assert(victimStory.story.grievance==8 && victimStory.story.intent==stories::None);
        victimStory.social.feud=1;victimStory.story.grievance=40;
        assert(!stories::peaceReady(storyTown,victimStory));victimStory.social.feud=0;
        assert(stories::harm(storyTown,2,stories::Hurt));
        const auto grievance=storyTown.families[1].story.grievance;
        assert(stories::harm(storyTown,2,stories::Helped) && storyTown.families[1].story.grievance==grievance-4);
        auto &dailyPerson=storyTown.families[2];
        assert(stories::tick(storyTown,3,false));const auto firstLife=dailyPerson.story.nextLife;
        assert(firstLife>storyTown.worldMinutes && dailyPerson.story.life==0);
        storyTown.worldMinutes=firstLife;
        assert(!stories::tick(storyTown,3,true) && dailyPerson.story.life==0); // Never interrupt an appointment.
        assert(stories::tick(storyTown,3,false) && dailyPerson.story.life>0);
        assert(stories::applyLife(storyTown,3,stories::Ill));
        assert(stories::unavailable(storyTown,dailyPerson) && !socialworld::spontaneous(storyTown,dailyPerson,720));
        assert(!bonds::ask(storyTown,3,3));
        storyTown.worldMinutes=dailyPerson.story.until;
        assert(stories::tick(storyTown,3,false) && dailyPerson.story.life==0);
        assert(!stories::unavailable(storyTown,dailyPerson));
        assert(stories::applyLife(storyTown,3,stories::Unemployed));
        storyTown.worldMinutes=dailyPerson.story.until;
        assert(stories::tick(storyTown,3,false) && dailyPerson.story.life==stories::Working);
        assert(network::link(storyTown,4,5,0));
        people::Point clue{};
        assert(stories::inquire(storyTown,4,5,clue)==stories::Lead::Unknown); // The registry is not a tracking service.
        assert(!stories::seen(storyTown,3,5,{100,100,10})); // No social link.
        assert(stories::seen(storyTown,4,5,{100,100,10}));
        assert(!stories::seen(storyTown,4,5,{999,999,10})); // At most one observation per game minute.
        storyTown.families[4].presence.x=2000; // Moving the archive does not silently update the report.
        assert(stories::inquire(storyTown,4,5,clue)==stories::Lead::Recent && clue.x==100);
        storyTown.families[4].social.cutOff=1;
        assert(stories::inquire(storyTown,4,5,clue)==stories::Lead::Refused);
        storyTown.families[4].social.cutOff=0;
        storyTown.families[3].daily.busyUntil=static_cast<unsigned>(storyTown.worldMinutes+10);
        assert(stories::inquire(storyTown,4,5,clue)==stories::Lead::Refused);
        storyTown.families[3].daily.busyUntil=0;
        storyTown.families[3].social.trust=0;
        assert(stories::inquire(storyTown,4,5,clue)==stories::Lead::Refused);
        storyTown.families[3].social.trust=60;storyTown.families[4].presence.alive=false;
        assert(stories::inquire(storyTown,4,5,clue)==stories::Lead::Unknown); // No remotely learned death announcement.
        storyTown.families[4].presence.alive=true;storyTown.worldMinutes+=1441;
        assert(stories::inquire(storyTown,4,5,clue)==stories::Lead::Unknown);
        std::stringstream storyArchive;storyTown.write(storyArchive);
        auto restoredStory=people::Registry::read(storyArchive);std::stringstream secondStory;restoredStory.write(secondStory);
        assert(secondStory.str()==storyArchive.str() && restoredStory.sightings.size()==1 && !restoredStory.news.empty());
        auto legacyStory=storyArchive.str();legacyStory.replace(0,11,"SAPEOPLE 17");legacyStory.resize(legacyStory.find("STORIES"));
        std::stringstream oldStories(legacyStory);const auto migratedStory=people::Registry::read(oldStories);
        assert(migratedStory.families.size()==5 && migratedStory.news.empty() && migratedStory.families[0].story.empty());
        auto badStory=storyArchive.str();const auto storyHeader=badStory.find("STORIES");
        badStory.resize(storyHeader);badStory+="STORIES 1 1 0 0\n99 0 0 0 0 0 0 0 0 0 0 0\n";
        bool badStoryRejected=false;try {std::stringstream bad(badStory);people::Registry::read(bad);}catch(const std::exception &){badStoryRejected=true;}
        assert(badStoryRejected);
        people::Registry newsTown;newsTown.create(false);newsTown.create(false);
        assert(stories::publish(newsTown,1,stories::Hurt) && stories::news(newsTown).empty());
        newsTown.families[1].social.journal=true;newsTown.families[1].social.known=people::Name;
        assert(!stories::publish(newsTown,1,stories::Hurt,2) && stories::news(newsTown).size()==1);
        for(unsigned n=0;n<250;++n){newsTown.worldMinutes+=61;stories::publish(newsTown,1,stories::Hurt,2);}
        assert(newsTown.news.size()==192 && newsTown.validStories(2));
        people::Registry replyTown;auto &replyPerson=replyTown.create(false);
        replyPerson.social.journal=true;replyPerson.social.known=people::Name;replyPerson.social.trust=60;
        replyPerson.presence={17,4,100,100,10,0,100,0,true,true};
        replyTown.post(1,"ST_MAIL_SICK");
        assert(phonedialogue::context(replyTown,1)==phonedialogue::Context::Health && phonedialogue::options(replyTown,1)[0]==11);
        const char *supportReply=nullptr;
        assert(std::string(phonedialogue::send(replyTown,1,11,false,700,&supportReply)).empty());
        assert(supportReply && std::string(supportReply)=="ST_SUPPORT_ACK");
        assert(phone::letterText(replyTown,replyTown.letters.back()).find("ST_")==std::string::npos);
        assert(stories::harm(replyTown,1,stories::Insulted));
        assert(stories::harm(replyTown,1,stories::Hurt) && replyPerson.story.lastDeed==stories::Hurt);
        assert(stories::harm(replyTown,1,stories::Robbed) && replyPerson.story.lastDeed==stories::Robbed);
        assert(!stories::harm(replyTown,1,stories::Robbed));
        const auto protectedId=newsTown.recycle([](unsigned){return false;});
        assert(!protectedId); // Event subjects and informants cannot be recycled.
        std::cout<<"PASS: personal motives, bounded harm, confirmed compensation, no honor farming, life cycles, occupied actors, witnessed-only clues, stale information, V17 migration and bounded persistent news.\n";
        for(unsigned lang=0;lang<locale::LanguageCount;++lang) {
            locale::language=static_cast<locale::Language>(lang);
            phone::State storyScreen;storyScreen.person=3;
            for(auto page:{phone::Page::News,phone::Page::History,phone::Page::Stories,phone::Page::Peace,phone::Page::PeaceConfirm,phone::Page::AskAbout}) {
                storyScreen.page=page;const auto scene=phone::compose(storyTown,storyScreen,"14:35");
                for(const auto &line:scene.texts)assert(line.value.find("ST_")==std::string::npos);
            }
        }
        locale::language=locale::Language::Turkish;
    }
