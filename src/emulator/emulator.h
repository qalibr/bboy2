#pragma once

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "cpu/cpu.h"
#include "cpu/timer.h"
#include "mmu/mmu.h"
#include "pak/pak.h"
#include "ppu/ppu.h"
#include "ppu/screen.h"
#include "joypad.h"

class Emulator {
   public:
    Pak&  pak;
    Mmu   mmu;
    Cpu   cpu;
    Ppu   ppu;
    Timer timer;
    Joypad joy;

    Emulator(Pak& p);
    ~Emulator() = default;

    void run_frame();
};