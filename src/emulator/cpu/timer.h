#pragma once

#include "../mmu/mmu.h"

class Timer {
   public:
    Mmu& mmu;

    Timer(Mmu& m);

    void tick(u8 cycles);

    void reset_div_counter();

   private:
    u16 counter;
    int tima_counter;

    int get_clock_threshold(u8 tac);
};