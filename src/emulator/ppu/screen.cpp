#include "screen.h"

#include "ppu.h"

Screen::Screen() {
    int window_width  = Ppu::SCREEN_WIDTH * screen_scaling_factor;
    int window_height = Ppu::SCREEN_HEIGHT * screen_scaling_factor;

    SetTraceLogLevel(LOG_ERROR);
    InitWindow(window_width, window_height, "bboy2");
    SetTargetFPS(60);

    image   = GenImageColor(Ppu::SCREEN_WIDTH, Ppu::SCREEN_HEIGHT, BLACK);
    texture = LoadTextureFromImage(image);
}

void Screen::connect_ppu(Ppu& p) { ppu_ptr = &p; }

void Screen::update(bool show_fps) {
    if (ppu_ptr) UpdateTexture(texture, ppu_ptr->get_frame_buffer().data());
    BeginDrawing();
    ClearBackground(DARKGRAY);
    DrawTextureEx(texture, {0.0f, 0.0f}, 0.0f, (float)screen_scaling_factor, WHITE);

    if (show_fps) {
        display_fps();
    }

    EndDrawing();
}

bool Screen::should_close() { return WindowShouldClose(); }

void Screen::window_terminate() {
    UnloadTexture(texture);
    UnloadImage(image);
    CloseWindow();
}

std::string Screen::drag_and_drop_wait() {
    std::string rom_path;

    Font customFont = LoadFontEx("assets/font/Gilroy-Light.ttf", 64, 0, 0);
    SetTextureFilter(customFont.texture, TEXTURE_FILTER_BILINEAR);

    const char* text     = "Drop .gb ROM file here to begin...";
    float       fontSize = 20.0f;
    float       spacing  = 1.0f;

    Vector2 textSize     = MeasureTextEx(customFont, text, fontSize, spacing);
    Vector2 textPosition = {(GetScreenWidth() - textSize.x) / 2.0f, (GetScreenHeight() - textSize.y) / 2.0f};

    while (rom_path.empty() && !WindowShouldClose()) {
        if (IsFileDropped()) {
            FilePathList droppedFiles = LoadDroppedFiles();
            if (droppedFiles.count > 0 && IsFileExtension(droppedFiles.paths[0], ".gb")) {
                rom_path = droppedFiles.paths[0];
            }
            UnloadDroppedFiles(droppedFiles);
        }

        BeginDrawing();
        ClearBackground(DARKGRAY);
        DrawTextEx(customFont, text, textPosition, fontSize, spacing, LIGHTGRAY);
        EndDrawing();
    }

    UnloadFont(customFont);
    return rom_path;
}

void Screen::display_fps() {
    const char* text      = TextFormat("%i", GetFPS());
    int         fontSize  = 20;
    int         textWidth = MeasureText(text, fontSize);
    DrawText(text, GetScreenWidth() - textWidth - 10, 10, fontSize, LIGHTGRAY);
}
