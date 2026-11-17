    {
        people::Registry careRegistry;
        careRegistry.create(false);careRegistry.create(true);careRegistry.create(false);
        careRegistry.worldMinutes=300;
        for(auto &carePerson:careRegistry.families){carePerson.presence={7,4,1,2,3,0,100,0,true,true};}
        auto &carePerson=careRegistry.families[0];carePerson.social.known=people::Name;carePerson.social.journal=true;carePerson.social.trust=45;
        int careCash=24;
        carePerson.story.life=stories::Ill;carePerson.story.until=careRegistry.worldMinutes+1440;
        assert(std::string(contactcare::apply(careRegistry,1,contactcare::Medicine,careCash))=="CARE_NO_MONEY");
        assert(careRegistry.letters.empty() && carePerson.story.until==1740 && careCash==24);
        careCash=100;
        assert(std::string(contactcare::apply(careRegistry,1,contactcare::Medicine,careCash))=="CARE_SENT");
        assert(careCash==75 && carePerson.story.until==1380 && carePerson.social.trust==47 && careRegistry.honor==0);
        assert(std::string(contactcare::apply(careRegistry,1,contactcare::Medicine,careCash))=="CARE_WAIT" && careCash==75);
        const auto careBefore=carePerson.story.until;
        assert(std::string(contactcare::apply(careRegistry,1,contactcare::JobLead,careCash))=="CARE_NOT_JOB" && careBefore==carePerson.story.until);
        assert(std::string(contactcare::apply(careRegistry,1,contactcare::CheckIn,careCash))=="CARE_SENT");
        assert(careRegistry.letters.back().body=="CARE_REPLY_SICK");
        std::stringstream careArchive;careRegistry.write(careArchive);auto careLoaded=people::Registry::read(careArchive);
        assert(contactcare::recent(careLoaded,1,contactcare::Medicine,1440));
        assert(std::string(contactcare::apply(careLoaded,1,contactcare::Medicine,careCash))=="CARE_WAIT");
        careRegistry.connections.push_back({1,2,0});careRegistry.connections.push_back({1,3,2});
        assert(contactcare::recommendation(careRegistry,1)==2);
        assert(std::string(contactcare::apply(careRegistry,1,contactcare::Introduction,careCash))=="CARE_SENT");
        assert((careRegistry.families[1].social.known&people::Name) && !careRegistry.families[1].social.journal && careRegistry.letters.back().source==2);
        assert(contactcare::recommendation(careRegistry,1)==0); // Enemy link is excluded, no invented identities.
        carePerson.social.cutOff=1;
        const auto careLetterCount=careRegistry.letters.size();
        assert(std::string(contactcare::apply(careRegistry,1,contactcare::CheckIn,careCash))=="CARE_REFUSED" && careRegistry.letters.size()==careLetterCount);
        carePerson.social.cutOff=0;careRegistry.worldMinutes+=1440;carePerson.story.life=stories::Unemployed;carePerson.story.until=careRegistry.worldMinutes+100;
        assert(std::string(contactcare::apply(careRegistry,1,contactcare::JobLead,careCash))=="CARE_SENT");
        assert(carePerson.story.until==careRegistry.worldMinutes+100 && careCash==75); // Never lengthen an already short wait.
        careRegistry.worldMinutes+=1440;carePerson.story.life=stories::Ill;carePerson.story.until=careRegistry.worldMinutes+300;
        assert(std::string(contactcare::apply(careRegistry,1,contactcare::Medicine,careCash))=="CARE_SENT");
        assert(carePerson.story.until==careRegistry.worldMinutes+60 && careCash==50);
        assert(std::string(contactcare::apply(careRegistry,0,contactcare::CheckIn,careCash))=="CARE_REFUSED");
        phone::State careState;careState.page=phone::Page::Care;careState.person=1;
        const auto careScene=phone::compose(careRegistry,careState,"12:00");assert(!careScene.texts.empty());
        std::cout<<"PASS: contextual contact care, money/precondition atomicity, careArchive cooldowns, bounded recovery, real introductions without phone-number consent or enemy recommendations.\n";
    }
