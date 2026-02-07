#pragma once

#include "../pak.h"
#include "mbc.h"

class Mbc1 : public Base {
   public:
    using Base::Base;

    void write_rom(u16 addr, u8 val) override {
        switch (addr >> 13) {
            case 0:  // 0x0000 - 0x1FFF
            {
                bool new_state = (val & 0x0F) == 0x0A;
                if (is_eram_enabled != new_state) {
                    is_eram_enabled = new_state;
                    update_ram_banking();
                }
            } break;
            case 1:  // 0x2000 - 0x3FFF
                bank1_register = val & 0x1F;
                if (bank1_register == 0) bank1_register = 1;
                update_rom_banking();
                break;
            case 2:  // 0x4000 - 0x5FFF
                bank2_register = val & 0x3;
                update_banking();
                break;
            case 3:  // 0x6000 - 0x7FFF
                banking_mode_select = val & 0x1;
                update_banking();
                break;
        }
    }

   private:
    bool banking_mode_select = false;

    u8 bank1_register = 1;
    u8 bank2_register = 0;

    void update_banking() {
        update_rom_banking();
        update_ram_banking();
    }

    void update_rom_banking() {
        if (mmu == nullptr) return;

        int lower_bank_num = 0;
        int upper_bank_num;

        if (banking_mode_select == 0) {
            lower_bank_num = (bank2_register << 5);
            upper_bank_num = lower_bank_num | bank1_register;
        } else {
            lower_bank_num = 0;
            upper_bank_num = bank1_register;
        }

        int rom_bank_mask = (pak.data.size() / 0x4000) - 1;

        lower_bank_num &= rom_bank_mask;
        upper_bank_num &= rom_bank_mask;

        rom_bank = upper_bank_num;

        u8* rom_data_ptr   = pak.data.data();
        int lower_offset   = lower_bank_num * 0x4000;
        mmu->memory_map[0] = rom_data_ptr + lower_offset;
        mmu->memory_map[1] = rom_data_ptr + lower_offset + 0x1000;
        mmu->memory_map[2] = rom_data_ptr + lower_offset + 0x2000;
        mmu->memory_map[3] = rom_data_ptr + lower_offset + 0x3000;

        int upper_offset   = upper_bank_num * 0x4000;
        mmu->memory_map[4] = rom_data_ptr + upper_offset;
        mmu->memory_map[5] = rom_data_ptr + upper_offset + 0x1000;
        mmu->memory_map[6] = rom_data_ptr + upper_offset + 0x2000;
        mmu->memory_map[7] = rom_data_ptr + upper_offset + 0x3000;
    }

    void update_ram_banking() {
        if (mmu == nullptr) return;

        int current_ram_bank = 0;

        if (banking_mode_select == 1) {
            current_ram_bank = bank2_register;
        }

        if (is_eram_enabled && !eram.empty()) {
            int max_ram_banks = eram.size() / 0x2000;
            if (max_ram_banks > 0) {
                current_ram_bank %= max_ram_banks;
            }

            int ram_offset = current_ram_bank * 0x2000;

            if (ram_offset + 0x2000 > eram.size()) return;

            u8* ram_ptr = eram.data() + ram_offset;

            mmu->map_ram_page(0xA, ram_ptr);
            mmu->map_ram_page(0xB, ram_ptr + 0x1000);
        } else {
            mmu->map_ram_page(0xA, nullptr);
            mmu->map_ram_page(0xB, nullptr);
        }
    }
};
