    {
        people::Family incidentStranger{91, false, 0};
        assert(street::strangerIncident(incidentStranger));
        for (unsigned evidence = 0; evidence < 13; ++evidence) {
            auto familiar = incidentStranger;
            switch (evidence) {
            case 0: familiar.social.known = people::Name; break;
            case 1: familiar.social.encounters = 1; break;
            case 2: familiar.social.attempts = 1; break;
            case 3: familiar.social.helped = 1; break;
            case 4: familiar.social.journal = true; break;
            case 5: familiar.social.romance = 3; break;
            case 6: familiar.social.feud = 1; break;
            case 7: familiar.social.robbed = 1; break;
            case 8: familiar.daily.outings = 1; break;
            case 9: familiar.social.events = 1; break;
            case 10: familiar.social.meaningful = 1; break;
            case 11: familiar.social.trust = 7; break;
            case 12: familiar.meetings = 1; break;
            }
            assert(!street::strangerIncident(familiar));
        }
        auto calmContact = incidentStranger;
        calmContact.social.known = people::Name;
        calmContact.social.trust = 20;
        assert(!calmContact.social.journal && neighborhood::personalPeace(calmContact));
        for (unsigned betrayal = 0; betrayal < 5; ++betrayal) {
            auto upset = calmContact;
            switch (betrayal) {
            case 0: upset.social.robbed = 1; break;
            case 1: upset.social.feud = 1; break;
            case 2: upset.social.cutOff = 1; break;
            case 3: upset.social.anger = 20; break;
            case 4: upset.social.resentment = 20; break;
            }
            assert(!neighborhood::personalPeace(upset));
        }
        street::Encounter rejected{91};
        street::rejectBusy(rejected);
        calmContact.social.anger = 30;
        assert(!street::mayGreet(rejected) && street::mayDefuse(calmContact, rejected));
        auto reply = street::exchange(calmContact, rejected, street::Action::Defuse, true);
        assert(reply.calmed && rejected.apologyUsed && !street::mayDefuse(calmContact, rejected, true));
        rejected = {91};
        calmContact.social.anger = 0;
        assert(!street::mayDefuse(calmContact, rejected) && street::mayDefuse(calmContact, rejected, true));
        auto peacefulFight = street::exchange(calmContact, rejected, street::Action::Defuse, true, 4, 0, true);
        assert(peacefulFight.calmed && peacefulFight.leaves && rejected.apologyUsed);
        assert(!street::mayDefuse(calmContact, rejected, true));
        // A broken promise or a blood feud cannot be erased by repeatedly pressing J.
        rejected = {91}; rejected.brokenTruce = true;
        assert(!street::mayDefuse(calmContact, rejected, true));
        rejected = {91}; calmContact.social.feud = 1;
        assert(!street::mayDefuse(calmContact, rejected, true));

        const auto originalBinding = controls::keys[controls::Defuse];
        controls::keys[controls::Defuse].primary = 'K';
        const auto originalLanguage = locale::language;
        for (unsigned language = 0; language < 4; ++language) {
            locale::language = static_cast<locale::Language>(language);
            for (bool ready : {false, true}) {
                panel::Hints hints;
                hints.canGreet = false;
                hints.angry = true;
                hints.ready = ready;
                auto hud = panel::roaming(&calmContact, false, "", "", false, false, 0, 1280, 720, hints);
                bool found = false, rebound = false;
                for (const auto &line : hud.texts) {
                    if (line.value == locale::tr("SAKINLESTIR")) {
                        found = true;
                        assert(line.color.r == (ready ? 240 : 160));
                    }
                    rebound = rebound || line.value == "K";
                }
                assert(found && rebound);
            }
        }
        locale::language = originalLanguage;
        controls::keys[controls::Defuse] = originalBinding;
        std::cout << "PASS: familiar people excluded from random attacks, personal gang peace, "
                     "defuse after rejected greetings and owned combat, no apology loops, "
                     "active localized and rebound calm prompts.\n";
    }
