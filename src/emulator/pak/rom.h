#pragma once

#include <string>
#include <unordered_map>

class Rom {
   public:
    Rom();

    u8   entry[4];         // 0x0100-0x0103: Entry point
    u8   logo[0x30];       // 0x0104-0x0133: Nintendo logo
    char title[16];        // 0x0134-0x0143: Game Title
    u16  new_license;      // 0x0144-0x0145
    u8   sgb_flag;         // 0x0146
    u8   type;             // 0x0147
    u8   rom_size;         // 0x0148
    u8   ram_size;         // 0x0149
    u8   destination;      // 0x014A
    u8   license;          // 0x014B
    u8   version;          // 0x014C
    u8   checksum;         // 0x014D
    u16  global_checksum;  // 0x014E-0x014F

    static const std::unordered_map<u8, std::string> RomTypes;
    static const std::unordered_map<u8, std::string> LicenseCodes;
};