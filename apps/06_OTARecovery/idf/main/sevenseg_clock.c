#include "sevenseg_clock.h"

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "time_service.h"

#define DIGITS 6
#define SEGMENTS 7

struct sevenseg_clock_view {
    lv_obj_t *segments[DIGITS][SEGMENTS];
    lv_obj_t *colon[2][2];
    uint32_t active_color;
    uint32_t off_color;
};

static int max_i(int a, int b) { return a > b ? a : b; }

static lv_obj_t *segment_create(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, max_i(2, (w < h ? w : h) / 2), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

static uint8_t digit_mask(int digit)
{
    static const uint8_t masks[10] = {
        0x3F, 0x06, 0x5B, 0x4F, 0x66,
        0x6D, 0x7D, 0x07, 0x7F, 0x6F,
    };
    if (digit >= 0 && digit <= 9) return masks[digit];
    return 0x40; /* Unsynchronized: middle segment, like '-'. */
}

static void set_digit(sevenseg_clock_view_t *view, int index, int digit)
{
    uint8_t mask = digit_mask(digit);
    for (int s = 0; s < SEGMENTS; ++s) {
        lv_obj_set_style_bg_color(view->segments[index][s],
                                  lv_color_hex((mask & (1U << s)) ? view->active_color : view->off_color), 0);
    }
}

sevenseg_clock_view_t *sevenseg_clock_create(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color)
{
    if (!parent || w < 320 || h < 90) return NULL;

    sevenseg_clock_view_t *view = calloc(1, sizeof(*view));
    if (!view) return NULL;
    view->active_color = color;

    uint32_t r = (color >> 16) & 0xFF;
    uint32_t g = (color >> 8) & 0xFF;
    uint32_t b = color & 0xFF;
    view->off_color = ((r / 8) << 16) | ((g / 8) << 8) | (b / 8);

    int gap = max_i(4, w / 120);
    int colon_w = max_i(14, w / 28);
    int digit_w = (w - (2 * colon_w) - (7 * gap)) / 6;
    int t = max_i(5, h / 12);
    int v = (h - (3 * t)) / 2;
    if (digit_w <= (2 * t) || v <= 4) {
        free(view);
        return NULL;
    }

    int cursor = x;
    int digit_index = 0;
    for (int cell = 0; cell < 8; ++cell) {
        bool is_colon = (cell == 2 || cell == 5);
        if (is_colon) {
            int colon_index = cell == 2 ? 0 : 1;
            int dot = max_i(7, t);
            int cx = cursor + (colon_w - dot) / 2;
            view->colon[colon_index][0] = segment_create(parent, cx, y + (h / 3) - (dot / 2), dot, dot, view->active_color);
            view->colon[colon_index][1] = segment_create(parent, cx, y + ((2 * h) / 3) - (dot / 2), dot, dot, view->active_color);
            cursor += colon_w + gap;
            continue;
        }

        int dx = cursor;
        int hor = digit_w - (2 * t);
        view->segments[digit_index][0] = segment_create(parent, dx + t, y, hor, t, view->off_color);               /* A */
        view->segments[digit_index][1] = segment_create(parent, dx + digit_w - t, y + t, t, v, view->off_color);   /* B */
        view->segments[digit_index][2] = segment_create(parent, dx + digit_w - t, y + (2 * t) + v, t, v, view->off_color); /* C */
        view->segments[digit_index][3] = segment_create(parent, dx + t, y + h - t, hor, t, view->off_color);       /* D */
        view->segments[digit_index][4] = segment_create(parent, dx, y + (2 * t) + v, t, v, view->off_color);       /* E */
        view->segments[digit_index][5] = segment_create(parent, dx, y + t, t, v, view->off_color);                 /* F */
        view->segments[digit_index][6] = segment_create(parent, dx + t, y + t + v, hor, t, view->off_color);       /* G */
        digit_index++;
        cursor += digit_w + gap;
    }

    sevenseg_clock_update(view);
    return view;
}

void sevenseg_clock_update(sevenseg_clock_view_t *view)
{
    if (!view) return;

    struct tm now = {0};
    bool valid = time_service_get_local_time(&now);
    int digits[DIGITS] = {-1, -1, -1, -1, -1, -1};
    if (valid) {
        digits[0] = now.tm_hour / 10;
        digits[1] = now.tm_hour % 10;
        digits[2] = now.tm_min / 10;
        digits[3] = now.tm_min % 10;
        digits[4] = now.tm_sec / 10;
        digits[5] = now.tm_sec % 10;
    }

    for (int i = 0; i < DIGITS; ++i) set_digit(view, i, digits[i]);

    bool colon_on = valid && ((now.tm_sec & 1) == 0);
    uint32_t colon_color = colon_on ? view->active_color : view->off_color;
    for (int c = 0; c < 2; ++c) {
        for (int d = 0; d < 2; ++d) {
            lv_obj_set_style_bg_color(view->colon[c][d], lv_color_hex(colon_color), 0);
        }
    }
}

void sevenseg_clock_destroy(sevenseg_clock_view_t *view)
{
    free(view);
}
