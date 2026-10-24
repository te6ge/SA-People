    {
        socialprogress::HonorBatch batch;
        assert(batch.record(1, true, 100) == 0);
        assert(batch.record(1, true, 4000) == 0);
        assert(batch.record(2, true, 8000) == 0);
        assert(batch.record(3, true, 12000) == 1);
        assert(batch.record(1, true, 15000) == 0);
        assert(batch.record(4, false, 20000) == 0);
        assert(batch.record(4, false, 24000) == 0);
        assert(batch.record(5, false, 28000) == 0);
        assert(batch.record(6, false, 32000) == -1);
        assert(batch.record(4, false, 34000) == 0);
        assert(batch.record(1, true, 700000) == 0);
        socialprogress::HonorSounds cues;
        cues.push(1); cues.push(-1); cues.push(-1);
        assert(cues.take(false) == 0 && cues.pending.size() == 2);
        assert(cues.take(true) == 1 && cues.take(false) == 0);
        assert(cues.take(true) == -1 && cues.take(true) == 0);
        assert(socialprogress::openness(-100) == -25 && socialprogress::openness(100) == 25);
        unsigned warmGreetings = 0, coldGreetings = 0;
        for (unsigned id = 1; id <= 200; ++id) {
            people::Family high{}, low{};
            high.id = low.id = id;
            street::Encounter highEncounter, lowEncounter;
            warmGreetings += street::exchange(high, highEncounter, street::Action::Greet, false, 4, 100)
                .reaction == dialogue::Reaction::Friendly ? 1u : 0u;
            coldGreetings += street::exchange(low, lowEncounter, street::Action::Greet, false, 4, -100)
                .reaction == dialogue::Reaction::Friendly ? 1u : 0u;
        }
        assert(warmGreetings > coldGreetings + 50);
        assert(!socialprogress::availableForEvent(180, 179, false, false, 0));
        assert(socialprogress::availableForEvent(180, 180, false, false, 0));
        assert(!socialprogress::availableForEvent(0, 180, false, false, 3));
        assert(!socialprogress::availableForEvent(0, 180, true, false, 0));
        assert(!socialprogress::availableForEvent(0, 180, false, true, 0));
        assert(socialprogress::defended(true, true, 25));
        assert(!socialprogress::defended(false, true, 0));
        assert(!socialprogress::defended(true, false, 0));
        assert(!socialprogress::defended(true, true, 80));
        socialprogress::LineDeck deck;
        std::set<unsigned> seenLines;
        unsigned previous = 99;
        for (unsigned i = 0; i < 24; ++i) {
            const unsigned line = deck.take(12, i * 31);
            assert(line != previous);
            previous = line;
            assert(seenLines.insert(line).second);
            if (i == 11) seenLines.clear();
        }
        socialprogress::Cheat code;
        unsigned clock = 1000, activations = 0;
        for (char c : std::string("wrongIMARTHURMORGAN"))
            activations += code.letter(c, clock += 200) ? 1u : 0u;
        assert(activations == 1 && code.typed.empty());
        for (char c : std::string("imarthur")) assert(!code.letter(c, clock += 200));
        clock += 20000;
        for (char c : std::string("morgan")) assert(!code.letter(c, clock += 200));
        for (char c : std::string("imarthurmorgan"))
            activations += code.letter(c, clock += 200) ? 1u : 0u;
        assert(activations == 2);
        socialcontext::TalkChoices quickQuestions;
        assert(quickQuestions.use(0, 1000) == 1);
        assert(quickQuestions.use(0, 2000) == 0);
        assert(quickQuestions.use(0, 4000) == 2); // No eight-second per-choice lock.
        std::cout << "PASS: distinct-person honor batches, no cooldown cue loss, event rest, "
                     "active offender attribution, opening decks and typed honor cheat.\n";
    }
    {
        people::Registry messageBook;
        auto &person = messageBook.create(true);
        person.social.known = people::Name;
        person.social.journal = true;
        person.social.trust = 50;
        person.presence.alive = true;
        const char *answer = nullptr;
        auto sendPreset = [&](unsigned choice) {
            messageBook.worldMinutes += 6;
            assert(std::string(phonedialogue::send(messageBook, 1, choice, false, 900, &answer)).empty());
            assert(answer);
            const std::string result(answer);
            messageBook.post(1, answer);
            return result;
        };
        assert(sendPreset(0).rfind("TXT_REPLY_HELLO_", 0) == 0);
        assert(sendPreset(0).rfind("TXT_REPLY_REPEAT_", 0) == 0);
        assert(sendPreset(1).rfind("TXT_REPLY_CHECK_", 0) == 0);
        assert(sendPreset(0).rfind("TXT_REPLY_HELLO_", 0) == 0); // A different preset breaks the streak.
        messageBook.post(1, "MAIL_GOOD_OUTING_3", false, 4);
        assert(sendPreset(7).rfind("TXT_ACK_OUTING_", 0) == 0);
        messageBook.post(1, "MAIL_GOOD_OUTING_1", false, 4);
        assert(sendPreset(7).rfind("TXT_ACK_OUTING_", 0) == 0); // Another outing is a new context.
        assert(sendPreset(7).rfind("TXT_REPLY_REPEAT_", 0) == 0);
        messageBook.worldMinutes += 70;
        assert(sendPreset(7).rfind("TXT_ACK_OUTING_", 0) == 0);
        messageBook.post(1, "MAIL_LATER_THANKS1", false, 1);
        assert(sendPreset(6).rfind("TXT_ACK_HELP_", 0) == 0);
        auto &datePerson = messageBook.families[0];
        datePerson.presence = {93, 5, 0, 0, 13, 0, 100, 0, true, true};
        people::Appointment outing;
        outing.person = 1; outing.state = 2;
        assert(socialworld::complete(messageBook, outing, datePerson, true, false));
        assert(datePerson.daily.busyUntil == messageBook.worldMinutes + 180);
        std::stringstream afterOuting;
        messageBook.write(afterOuting);
        auto savedOuting = people::Registry::read(afterOuting);
        assert(savedOuting.families[0].daily.busyUntil == datePerson.daily.busyUntil);
        messageBook.post(1, "MAIL_FAMILY_PLANS", false, 3);
        assert(sendPreset(6).rfind("TXT_ACK_HELP_", 0) == 0);
        std::cout << "PASS: repeated presets require a consecutive streak; fresh events, "
                     "other presets and stale conversations do not trigger repeat replies.\n";
    }
