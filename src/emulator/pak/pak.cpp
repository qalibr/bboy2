#include "pak.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>

#include "mbc/mbc.h"
#include "mbc/mbc0.h"
#include "mbc/mbc1.h"
#include "mbc/mbc3.h"

Pak::Pak(std::string rom_path) {
    rom_name = rom_path;

    std::ifstream file(rom_path, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        std::cerr << "Error: Failed to open file: " << rom_path << std::endl;
        return;
    }

    std::streamsize size = file.tellg();

    file.seekg(0, std::ios::beg);

    this->data.resize(size);

    if (file.read(reinterpret_cast<char*>(this->data.data()), size)) {
        print_header();
    } else {
        std::cerr << "Error: Failed to read file data." << std::endl;
        return;
    }

    int actual_eram_size = get_eram_size();

    switch (rom.type) {
        case 0x00:
            mbc = std::make_unique<Mbc0>(*this, actual_eram_size);
            break;
        case 0x01:
        case 0x02:
        case 0x03:
            mbc = std::make_unique<Mbc1>(*this, actual_eram_size);
            break;
        case 0x0F:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
            mbc = std::make_unique<Mbc3>(*this, actual_eram_size);
            break;
        default:
            std::cerr << "FATAL: Unsupported MBC type: 0x" << std::hex << (int)rom.type << std::dec << std::endl;
            std::exit(EXIT_FAILURE);
            break;
    }
}

void Pak::print_header() {
    std::copy_n(&data[0x134], 16, this->rom.title);

    u8 license_lo         = data[0x144];
    u8 license_hi         = data[0x145];
    this->rom.sgb_flag    = data[0x146];
    this->rom.type        = data[0x147];
    this->rom.rom_size    = data[0x148];
    this->rom.ram_size    = data[0x149];
    this->rom.destination = data[0x14A];
    this->rom.license     = data[0x14B];
    this->rom.version     = data[0x14C];
    this->rom.checksum    = data[0x14D];
    u8 checksum_lo        = data[0x14E];
    u8 checksum_hi        = data[0x14F];

    this->rom.new_license     = license_lo | (license_hi << 8);
    this->rom.global_checksum = checksum_lo | (checksum_hi << 8);
}

void Pak::rom_info() {
    std::cout << "Loaded ROM: " << rom_name << std::endl;
    std::cout << "\t[" << std::string(this->rom.title, 16) << "]" << std::endl;

    auto type_it = Rom::RomTypes.find(this->rom.type);
    if (type_it != Rom::RomTypes.end()) {
        std::cout << "\t-- " << type_it->second << std::endl;
    } else {
        std::cout << "\t-- Unknown (0x" << std::hex << (int)this->rom.type << std::dec << ")" << std::endl;
    }

    int rom_kb = 32 * (1 << this->rom.rom_size);
    int ram_kb = get_eram_size() / 1024;

    if (rom_kb >= 1024) {
        std::cout << "\t-- " << rom_kb / 1024 << "MiB (ROM)" << std::endl;
    } else {
        std::cout << "\t-- " << rom_kb << "KiB (ROM)" << std::endl;
    }

    if (ram_kb >= 1024) {
        std::cout << "\t-- " << ram_kb / 1024 << "MiB (RAM)" << std::endl;
    } else {
        std::cout << "\t-- " << ram_kb << "KiB (RAM)" << std::endl;
    }

    auto lic_it = Rom::LicenseCodes.find(this->rom.license);
    if (lic_it != Rom::LicenseCodes.end()) {
        std::cout << "\t-- " << lic_it->second << " (License)" << std::endl;
    } else {
        std::cout << "\t-- Unknown (0x" << std::hex << (int)this->rom.license << std::dec << ") (License)" << std::endl;
    }
}

void Pak::checksum() {
    u16 calculated_checksum = 0;

    for (int i = 0x134; i <= 0x14C; i++) {
        calculated_checksum = calculated_checksum - data[i] - 1;
    }

    u8 result = calculated_checksum & 0xFF;

    std::cout << "\t-- Checksum: 0x" << std::hex << (int)result << " (Expected: 0x" << std::hex
              << (int)this->rom.checksum << ")" << std::dec
              << (result == this->rom.checksum ? " [PASSED]" : " [FAILED]") << std::endl;
}

int Pak::get_eram_size() {
    switch (this->rom.ram_size) {
        case 0x00:
            return 0;
        case 0x01:
            return 2 * 1024;
        case 0x02:
            return 8 * 1024;
        case 0x03:
            return 32 * 1024;
        case 0x04:
            return 128 * 1024;
        case 0x05:
            return 64 * 1024;
        default:
            std::cerr << "Error: Invalid RAM size code." << std::endl;
            return 0;
    }
}