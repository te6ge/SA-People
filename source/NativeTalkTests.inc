    {
        using namespace nativetalk;
        static_assert(Dealer == 17 && Criminal == 20, "SA ped type mapping");
        assert(ambientRoutine(17) && ambientRoutine(7) && ambientRoutine(16));
        assert(!ambientRoutine(20) && !ambientRoutine(6));
        assert(replaceDealer(true, false, true, true, false, 17));
        assert(!replaceDealer(true, true, true, true, false, 17));
        assert(!replaceDealer(false, false, true, true, false, 17));
        assert(!replaceDealer(true, false, false, true, false, 17));
        assert(!replaceDealer(true, false, true, false, false, 17));
        assert(!replaceDealer(true, false, true, true, true, 17));
        assert(!replaceDealer(true, false, true, true, false, 20));
        ScriptGate gate;
        const people::Handle first{12, 3, 29}, reused{12, 4, 29};
        unsigned char enabled = 1;
        unsigned state = 3; // The exact scripted WAITINGFORINPUT path behind the Y/N box.
        assert(gate.apply(0, first, true, true, enabled, state));
        assert(!enabled && !state && gate.slots[0].owned);
        assert(!gate.apply(0, first, true, true, enabled, state));
        gate.apply(0, first, true, false, enabled, state); // Story mission begins.
        assert(enabled == 1 && !gate.slots[0].owned && !state);
        enabled = 0; state = 0;
        gate.apply(1, first, true, true, enabled, state);
        gate.apply(1, first, true, false, enabled, state);
        assert(!enabled); // Preserve a flag already disabled by the game's own script.
        enabled = 1;
        gate.apply(2, first, true, true, enabled, state);
        enabled = 0;
        gate.apply(2, reused, true, false, enabled, state);
        assert(!enabled && !gate.slots[2].owned); // Never restore a recycled pool slot.
        enabled = 1; state = 2;
        assert(!gate.apply(3, first, true, false, enabled, state));
        assert(enabled == 1 && state == 2); // Civilian/mission conversations untouched.
        gate.apply(4, first, true, true, enabled, state);
        gate.apply(4, first, false, false, enabled, state);
        assert(!gate.slots[4].owned); // Deleted actor: discard ownership.
        std::cout << "PASS: actual DEALER=17/CRIMINAL=20 mapping, scripted Y/N gate, "
                     "mission/setting restoration, recycled-slot and foreign-dialogue safety.\n";
    }
