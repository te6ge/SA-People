    {
        // Districts: deeds move the local opinion, prices drop where CJ is loved, opinion fades.
        people::Registry town;
        assert(bonds::district(2495, -1680) == 1 && bonds::district(1950, -1700) == 2 &&
               bonds::district(-2000, 200) == 10 && bonds::district(2000, 1500) == 11 &&
               bonds::district(1700, -2400) == 9 && bonds::district(-500, -500) == 0);
        for (int i = 0; i < 12; ++i)
            bonds::deed(town, 1, 2);
        assert(town.districts[1] == 72 && bonds::price(town, 1, 80) == 40 &&
               bonds::price(town, 2, 80) == 80);
        bonds::deed(town, 2, -15);
        assert(town.districts[2] == -45 && std::string(bonds::mood(-45)) == "DIST_MOOD_FEARED");
        bonds::fade(town);
        assert(town.districts[1] == 71 && town.districts[2] == -44);

        // Week tally rolls over.
        bonds::count(town, bonds::Stat::Met);
        bonds::count(town, bonds::Stat::Met);
        assert(bonds::current(town).met == 2);
        town.worldMinutes += 10080;
        assert(bonds::current(town).met == 0);

        // Requests: ignoring costs nothing, a broken promise costs trust, a loan is paid at once.
        auto &pal = town.create(false);
        pal.presence.alive = pal.presence.located = true;
        pal.presence.type = 4;
        pal.presence.model = 7;
        pal.social.known = people::Name;
        pal.social.journal = true;
        pal.social.trust = 60;
        auto &other = town.create(true);
        other.presence.alive = other.presence.located = true;
        other.presence.type = 5;
        other.presence.model = 12;
        network::link(town, pal.id, other.id, 0);
        assert(bonds::ask(town, pal.id, 1) && town.request.kind == bonds::TalkTo &&
               town.request.target == other.id && town.letters.back().kind == 5);
        assert(!bonds::ask(town, pal.id, 2)); // One at a time.
        town.worldMinutes += bonds::AnswerMinutes + 1;
        assert(!bonds::expire(town) && !town.request.state && pal.social.trust == 60);
        assert(bonds::ask(town, pal.id, 1));
        int bondsCash = 0;
        assert(std::string(bonds::accept(town, bondsCash)) == "REQ_TALK_ACCEPTED" &&
               town.request.state == 2);
        assert(!bonds::complete(town, bonds::TalkTo, pal.id) &&
               bonds::complete(town, bonds::TalkTo, other.id) && pal.social.trust == 68);
        assert(bonds::ask(town, pal.id, 1) && bonds::accept(town, bondsCash));
        town.worldMinutes += bonds::DoMinutes + 1;
        assert(bonds::expire(town) && pal.social.trust == 60 &&
               town.letters.back().body == "MAIL_REQ_LET_DOWN");
        assert(bonds::ask(town, pal.id, 2) && town.request.kind == bonds::Lend);
        const unsigned amount = town.request.amount;
        bondsCash = static_cast<int>(amount) - 1;
        assert(std::string(bonds::accept(town, bondsCash)) == "REQ_NO_MONEY" && town.request.state == 1);
        bondsCash = 500;
        assert(std::string(bonds::accept(town, bondsCash)) == "REQ_LENT" &&
               bondsCash == 500 - static_cast<int>(amount) && town.request.state == 3);
        assert(!bonds::repay(town, bondsCash));
        std::stringstream debtSave; town.write(debtSave);
        auto debtRestored = people::Registry::read(debtSave);
        assert(debtRestored.request.state == 3 && debtRestored.request.amount == amount);
        town.worldMinutes = town.request.due;
        assert(!bonds::expire(town) && town.request.state == 3);
        assert(bonds::repay(town, bondsCash) == amount && bondsCash == 500);
        assert(!bonds::repay(town, bondsCash));

        people::Registry crowded;
        auto &borrower = crowded.create(false);
        borrower.presence.alive = true; borrower.social.journal = true; borrower.social.known=people::Name; borrower.social.trust = 60;
        for (unsigned i=0; i<64; ++i) crowded.later.push_back({borrower.id,later::Cooldown,0,1440});
        assert(bonds::ask(crowded, borrower.id, 150) && crowded.request.amount == 200);
        int crowdedCash=500;
        assert(std::string(bonds::accept(crowded,crowdedCash)) == "REQ_LENT" && crowdedCash==300);
        assert(crowded.request.state==3 && crowded.request.amount==200 && crowded.later.size()==64);
        std::stringstream crowdedSave; crowded.write(crowdedSave);
        auto crowdedLoaded=people::Registry::read(crowdedSave);
        crowdedLoaded.worldMinutes=crowdedLoaded.request.due;
        int cappedCash=999999950;
        assert(!bonds::repay(crowdedLoaded,cappedCash) && crowdedLoaded.request.state==3);
        assert(!bonds::expire(crowdedLoaded));
        assert(bonds::repay(crowdedLoaded,crowdedCash)==200 && crowdedCash==500);
        assert(!bonds::repay(crowdedLoaded,crowdedCash));

        // Birthdays: one invitation per year, one $50 gift.
        while (!bonds::birthdayToday(town, pal))
            town.worldMinutes += 1440;
        assert(bonds::invite(town, pal) && !bonds::invite(town, pal));
        const int trustBefore = pal.social.trust;
        bondsCash = 49;
        assert(!bonds::gift(town, pal, bondsCash, true));
        bondsCash = 100;
        assert(!bonds::gift(town, pal, bondsCash) && bondsCash == 100); // No confirmation: no spend.
        assert(bonds::gift(town, pal, bondsCash, true) && bondsCash == 50 && pal.social.trust == trustBefore + 12 &&
               !bonds::gift(town, pal, bondsCash, true));
        assert(later::pending(town) == 0); // Markers never count as pending.

        // A newer incoming text does not hide the outstanding request or its action.
        assert(bonds::ask(town,pal.id,1));
        town.post(pal.id,"MAIL_BDAY_THANKS");
        assert(town.letters.back().kind!=5 && bonds::open(town,pal.id));
        phone::State requestState; requestState.person=pal.id;requestState.page=phone::Page::Messages;
        auto requestScene=phone::compose(town,requestState,"12:00");
        bool requestHint=false;
        for(const auto &text:requestScene.texts)
            requestHint=requestHint || text.value==locale::tr("MAIL_ACCEPT_REQUEST");
        assert(requestHint);
        const auto requestActions=phone::actionKeys(town,requestState);
        assert(std::find(requestActions.begin(),requestActions.end(),std::string("PHONE_REQUEST"))!=requestActions.end());
        const int declineTrust=pal.social.trust;
        assert(bonds::decline(town,pal.id) && !town.request.state && pal.social.trust==declineTrust);
        assert(!bonds::decline(town,pal.id));
        assert(!bonds::localReaction(other,100) && !bonds::localReaction(other,-100));
        auto informed=other;
        informed.community.incident=1;informed.community.opinion=20;
        assert(bonds::localReaction(informed,50));
        informed.community.opinion=-20;
        assert(bonds::localReaction(informed,-50));
        informed.social.trust=60;assert(!bonds::localReaction(informed,-50));

        // V15 archives gain empty bonds without changing known contacts.
        people::Registry legacyTown;
        auto &legacyContact = legacyTown.create(false);
        legacyContact.social.known=people::Name;legacyContact.social.journal=true;legacyContact.social.trust=45;
        std::stringstream oldSave;legacyTown.write(oldSave);
        auto v15=oldSave.str();v15.replace(0,11,"SAPEOPLE 15");
        v15.resize(v15.find("BONDS"));
        std::stringstream v15Input(v15);
        auto importedV15=people::Registry::read(v15Input);
        assert(importedV15.families[0].social.trust==45 && !importedV15.request.state && importedV15.districts[1]==0);
        auto invalidRequest=importedV15;
        invalidRequest.request={1,bonds::Ride,0,3,200,100};
        assert(!invalidRequest.validBonds(1));
        invalidRequest.request={1,bonds::None,0,1,0,100};
        assert(!invalidRequest.validBonds(1));
        invalidRequest.request={1,bonds::TalkTo,0,1,0,100};
        assert(!invalidRequest.validBonds(1));

        // V16 persistence of bonds and day markers; markers expire with the day.
        std::stringstream saved;
        town.write(saved);
        auto loaded = people::Registry::read(saved);
        assert(loaded.districts[1] == 71 && loaded.districts[2] == -44 &&
               loaded.week.week == town.week.week && loaded.week.gifts == 1 &&
               bonds::marked(loaded, pal.id, bonds::GiftKind));
        loaded.worldMinutes += 1440;
        people::Later item;
        while (later::next(loaded, true, item))
            assert(item.kind < 8); // The loan's repayment may be due; markers never surface.
        assert(!bonds::marked(loaded, pal.id, bonds::GiftKind));
        std::cout << "PASS: district opinion and prices, weekly tally, friend requests, birthdays, "
                     "V16 bonds.\n";
    }
