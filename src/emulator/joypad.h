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

    bool is_fps_uncapped() const;
    bool should_display_fps() const;

   private:
    bool up, down, left, right;
    bool a, b, start, select;
    bool uncapped_fps;
    bool display_fps;

    int calculate_uncapped_fps(int budget_utilized);
};