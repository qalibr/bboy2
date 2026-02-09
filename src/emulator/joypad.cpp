#include "joypad.h"

#include <tracy/Tracy.hpp>

Joypad::Joypad(Mmu& m) : mmu(m) {
    mmu.p1()     = 0x30;
    up           = false;
    down         = false;
    left         = false;
    right        = false;
    a            = false;
    b            = false;
    start        = false;
    select       = false;
    uncapped_fps = false;
    display_fps  = true;
    trigger_save = false;
    trigger_load = false;
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

bool Joypad::is_fps_uncapped() const { return uncapped_fps; }

bool Joypad::should_display_fps() const { return display_fps; }

bool Joypad::should_trigger_save() const { return trigger_save; }

bool Joypad::should_trigger_load() const { return trigger_load; }

void Joypad::action_performed() {
    trigger_save = false;
    trigger_load = false;
}

// Polling the keyboard and letting the CPU know via interrupt whenever the input state
// has changed. The CPU will then check 0xFF00 for the new input state.
void Joypad::handle_input() {
    ZoneScoped;

    bool prev_up = up, prev_down = down, prev_left = left, prev_right = right;
    bool prev_a = a, prev_b = b, prev_start = start, prev_select = select;

    bool is_ctrl_down = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

    if (is_ctrl_down && IsKeyPressed(KeyboardKey::KEY_T)) {
        trigger_save = true;
    }
    if (is_ctrl_down && IsKeyPressed(KeyboardKey::KEY_L)) {
        trigger_load = true;
    }

    up    = IsKeyDown(KeyboardKey::KEY_W);
    down  = IsKeyDown(KeyboardKey::KEY_S);
    left  = IsKeyDown(KeyboardKey::KEY_A);
    right = IsKeyDown(KeyboardKey::KEY_D);

    a      = IsKeyDown(KeyboardKey::KEY_E);
    b      = IsKeyDown(KeyboardKey::KEY_R);
    start  = IsKeyDown(KeyboardKey::KEY_F);
    select = IsKeyDown(KeyboardKey::KEY_Z);

    uncapped_fps = IsKeyDown(KeyboardKey::KEY_SPACE);

    if (IsKeyPressed(KeyboardKey::KEY_I)) {
        display_fps = !display_fps;
    }

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