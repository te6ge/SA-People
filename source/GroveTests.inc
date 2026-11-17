    {
        people::Registry groveTown;int groveCash=120000;
        assert(grove::nearby(2488,-1668,13.3f,0) && grove::atLedger(2487.5f,-1686,13.4f,0));
        assert(!grove::atLedger(2488,-1668,13.3f,0) && !grove::atLedger(2495,-1691,14.7f,1));
        assert(!grove::nearby(NAN,-1668,13.3f,0));
        assert(std::string(grove::donate(groveTown,groveCash,25))=="GROVE_INVALID");
        for(unsigned donation=0;donation<12;++donation)assert(std::string(grove::donate(groveTown,groveCash,10000))=="GROVE_DONATED");
        assert(groveCash==0 && groveTown.grove.balance==120000 && groveTown.honor==0);
        assert(std::string(grove::donate(groveTown,groveCash,100))=="GROVE_NO_CASH");
        assert(std::string(grove::upgrade(groveTown,grove::Pantry,300))=="GROVE_CHANGED");
        assert(std::string(grove::upgrade(groveTown,grove::Pantry,2500))=="GROVE_BUILT");
        assert(grove::ready(groveTown,grove::Health) && !grove::ready(groveTown,grove::Armor));
        assert(grove::collected(groveTown,grove::Health) && !grove::ready(groveTown,grove::Health));
        assert(std::string(grove::upgrade(groveTown,grove::Pantry,2500))=="GROVE_CHANGED");
        assert(std::string(grove::upgrade(groveTown,grove::Pantry,6500))=="GROVE_BUILT");
        assert(!grove::ready(groveTown,grove::Health)); // Upgrading never grants an immediate second pickup.
        for(unsigned project=1;project<grove::Count;++project)
            for(unsigned tier=0;tier<2;++tier)assert(std::string(grove::upgrade(groveTown,project,people::GroveFund::cost(project,tier)))=="GROVE_BUILT");
        assert(groveTown.grove.balance==13000 && groveTown.grove.valid(0));
        assert(grove::collected(groveTown,grove::Pistol));
        groveTown.worldMinutes=719;assert(!grove::ready(groveTown,grove::Health));
        groveTown.worldMinutes=720;assert(grove::ready(groveTown,grove::Health) && !grove::ready(groveTown,grove::Pistol));
        groveTown.worldMinutes=1440;assert(grove::ready(groveTown,grove::Pistol));
        grove::Visit groveVisit;groveVisit.here=true;groveVisit.safe=true;groveVisit.driver=true;groveVisit.parked=true;groveVisit.carHealth=250;
        assert(std::string(grove::use(groveTown,grove::Pantry,groveVisit))=="GROVE_COLLECT_OUTSIDE");
        assert(std::string(grove::use(groveTown,grove::Workshop,groveVisit))=="GROVE_WRECK");
        groveVisit.carHealth=700;
        assert(std::string(grove::use(groveTown,grove::Workshop,groveVisit))=="GROVE_REPAIR_DONE" && groveVisit.carHealth==1000);
        assert(std::string(grove::use(groveTown,grove::Workshop,groveVisit))=="GROVE_RESTOCK");
        std::stringstream groveArchive;groveTown.write(groveArchive);
        auto groveLoaded=people::Registry::read(groveArchive);
        assert(groveLoaded.grove.balance==13000 && groveLoaded.grove.level[3]==2 && groveLoaded.grove.usedAt[5]==1441);
        auto groveV18=groveArchive.str();groveV18.replace(0,11,"SAPEOPLE 18");groveV18.resize(groveV18.find("GROVE2 "));
        std::stringstream groveOld(groveV18);assert(people::Registry::read(groveOld).grove.balance==0);
        auto groveV19=groveV18;groveV19.replace(0,11,"SAPEOPLE 19");groveV19+="GROVE 450 5000 2 2 2 2 2 2\n";
        std::stringstream groveOld19(groveV19);const auto groveMigrated=people::Registry::read(groveOld19);
        assert(groveMigrated.grove.balance==450 && groveMigrated.grove.contributed==5000 && groveMigrated.grove.legacyCredit==37450);
        assert(groveMigrated.grove.level[0]==2 && !groveMigrated.grove.level[3] && !grove::ready(groveMigrated,grove::Armor));
        auto groveBad=groveArchive.str();groveBad.resize(groveBad.find("GROVE2 "));groveBad+="GROVE2 999 0 0 0 0 0 0 0 0 0 0 0 0\n";
        bool groveRejected=false;try{std::stringstream groveCorrupt(groveBad);people::Registry::read(groveCorrupt);}catch(const std::exception &){groveRejected=true;}assert(groveRejected);
        struct GroveTestAPI {
            std::array<int,5> live{{-1,-1,-1,-1,-1}};
            std::array<bool,5> taken{},drained{};
            unsigned created=0,removed=0;int sequence=100;
            int create(unsigned slot){++created;live[slot]=++sequence;return live[slot];}
            bool matches(int handle,unsigned slot){return live[slot]==handle;}
            bool depleted(int handle,unsigned slot){return matches(handle,slot) && drained[slot];}
            bool collected(int handle){for(unsigned i=0;i<5;++i)if(live[i]==handle && taken[i]){taken[i]=false;live[i]=-1;return true;}return false;}
            void remove(int handle){for(auto &current:live)if(current==handle){current=-1;++removed;}}
        } groveApi;
        groveTown.grove.usedAt.fill(0);grovestock::Manager groveManager;
        assert(!groveManager.update(groveTown,true,groveApi) && groveApi.created==5);
        assert(!groveManager.update(groveTown,true,groveApi) && groveApi.created==5);
        groveApi.taken[0]=true;
        assert(groveManager.update(groveTown,true,groveApi) && !grove::ready(groveTown,grove::Health) && groveApi.created==5);
        groveApi.drained[2]=true;
        assert(groveManager.update(groveTown,true,groveApi) && !grove::ready(groveTown,grove::Pistol));
        const auto ammoClaimedAt=groveTown.grove.usedAt[grove::Pistol];
        assert(groveManager.handle(2)!=-1 && !groveManager.update(groveTown,true,groveApi));
        assert(groveTown.grove.usedAt[grove::Pistol]==ammoClaimedAt && groveApi.created==5);
        // A recycled foreign handle is never removed when saving/pausing.
        groveApi.live[1]=9000;
        assert(!groveManager.clear(groveTown,groveApi) && groveApi.removed==3 && groveApi.live[1]==9000);
        assert(!groveManager.clear(groveTown,groveApi) && groveApi.removed==3);
        for(unsigned groveLanguage=0;groveLanguage<locale::LanguageCount;++groveLanguage) {
            locale::language=static_cast<locale::Language>(groveLanguage);
            for(auto grovePage:{phone::Page::Grove,phone::Page::GroveDonate,phone::Page::GroveUpgrades,phone::Page::GroveSupplies,phone::Page::GroveConfirm}) {
                phone::State groveScreen;groveScreen.page=grovePage;groveScreen.groveHere=true;groveScreen.groveAmount=2500;
                for(const auto &line:phone::compose(groveTown,groveScreen,"12:00").texts)assert(line.value.find("GROVE_")==std::string::npos);
                for(auto dimensions:{std::pair<float,float>{640.0f,480.0f},{800.0f,600.0f},{1024.0f,768.0f},{1280.0f,720.0f},{3440.0f,1440.0f}}) {
                    const auto lay=phone::viewLayout(grovePage,dimensions.first,dimensions.second);
                    assert(std::abs(lay.x-(dimensions.first-940*lay.scale)/2)<.1f && std::abs(lay.y-(dimensions.second-580*lay.scale)/2)<.1f);
                    assert(lay.x>=24 && lay.y>=32 && lay.x+940*lay.scale<=dimensions.first && lay.y+580*lay.scale<=dimensions.second);
                }
            }
        }
        locale::language=locale::Language::Turkish;
        std::cout<<"PASS: expensive Grove ledger, independent timed stock, pickup ownership/cleanup, per-save persistence, lossless V19 price migration, V18 migration and seven-language book layouts.\n";
    }
