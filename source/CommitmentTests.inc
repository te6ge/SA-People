    {
        people::Registry commitmentTown;
        auto &commitmentPerson=commitmentTown.create(true);commitmentPerson.household=1;commitmentPerson.presence={41,5,2495,-1688,13.5f,0,100,0,true,true};
        commitmentPerson.social.known=people::Name;commitmentPerson.social.journal=true;commitmentPerson.social.meaningful=20;
        commitmentPerson.social.romance=3;commitmentPerson.social.dates=6;commitmentPerson.social.trust=90;
        assert(commitment::update(commitmentTown,commitmentPerson) && commitmentPerson.commitment.since==1);
        int commitmentCash=2000;
        assert(!commitment::propose(commitmentTown,commitmentPerson.id,commitmentCash,true) && commitmentCash==2000); // Time together matters.
        commitmentTown.worldMinutes=1440;
        assert(!commitment::propose(commitmentTown,commitmentPerson.id,commitmentCash,false) && commitmentCash==2000); // Explicit consent to cost.
        assert(commitment::propose(commitmentTown,commitmentPerson.id,commitmentCash,true) && commitmentCash==1750 && commitmentPerson.commitment.stage==1);
        assert(!commitment::propose(commitmentTown,commitmentPerson.id,commitmentCash,true) && commitmentCash==1750);
        auto &other=commitmentTown.create(true);other.household=1;other.presence.alive=true;
        assert(!romance::eligible(commitmentTown,other));
        assert(!commitment::book(commitmentTown,commitmentPerson.id,commitmentCash,1,false) && commitmentCash==1750);
        assert(commitment::book(commitmentTown,commitmentPerson.id,commitmentCash,1,true) && commitmentCash==1250 && commitment::booked(commitmentTown,commitmentPerson));
        assert(!commitment::book(commitmentTown,commitmentPerson.id,commitmentCash,2,true) && commitmentCash==1250);
        std::stringstream scheduled;commitmentTown.write(scheduled);
        auto loaded=people::Registry::read(scheduled);
        assert(commitment::booked(loaded,loaded.families[0]) && loaded.families[0].commitment.proposedAt==1441);
        assert(commitment::marry(loaded,1) && !commitment::marry(loaded,1));
        assert(loaded.families[0].commitment.stage==3 && loaded.appointments[0].state==3);
        std::stringstream married;loaded.write(married);
        auto spouse=people::Registry::read(married);
        assert(spouse.families[0].commitment.marriedAt==1441 &&
               std::string(commitment::status(spouse.families[0]))=="REL_MARRIED");
        spouse.families[0].social.romance=0;spouse.families[0].social.cutOff=1;
        assert(std::string(commitment::status(spouse.families[0]))=="REL_ESTRANGED");
        assert(!romance::eligible(spouse,spouse.families[1])); // An argument is not a silent divorce.
        spouse.families[0].social.childStage=2;spouse.families[0].social.childMinutes=10080;
        assert(!commitment::separate(spouse,1,false) && spouse.families[0].commitment.stage==3);
        assert(commitment::separate(spouse,1,true) && !commitment::separate(spouse,1,true));
        assert(spouse.families[0].social.childStage==2 && spouse.families[0].social.dates==6 &&
               spouse.families[0].commitment.empty());
        assert(commitment::cancel(commitmentTown,1) && !commitment::booked(commitmentTown,commitmentPerson) && commitmentPerson.commitment.stage==1);
        commitmentPerson.social.resentment=60;
        assert(!commitment::book(commitmentTown,1,commitmentCash,1,true) && commitmentCash==1250); // No marriage to erase abuse.
        const auto oldTrust=commitmentPerson.social.trust;
        assert(std::string(commitment::talk(commitmentTown,1))=="REL_TALK_HURT");
        assert(std::string(commitment::talk(commitmentTown,1))=="REL_TALK_LATER" && commitmentPerson.social.trust==oldTrust);
        auto commitmentLegacyText=married.str();commitmentLegacyText.replace(0,11,"SAPEOPLE 16");commitmentLegacyText.resize(commitmentLegacyText.find("RELATIONSHIPS"));
        std::stringstream old(commitmentLegacyText);auto migratedCommitment=people::Registry::read(old);
        assert(migratedCommitment.families[0].commitment.empty() && migratedCommitment.families[0].social.dates==6);
        auto malformed=married.str();auto at=malformed.find("RELATIONSHIPS");
        malformed.resize(at);malformed+="RELATIONSHIPS 1\n1 2 0 1 0 1 0\n";
        bool rejected=false;try{std::stringstream bad(malformed);people::Registry::read(bad);}catch(const std::exception &){rejected=true;}
        assert(rejected); // A booked ceremony must identify a supported venue.
        for(unsigned stage=0;stage<4;++stage) {
            auto view=spouse;view.families[0].commitment.stage=stage;
            phone::State screen;screen.person=1;screen.page=phone::Page::Relationship;
            for(unsigned lang=0;lang<locale::LanguageCount;++lang) {
                locale::language=static_cast<locale::Language>(lang);
                for(auto page:{phone::Page::Relationship,phone::Page::RelationshipConfirm}) {
                    screen.page=page;auto scene=phone::compose(view,screen,"12:00");
                    for(const auto &t:scene.texts)assert(t.value.find("REL_")==std::string::npos);
                }
            }
        }
        const std::string chineseSentence=u8"我们今天在这里见面";
        const auto chineseLines=locale::wrap(chineseSentence,6);
        std::string chineseRejoined;for(const auto &line:chineseLines){assert(line.size()<=9);chineseRejoined+=line;}
        assert(chineseRejoined==chineseSentence && !locale::wrap(u8"好",0).empty());
        locale::language=locale::Language::Turkish;
        std::cout<<"PASS: relationship readiness, confirmed one-time costs, monogamy, ceremony persistence, marriage, estrangement, cancellation, V16 migration and five-language pages.\n";
    }
    {
        assert(npcbrain::mayAssist(8,1,false,false,true,true,false,false,true,false));
        assert(!npcbrain::mayAssist(7,2,false,false,true,true,false,false,true,false));
        assert(npcbrain::mayAssist(7,2,false,false,true,true,false,false,true,true));
        assert(!npcbrain::mayAssist(7,2,true,false,true,true,false,false,true,true));
        assert(!npcbrain::mayAssist(8,1,false,false,true,true,false,true,true,false));
        assert(!npcbrain::mayAssist(8,1,false,false,true,true,true,false,true,false));
        assert(npcbrain::score(100,2,true)>npcbrain::score(200,0,true));
        std::vector<unsigned char> ped(0x7c4,0);ped[0x46e]=0x10;ped[0x46f]=0x40;
        ped[0x719]=20;ped[0x71a]=25;const auto before=ped;
        npcbrain::Posture posture;posture.apply(ped.data(),npcbrain::Role::Support,225);
        assert((ped[0x46e]&0x48)==0x48 && (ped[0x46f]&2) && ped[0x71a]==45);
        posture.apply(ped.data(),npcbrain::Role::Support,225);posture.restore(ped.data());
        assert(ped==before);
        posture.apply(ped.data(),npcbrain::Role::Support,900,25);
        assert((ped[0x46e]&0x48)==0);posture.restore(ped.data());assert(ped==before);
        ped[0x46f]|=1;posture.apply(ped.data(),npcbrain::Role::Assault,4);
        assert(!(ped[0x46f]&2));ped[0x71a]=77;
        posture.apply(ped.data(),npcbrain::Role::Assault,4);assert(ped[0x71a]==77);
        posture.restore(ped.data());assert(ped[0x71a]==77);
        npcbrain::BarkMemory barkHistory;std::vector<std::string> lines={"a","barkHistory","c","d","e"};
        auto first=barkHistory.choose(1,lines,0,1000);assert(!first.empty());
        assert(barkHistory.choose(1,lines,1,2000).empty());
        assert(barkHistory.choose(2,{first},0,3000).empty());
        assert(barkHistory.choose(1,lines,0,242000)!=first);
        assert(barkHistory.choose(1,{},0,600000).empty());
        for(unsigned identity=3;identity<1500;++identity)barkHistory.choose(identity,{first},0,600001);
        assert(barkHistory.people.size()<=512);
        std::cout<<"PASS: gang-war eligibility and mission/social exclusions, target spreading, reversible bounded combat posture, external field changes and per-person/global bark repetition limits.\n";
    }

    {
        people::Registry town;auto &renewedPartner=town.create(true);renewedPartner.household=1;renewedPartner.presence.alive=true;
        renewedPartner.social.romance=3;renewedPartner.social.dates=6;renewedPartner.social.trust=90;
        assert(commitment::update(town,renewedPartner));town.worldMinutes=2000;
        renewedPartner.social.romance=0;assert(commitment::update(town,renewedPartner) && renewedPartner.commitment.empty());
        renewedPartner.social.romance=3;assert(commitment::update(town,renewedPartner) && renewedPartner.commitment.since==2001);
        assert(std::string(commitment::proposalBlock(town,renewedPartner))=="REL_NEED_TIME");
        renewedPartner.social.romance=1;renewedPartner.commitment={};renewedPartner.commitment.lastTalk=2001;
        assert(!commitment::update(town,renewedPartner) && renewedPartner.commitment.lastTalk==2001);
        renewedPartner.social.romance=3;renewedPartner.commitment.since=2001;
        renewedPartner.commitment.stage=3;renewedPartner.commitment.proposedAt=2001;renewedPartner.commitment.marriedAt=2001;
        renewedPartner.presence.alive=false;
        assert(std::string(street::relationship(renewedPartner))=="REL_BEREAVED");
        std::vector<unsigned char> ped(0x7c4,0);npcbrain::Posture lease;
        lease.apply(ped.data(),npcbrain::Role::Support,225);
        ped[0x46e]&=~0x48;ped[0x46f]&=~2; // Another mod changes owned bits during the lease.
        lease.apply(ped.data(),npcbrain::Role::Assault,4);lease.restore(ped.data());
        assert(!(ped[0x46e]&0x48) && !(ped[0x46f]&2));
        assert(awareness::nearMiss(8,1,0,12,0));
        assert(!awareness::nearMiss(-8,1,0,12,0)); // Receding car.
        assert(!awareness::nearMiss(8,5,0,12,0)); // Safe lateral clearance.
        assert(!awareness::nearMiss(8,1,4,12,0)); // Overpass.
        assert(!awareness::nearMiss(8,1,0,0,0));
        assert(!awareness::nearMiss(14,1,0,6,0)); // Not an imminent approach.
        assert(!awareness::nearMiss(8,1,0,std::numeric_limits<float>::infinity(),0));
        assert(awareness::civilian(4,1,false,true,true,false,false,false));
        assert(!awareness::civilian(7,1,false,true,true,false,false,false));
        assert(!awareness::civilian(6,1,false,true,true,false,false,false)); // Police keep native responses.
        assert(!awareness::civilian(18,1,false,true,true,false,false,false));
        assert(!awareness::civilian(4,2,false,true,true,false,false,false));
        assert(!awareness::civilian(4,1,false,true,true,false,true,false));
        assert(!awareness::civilian(4,1,false,true,true,true,false,false));
        assert(!awareness::civilian(4,1,true,true,true,false,false,false));
        awareness::Cooldowns cooldown;assert(cooldown.ready(1,100));cooldown.record(1,100);
        assert(!cooldown.ready(1,50000) && !cooldown.ready(2,200));
        assert(cooldown.ready(2,3100));assert(cooldown.ready(1,90100));
        for(unsigned n=1;n<300;++n)cooldown.record(n,n*3000);
        assert(cooldown.people.size()<=128);
        cooldown={};cooldown.record(1,0xfffffff0u);
        assert(!cooldown.ready(1,100) && cooldown.ready(1,100000));
        for(unsigned lang=0;lang<locale::LanguageCount;++lang) {
            locale::language=static_cast<locale::Language>(lang);
            for(const auto *prefix:{"WORLD_TRAFFIC_","WORLD_HURT_","WORLD_NIGHT_"})
                for(unsigned n=0;n<(std::string(prefix)=="WORLD_TRAFFIC_"?4u:3u);++n) {
                    const auto key=std::string(prefix)+std::to_string(n);
                    assert(locale::tr(key)!=key);
                }
        }
        locale::language=locale::Language::Turkish;
        std::cout<<"PASS: renewed relationship clock, bereaved status, external posture bits, traffic prediction/exclusions, bounded rollover-safe cooldowns and five-language world remarks.\n";
    }
