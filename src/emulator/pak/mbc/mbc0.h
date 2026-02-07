#pragma once

#include "mbc.h"

class Mbc0 : public Base {
   public:
    using Base::Base;

    void write_rom(u16 addr, u8 val) override {}
};