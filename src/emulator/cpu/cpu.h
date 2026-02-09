#pragma once

#include <array>
#include <functional>
#include <iostream>
#include <string>

#include "../mmu/mmu.h"
#include "registers.h"

class Cpu;

struct CpuState;

using InstructionHandler = void (Cpu::*)();

struct Instruction {
    const char*        name;
    InstructionHandler handler;
    u8                 cycles;
    u8                 length;
};

class Cpu {
   public:
    Mmu&      mmu;
    Registers reg;

    Cpu(Mmu& m);

    std::array<Instruction, 256> instructions;
    std::array<Instruction, 256> cb_instructions;

    bool IME;
    bool halted;
    bool halt_bug = false;

    void save_state(CpuState& state) const;
    void load_state(const CpuState& state);

    inline u8 step() {
        handle_interrupts();

        if (halted) {
            cycles_this_step = 4;
        } else {
            u8 opcode = mmu.read_u8(reg.PC);

            if (halt_bug) {
                halt_bug = false;
            } else {
                reg.PC++;
            }

            Instruction inst       = instructions[opcode];
            this->cycles_this_step = inst.cycles;
            (this->*inst.handler)();
        }

        if (ime_schedule > 0) {
            if (--ime_schedule == 0) {
                IME = true;
            }
        }

        return this->cycles_this_step;
    }

    inline void PREFIX() {
        u8 cb_opcode = fetch_u8();

        Instruction cb_inst = cb_instructions[cb_opcode];

        this->cycles_this_step = cb_inst.cycles;

        (this->*cb_inst.handler)();
    }

   private:
    void init_registers() {
        reg.AF = 0x01B0;
        reg.BC = 0x0013;
        reg.DE = 0x00D8;
        reg.HL = 0x014D;
        reg.SP = 0xFFFE;
        reg.PC = 0x0100;
    }

    u8 cycles_this_step;
    u8 ime_schedule;

    void handle_interrupts();

    void init_instructions();

    void UNIMPLEMENTED();

    void CB_UNIMPLEMENTED();

    // =============================================================
    //  Stack and Bus
    // =============================================================
    inline u16 pop() {
        u8 lo = mmu.read_u8(reg.SP++);
        u8 hi = mmu.read_u8(reg.SP++);

        return lo | (hi << 8);
    }

    inline void push(u16 r) {
        mmu.write_u8(--reg.SP, r >> 8);
        mmu.write_u8(--reg.SP, r & 0xFF);
    }

    inline u8 Cpu::fetch_u8() {
        u8 val = mmu.read_u8(reg.PC++);
        return val;
    }

    inline u16 Cpu::fetch_u16() {
        u16 val = fetch_u8();
        val |= (fetch_u8() << 8);
        return val;
    }

    // =============================================================
    //  Helpers
    // =============================================================
    void LD_R_n8(u8& operand) { operand = fetch_u8(); }

    void LD_R_R(u8& dest, u8 src) { dest = src; }

    void LD_R_HL_a16(u8& dest) { dest = mmu.read_u8(reg.HL); }

    void LD_HL_R(u8 operand) { mmu.write_u8(reg.HL, operand); }

    void BIT_R(u8 bit, u8 operand) {
        reg.z = !((operand >> bit) & 1);
        reg.n = 0;
        reg.h = 1;
    }

    void RES_R(u8 bit, u8& operand) { operand &= ~(1 << bit); }

    void RES_HL(u8 bit) {
        u8 val = mmu.read_u8(reg.HL);
        val &= ~(1 << bit);
        mmu.write_u8(reg.HL, val);
    }

    void SET_R(u8 bit, u8& operand) { operand |= (1 << bit); }

    void SET_HL(u8 bit) {
        u8 val = mmu.read_u8(reg.HL);
        val |= (1 << bit);
        mmu.write_u8(reg.HL, val);
    }

    void RLC_R(u8& operand) {
        reg.c   = (operand & 0x80) != 0;
        operand = (operand << 1) | (operand >> 7);
        reg.z   = operand == 0;
        reg.n   = 0;
        reg.h   = 0;
    }

    void RRC_R(u8& operand) {
        reg.c   = (operand & 0x01) != 0;
        operand = (operand >> 1) | (operand << 7);
        reg.z   = operand == 0;
        reg.n   = 0;
        reg.h   = 0;
    }

    void ADD_A_R(u8 operand) {
        int res = reg.A + operand;
        reg.z   = (res & 0xFF) == 0;
        reg.n   = 0;
        reg.h   = ((reg.A & 0x0F) + (operand & 0x0F)) > 0x0F;
        reg.c   = res > 0xFF;
        reg.A   = res;
    }

    void ADC_A_R(u8 operand) {
        u8  carry = reg.c ? 1 : 0;
        int res   = reg.A + operand + carry;
        reg.z     = (res & 0xFF) == 0;
        reg.n     = 0;
        reg.h     = ((reg.A & 0x0F) + (operand & 0x0F) + carry) > 0x0F;
        reg.c     = res > 0xFF;
        reg.A     = res;
    }

    void SUB_A_R(u8 operand) {
        int res = reg.A - operand;
        reg.z   = (res & 0xFF) == 0;
        reg.n   = 1;
        reg.h   = (reg.A & 0x0F) < (operand & 0x0F);
        reg.c   = reg.A < operand;
        reg.A   = res;
    }

    void SBC_A_R(u8 operand) {
        u8  carry = reg.c ? 1 : 0;
        int res   = reg.A - operand - carry;
        reg.z     = (res & 0xFF) == 0;
        reg.n     = 1;
        reg.h     = (reg.A & 0x0F) < ((operand & 0x0F) + carry);
        reg.c     = reg.A < (operand + carry);
        reg.A     = res;
    }

    void CP_A_R(u8 operand) {
        reg.z = reg.A == operand;
        reg.n = 1;
        reg.h = (reg.A & 0x0F) < (operand & 0x0F);
        reg.c = reg.A < operand;
    }

    void XOR_A_R(u8 operand) {
        reg.A ^= operand;
        reg.z = reg.A == 0;
        reg.n = 0;
        reg.h = 0;
        reg.c = 0;
    }

    void OR_A_R(u8 operand) {
        reg.A |= operand;
        reg.z = reg.A == 0;
        reg.n = 0;
        reg.h = 0;
        reg.c = 0;
    }

    void AND_A_R(u8 operand) {
        reg.A &= operand;
        reg.z = reg.A == 0;
        reg.n = 0;
        reg.h = 1;
        reg.c = 0;
    }

    void JP_COND(bool cond) {
        u16 addr = fetch_u16();
        if (cond) {
            reg.PC = addr;
            cycles_this_step += 4;
        }
    }

    void JR_COND(bool cond) {
        i8 offset = fetch_u8();
        if (cond) {
            reg.PC += offset;
            cycles_this_step += 4;
        }
    }

    void RET_COND(bool cond) {
        if (cond) {
            reg.PC = pop();
            cycles_this_step += 12;
        }
    }

    void CALL_COND(bool cond) {
        u16 addr = fetch_u16();
        if (cond) {
            push(reg.PC);
            reg.PC = addr;
            cycles_this_step += 12;
        }
    }

    void RST(u16 addr) {
        push(reg.PC);
        reg.PC = addr;
    }

    void INC_R16(u16& operand) { operand++; }

    void DEC_R16(u16& operand) { operand--; }

    void ADD_HL_R16(u16 operand) {
        int res = reg.HL + operand;
        reg.n   = 0;
        reg.h   = ((reg.HL & 0x0FFF) + (operand & 0x0FFF)) > 0x0FFF;
        reg.c   = res > 0xFFFF;
        reg.HL  = res;
    }

    void DEC_R(u8& operand) {
        reg.h = (operand & 0x0F) == 0x00;
        operand--;
        reg.z = operand == 0;
        reg.n = 1;
    }

    void INC_R(u8& operand) {
        reg.h = (operand & 0x0F) == 0x0F;
        operand++;
        reg.z = operand == 0;
        reg.n = 0;
    }

    void SWAP_R(u8& operand) {
        operand = (((operand & 0xF0) >> 4) | ((operand & 0x0F) << 4));
        reg.z   = operand == 0;
        reg.n   = 0;
        reg.h   = 0;
        reg.c   = 0;
    }

    void SRL_R(u8& operand) {
        reg.c = (operand & 0x01) != 0;
        operand >>= 1;
        reg.z = operand == 0;
        reg.n = 0;
        reg.h = 0;
    }

    void RL_R(u8& operand) {
        u8 carry = reg.c ? 1 : 0;
        reg.c    = (operand & 0x80) != 0;
        operand  = (operand << 1) | carry;
        reg.z    = operand == 0;
        reg.n    = 0;
        reg.h    = 0;
    }

    void RR_R(u8& operand) {
        u8 carry = reg.c ? 0x80 : 0;
        reg.c    = (operand & 0x01) != 0;
        operand  = (operand >> 1) | carry;
        reg.z    = operand == 0;
        reg.n    = 0;
        reg.h    = 0;
    }

    void SLA_R(u8& operand) {
        reg.c = (operand & 0x80) != 0;
        operand <<= 1;
        reg.z = operand == 0;
        reg.n = 0;
        reg.h = 0;
    }

    void SRA_R(u8& operand) {
        reg.c   = (operand & 0x01) != 0;
        operand = (operand >> 1) | (operand & 0x80);
        reg.z   = operand == 0;
        reg.n   = 0;
        reg.h   = 0;
    }

    // =============================================================
    //  Control & Misc
    // =============================================================
    void NOP();   // 0x00
    void STOP();  // 0x10
    void HALT();  // 0x76
    void DI();    // 0xF3
    void EI();    // 0xFB

    // =============================================================
    //  Jump Calls
    // =============================================================
    void JR_NZ_e8();     // 0x20
    void JR_NC_e8();     // 0x30
    void JR_e8();        // 0x18
    void JR_Z_e8();      // 0x28
    void JR_C_e8();      // 0x38
    void RET_NZ();       // 0xC0
    void RET_NC();       // 0xD0
    void JP_NZ_a16();    // 0xC2
    void JP_NC_a16();    // 0xD2
    void JP_a16();       // 0xC3
    void CALL_NZ_a16();  // 0xC4
    void CALL_NC_a16();  // 0xD4
    void RST_00();       // 0xC7
    void RST_10();       // 0xD7
    void RST_20();       // 0xE7
    void RST_30();       // 0xF7
    void RET_Z();        // 0xC8
    void RET_C();        // 0xD8
    void RET();          // 0xC9
    void RETI();         // 0xD9
    void JP_HL();        // 0xE9
    void JP_Z_a16();     // 0xCA
    void JP_C_a16();     // 0xDA
    void CALL_Z_a16();   // 0xCC
    void CALL_C_a16();   // 0xDC
    void CALL_a16();     // 0xCD
    void RST_08();       // 0xCF
    void RST_18();       // 0xDF
    void RST_28();       // 0xEF
    void RST_38();       // 0xFF

    // =============================================================
    //  8-bit Load
    // =============================================================
    void LD_BC_a16_A();   // 0x02
    void LD_DE_a16_A();   // 0x12
    void LD_HLi_a16_A();  // 0x22
    void LD_HLd_a16_A();  // 0x32
    void LD_B_n8();       // 0x06
    void LD_D_n8();       // 0x16
    void LD_H_n8();       // 0x26
    void LD_HL_a16_n8();  // 0x36
    void LD_A_BC_a16();   // 0x0A
    void LD_A_DE_a16();   // 0x1A
    void LD_A_HLi_a16();  // 0x2A
    void LD_A_HLd_a16();  // 0x3A
    void LD_C_n8();       // 0x0E
    void LD_E_n8();       // 0x1E
    void LD_L_n8();       // 0x2E
    void LD_A_n8();       // 0x3E

    // =============================================================
    //  LD r, r'
    // =============================================================
    void LD_B_B();       // 0x40
    void LD_B_C();       // 0x41
    void LD_B_D();       // 0x42
    void LD_B_E();       // 0x43
    void LD_B_H();       // 0x44
    void LD_B_L();       // 0x45
    void LD_B_HL_a16();  // 0x46
    void LD_B_A();       // 0x47
    void LD_C_B();       // 0x48
    void LD_C_C();       // 0x49
    void LD_C_D();       // 0x4A
    void LD_C_E();       // 0x4B
    void LD_C_H();       // 0x4C
    void LD_C_L();       // 0x4D
    void LD_C_HL_a16();  // 0x4E
    void LD_C_A();       // 0x4F
    void LD_D_B();       // 0x50
    void LD_D_C();       // 0x51
    void LD_D_D();       // 0x52
    void LD_D_E();       // 0x53
    void LD_D_H();       // 0x54
    void LD_D_L();       // 0x55
    void LD_D_HL_a16();  // 0x56
    void LD_D_A();       // 0x57
    void LD_E_B();       // 0x58
    void LD_E_C();       // 0x59
    void LD_E_D();       // 0x5A
    void LD_E_E();       // 0x5B
    void LD_E_H();       // 0x5C
    void LD_E_L();       // 0x5D
    void LD_E_HL_a16();  // 0x5E
    void LD_E_A();       // 0x5F
    void LD_H_B();       // 0x60
    void LD_H_C();       // 0x61
    void LD_H_D();       // 0x62
    void LD_H_E();       // 0x63
    void LD_H_H();       // 0x64
    void LD_H_L();       // 0x65
    void LD_H_HL_a16();  // 0x66
    void LD_H_A();       // 0x67
    void LD_L_B();       // 0x68
    void LD_L_C();       // 0x69
    void LD_L_D();       // 0x6A
    void LD_L_E();       // 0x6B
    void LD_L_H();       // 0x6C
    void LD_L_L();       // 0x6D
    void LD_L_HL_a16();  // 0x6E
    void LD_L_A();       // 0x6F
    void LD_HL_a16_B();  // 0x70
    void LD_HL_a16_C();  // 0x71
    void LD_HL_a16_D();  // 0x72
    void LD_HL_a16_E();  // 0x73
    void LD_HL_a16_H();  // 0x74
    void LD_HL_a16_L();  // 0x75
    void LD_HL_a16_A();  // 0x77
    void LD_A_B();       // 0x78
    void LD_A_C();       // 0x79
    void LD_A_D();       // 0x7A
    void LD_A_E();       // 0x7B
    void LD_A_H();       // 0x7C
    void LD_A_L();       // 0x7D
    void LD_A_HL_a16();  // 0x7E
    void LD_A_A();       // 0x7F

    // =============================================================
    //  Other 8-bit Load
    // =============================================================
    void LDH_a8_A();    // 0xE0
    void LDH_A_a8();    // 0xF0
    void LD_C_a16_A();  // 0xE2
    void LD_A_C_a16();  // 0xF2
    void LD_a16_A();    // 0xEA
    void LD_A_a16();    // 0xFA

    // =============================================================
    //  16-bit Load (& Stack)
    // =============================================================
    void LD_BC_n16();    // 0x01
    void LD_DE_n16();    // 0x11
    void LD_HL_n16();    // 0x21
    void LD_SP_n16();    // 0x31
    void LD_a16_SP();    // 0x08
    void POP_BC();       // 0xC1
    void POP_DE();       // 0xD1
    void POP_HL();       // 0xE1
    void POP_AF();       // 0xF1
    void PUSH_BC();      // 0xC5
    void PUSH_DE();      // 0xD5
    void PUSH_HL();      // 0xE5
    void PUSH_AF();      // 0xF5
    void LD_HL_SP_e8();  // 0xF8
    void LD_SP_HL();     // 0xF9

    // =============================================================
    //  8-bit Arithmetic & Logical
    // =============================================================
    void INC_B();         // 0x04
    void INC_D();         // 0x14
    void INC_H();         // 0x24
    void INC_HL_a16();    // 0x34
    void DEC_B();         // 0x05
    void DEC_D();         // 0x15
    void DEC_H();         // 0x25
    void DEC_HL_a16();    // 0x35
    void DAA();           // 0x27
    void SCF();           // 0x37
    void INC_C();         // 0x0C
    void INC_E();         // 0x1C
    void INC_L();         // 0x2C
    void INC_A();         // 0x3C
    void DEC_C();         // 0x0D
    void DEC_E();         // 0x1D
    void DEC_L();         // 0x2D
    void DEC_A();         // 0x3D
    void CPL();           // 0x2F
    void CCF();           // 0x3F
    void ADD_A_B();       // 0x80
    void ADD_A_C();       // 0x81
    void ADD_A_D();       // 0x82
    void ADD_A_E();       // 0x83
    void ADD_A_H();       // 0x84
    void ADD_A_L();       // 0x85
    void ADD_A_HL_a16();  // 0x86
    void ADD_A_A();       // 0x87
    void ADC_A_B();       // 0x88
    void ADC_A_C();       // 0x89
    void ADC_A_D();       // 0x8A
    void ADC_A_E();       // 0x8B
    void ADC_A_H();       // 0x8C
    void ADC_A_L();       // 0x8D
    void ADC_A_HL_a16();  // 0x8E
    void ADC_A_A();       // 0x8F
    void SUB_A_B();       // 0x90
    void SUB_A_C();       // 0x91
    void SUB_A_D();       // 0x92
    void SUB_A_E();       // 0x93
    void SUB_A_H();       // 0x94
    void SUB_A_L();       // 0x95
    void SUB_A_HL_a16();  // 0x96
    void SUB_A_A();       // 0x97
    void SBC_A_B();       // 0x98
    void SBC_A_C();       // 0x99
    void SBC_A_D();       // 0x9A
    void SBC_A_E();       // 0x9B
    void SBC_A_H();       // 0x9C
    void SBC_A_L();       // 0x9D
    void SBC_A_HL_a16();  // 0x9E
    void SBC_A_A();       // 0x9F
    void AND_A_B();       // 0xA0
    void AND_A_C();       // 0xA1
    void AND_A_D();       // 0xA2
    void AND_A_E();       // 0xA3
    void AND_A_H();       // 0xA4
    void AND_A_L();       // 0xA5
    void AND_A_HL_a16();  // 0xA6
    void AND_A_A();       // 0xA7
    void XOR_A_B();       // 0xA8
    void XOR_A_C();       // 0xA9
    void XOR_A_D();       // 0xAA
    void XOR_A_E();       // 0xAB
    void XOR_A_H();       // 0xAC
    void XOR_A_L();       // 0xAD
    void XOR_A_HL_a16();  // 0xAE
    void XOR_A_A();       // 0xAF
    void OR_A_B();        // 0xB0
    void OR_A_C();        // 0xB1
    void OR_A_D();        // 0xB2
    void OR_A_E();        // 0xB3
    void OR_A_H();        // 0xB4
    void OR_A_L();        // 0xB5
    void OR_A_HL_a16();   // 0xB6
    void OR_A_A();        // 0xB7
    void CP_A_B();        // 0xB8
    void CP_A_C();        // 0xB9
    void CP_A_D();        // 0xBA
    void CP_A_E();        // 0xBB
    void CP_A_H();        // 0xBC
    void CP_A_L();        // 0xBD
    void CP_A_HL_a16();   // 0xBE
    void CP_A_A();        // 0xBF
    void ADD_A_n8();      // 0xC6
    void SUB_A_n8();      // 0xD6
    void AND_A_n8();      // 0xE6
    void OR_A_n8();       // 0xF6
    void ADC_A_n8();      // 0xCE
    void SBC_A_n8();      // 0xDE
    void XOR_A_n8();      // 0xEE
    void CP_A_n8();       // 0xEF

    // =============================================================
    //  16-bit Arithmetic & Logical
    // =============================================================
    void INC_BC();     // 0x03
    void INC_DE();     // 0x13
    void INC_HL();     // 0x23
    void INC_SP();     // 0x33
    void ADD_HL_BC();  // 0x09
    void ADD_HL_DE();  // 0x19
    void ADD_HL_HL();  // 0x29
    void ADD_HL_SP();  // 0x39
    void DEC_BC();     // 0x0B
    void DEC_DE();     // 0x1B
    void DEC_HL();     // 0x2B
    void DEC_SP();     // 0x3B
    void ADD_SP_e8();  // 0xE8

    // =============================================================
    //  8-bit Bit Ops
    // =============================================================
    void RLCA();  // 0x07
    void RLA();   // 0x17
    void RRCA();  // 0x0F
    void RRA();   // 0x1F

    // =============================================================
    //  PREFIXED
    // =============================================================
    void RLC_B();       // 0x00
    void RLC_C();       // 0x01
    void RLC_D();       // 0x02
    void RLC_E();       // 0x03
    void RLC_H();       // 0x04
    void RLC_L();       // 0x05
    void RLC_HL_a16();  // 0x06
    void RLC_A();       // 0x07
    void RRC_B();       // 0x08
    void RRC_C();       // 0x09
    void RRC_D();       // 0x0A
    void RRC_E();       // 0x0B
    void RRC_H();       // 0x0C
    void RRC_L();       // 0x0D
    void RRC_HL_a16();  // 0x0E
    void RRC_A();       // 0x0F

    void RL_B();       // 0x10
    void RL_C();       // 0x11
    void RL_D();       // 0x12
    void RL_E();       // 0x13
    void RL_H();       // 0x14
    void RL_L();       // 0x15
    void RL_HL_a16();  // 0x16
    void RL_A();       // 0x17
    void RR_B();       // 0x18
    void RR_C();       // 0x19
    void RR_D();       // 0x1A
    void RR_E();       // 0x1B
    void RR_H();       // 0x1C
    void RR_L();       // 0x1D
    void RR_HL_a16();  // 0x1E
    void RR_A();       // 0x1F

    void SLA_B();       // 0x20
    void SLA_C();       // 0x21
    void SLA_D();       // 0x22
    void SLA_E();       // 0x23
    void SLA_H();       // 0x24
    void SLA_L();       // 0x25
    void SLA_HL_a16();  // 0x26
    void SLA_A();       // 0x27
    void SRA_B();       // 0x28
    void SRA_C();       // 0x29
    void SRA_D();       // 0x2A
    void SRA_E();       // 0x2B
    void SRA_H();       // 0x2C
    void SRA_L();       // 0x2D
    void SRA_HL_a16();  // 0x2E
    void SRA_A();       // 0x2F

    void SWAP_B();       // 0x30
    void SWAP_C();       // 0x31
    void SWAP_D();       // 0x32
    void SWAP_E();       // 0x33
    void SWAP_H();       // 0x34
    void SWAP_L();       // 0x35
    void SWAP_HL_a16();  // 0x36
    void SWAP_A();       // 0x37
    void SRL_B();        // 0x38
    void SRL_C();        // 0x39
    void SRL_D();        // 0x3A
    void SRL_E();        // 0x3B
    void SRL_H();        // 0x3C
    void SRL_L();        // 0x3D
    void SRL_HL_a16();   // 0x3E
    void SRL_A();        // 0x3F

    void BIT_0_B();       // 0x40
    void BIT_0_C();       // 0x41
    void BIT_0_D();       // 0x42
    void BIT_0_E();       // 0x43
    void BIT_0_H();       // 0x44
    void BIT_0_L();       // 0x45
    void BIT_0_HL_a16();  // 0x46
    void BIT_0_A();       // 0x47
    void BIT_1_B();       // 0x48
    void BIT_1_C();       // 0x49
    void BIT_1_D();       // 0x4A
    void BIT_1_E();       // 0x4B
    void BIT_1_H();       // 0x4C
    void BIT_1_L();       // 0x4D
    void BIT_1_HL_a16();  // 0x4E
    void BIT_1_A();       // 0x4F

    void BIT_2_B();       // 0x50
    void BIT_2_C();       // 0x51
    void BIT_2_D();       // 0x52
    void BIT_2_E();       // 0x53
    void BIT_2_H();       // 0x54
    void BIT_2_L();       // 0x55
    void BIT_2_HL_a16();  // 0x56
    void BIT_2_A();       // 0x57
    void BIT_3_B();       // 0x58
    void BIT_3_C();       // 0x59
    void BIT_3_D();       // 0x5A
    void BIT_3_E();       // 0x5B
    void BIT_3_H();       // 0x5C
    void BIT_3_L();       // 0x5D
    void BIT_3_HL_a16();  // 0x5E
    void BIT_3_A();       // 0x5F

    void BIT_4_B();       // 0x60
    void BIT_4_C();       // 0x61
    void BIT_4_D();       // 0x62
    void BIT_4_E();       // 0x63
    void BIT_4_H();       // 0x64
    void BIT_4_L();       // 0x65
    void BIT_4_HL_a16();  // 0x66
    void BIT_4_A();       // 0x67
    void BIT_5_B();       // 0x68
    void BIT_5_C();       // 0x69
    void BIT_5_D();       // 0x6A
    void BIT_5_E();       // 0x6B
    void BIT_5_H();       // 0x6C
    void BIT_5_L();       // 0x6D
    void BIT_5_HL_a16();  // 0x6E
    void BIT_5_A();       // 0x6F

    void BIT_6_B();       // 0x70
    void BIT_6_C();       // 0x71
    void BIT_6_D();       // 0x72
    void BIT_6_E();       // 0x73
    void BIT_6_H();       // 0x74
    void BIT_6_L();       // 0x75
    void BIT_6_HL_a16();  // 0x76
    void BIT_6_A();       // 0x77
    void BIT_7_B();       // 0x78
    void BIT_7_C();       // 0x79
    void BIT_7_D();       // 0x7A
    void BIT_7_E();       // 0x7B
    void BIT_7_H();       // 0x7C
    void BIT_7_L();       // 0x7D
    void BIT_7_HL_a16();  // 0x7E
    void BIT_7_A();       // 0x7F

    void RES_0_B();       // 0x80
    void RES_0_C();       // 0x81
    void RES_0_D();       // 0x82
    void RES_0_E();       // 0x83
    void RES_0_H();       // 0x84
    void RES_0_L();       // 0x85
    void RES_0_HL_a16();  // 0x86
    void RES_0_A();       // 0x87
    void RES_1_B();       // 0x88
    void RES_1_C();       // 0x89
    void RES_1_D();       // 0x8A
    void RES_1_E();       // 0x8B
    void RES_1_H();       // 0x8C
    void RES_1_L();       // 0x8D
    void RES_1_HL_a16();  // 0x8E
    void RES_1_A();       // 0x8F

    void RES_2_B();       // 0x90
    void RES_2_C();       // 0x91
    void RES_2_D();       // 0x92
    void RES_2_E();       // 0x93
    void RES_2_H();       // 0x94
    void RES_2_L();       // 0x95
    void RES_2_HL_a16();  // 0x96
    void RES_2_A();       // 0x97
    void RES_3_B();       // 0x98
    void RES_3_C();       // 0x99
    void RES_3_D();       // 0x9A
    void RES_3_E();       // 0x9B
    void RES_3_H();       // 0x9C
    void RES_3_L();       // 0x9D
    void RES_3_HL_a16();  // 0x9E
    void RES_3_A();       // 0x9F

    void RES_4_B();       // 0xA0
    void RES_4_C();       // 0xA1
    void RES_4_D();       // 0xA2
    void RES_4_E();       // 0xA3
    void RES_4_H();       // 0xA4
    void RES_4_L();       // 0xA5
    void RES_4_HL_a16();  // 0xA6
    void RES_4_A();       // 0xA7
    void RES_5_B();       // 0xA8
    void RES_5_C();       // 0xA9
    void RES_5_D();       // 0xAA
    void RES_5_E();       // 0xAB
    void RES_5_H();       // 0xAC
    void RES_5_L();       // 0xAD
    void RES_5_HL_a16();  // 0xAE
    void RES_5_A();       // 0xAF

    void RES_6_B();       // 0xB0
    void RES_6_C();       // 0xB1
    void RES_6_D();       // 0xB2
    void RES_6_E();       // 0xB3
    void RES_6_H();       // 0xB4
    void RES_6_L();       // 0xB5
    void RES_6_HL_a16();  // 0xB6
    void RES_6_A();       // 0xB7
    void RES_7_B();       // 0xB8
    void RES_7_C();       // 0xB9
    void RES_7_D();       // 0xBA
    void RES_7_E();       // 0xBB
    void RES_7_H();       // 0xBC
    void RES_7_L();       // 0xBD
    void RES_7_HL_a16();  // 0xBE
    void RES_7_A();       // 0xBF

    void SET_0_B();       // 0xC0
    void SET_0_C();       // 0xC1
    void SET_0_D();       // 0xC2
    void SET_0_E();       // 0xC3
    void SET_0_H();       // 0xC4
    void SET_0_L();       // 0xC5
    void SET_0_HL_a16();  // 0xC6
    void SET_0_A();       // 0xC7
    void SET_1_B();       // 0xC8
    void SET_1_C();       // 0xC9
    void SET_1_D();       // 0xCA
    void SET_1_E();       // 0xCB
    void SET_1_H();       // 0xCC
    void SET_1_L();       // 0xCD
    void SET_1_HL_a16();  // 0xCE
    void SET_1_A();       // 0xCF

    void SET_2_B();       // 0xD0
    void SET_2_C();       // 0xD1
    void SET_2_D();       // 0xD2
    void SET_2_E();       // 0xD3
    void SET_2_H();       // 0xD4
    void SET_2_L();       // 0xD5
    void SET_2_HL_a16();  // 0xD6
    void SET_2_A();       // 0xD7
    void SET_3_B();       // 0xD8
    void SET_3_C();       // 0xD9
    void SET_3_D();       // 0xDA
    void SET_3_E();       // 0xDB
    void SET_3_H();       // 0xDC
    void SET_3_L();       // 0xDD
    void SET_3_HL_a16();  // 0xDE
    void SET_3_A();       // 0xDF

    void SET_4_B();       // 0xE0
    void SET_4_C();       // 0xE1
    void SET_4_D();       // 0xE2
    void SET_4_E();       // 0xE3
    void SET_4_H();       // 0xE4
    void SET_4_L();       // 0xE5
    void SET_4_HL_a16();  // 0xE6
    void SET_4_A();       // 0xE7
    void SET_5_B();       // 0xE8
    void SET_5_C();       // 0xE9
    void SET_5_D();       // 0xEA
    void SET_5_E();       // 0xEB
    void SET_5_H();       // 0xEC
    void SET_5_L();       // 0xED
    void SET_5_HL_a16();  // 0xEE
    void SET_5_A();       // 0xEF

    void SET_6_B();       // 0xF0
    void SET_6_C();       // 0xF1
    void SET_6_D();       // 0xF2
    void SET_6_E();       // 0xF3
    void SET_6_H();       // 0xF4
    void SET_6_L();       // 0xF5
    void SET_6_HL_a16();  // 0xF6
    void SET_6_A();       // 0xF7
    void SET_7_B();       // 0xF8
    void SET_7_C();       // 0xF9
    void SET_7_D();       // 0xFA
    void SET_7_E();       // 0xFB
    void SET_7_H();       // 0xFC
    void SET_7_L();       // 0xFD
    void SET_7_HL_a16();  // 0xFE
    void SET_7_A();       // 0xFF
};