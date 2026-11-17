// Stock 1.0 US call sites verified against plugin-sdk and the executable:
// C_PcSave::SaveSlot -> GenericSave(int), CGame load -> GenericLoad(bool*).
// Original calls always run once, and their return values are preserved.
using NativeGenericSave=bool(__cdecl *)(int);
using NativeGenericLoad=bool(__cdecl *)(bool *);
NativeGenericSave previousGameSave=nullptr;
NativeGenericLoad previousGameLoad=nullptr;
constexpr std::uintptr_t SaveCall=0x619081,LoadCall=0x53c70b;
std::filesystem::path nativeSaveName(std::uintptr_t address,std::size_t capacity,
                                     const std::filesystem::path &cwd) {
    const auto first=reinterpret_cast<const char *>(address);
    const auto end=std::find(first,first+capacity,'\0');
    if(end==first || end==first+capacity)throw std::runtime_error("Native save filename unavailable");
    const auto length=static_cast<int>(end-first);
    const int count=MultiByteToWideChar(CP_ACP,0,first,length,nullptr,0);
    if(!count)throw std::runtime_error("Native save filename conversion failed");
    std::wstring wide(static_cast<std::size_t>(count),0);
    MultiByteToWideChar(CP_ACP,0,first,length,wide.data(),count);
    auto file=std::filesystem::path(wide);
    return (file.is_absolute()?file:cwd/file).lexically_normal();
}
void queueNewProfile() {
    profileReady=false;pendingProfile=ProfilePending::NewGame;pendingGameSave.clear();
}
bool __cdecl gameLoadProfileHook(bool *variablesLoaded) {
    clearGroveStock();
    std::filesystem::path file;
    try {
        file=nativeSaveName(0xc15fc8,104,std::filesystem::current_path());
    } catch(const std::exception &e){log(std::string("PROFILE before load: ")+e.what());}
    if(dirty)save();
    resetRuntime(); // Release the outgoing world's actor handles even if its filename is unavailable.
    queueNewProfile();
    const bool success=previousGameLoad(variablesLoaded);
    if(success) {
        pendingProfile=ProfilePending::Load;pendingGameSave=file;
        // No mod data is read and no gameplay tasks run inside the native load routine.
    } else {
        pendingProfile=ProfilePending::None;
        log("PROFILE native load failed; social gameplay paused until a successful load or new game.");
    }
    return success;
}
bool __cdecl gameSaveProfileHook(int unused) {
    clearGroveStock(); // Runtime rewards must never be serialized into the GTA save itself.
    std::filesystem::path cwd;
    try {cwd=std::filesystem::current_path();}catch(const std::exception &){}
    const bool success=previousGameSave(unused);
    if(!success)return false;
    try {
        if(!profileReady)throw std::runtime_error("No isolated world is active");
        const auto file=nativeSaveName(0xc16030,260,cwd);
        registry.savedAt=static_cast<std::uint64_t>(std::time(nullptr));
        profileKey=saveprofiles::save(directory,file,registry);
        activeArchive=saveprofiles::live(directory,profileKey);
        save();
        log("PROFILE checkpoint saved key="+profileKey);
    } catch(const std::exception &e) {
        log(std::string("PROFILE checkpoint failed: ")+e.what());
        help("PROFILE_WRITE_FAILED");
    }
    return success; // A mod-data failure must never change GTA's successful-save result.
}
bool stockProfileCall(std::uintptr_t call,std::uintptr_t expected) {
    const auto bytes=reinterpret_cast<const Byte *>(call);
    if(bytes[0]!=0xe8)return false;
    int displacement=0;std::memcpy(&displacement,bytes+1,4);
    return call+5+displacement==expected;
}
bool installProfileHooks() {
    if(profileHooksReady)return true;
    // Unknown replacements are left untouched. Do not run shared-state gameplay as a fallback.
    if(!stockProfileCall(SaveCall,0x5d13e0) || !stockProfileCall(LoadCall,0x5d17b0))return false;
    Process oldSave=nullptr,oldLoad=nullptr;
    if(!patchCall(SaveCall,reinterpret_cast<Process>(&gameSaveProfileHook),oldSave))return false;
    if(!patchCall(LoadCall,reinterpret_cast<Process>(&gameLoadProfileHook),oldLoad)) {
        Process ignored=nullptr;patchCall(SaveCall,oldSave,ignored);return false;
    }
    previousGameSave=reinterpret_cast<NativeGenericSave>(oldSave);
    previousGameLoad=reinterpret_cast<NativeGenericLoad>(oldLoad);
    profileHooksReady=true;log("PROFILE isolated save/load hooks installed.");return true;
}
void rebuildProfileWorld() {
    spatial=people::SpatialIndex{};
    const auto existingCount=registry.families.size();
    for(unsigned i=0;i<existingCount;++i)
        if(!registry.families[i].origin && (registry.families[i].social.known & people::Name))
            network::discover(registry,i+1);
    returnAfter.clear();restored.clear();restoreCursor=0;selected=currentPage=0;
    nextLaterCheck=nextIncoming=nextRoutineTick=0;avoidAfter.clear();routineCursor=0;
    nextInvitation.clear();phoneState={};eventDirector={};
    for(auto &f:registry.families) {
        if(f.presence.located)f.female=modelTraits.female(f.presence.model,f.female);
        spatial.update(f);
    }
    expiredMeetings=0;recovered=false;panelInvalid=true;dirty=false;
}
void applyPendingProfile() {
    const auto action=pendingProfile;const auto file=pendingGameSave;
    pendingProfile=ProfilePending::None;pendingGameSave.clear();
    if(action==ProfilePending::None)return;
    profileReady=false;resetRuntime();
    try {
        const char *notice="PROFILE_NEW_GAME";
        if(action==ProfilePending::Load) {
            auto loaded=saveprofiles::load(directory,file);
            if(loaded.status==saveprofiles::Status::Corrupt)throw std::runtime_error("Matched checkpoint and backup are unreadable");
            profileKey=loaded.key;registry=std::move(loaded.world);
            notice=loaded.status==saveprofiles::Status::Fresh?"PROFILE_FRESH":
                loaded.status==saveprofiles::Status::Recovered?"SAVE_RECOVERED":"PROFILE_LOADED";
        } else {profileKey.clear();registry=people::Registry{};}
        activeArchive=saveprofiles::live(directory,profileKey);
        rebuildProfileWorld();profileReady=true;
        help(notice);log("PROFILE active key="+(profileKey.empty()?std::string("new-game"):profileKey)+
            " records="+std::to_string(registry.families.size()));
    } catch(const std::exception &e) {
        registry=people::Registry{};spatial=people::SpatialIndex{};activeArchive.clear();profileKey.clear();
        help("PROFILE_READ_FAILED");log(std::string("PROFILE load refused: ")+e.what());
    }
}
bool canImportSharedProfile() {
    try {return profileReady && saveprofiles::mayImport(directory,profileKey);}
    catch(const std::exception &){return false;}
}
void importSharedProfile() {
    if(!canImportSharedProfile()){phoneState.notice="PROFILE_IMPORT_UNAVAILABLE";return;}
    try {
        people::Registry imported;
        if(!saveprofiles::importLegacy(directory,profileKey,true,imported)) {
            phoneState.notice="PROFILE_IMPORT_UNAVAILABLE";return;
        }
        resetRuntime();registry=std::move(imported);rebuildProfileWorld();profileReady=true;
        dirty=true;save();
        phoneState.page=phone::Page::Settings;phoneState.cursor=0;
        // Reopen without carrying the former world's selected contact or queued reply.
        togglePhone();phoneState.notice="PROFILE_IMPORTED";panelInvalid=true;
        log("PROFILE legacy world explicitly imported into key="+profileKey);
    } catch(const std::exception &e){phoneState.notice="PROFILE_READ_FAILED";log(std::string("PROFILE import: ")+e.what());}
}
