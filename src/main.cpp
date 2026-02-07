#include <iostream>
#include <string>

#include "emulator/emulator.h"
#include "emulator/pak/pak.h"
#include "emulator/ppu/screen.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <rom_path>" << std::endl;
        return 1;
    }

    std::string rom_path = argv[1];
    Pak         pak(rom_path);
    pak.rom_info();
    pak.checksum();

    Emulator emulator(pak);
    Screen   screen(emulator.ppu);

    while (!screen.should_close()) {
        emulator.run_frame();
        screen.update();
    }

    screen.window_terminate();
    return 0;
}