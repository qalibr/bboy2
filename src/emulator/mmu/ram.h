#pragma once

#include <array>

class Ram {
   public:
    std::array<u8, 0x2000> wram;
    std::array<u8, 0x80>   io;
    std::array<u8, 0x80>   hram;
    std::array<u8, 0x2000> vram;
    std::array<u8, 0xA0>   oam;

    u8   read_echo(u16 addr);
    u8   read_io(u16 addr);
    u8   read_hram(u16 addr);
    u8   read_oam(u16 addr);
    void write_echo(u16 addr, u8 val);
    void write_io(u16 addr, u8 val);
    void write_hram(u16 addr, u8 val);
    void write_oam(u16 addr, u8 val);
};