#include "ppu.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "../emulator.h"

Ppu::Ppu(Mmu& m) : mmu(m) {
    scanline_counter    = 0;
    window_line_counter = 0;
    sprite_count        = 0;
    prev_signal         = false;
    mode_3_extra_cycles = 0;

    frame_buffer.fill(WHITE);

    for (size_t i = 0; i < 4; ++i) {
        bg_palette.colors[i]   = default_colors[i];
        obj_palette0.colors[i] = default_colors[i];
        obj_palette1.colors[i] = default_colors[i];
    }
}

void Ppu::save_state(PpuState& state) const {
    state.scanline_counter    = scanline_counter;
    state.window_line_counter = window_line_counter;
    state.prev_signal         = prev_signal;
}

void Ppu::load_state(const PpuState& state) {
    scanline_counter    = state.scanline_counter;
    window_line_counter = state.window_line_counter;
    prev_signal         = state.prev_signal;

    update_palettes();
}

const std::array<Color, Ppu::SCREEN_WIDTH * Ppu::SCREEN_HEIGHT>& Ppu::get_frame_buffer() const { return frame_buffer; }

void Ppu::tick(u8 cycles) {
    bool lcd_enabled = is_lcd_enabled();

    if (lcd_enabled) {
        scanline_counter += cycles;

        switch (get_mode()) {
            case Mode::OamScan:
                if (scanline_counter >= OAM_SCAN_CYCLES) {
                    oam_scan();
                    mode_3_extra_cycles = sprite_count * 6;
                    set_mode(Mode::Drawing);
                }
                break;
            case Mode::Drawing: {
                int total_mode_3_time = VRAM_READ_CYCLES + (mmu.scx() % 8) + mode_3_extra_cycles;
                if (scanline_counter >= OAM_SCAN_CYCLES + total_mode_3_time) {
                    set_mode(Mode::HBlank);
                    render_scanline();
                }
            } break;
            case Mode::HBlank:
                if (scanline_counter >= CYCLES_PER_SCANLINE) {
                    scanline_counter = 0;
                    mmu.ly()++;

                    if (mmu.ly() >= VISIBLE_SCANLINES) {
                        set_mode(Mode::VBlank);
                        mmu.request_interrupt(InterruptType::VBlank);
                    } else {
                        set_mode(Mode::OamScan);
                    }
                }
                break;
            case Mode::VBlank:
                if (scanline_counter >= CYCLES_PER_SCANLINE) {
                    scanline_counter = 0;
                    mmu.ly()++;

                    if (mmu.ly() >= TOTAL_SCANLINES) {
                        mmu.ly()            = 0;
                        window_line_counter = 0;
                        set_mode(Mode::OamScan);
                    }
                }
                break;
            default:
                break;
        }
    } else {
        scanline_counter    = 0;
        window_line_counter = 0;
        mmu.ly()            = 0;
        set_mode(Mode::HBlank);
    }

    check_ly_coincidence();
    update_stat_interrupt();
}

void Ppu::render_scanline() {
    render_background_line();
    render_window_line();
    render_sprite_line();
}

void Ppu::render_background_line() {
    int y             = mmu.ly();
    int canvas_offset = y * Ppu::SCREEN_WIDTH;

    if (!is_bit(0, mmu.lcdc())) {
        for (int x = 0; x < Ppu::SCREEN_WIDTH; x++) {
            frame_buffer[canvas_offset + x] = WHITE;
        }
        return;
    }

    u8 bg_y = (y + mmu.scy()) & 0xFF;

    u8 tile_row = bg_y & 7;

    u16 tile_map_base = get_tile_map();

    for (int x = 0; x < Ppu::SCREEN_WIDTH; x++) {
        u8 bg_x = (x + mmu.scx()) & 0xFF;

        u16 tile_x = bg_x / 8;
        u16 tile_y = bg_y / 8;

        u16 tile_map_addr = tile_map_base + (tile_y * 32) + tile_x;
        u8  tile_index    = mmu.ppu_read_u8(tile_map_addr);

        u16 tile_data_addr = get_tile_data_address(tile_index);

        u16 tile_row_addr = tile_data_addr + (tile_row * 2);
        u8  byte1         = mmu.ppu_read_u8(tile_row_addr);
        u8  byte2         = mmu.ppu_read_u8(tile_row_addr + 1);

        u8 tile_col = bg_x & 7;

        int color_bit = 7 - tile_col;

        u8 color_id = (((byte2 >> color_bit) & 1) << 1) | ((byte1 >> color_bit) & 1);

        DmgColor c = bg_palette.colors[color_id];

        frame_buffer[canvas_offset + x] = {c.r, c.g, c.b, 255};
    }
}

void Ppu::render_sprite_line() {
    if (!is_bit(1, mmu.lcdc())) {
        return;
    }

    // Sorting sprites in order of priority, fixes the mole issue on acid test.
    std::sort(sprite_buffer.begin(), sprite_buffer.begin() + sprite_count, [](const Sprite& a, const Sprite& b) {
        if (a.x != b.x) {
            return a.x < b.x;
        }
        return a.oam_index < b.oam_index;
    });

    int y_size   = sprite_size();
    int scanline = mmu.ly();

    for (int i = sprite_count - 1; i >= 0; --i) {
        const Sprite& s = sprite_buffer[i];

        if (s.x == 0 || s.x >= 168) {
            continue;
        }

        int pixel_y = scanline - s.y;
        if (is_y_flipped(s.attributes)) {
            pixel_y = (y_size - 1) - pixel_y;
        }

        // 8x16 sprites special case
        u8 tile_index = (y_size == 16) ? (s.tile_index & 0xFE) : s.tile_index;

        u16 tile_addr = 0x8000 + (tile_index * 16);
        u8  byte1     = mmu.ppu_read_u8(tile_addr + (pixel_y * 2));
        u8  byte2     = mmu.ppu_read_u8(tile_addr + (pixel_y * 2) + 1);

        Palette& pal = is_bit(4, s.attributes) ? obj_palette1 : obj_palette0;

        for (int pixel_x = 0; pixel_x < 8; ++pixel_x) {
            int screen_x = (s.x - 8) + pixel_x;

            if (screen_x < 0 || screen_x >= SCREEN_WIDTH) {
                continue;
            }

            int color_bit = is_x_flipped(s.attributes) ? pixel_x : (7 - pixel_x);

            u8 color_id = (((byte2 >> color_bit) & 1) << 1) | ((byte1 >> color_bit) & 1);

            if (color_id == 0) {
                continue;
            }

            if (is_bit(7, s.attributes)) {
                Color    bgColor    = frame_buffer[scanline * SCREEN_WIDTH + screen_x];
                DmgColor bg_color_0 = bg_palette.colors[0];
                if (bgColor.r != bg_color_0.r || bgColor.g != bg_color_0.g || bgColor.b != bg_color_0.b) {
                    continue;
                }
            }

            DmgColor c = pal.colors[color_id];

            frame_buffer[scanline * SCREEN_WIDTH + screen_x] = {c.r, c.g, c.b, 255};
        }
    }
}

void Ppu::render_window_line() {
    if (!is_window()) {
        return;
    }

    int wx            = mmu.wx() - 7;
    int y             = mmu.ly();
    int canvas_offset = y * Ppu::SCREEN_WIDTH;

    u16 tile_map_base = get_window_tile_map();

    int window_y = window_line_counter;

    for (int x = 0; x < Ppu::SCREEN_WIDTH; x++) {
        if (x < wx) {
            continue;  // Don't need offscreen pixels
        }

        int window_x = x - wx;

        u16 tile_x = window_x / 8;
        u16 tile_y = window_y / 8;

        u16 tile_map_addr = tile_map_base + (tile_y * 32) + tile_x;
        u8  tile_index    = mmu.ppu_read_u8(tile_map_addr);

        u16 tile_data_addr = get_tile_data_address(tile_index);

        u8  tile_row      = window_y % 8;
        u16 tile_row_addr = tile_data_addr + (tile_row * 2);
        u8  byte1         = mmu.ppu_read_u8(tile_row_addr);
        u8  byte2         = mmu.ppu_read_u8(tile_row_addr + 1);

        u8  tile_col  = window_x % 8;
        int color_bit = 7 - tile_col;
        u8  color_id  = (((byte2 >> color_bit) & 1) << 1) | ((byte1 >> color_bit) & 1);

        DmgColor c                      = bg_palette.colors[color_id];
        frame_buffer[canvas_offset + x] = {c.r, c.g, c.b, 255};
    }

    window_line_counter++;
}

Mode Ppu::get_mode() { return static_cast<Mode>(mmu.stat() & 0x3); }

void Ppu::set_mode(Mode m) { mmu.stat() = static_cast<u8>(m) | mmu.stat() & 0b11111100; }

void Ppu::update_stat_interrupt() {
    bool lyc_interrupt_enabled    = is_bit(6, mmu.stat());
    bool mode_2_interrupt_enabled = is_bit(5, mmu.stat());
    bool mode_1_interrupt_enabled = is_bit(4, mmu.stat());
    bool mode_0_interrupt_enabled = is_bit(3, mmu.stat());

    bool lyc_cond    = lyc_interrupt_enabled && is_bit(2, mmu.stat());
    bool mode_2_cond = mode_2_interrupt_enabled && (get_mode() == Mode::OamScan);
    bool mode_1_cond = mode_1_interrupt_enabled && (get_mode() == Mode::VBlank);
    bool mode_0_cond = mode_0_interrupt_enabled && (get_mode() == Mode::HBlank);

    bool signal = lyc_cond || mode_2_cond || mode_1_cond || mode_0_cond;
    if (signal && !prev_signal) {
        mmu.request_interrupt(InterruptType::Lcd);
    }

    prev_signal = signal;
}

void Ppu::check_ly_coincidence() {
    if (mmu.ly() == mmu.lyc()) {
        mmu.stat() = set_bit(2, mmu.stat());
    } else {
        mmu.stat() = clear_bit(2, mmu.stat());
    }
}

u16 Ppu::get_tile_data_address(u8 tile_index) {
    if (is_bit(4, mmu.lcdc())) {
        return 0x8000 + (tile_index * 16);
    } else {
        return 0x9000 + (static_cast<i8>(tile_index) * 16);
    }
}

u16 Ppu::get_tile_map() { return is_bit(3, mmu.lcdc()) ? 0x9C00 : 0x9800; }

u16 Ppu::get_window_tile_map() { return is_bit(6, mmu.lcdc()) ? 0x9C00 : 0x9800; }

void Ppu::oam_scan() {
    sprite_buffer.fill(Sprite{});
    int y_size   = sprite_size();
    int scanline = mmu.ly();

    sprite_count = 0;
    for (int i = 0; i < 40 && sprite_count < MAX_SPRITES_PER_LINE; ++i) {
        u16 sprite_addr = 0xFE00 + (i * 4);
        u8  sprite_y    = (u8)(mmu.ppu_read_u8(sprite_addr) - 16);

        if (scanline >= sprite_y && scanline < (sprite_y + y_size)) {
            Sprite& s    = sprite_buffer[sprite_count++];
            s.y          = sprite_y;
            s.x          = mmu.ppu_read_u8(sprite_addr + 1);
            s.tile_index = mmu.ppu_read_u8(sprite_addr + 2);
            s.attributes = mmu.ppu_read_u8(sprite_addr + 3);
            s.oam_index  = i;
        }
    }
}

void Ppu::decode_palette(u8 palette_data, Palette& palette) {
    palette.colors[0] = default_colors[(palette_data >> 0) & 0x3];
    palette.colors[1] = default_colors[(palette_data >> 2) & 0x3];
    palette.colors[2] = default_colors[(palette_data >> 4) & 0x3];
    palette.colors[3] = default_colors[(palette_data >> 6) & 0x3];
}

int Ppu::sprite_size() { return is_bit(2, mmu.lcdc()) ? 16 : 8; }

bool Ppu::is_x_flipped(u8 attr) { return is_bit(5, attr); }

bool Ppu::is_y_flipped(u8 attr) { return is_bit(6, attr); }

bool Ppu::is_window() {
    bool window_enable = is_bit(5, mmu.lcdc());
    bool wy_cond       = mmu.wy() <= mmu.ly();
    bool wx_cond       = mmu.wx() < (Ppu::SCREEN_WIDTH + 7);

    return window_enable && wy_cond && wx_cond;
}

bool Ppu::is_lcd_enabled() { return is_bit(7, mmu.lcdc()); }

bool Ppu::is_bit(int bit, int val) { return (u8)((val >> bit) & 0x1) == 1; }

u8 Ppu::set_bit(u8 bit, u8 val) { return val |= (u8)(1 << bit); }

u8 Ppu::clear_bit(u8 bit, u8 val) { return val &= (u8) ~(1 << bit); }