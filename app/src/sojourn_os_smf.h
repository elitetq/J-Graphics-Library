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
                    Function Prototypes
----------------------------------------------------------*/
static void home_screen_state_entry(void* o);
static enum smf_state_result home_screen_state_run(void* o);
static void home_screen_state_exit(void* o);

static void about_page_state_entry(void* o);
static enum smf_state_result about_page_state_run(void* o);
static void about_page_state_exit(void* o);

static void lock_screen_state_entry(void* o);
static enum smf_state_result lock_screen_state_run(void* o);
static void lock_screen_state_exit(void* o);


static void snake_game_state_entry(void* o);
static enum smf_state_result snake_game_state_run(void* o);
static void snake_game_state_exit(void* o);

static void snake_reset_state_entry(void* o);
static enum smf_state_result snake_reset_state_run(void* o);
static void snake_reset_state_exit(void* o);

static void start_up_state_entry(void* o);
static enum smf_state_result start_up_state_run(void* o);
static void start_up_state_exit(void* o);
/*----------------------------------------------------------
                Component Style Defines
----------------------------------------------------------*/

#define J_STYLE_COL ctx->global_col
#define J_STYLE_BG_COL ctx->global_bg_col
#define J_STYLE_ACCENT_COL ctx->global_accent_col
#define J_STYLE_ERROR_COL ctx->error_col
#define J_STYLE_CONFIRM_COL ctx->confirm_col

#define J_STYLE_BUTTON_DECAL(j_color_pick, j_color_bg_pick, j_decal_data, x_len, y_len) (j_button_data){.bg_col = j_color_bg_pick, .col = j_color_pick, .border_col = j_color_pick, .border_width = 0, .decal_dat = j_decal_data, .font_size = FONT_SMALL, .height = x_len, .length = y_len, .pressed_status = 0, .tag = 0}
#define J_STYLE_BUTTON_STATUS_MESSAGE (j_button_data){.bg_col = ctx->global_bg_col, .col = ctx->global_col, .border_col = ctx->global_accent_col, .border_width = 2, .decal_dat = NULL, .font_size = FONT_MEDIUM, .height = 50, .length = 200, .pressed_status = 0, .tag = 0}
#define J_STYLE_BUTTON_CTX_MENU(n) (j_button_data){.bg_col = ctx->global_bg_col, .col = ctx->global_bg_col, .border_col = ctx->global_accent_col, .border_width = 2, .decal_dat = NULL, .font_size = FONT_MEDIUM, .height = 50*n, .length = 175, .pressed_status = 0, .tag = 0}
#define J_STYLE_BUTTON_SQUARE_SMALL(j_decal_data, j_font_size) (j_button_data){.bg_col = ctx->global_col, .col = ctx->global_bg_col, .border_col = ctx->global_accent_col, .font_size = j_font_size, .height = 35, .length = 35, .pressed_status = 0, .decal_dat = j_decal_data, .border_width = 2, .tag = 0}
#define J_STYLE_BUTTON_SQUARE_MEDIUM(j_decal_data, j_font_size) (j_button_data){.bg_col = ctx->global_col, .col = ctx->global_bg_col, .border_col = ctx->global_accent_col, .font_size = j_font_size, .height = 44, .length = 44, .pressed_status = 0, .decal_dat = j_decal_data, .border_width = 2, .tag = 0}

#define J_STYLE_DECAL_DEFAULT (j_decal_data){.animation_dat = NULL, .bg_col = ctx->global_bg_col, .col = ctx->global_col};
#define J_STYLE_DECAL_DEFAULT_INVERTED (j_decal_data){.animation_dat = NULL, .bg_col = ctx->global_col, .col = ctx->global_bg_col};

#define J_STYLE_TEXT_DEFAULT(j_centering, j_font_size) (j_text_data){.bg_col = ctx->global_bg_col, .col = ctx->global_accent_col, .centering = j_centering, .font_size = j_font_size};
#define J_STYLE_TEXT_DEFAULT_COLOR(j_centering, j_font_size, j_color_pick) (j_text_data){.bg_col = ctx->global_bg_col, .col = j_color_pick, .centering = j_centering, .font_size = j_font_size};
/*----------------------------------------------------------
                Sojourn OS Page Contexts
----------------------------------------------------------*/
typedef struct {
  j_button_data button_dat_1;
  j_decal_data decal_dat;
  j_text_data text_dat;
} os_about_page_ctx;

typedef struct {
  j_button_data button_dat_1, button_dat_2;
  j_decal_data banner_decal_dat;
  j_button_data app1, app2, app3;
  j_button_data ctx_menu_dat, ctx_menu_button_dat;
  j_shape_data shape_dat;
  j_animation_data shape_anim_dat;
  bool ctx_pressed;
  uint8_t ctx_offset; // For ctx button tracking
  j_component *ctx_button1, *ctx_button2, *ctx_menu_background, *shape;

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

typedef enum {
  SG_NONE = 0,
  SG_LEFT,
  SG_RIGHT,
  SG_UP,
  SG_DOWN
} sg_directions;

typedef enum {
  SG_NFILL = 0,
  SG_FILL,
  SG_FOOD
} sg_tile;

typedef struct {
  // Game related variables
  uint8_t game_array[100]; // Array showing where the current snake is
  uint8_t dir_array[100]; // Stores the directions taken by user so that the program can follow
  uint8_t cur_x, cur_y, cur_xf, cur_yf, snake_len, cur_dir_index, pellet_amt;
  sg_directions next_dir, cur_dir, dir_global;
  bool pellet_flag, cover_flag;
  int pellet_timer; // Used to spawn pellets in the game periodically
  uint32_t user_score;
  char user_score_str[20];
  uint8_t temp_pellet_x, temp_pellet_y;

  // Visual variables
  uint16_t DELAY_PER_CPU_CYCLE;
  uint8_t x_board_offset, y_board_offset;
  j_decal_data bg_decal_dat1, bg_decal_dat2, pellet_decal_dat;
  j_text_data score_text_dat, announce_text_dat;
  j_button_data button_decal_dat, button_decal_left_dat, button_decal_right_dat, button_decal_up_dat, button_decal_down_dat, announce_button_dat, announce_bg_dat;
  j_component *bg_comp_dat, *snake_shape_comp_dat, *snake_eyes_comp_dat, *button_comp_left, *button_comp_right, *button_comp_up, *button_comp_down, *pellet_comp_dat, *announce_but_comp_dat1, *announce_but_comp_dat2, *score_text_comp_dat, *announce_bg_comp_dat;
  j_shape_data shape_dat, shape_dat2;
} os_snake_game_ctx;

typedef struct {
  j_animation_data anim_dat;
  j_decal_data logo_decal_dat;
  j_text_data text_dat;
  j_bar_data bar_dat;
  uint8_t bar_val, start_count; // Start count is a variable for switching text in the loading screen
  j_component *bg_comp_dat, *logo_comp_dat, *text_comp_dat, *loading_bar_comp_dat;
} os_start_up_ctx;
/*----------------------------------------------------------
            Sojourn OS Context Structs / Enums
----------------------------------------------------------*/
typedef enum {
  HOME_SCREEN = 0,
  ABOUT_PAGE,
  LOCK_SCREEN,
  THEME_PAGE,
  SHUT_DOWN,
  RESTART,
  SNAKE_GAME,
  SNAKE_RESET,
  START_UP
} sos_smf_states;

typedef union {
  os_about_page_ctx about_page_dat;
  os_lock_screen_ctx lock_screen_dat;
  os_home_screen_ctx home_screen_dat;
  os_snake_game_ctx snake_game_dat;
  os_start_up_ctx start_up_dat;
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