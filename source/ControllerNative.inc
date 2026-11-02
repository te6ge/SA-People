controller::Policy controllerPolicy;
ginput::Pad *ginputPad=nullptr;
Process previousPadUpdate=nullptr;
DWORD nextGInputProbe=0;
bool controllerHookReady=false;
void resetControllerInput() {
    controllerPolicy.reset();controls::padActions.fill(false);
    if(controls::padInHands)panelInvalid=true;
    controls::padInHands=false;
}
void __cdecl controllerUpdateHook() {
    previousPadUpdate(); // GInput and any earlier CPad wrapper finish first.
    controls::padActions.fill(false);
    if(!controls::padEnabled){resetControllerInput();return;}
    const auto now=GetTickCount();
    if(!ginputPad && due(now,nextGInputProbe)) {
        nextGInputProbe=now+2000;
        ginputPad=ginput::resolve(ginput::findModule());
        if(ginputPad)log("INPUT GInput API connected; version="+std::to_string(ginputPad->GetVersion()));
    }
    if(!ginputPad){resetControllerInput();return;}
    DWORD foregroundProcess=0;
    GetWindowThreadProcessId(GetForegroundWindow(),&foregroundProcess);
    const bool allowed=foregroundProcess==GetCurrentProcessId() && profileReady && !disabled &&
        !at<bool>(0xB7CB48) && !at<bool>(0xB7CB49) && !at<bool>(0xBA67A4) && !storyActive();
    const bool connected=ginputPad->IsPadConnected();
    const bool inHands=connected && ginputPad->HasPadInHands();
    if(!allowed || !connected) {resetControllerInput();return;}
    auto pad=reinterpret_cast<Byte *(__cdecl *)(int)>(0x53FB70)(0);
    if(!pad){resetControllerInput();return;}
    controller::State state{};std::memcpy(state.data(),pad,sizeof(state));
    auto player=reinterpret_cast<Byte *(__cdecl *)(int)>(0x56E210)(0);
    const auto car=player?drivenCar(player):nullptr;
    const bool canFocus=player && (onFoot(player)||(car && stoppedCar(car)));
    const auto input=controllerPolicy.update(state,connected,inHands,allowed,panelOpen,canFocus);
    controls::padActions=input.actions;
    if(controls::padInHands!=inHands)panelInvalid=true;
    controls::padInHands=inHands;
    if(inHands) {
        ginput::GeneralSettings settings{};
        ginputPad->SendConstEvent(ginput::FetchGeneralSettings,&settings);
        const bool playStation=controls::padGlyphs<0?settings.PlayStationButtons:controls::padGlyphs==1;
        if(playStation!=controls::padPlayStation){controls::padPlayStation=playStation;panelInvalid=true;}
    }
    controller::filter(state,input);
    std::memcpy(pad,state.data(),sizeof(state)); // Only this frame's consumed buttons; sticks remain live during focus.
}
bool installControllerHook() {
    if(controllerHookReady)return true;
    if(!controls::padEnabled){log("INPUT gamepad support disabled in settings.");return false;}
    controllerHookReady=patchCall(0x53BEE6,controllerUpdateHook,previousPadUpdate);
    log(controllerHookReady?"INPUT pad-update hook installed; GInput detection is deferred.":
        "INPUT pad-update hook unavailable; keyboard controls remain active.");
    return controllerHookReady;
}
