#pragma once

struct Registers {
    union {
        struct {
            union {
                struct {
                    u8 unused : 4;
                    u8 c : 1;
                    u8 h : 1;
                    u8 n : 1;
                    u8 z : 1;
                };
                u8 F;
            };
            u8 A;
        };
        u16 AF;
    };

    union {
        struct {
            u8 C;
            u8 B;
        };
        u16 BC;
    };

    union {
        struct {
            u8 E;
            u8 D;
        };
        u16 DE;
    };

    union {
        struct {
            u8 L;
            u8 H;
        };
        u16 HL;
    };

    u16 SP;
    u16 PC;
};