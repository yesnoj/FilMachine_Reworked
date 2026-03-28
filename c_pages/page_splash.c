/**
 * @file page_splash.c
 * @brief Splash screen with standard (Deep Ocean) and random generation modes.
 *
 * When splashDefault == true  (or uninitialised): shows the fixed Deep Ocean splash.
 * When splashRandom  == true:  generates random shapes each boot.
 * Otherwise:                   uses saved palette, style, complexity, seed.
 *
 * 10 palettes, 6 shape styles — matches the JSX generator.
 * LVGL 9.x — zero images, pure native objects.
 */

#include "FilMachine.h"
#include "ws_server.h"

extern struct gui_components gui;

#define SPLASH_W  LCD_H_RES
#define SPLASH_H  LCD_V_RES

static lv_obj_t *splash_scr = NULL;

/* ═══════════════════════════════════════════════
 * Simple seeded PRNG (xorshift32)
 * ═══════════════════════════════════════════════ */
static uint32_t prng_state;

static void     prng_seed(uint32_t s) { prng_state = s ? s : 1u; }
static uint32_t prng_next(void)
{
    uint32_t x = prng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    prng_state = x;
    return x;
}
/* Returns value in [lo, hi] inclusive */
static int32_t prng_range(int32_t lo, int32_t hi)
{
    if (lo >= hi) return lo;
    return lo + (int32_t)(prng_next() % (uint32_t)(hi - lo + 1));
}

/* ═══════════════════════════════════════════════
 * Color palettes — 10 themes (matching JSX generator)
 * bg_start, bg_end for gradient; accent, text for UI;
 * shapes[6] — all palette colors used for shapes.
 * ═══════════════════════════════════════════════ */
typedef struct {
    uint32_t bg_start;
    uint32_t bg_end;
    uint32_t accent;
    uint32_t text;
    uint32_t shapes[6];
} splash_palette_t;

static const splash_palette_t palettes[] = {
    /* 0 — Cyberpunk */
    { 0x0D0221, 0x0F084B, 0xE83F6F, 0xF0E6FF,
      { 0x0D0221, 0x0F084B, 0x26408B, 0xA6209E, 0xE83F6F, 0xFF5E5B } },
    /* 1 — Aurora */
    { 0x0B0C10, 0x1F2833, 0x38EF7D, 0xE0FFF4,
      { 0x0B0C10, 0x1F2833, 0x0B3D4E, 0x11998E, 0x38EF7D, 0x2DE2E6 } },
    /* 2 — Lava */
    { 0x1A0A0A, 0x3D0C02, 0xFF7A45, 0xFFF1E6,
      { 0x1A0A0A, 0x3D0C02, 0x8B1A00, 0xD4380D, 0xFF7A45, 0xFFC53D } },
    /* 3 — Deep Ocean */
    { 0x020024, 0x030637, 0x42A5F5, 0xE0F7FA,
      { 0x020024, 0x030637, 0x0D1B6F, 0x1565C0, 0x42A5F5, 0x80DEEA } },
    /* 4 — Forest */
    { 0x0A1A0A, 0x1B3A1B, 0x81C784, 0xE8F5E9,
      { 0x0A1A0A, 0x1B3A1B, 0x2D5A27, 0x4CAF50, 0x81C784, 0xA5D6A7 } },
    /* 5 — Sunset */
    { 0x1A0533, 0x4A0E4E, 0xF28A30, 0xFFF3E0,
      { 0x1A0533, 0x4A0E4E, 0x8E244D, 0xCF4647, 0xF28A30, 0xF7D060 } },
    /* 6 — Machinery */
    { 0x1A1A1A, 0x2E2E2E, 0xFFB300, 0xFFF8E1,
      { 0x1A1A1A, 0x2E2E2E, 0x4A4A4A, 0xFFB300, 0xFF8F00, 0xF57C00 } },
    /* 7 — Arctic */
    { 0xE8EAF6, 0xB0BEC5, 0x0288D1, 0x263238,
      { 0xE8EAF6, 0xB0BEC5, 0x78909C, 0x546E7A, 0x37474F, 0x263238 } },
    /* 8 — Neon */
    { 0x0A0A0A, 0x1A0A2E, 0x00FFAA, 0xE0E7FF,
      { 0x0A0A0A, 0x1A0A2E, 0x2D1B69, 0x7C3AED, 0xA78BFA, 0x00FFAA } },
    /* 9 — Pastel */
    { 0xFCE4EC, 0xF8BBD0, 0x7B1FA2, 0x37474F,
      { 0xFCE4EC, 0xF8BBD0, 0xCE93D8, 0x9FA8DA, 0x80CBC4, 0xA5D6A7 } },
};

#define PALETTE_COUNT  (sizeof(palettes) / sizeof(palettes[0]))

/* ═══════════════════════════════════════════════
 * Play button callback → enter menu
 * ═══════════════════════════════════════════════ */
static void splash_play_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    readConfigFile(FILENAME_SAVE, false);
    menu();
    refreshSettingsUI();
    /* Start WS server only after real processes are loaded — prevents
       Flutter from ever seeing the demo fallback list. */
    ws_server_start(WS_SERVER_PORT);
}

/* ═══════════════════════════════════════════════
 * Helper: create a decorative arc
 * ═══════════════════════════════════════════════ */
static void create_arc(lv_obj_t *parent, int32_t x, int32_t y,
                        int32_t size, int32_t width,
                        uint16_t start_angle, uint16_t end_angle,
                        uint32_t color, uint8_t opa)
{
    lv_obj_t *a = lv_arc_create(parent);
    lv_obj_set_size(a, size, size);
    lv_obj_set_pos(a, x, y);
    lv_arc_set_bg_start_angle(a, start_angle);
    lv_arc_set_bg_end_angle(a, end_angle);
    lv_arc_set_value(a, 0);
    lv_obj_set_style_arc_width(a, width, LV_PART_MAIN);
    lv_obj_set_style_arc_color(a, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(a, opa, LV_PART_MAIN);
    lv_obj_set_style_arc_width(a, 0, LV_PART_INDICATOR);
    lv_obj_remove_flag(a, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_style(a, NULL, LV_PART_KNOB);
}

/* ═══════════════════════════════════════════════
 * Helper: create a decorative circle
 * ═══════════════════════════════════════════════ */
static void create_circle(lv_obj_t *parent, int32_t x, int32_t y,
                           int32_t size, uint32_t color, uint8_t opa)
{
    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_remove_style_all(c);
    lv_obj_set_size(c, size, size);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_style_bg_color(c, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(c, opa, 0);
    lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(c, 0, 0);
    lv_obj_remove_flag(c, LV_OBJ_FLAG_SCROLLABLE);
}

/* ═══════════════════════════════════════════════
 * Helper: create a decorative rectangle (with optional gradient & rotation)
 * ═══════════════════════════════════════════════ */
static void create_rect(lv_obj_t *parent, int32_t x, int32_t y,
                         int32_t w, int32_t h, int32_t radius,
                         uint32_t color1, uint32_t color2,
                         lv_grad_dir_t grad_dir,
                         int16_t rotation_deg10,
                         uint8_t opa)
{
    lv_obj_t *r = lv_obj_create(parent);
    lv_obj_remove_style_all(r);
    lv_obj_set_size(r, w, h);
    lv_obj_set_pos(r, x, y);
    lv_obj_set_style_bg_color(r, lv_color_hex(color1), 0);
    lv_obj_set_style_bg_grad_color(r, lv_color_hex(color2), 0);
    lv_obj_set_style_bg_grad_dir(r, grad_dir, 0);
    lv_obj_set_style_bg_opa(r, opa, 0);
    lv_obj_set_style_radius(r, radius, 0);
    lv_obj_set_style_border_width(r, 0, 0);
    lv_obj_remove_flag(r, LV_OBJ_FLAG_SCROLLABLE);
    /* Rotation disabled — causes intermittent LVGL 9.2 rendering crash
     * on certain seed/size combos.  Re-enable once root-caused. */
    (void)rotation_deg10;
}

/* ═══════════════════════════════════════════════
 * Helper: create a decorative line
 * Static point storage: max 30 lines per splash.
 * ═══════════════════════════════════════════════ */
#define MAX_LINES 100
static lv_point_precise_t line_pts[MAX_LINES][2];
static int                line_pts_idx;

static void create_line(lv_obj_t *parent,
                         int32_t x1, int32_t y1,
                         int32_t x2, int32_t y2,
                         int32_t thickness, uint32_t color,
                         uint8_t opa, bool rounded)
{
    if (line_pts_idx >= MAX_LINES) return;
    int idx = line_pts_idx++;
    line_pts[idx][0].x = x1;
    line_pts[idx][0].y = y1;
    line_pts[idx][1].x = x2;
    line_pts[idx][1].y = y2;

    lv_obj_t *l = lv_line_create(parent);
    lv_line_set_points(l, line_pts[idx], 2);
    lv_obj_set_style_line_color(l, lv_color_hex(color), 0);
    lv_obj_set_style_line_width(l, thickness, 0);
    lv_obj_set_style_line_opa(l, opa, 0);
    lv_obj_set_style_line_rounded(l, rounded, 0);
}

/* ═══════════════════════════════════════════════
 * Title position table (matches JSX generator)
 * 0=center, 1=center-left, 2=top-left, 3=top-center,
 * 4=bottom-center, 5=bottom-left
 * ═══════════════════════════════════════════════ */
#define TITLE_POS_COUNT 6

typedef struct {
    lv_align_t align;
    int        x_num;   /* numerator for SPLASH_W fraction (or 0) */
    int        x_den;   /* denominator (or 1) */
    int        y_num;   /* numerator for SPLASH_H fraction */
    int        y_den;   /* denominator */
    bool       y_neg;   /* true = negate the y offset */
    lv_text_align_t text_align;
    lv_align_t sub_align;  /* subtitle relative to title */
} title_pos_t;

static const title_pos_t title_positions[TITLE_POS_COUNT] = {
    /* 0: center */
    { LV_ALIGN_CENTER,      0, 1,   8, 100, true,  LV_TEXT_ALIGN_CENTER, LV_ALIGN_OUT_BOTTOM_MID  },
    /* 1: center-left */
    { LV_ALIGN_LEFT_MID,    5, 100, 8, 100, true,  LV_TEXT_ALIGN_LEFT,   LV_ALIGN_OUT_BOTTOM_LEFT },
    /* 2: top-left */
    { LV_ALIGN_TOP_LEFT,    5, 100, 10, 100, false, LV_TEXT_ALIGN_LEFT,  LV_ALIGN_OUT_BOTTOM_LEFT },
    /* 3: top-center */
    { LV_ALIGN_TOP_MID,     0, 1,   8, 100, false, LV_TEXT_ALIGN_CENTER, LV_ALIGN_OUT_BOTTOM_MID  },
    /* 4: bottom-center */
    { LV_ALIGN_BOTTOM_MID,  0, 1,  18, 100, true,  LV_TEXT_ALIGN_CENTER, LV_ALIGN_OUT_BOTTOM_MID  },
    /* 5: bottom-left */
    { LV_ALIGN_BOTTOM_LEFT, 5, 100, 14, 100, true,  LV_TEXT_ALIGN_LEFT,  LV_ALIGN_OUT_BOTTOM_LEFT },
};

/* ═══════════════════════════════════════════════
 * Custom splash title fonts array
 * ═══════════════════════════════════════════════ */
static const lv_font_t * const splash_title_fonts[] = {
    &lv_font_montserrat_48,         /* 0 – default Montserrat */
    &font_air_americana_48,         /* 1 */
    &font_decaying_felt_pen_48,     /* 2 */
    &font_ds_digital_48,            /* 3 */
    &font_evanescent_48,            /* 4 */
    &font_nerdropol_lattice_48,     /* 5 */
    &font_retrolight_48,            /* 6 */
    &font_tropical_leaves_48,       /* 7 */
    &font_wishful_melisande_48,     /* 8 */
};
#define SPLASH_TITLE_FONT_COUNT  (sizeof(splash_title_fonts) / sizeof(splash_title_fonts[0]))

/* ═══════════════════════════════════════════════
 * Add title, subtitle, version & play button
 * pos_idx: 0–5 title position (see table above), <0 = default bottom-left (5)
 * title_font: NULL = use default (montserrat_48)
 * ═══════════════════════════════════════════════ */
static void add_title_and_play(lv_obj_t *scr, uint32_t text_color, uint32_t accent_color,
                                int pos_idx, const lv_font_t *title_font)
{
    if (!title_font) title_font = &lv_font_montserrat_48;
    if (pos_idx < 0 || pos_idx >= TITLE_POS_COUNT) pos_idx = 5; /* default bottom-left */
    const title_pos_t *tp = &title_positions[pos_idx];

    int x_off = (SPLASH_W * tp->x_num) / tp->x_den;
    int y_off = (SPLASH_H * tp->y_num) / tp->y_den;
    if (tp->y_neg) y_off = -y_off;

    lv_obj_t *lbl_title = lv_label_create(scr);
    lv_label_set_text(lbl_title, splashTitle_text);
    lv_obj_set_style_text_font(lbl_title, title_font, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(text_color), 0);
    lv_obj_set_style_text_align(lbl_title, tp->text_align, 0);
    lv_obj_align(lbl_title, tp->align, x_off, y_off);

    lv_obj_t *lbl_sub = lv_label_create(scr);
    lv_label_set_text(lbl_sub, splashSubtitle_text);
    lv_obj_set_style_text_font(lbl_sub, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_sub, lv_color_hex(text_color), 0);
    lv_obj_set_style_text_opa(lbl_sub, 180, 0);
    const ui_splash_screen_layout_t *ss = &ui_get_profile()->splash_screen;
    lv_obj_align_to(lbl_sub, lbl_title, tp->sub_align, ss->subtitle_x, ss->subtitle_y);

    lv_obj_t *lbl_ver = lv_label_create(scr);
    lv_label_set_text(lbl_ver, splashVersion_text);
    lv_obj_set_style_text_font(lbl_ver, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_ver, lv_color_hex(text_color), 0);
    lv_obj_set_style_text_opa(lbl_ver, 100, 0);
    lv_obj_align(lbl_ver, LV_ALIGN_BOTTOM_RIGHT, ss->version_x, ss->version_y);

    /* Play button – 2× scaled, equal distance from bottom & right
     * transform_scale 512 = 2×, but LVGL aligns on the original 48×48 box.
     * The visual extends ~24px beyond each side of the box,
     * so offset must be (desired_margin + 24) to keep icon fully visible. */
    lv_obj_t *play = lv_label_create(scr);
    lv_label_set_text(play, LV_SYMBOL_PLAY);
    lv_obj_set_style_text_font(play, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(play, lv_color_hex(accent_color), 0);
    lv_obj_set_style_text_opa(play, 200, 0);
    lv_obj_set_style_transform_scale(play, 512, 0);  /* 256 = 1×, 512 = 2× */
    lv_obj_align(play, LV_ALIGN_BOTTOM_RIGHT, ss->play_icon_x, ss->play_icon_y);

    lv_obj_t *play_btn = lv_button_create(scr);
    lv_obj_remove_style_all(play_btn);
    lv_obj_set_size(play_btn, ss->play_hit_w, ss->play_hit_h);
    lv_obj_align(play_btn, LV_ALIGN_BOTTOM_RIGHT, ss->play_hit_x, ss->play_hit_y);
    lv_obj_set_style_bg_opa(play_btn, LV_OPA_TRANSP, 0);
    lv_obj_add_flag(play_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(play_btn, splash_play_event_cb, LV_EVENT_CLICKED, NULL);
}

/* ═══════════════════════════════════════════════
 * STANDARD splash (Deep Ocean | Radial)
 * ═══════════════════════════════════════════════ */
static void create_standard_splash(lv_obj_t *scr)
{
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x020024), 0);
    lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0x030637), 0);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    create_arc(scr, 101, -6, 326, 76, 219, 349, 0x80DEEA, 46);
    create_circle(scr, 654, 254, 264, 0x42A5F5, 86);
    create_arc(scr, -40, 29, 140, 11, 68, 387, 0x42A5F5, 64);
    create_circle(scr, 275, 141, 316, 0x42A5F5, 110);
    create_circle(scr, -33, -78, 204, 0x0D1B6F, 52);
    create_arc(scr, 624, 71, 174, 22, 147, 448, 0x020024, 130);
    create_arc(scr, -87, 122, 326, 57, 22, 296, 0x80DEEA, 73);
    create_arc(scr, 288, 159, 78, 20, 67, 194, 0x42A5F5, 155);
    create_circle(scr, 267, 220, 84, 0x42A5F5, 106);
    create_arc(scr, 198, 183, 282, 50, 258, 477, 0x80DEEA, 115);
    create_arc(scr, 479, -35, 322, 98, 307, 648, 0x0D1B6F, 72);
    create_arc(scr, 85, 131, 312, 6, 64, 264, 0x80DEEA, 145);
    create_arc(scr, -25, 102, 226, 33, 139, 380, 0x020024, 126);
    create_arc(scr, 650, 70, 236, 28, 104, 236, 0x020024, 128);
    create_arc(scr, 193, 105, 92, 22, 39, 339, 0x0D1B6F, 83);
    create_arc(scr, 147, 241, 170, 40, 222, 372, 0x1565C0, 153);
    create_circle(scr, 95, 11, 56, 0x42A5F5, 101);
    create_arc(scr, 516, -83, 262, 54, 5, 231, 0x020024, 83);
    create_circle(scr, 325, -10, 284, 0x0D1B6F, 98);
    create_arc(scr, -59, 44, 330, 44, 75, 283, 0x030637, 59);
    create_circle(scr, 179, 79, 212, 0x020024, 83);
    create_arc(scr, 319, 359, 74, 11, 165, 268, 0x80DEEA, 101);
    create_circle(scr, 203, 212, 318, 0x42A5F5, 30);
    create_circle(scr, 688, 355, 216, 0x42A5F5, 118);
    create_circle(scr, 742, 68, 84, 0x1565C0, 58);
    create_arc(scr, 33, 22, 294, 68, 118, 375, 0x80DEEA, 111);
    create_circle(scr, 682, 84, 68, 0x1565C0, 89);
    create_arc(scr, 683, 248, 158, 18, 53, 244, 0x020024, 64);
    create_arc(scr, 511, 130, 244, 8, 65, 229, 0x030637, 130);
    create_arc(scr, 400, 252, 66, 18, 76, 352, 0x80DEEA, 129);

    add_title_and_play(scr, 0xE0F7FA, 0x42A5F5, 5, NULL); /* default: bottom-left, default font */
}

/* ═══════════════════════════════════════════════
 * Shape generators for each style
 *
 * Style indices:
 *   0 = Overlapping Rects
 *   1 = Circles & Arcs
 *   2 = Mixed Shapes
 *   3 = Diagonal Bands
 *   4 = Grid Blocks
 *   5 = Radial Arcs
 * ═══════════════════════════════════════════════ */
static void gen_rect_shape(lv_obj_t *scr, const splash_palette_t *pal)
{
    int32_t minD = SPLASH_W < SPLASH_H ? SPLASH_W : SPLASH_H;
    int32_t sw = prng_range(minD / 12, SPLASH_W / 2);
    int32_t sh = prng_range(minD / 12, SPLASH_H / 2);
    int32_t sx = prng_range(-(SPLASH_W / 7), SPLASH_W - SPLASH_W / 7);
    int32_t sy = prng_range(-(SPLASH_H / 7), SPLASH_H - SPLASH_H / 7);
    int16_t rot = (int16_t)prng_range(-300, 300);   /* 0.1° units */
    uint8_t opa = (uint8_t)prng_range(30, 150);
    int32_t rad = (prng_next() & 1) ? prng_range(2, minD / 12) : 0;
    uint32_t c1 = pal->shapes[prng_next() % 6];
    uint32_t c2 = (prng_next() % 3) ? pal->shapes[prng_next() % 6] : c1;
    lv_grad_dir_t gd = (prng_next() & 1) ? LV_GRAD_DIR_VER : LV_GRAD_DIR_HOR;
    create_rect(scr, sx, sy, sw, sh, rad, c1, c2, gd, rot, opa);
}

static void gen_arc_shape(lv_obj_t *scr, const splash_palette_t *pal)
{
    int32_t minD = SPLASH_W < SPLASH_H ? SPLASH_W : SPLASH_H;
    uint32_t color = pal->shapes[prng_next() % 6];
    uint8_t opa = (uint8_t)prng_range(30, 160);
    bool filled = (prng_next() % 5 >= 3);  /* ~40% chance filled circle */

    if (filled) {
        int32_t r = prng_range(minD / 20, minD / 3);
        int32_t cx = prng_range(-r / 2, SPLASH_W - r / 2);
        int32_t cy = prng_range(-r / 2, SPLASH_H - r / 2);
        create_circle(scr, cx - r, cy - r, r * 2, color, opa);
    } else {
        int32_t r = prng_range(minD / 20, minD / 3);
        int32_t cx = prng_range(0, SPLASH_W);
        int32_t cy = prng_range(0, SPLASH_H);
        int32_t th = prng_range(3, r * 6 / 10);
        if (th < 3) th = 3;
        uint16_t sa = (uint16_t)prng_range(0, 359);
        uint16_t span = (uint16_t)prng_range(60, 300);
        create_arc(scr, cx - r, cy - r, r * 2, th, sa, sa + span, color, opa);
    }
}

static void gen_line_shape(lv_obj_t *scr, const splash_palette_t *pal)
{
    int32_t maxD = SPLASH_W > SPLASH_H ? SPLASH_W : SPLASH_H;
    int32_t minD = SPLASH_W < SPLASH_H ? SPLASH_W : SPLASH_H;
    int32_t x1 = prng_range(-(SPLASH_W / 10), SPLASH_W + SPLASH_W / 10);
    int32_t y1 = prng_range(-(SPLASH_H / 10), SPLASH_H + SPLASH_H / 10);
    /* Simple angle approximation — random dx/dy offset */
    int32_t len = prng_range(40, maxD * 8 / 10);
    int32_t dx = prng_range(-len, len);
    int32_t dy_sign = (prng_next() & 1) ? 1 : -1;
    /* Ensure line has reasonable length */
    int32_t dy_mag = len - (dx < 0 ? -dx : dx);
    if (dy_mag < 0) dy_mag = 0;
    int32_t x2 = x1 + dx;
    int32_t y2 = y1 + dy_sign * dy_mag;
    int32_t th = prng_range(2, minD / 16);
    uint32_t color = pal->shapes[prng_next() % 6];
    uint8_t opa = (uint8_t)prng_range(40, 170);
    bool rounded = (prng_next() & 1);
    create_line(scr, x1, y1, x2, y2, th, color, opa, rounded);
}

/* ═══════════════════════════════════════════════
 * RANDOM splash — shapes only (no title/play)
 * Used by both the real splash and the popup preview.
 * ═══════════════════════════════════════════════ */
static void create_random_shapes(lv_obj_t *scr, uint32_t seed,
                                  uint8_t palette_idx,
                                  uint8_t shape_style,
                                  uint8_t complexity)
{
    if (palette_idx >= PALETTE_COUNT) palette_idx = 0;
    const splash_palette_t *pal = &palettes[palette_idx];

    /* Background gradient */
    lv_obj_set_style_bg_color(scr, lv_color_hex(pal->bg_start), 0);
    lv_obj_set_style_bg_grad_color(scr, lv_color_hex(pal->bg_end), 0);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    /* complexity IS the shape count (2–100) */
    int num_shapes = complexity;
    if (num_shapes < 2) num_shapes = 2;
    if (num_shapes > 100) num_shapes = 100;

    /* Reset line points index */
    line_pts_idx = 0;

    prng_seed(seed);

    for (int i = 0; i < num_shapes; i++) {
        switch (shape_style) {
            case 0: /* Overlapping Rects */
                gen_rect_shape(scr, pal);
                break;
            case 1: /* Circles & Arcs */
                gen_arc_shape(scr, pal);
                break;
            case 2: /* Mixed Shapes */
            {
                uint32_t r = prng_next() % 10;
                if (r < 4)      gen_rect_shape(scr, pal);
                else if (r < 7) gen_arc_shape(scr, pal);
                else             gen_line_shape(scr, pal);
                break;
            }
            case 3: /* Diagonal Bands */
                if (prng_next() % 10 < 7)
                    gen_line_shape(scr, pal);
                else
                    gen_rect_shape(scr, pal);
                break;
            case 4: /* Grid Blocks — rects with no rotation */
            {
                int32_t minD = SPLASH_W < SPLASH_H ? SPLASH_W : SPLASH_H;
                int32_t sw = prng_range(minD / 12, SPLASH_W / 2);
                int32_t sh = prng_range(minD / 12, SPLASH_H / 2);
                int32_t sx = prng_range(-(SPLASH_W / 7), SPLASH_W - SPLASH_W / 7);
                int32_t sy = prng_range(-(SPLASH_H / 7), SPLASH_H - SPLASH_H / 7);
                uint8_t opa = (uint8_t)prng_range(30, 150);
                int32_t rad = (prng_next() & 1) ? prng_range(2, minD / 12) : 0;
                uint32_t c1 = pal->shapes[prng_next() % 6];
                uint32_t c2 = (prng_next() % 3) ? pal->shapes[prng_next() % 6] : c1;
                lv_grad_dir_t gd = (prng_next() & 1) ? LV_GRAD_DIR_VER : LV_GRAD_DIR_HOR;
                create_rect(scr, sx, sy, sw, sh, rad, c1, c2, gd, 0, opa);
                break;
            }
            case 5: /* Radial Arcs */
                gen_arc_shape(scr, pal);
                break;
            default:
                gen_arc_shape(scr, pal);
                break;
        }
    }
}

/* Full random splash: shapes + title + play button */
static void create_random_splash(lv_obj_t *scr, uint32_t seed,
                                  uint8_t palette_idx,
                                  uint8_t shape_style,
                                  uint8_t complexity)
{
    if (palette_idx >= PALETTE_COUNT) palette_idx = 0;
    const splash_palette_t *pal = &palettes[palette_idx];
    create_random_shapes(scr, seed, palette_idx, shape_style, complexity);
    /* Random title position and font using PRNG (state continues from shapes) */
    int title_pos = (int)(prng_next() % TITLE_POS_COUNT);
    uint8_t font_idx = (uint8_t)(prng_next() % SPLASH_TITLE_FONT_COUNT);
    const lv_font_t *title_font = splash_title_fonts[font_idx];
    add_title_and_play(scr, pal->text, pal->accent, title_pos, title_font);
}

/* ═══════════════════════════════════════════════
 * Public API
 * ═══════════════════════════════════════════════ */
lv_obj_t * splash_screen_create(void)
{
    splash_scr = lv_obj_create(NULL);
    lv_obj_set_size(splash_scr, SPLASH_W, SPLASH_H);
    lv_obj_remove_flag(splash_scr, LV_OBJ_FLAG_SCROLLABLE);

    if (gui.page.settings.settingsParams.splashDefault ||
        (!gui.page.settings.settingsParams.splashRandom &&
         gui.page.settings.settingsParams.splashComplexity == 0)) {
        /* Default mode or uninitialised config → standard Deep Ocean */
        create_standard_splash(splash_scr);
    } else if (gui.page.settings.settingsParams.splashRandom) {
        /* Random mode → generate fresh random params from LVGL tick */
        uint32_t tick = lv_tick_get();
        uint32_t rng  = tick * 1103515245u + 12345u;
        uint8_t  rpal = (uint8_t)(rng % PALETTE_COUNT);
        rng = rng * 1103515245u + 12345u;
        uint8_t  rsty = (uint8_t)(rng % 6);
        rng = rng * 1103515245u + 12345u;
        uint8_t  rcmx = 2 + (uint8_t)(rng % 29);  /* 2–30 */
        rng = rng * 1103515245u + 12345u;
        LV_LOG_USER("SPLASH RANDOM: tick=%"PRIu32" seed=%"PRIu32" pal=%d style=%d cmx=%d",
                     tick, rng, rpal, rsty, rcmx);
        line_pts_idx = 0;
        create_random_splash(splash_scr, rng, rpal, rsty, rcmx);
        LV_LOG_USER("SPLASH RANDOM: creation OK");
    } else {
        /* Custom mode → use saved params (palette, style, complexity, seed) */
        uint32_t seed = gui.page.settings.settingsParams.splashSeed;
        if (seed == 0) seed = lv_tick_get() * 1103515245u + 12345u;
        LV_LOG_USER("SPLASH CUSTOM: seed=%"PRIu32" pal=%d style=%d cmx=%d",
                     seed,
                     gui.page.settings.settingsParams.splashPalette,
                     gui.page.settings.settingsParams.splashShapeStyle,
                     gui.page.settings.settingsParams.splashComplexity);
        line_pts_idx = 0;
        create_random_splash(
            splash_scr,
            seed,
            gui.page.settings.settingsParams.splashPalette,
            gui.page.settings.settingsParams.splashShapeStyle,
            gui.page.settings.settingsParams.splashComplexity
        );
        LV_LOG_USER("SPLASH CUSTOM: creation OK");
    }

    return splash_scr;
}

void splash_screen_delete(void)
{
    if (splash_scr) {
        lv_obj_delete(splash_scr);
        splash_scr = NULL;
    }
}

/* ═══════════════════════════════════════════════
 * Public preview API for the splash popup
 *
 * Generates shapes inside `parent` using the same
 * engine as the full splash screen.  Caller should
 * create an lv_obj container, pass it here, then
 * overlay a semi-transparent rect on top for
 * readability.
 *
 * To regenerate: delete all children of `parent`,
 * then call again.
 * ═══════════════════════════════════════════════ */
void splash_preview_generate(lv_obj_t *parent, uint32_t seed,
                              uint8_t palette_idx, uint8_t shape_style,
                              uint8_t complexity)
{
    /* Cap complexity for preview to avoid crash with too many objects */
    if (complexity > 30) complexity = 30;
    line_pts_idx = 0;
    create_random_shapes(parent, seed, palette_idx, shape_style, complexity);
}

void splash_standard_preview_generate(lv_obj_t *parent)
{
    create_standard_splash(parent);
}
