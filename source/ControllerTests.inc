    {
        using namespace controller;
        State pad{};Policy policy;
        auto input=policy.update(pad,true,true,true,false,true);
        pad[LB]=pad[RB]=pad[A]=128;pad[LX]=50;pad[RY]=-40;
        input=policy.update(pad,true,true,true,false,true);
        assert(input.actions[controls::Focus] && input.actions[controls::Greet]);
        auto filtered=pad;filter(filtered,input);
        assert(!filtered[A] && !filtered[LB] && !filtered[RB] && filtered[LX]==50 && filtered[RY]==-40);
        pad[LB]=pad[RB]=0;
        input=policy.update(pad,true,true,true,false,true);filtered=pad;filter(filtered,input);
        assert(!filtered[A] && !input.actions[controls::Greet]); // Do not sprint/fire after releasing focus first.
        pad={};policy.update(pad,true,true,true,false,true);
        pad[LB]=pad[Up]=128;input=policy.update(pad,true,true,true,false,true);
        assert(input.actions[controls::Phone] && !input.actions[controls::Focus]);
        input=policy.update(pad,true,true,true,true,true);
        assert(!input.actions[controls::PhoneUp]); // Opening combo does not scroll the new page.
        pad={};policy.update(pad,true,true,true,true,true);
        pad[A]=128;pad[LX]=100;pad[RY]=-90;
        input=policy.update(pad,true,true,true,true,true);filtered=pad;filter(filtered,input);
        assert(input.actions[controls::PhoneSelect] && !filtered[A] && !filtered[LX] && !filtered[RY]);
        pad={};policy.update(pad,true,true,true,true,true);
        pad[B]=128;input=policy.update(pad,true,true,true,true,true);
        assert(input.actions[controls::PhoneBack]);
        input=policy.update(pad,true,true,true,false,true);filtered=pad;filter(filtered,input);
        assert(!filtered[B] && !input.actions[controls::Provoke]);
        input=policy.update(pad,false,true,true,false,true);
        assert(!input.consumed && !input.actions[controls::PhoneBack]);
        pad={};policy.update(pad,true,false,true,false,true);
        pad[LB]=pad[Down]=128;input=policy.update(pad,true,true,true,false,true);
        assert(input.actions[controls::Vitals]);
        input=policy.update(pad,true,true,false,false,true);
        assert(!input.consumed && !input.actions[controls::Vitals]); // Missions and focus loss never capture controls.
        pad={};policy.update(pad,true,true,true,false,false);
        pad[LB]=pad[RB]=pad[B]=128;input=policy.update(pad,true,true,true,false,false);
        assert(!input.consumed && !input.actions[controls::Focus]); // Moving car cannot enter a social focus.
        controls::padInHands=true;controls::padPlayStation=false;
        assert(controls::buttonHint(controls::Greet)=="A" && controls::phoneHint("ENTER / BACKSPACE")=="A / B");
        controls::padPlayStation=true;assert(controls::buttonHint(controls::Rob)==u8"△");
        controls::padInHands=false;controls::padPlayStation=false;
        assert(controls::buttonHint(controls::Greet)==controls::keyName(controls::keys[controls::Greet].primary));
        std::cout<<"PASS: controller chords, action isolation, walking/camera preservation, modal/release latches, "
                   "disconnect/mission safety, parked-only focus and Xbox/PlayStation hints.\n";
    }
