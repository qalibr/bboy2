#include "joypad.h"

Joypad::Joypad(Mmu& m) : mmu(m) {
    // Initializing no buttons selected
    mmu.p1() = 0x30;
}

u8 Joypad::get_joyp_register() {
    u8 selection = mmu.p1() & 0x30;

    u8 result = (selection | 0xCF);

    if ((selection & 0x20) == 0) {
        if (a) result &= 0b11111110;
        if (b) result &= 0b11111101;
        if (select) result &= 0b11111011;
        if (start) result &= 0b11110111;
    }

    if ((selection & 0x10) == 0) {
        if (right) result &= 0b11111110;
        if (left) result &= 0b11111101;
        if (up) result &= 0b11111011;
        if (down) result &= 0b11110111;
    }

    return result;
}

void Joypad::set_joyp_register(u8 val) { mmu.p1() = (mmu.p1() & 0xCF) | (val & 0x30); }

// Polling the keyboard and letting the CPU know via interrupt whenever the input state
// has changed. The CPU will then check 0xFF00 for the new input state.
void Joypad::handle_input() {
    bool prev_up = up, prev_down = down, prev_left = left, prev_right = right;
    bool prev_a = a, prev_b = b, prev_start = start, prev_select = select;

    up    = IsKeyDown(KeyboardKey::KEY_W);
    down  = IsKeyDown(KeyboardKey::KEY_S);
    left  = IsKeyDown(KeyboardKey::KEY_A);
    right = IsKeyDown(KeyboardKey::KEY_D);

    a      = IsKeyDown(KeyboardKey::KEY_E);
    b      = IsKeyDown(KeyboardKey::KEY_R);
    start  = IsKeyDown(KeyboardKey::KEY_F);
    select = IsKeyDown(KeyboardKey::KEY_Z);

    // clang-format off
    bool button_pressed = 
        (up && !prev_up) || 
        (down && !prev_down) || 
        (left && !prev_left) || 
        (right && !prev_right) ||
        (a && !prev_a) || 
        (b && !prev_b) || 
        (start && !prev_start) || 
        (select && !prev_select);
    // clang-format on

    if (button_pressed) {
        mmu.request_interrupt(InterruptType::Joypad);
    }
}