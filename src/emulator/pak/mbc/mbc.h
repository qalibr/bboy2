#pragma once

#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

#include "../../emulator.h"
#include "../../mmu/mmu.h"
#include "Imbc.h"

using namespace std::chrono;

class Pak;

class Base : public Imbc {
   public:
    Pak& pak;
    Mmu* mmu = nullptr;

    Base(Pak& p, int eram_size) : pak(p) {
        eram.resize(eram_size);
        is_eram_enabled = false;
        rom_bank        = 1;
        ram_bank        = 0;
    }

    ~Base() override = default;

    void set_mmu(Mmu* m) override { this->mmu = m; }

    void save_state(std::ostream& out) override;
    void load_state(std::istream& in) override;

    std::vector<u8> eram;
    bool            is_eram_enabled;
    int             rom_bank;
    int             ram_bank;
};