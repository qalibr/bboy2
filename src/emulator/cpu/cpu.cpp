#include "cpu.h"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#define STUB(name)                                \
    void Cpu::name() {                            \
        std::cerr << "STUB: " #name << std::endl; \
        exit(1);                                  \
    }

Cpu::Cpu(Mmu& m) : mmu(m) {
    init_registers();
    init_instructions();
    ime_schedule = 0;
    halted       = false;
}

// =============================================================
//  Control & Misc
// =============================================================
void Cpu::NOP() {}

void Cpu::STOP() {
    // Simplified
    fetch_u8();
}

void Cpu::HALT() {
    if (!IME && (mmu.IE & mmu.IF & 0x1F) != 0) {
        halt_bug = true;
    } else {
        halted = true;
    }
}

void Cpu::handle_interrupts() {
    u8 servicable_interrupts = mmu.IE & mmu.IF;

    if (halted && (servicable_interrupts & 0x1F) > 0) {
        halted = false;
    }

    if (!IME || (servicable_interrupts & 0x1F) == 0) {
        return;
    }

    halted = false;
    cycles_this_step += 20;
    IME = false;

    push(reg.PC);

    if (servicable_interrupts & (1 << static_cast<u8>(InterruptType::VBlank))) {
        reg.PC = 0x0040;
        mmu.IF &= ~(1 << static_cast<u8>(InterruptType::VBlank));
    } else if (servicable_interrupts & (1 << static_cast<u8>(InterruptType::Lcd))) {
        reg.PC = 0x0048;
        mmu.IF &= ~(1 << static_cast<u8>(InterruptType::Lcd));
    } else if (servicable_interrupts & (1 << static_cast<u8>(InterruptType::Timer))) {
        reg.PC = 0x0050;
        mmu.IF &= ~(1 << static_cast<u8>(InterruptType::Timer));
    } else if (servicable_interrupts & (1 << static_cast<u8>(InterruptType::Serial))) {
        reg.PC = 0x0058;
        mmu.IF &= ~(1 << static_cast<u8>(InterruptType::Serial));
    } else if (servicable_interrupts & (1 << static_cast<u8>(InterruptType::Joypad))) {
        reg.PC = 0x0060;
        mmu.IF &= ~(1 << static_cast<u8>(InterruptType::Joypad));
    }
}

void Cpu::DI() {
    IME          = false;
    ime_schedule = 0;
}

// NOTE:
// The EI instruction enables interrupts after the *next* instruction.
// EI Bug: if an interrupt is pending when EI is executed, the instruction
// after EI is skipped, and the ISR is executed instead.
void Cpu::EI() { ime_schedule = 2; }

// =============================================================
//  Jump Calls
// =============================================================
void Cpu::RETI() {  // 0xD9
    reg.PC = pop();
    IME    = true;
}
void Cpu::JR_e8() {  // 0x18
    i8 offset = fetch_u8();
    reg.PC += offset;
}
void Cpu::CALL_a16() {  // 0xCD
    u16 addr = fetch_u16();
    push(reg.PC);
    reg.PC = addr;
}
void Cpu::JP_a16() { reg.PC = fetch_u16(); }    // 0xC3
void Cpu::JP_HL() { reg.PC = reg.HL; }          // 0xE9
void Cpu::RET() { reg.PC = pop(); }             // 0xC9
void Cpu::JR_NZ_e8() { JR_COND(!reg.z); }       // 0x20
void Cpu::JR_NC_e8() { JR_COND(!reg.c); }       // 0x30
void Cpu::JR_Z_e8() { JR_COND(reg.z); }         // 0x28
void Cpu::JR_C_e8() { JR_COND(reg.c); }         // 0x38
void Cpu::RET_NZ() { RET_COND(!reg.z); }        // 0xC0
void Cpu::RET_NC() { RET_COND(!reg.c); }        // 0xD0
void Cpu::RET_Z() { RET_COND(reg.z); }          // 0xC8
void Cpu::RET_C() { RET_COND(reg.c); }          // 0xD8
void Cpu::JP_NZ_a16() { JP_COND(!reg.z); }      // 0xC2
void Cpu::JP_NC_a16() { JP_COND(!reg.c); }      // 0xD2
void Cpu::JP_Z_a16() { JP_COND(reg.z); }        // 0xCA
void Cpu::JP_C_a16() { JP_COND(reg.c); }        // 0xDA
void Cpu::CALL_NZ_a16() { CALL_COND(!reg.z); }  // 0xC4
void Cpu::CALL_NC_a16() { CALL_COND(!reg.c); }  // 0xD4
void Cpu::CALL_Z_a16() { CALL_COND(reg.z); }    // 0xCC
void Cpu::CALL_C_a16() { CALL_COND(reg.c); }    // 0xDC
void Cpu::RST_00() { RST(0x00); }               // 0xC7
void Cpu::RST_08() { RST(0x08); }               // 0xCF
void Cpu::RST_10() { RST(0x10); }               // 0xD7
void Cpu::RST_18() { RST(0x18); }               // 0xDF
void Cpu::RST_20() { RST(0x20); }               // 0xE7
void Cpu::RST_28() { RST(0x28); }               // 0xEF
void Cpu::RST_30() { RST(0x30); }               // 0xF7
void Cpu::RST_38() { RST(0x38); }               // 0xFF

// =============================================================
//  8-bit Load
// =============================================================
void Cpu::LD_BC_a16_A() { mmu.write_u8(reg.BC, reg.A); }        // 0x02
void Cpu::LD_DE_a16_A() { mmu.write_u8(reg.DE, reg.A); }        // 0x12
void Cpu::LD_HLi_a16_A() { mmu.write_u8(reg.HL++, reg.A); }     // 0x22
void Cpu::LD_HLd_a16_A() { mmu.write_u8(reg.HL--, reg.A); }     // 0x32
void Cpu::LD_B_n8() { LD_R_n8(reg.B); }                         // 0x06
void Cpu::LD_D_n8() { LD_R_n8(reg.D); }                         // 0x16
void Cpu::LD_H_n8() { LD_R_n8(reg.H); }                         // 0x26
void Cpu::LD_HL_a16_n8() { mmu.write_u8(reg.HL, fetch_u8()); }  // 0x36
void Cpu::LD_A_BC_a16() { reg.A = mmu.read_u8(reg.BC); }        // 0x0A
void Cpu::LD_A_DE_a16() { reg.A = mmu.read_u8(reg.DE); }        // 0x1A
void Cpu::LD_A_HLi_a16() { reg.A = mmu.read_u8(reg.HL++); }     // 0x2A
void Cpu::LD_A_HLd_a16() { reg.A = mmu.read_u8(reg.HL--); }     // 0x3A
void Cpu::LD_C_n8() { LD_R_n8(reg.C); }                         // 0x0E
void Cpu::LD_E_n8() { LD_R_n8(reg.E); }                         // 0x1E
void Cpu::LD_L_n8() { LD_R_n8(reg.L); }                         // 0x2E
void Cpu::LD_A_n8() { LD_R_n8(reg.A); }                         // 0x3E

// =============================================================
//  LD r, r'
// =============================================================
void Cpu::LD_B_B() { LD_R_R(reg.B, reg.B); }  // 0x40
void Cpu::LD_B_C() { LD_R_R(reg.B, reg.C); }  // 0x41
void Cpu::LD_B_D() { LD_R_R(reg.B, reg.D); }  // 0x42
void Cpu::LD_B_E() { LD_R_R(reg.B, reg.E); }  // 0x43
void Cpu::LD_B_H() { LD_R_R(reg.B, reg.H); }  // 0x44
void Cpu::LD_B_L() { LD_R_R(reg.B, reg.L); }  // 0x45
void Cpu::LD_B_A() { LD_R_R(reg.B, reg.A); }  // 0x47

void Cpu::LD_C_B() { LD_R_R(reg.C, reg.B); }  // 0x48
void Cpu::LD_C_C() { LD_R_R(reg.C, reg.C); }  // 0x49
void Cpu::LD_C_D() { LD_R_R(reg.C, reg.D); }  // 0x4A
void Cpu::LD_C_E() { LD_R_R(reg.C, reg.E); }  // 0x4B
void Cpu::LD_C_H() { LD_R_R(reg.C, reg.H); }  // 0x4C
void Cpu::LD_C_L() { LD_R_R(reg.C, reg.L); }  // 0x4D
void Cpu::LD_C_A() { LD_R_R(reg.C, reg.A); }  // 0x4F

void Cpu::LD_D_B() { LD_R_R(reg.D, reg.B); }  // 0x50
void Cpu::LD_D_C() { LD_R_R(reg.D, reg.C); }  // 0x51
void Cpu::LD_D_D() { LD_R_R(reg.D, reg.D); }  // 0x52
void Cpu::LD_D_E() { LD_R_R(reg.D, reg.E); }  // 0x53
void Cpu::LD_D_H() { LD_R_R(reg.D, reg.H); }  // 0x54
void Cpu::LD_D_L() { LD_R_R(reg.D, reg.L); }  // 0x55
void Cpu::LD_D_A() { LD_R_R(reg.D, reg.A); }  // 0x57

void Cpu::LD_E_B() { LD_R_R(reg.E, reg.B); }  // 0x58
void Cpu::LD_E_C() { LD_R_R(reg.E, reg.C); }  // 0x59
void Cpu::LD_E_D() { LD_R_R(reg.E, reg.D); }  // 0x5A
void Cpu::LD_E_E() { LD_R_R(reg.E, reg.E); }  // 0x5B
void Cpu::LD_E_H() { LD_R_R(reg.E, reg.H); }  // 0x5C
void Cpu::LD_E_L() { LD_R_R(reg.E, reg.L); }  // 0x5D
void Cpu::LD_E_A() { LD_R_R(reg.E, reg.A); }  // 0x5F

void Cpu::LD_H_B() { LD_R_R(reg.H, reg.B); }  // 0x60
void Cpu::LD_H_C() { LD_R_R(reg.H, reg.C); }  // 0x61
void Cpu::LD_H_D() { LD_R_R(reg.H, reg.D); }  // 0x62
void Cpu::LD_H_E() { LD_R_R(reg.H, reg.E); }  // 0x63
void Cpu::LD_H_H() { LD_R_R(reg.H, reg.H); }  // 0x64
void Cpu::LD_H_L() { LD_R_R(reg.H, reg.L); }  // 0x65
void Cpu::LD_H_A() { LD_R_R(reg.H, reg.A); }  // 0x67

void Cpu::LD_L_B() { LD_R_R(reg.L, reg.B); }  // 0x68
void Cpu::LD_L_C() { LD_R_R(reg.L, reg.C); }  // 0x69
void Cpu::LD_L_D() { LD_R_R(reg.L, reg.D); }  // 0x6A
void Cpu::LD_L_E() { LD_R_R(reg.L, reg.E); }  // 0x6B
void Cpu::LD_L_H() { LD_R_R(reg.L, reg.H); }  // 0x6C
void Cpu::LD_L_L() { LD_R_R(reg.L, reg.L); }  // 0x6D
void Cpu::LD_L_A() { LD_R_R(reg.L, reg.A); }  // 0x6F

void Cpu::LD_B_HL_a16() { LD_R_HL_a16(reg.B); }  // 0x46
void Cpu::LD_C_HL_a16() { LD_R_HL_a16(reg.C); }  // 0x4E
void Cpu::LD_D_HL_a16() { LD_R_HL_a16(reg.D); }  // 0x56
void Cpu::LD_E_HL_a16() { LD_R_HL_a16(reg.E); }  // 0x5E
void Cpu::LD_H_HL_a16() { LD_R_HL_a16(reg.H); }  // 0x66
void Cpu::LD_L_HL_a16() { LD_R_HL_a16(reg.L); }  // 0x6E
void Cpu::LD_A_HL_a16() { LD_R_HL_a16(reg.A); }  // 0x7E

void Cpu::LD_HL_a16_B() { LD_HL_R(reg.B); }  // 0x70
void Cpu::LD_HL_a16_C() { LD_HL_R(reg.C); }  // 0x71
void Cpu::LD_HL_a16_D() { LD_HL_R(reg.D); }  // 0x72
void Cpu::LD_HL_a16_E() { LD_HL_R(reg.E); }  // 0x73
void Cpu::LD_HL_a16_H() { LD_HL_R(reg.H); }  // 0x74
void Cpu::LD_HL_a16_L() { LD_HL_R(reg.L); }  // 0x75
void Cpu::LD_HL_a16_A() { LD_HL_R(reg.A); }  // 0x77

void Cpu::LD_A_B() { LD_R_R(reg.A, reg.B); }  // 0x78
void Cpu::LD_A_C() { LD_R_R(reg.A, reg.C); }  // 0x79
void Cpu::LD_A_D() { LD_R_R(reg.A, reg.D); }  // 0x7A
void Cpu::LD_A_E() { LD_R_R(reg.A, reg.E); }  // 0x7B
void Cpu::LD_A_H() { LD_R_R(reg.A, reg.H); }  // 0x7C
void Cpu::LD_A_L() { LD_R_R(reg.A, reg.L); }  // 0x7D
void Cpu::LD_A_A() { LD_R_R(reg.A, reg.A); }  // 0x7F

// =============================================================
//  Other 8-bit Load
// =============================================================
void Cpu::LDH_a8_A() { mmu.write_u8(0xFF00 | fetch_u8(), reg.A); }  // 0xE0
void Cpu::LDH_A_a8() { reg.A = mmu.read_u8(0xFF00 | fetch_u8()); }  // 0xF0
void Cpu::LD_C_a16_A() { mmu.write_u8(0xFF00 | reg.C, reg.A); }     // 0xE2
void Cpu::LD_A_C_a16() { reg.A = mmu.read_u8(0xFF00 | reg.C); }     // 0xF2
void Cpu::LD_a16_A() { mmu.write_u8(fetch_u16(), reg.A); }          // 0xEA
void Cpu::LD_A_a16() { reg.A = mmu.read_u8(fetch_u16()); }          // 0xFA

// =============================================================
//  16-bit Load (& Stack)
// =============================================================
void Cpu::LD_BC_n16() { reg.BC = fetch_u16(); }  // 0x01
void Cpu::LD_DE_n16() { reg.DE = fetch_u16(); }  // 0x11
void Cpu::LD_HL_n16() { reg.HL = fetch_u16(); }  // 0x21
void Cpu::LD_SP_n16() { reg.SP = fetch_u16(); }  // 0x31
void Cpu::LD_a16_SP() {                          // 0x08
    u16 addr = fetch_u16();
    mmu.write_u8(addr, reg.SP & 0xFF);    // Low
    mmu.write_u8(addr + 1, reg.SP >> 8);  // High
}
void Cpu::PUSH_BC() { push(reg.BC); }            // 0xC5
void Cpu::PUSH_DE() { push(reg.DE); }            // 0xD5
void Cpu::PUSH_HL() { push(reg.HL); }            // 0xE5
void Cpu::PUSH_AF() { push(reg.AF & 0xFFF0); }   // 0xF5
void Cpu::POP_BC() { reg.BC = pop(); }           // 0xC1
void Cpu::POP_DE() { reg.DE = pop(); }           // 0xD1
void Cpu::POP_HL() { reg.HL = pop(); }           // 0xE1
void Cpu::POP_AF() { reg.AF = pop() & 0xFFF0; }  // 0xF1
void Cpu::LD_HL_SP_e8() {                        // 0xF8
    i8  offset = (i8)fetch_u8();
    u16 result = reg.SP + offset;
    reg.z      = 0;
    reg.n      = 0;
    reg.h      = ((reg.SP & 0x0F) + (offset & 0x0F)) > 0x0F;
    reg.c      = ((reg.SP & 0xFF) + (offset & 0xFF)) > 0xFF;
    reg.HL     = result;
}
void Cpu::LD_SP_HL() { reg.SP = reg.HL; }  // 0xF9

// =============================================================
//  8-bit Arithmetic & Logical
// =============================================================
void Cpu::DAA() {  // 0x27
    if (reg.n) {
        if (reg.c) {
            reg.A -= 0x60;
        }
        if (reg.h) {
            reg.A -= 0x6;
        }
    } else {
        if (reg.c || (reg.A > 0x99)) {
            reg.A += 0x60;
            reg.c = 1;
        }
        if (reg.h || (reg.A & 0xF) > 0x9) {
            reg.A += 0x6;
        }
    }
    reg.z = reg.A == 0;
    reg.h = 0;
}
void Cpu::CPL() {  // 0x2F
    reg.A = ~reg.A;
    reg.n = 1;
    reg.h = 1;
}
void Cpu::SCF() {  // 0x37
    reg.n = 0;
    reg.h = 0;
    reg.c = 1;
}
void Cpu::CCF() {  // 0x3F
    reg.n = 0;
    reg.h = 0;
    reg.c = !reg.c;
}
void Cpu::INC_B() { INC_R(reg.B); }      // 0x04
void Cpu::INC_C() { INC_R(reg.C); }      // 0x0C
void Cpu::INC_D() { INC_R(reg.D); }      // 0x14
void Cpu::INC_E() { INC_R(reg.E); }      // 0x1C
void Cpu::INC_H() { INC_R(reg.H); }      // 0x24
void Cpu::INC_L() { INC_R(reg.L); }      // 0x2C
void Cpu::INC_A() { INC_R(reg.A); }      // 0x3C
void Cpu::DEC_B() { DEC_R(reg.B); }      // 0x05
void Cpu::DEC_C() { DEC_R(reg.C); }      // 0x0D
void Cpu::DEC_D() { DEC_R(reg.D); }      // 0x15
void Cpu::DEC_E() { DEC_R(reg.E); }      // 0x1D
void Cpu::DEC_H() { DEC_R(reg.H); }      // 0x25
void Cpu::DEC_L() { DEC_R(reg.L); }      // 0x2D
void Cpu::DEC_A() { DEC_R(reg.A); }      // 0x3D
void Cpu::ADD_A_B() { ADD_A_R(reg.B); }  // 0x80
void Cpu::ADD_A_C() { ADD_A_R(reg.C); }  // 0x81
void Cpu::ADD_A_D() { ADD_A_R(reg.D); }  // 0x82
void Cpu::ADD_A_E() { ADD_A_R(reg.E); }  // 0x83
void Cpu::ADD_A_H() { ADD_A_R(reg.H); }  // 0x84
void Cpu::ADD_A_L() { ADD_A_R(reg.L); }  // 0x85
void Cpu::ADD_A_A() { ADD_A_R(reg.A); }  // 0x87
void Cpu::ADC_A_B() { ADC_A_R(reg.B); }  // 0x88
void Cpu::ADC_A_C() { ADC_A_R(reg.C); }  // 0x89
void Cpu::ADC_A_D() { ADC_A_R(reg.D); }  // 0x8A
void Cpu::ADC_A_E() { ADC_A_R(reg.E); }  // 0x8B
void Cpu::ADC_A_H() { ADC_A_R(reg.H); }  // 0x8C
void Cpu::ADC_A_L() { ADC_A_R(reg.L); }  // 0x8D
void Cpu::ADC_A_A() { ADC_A_R(reg.A); }  // 0x8F
void Cpu::SUB_A_B() { SUB_A_R(reg.B); }  // 0x90
void Cpu::SUB_A_C() { SUB_A_R(reg.C); }  // 0x91
void Cpu::SUB_A_D() { SUB_A_R(reg.D); }  // 0x92
void Cpu::SUB_A_E() { SUB_A_R(reg.E); }  // 0x93
void Cpu::SUB_A_H() { SUB_A_R(reg.H); }  // 0x94
void Cpu::SUB_A_L() { SUB_A_R(reg.L); }  // 0x95
void Cpu::SUB_A_A() { SUB_A_R(reg.A); }  // 0x97
void Cpu::SBC_A_B() { SBC_A_R(reg.B); }  // 0x98
void Cpu::SBC_A_C() { SBC_A_R(reg.C); }  // 0x99
void Cpu::SBC_A_D() { SBC_A_R(reg.D); }  // 0x9A
void Cpu::SBC_A_E() { SBC_A_R(reg.E); }  // 0x9B
void Cpu::SBC_A_H() { SBC_A_R(reg.H); }  // 0x9C
void Cpu::SBC_A_L() { SBC_A_R(reg.L); }  // 0x9D
void Cpu::SBC_A_A() { SBC_A_R(reg.A); }  // 0x9F
void Cpu::AND_A_B() { AND_A_R(reg.B); }  // 0xA0
void Cpu::AND_A_C() { AND_A_R(reg.C); }  // 0xA1
void Cpu::AND_A_D() { AND_A_R(reg.D); }  // 0xA2
void Cpu::AND_A_E() { AND_A_R(reg.E); }  // 0xA3
void Cpu::AND_A_H() { AND_A_R(reg.H); }  // 0xA4
void Cpu::AND_A_L() { AND_A_R(reg.L); }  // 0xA5
void Cpu::AND_A_A() { AND_A_R(reg.A); }  // 0xA7
void Cpu::XOR_A_B() { XOR_A_R(reg.B); }  // 0xA8
void Cpu::XOR_A_C() { XOR_A_R(reg.C); }  // 0xA9
void Cpu::XOR_A_D() { XOR_A_R(reg.D); }  // 0xAA
void Cpu::XOR_A_E() { XOR_A_R(reg.E); }  // 0xAB
void Cpu::XOR_A_H() { XOR_A_R(reg.H); }  // 0xAC
void Cpu::XOR_A_L() { XOR_A_R(reg.L); }  // 0xAD
void Cpu::XOR_A_A() { XOR_A_R(reg.A); }  // 0xAF
void Cpu::OR_A_B() { OR_A_R(reg.B); }    // 0xB0
void Cpu::OR_A_C() { OR_A_R(reg.C); }    // 0xB1
void Cpu::OR_A_D() { OR_A_R(reg.D); }    // 0xB2
void Cpu::OR_A_E() { OR_A_R(reg.E); }    // 0xB3
void Cpu::OR_A_H() { OR_A_R(reg.H); }    // 0xB4
void Cpu::OR_A_L() { OR_A_R(reg.L); }    // 0xB5
void Cpu::OR_A_A() { OR_A_R(reg.A); }    // 0xB7
void Cpu::CP_A_B() { CP_A_R(reg.B); }    // 0xB8
void Cpu::CP_A_C() { CP_A_R(reg.C); }    // 0xB9
void Cpu::CP_A_D() { CP_A_R(reg.D); }    // 0xBA
void Cpu::CP_A_E() { CP_A_R(reg.E); }    // 0xBB
void Cpu::CP_A_H() { CP_A_R(reg.H); }    // 0xBC
void Cpu::CP_A_L() { CP_A_R(reg.L); }    // 0xBD
void Cpu::CP_A_A() { CP_A_R(reg.A); }    // 0xBF
void Cpu::INC_HL_a16() {                 // 0x34
    u8 val = mmu.read_u8(reg.HL);
    reg.h  = (val & 0x0F) == 0x0F;
    val++;
    reg.z = val == 0;
    reg.n = 0;
    mmu.write_u8(reg.HL, val);
}
void Cpu::DEC_HL_a16() {  // 0x35
    u8 val = mmu.read_u8(reg.HL);
    reg.h  = (val & 0x0F) == 0x00;
    val--;
    reg.z = val == 0;
    reg.n = 1;
    mmu.write_u8(reg.HL, val);
}
void Cpu::ADD_A_HL_a16() { ADD_A_R(mmu.read_u8(reg.HL)); }  // 0x86
void Cpu::ADC_A_HL_a16() { ADC_A_R(mmu.read_u8(reg.HL)); }  // 0x8E
void Cpu::SUB_A_HL_a16() { SUB_A_R(mmu.read_u8(reg.HL)); }  // 0x96
void Cpu::SBC_A_HL_a16() { SBC_A_R(mmu.read_u8(reg.HL)); }  // 0x9E
void Cpu::AND_A_HL_a16() { AND_A_R(mmu.read_u8(reg.HL)); }  // 0xA6
void Cpu::XOR_A_HL_a16() { XOR_A_R(mmu.read_u8(reg.HL)); }  // 0xAE
void Cpu::OR_A_HL_a16() { OR_A_R(mmu.read_u8(reg.HL)); }    // 0xB6
void Cpu::CP_A_HL_a16() { CP_A_R(mmu.read_u8(reg.HL)); }    // 0xBE
void Cpu::ADD_A_n8() { ADD_A_R(fetch_u8()); }               // 0xC6
void Cpu::SUB_A_n8() { SUB_A_R(fetch_u8()); }               // 0xD6
void Cpu::AND_A_n8() { AND_A_R(fetch_u8()); }               // 0xE6
void Cpu::OR_A_n8() { OR_A_R(fetch_u8()); }                 // 0xF6
void Cpu::ADC_A_n8() { ADC_A_R(fetch_u8()); }               // 0xCE
void Cpu::SBC_A_n8() { SBC_A_R(fetch_u8()); }               // 0xDE
void Cpu::XOR_A_n8() { XOR_A_R(fetch_u8()); }               // 0xEE
void Cpu::CP_A_n8() { CP_A_R(fetch_u8()); }                 // 0xEF

// =============================================================
//  16-bit Arithmetic & Logical
// =============================================================
void Cpu::INC_BC() { INC_R16(reg.BC); }        // 0x03
void Cpu::INC_DE() { INC_R16(reg.DE); }        // 0x13
void Cpu::INC_HL() { INC_R16(reg.HL); }        // 0x23
void Cpu::INC_SP() { INC_R16(reg.SP); }        // 0x33
void Cpu::ADD_HL_BC() { ADD_HL_R16(reg.BC); }  // 0x09
void Cpu::ADD_HL_DE() { ADD_HL_R16(reg.DE); }  // 0x19
void Cpu::ADD_HL_HL() { ADD_HL_R16(reg.HL); }  // 0x29
void Cpu::ADD_HL_SP() { ADD_HL_R16(reg.SP); }  // 0x39
void Cpu::DEC_BC() { DEC_R16(reg.BC); }        // 0x0B
void Cpu::DEC_DE() { DEC_R16(reg.DE); }        // 0x1B
void Cpu::DEC_HL() { DEC_R16(reg.HL); }        // 0x2B
void Cpu::DEC_SP() { DEC_R16(reg.SP); }        // 0x3B
void Cpu::ADD_SP_e8() {                        // 0xE8
    i8  offset = (i8)fetch_u8();
    int result = reg.SP + offset;
    reg.z      = 0;
    reg.n      = 0;
    reg.h      = ((reg.SP & 0xF) + (offset & 0xF)) > 0xF;
    reg.c      = ((reg.SP & 0xFF) + (offset & 0xFF)) > 0xFF;
    reg.SP     = (u16)result;
}

// =============================================================
//  8-bit Bit Ops
// =============================================================
void Cpu::RLCA() {  // 0x07
    reg.c = (reg.A & 0x80) != 0;
    reg.A = (reg.A << 1) | (reg.A >> 7);
    reg.z = 0;
    reg.n = 0;
    reg.h = 0;
}
void Cpu::RLA() {  // 0x17
    u8 carry = reg.c ? 1 : 0;
    reg.c    = (reg.A & 0x80) != 0;
    reg.A    = (reg.A << 1) | carry;
    reg.z    = 0;
    reg.n    = 0;
    reg.h    = 0;
}
void Cpu::RRCA() {  // 0x0F
    reg.c = (reg.A & 0x01) != 0;
    reg.A = (reg.A >> 1) | (reg.A << 7);
    reg.z = 0;
    reg.n = 0;
    reg.h = 0;
}
void Cpu::RRA() {  // 0x1F
    u8 carry = reg.c ? 0x80 : 0;
    reg.c    = (reg.A & 0x01) != 0;
    reg.A    = (reg.A >> 1) | carry;
    reg.z    = 0;
    reg.n    = 0;
    reg.h    = 0;
}

// =============================================================
//  PREFIXED
// =============================================================
void Cpu::RLC_B() { RLC_R(reg.B); }
void Cpu::RLC_C() { RLC_R(reg.C); }
void Cpu::RLC_D() { RLC_R(reg.D); }
void Cpu::RLC_E() { RLC_R(reg.E); }
void Cpu::RLC_H() { RLC_R(reg.H); }
void Cpu::RLC_L() { RLC_R(reg.L); }
void Cpu::RLC_A() { RLC_R(reg.A); }
void Cpu::RLC_HL_a16() {
    u8 val = mmu.read_u8(reg.HL);
    reg.c  = (val & 0x80) != 0;
    val    = (val << 1) | (val >> 7);
    mmu.write_u8(reg.HL, val);
    reg.z = val == 0;
    reg.n = 0;
    reg.h = 0;
}

void Cpu::RRC_B() { RRC_R(reg.B); }
void Cpu::RRC_C() { RRC_R(reg.C); }
void Cpu::RRC_D() { RRC_R(reg.D); }
void Cpu::RRC_E() { RRC_R(reg.E); }
void Cpu::RRC_H() { RRC_R(reg.H); }
void Cpu::RRC_L() { RRC_R(reg.L); }
void Cpu::RRC_A() { RRC_R(reg.A); }
void Cpu::RRC_HL_a16() {
    u8 val = mmu.read_u8(reg.HL);
    reg.c  = (val & 0x01) != 0;
    val    = (val >> 1) | (val << 7);
    mmu.write_u8(reg.HL, val);
    reg.z = val == 0;
    reg.n = 0;
    reg.h = 0;
}

void Cpu::RL_B() { RL_R(reg.B); }
void Cpu::RL_C() { RL_R(reg.C); }
void Cpu::RL_D() { RL_R(reg.D); }
void Cpu::RL_E() { RL_R(reg.E); }
void Cpu::RL_H() { RL_R(reg.H); }
void Cpu::RL_L() { RL_R(reg.L); }
void Cpu::RL_A() { RL_R(reg.A); }
void Cpu::RL_HL_a16() {
    u8 val   = mmu.read_u8(reg.HL);
    u8 carry = reg.c ? 1 : 0;
    reg.c    = (val & 0x80) != 0;
    val      = (val << 1) | carry;
    reg.z    = val == 0;
    reg.n    = 0;
    reg.h    = 0;
    mmu.write_u8(reg.HL, val);
}

void Cpu::RR_B() { RR_R(reg.B); }
void Cpu::RR_C() { RR_R(reg.C); }
void Cpu::RR_D() { RR_R(reg.D); }
void Cpu::RR_E() { RR_R(reg.E); }
void Cpu::RR_H() { RR_R(reg.H); }
void Cpu::RR_L() { RR_R(reg.L); }
void Cpu::RR_A() { RR_R(reg.A); }
void Cpu::RR_HL_a16() {
    u8 val   = mmu.read_u8(reg.HL);
    u8 carry = reg.c ? 0x80 : 0;
    reg.c    = (val & 0x01) != 0;
    val      = (val >> 1) | carry;
    reg.z    = val == 0;
    reg.n    = 0;
    reg.h    = 0;
    mmu.write_u8(reg.HL, val);
}

void Cpu::SLA_B() { SLA_R(reg.B); }
void Cpu::SLA_C() { SLA_R(reg.C); }
void Cpu::SLA_D() { SLA_R(reg.D); }
void Cpu::SLA_E() { SLA_R(reg.E); }
void Cpu::SLA_H() { SLA_R(reg.H); }
void Cpu::SLA_L() { SLA_R(reg.L); }
void Cpu::SLA_A() { SLA_R(reg.A); }
void Cpu::SLA_HL_a16() {
    u8 val = mmu.read_u8(reg.HL);
    reg.c  = (val & 0x80) != 0;
    val <<= 1;
    reg.z = val == 0;
    reg.n = 0;
    reg.h = 0;
    mmu.write_u8(reg.HL, val);
}

void Cpu::SRA_B() { SRA_R(reg.B); }
void Cpu::SRA_C() { SRA_R(reg.C); }
void Cpu::SRA_D() { SRA_R(reg.D); }
void Cpu::SRA_E() { SRA_R(reg.E); }
void Cpu::SRA_H() { SRA_R(reg.H); }
void Cpu::SRA_L() { SRA_R(reg.L); }
void Cpu::SRA_A() { SRA_R(reg.A); }
void Cpu::SRA_HL_a16() {
    u8 val = mmu.read_u8(reg.HL);
    reg.c  = (val & 0x01) != 0;
    val    = (val >> 1) | (val & 0x80);
    reg.z  = val == 0;
    reg.n  = 0;
    reg.h  = 0;
    mmu.write_u8(reg.HL, val);
}

void Cpu::SWAP_B() { SWAP_R(reg.B); }
void Cpu::SWAP_C() { SWAP_R(reg.C); }
void Cpu::SWAP_D() { SWAP_R(reg.D); }
void Cpu::SWAP_E() { SWAP_R(reg.E); }
void Cpu::SWAP_H() { SWAP_R(reg.H); }
void Cpu::SWAP_L() { SWAP_R(reg.L); }
void Cpu::SWAP_A() { SWAP_R(reg.A); }
void Cpu::SWAP_HL_a16() {
    u8 val = mmu.read_u8(reg.HL);
    val    = (((val & 0xF0) >> 4) | ((val & 0x0F) << 4));
    reg.z  = val == 0;
    reg.n  = 0;
    reg.h  = 0;
    reg.c  = 0;
    mmu.write_u8(reg.HL, val);
}

void Cpu::SRL_B() { SRL_R(reg.B); }
void Cpu::SRL_C() { SRL_R(reg.C); }
void Cpu::SRL_D() { SRL_R(reg.D); }
void Cpu::SRL_E() { SRL_R(reg.E); }
void Cpu::SRL_H() { SRL_R(reg.H); }
void Cpu::SRL_L() { SRL_R(reg.L); }
void Cpu::SRL_A() { SRL_R(reg.A); }
void Cpu::SRL_HL_a16() {
    u8 val = mmu.read_u8(reg.HL);
    reg.c  = (val & 0x01) != 0;
    val >>= 1;
    reg.z = val == 0;
    reg.n = 0;
    reg.h = 0;
    mmu.write_u8(reg.HL, val);
}

void Cpu::BIT_0_B() { BIT_R(0, reg.B); }
void Cpu::BIT_0_C() { BIT_R(0, reg.C); }
void Cpu::BIT_0_D() { BIT_R(0, reg.D); }
void Cpu::BIT_0_E() { BIT_R(0, reg.E); }
void Cpu::BIT_0_H() { BIT_R(0, reg.H); }
void Cpu::BIT_0_L() { BIT_R(0, reg.L); }
void Cpu::BIT_0_A() { BIT_R(0, reg.A); }
void Cpu::BIT_1_B() { BIT_R(1, reg.B); }
void Cpu::BIT_1_C() { BIT_R(1, reg.C); }
void Cpu::BIT_1_D() { BIT_R(1, reg.D); }
void Cpu::BIT_1_E() { BIT_R(1, reg.E); }
void Cpu::BIT_1_H() { BIT_R(1, reg.H); }
void Cpu::BIT_1_L() { BIT_R(1, reg.L); }
void Cpu::BIT_1_A() { BIT_R(1, reg.A); }
void Cpu::BIT_2_B() { BIT_R(2, reg.B); }
void Cpu::BIT_2_C() { BIT_R(2, reg.C); }
void Cpu::BIT_2_D() { BIT_R(2, reg.D); }
void Cpu::BIT_2_E() { BIT_R(2, reg.E); }
void Cpu::BIT_2_H() { BIT_R(2, reg.H); }
void Cpu::BIT_2_L() { BIT_R(2, reg.L); }
void Cpu::BIT_2_A() { BIT_R(2, reg.A); }
void Cpu::BIT_3_B() { BIT_R(3, reg.B); }
void Cpu::BIT_3_C() { BIT_R(3, reg.C); }
void Cpu::BIT_3_D() { BIT_R(3, reg.D); }
void Cpu::BIT_3_E() { BIT_R(3, reg.E); }
void Cpu::BIT_3_H() { BIT_R(3, reg.H); }
void Cpu::BIT_3_L() { BIT_R(3, reg.L); }
void Cpu::BIT_3_A() { BIT_R(3, reg.A); }
void Cpu::BIT_4_B() { BIT_R(4, reg.B); }
void Cpu::BIT_4_C() { BIT_R(4, reg.C); }
void Cpu::BIT_4_D() { BIT_R(4, reg.D); }
void Cpu::BIT_4_E() { BIT_R(4, reg.E); }
void Cpu::BIT_4_H() { BIT_R(4, reg.H); }
void Cpu::BIT_4_L() { BIT_R(4, reg.L); }
void Cpu::BIT_4_A() { BIT_R(4, reg.A); }
void Cpu::BIT_5_B() { BIT_R(5, reg.B); }
void Cpu::BIT_5_C() { BIT_R(5, reg.C); }
void Cpu::BIT_5_D() { BIT_R(5, reg.D); }
void Cpu::BIT_5_E() { BIT_R(5, reg.E); }
void Cpu::BIT_5_H() { BIT_R(5, reg.H); }
void Cpu::BIT_5_L() { BIT_R(5, reg.L); }
void Cpu::BIT_5_A() { BIT_R(5, reg.A); }
void Cpu::BIT_6_B() { BIT_R(6, reg.B); }
void Cpu::BIT_6_C() { BIT_R(6, reg.C); }
void Cpu::BIT_6_D() { BIT_R(6, reg.D); }
void Cpu::BIT_6_E() { BIT_R(6, reg.E); }
void Cpu::BIT_6_H() { BIT_R(6, reg.H); }
void Cpu::BIT_6_L() { BIT_R(6, reg.L); }
void Cpu::BIT_6_A() { BIT_R(6, reg.A); }
void Cpu::BIT_7_B() { BIT_R(7, reg.B); }
void Cpu::BIT_7_C() { BIT_R(7, reg.C); }
void Cpu::BIT_7_D() { BIT_R(7, reg.D); }
void Cpu::BIT_7_E() { BIT_R(7, reg.E); }
void Cpu::BIT_7_H() { BIT_R(7, reg.H); }
void Cpu::BIT_7_L() { BIT_R(7, reg.L); }
void Cpu::BIT_7_A() { BIT_R(7, reg.A); }
void Cpu::BIT_0_HL_a16() { BIT_R(0, mmu.read_u8(reg.HL)); }
void Cpu::BIT_1_HL_a16() { BIT_R(1, mmu.read_u8(reg.HL)); }
void Cpu::BIT_2_HL_a16() { BIT_R(2, mmu.read_u8(reg.HL)); }
void Cpu::BIT_3_HL_a16() { BIT_R(3, mmu.read_u8(reg.HL)); }
void Cpu::BIT_4_HL_a16() { BIT_R(4, mmu.read_u8(reg.HL)); }
void Cpu::BIT_5_HL_a16() { BIT_R(5, mmu.read_u8(reg.HL)); }
void Cpu::BIT_6_HL_a16() { BIT_R(6, mmu.read_u8(reg.HL)); }
void Cpu::BIT_7_HL_a16() { BIT_R(7, mmu.read_u8(reg.HL)); }

void Cpu::RES_0_B() { RES_R(0, reg.B); }
void Cpu::RES_0_C() { RES_R(0, reg.C); }
void Cpu::RES_0_D() { RES_R(0, reg.D); }
void Cpu::RES_0_E() { RES_R(0, reg.E); }
void Cpu::RES_0_H() { RES_R(0, reg.H); }
void Cpu::RES_0_L() { RES_R(0, reg.L); }
void Cpu::RES_0_A() { RES_R(0, reg.A); }
void Cpu::RES_1_B() { RES_R(1, reg.B); }
void Cpu::RES_1_C() { RES_R(1, reg.C); }
void Cpu::RES_1_D() { RES_R(1, reg.D); }
void Cpu::RES_1_E() { RES_R(1, reg.E); }
void Cpu::RES_1_H() { RES_R(1, reg.H); }
void Cpu::RES_1_L() { RES_R(1, reg.L); }
void Cpu::RES_1_A() { RES_R(1, reg.A); }
void Cpu::RES_2_B() { RES_R(2, reg.B); }
void Cpu::RES_2_C() { RES_R(2, reg.C); }
void Cpu::RES_2_D() { RES_R(2, reg.D); }
void Cpu::RES_2_E() { RES_R(2, reg.E); }
void Cpu::RES_2_H() { RES_R(2, reg.H); }
void Cpu::RES_2_L() { RES_R(2, reg.L); }
void Cpu::RES_2_A() { RES_R(2, reg.A); }
void Cpu::RES_3_B() { RES_R(3, reg.B); }
void Cpu::RES_3_C() { RES_R(3, reg.C); }
void Cpu::RES_3_D() { RES_R(3, reg.D); }
void Cpu::RES_3_E() { RES_R(3, reg.E); }
void Cpu::RES_3_H() { RES_R(3, reg.H); }
void Cpu::RES_3_L() { RES_R(3, reg.L); }
void Cpu::RES_3_A() { RES_R(3, reg.A); }
void Cpu::RES_4_B() { RES_R(4, reg.B); }
void Cpu::RES_4_C() { RES_R(4, reg.C); }
void Cpu::RES_4_D() { RES_R(4, reg.D); }
void Cpu::RES_4_E() { RES_R(4, reg.E); }
void Cpu::RES_4_H() { RES_R(4, reg.H); }
void Cpu::RES_4_L() { RES_R(4, reg.L); }
void Cpu::RES_4_A() { RES_R(4, reg.A); }
void Cpu::RES_5_B() { RES_R(5, reg.B); }
void Cpu::RES_5_C() { RES_R(5, reg.C); }
void Cpu::RES_5_D() { RES_R(5, reg.D); }
void Cpu::RES_5_E() { RES_R(5, reg.E); }
void Cpu::RES_5_H() { RES_R(5, reg.H); }
void Cpu::RES_5_L() { RES_R(5, reg.L); }
void Cpu::RES_5_A() { RES_R(5, reg.A); }
void Cpu::RES_6_B() { RES_R(6, reg.B); }
void Cpu::RES_6_C() { RES_R(6, reg.C); }
void Cpu::RES_6_D() { RES_R(6, reg.D); }
void Cpu::RES_6_E() { RES_R(6, reg.E); }
void Cpu::RES_6_H() { RES_R(6, reg.H); }
void Cpu::RES_6_L() { RES_R(6, reg.L); }
void Cpu::RES_6_A() { RES_R(6, reg.A); }
void Cpu::RES_7_B() { RES_R(7, reg.B); }
void Cpu::RES_7_C() { RES_R(7, reg.C); }
void Cpu::RES_7_D() { RES_R(7, reg.D); }
void Cpu::RES_7_E() { RES_R(7, reg.E); }
void Cpu::RES_7_H() { RES_R(7, reg.H); }
void Cpu::RES_7_L() { RES_R(7, reg.L); }
void Cpu::RES_7_A() { RES_R(7, reg.A); }
void Cpu::RES_0_HL_a16() { RES_HL(0); }
void Cpu::RES_1_HL_a16() { RES_HL(1); }
void Cpu::RES_2_HL_a16() { RES_HL(2); }
void Cpu::RES_3_HL_a16() { RES_HL(3); }
void Cpu::RES_4_HL_a16() { RES_HL(4); }
void Cpu::RES_5_HL_a16() { RES_HL(5); }
void Cpu::RES_6_HL_a16() { RES_HL(6); }
void Cpu::RES_7_HL_a16() { RES_HL(7); }

void Cpu::SET_0_B() { SET_R(0, reg.B); }
void Cpu::SET_0_C() { SET_R(0, reg.C); }
void Cpu::SET_0_D() { SET_R(0, reg.D); }
void Cpu::SET_0_E() { SET_R(0, reg.E); }
void Cpu::SET_0_H() { SET_R(0, reg.H); }
void Cpu::SET_0_L() { SET_R(0, reg.L); }
void Cpu::SET_0_A() { SET_R(0, reg.A); }
void Cpu::SET_1_B() { SET_R(1, reg.B); }
void Cpu::SET_1_C() { SET_R(1, reg.C); }
void Cpu::SET_1_D() { SET_R(1, reg.D); }
void Cpu::SET_1_E() { SET_R(1, reg.E); }
void Cpu::SET_1_H() { SET_R(1, reg.H); }
void Cpu::SET_1_L() { SET_R(1, reg.L); }
void Cpu::SET_1_A() { SET_R(1, reg.A); }
void Cpu::SET_2_B() { SET_R(2, reg.B); }
void Cpu::SET_2_C() { SET_R(2, reg.C); }
void Cpu::SET_2_D() { SET_R(2, reg.D); }
void Cpu::SET_2_E() { SET_R(2, reg.E); }
void Cpu::SET_2_H() { SET_R(2, reg.H); }
void Cpu::SET_2_L() { SET_R(2, reg.L); }
void Cpu::SET_2_A() { SET_R(2, reg.A); }
void Cpu::SET_3_B() { SET_R(3, reg.B); }
void Cpu::SET_3_C() { SET_R(3, reg.C); }
void Cpu::SET_3_D() { SET_R(3, reg.D); }
void Cpu::SET_3_E() { SET_R(3, reg.E); }
void Cpu::SET_3_H() { SET_R(3, reg.H); }
void Cpu::SET_3_L() { SET_R(3, reg.L); }
void Cpu::SET_3_A() { SET_R(3, reg.A); }
void Cpu::SET_4_B() { SET_R(4, reg.B); }
void Cpu::SET_4_C() { SET_R(4, reg.C); }
void Cpu::SET_4_D() { SET_R(4, reg.D); }
void Cpu::SET_4_E() { SET_R(4, reg.E); }
void Cpu::SET_4_H() { SET_R(4, reg.H); }
void Cpu::SET_4_L() { SET_R(4, reg.L); }
void Cpu::SET_4_A() { SET_R(4, reg.A); }
void Cpu::SET_5_B() { SET_R(5, reg.B); }
void Cpu::SET_5_C() { SET_R(5, reg.C); }
void Cpu::SET_5_D() { SET_R(5, reg.D); }
void Cpu::SET_5_E() { SET_R(5, reg.E); }
void Cpu::SET_5_H() { SET_R(5, reg.H); }
void Cpu::SET_5_L() { SET_R(5, reg.L); }
void Cpu::SET_5_A() { SET_R(5, reg.A); }
void Cpu::SET_6_B() { SET_R(6, reg.B); }
void Cpu::SET_6_C() { SET_R(6, reg.C); }
void Cpu::SET_6_D() { SET_R(6, reg.D); }
void Cpu::SET_6_E() { SET_R(6, reg.E); }
void Cpu::SET_6_H() { SET_R(6, reg.H); }
void Cpu::SET_6_L() { SET_R(6, reg.L); }
void Cpu::SET_6_A() { SET_R(6, reg.A); }
void Cpu::SET_7_B() { SET_R(7, reg.B); }
void Cpu::SET_7_C() { SET_R(7, reg.C); }
void Cpu::SET_7_D() { SET_R(7, reg.D); }
void Cpu::SET_7_E() { SET_R(7, reg.E); }
void Cpu::SET_7_H() { SET_R(7, reg.H); }
void Cpu::SET_7_L() { SET_R(7, reg.L); }
void Cpu::SET_7_A() { SET_R(7, reg.A); }
void Cpu::SET_0_HL_a16() { SET_HL(0); }
void Cpu::SET_1_HL_a16() { SET_HL(1); }
void Cpu::SET_2_HL_a16() { SET_HL(2); }
void Cpu::SET_3_HL_a16() { SET_HL(3); }
void Cpu::SET_4_HL_a16() { SET_HL(4); }
void Cpu::SET_5_HL_a16() { SET_HL(5); }
void Cpu::SET_6_HL_a16() { SET_HL(6); }
void Cpu::SET_7_HL_a16() { SET_HL(7); }