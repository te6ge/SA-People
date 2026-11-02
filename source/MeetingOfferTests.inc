    {
        people::Registry offerTown;
        auto &offered=offerTown.create(false);offered.social.journal=true;
        offered.social.known=people::Name;offered.social.trust=30;
        offered.presence={7,4,100,100,10,0,100,0,true,true};
        assert(std::string(phonedialogue::availability(offerTown,1))=="TXT_AVAIL_FREE");
        assert(!offerTown.families[0].community.requests); // Text enquiries cannot consume favors.
        offerTown.post(1,"TXT_AVAIL_FREE");
        assert(phonedialogue::planOffer(offerTown.letters.back()));
        offerTown.post(1,"TXT_AVAIL_FREE",true);
        assert(!phonedialogue::planOffer(offerTown.letters.back()));
        offered.social.trust=10;
        assert(std::string(phonedialogue::availability(offerTown,1))=="PHONE_INVITE_REJECT");
        offered.social.trust=30;offered.social.cutOff=1;
        assert(!meetings::canInvite(offered));offered.social.cutOff=0;
        offered.daily.busyUntil=60;
        assert(std::string(phonedialogue::availability(offerTown,1))=="TXT_AVAIL_BUSY");
        offered.daily.busyUntil=0;offered.community.strain=80;offered.community.lastRequest=1;
        assert(std::string(phonedialogue::availability(offerTown,1))=="BOUND_SPACE");
        assert(offered.community.strain==80 && !offered.community.requests);
        offered.community.strain=0;
        assert(meetings::schedule(offerTown,1,1,2,3));
        assert(std::string(phonedialogue::availability(offerTown,1))=="PHONE_INVITE_ACTIVE");
        assert(meetings::safeToPlan(false,true,false,false,true)); // Any activity from a parked driver's seat.
        assert(meetings::safeToPlan(false,true,false,true,false));
        assert(!meetings::safeToPlan(false,true,false,false,false));
        assert(!meetings::safeToPlan(true,true,false,false,true));
        assert(!meetings::safeToPlan(false,false,false,false,true));
        assert(!meetings::safeToPlan(false,true,true,false,true));
        std::cout<<"PASS: actionable availability replies, shared invitation eligibility, read-only enquiries, "
                   "blocked/busy/strained contacts, existing plans and safe parked-car planning.\n";
    }
