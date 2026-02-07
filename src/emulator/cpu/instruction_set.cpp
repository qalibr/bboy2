#include <iostream>

#include "cpu.h"

void Cpu::UNIMPLEMENTED() {
    std::cerr << "FATAL: UNIMPLEMENTED" << std::endl;
    exit(1);
}

void Cpu::CB_UNIMPLEMENTED() {
    std::cerr << "FATAL: CB_UNIMPLEMENTED" << std::endl;
    exit(1);
}

void Cpu::init_instructions() {
    Instruction empty = {"???", &Cpu::UNIMPLEMENTED, 0, 1};
    instructions.fill(empty);

    Instruction cb_empty = {"???", &Cpu::CB_UNIMPLEMENTED, 0, 1};
    cb_instructions.fill(cb_empty);

    // =============================================================
    //  Control & Misc
    // =============================================================
    instructions[0xCB] = {"PREFIX CB", &Cpu::PREFIX, 4, 1};
    instructions[0x00] = {"NOP", &Cpu::NOP, 4, 1};
    instructions[0x10] = {"STOP", &Cpu::STOP, 4, 2};
    instructions[0x76] = {"HALT", &Cpu::HALT, 4, 1};
    instructions[0xF3] = {"DI", &Cpu::DI, 4, 1};
    instructions[0xFB] = {"EI", &Cpu::EI, 4, 1};

    // =============================================================
    //  Jump Calls
    // =============================================================
    instructions[0x20] = {"JR NZ, e8", &Cpu::JR_NZ_e8, 8, 2};
    instructions[0x30] = {"JR NC, e8", &Cpu::JR_NC_e8, 8, 2};
    instructions[0x18] = {"JR e8", &Cpu::JR_e8, 12, 2};
    instructions[0x28] = {"JR Z, e8", &Cpu::JR_Z_e8, 8, 2};
    instructions[0x38] = {"JR C, e8", &Cpu::JR_C_e8, 8, 2};
    instructions[0xC0] = {"RET NZ", &Cpu::RET_NZ, 8, 1};
    instructions[0xD0] = {"RET NC", &Cpu::RET_NC, 8, 1};
    instructions[0xC2] = {"JP NZ, a16", &Cpu::JP_NZ_a16, 12, 3};
    instructions[0xD2] = {"JP NC, a16", &Cpu::JP_NC_a16, 12, 3};
    instructions[0xC3] = {"JP a16", &Cpu::JP_a16, 16, 3};
    instructions[0xC4] = {"CALL NZ, a16", &Cpu::CALL_NZ_a16, 12, 3};
    instructions[0xD4] = {"CALL NC, a16", &Cpu::CALL_NC_a16, 12, 3};
    instructions[0xC7] = {"RST 00H", &Cpu::RST_00, 16, 1};
    instructions[0xD7] = {"RST 10H", &Cpu::RST_10, 16, 1};
    instructions[0xE7] = {"RST 20H", &Cpu::RST_20, 16, 1};
    instructions[0xF7] = {"RST 30H", &Cpu::RST_30, 16, 1};
    instructions[0xC8] = {"RET Z", &Cpu::RET_Z, 8, 1};
    instructions[0xD8] = {"RET C", &Cpu::RET_C, 8, 1};
    instructions[0xC9] = {"RET", &Cpu::RET, 16, 1};
    instructions[0xD9] = {"RETI", &Cpu::RETI, 16, 1};
    instructions[0xE9] = {"JP HL", &Cpu::JP_HL, 4, 1};
    instructions[0xCA] = {"JP Z, a16", &Cpu::JP_Z_a16, 12, 3};
    instructions[0xDA] = {"JP C, a16", &Cpu::JP_C_a16, 12, 3};
    instructions[0xCC] = {"CALL Z, a16", &Cpu::CALL_Z_a16, 12, 3};
    instructions[0xDC] = {"CALL C, a16", &Cpu::CALL_C_a16, 12, 3};
    instructions[0xCD] = {"CALL a16", &Cpu::CALL_a16, 24, 3};
    instructions[0xCF] = {"RST 08H", &Cpu::RST_08, 16, 1};
    instructions[0xDF] = {"RST 18H", &Cpu::RST_18, 16, 1};
    instructions[0xEF] = {"RST 28H", &Cpu::RST_28, 16, 1};
    instructions[0xFF] = {"RST 38H", &Cpu::RST_38, 16, 1};

    // =============================================================
    //  8-bit Load
    // =============================================================
    instructions[0x02] = {"LD [BC], A", &Cpu::LD_BC_a16_A, 8, 1};
    instructions[0x12] = {"LD [DE], A", &Cpu::LD_DE_a16_A, 8, 1};
    instructions[0x22] = {"LD [HL+], A", &Cpu::LD_HLi_a16_A, 8, 1};
    instructions[0x32] = {"LD [HL-], A", &Cpu::LD_HLd_a16_A, 8, 1};
    instructions[0x06] = {"LD B, n8", &Cpu::LD_B_n8, 8, 2};
    instructions[0x16] = {"LD D, n8", &Cpu::LD_D_n8, 8, 2};
    instructions[0x26] = {"LD H, n8", &Cpu::LD_H_n8, 8, 2};
    instructions[0x36] = {"LD [HL], n8", &Cpu::LD_HL_a16_n8, 12, 2};
    instructions[0x0A] = {"LD A, [BC]", &Cpu::LD_A_BC_a16, 8, 1};
    instructions[0x1A] = {"LD A, [DE]", &Cpu::LD_A_DE_a16, 8, 1};
    instructions[0x2A] = {"LD A, [HL+]", &Cpu::LD_A_HLi_a16, 8, 1};
    instructions[0x3A] = {"LD A, [HL-]", &Cpu::LD_A_HLd_a16, 8, 1};
    instructions[0x0E] = {"LD C, n8", &Cpu::LD_C_n8, 8, 2};
    instructions[0x1E] = {"LD E, n8", &Cpu::LD_E_n8, 8, 2};
    instructions[0x2E] = {"LD L, n8", &Cpu::LD_L_n8, 8, 2};
    instructions[0x3E] = {"LD A, n8", &Cpu::LD_A_n8, 8, 2};

    // =============================================================
    //  LD r, r'
    // =============================================================
    instructions[0x40] = {"LD B, B", &Cpu::LD_B_B, 4, 1};
    instructions[0x41] = {"LD B, C", &Cpu::LD_B_C, 4, 1};
    instructions[0x42] = {"LD B, D", &Cpu::LD_B_D, 4, 1};
    instructions[0x43] = {"LD B, E", &Cpu::LD_B_E, 4, 1};
    instructions[0x44] = {"LD B, H", &Cpu::LD_B_H, 4, 1};
    instructions[0x45] = {"LD B, L", &Cpu::LD_B_L, 4, 1};
    instructions[0x46] = {"LD B, [HL]", &Cpu::LD_B_HL_a16, 8, 1};
    instructions[0x47] = {"LD B, A", &Cpu::LD_B_A, 4, 1};
    instructions[0x48] = {"LD C, B", &Cpu::LD_C_B, 4, 1};
    instructions[0x49] = {"LD C, C", &Cpu::LD_C_C, 4, 1};
    instructions[0x4A] = {"LD C, D", &Cpu::LD_C_D, 4, 1};
    instructions[0x4B] = {"LD C, E", &Cpu::LD_C_E, 4, 1};
    instructions[0x4C] = {"LD C, H", &Cpu::LD_C_H, 4, 1};
    instructions[0x4D] = {"LD C, L", &Cpu::LD_C_L, 4, 1};
    instructions[0x4E] = {"LD C, [HL]", &Cpu::LD_C_HL_a16, 8, 1};
    instructions[0x4F] = {"LD C, A", &Cpu::LD_C_A, 4, 1};
    instructions[0x50] = {"LD D, B", &Cpu::LD_D_B, 4, 1};
    instructions[0x51] = {"LD D, C", &Cpu::LD_D_C, 4, 1};
    instructions[0x52] = {"LD D, D", &Cpu::LD_D_D, 4, 1};
    instructions[0x53] = {"LD D, E", &Cpu::LD_D_E, 4, 1};
    instructions[0x54] = {"LD D, H", &Cpu::LD_D_H, 4, 1};
    instructions[0x55] = {"LD D, L", &Cpu::LD_D_L, 4, 1};
    instructions[0x56] = {"LD D, [HL]", &Cpu::LD_D_HL_a16, 8, 1};
    instructions[0x57] = {"LD D, A", &Cpu::LD_D_A, 4, 1};
    instructions[0x58] = {"LD E, B", &Cpu::LD_E_B, 4, 1};
    instructions[0x59] = {"LD E, C", &Cpu::LD_E_C, 4, 1};
    instructions[0x5A] = {"LD E, D", &Cpu::LD_E_D, 4, 1};
    instructions[0x5B] = {"LD E, E", &Cpu::LD_E_E, 4, 1};
    instructions[0x5C] = {"LD E, H", &Cpu::LD_E_H, 4, 1};
    instructions[0x5D] = {"LD E, L", &Cpu::LD_E_L, 4, 1};
    instructions[0x5E] = {"LD E, [HL]", &Cpu::LD_E_HL_a16, 8, 1};
    instructions[0x5F] = {"LD E, A", &Cpu::LD_E_A, 4, 1};
    instructions[0x60] = {"LD H, B", &Cpu::LD_H_B, 4, 1};
    instructions[0x61] = {"LD H, C", &Cpu::LD_H_C, 4, 1};
    instructions[0x62] = {"LD H, D", &Cpu::LD_H_D, 4, 1};
    instructions[0x63] = {"LD H, E", &Cpu::LD_H_E, 4, 1};
    instructions[0x64] = {"LD H, H", &Cpu::LD_H_H, 4, 1};
    instructions[0x65] = {"LD H, L", &Cpu::LD_H_L, 4, 1};
    instructions[0x66] = {"LD H, [HL]", &Cpu::LD_H_HL_a16, 8, 1};
    instructions[0x67] = {"LD H, A", &Cpu::LD_H_A, 4, 1};
    instructions[0x68] = {"LD L, B", &Cpu::LD_L_B, 4, 1};
    instructions[0x69] = {"LD L, C", &Cpu::LD_L_C, 4, 1};
    instructions[0x6A] = {"LD L, D", &Cpu::LD_L_D, 4, 1};
    instructions[0x6B] = {"LD L, E", &Cpu::LD_L_E, 4, 1};
    instructions[0x6C] = {"LD L, H", &Cpu::LD_L_H, 4, 1};
    instructions[0x6D] = {"LD L, L", &Cpu::LD_L_L, 4, 1};
    instructions[0x6E] = {"LD L, [HL]", &Cpu::LD_L_HL_a16, 8, 1};
    instructions[0x6F] = {"LD L, A", &Cpu::LD_L_A, 4, 1};
    instructions[0x70] = {"LD [HL], B", &Cpu::LD_HL_a16_B, 8, 1};
    instructions[0x71] = {"LD [HL], C", &Cpu::LD_HL_a16_C, 8, 1};
    instructions[0x72] = {"LD [HL], D", &Cpu::LD_HL_a16_D, 8, 1};
    instructions[0x73] = {"LD [HL], E", &Cpu::LD_HL_a16_E, 8, 1};
    instructions[0x74] = {"LD [HL], H", &Cpu::LD_HL_a16_H, 8, 1};
    instructions[0x75] = {"LD [HL], L", &Cpu::LD_HL_a16_L, 8, 1};
    instructions[0x77] = {"LD [HL], A", &Cpu::LD_HL_a16_A, 8, 1};
    instructions[0x78] = {"LD A, B", &Cpu::LD_A_B, 4, 1};
    instructions[0x79] = {"LD A, C", &Cpu::LD_A_C, 4, 1};
    instructions[0x7A] = {"LD A, D", &Cpu::LD_A_D, 4, 1};
    instructions[0x7B] = {"LD A, E", &Cpu::LD_A_E, 4, 1};
    instructions[0x7C] = {"LD A, H", &Cpu::LD_A_H, 4, 1};
    instructions[0x7D] = {"LD A, L", &Cpu::LD_A_L, 4, 1};
    instructions[0x7E] = {"LD A, [HL]", &Cpu::LD_A_HL_a16, 8, 1};
    instructions[0x7F] = {"LD A, A", &Cpu::LD_A_A, 4, 1};

    // =============================================================
    //  Other 8-bit Load
    // =============================================================
    instructions[0xE0] = {"LDH (a8), A", &Cpu::LDH_a8_A, 12, 2};
    instructions[0xF0] = {"LDH A, (a8)", &Cpu::LDH_A_a8, 12, 2};
    instructions[0xE2] = {"LD (C), A", &Cpu::LD_C_a16_A, 8, 1};
    instructions[0xF2] = {"LD A, (C)", &Cpu::LD_A_C_a16, 8, 1};
    instructions[0xEA] = {"LD (a16), A", &Cpu::LD_a16_A, 16, 3};
    instructions[0xFA] = {"LD A, (a16)", &Cpu::LD_A_a16, 16, 3};

    // =============================================================
    //  16-bit Load (& Stack)
    // =============================================================
    instructions[0x01] = {"LD BC, n16", &Cpu::LD_BC_n16, 12, 3};
    instructions[0x11] = {"LD DE, n16", &Cpu::LD_DE_n16, 12, 3};
    instructions[0x21] = {"LD HL, n16", &Cpu::LD_HL_n16, 12, 3};
    instructions[0x31] = {"LD SP, n16", &Cpu::LD_SP_n16, 12, 3};
    instructions[0x08] = {"LD (a16), SP", &Cpu::LD_a16_SP, 20, 3};
    instructions[0xC1] = {"POP BC", &Cpu::POP_BC, 12, 1};
    instructions[0xD1] = {"POP DE", &Cpu::POP_DE, 12, 1};
    instructions[0xE1] = {"POP HL", &Cpu::POP_HL, 12, 1};
    instructions[0xF1] = {"POP AF", &Cpu::POP_AF, 12, 1};
    instructions[0xC5] = {"PUSH BC", &Cpu::PUSH_BC, 16, 1};
    instructions[0xD5] = {"PUSH DE", &Cpu::PUSH_DE, 16, 1};
    instructions[0xE5] = {"PUSH HL", &Cpu::PUSH_HL, 16, 1};
    instructions[0xF5] = {"PUSH AF", &Cpu::PUSH_AF, 16, 1};
    instructions[0xF8] = {"LD HL, SP+e8", &Cpu::LD_HL_SP_e8, 12, 2};
    instructions[0xF9] = {"LD SP, HL", &Cpu::LD_SP_HL, 8, 1};

    // =============================================================
    //  8-bit Arithmetic & Logical
    // =============================================================
    instructions[0x04] = {"INC B", &Cpu::INC_B, 4, 1};
    instructions[0x14] = {"INC D", &Cpu::INC_D, 4, 1};
    instructions[0x24] = {"INC H", &Cpu::INC_H, 4, 1};
    instructions[0x34] = {"INC [HL]", &Cpu::INC_HL_a16, 12, 1};
    instructions[0x05] = {"DEC B", &Cpu::DEC_B, 4, 1};
    instructions[0x15] = {"DEC D", &Cpu::DEC_D, 4, 1};
    instructions[0x25] = {"DEC H", &Cpu::DEC_H, 4, 1};
    instructions[0x35] = {"DEC [HL]", &Cpu::DEC_HL_a16, 12, 1};
    instructions[0x27] = {"DAA", &Cpu::DAA, 4, 1};
    instructions[0x37] = {"SCF", &Cpu::SCF, 4, 1};
    instructions[0x0C] = {"INC C", &Cpu::INC_C, 4, 1};
    instructions[0x1C] = {"INC E", &Cpu::INC_E, 4, 1};
    instructions[0x2C] = {"INC L", &Cpu::INC_L, 4, 1};
    instructions[0x3C] = {"INC A", &Cpu::INC_A, 4, 1};
    instructions[0x0D] = {"DEC C", &Cpu::DEC_C, 4, 1};
    instructions[0x1D] = {"DEC E", &Cpu::DEC_E, 4, 1};
    instructions[0x2D] = {"DEC L", &Cpu::DEC_L, 4, 1};
    instructions[0x3D] = {"DEC A", &Cpu::DEC_A, 4, 1};
    instructions[0x2F] = {"CPL", &Cpu::CPL, 4, 1};
    instructions[0x3F] = {"CCF", &Cpu::CCF, 4, 1};
    instructions[0x80] = {"ADD A, B", &Cpu::ADD_A_B, 4, 1};
    instructions[0x81] = {"ADD A, C", &Cpu::ADD_A_C, 4, 1};
    instructions[0x82] = {"ADD A, D", &Cpu::ADD_A_D, 4, 1};
    instructions[0x83] = {"ADD A, E", &Cpu::ADD_A_E, 4, 1};
    instructions[0x84] = {"ADD A, H", &Cpu::ADD_A_H, 4, 1};
    instructions[0x85] = {"ADD A, L", &Cpu::ADD_A_L, 4, 1};
    instructions[0x86] = {"ADD A, [HL]", &Cpu::ADD_A_HL_a16, 8, 1};
    instructions[0x87] = {"ADD A, A", &Cpu::ADD_A_A, 4, 1};
    instructions[0x88] = {"ADC A, B", &Cpu::ADC_A_B, 4, 1};
    instructions[0x89] = {"ADC A, C", &Cpu::ADC_A_C, 4, 1};
    instructions[0x8A] = {"ADC A, D", &Cpu::ADC_A_D, 4, 1};
    instructions[0x8B] = {"ADC A, E", &Cpu::ADC_A_E, 4, 1};
    instructions[0x8C] = {"ADC A, H", &Cpu::ADC_A_H, 4, 1};
    instructions[0x8D] = {"ADC A, L", &Cpu::ADC_A_L, 4, 1};
    instructions[0x8E] = {"ADC A, [HL]", &Cpu::ADC_A_HL_a16, 8, 1};
    instructions[0x8F] = {"ADC A, A", &Cpu::ADC_A_A, 4, 1};
    instructions[0x90] = {"SUB A, B", &Cpu::SUB_A_B, 4, 1};
    instructions[0x91] = {"SUB A, C", &Cpu::SUB_A_C, 4, 1};
    instructions[0x92] = {"SUB A, D", &Cpu::SUB_A_D, 4, 1};
    instructions[0x93] = {"SUB A, E", &Cpu::SUB_A_E, 4, 1};
    instructions[0x94] = {"SUB A, H", &Cpu::SUB_A_H, 4, 1};
    instructions[0x95] = {"SUB A, L", &Cpu::SUB_A_L, 4, 1};
    instructions[0x96] = {"SUB A, [HL]", &Cpu::SUB_A_HL_a16, 8, 1};
    instructions[0x97] = {"SUB A, A", &Cpu::SUB_A_A, 4, 1};
    instructions[0x98] = {"SBC A, B", &Cpu::SBC_A_B, 4, 1};
    instructions[0x99] = {"SBC A, C", &Cpu::SBC_A_C, 4, 1};
    instructions[0x9A] = {"SBC A, D", &Cpu::SBC_A_D, 4, 1};
    instructions[0x9B] = {"SBC A, E", &Cpu::SBC_A_E, 4, 1};
    instructions[0x9C] = {"SBC A, H", &Cpu::SBC_A_H, 4, 1};
    instructions[0x9D] = {"SBC A, L", &Cpu::SBC_A_L, 4, 1};
    instructions[0x9E] = {"SBC A, [HL]", &Cpu::SBC_A_HL_a16, 8, 1};
    instructions[0x9F] = {"SBC A, A", &Cpu::SBC_A_A, 4, 1};
    instructions[0xA0] = {"AND A, B", &Cpu::AND_A_B, 4, 1};
    instructions[0xA1] = {"AND A, C", &Cpu::AND_A_C, 4, 1};
    instructions[0xA2] = {"AND A, D", &Cpu::AND_A_D, 4, 1};
    instructions[0xA3] = {"AND A, E", &Cpu::AND_A_E, 4, 1};
    instructions[0xA4] = {"AND A, H", &Cpu::AND_A_H, 4, 1};
    instructions[0xA5] = {"AND A, L", &Cpu::AND_A_L, 4, 1};
    instructions[0xA6] = {"AND A, [HL]", &Cpu::AND_A_HL_a16, 8, 1};
    instructions[0xA7] = {"AND A, A", &Cpu::AND_A_A, 4, 1};
    instructions[0xA8] = {"XOR A, B", &Cpu::XOR_A_B, 4, 1};
    instructions[0xA9] = {"XOR A, C", &Cpu::XOR_A_C, 4, 1};
    instructions[0xAA] = {"XOR A, D", &Cpu::XOR_A_D, 4, 1};
    instructions[0xAB] = {"XOR A, E", &Cpu::XOR_A_E, 4, 1};
    instructions[0xAC] = {"XOR A, H", &Cpu::XOR_A_H, 4, 1};
    instructions[0xAD] = {"XOR A, L", &Cpu::XOR_A_L, 4, 1};
    instructions[0xAE] = {"XOR A, [HL]", &Cpu::XOR_A_HL_a16, 8, 1};
    instructions[0xAF] = {"XOR A, A", &Cpu::XOR_A_A, 4, 1};
    instructions[0xB0] = {"OR A, B", &Cpu::OR_A_B, 4, 1};
    instructions[0xB1] = {"OR A, C", &Cpu::OR_A_C, 4, 1};
    instructions[0xB2] = {"OR A, D", &Cpu::OR_A_D, 4, 1};
    instructions[0xB3] = {"OR A, E", &Cpu::OR_A_E, 4, 1};
    instructions[0xB4] = {"OR A, H", &Cpu::OR_A_H, 4, 1};
    instructions[0xB5] = {"OR A, L", &Cpu::OR_A_L, 4, 1};
    instructions[0xB6] = {"OR A, [HL]", &Cpu::OR_A_HL_a16, 8, 1};
    instructions[0xB7] = {"OR A, A", &Cpu::OR_A_A, 4, 1};
    instructions[0xB8] = {"CP A, B", &Cpu::CP_A_B, 4, 1};
    instructions[0xB9] = {"CP A, C", &Cpu::CP_A_C, 4, 1};
    instructions[0xBA] = {"CP A, D", &Cpu::CP_A_D, 4, 1};
    instructions[0xBB] = {"CP A, E", &Cpu::CP_A_E, 4, 1};
    instructions[0xBC] = {"CP A, H", &Cpu::CP_A_H, 4, 1};
    instructions[0xBD] = {"CP A, L", &Cpu::CP_A_L, 4, 1};
    instructions[0xBE] = {"CP A, [HL]", &Cpu::CP_A_HL_a16, 8, 1};
    instructions[0xBF] = {"CP A, A", &Cpu::CP_A_A, 4, 1};
    instructions[0xC6] = {"ADD A, n8", &Cpu::ADD_A_n8, 8, 2};
    instructions[0xD6] = {"SUB A, n8", &Cpu::SUB_A_n8, 8, 2};
    instructions[0xE6] = {"AND A, n8", &Cpu::AND_A_n8, 8, 2};
    instructions[0xF6] = {"OR A, n8", &Cpu::OR_A_n8, 8, 2};
    instructions[0xCE] = {"ADC A, n8", &Cpu::ADC_A_n8, 8, 2};
    instructions[0xDE] = {"SBC A, n8", &Cpu::SBC_A_n8, 8, 2};
    instructions[0xEE] = {"XOR A, n8", &Cpu::XOR_A_n8, 8, 2};
    instructions[0xFE] = {"CP A, n8", &Cpu::CP_A_n8, 8, 2};

    // =============================================================
    //  16-bit Arithmetic & Logical
    // =============================================================
    instructions[0x03] = {"INC BC", &Cpu::INC_BC, 8, 1};
    instructions[0x13] = {"INC DE", &Cpu::INC_DE, 8, 1};
    instructions[0x23] = {"INC HL", &Cpu::INC_HL, 8, 1};
    instructions[0x33] = {"INC SP", &Cpu::INC_SP, 8, 1};
    instructions[0x09] = {"ADD HL, BC", &Cpu::ADD_HL_BC, 8, 1};
    instructions[0x19] = {"ADD HL, DE", &Cpu::ADD_HL_DE, 8, 1};
    instructions[0x29] = {"ADD HL, HL", &Cpu::ADD_HL_HL, 8, 1};
    instructions[0x39] = {"ADD HL, SP", &Cpu::ADD_HL_SP, 8, 1};
    instructions[0x0B] = {"DEC BC", &Cpu::DEC_BC, 8, 1};
    instructions[0x1B] = {"DEC DE", &Cpu::DEC_DE, 8, 1};
    instructions[0x2B] = {"DEC HL", &Cpu::DEC_HL, 8, 1};
    instructions[0x3B] = {"DEC SP", &Cpu::DEC_SP, 8, 1};
    instructions[0xE8] = {"ADD SP, e8", &Cpu::ADD_SP_e8, 16, 2};

    // =============================================================
    //  8-bit Bit Ops
    // =============================================================
    instructions[0x07] = {"RLCA", &Cpu::RLCA, 4, 1};
    instructions[0x17] = {"RLA", &Cpu::RLA, 4, 1};
    instructions[0x0F] = {"RRCA", &Cpu::RRCA, 4, 1};
    instructions[0x1F] = {"RRA", &Cpu::RRA, 4, 1};

    // =============================================================
    //  PREFIXED
    // =============================================================
    cb_instructions[0x00] = {"RLC B", &Cpu::RLC_B, 8, 2};
    cb_instructions[0x01] = {"RLC C", &Cpu::RLC_C, 8, 2};
    cb_instructions[0x02] = {"RLC D", &Cpu::RLC_D, 8, 2};
    cb_instructions[0x03] = {"RLC E", &Cpu::RLC_E, 8, 2};
    cb_instructions[0x04] = {"RLC H", &Cpu::RLC_H, 8, 2};
    cb_instructions[0x05] = {"RLC L", &Cpu::RLC_L, 8, 2};
    cb_instructions[0x06] = {"RLC [HL]", &Cpu::RLC_HL_a16, 16, 2};
    cb_instructions[0x07] = {"RLC A", &Cpu::RLC_A, 8, 2};
    cb_instructions[0x08] = {"RRC B", &Cpu::RRC_B, 8, 2};
    cb_instructions[0x09] = {"RRC C", &Cpu::RRC_C, 8, 2};
    cb_instructions[0x0A] = {"RRC D", &Cpu::RRC_D, 8, 2};
    cb_instructions[0x0B] = {"RRC E", &Cpu::RRC_E, 8, 2};
    cb_instructions[0x0C] = {"RRC H", &Cpu::RRC_H, 8, 2};
    cb_instructions[0x0D] = {"RRC L", &Cpu::RRC_L, 8, 2};
    cb_instructions[0x0E] = {"RRC [HL]", &Cpu::RRC_HL_a16, 16, 2};
    cb_instructions[0x0F] = {"RRC A", &Cpu::RRC_A, 8, 2};

    cb_instructions[0x10] = {"RL B", &Cpu::RL_B, 8, 2};
    cb_instructions[0x11] = {"RL C", &Cpu::RL_C, 8, 2};
    cb_instructions[0x12] = {"RL D", &Cpu::RL_D, 8, 2};
    cb_instructions[0x13] = {"RL E", &Cpu::RL_E, 8, 2};
    cb_instructions[0x14] = {"RL H", &Cpu::RL_H, 8, 2};
    cb_instructions[0x15] = {"RL L", &Cpu::RL_L, 8, 2};
    cb_instructions[0x16] = {"RL [HL]", &Cpu::RL_HL_a16, 16, 2};
    cb_instructions[0x17] = {"RL A", &Cpu::RL_A, 8, 2};
    cb_instructions[0x18] = {"RR B", &Cpu::RR_B, 8, 2};
    cb_instructions[0x19] = {"RR C", &Cpu::RR_C, 8, 2};
    cb_instructions[0x1A] = {"RR D", &Cpu::RR_D, 8, 2};
    cb_instructions[0x1B] = {"RR E", &Cpu::RR_E, 8, 2};
    cb_instructions[0x1C] = {"RR H", &Cpu::RR_H, 8, 2};
    cb_instructions[0x1D] = {"RR L", &Cpu::RR_L, 8, 2};
    cb_instructions[0x1E] = {"RR [HL]", &Cpu::RR_HL_a16, 16, 2};
    cb_instructions[0x1F] = {"RR A", &Cpu::RR_A, 8, 2};

    cb_instructions[0x20] = {"SLA B", &Cpu::SLA_B, 8, 2};
    cb_instructions[0x21] = {"SLA C", &Cpu::SLA_C, 8, 2};
    cb_instructions[0x22] = {"SLA D", &Cpu::SLA_D, 8, 2};
    cb_instructions[0x23] = {"SLA E", &Cpu::SLA_E, 8, 2};
    cb_instructions[0x24] = {"SLA H", &Cpu::SLA_H, 8, 2};
    cb_instructions[0x25] = {"SLA L", &Cpu::SLA_L, 8, 2};
    cb_instructions[0x26] = {"SLA [HL]", &Cpu::SLA_HL_a16, 16, 2};
    cb_instructions[0x27] = {"SLA A", &Cpu::SLA_A, 8, 2};
    cb_instructions[0x28] = {"SRA B", &Cpu::SRA_B, 8, 2};
    cb_instructions[0x29] = {"SRA C", &Cpu::SRA_C, 8, 2};
    cb_instructions[0x2A] = {"SRA D", &Cpu::SRA_D, 8, 2};
    cb_instructions[0x2B] = {"SRA E", &Cpu::SRA_E, 8, 2};
    cb_instructions[0x2C] = {"SRA H", &Cpu::SRA_H, 8, 2};
    cb_instructions[0x2D] = {"SRA L", &Cpu::SRA_L, 8, 2};
    cb_instructions[0x2E] = {"SRA [HL]", &Cpu::SRA_HL_a16, 16, 2};
    cb_instructions[0x2F] = {"SRA A", &Cpu::SRA_A, 8, 2};

    cb_instructions[0x30] = {"SWAP B", &Cpu::SWAP_B, 8, 2};
    cb_instructions[0x31] = {"SWAP C", &Cpu::SWAP_C, 8, 2};
    cb_instructions[0x32] = {"SWAP D", &Cpu::SWAP_D, 8, 2};
    cb_instructions[0x33] = {"SWAP E", &Cpu::SWAP_E, 8, 2};
    cb_instructions[0x34] = {"SWAP H", &Cpu::SWAP_H, 8, 2};
    cb_instructions[0x35] = {"SWAP L", &Cpu::SWAP_L, 8, 2};
    cb_instructions[0x36] = {"SWAP [HL]", &Cpu::SWAP_HL_a16, 16, 2};
    cb_instructions[0x37] = {"SWAP A", &Cpu::SWAP_A, 8, 2};
    cb_instructions[0x38] = {"SRL B", &Cpu::SRL_B, 8, 2};
    cb_instructions[0x39] = {"SRL C", &Cpu::SRL_C, 8, 2};
    cb_instructions[0x3A] = {"SRL D", &Cpu::SRL_D, 8, 2};
    cb_instructions[0x3B] = {"SRL E", &Cpu::SRL_E, 8, 2};
    cb_instructions[0x3C] = {"SRL H", &Cpu::SRL_H, 8, 2};
    cb_instructions[0x3D] = {"SRL L", &Cpu::SRL_L, 8, 2};
    cb_instructions[0x3E] = {"SRL [HL]", &Cpu::SRL_HL_a16, 16, 2};
    cb_instructions[0x3F] = {"SRL A", &Cpu::SRL_A, 8, 2};

    cb_instructions[0x40] = {"BIT 0, B", &Cpu::BIT_0_B, 8, 2};
    cb_instructions[0x41] = {"BIT 0, C", &Cpu::BIT_0_C, 8, 2};
    cb_instructions[0x42] = {"BIT 0, D", &Cpu::BIT_0_D, 8, 2};
    cb_instructions[0x43] = {"BIT 0, E", &Cpu::BIT_0_E, 8, 2};
    cb_instructions[0x44] = {"BIT 0, H", &Cpu::BIT_0_H, 8, 2};
    cb_instructions[0x45] = {"BIT 0, L", &Cpu::BIT_0_L, 8, 2};
    cb_instructions[0x46] = {"BIT 0, [HL]", &Cpu::BIT_0_HL_a16, 12, 2};
    cb_instructions[0x47] = {"BIT 0, A", &Cpu::BIT_0_A, 8, 2};
    cb_instructions[0x48] = {"BIT 1, B", &Cpu::BIT_1_B, 8, 2};
    cb_instructions[0x49] = {"BIT 1, C", &Cpu::BIT_1_C, 8, 2};
    cb_instructions[0x4A] = {"BIT 1, D", &Cpu::BIT_1_D, 8, 2};
    cb_instructions[0x4B] = {"BIT 1, E", &Cpu::BIT_1_E, 8, 2};
    cb_instructions[0x4C] = {"BIT 1, H", &Cpu::BIT_1_H, 8, 2};
    cb_instructions[0x4D] = {"BIT 1, L", &Cpu::BIT_1_L, 8, 2};
    cb_instructions[0x4E] = {"BIT 1, [HL]", &Cpu::BIT_1_HL_a16, 12, 2};
    cb_instructions[0x4F] = {"BIT 1, A", &Cpu::BIT_1_A, 8, 2};

    cb_instructions[0x50] = {"BIT 2, B", &Cpu::BIT_2_B, 8, 2};
    cb_instructions[0x51] = {"BIT 2, C", &Cpu::BIT_2_C, 8, 2};
    cb_instructions[0x52] = {"BIT 2, D", &Cpu::BIT_2_D, 8, 2};
    cb_instructions[0x53] = {"BIT 2, E", &Cpu::BIT_2_E, 8, 2};
    cb_instructions[0x54] = {"BIT 2, H", &Cpu::BIT_2_H, 8, 2};
    cb_instructions[0x55] = {"BIT 2, L", &Cpu::BIT_2_L, 8, 2};
    cb_instructions[0x56] = {"BIT 2, [HL]", &Cpu::BIT_2_HL_a16, 12, 2};
    cb_instructions[0x57] = {"BIT 2, A", &Cpu::BIT_2_A, 8, 2};
    cb_instructions[0x58] = {"BIT 3, B", &Cpu::BIT_3_B, 8, 2};
    cb_instructions[0x59] = {"BIT 3, C", &Cpu::BIT_3_C, 8, 2};
    cb_instructions[0x5A] = {"BIT 3, D", &Cpu::BIT_3_D, 8, 2};
    cb_instructions[0x5B] = {"BIT 3, E", &Cpu::BIT_3_E, 8, 2};
    cb_instructions[0x5C] = {"BIT 3, H", &Cpu::BIT_3_H, 8, 2};
    cb_instructions[0x5D] = {"BIT 3, L", &Cpu::BIT_3_L, 8, 2};
    cb_instructions[0x5E] = {"BIT 3, [HL]", &Cpu::BIT_3_HL_a16, 12, 2};
    cb_instructions[0x5F] = {"BIT 3, A", &Cpu::BIT_3_A, 8, 2};

    cb_instructions[0x60] = {"BIT 4, B", &Cpu::BIT_4_B, 8, 2};
    cb_instructions[0x61] = {"BIT 4, C", &Cpu::BIT_4_C, 8, 2};
    cb_instructions[0x62] = {"BIT 4, D", &Cpu::BIT_4_D, 8, 2};
    cb_instructions[0x63] = {"BIT 4, E", &Cpu::BIT_4_E, 8, 2};
    cb_instructions[0x64] = {"BIT 4, H", &Cpu::BIT_4_H, 8, 2};
    cb_instructions[0x65] = {"BIT 4, L", &Cpu::BIT_4_L, 8, 2};
    cb_instructions[0x66] = {"BIT 4, [HL]", &Cpu::BIT_4_HL_a16, 12, 2};
    cb_instructions[0x67] = {"BIT 4, A", &Cpu::BIT_4_A, 8, 2};
    cb_instructions[0x68] = {"BIT 5, B", &Cpu::BIT_5_B, 8, 2};
    cb_instructions[0x69] = {"BIT 5, C", &Cpu::BIT_5_C, 8, 2};
    cb_instructions[0x6A] = {"BIT 5, D", &Cpu::BIT_5_D, 8, 2};
    cb_instructions[0x6B] = {"BIT 5, E", &Cpu::BIT_5_E, 8, 2};
    cb_instructions[0x6C] = {"BIT 5, H", &Cpu::BIT_5_H, 8, 2};
    cb_instructions[0x6D] = {"BIT 5, L", &Cpu::BIT_5_L, 8, 2};
    cb_instructions[0x6E] = {"BIT 5, [HL]", &Cpu::BIT_5_HL_a16, 12, 2};
    cb_instructions[0x6F] = {"BIT 5, A", &Cpu::BIT_5_A, 8, 2};

    cb_instructions[0x70] = {"BIT 6, B", &Cpu::BIT_6_B, 8, 2};
    cb_instructions[0x71] = {"BIT 6, C", &Cpu::BIT_6_C, 8, 2};
    cb_instructions[0x72] = {"BIT 6, D", &Cpu::BIT_6_D, 8, 2};
    cb_instructions[0x73] = {"BIT 6, E", &Cpu::BIT_6_E, 8, 2};
    cb_instructions[0x74] = {"BIT 6, H", &Cpu::BIT_6_H, 8, 2};
    cb_instructions[0x75] = {"BIT 6, L", &Cpu::BIT_6_L, 8, 2};
    cb_instructions[0x76] = {"BIT 6, [HL]", &Cpu::BIT_6_HL_a16, 12, 2};
    cb_instructions[0x77] = {"BIT 6, A", &Cpu::BIT_6_A, 8, 2};
    cb_instructions[0x78] = {"BIT 7, B", &Cpu::BIT_7_B, 8, 2};
    cb_instructions[0x79] = {"BIT 7, C", &Cpu::BIT_7_C, 8, 2};
    cb_instructions[0x7A] = {"BIT 7, D", &Cpu::BIT_7_D, 8, 2};
    cb_instructions[0x7B] = {"BIT 7, E", &Cpu::BIT_7_E, 8, 2};
    cb_instructions[0x7C] = {"BIT 7, H", &Cpu::BIT_7_H, 8, 2};
    cb_instructions[0x7D] = {"BIT 7, L", &Cpu::BIT_7_L, 8, 2};
    cb_instructions[0x7E] = {"BIT 7, [HL]", &Cpu::BIT_7_HL_a16, 12, 2};
    cb_instructions[0x7F] = {"BIT 7, A", &Cpu::BIT_7_A, 8, 2};

    cb_instructions[0x80] = {"RES 0, B", &Cpu::RES_0_B, 8, 2};
    cb_instructions[0x81] = {"RES 0, C", &Cpu::RES_0_C, 8, 2};
    cb_instructions[0x82] = {"RES 0, D", &Cpu::RES_0_D, 8, 2};
    cb_instructions[0x83] = {"RES 0, E", &Cpu::RES_0_E, 8, 2};
    cb_instructions[0x84] = {"RES 0, H", &Cpu::RES_0_H, 8, 2};
    cb_instructions[0x85] = {"RES 0, L", &Cpu::RES_0_L, 8, 2};
    cb_instructions[0x86] = {"RES 0, [HL]", &Cpu::RES_0_HL_a16, 16, 2};
    cb_instructions[0x87] = {"RES 0, A", &Cpu::RES_0_A, 8, 2};
    cb_instructions[0x88] = {"RES 1, B", &Cpu::RES_1_B, 8, 2};
    cb_instructions[0x89] = {"RES 1, C", &Cpu::RES_1_C, 8, 2};
    cb_instructions[0x8A] = {"RES 1, D", &Cpu::RES_1_D, 8, 2};
    cb_instructions[0x8B] = {"RES 1, E", &Cpu::RES_1_E, 8, 2};
    cb_instructions[0x8C] = {"RES 1, H", &Cpu::RES_1_H, 8, 2};
    cb_instructions[0x8D] = {"RES 1, L", &Cpu::RES_1_L, 8, 2};
    cb_instructions[0x8E] = {"RES 1, [HL]", &Cpu::RES_1_HL_a16, 16, 2};
    cb_instructions[0x8F] = {"RES 1, A", &Cpu::RES_1_A, 8, 2};

    cb_instructions[0x90] = {"RES 2, B", &Cpu::RES_2_B, 8, 2};
    cb_instructions[0x91] = {"RES 2, C", &Cpu::RES_2_C, 8, 2};
    cb_instructions[0x92] = {"RES 2, D", &Cpu::RES_2_D, 8, 2};
    cb_instructions[0x93] = {"RES 2, E", &Cpu::RES_2_E, 8, 2};
    cb_instructions[0x94] = {"RES 2, H", &Cpu::RES_2_H, 8, 2};
    cb_instructions[0x95] = {"RES 2, L", &Cpu::RES_2_L, 8, 2};
    cb_instructions[0x96] = {"RES 2, [HL]", &Cpu::RES_2_HL_a16, 16, 2};
    cb_instructions[0x97] = {"RES 2, A", &Cpu::RES_2_A, 8, 2};
    cb_instructions[0x98] = {"RES 3, B", &Cpu::RES_3_B, 8, 2};
    cb_instructions[0x99] = {"RES 3, C", &Cpu::RES_3_C, 8, 2};
    cb_instructions[0x9A] = {"RES 3, D", &Cpu::RES_3_D, 8, 2};
    cb_instructions[0x9B] = {"RES 3, E", &Cpu::RES_3_E, 8, 2};
    cb_instructions[0x9C] = {"RES 3, H", &Cpu::RES_3_H, 8, 2};
    cb_instructions[0x9D] = {"RES 3, L", &Cpu::RES_3_L, 8, 2};
    cb_instructions[0x9E] = {"RES 3, [HL]", &Cpu::RES_3_HL_a16, 16, 2};
    cb_instructions[0x9F] = {"RES 3, A", &Cpu::RES_3_A, 8, 2};

    cb_instructions[0xA0] = {"RES 4, B", &Cpu::RES_4_B, 8, 2};
    cb_instructions[0xA1] = {"RES 4, C", &Cpu::RES_4_C, 8, 2};
    cb_instructions[0xA2] = {"RES 4, D", &Cpu::RES_4_D, 8, 2};
    cb_instructions[0xA3] = {"RES 4, E", &Cpu::RES_4_E, 8, 2};
    cb_instructions[0xA4] = {"RES 4, H", &Cpu::RES_4_H, 8, 2};
    cb_instructions[0xA5] = {"RES 4, L", &Cpu::RES_4_L, 8, 2};
    cb_instructions[0xA6] = {"RES 4, [HL]", &Cpu::RES_4_HL_a16, 16, 2};
    cb_instructions[0xA7] = {"RES 4, A", &Cpu::RES_4_A, 8, 2};
    cb_instructions[0xA8] = {"RES 5, B", &Cpu::RES_5_B, 8, 2};
    cb_instructions[0xA9] = {"RES 5, C", &Cpu::RES_5_C, 8, 2};
    cb_instructions[0xAA] = {"RES 5, D", &Cpu::RES_5_D, 8, 2};
    cb_instructions[0xAB] = {"RES 5, E", &Cpu::RES_5_E, 8, 2};
    cb_instructions[0xAC] = {"RES 5, H", &Cpu::RES_5_H, 8, 2};
    cb_instructions[0xAD] = {"RES 5, L", &Cpu::RES_5_L, 8, 2};
    cb_instructions[0xAE] = {"RES 5, [HL]", &Cpu::RES_5_HL_a16, 16, 2};
    cb_instructions[0xAF] = {"RES 5, A", &Cpu::RES_5_A, 8, 2};

    cb_instructions[0xB0] = {"RES 6, B", &Cpu::RES_6_B, 8, 2};
    cb_instructions[0xB1] = {"RES 6, C", &Cpu::RES_6_C, 8, 2};
    cb_instructions[0xB2] = {"RES 6, D", &Cpu::RES_6_D, 8, 2};
    cb_instructions[0xB3] = {"RES 6, E", &Cpu::RES_6_E, 8, 2};
    cb_instructions[0xB4] = {"RES 6, H", &Cpu::RES_6_H, 8, 2};
    cb_instructions[0xB5] = {"RES 6, L", &Cpu::RES_6_L, 8, 2};
    cb_instructions[0xB6] = {"RES 6, [HL]", &Cpu::RES_6_HL_a16, 16, 2};
    cb_instructions[0xB7] = {"RES 6, A", &Cpu::RES_6_A, 8, 2};
    cb_instructions[0xB8] = {"RES 7, B", &Cpu::RES_7_B, 8, 2};
    cb_instructions[0xB9] = {"RES 7, C", &Cpu::RES_7_C, 8, 2};
    cb_instructions[0xBA] = {"RES 7, D", &Cpu::RES_7_D, 8, 2};
    cb_instructions[0xBB] = {"RES 7, E", &Cpu::RES_7_E, 8, 2};
    cb_instructions[0xBC] = {"RES 7, H", &Cpu::RES_7_H, 8, 2};
    cb_instructions[0xBD] = {"RES 7, L", &Cpu::RES_7_L, 8, 2};
    cb_instructions[0xBE] = {"RES 7, [HL]", &Cpu::RES_7_HL_a16, 16, 2};
    cb_instructions[0xBF] = {"RES 7, A", &Cpu::RES_7_A, 8, 2};

    cb_instructions[0xC0] = {"SET 0, B", &Cpu::SET_0_B, 8, 2};
    cb_instructions[0xC1] = {"SET 0, C", &Cpu::SET_0_C, 8, 2};
    cb_instructions[0xC2] = {"SET 0, D", &Cpu::SET_0_D, 8, 2};
    cb_instructions[0xC3] = {"SET 0, E", &Cpu::SET_0_E, 8, 2};
    cb_instructions[0xC4] = {"SET 0, H", &Cpu::SET_0_H, 8, 2};
    cb_instructions[0xC5] = {"SET 0, L", &Cpu::SET_0_L, 8, 2};
    cb_instructions[0xC6] = {"SET 0, [HL]", &Cpu::SET_0_HL_a16, 16, 2};
    cb_instructions[0xC7] = {"SET 0, A", &Cpu::SET_0_A, 8, 2};
    cb_instructions[0xC8] = {"SET 1, B", &Cpu::SET_1_B, 8, 2};
    cb_instructions[0xC9] = {"SET 1, C", &Cpu::SET_1_C, 8, 2};
    cb_instructions[0xCA] = {"SET 1, D", &Cpu::SET_1_D, 8, 2};
    cb_instructions[0xCB] = {"SET 1, E", &Cpu::SET_1_E, 8, 2};
    cb_instructions[0xCC] = {"SET 1, H", &Cpu::SET_1_H, 8, 2};
    cb_instructions[0xCD] = {"SET 1, L", &Cpu::SET_1_L, 8, 2};
    cb_instructions[0xCE] = {"SET 1, [HL]", &Cpu::SET_1_HL_a16, 16, 2};
    cb_instructions[0xCF] = {"SET 1, A", &Cpu::SET_1_A, 8, 2};

    cb_instructions[0xD0] = {"SET 2, B", &Cpu::SET_2_B, 8, 2};
    cb_instructions[0xD1] = {"SET 2, C", &Cpu::SET_2_C, 8, 2};
    cb_instructions[0xD2] = {"SET 2, D", &Cpu::SET_2_D, 8, 2};
    cb_instructions[0xD3] = {"SET 2, E", &Cpu::SET_2_E, 8, 2};
    cb_instructions[0xD4] = {"SET 2, H", &Cpu::SET_2_H, 8, 2};
    cb_instructions[0xD5] = {"SET 2, L", &Cpu::SET_2_L, 8, 2};
    cb_instructions[0xD6] = {"SET 2, [HL]", &Cpu::SET_2_HL_a16, 16, 2};
    cb_instructions[0xD7] = {"SET 2, A", &Cpu::SET_2_A, 8, 2};
    cb_instructions[0xD8] = {"SET 3, B", &Cpu::SET_3_B, 8, 2};
    cb_instructions[0xD9] = {"SET 3, C", &Cpu::SET_3_C, 8, 2};
    cb_instructions[0xDA] = {"SET 3, D", &Cpu::SET_3_D, 8, 2};
    cb_instructions[0xDB] = {"SET 3, E", &Cpu::SET_3_E, 8, 2};
    cb_instructions[0xDC] = {"SET 3, H", &Cpu::SET_3_H, 8, 2};
    cb_instructions[0xDD] = {"SET 3, L", &Cpu::SET_3_L, 8, 2};
    cb_instructions[0xDE] = {"SET 3, [HL]", &Cpu::SET_3_HL_a16, 16, 2};
    cb_instructions[0xDF] = {"SET 3, A", &Cpu::SET_3_A, 8, 2};

    cb_instructions[0xE0] = {"SET 4, B", &Cpu::SET_4_B, 8, 2};
    cb_instructions[0xE1] = {"SET 4, C", &Cpu::SET_4_C, 8, 2};
    cb_instructions[0xE2] = {"SET 4, D", &Cpu::SET_4_D, 8, 2};
    cb_instructions[0xE3] = {"SET 4, E", &Cpu::SET_4_E, 8, 2};
    cb_instructions[0xE4] = {"SET 4, H", &Cpu::SET_4_H, 8, 2};
    cb_instructions[0xE5] = {"SET 4, L", &Cpu::SET_4_L, 8, 2};
    cb_instructions[0xE6] = {"SET 4, [HL]", &Cpu::SET_4_HL_a16, 16, 2};
    cb_instructions[0xE7] = {"SET 4, A", &Cpu::SET_4_A, 8, 2};
    cb_instructions[0xE8] = {"SET 5, B", &Cpu::SET_5_B, 8, 2};
    cb_instructions[0xE9] = {"SET 5, C", &Cpu::SET_5_C, 8, 2};
    cb_instructions[0xEA] = {"SET 5, D", &Cpu::SET_5_D, 8, 2};
    cb_instructions[0xEB] = {"SET 5, E", &Cpu::SET_5_E, 8, 2};
    cb_instructions[0xEC] = {"SET 5, H", &Cpu::SET_5_H, 8, 2};
    cb_instructions[0xED] = {"SET 5, L", &Cpu::SET_5_L, 8, 2};
    cb_instructions[0xEE] = {"SET 5, [HL]", &Cpu::SET_5_HL_a16, 16, 2};
    cb_instructions[0xEF] = {"SET 5, A", &Cpu::SET_5_A, 8, 2};

    cb_instructions[0xF0] = {"SET 6, B", &Cpu::SET_6_B, 8, 2};
    cb_instructions[0xF1] = {"SET 6, C", &Cpu::SET_6_C, 8, 2};
    cb_instructions[0xF2] = {"SET 6, D", &Cpu::SET_6_D, 8, 2};
    cb_instructions[0xF3] = {"SET 6, E", &Cpu::SET_6_E, 8, 2};
    cb_instructions[0xF4] = {"SET 6, H", &Cpu::SET_6_H, 8, 2};
    cb_instructions[0xF5] = {"SET 6, L", &Cpu::SET_6_L, 8, 2};
    cb_instructions[0xF6] = {"SET 6, [HL]", &Cpu::SET_6_HL_a16, 16, 2};
    cb_instructions[0xF7] = {"SET 6, A", &Cpu::SET_6_A, 8, 2};
    cb_instructions[0xF8] = {"SET 7, B", &Cpu::SET_7_B, 8, 2};
    cb_instructions[0xF9] = {"SET 7, C", &Cpu::SET_7_C, 8, 2};
    cb_instructions[0xFA] = {"SET 7, D", &Cpu::SET_7_D, 8, 2};
    cb_instructions[0xFB] = {"SET 7, E", &Cpu::SET_7_E, 8, 2};
    cb_instructions[0xFC] = {"SET 7, H", &Cpu::SET_7_H, 8, 2};
    cb_instructions[0xFD] = {"SET 7, L", &Cpu::SET_7_L, 8, 2};
    cb_instructions[0xFE] = {"SET 7, [HL]", &Cpu::SET_7_HL_a16, 16, 2};
    cb_instructions[0xFF] = {"SET 7, A", &Cpu::SET_7_A, 8, 2};
}
