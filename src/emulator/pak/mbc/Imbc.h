#pragma once

#include <iostream>

class Mmu;

class Imbc {
   public:
    virtual ~Imbc()                            = default;
    virtual void write_rom(u16 addr, u8 val)   = 0;
    virtual void set_mmu(Mmu* m)               = 0;
    virtual void save_state(std::ostream& out) = 0;
    virtual void load_state(std::istream& in)  = 0;
};