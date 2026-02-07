#include "ram.h"

u8   Ram::read_echo(u16 addr) { return wram[addr & 0x1FFF]; }
u8   Ram::read_io(u16 addr) { return io[addr - 0xFF00]; }
u8   Ram::read_hram(u16 addr) { return hram[addr - 0xFF80]; }
u8   Ram::read_oam(u16 addr) { return oam[addr - 0xFE00]; }
void Ram::write_echo(u16 addr, u8 val) { wram[addr & 0x1FFF] = val; }
void Ram::write_io(u16 addr, u8 val) { io[addr - 0xFF00] = val; }
void Ram::write_hram(u16 addr, u8 val) { hram[addr - 0xFF80] = val; }
void Ram::write_oam(u16 addr, u8 val) { oam[addr - 0xFE00] = val; }