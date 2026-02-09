#pragma once

#include <array>

#include "../mmu/mmu.h"
#include "raylib.h"

struct PpuState;

enum class Mode {
    HBlank  = 0,
    VBlank  = 1,
    OamScan = 2,
    Drawing = 3,
};

struct Sprite {
    u8 y;
    u8 x;
    u8 tile_index;
    u8 attributes;
    u8 oam_index;
};

struct DmgColor {
    u8 r, g, b;
};

struct Palette {
    std::array<DmgColor, 4> colors;
};

class Ppu {
   public:
    Mmu& mmu;

    Ppu(Mmu& m);

    static constexpr int SCREEN_WIDTH  = 160;
    static constexpr int SCREEN_HEIGHT = 144;

    static constexpr int OAM_SCAN_CYCLES     = 80;
    static constexpr int VRAM_READ_CYCLES    = 172;
    static constexpr int HBLANK_CYCLES       = 204;
    static constexpr int CYCLES_PER_SCANLINE = OAM_SCAN_CYCLES + VRAM_READ_CYCLES + HBLANK_CYCLES;

    static constexpr int VISIBLE_SCANLINES = 144;
    static constexpr int VBLANK_SCANLINES  = 10;
    static constexpr int TOTAL_SCANLINES   = VISIBLE_SCANLINES + VBLANK_SCANLINES;
    static constexpr int CYCLES_PER_FRAME  = CYCLES_PER_SCANLINE * TOTAL_SCANLINES;

    void tick(u8 cycles);

    void save_state(PpuState& state) const;
    void load_state(const PpuState& state);

    const std::array<Color, SCREEN_WIDTH * SCREEN_HEIGHT>& get_frame_buffer() const;

    inline void update_palettes() {
        decode_palette(mmu.bgp(), bg_palette);
        decode_palette(mmu.obp0(), obj_palette0);
        decode_palette(mmu.obp1(), obj_palette1);
    }

    Mode get_mode();

   private:
    static constexpr int MAX_SPRITES_PER_LINE = 10;

    std::array<Color, SCREEN_WIDTH * SCREEN_HEIGHT> frame_buffer;
    std::array<Sprite, MAX_SPRITES_PER_LINE>        sprite_buffer;

    int     sprite_count;
    Palette bg_palette;
    Palette obj_palette0;
    Palette obj_palette1;
    int     scanline_counter;
    int     window_line_counter;
    bool    prev_signal;
    int     mode_3_extra_cycles;

    static inline const std::array<DmgColor, 4> default_colors = {
        {{0xFF, 0xFF, 0xFF},  // White
         {0xD3, 0xD3, 0xD3},  // Light Gray
         {0x88, 0x88, 0x88},  // Dark Gray
         {0x00, 0x00, 0x00}}  // Black
    };

    // =============================================================
    //  Render
    // =============================================================
    void render_scanline();
    void render_background_line();
    void render_sprite_line();
    void render_window_line();

    // =============================================================
    //  State Machine
    // =============================================================
    void set_mode(Mode m);
    void update_stat_interrupt();
    void check_ly_coincidence();

    // =============================================================
    //  Assets
    // =============================================================
    u16  get_tile_data_address(u8 tile_index);
    u16  get_tile_map();
    u16  get_window_tile_map();
    void oam_scan();
    void decode_palette(u8 palette_data, Palette& palette);
    int  sprite_size();

    // =============================================================
    //  Helpers
    // =============================================================
    bool        is_x_flipped(u8 attr);
    bool        is_y_flipped(u8 attr);
    bool        is_window();
    bool        is_lcd_enabled();
    static bool is_bit(int bit, int val);
    static u8   set_bit(u8 bit, u8 val);
    static u8   clear_bit(u8 bit, u8 val);
};