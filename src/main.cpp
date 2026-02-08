#include <iostream>
#include <string>

#include "emulator/emulator.h"
#include "emulator/pak/pak.h"
#include "emulator/ppu/screen.h"

int main(int argc, char** argv) {
    std::string rom_path;
    Screen      screen;

    if (argc > 1) {
        rom_path = argv[1];
    } else {
        rom_path = screen.drag_and_drop_wait();
    }

    if (rom_path.empty()) {
        screen.window_terminate();
        return 0;
    }

    Pak pak(rom_path);
    pak.rom_info();
    pak.checksum();

    Emulator emulator(pak);
    screen.connect_ppu(emulator.ppu);

    while (!screen.should_close()) {
        emulator.joy.handle_input();

        if (emulator.joy.is_fps_uncapped()) {
            SetTargetFPS(0);
        } else {
            SetTargetFPS(60);
        }

        emulator.run_frame();
        screen.update(emulator.joy.should_display_fps());
    }

    screen.window_terminate();
    return 0;
}