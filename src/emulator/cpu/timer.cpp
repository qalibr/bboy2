#include "timer.h"

#include <tracy/Tracy.hpp>

#include "../emulator.h"

Timer::Timer(Mmu& m) : mmu(m) {
    counter      = 0;
    tima_counter = 0;
}

void Timer::save_state(TimerState& state) const {
    state.counter      = counter;
    state.tima_counter = tima_counter;
}

void Timer::load_state(const TimerState& state) {
    counter      = state.counter;
    tima_counter = state.tima_counter;
}

void Timer::tick(u8 cycles) {
    ZoneScoped;

    counter += cycles;
    mmu.div() = counter >> 8;

    if ((mmu.tac() & 0x04) != 0) {
        tima_counter += cycles;

        int threshold = get_clock_threshold(mmu.tac());

        while (tima_counter >= threshold) {
            tima_counter -= threshold;
            mmu.tima()++;

            if (mmu.tima() == 0x00) {
                mmu.tima() = mmu.tma();
                mmu.request_interrupt(InterruptType::Timer);
            }
        }
    }
}

void Timer::reset_div_counter() {
    counter   = 0;
    mmu.div() = 0;
}

int Timer::get_clock_threshold(u8 tac) {
    switch (tac & 0x03) {
        case 0x00:
            return 1024;
        case 0x01:
            return 16;
        case 0x02:
            return 64;
        case 0x03:
            return 256;
        default:
            return 1024;
    }
}
