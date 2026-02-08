#pragma once

#include "mmu/mmu.h"
#include "raylib.h"

class Mmu;

class Joypad {
   public:
    Mmu& mmu;

    Joypad(Mmu& m);

    u8 get_joyp_register();
    void set_joyp_register(u8 val);

    void handle_input();

   private:
    bool up, down, left, right;
    bool a, b, start, select;
};