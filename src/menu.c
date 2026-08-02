#include <gbdk/platform.h>
#include <gb/gb.h>

#include "menu.h"
#include "render.h"
#include "input.h"
#include "frame.h"
#include "settings.h"
#include "sound.h"
#include "save.h"

#define ITEM_SPEED   0
#define ITEM_SOUND   1
#define ITEM_NEW     2
#define ITEM_RESUME  3
#define ITEM_COUNT   4

#define VALUE_COL    12
#define LABEL_COL     3
#define CURSOR_COL    1

static const uint8_t item_row[ITEM_COUNT]   = { 5, 7, 10, 12 };
static const char   *item_label[ITEM_COUNT] = { "SPEED", "SOUND", "NEW GAME", "RESUME" };

static void draw_cursor(uint8_t sel) {
    uint8_t i;
    for (i = 0; i < ITEM_COUNT; i++) {
        render_text(CURSOR_COL, item_row[i], (i == sel) ? ">" : " ");
    }
}

/* Values are padded to the full field width so a shorter word cannot leave
   the tail of a longer one behind. */
static void draw_values(void) {
    render_text(VALUE_COL, item_row[ITEM_SPEED], "       ");
    render_text(VALUE_COL, item_row[ITEM_SPEED], settings_speed_name(settings.speed));
    render_text(VALUE_COL, item_row[ITEM_SOUND], "       ");
    render_text(VALUE_COL, item_row[ITEM_SOUND], settings.sound ? "ON" : "OFF");
}

static void apply_sound_setting(void) {
    if (settings.sound == 0) {
        sound_silence();
    } else {
        sound_move();     /* confirm the change is audible straight away */
    }
}

static void adjust(uint8_t sel, int8_t delta) {
    if (sel == ITEM_SPEED) {
        settings.speed = (uint8_t)((settings.speed + SPEED_COUNT + delta) % SPEED_COUNT);
        draw_values();
        sound_move();
    } else if (sel == ITEM_SOUND) {
        settings.sound = settings.sound ? 0 : 1;
        draw_values();
        apply_sound_setting();
    }
}

MenuResult menu_open(uint32_t best) {
    uint8_t sel = 0, pressed, i;

    render_clear();
    render_text_centered(2, "SETTINGS");
    for (i = 0; i < ITEM_COUNT; i++) {
        render_text(LABEL_COL, item_row[i], item_label[i]);
    }
    draw_values();
    draw_cursor(sel);
    render_text_centered(14, "< > CHANGE");
    render_text_centered(16, "B CLOSE");

    input_reset();

    for (;;) {
        frame_next();
        pressed = input_pressed();

        if (pressed & (J_B | J_SELECT)) break;

        if (pressed & J_UP) {
            sel = (uint8_t)((sel + ITEM_COUNT - 1) % ITEM_COUNT);
            draw_cursor(sel);
            sound_move();
        }
        if (pressed & J_DOWN) {
            sel = (uint8_t)((sel + 1) % ITEM_COUNT);
            draw_cursor(sel);
            sound_move();
        }
        if (pressed & J_LEFT)  adjust(sel, -1);
        if (pressed & J_RIGHT) adjust(sel, 1);

        if (pressed & (J_A | J_START)) {
            if (sel == ITEM_RESUME) break;
            if (sel == ITEM_NEW) {
                save_store(best);
                input_reset();
                return MENU_NEW_GAME;
            }
            adjust(sel, 1);
        }
    }

    save_store(best);
    input_reset();
    return MENU_RESUME;
}
