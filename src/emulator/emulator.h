#pragma once

#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "cpu/cpu.h"
#include "cpu/timer.h"
#include "joypad.h"
#include "mmu/mmu.h"
#include "pak/pak.h"
#include "ppu/ppu.h"
#include "ppu/screen.h"

struct SaveHeader {
    char magic[4] = {'G', 'B', 'S', 'T'};
    u32  version  = 1;
};

#pragma pack(push, 1)
struct CpuState {
    u8   a, f, b, c, d, e, h, l;
    u16  sp, pc;
    bool ime, halted, halt_bug;
    u8   ime_schedule;
};

struct PpuState {
    int  scanline_counter;
    int  window_line_counter;
    bool prev_signal;
};

struct TimerState {
    u16 counter;
    int tima_counter;
};

struct MmuState {
    std::array<u8, 0x2000> wram;
    std::array<u8, 0x80>   io;
    std::array<u8, 0x80>   hram;
    std::array<u8, 0x2000> vram;
    std::array<u8, 0xA0>   oam;
    u8                     IE, IF;
    bool                   dma_active;
    u16                    dma_source_addr;
    u8                     dma_progress;
};

struct MbcState {
    bool is_eram_enabled;
    int  rom_bank;
    int  ram_bank;
};
#pragma pack(pop)

class Emulator {
   public:
    Pak&   pak;
    Mmu    mmu;
    Cpu    cpu;
    Ppu    ppu;
    Timer  timer;
    Joypad joy;

    Emulator(Pak& p);
    ~Emulator() = default;

    void run_frame();

    void save_state(const std::string& rom_path);
    void load_state(const std::string& rom_path);
};