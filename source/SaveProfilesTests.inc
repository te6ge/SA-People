    {
        namespace fs=std::filesystem;
        const auto root=fs::temp_directory_path()/fs::path("sa-people-profiles-"+
            std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
        fs::create_directories(root);
        const auto slotA=root/"GTASAsf1.b",slotB=root/"GTASAsf2.b";
        auto writeFile=[](const fs::path &p,const std::string &s){std::ofstream f(p,std::ios::binary);f<<s;};
        writeFile(slotA,std::string(256,'a'));writeFile(slotB,std::string(256,'a'));
        people::Registry profileA,profileB;
        profileA.honor=-40;profileA.create(false).social.trust=-20;
        profileA.families[0].social.cutOff=1;
        profileB.honor=70;profileB.create(true).social.trust=65;
        const auto keyA=saveprofiles::save(root,slotA,profileA),keyB=saveprofiles::save(root,slotB,profileB);
        assert(keyA!=keyB); // Even byte-identical GTA saves in different slots are independent.
        auto loadedA=saveprofiles::load(root,slotA),loadedB=saveprofiles::load(root,slotB);
        assert(loadedA.status==saveprofiles::Status::Loaded && loadedA.world.honor==-40);
        assert(loadedB.world.honor==70 && loadedB.world.families[0].social.trust==65);
        assert(loadedA.world.families[0].social.cutOff==1 && !loadedB.world.families[0].social.cutOff);
        loadedA.world.honor=-100;loadedA.world.families[0].social.trust=-80;
        saveprofiles::atomicWrite(saveprofiles::live(root,keyA),loadedA.world);
        auto rollback=saveprofiles::load(root,slotA);
        assert(rollback.world.honor==-40 && rollback.world.families[0].social.trust==-20);
        // An overwritten slot cannot inherit the previous save's contacts or honor.
        writeFile(slotA,std::string(256,'c'));
        auto fresh=saveprofiles::load(root,slotA);
        assert(fresh.status==saveprofiles::Status::Fresh && fresh.world.honor==0 && fresh.world.families.empty());
        // Save-as forks the current world; subsequent changes stay in their own checkpoint.
        const auto forkKey=saveprofiles::save(root,slotA,profileB);
        profileB.honor=15;saveprofiles::save(root,slotB,profileB);
        assert(saveprofiles::load(root,slotA).world.honor==70);
        assert(saveprofiles::load(root,slotB).world.honor==15);
        // Only the matching checkpoint's backup can be used for recovery.
        const auto target=saveprofiles::checkpoint(root,keyB);
        writeFile(target,"broken");
        assert(saveprofiles::load(root,slotB).status==saveprofiles::Status::Recovered);
        assert(saveprofiles::load(root,slotB).world.honor==70);
        fs::remove(target);
        assert(saveprofiles::load(root,slotB).status==saveprofiles::Status::Recovered);
        writeFile(target,"broken");
        writeFile(fs::path(target.wstring()+L".bak"),"broken too");
        assert(saveprofiles::load(root,slotB).status==saveprofiles::Status::Corrupt);
        assert(saveprofiles::load(root,slotA).world.honor==70);
        // Legacy imports require an explicit choice and never modify the shared input.
        saveprofiles::atomicWrite(root/"people.dat",profileA);
        people::Registry imported;
        assert(!saveprofiles::importLegacy(root,forkKey,false,imported));
        assert(saveprofiles::load(root,slotA).world.honor==70);
        assert(saveprofiles::importLegacy(root,forkKey,true,imported));
        assert(imported.honor==-40 && saveprofiles::load(root,slotA).world.honor==-40);
        assert(saveprofiles::read(root/"people.dat").honor==-40);
        assert(saveprofiles::read(root/"people.pre-save-profiles.dat").honor==-40);
        assert(saveprofiles::load(root,slotB).status==saveprofiles::Status::Corrupt);
        bool rejected=false;
        try {saveprofiles::checkpoint(root,"../other");}catch(const std::exception &){rejected=true;}
        assert(rejected);
        writeFile(slotA,std::string(256,'a')); // Restoring the original GTA file restores its original branch.
        assert(saveprofiles::load(root,slotA).world.honor==-40);
        fs::remove_all(root);
        std::cout<<"PASS: per-save honor/contact isolation, same-slot rollback, save-as forks, replaced slots, "
                   "matching-backup recovery, confirmed legacy import and original archive preservation.\n";
    }
