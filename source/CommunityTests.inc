    {
        people::Registry town;
        for(unsigned i=0;i<8;++i) {
            auto &p=town.create(false);p.presence={50,4,0,0,13,0,100,0,true,true};
        }
        assert(network::link(town,1,2,0));assert(network::link(town,2,3,0));
        assert(network::link(town,3,4,0));assert(network::link(town,2,5,2));
        town.worldMinutes=600;
        const auto crime=community::incident(town);
        assert(community::remember(town,1,8,community::Deed::Robbery,crime));
        const int firstOpinion=town.families[0].community.opinion;
        assert(firstOpinion<0 && !community::remember(town,1,8,community::Deed::Robbery,crime));
        assert(town.families[0].community.opinion==firstOpinion);
        assert(town.families[1].community.opinion==0 && town.families[5].community.opinion==0);
        assert(community::gossip(town,1,2));
        assert(town.families[1].community.teller==1 && town.families[1].community.hops==1);
        assert(!community::gossip(town,2,5)); // Enemies do not exchange credible rumors.
        assert(community::gossip(town,2,3));
        assert(!community::gossip(town,3,4)); // Two hops, no endless worldwide spread.
        assert(!community::gossip(town,1,2));
        assert(!community::gossip(town,1,6));
        assert(town.families[5].community.opinion==0);
        auto goodDeed=community::incident(town);
        assert(community::remember(town,1,8,community::Deed::Help,goodDeed));
        assert(!community::remember(town,1,8,community::Deed::Killing,crime));
        town.worldMinutes+=3000;
        assert(!community::gossip(town,1,2));
        assert(network::link(town,6,8,1));
        assert(community::remember(town,6,8,community::Deed::Killing,community::incident(town)));
        assert(town.families[5].social.feud && !town.families[6].social.feud);
        for(unsigned i=0;i<3;++i) {
            const auto reason=community::request(town,7);
            if(i==2) assert(reason && std::string(reason)=="BOUND_TOO_MUCH");
        }
        assert(town.families[6].community.strain>=10);
        const auto previousRequests=town.families[6].community.requests;
        town.worldMinutes+=120;
        (void)community::request(town,7);
        assert(town.families[6].community.requests==1 && previousRequests==3);
        auto &rider=town.families[6];
        assert(!community::unsafeRide(rider,0,false,10000));
        assert(!community::unsafeRide(rider,100,false,3000));
        for(unsigned i=0;i<3;++i) assert(community::unsafeRide(rider,100,false,5000));
        assert(rider.community.drivingWarnings==3 && rider.community.strain>=36);
        assert(!community::aid(town,7,true)); // CJ cannot injure someone to farm rescue honor.
        assert(community::aid(town,7,false));
        assert(!community::aid(town,7,false));
        town.worldMinutes+=1440;assert(community::mayAid(town,7));
        std::stringstream townSave;town.write(townSave);
        auto importedTown=people::Registry::read(townSave);
        std::stringstream townAgain;importedTown.write(townAgain);
        assert(townSave.str()==townAgain.str());
        assert(importedTown.families[6].community.drivingWarnings==3);
        assert(importedTown.families[5].social.feud);
        assert(importedTown.nextIncident==town.nextIncident);
        auto oldTown=townSave.str();oldTown.replace(0,11,"SAPEOPLE 14");oldTown.resize(oldTown.find("COMMUNITY "));
        std::stringstream oldTownInput(oldTown);auto migratedTown=people::Registry::read(oldTownInput);
        assert(migratedTown.families[0].community.empty() && migratedTown.nextIncident==1);
        assert(migratedTown.families[5].social.feud); // Migration retains existing relationships.
        auto corruptTown=townSave.str();corruptTown.replace(corruptTown.find("COMMUNITY "),10,"COMMUNITYX");
        bool invalidTown=false;
        try {std::stringstream badTown(corruptTown);(void)people::Registry::read(badTown);}catch(const std::runtime_error &){invalidTown=true;}
        assert(invalidTown);
        people::Registry recycleTown;recycleTown.create(false);recycleTown.create(false);
        recycleTown.families[0].community.opinion=-1;
        assert(recycleTown.recycle([](unsigned){return false;})==2);
        community::Progress route;
        assert(!route.stuck(100,1000));assert(!route.stuck(80,10000));
        assert(route.stuck(80,22000));route.retried(22000);assert(!route.stuck(60,22001));
        assert(!route.stuck(4,50000));
        for(unsigned l=0;l<4;++l) {
            const auto beforeLanguage=locale::language;locale::language=static_cast<locale::Language>(l);
            for(const char *key:{"SERVICE_COMING","RESCUE_AID_OR_DRIVE","WIT_REPORT","BOUND_TOO_MUCH","PAIR_SIBLING"})
                assert(locale::tr(key)!=key);
            locale::language=beforeLanguage;
        }
        std::cout<<"PASS: local witnesses, traceable two-hop rumors, no duplicate opinion farming, "
                   "heard family feuds, social boundaries, driving warnings, rescue anti-farming, "
                   "V14/V15 migration, memory retention and stalled-route recovery.\n";
    }
