#pragma once

#include <string>

#include "emulator/ppu/ppu.h"
#include "raylib.h"

class Screen {
   public:
    Ppu* ppu_ptr = nullptr;

    Screen();

    int screen_scaling_factor = 3;

    void connect_ppu(Ppu& p);

    void update();

    bool should_close();

    void window_terminate();

    std::string drag_and_drop_wait();

   private:
    Texture2D texture;
    Image     image;
};