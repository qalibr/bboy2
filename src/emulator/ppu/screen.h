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

    void update(bool show_fps);

    bool should_close();

    void window_terminate();

    std::string drag_and_drop_wait();

   private:
    void      display_fps();
    Texture2D texture;
    Image     image;
};