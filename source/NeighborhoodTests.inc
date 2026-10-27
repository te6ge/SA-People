    {
        using namespace neighborhood;
        assert(hostile(1u<<8,0) && hostile(1u,0) && hostile(0,1u<<8));
        assert(!hostile(1u<<6,0) && !rival(8) && rival(7) && rival(9) && !rival(17));
        unsigned memberDislike=(1u<<8)|(1u<<6), memberHate=1u|(1u<<9);
        TruceBits personalTruce;
        personalTruce.lift(memberDislike,memberHate);
        assert(memberDislike==(1u<<6) && memberHate==(1u<<9));
        personalTruce.lift(memberDislike,memberHate); // A refresh must retain original hostility.
        memberHate |= 1u<<7; // Keep another script's unrelated change when restoring.
        personalTruce.restore(memberDislike,memberHate);
        assert(memberDislike==((1u<<8)|(1u<<6)) && memberHate==(1u|(1u<<9)|(1u<<7)));
        for(unsigned activity=0;activity<4;++activity)
            assert(plannedActivity(plan(activity))==static_cast<int>(activity));
        assert(plannedActivity("MAIL_GOOD_OUTING")==-1);
        people::Registry story;
        unsigned owner=story.create(false).id;
        while(people::mix(owner)%2) owner=story.create(false).id;
        auto &person=story.families[owner-1];
        person.presence={50,4,100,100,13,0,100,0,true,true};
        person.social.known=people::Name; person.social.journal=1; person.social.trust=50;
        person.social.helped=1;
        auto &friendPerson=story.create(true);
        friendPerson.presence={41,5,110,110,13,0,100,0,true,true};
        const unsigned friendId=friendPerson.id;
        assert(canRecognize(story.families[owner-1],false,false,false,0,100));
        assert(!canRecognize(story.families[owner-1],true,false,false,0,100));
        assert(!canRecognize(story.families[owner-1],false,true,false,0,100));
        assert(!canRecognize(story.families[owner-1],false,false,true,0,100));
        assert(!canRecognize(story.families[owner-1],false,false,false,11,100));
        assert(later::helped(story,owner));
        assert(story.families[owner-1].daily.memories & later::StoryStarted);
        const auto count=story.later.size(); later::helped(story,owner);
        assert(story.later.size()==count);
        auto follow=std::find_if(story.later.begin(),story.later.end(),[](auto &l){return l.kind==later::FollowUp;});
        assert(follow!=story.later.end() && follow->due>=1440);
        std::stringstream savedStory; story.write(savedStory);
        auto loadedStory=people::Registry::read(savedStory);
        assert(loadedStory.later.size()==story.later.size());
        unsigned storyCash=0;
        assert(later::deliver(loadedStory,{owner,later::FollowUp,0,1440},storyCash));
        assert(loadedStory.letters.back().body=="MAIL_STORY_FRIEND");
        assert(later::deliver(loadedStory,{owner,later::Introduction,0,2880},storyCash));
        assert(loadedStory.families[friendId-1].social.journal);
        assert(loadedStory.letters.back().person==friendId && loadedStory.letters.back().source==owner);
        assert(!later::deliver(loadedStory,{owner,later::Introduction,0,2880},storyCash));
        assert(loadedStory.connections.size()==1);
        loadedStory.families[owner-1].social.robbed=1;
        assert(!later::deliver(loadedStory,{owner,later::FollowUp,0,1440},storyCash));
        // Old V13 files still import, while new story event kinds must not be accepted as V13.
        people::Registry empty; std::stringstream compatible; empty.write(compatible);
        auto legacyStory=compatible.str(); legacyStory.replace(0,11,"SAPEOPLE 13");
        legacyStory.resize(legacyStory.find("COMMUNITY "));
        std::stringstream previous(legacyStory); assert(people::Registry::read(previous).families.empty());
        auto incompatible=savedStory.str(); incompatible.replace(0,11,"SAPEOPLE 13");
        bool oldRejected=false;
        try {std::stringstream oldInput(incompatible); (void)people::Registry::read(oldInput);}
        catch(const std::runtime_error &) {oldRejected=true;}
        assert(oldRejected);
        people::Registry proposals;
        auto &planner=proposals.create(false);
        planner.presence={50,4,0,0,13,0,100,0,true,true};
        planner.social.journal=1; planner.social.trust=50; planner.social.known=people::Name;
        proposals.worldMinutes=600;
        assert(socialworld::spontaneous(proposals,planner,600));
        assert(plannedActivity(proposals.letters.back().body)>=0);
        assert(!socialworld::spontaneous(proposals,planner,600));
        std::cout << "PASS: Grove rivalry masks, specific activity proposals, safe recognition gates, "
                     "persistent multi-day introductions, no duplicates and betrayal cancellation.\n";
    }
