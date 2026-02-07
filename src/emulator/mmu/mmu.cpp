#include "mmu.h"

#include "../cpu/timer.h"
#include "../pak/pak.h"
#include "../ppu/ppu.h"

Mmu::Mmu(Pak& p) : pak(p) {
    memory_map.fill(nullptr);

    u8* rom_ptr = pak.data.data();

    // ROM | 0x0000 - 0x7FFF
    for (int i = 0; i <= 0x7; i++) {
        memory_map[i] = rom_ptr + (i * 0x1000);
    }

    // VRAM | 0x8000 - 0x9FFF
    u8* vram_ptr    = ram.vram.data();
    memory_map[0x8] = vram_ptr;
    memory_map[0x9] = vram_ptr + 0x1000;

    // WRAM | 0xC000 - 0xDFFF
    u8* wram_ptr    = ram.wram.data();
    memory_map[0xC] = ram.wram.data();
    memory_map[0xD] = wram_ptr + 0x1000;

    // Echo RAM | 0xE000 - 0xEFFF
    memory_map[0xE] = wram_ptr;

    if (pak.mbc) {
        pak.mbc->set_mmu(this);
    }

    IE = 0;
    IF = 0;

    init_io_registers();
};

void Mmu::map_rom_page(u8 page_i, u16 bank_n) {
    int rom_offset  = bank_n * 0x4000;
    int page_offset = (page_i - 4) * 0x1000;

    memory_map[page_i] = pak.data.data() + rom_offset + page_offset;
}

void Mmu::map_ram_page(u8 page_i, u8* ptr) { memory_map[page_i] = ptr; }

u8 Mmu::read_u8(u16 addr) {
    if (dma_active && addr < 0xFF80) {
        return 0xFF;
    }

    if (addr < 0xF000) {  // ----------- ROM, VRAM, ERAM, WRAM, Echo RAM
        if (addr >= 0x8000 && addr < 0xA000) {
            Mode mode = ppu_ptr->get_mode();
            if (mode == Mode::Drawing) {
                return 0xFF;
            }
        }

        u8* page = memory_map[addr >> 12];
        if (page == nullptr) return 0xFF;
        return page[addr & 0x0FFF];
    } else if (addr < 0xFE00) {  // ---- Remaining Echo RAM, Mirror of C000-DDFF | 0xE000 - FDFF
        return ram.read_echo(addr);
    } else if (addr < 0xFEA0) {  // ---- OAM | 0xFE00 - FE9F
        Mode mode = ppu_ptr->get_mode();
        if (mode == Mode::OamScan || mode == Mode::Drawing) {
            return 0xFF;
        }
        return ram.read_oam(addr);
    } else if (addr < 0xFF00) {  // ---- Not Usable | 0xFEA0 - 0xFEFF
        return 0xFF;
    } else if (addr < 0xFF80) {  // ---- IO Registers | 0xFF00 - 0xFF7F
        if (addr == 0xFF0F) {
            return IF;
        }
        return ram.read_io(addr);
    } else if (addr < 0xFFFF) {  // ---- HRAM | 0xFF80 - 0xFFFE
        return ram.read_hram(addr);
    } else {
        return IE;
    }
}

void Mmu::write_u8(u16 addr, u8 val) {
    if (dma_active && addr < 0xFF80) {
        return;
    }

    if (addr < 0x8000) {  // ----------- ROM
        if (pak.mbc) {
            pak.mbc->write_rom(addr, val);
        }
        return;
    }
    if (addr < 0xF000) {  // ----------- VRAM, ERAM, WRAM, Echo RAM
        if (addr >= 0x8000 && addr < 0xA000) {
            Mode mode = ppu_ptr->get_mode();
            if (mode == Mode::Drawing) {
                return;
            }
        }

        u8* page = memory_map[addr >> 12];
        if (page != nullptr) {
            page[addr & 0x0FFF] = val;
        }
        return;
    }

    if (addr < 0xFE00) {  // ----------- Remaining Echo RAM, Mirror of C000-DDFF | 0xE000 - FDFF
        ram.write_echo(addr, val);
    } else if (addr < 0xFEA0) {  // ---- OAM | 0xFE00 - FE9F
        Mode mode = ppu_ptr->get_mode();
        if (mode == Mode::OamScan || mode == Mode::Drawing) {
            return;
        }
        ram.write_oam(addr, val);
    } else if (addr < 0xFF00) {  // ---- Not Usable | 0xFEA0 - 0xFEFF
    } else if (addr < 0xFF80) {  // ---- IO Registers | 0xFF00 - 0xFF7F
        // ##############################################
        // # Special cases, return immediately
        if (addr == 0xFF04) {
            if (timer_ptr) timer_ptr->reset_div_counter();
            return;
        } else if (addr == 0xFF41) {  // Protecting STAT register's lower 3 bits.
            u8 current_val = stat();
            stat()         = (current_val & 0x07) | (val & 0xF8);
            return;
        } else if (addr == 0xFF44) {  // LY register is reset on write
            ly() = 0;
            return;
        }

        // ##############################################
        // # Perform write for other registers
        ram.write_io(addr, val);

        // ##############################################
        // # Handling side-effects of a write
        if (addr == 0xFF0F) {
            IF = val;
        } else if (addr == 0xFF46) {  // DMA Transfer
            dma_active      = true;
            dma_source_addr = val << 8;
            dma_progress    = 0;
        } else if (addr == 0xFF47 || addr == 0xFF48 || addr == 0xFF49) {
            if (ppu_ptr) ppu_ptr->update_palettes();
        }
    } else if (addr < 0xFFFF) {  // ---- HRAM | 0xFF80 - 0xFFFE
        ram.write_hram(addr, val);
    } else {
        IE = val;
    }
}

u8 Mmu::ppu_read_u8(u16 addr) {
    if (addr >= 0x8000 && addr <= 0x9FFF) {
        u8* page = memory_map[addr >> 12];
        if (page == nullptr) return 0xFF;
        return page[addr & 0x0FFF];
    }

    if (addr >= 0xFE00 && addr <= 0xFE9F) {
        return ram.read_oam(addr);
    }

    return 0xFF;
}

void Mmu::tick_dma(u8 cycles) {
    if (!dma_active) return;

    int bytes_to_copy = cycles / 4;
    for (int i = 0; i < bytes_to_copy; i++) {
        u16 src_addr = dma_source_addr + dma_progress;
        u8  data     = 0xFF;  // default 

        if (src_addr < 0xF000) {  // ROM, VRAM, ERAM, WRAM
            u8* page = memory_map[src_addr >> 12];
            if (page != nullptr) {
                data = page[src_addr & 0x0FFF];
            }
        } else if (src_addr < 0xFE00) {  // Echo RAM
            data = ram.read_echo(src_addr);
        }

        ram.write_oam(0xFE00 + dma_progress, data);
        dma_progress++;

        if (dma_progress >= 160) {
            dma_active = false;
            break;
        }
    }
}

void Mmu::request_interrupt(InterruptType type) { IF |= (1 << static_cast<u8>(type)); }

void Mmu::init_io_registers() {
    ram.io[0x05] = 0x00;  // TIMA
    ram.io[0x06] = 0x00;  // TMA
    ram.io[0x07] = 0x00;  // TAC
    ram.io[0x10] = 0x80;  // NR10
    ram.io[0x11] = 0xBF;  // NR11
    ram.io[0x12] = 0xF3;  // NR12
    ram.io[0x14] = 0xBF;  // NR14
    ram.io[0x16] = 0x3F;  // NR21
    ram.io[0x17] = 0x00;  // NR22
    ram.io[0x19] = 0xBF;  // NR24
    ram.io[0x1A] = 0x7F;  // NR30
    ram.io[0x1B] = 0xFF;  // NR31
    ram.io[0x1C] = 0x9F;  // NR32
    ram.io[0x1E] = 0xBF;  // NR34
    ram.io[0x20] = 0xFF;  // NR41
    ram.io[0x21] = 0x00;  // NR42
    ram.io[0x22] = 0x00;  // NR43
    ram.io[0x23] = 0xBF;  // NR44
    ram.io[0x24] = 0x77;  // NR50
    ram.io[0x25] = 0xF3;  // NR51
    ram.io[0x26] = 0xF1;  // NR52
    ram.io[0x40] = 0x91;  // LCDC
    ram.io[0x42] = 0x00;  // SCY
    ram.io[0x43] = 0x00;  // SCX
    ram.io[0x45] = 0x00;  // LYC
    ram.io[0x47] = 0xFC;  // BGP
    ram.io[0x48] = 0xFF;  // OBP0
    ram.io[0x49] = 0xFF;  // OBP1
    ram.io[0x4A] = 0x00;  // WY
    ram.io[0x4B] = 0x00;  // WX
}