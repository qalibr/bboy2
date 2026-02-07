#pragma once

#include <memory>
#include <vector>

#include "mbc/Imbc.h"
#include "rom.h"

class Pak {
   public:
    Pak(std::string rom_path);

    std::unique_ptr<Imbc> mbc;
    Rom                   rom;

    std::vector<u8> data;
    std::string     rom_name;

    void print_header();
    void rom_info();
    void checksum();
    int  get_eram_size();
};