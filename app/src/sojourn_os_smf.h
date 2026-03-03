#ifndef SOJOURN_OS
#define SOJOURN_OS
#include <zephyr/smf.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/random/random.h>
#include <zephyr/drivers/i2c.h>
#include "J_GL.h"
#include "J_ASSETS.h"
#include <stdio.h>


#include "LED.h"

void state_machine_init();

int state_machine_run();

typedef enum {
  VOID = 0,
  DIAMOND,
  AMETHYST,
  DUST
} sos_themes;

/*----------------------------------------------------------
                Component Style Defines
----------------------------------------------------------*/

#define J_STYLE_COL ctx->global_col
#define J_STYLE_BG_COL ctx->global_bg_col
#define J_STYLE_ACCENT_COL ctx->global_accent_col
#define J_STYLE_ERROR_COL ctx->error_col
#define J_STYLE_CONFIRM_COL ctx->confirm_col

#define J_STYLE_BUTTON_DECAL(j_color_pick, j_color_bg_pick, j_decal_data, x_len, y_len) (j_button_data){.bg_col = j_color_bg_pick, .col = j_color_pick, .border_col = j_color_pick, .border_width = 0, .decal_dat = j_decal_data, .font_size = FONT_SMALL, .height = x_len, .length = y_len, .pressed_status = 0}
#define J_STYLE_BUTTON_STATUS_MESSAGE (j_button_data){.bg_col = ctx->global_bg_col, .col = ctx->global_col, .border_col = ctx->global_accent_col, .border_width = 2, .decal_dat = NULL, .font_size = FONT_MEDIUM, .height = 50, .length = 200, .pressed_status = 0}
#define J_STYLE_BUTTON_CTX_MENU(n) (j_button_data){.bg_col = ctx->global_bg_col, .col = ctx->global_bg_col, .border_col = ctx->global_accent_col, .border_width = 2, .decal_dat = NULL, .font_size = FONT_MEDIUM, .height = 50*n, .length = 175, .pressed_status = 0}
#define J_STYLE_BUTTON_SQUARE_SMALL(j_decal_data, j_font_size) (j_button_data){.bg_col = ctx->global_col, .col = ctx->global_bg_col, .border_col = ctx->global_accent_col, .font_size = j_font_size, .height = 35, .length = 35, .pressed_status = 0, .decal_dat = j_decal_data, .border_width = 2}
#define J_STYLE_BUTTON_SQUARE_MEDIUM(j_decal_data, j_font_size) (j_button_data){.bg_col = ctx->global_col, .col = ctx->global_bg_col, .border_col = ctx->global_accent_col, .font_size = j_font_size, .height = 45:, .length = 45, .pressed_status = 0, .decal_dat = j_decal_data, .border_width = 2}

#define J_STYLE_DECAL_DEFAULT (j_decal_data){.animation_dat = NULL, .bg_col = ctx->global_bg_col, .col = ctx->global_col};
#define J_STYLE_DECAL_DEFAULT_INVERTED (j_decal_data){.animation_dat = NULL, .bg_col = ctx->global_col, .col = ctx->global_bg_col};

#define J_STYLE_TEXT_DEFAULT(j_centering, j_font_size) (j_text_data){.bg_col = ctx->global_bg_col, .col = ctx->global_accent_col, .centering = j_centering, .font_size = j_font_size};
#define J_STYLE_TEXT_DEFAULT_COLOR(j_centering, j_font_size, j_color_pick) (j_text_data){.bg_col = ctx->global_bg_col, .col = j_color_pick, .centering = j_centering, .font_size = j_font_size};
/*----------------------------------------------------------
                Sojourn OS Page Contexts
----------------------------------------------------------*/
typedef struct {
  j_button_data button_dat_1, button_dat_2;
  j_decal_data decal_dat;
} os_about_page_ctx;

typedef struct {
  j_button_data button_dat_1, button_dat_2;
  j_decal_data banner_decal_dat;
  j_button_data app1, app2, app3;
  j_button_data ctx_menu_1, ctx_menu_2;
} os_home_screen_ctx;

typedef struct {
  j_button_data button_dat, button_dat_2;

  j_button_data message_dat;
  j_component* message_comp_ptr;
  char* message_str[30];

  j_shape_data shape_dat;
  j_decal_data decal_dat;
  j_text_data text_dat;
  uint8_t cur_number_index;
  j_component *shape, *text1, *text2;
} os_lock_screen_ctx;

/*----------------------------------------------------------
            Sojourn OS Context Structs / Enums
----------------------------------------------------------*/
typedef enum {
  HOME_SCREEN = 0,
  ABOUT_PAGE,
  LOCK_SCREEN
} sos_smf_states;

typedef union {
  os_about_page_ctx about_page_dat;
  os_lock_screen_ctx lock_screen_dat;
  os_home_screen_ctx home_screen_dat;
} page_ctx;

typedef struct {
  struct smf_ctx ctx;
  j_color global_col, global_bg_col, global_accent_col, error_col, confirm_col; // Theme colors
  page_ctx page_dat;
  uint8_t os_flags; // From MSB to LSB: first_time_setup,
  uint8_t password[6];
} os_state_ctx;

void set_theme(sos_themes theme);

#endif