#pragma once

#include "../pak.h"
#include "mbc.h"

class Mbc3 : public Base {
   public:
    using Base::Base;

    Mbc3(Pak& p, int eram_size) : Base(p, eram_size) { last_rtc_update = system_clock::now(); }

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
                bank1_register = val & 0x7F;
                if (bank1_register == 0) bank1_register = 1;
                update_rom_banking();
                break;
            case 2:  // 0x4000 - 0x5FFF
                bank2_register = val;
                update_ram_banking();
                break;
            case 3:  // 0x6000 - 0x7FFF
                if (latch_sequence_state == 0x00 && val == 0x01) {
                    latch_rtc();
                }
                latch_sequence_state = val;
                break;
        }
    }

   private:
    u8 bank1_register = 1;
    u8 bank2_register = 0;

    u8 rtc_s;
    u8 rtc_m;
    u8 rtc_h;
    u8 rtc_dl;
    u8 rtc_dh;

    u8 latched_s;
    u8 latched_m;
    u8 latched_h;
    u8 latched_dl;
    u8 latched_dh;

    u8 latch_sequence_state = 0xFF;

    system_clock::time_point last_rtc_update;

    void update_rom_banking() {
        if (mmu == nullptr) return;

        int target_bank = bank1_register & 0x7F;

        if (target_bank == 0) target_bank = 1;

        int rom_bank_mask = (pak.data.size() / 0x4000) - 1;

        target_bank &= rom_bank_mask;
        rom_bank = target_bank;

        u8* rom_data_ptr   = pak.data.data();
        mmu->memory_map[0] = rom_data_ptr;
        mmu->memory_map[1] = rom_data_ptr + 0x1000;
        mmu->memory_map[2] = rom_data_ptr + 0x2000;
        mmu->memory_map[3] = rom_data_ptr + 0x3000;

        int offset         = target_bank * 0x4000;
        mmu->memory_map[4] = rom_data_ptr + offset;
        mmu->memory_map[5] = rom_data_ptr + offset + 0x1000;
        mmu->memory_map[6] = rom_data_ptr + offset + 0x2000;
        mmu->memory_map[7] = rom_data_ptr + offset + 0x3000;
    }

    void update_ram_banking() {
        if (mmu == nullptr) return;

        if (is_eram_enabled && bank2_register <= 0x03) {
            if (eram.empty()) {
                mmu->map_ram_page(0xA, nullptr);
                mmu->map_ram_page(0xB, nullptr);
                return;
            }

            int max_ram_banks    = eram.size() / 0x2000;
            int current_ram_bank = bank2_register;

            if (max_ram_banks > 0) {
                current_ram_bank %= max_ram_banks;
            } else {
                mmu->map_ram_page(0xA, nullptr);
                mmu->map_ram_page(0xB, nullptr);
                return;
            }
            int ram_offset = current_ram_bank * 0x2000;
            u8* ram_ptr    = eram.data() + ram_offset;
            mmu->map_ram_page(0xA, ram_ptr);
            mmu->map_ram_page(0xB, ram_ptr + 0x1000);
        } else {
            mmu->map_ram_page(0xA, nullptr);
            mmu->map_ram_page(0xB, nullptr);
        }
    }

    void latch_rtc() {
        update_rtc();
        latched_s  = rtc_s;
        latched_m  = rtc_m;
        latched_h  = rtc_h;
        latched_dl = rtc_dl;
        latched_dh = rtc_dh;
    }

    void update_rtc() {
        if ((rtc_dh & 0x40) != 0) {
            return;
        }

        auto now     = system_clock::now();
        auto elapsed = now - last_rtc_update;

        auto      elapsed_seconds_duration = duration_cast<seconds>(elapsed);
        long long total_seconds            = elapsed_seconds_duration.count();

        if (total_seconds < 1) {
            return;
        }

        last_rtc_update += seconds(total_seconds);

        long long s = rtc_s + total_seconds;
        rtc_s       = s % 60;

        long long m = rtc_m + (s / 60);
        rtc_m       = m % 60;

        long long h = rtc_h + (m / 60);
        rtc_h       = h % 24;

        long long days = rtc_dl | ((rtc_dh & 1) << 8);
        days += (h / 24);

        rtc_dl = days & 0xFF;

        rtc_dh = (rtc_dh & 0xFE) | ((days >> 8) & 1);

        if (days > 511) {
            rtc_dh |= 0x80;
        }
    }
};