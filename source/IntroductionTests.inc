    {
        const char *dialogueKeys[]={"INTRO_BOUNDARY_0","INTRO_BOUNDARY_1","INTRO_BOUNDARY_2","INTRO_BOUNDARY_3",
            "INTRO_NAME_0","INTRO_NAME_1","INTRO_NAME_2","INTRO_CONTACT_OFFER","INTRO_CONTACT_OFFER_1",
            "INTRO_CONTACT_OFFER_2","GANG_NOT_FRIEND","GANG_TRUCE_0","GANG_TRUCE_1","GANG_TRUCE_2"};
        for(const auto *key:dialogueKeys) for(unsigned lang=0;lang<4;++lang)
            assert(locale::catalog.translate(key,static_cast<locale::Language>(lang))!=key);
        assert(introductions::gangRange(64,true) && !introductions::gangRange(65,true));
        assert(!introductions::gangRange(9,false));
        unsigned namesOnFirstVisit=0;
        for(unsigned identity=1;identity<=200;++identity) {
            people::Family firstVisitPerson{identity,false,0,{},{}};
            street::Encounter firstVisit{identity};
            for(unsigned turn=0;turn<6 && !firstVisit.ended;++turn)
                street::exchange(firstVisitPerson,firstVisit,street::Action::Greet,false,4,50);
            namesOnFirstVisit += (firstVisitPerson.social.known & people::Name) ? 1u:0u;
            assert(!(firstVisitPerson.social.known & people::Job));
            assert(!(firstVisitPerson.social.known & people::Background));
            assert(!firstVisitPerson.social.journal);
        }
        assert(namesOnFirstVisit>0 && namesOnFirstVisit<200);
        people::Family introContact{12,false,0,{},{}};
        introContact.social.known=people::Name; introContact.social.trust=50;
        introContact.social.encounters=3; introContact.social.meaningful=5;
        assert(introductions::contact(introContact));
        introContact.social.resentment=30;
        assert(!introductions::contact(introContact) && !introductions::personal(introContact));
        introContact.social.resentment=0; introContact.social.encounters=1;
        assert(!introductions::job(introContact,7));
        introContact.social.encounters=5;
        assert(introductions::job(introContact,7));
        // A introContact conversation can disclose one detail, never a biography in one visit.
        introContact.social.meaningful=3;
        street::Encounter anotherVisit{introContact.id}; anotherVisit.patience=8;
        for(unsigned turn=0;turn<6 && !anotherVisit.ended;++turn)
            street::exchange(introContact,anotherVisit,street::Action::Greet,false,4,50);
        assert(introContact.social.known & people::Job);
        assert(!introContact.social.journal);
        street::Encounter contactVisit{introContact.id};
        auto contactOffer=street::exchange(introContact,contactVisit,street::Action::Greet,false,4,50);
        assert(contactOffer.newJournal && contactOffer.line.rfind("INTRO_CONTACT_OFFER",0)==0);
        assert(!(introContact.social.known & (people::Mother|people::Father|people::Spouse|people::Child|people::Background)));
        std::cout << "PASS: eight-metre visible parley, mixed first-visit disclosures, delayed contacts, "
                     "rival trust gates and one personal disclosure per encounter.\n";
    }
