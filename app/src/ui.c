#include "ui.h"
#include "display.h"

#include <stdint.h>

/*
 * 5x7 pixel font bitmaps.
 * Each row is a uint8_t: bit4 = col0 (leftmost), bit0 = col4 (rightmost).
 * Index: 0-9 = digits '0'-'9', 10 = 'C', 11 = '%', 15 = '-'
 */
static const uint8_t font_5x7[][7] = {
    /* '0' */ { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E },
    /* '1' */ { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E },
    /* '2' */ { 0x0E, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1F },
    /* '3' */ { 0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E },
    /* '4' */ { 0x11, 0x11, 0x11, 0x1F, 0x01, 0x01, 0x01 },
    /* '5' */ { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E },
    /* '6' */ { 0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E },
    /* '7' */ { 0x1F, 0x01, 0x02, 0x04, 0x04, 0x04, 0x04 },
    /* '8' */ { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E },
    /* '9' */ { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E },
    /* 'C' */ { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E },
    /* '%' */ { 0x19, 0x1A, 0x02, 0x04, 0x08, 0x13, 0x03 },
    /* 'D' */ { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E },
    /* 'P' */ { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 },
    /* ':' */ { 0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00 },
    /* '-' */ { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 },
};

static int g_temp;
static int g_humidity;

void ui_set_temperature(int temp_c)
{
    g_temp = temp_c;
}

void ui_set_humidity(int humidity_pct)
{
    g_humidity = humidity_pct;
}

/* Set a single pixel black in buf (200x200, 1bpp, row-major) */
static void set_pixel(uint8_t *buf, int x, int y)
{
    if (x < 0 || x >= 200 || y < 0 || y >= 200) {
        return;
    }
    int byte_idx = (y * 200 + x) / 8;
    int bit_idx  = 7 - (x % 8);
    buf[byte_idx] &= (uint8_t)~(1 << bit_idx);
}

/* Fill a rectangle black */
static void draw_rect(uint8_t *buf, int x, int y, int w, int h)
{
    int r, c;
    for (r = y; r < y + h; r++) {
        for (c = x; c < x + w; c++) {
            set_pixel(buf, c, r);
        }
    }
}

/* Draw only the border of a rectangle (1px thick) */
static void draw_rect_outline(uint8_t *buf, int x, int y, int w, int h)
{
    draw_rect(buf, x,       y,       w, 1);
    draw_rect(buf, x,       y+h-1,   w, 1);
    draw_rect(buf, x,       y,       1, h);
    draw_rect(buf, x+w-1,   y,       1, h);
}

/* Render one character from font_5x7 at (x, y) with given scale */
static void draw_char(uint8_t *buf, int x, int y, char ch, int scale)
{
    int idx;
    int row, col;

    if (ch >= '0' && ch <= '9') {
        idx = ch - '0';
    } else if (ch == 'C') {
        idx = 10;
    } else if (ch == '%') {
        idx = 11;
    } else if (ch == 'D') {
        idx = 12;
    } else if (ch == 'P') {
        idx = 13;
    } else if (ch == ':') {
        idx = 14;
    } else if (ch == '-') {
        idx = 15;
    } else {
        return;
    }

    for (row = 0; row < 7; row++) {
        uint8_t bits = font_5x7[idx][row];
        for (col = 0; col < 5; col++) {
            if (bits & (uint8_t)(0x10U >> col)) {
                draw_rect(buf,
                          x + col * scale,
                          y + row * scale,
                          scale, scale);
            }
        }
    }
}

/*
 * Draw temperature in range -9..99°C.
 * Negative: '-' + 2 digits. Non-negative: 2 digits (zero-padded).
 * Followed by 4×4 ° rect and 'C' at scale=2.
 */
static void draw_temperature(uint8_t *buf, int x, int y, int value, int scale)
{
    int char_w = 5 * scale;
    int gap = 8;
    int cx = x;

    if (value < 0) {
        draw_char(buf, cx, y, '-', scale);
        cx += char_w + gap;
        value = -value;
    }

    if (value >= 10) {
        draw_char(buf, cx, y, (char)('0' + (value / 10) % 10), scale);
        cx += char_w + gap;
    }
    draw_char(buf, cx, y, (char)('0' + value % 10), scale);
    cx += char_w;

    draw_rect(buf, cx + 5, y, 4, 4);   /* ° symbol */
    cx += 5 + 4 + 2;

    draw_char(buf, cx, y, 'C', 2);
}

/*
 * Draw humidity in range 0..100%.
 * 100 → three digits '1','0','0'. Otherwise 2 digits (zero-padded).
 * Followed by '%' at scale=2.
 */
static void draw_humidity(uint8_t *buf, int x, int y, int value, int scale)
{
    int char_w = 5 * scale;
    int gap = 8;
    int cx = x;

    if (value == 100) {
        draw_char(buf, cx, y, '1', scale);
        cx += char_w + gap;
        draw_char(buf, cx, y, '0', scale);
        cx += char_w + gap;
        draw_char(buf, cx, y, '0', scale);
        cx += char_w;
    } else if (value < 10) {
        draw_char(buf, cx, y, (char)('0' + value), scale);
        cx += char_w;
    } else {
        draw_char(buf, cx, y, (char)('0' + (value / 10) % 10), scale);
        cx += char_w + gap;
        draw_char(buf, cx, y, (char)('0' + value % 10), scale);
        cx += char_w;
    }

    draw_char(buf, cx + 5, y, '%', 2);
}

void ui_render(void)
{
    uint8_t *buf = display_get_buf();

    epd_clear();

    /* --- Battery indicator (top right) --- */
    draw_rect_outline(buf, 120,  7, 62, 18);        /* outer frame  */
    draw_rect(buf, 182, 10,  4, 11);                /* right contact */
    draw_rect(buf, 122,  9, 11, 14);                /* seg 0 filled */
    draw_rect(buf, 135,  9, 11, 14);                /* seg 1 filled */
    draw_rect(buf, 148,  9, 11, 14);                /* seg 2 filled */
    draw_rect_outline(buf, 161,  9, 11, 14);        /* seg 3 empty  */

    /* --- Dew risk drop icon (top left, outline) --- */
    draw_rect(buf, 12,  4,  2,  4);
    draw_rect(buf, 10,  8,  2,  3);
    draw_rect(buf, 14,  8,  2,  3);
    draw_rect(buf,  8, 11,  2,  3);
    draw_rect(buf, 16, 11,  2,  3);
    draw_rect(buf,  6, 14,  2,  3);
    draw_rect(buf, 18, 14,  2,  3);
    draw_rect(buf,  5, 17,  2,  4);
    draw_rect(buf, 19, 17,  2,  4);
    draw_rect(buf,  5, 21,  2,  3);
    draw_rect(buf, 19, 21,  2,  3);
    draw_rect(buf,  6, 24,  2,  3);
    draw_rect(buf, 18, 24,  2,  3);
    draw_rect(buf,  8, 27,  2,  3);
    draw_rect(buf, 16, 27,  2,  3);
    draw_rect(buf, 10, 30,  6,  2);

    /* Dew risk segments — all empty (risk=0), fill from bottom up */
    draw_rect_outline(buf, 3, 154, 20, 26);  /* seg 0 (bottom) */
    draw_rect_outline(buf, 3, 126, 20, 26);  /* seg 1 */
    draw_rect_outline(buf, 3,  98, 20, 26);  /* seg 2 */
    draw_rect_outline(buf, 3,  70, 20, 26);  /* seg 3 */
    draw_rect_outline(buf, 3,  42, 20, 26);  /* seg 4 (top) */

    /* --- Vertical divider --- */
    draw_rect(buf, 28, 2, 1, 196);

    /* Temperature centered in x:29..199 (w=170), y:30..110 (h=80) */
    {
        const int scale = 6;
        int char_w = 5 * scale;
        int gap = 8;
        int abs_temp = (g_temp < 0) ? -g_temp : g_temp;
        int n_chars = (g_temp < 0 ? 1 : 0) + (abs_temp < 10 ? 1 : 2);
        int total_w = n_chars * char_w + (n_chars - 1) * gap
                    + 4 + 2
                    + 5 * 2;
        int x_start = 29 + (170 - total_w) / 2;
        int y_start = 30 + (80 - 7 * scale) / 2;
        draw_temperature(buf, x_start, y_start, g_temp, scale);
    }

    /* Humidity centered in x:29..199 (w=170), y:115..185 (h=70) */
    {
        const int scale = 6;
        int char_w = 5 * scale;
        int gap = 8;
        int n_chars = (g_humidity < 10) ? 1 : (g_humidity == 100) ? 3 : 2;
        int total_w = n_chars * char_w + (n_chars - 1) * gap
                    + 5 * 2;
        int x_start = 29 + (170 - total_w) / 2;
        int y_start = 115 + (70 - 7 * scale) / 2;
        draw_humidity(buf, x_start, y_start, g_humidity, scale);
    }

    display_refresh();
}
