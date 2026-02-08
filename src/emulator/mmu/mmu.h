#pragma once

#include <array>

#include "ram.h"

class Pak;
class Timer;
class Ppu;
class Joypad;

enum class InterruptType : u8 {
    VBlank = 0,
    Lcd    = 1,
    Timer  = 2,
    Serial = 3,
    Joypad = 4,
};

class Mmu {
   public:
    Pak& pak;
    Ram  ram;

    Mmu(Pak& p);

    u8     IE;
    u8     IF;
    Timer* timer_ptr = nullptr;
    Ppu*   ppu_ptr   = nullptr;
    Joypad* joy_ptr = nullptr;

    std::array<u8*, 16> memory_map;

    bool dma_active = false;
    u16  dma_source_addr;
    u8   dma_progress;

    void map_rom_page(u8 page_i, u16 bank_n);
    void map_ram_page(u8 page_i, u8* ptr);

    void set_timer(Timer* t) { timer_ptr = t; }
    void connect_ppu(Ppu* p) { ppu_ptr = p; }
    void connect_joypad(Joypad* joy) {joy_ptr = joy;}
    
    void tick_dma(u8 cycles);

    void request_interrupt(InterruptType type);

    u8   read_u8(u16 addr);
    void write_u8(u16 addr, u8 val);
    u8   ppu_read_u8(u16 addr);

    u8& p1() { return ram.io[0x00]; }

    // =============================================================
    //  Graphics Registers
    // =============================================================
    u8& div() { return ram.io[0x04]; }

    u8& tima() { return ram.io[0x05]; }

    u8& tma() { return ram.io[0x06]; }

    u8& tac() { return ram.io[0x07]; }

    u8& bgp() { return ram.io[0x47]; }

    u8& obp0() { return ram.io[0x48]; }

    u8& obp1() { return ram.io[0x49]; }

    u8& lcdc() { return ram.io[0x40]; }

    u8& stat() { return ram.io[0x41]; }

    u8& scy() { return ram.io[0x42]; }

    u8& scx() { return ram.io[0x43]; }

    u8& ly() { return ram.io[0x44]; }

    u8& lyc() { return ram.io[0x45]; }

    u8& wy() { return ram.io[0x4A]; }

    u8& wx() { return ram.io[0x4B]; }

   private:
    void init_io_registers();
};