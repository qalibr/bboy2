#include "emulator.h"

#include <fstream>
#include <sstream>

Emulator::Emulator(Pak& p) : pak(p), mmu(pak), cpu(mmu), ppu(mmu), timer(mmu), joy(mmu) {
    mmu.set_timer(&timer);
    mmu.connect_ppu(&ppu);
    mmu.connect_joypad(&joy);
}

void Emulator::run_frame() {
    int cycles_this_frame = 0;

    while (cycles_this_frame < Ppu::CYCLES_PER_FRAME) {
        u8 cycles_ran;

        if (mmu.dma_active) {
            cycles_ran = 4;
        } else {
            cycles_ran = cpu.step();
        }

        timer.tick(cycles_ran);
        ppu.tick(cycles_ran);
        mmu.tick_dma(cycles_ran);

        cycles_this_frame += cycles_ran;
    }
}