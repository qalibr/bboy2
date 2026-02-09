#include "emulator.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

Emulator::Emulator(Pak& p) : pak(p), mmu(pak), cpu(mmu), ppu(mmu), timer(mmu), joy(mmu) {
    mmu.set_timer(&timer);
    mmu.connect_ppu(&ppu);
    mmu.connect_joypad(&joy);
}

void Emulator::run_frame() {
    int cycles_this_frame = 0;

    while (cycles_this_frame < Ppu::CYCLES_PER_FRAME) {
        u8 cycles_ran;

        if (mmu.dma_active) {
            cycles_ran = 4;
        } else {
            cycles_ran = cpu.step();
        }

        timer.tick(cycles_ran);
        ppu.tick(cycles_ran);
        mmu.tick_dma(cycles_ran);

        cycles_this_frame += cycles_ran;
    }
}

void Emulator::save_state(const std::string& rom_path) {
    std::filesystem::path p(rom_path);
    std::string           savefile = p.stem().string() + ".sav";
    std::ofstream         out(savefile, std::ios::binary);

    if (!out.is_open()) {
        std::cerr << "Failed to open save file: " << savefile << std::endl;
        return;
    }

    SaveHeader header;
    out.write(reinterpret_cast<char*>(&header), sizeof(SaveHeader));

    MmuState mmu_state;
    mmu.save_state(mmu_state);
    out.write(reinterpret_cast<char*>(&mmu_state), sizeof(MmuState));

    CpuState cpu_state;
    cpu.save_state(cpu_state);
    out.write(reinterpret_cast<char*>(&cpu_state), sizeof(CpuState));

    PpuState ppu_state;
    ppu.save_state(ppu_state);
    out.write(reinterpret_cast<char*>(&ppu_state), sizeof(PpuState));

    TimerState timer_state;
    timer.save_state(timer_state);
    out.write(reinterpret_cast<char*>(&timer_state), sizeof(TimerState));

    if (pak.mbc) {
        pak.mbc->save_state(out);
    }

    out.close();
    std::cout << "ROM state saved to file: " << savefile << std::endl;
}

void Emulator::load_state(const std::string& rom_path) {
    std::filesystem::path p(rom_path);
    std::string           savefile = p.stem().string() + ".sav";
    std::ifstream         in(savefile, std::ios::binary);

    if (!in.is_open()) {
        std::cerr << "Could not load save file: " << savefile << std::endl;
        return;
    }

    SaveHeader header;
    in.read(reinterpret_cast<char*>(&header), sizeof(SaveHeader));

    SaveHeader expected_header;
    if (std::memcmp(header.magic, expected_header.magic, sizeof(header.magic)) != 0) {
        std::cerr << "Error: Invalid save file format." << std::endl;
        return;
    }

    if (header.version != expected_header.version) {
        if (header.version > expected_header.version) {
            std::cerr << "Error: Save file is from a newer version of the emulator." << std::endl;
        } else {
            std::cerr << "Error: Save file is from an older, incompatible version of the emulator." << std::endl;
        }
        return;
    }

    MmuState mmu_state;
    in.read(reinterpret_cast<char*>(&mmu_state), sizeof(MmuState));
    mmu.load_state(mmu_state);

    CpuState cpu_state;
    in.read(reinterpret_cast<char*>(&cpu_state), sizeof(CpuState));
    cpu.load_state(cpu_state);

    PpuState ppu_state;
    in.read(reinterpret_cast<char*>(&ppu_state), sizeof(PpuState));
    ppu.load_state(ppu_state);

    TimerState timer_state;
    in.read(reinterpret_cast<char*>(&timer_state), sizeof(TimerState));
    timer.load_state(timer_state);

    if (pak.mbc) {
        pak.mbc->load_state(in);
    }
}
