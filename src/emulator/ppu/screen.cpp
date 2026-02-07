#include "screen.h"

#include "ppu.h"

Screen::Screen(Ppu& p) : ppu(p) {
    int window_width  = Ppu::SCREEN_WIDTH * screen_scaling_factor;
    int window_height = Ppu::SCREEN_HEIGHT * screen_scaling_factor;

    SetTraceLogLevel(LOG_ERROR);
    InitWindow(window_width, window_height, "bboy2");
    SetTargetFPS(60);

    image   = GenImageColor(Ppu::SCREEN_WIDTH, Ppu::SCREEN_HEIGHT, BLACK);
    texture = LoadTextureFromImage(image);
}

void Screen::update() {
    UpdateTexture(texture, ppu.get_frame_buffer().data());
    BeginDrawing();
    ClearBackground(DARKGRAY);
    DrawTextureEx(texture, {0.0f, 0.0f}, 0.0f, (float)screen_scaling_factor, WHITE);
    EndDrawing();
}

bool Screen::should_close() { return WindowShouldClose(); }

void Screen::window_terminate() {
    UnloadTexture(texture);
    UnloadImage(image);
    CloseWindow();
}