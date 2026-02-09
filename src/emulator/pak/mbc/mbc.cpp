#include "mbc.h"

#include <iostream>

#include "../pak.h"

void Base::save_state(std::ostream& out) {
    MbcState mbc_state;
    mbc_state.is_eram_enabled = is_eram_enabled;
    mbc_state.rom_bank        = rom_bank;
    mbc_state.ram_bank        = ram_bank;
    out.write(reinterpret_cast<const char*>(&mbc_state), sizeof(MbcState));

    if (is_eram_enabled && !eram.empty()) {
        out.write(reinterpret_cast<const char*>(eram.data()), eram.size());
    }
}

void Base::load_state(std::istream& in) {
    MbcState mbc_state;
    in.read(reinterpret_cast<char*>(&mbc_state), sizeof(MbcState));
    is_eram_enabled = mbc_state.is_eram_enabled;
    rom_bank        = mbc_state.rom_bank;
    ram_bank        = mbc_state.ram_bank;

    if (is_eram_enabled && !eram.empty()) {
        in.read(reinterpret_cast<char*>(eram.data()), eram.size());
    }

    // Simplistic reapplication of banking to the MMU to restore correct 
    // memory map. Will need to keep that in mind if/when implementing
    // future MBCs.
    if (mmu) {
        // ROM Bank: 0x4000-0x7FFF
        u16 effective_rom_bank = (rom_bank == 0) ? 1 : rom_bank;
        mmu->map_rom_page(4, effective_rom_bank);
        mmu->map_rom_page(5, effective_rom_bank);
        mmu->map_rom_page(6, effective_rom_bank);
        mmu->map_rom_page(7, effective_rom_bank);

        // RAM Bank: 0xA000-0xBFFF
        if (is_eram_enabled && !eram.empty() && ram_bank < (eram.size() / 0x2000)) {
            mmu->map_ram_page(0xA, eram.data() + (ram_bank * 0x2000));
            mmu->map_ram_page(0xB, eram.data() + (ram_bank * 0x2000) + 0x1000);
        }
    }
}
