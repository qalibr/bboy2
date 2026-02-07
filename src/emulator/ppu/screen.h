#pragma once

#include "emulator/ppu/ppu.h"
#include "raylib.h"

class Screen {
   public:
    Ppu& ppu;

    Screen(Ppu& p);

    int screen_scaling_factor = 3;

    void handle_input();

    void update();

    bool should_close();

    void window_terminate();

   private:
    Texture2D texture;
    Image     image;
};