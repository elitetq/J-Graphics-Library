#include <zephyr/smf.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/random/random.h>
#include <zephyr/drivers/i2c.h>

#include "LED.h"
#include "sojourn_os_smf.h"
#include "J_GL.h"
#include "J_ASSETS.h"
#include <stdio.h>

extern const struct gpio_dt_spec dcx_gpio;
extern const struct spi_cs_control cs_ctrl;
extern const struct device *dev;
extern const struct device *dev_i2c;
extern const struct spi_config spi_cfg;
extern uint16_t bounds[4];


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
/*----------------------------------------------------------
                Sojourn OS Page Contexts
----------------------------------------------------------*/
typedef struct {
  j_color bg_col;
  j_button_data button_dat_1, button_dat_2;
  j_decal_data decal_dat;
} os_about_page_ctx;

typedef struct {
  j_color bg_col;
  j_button_data button_dat, button_dat_2;
  j_shape_data shape_dat;
  j_decal_data decal_dat;
  j_text_data text_dat;
  uint8_t cur_number_index;
  j_component* shape;
} os_lock_screen_ctx;


/*----------------------------------------------------------
            Sojourn OS Context Structs / Enums
----------------------------------------------------------*/
enum sos_smf_states {
  HOME_SCREEN = 0,
  ABOUT_PAGE,
  LOCK_SCREEN
};

typedef union {
  os_about_page_ctx about_page_dat;
  os_lock_screen_ctx lock_screen_dat;
} page_ctx;

typedef struct {
  struct smf_ctx ctx;
  j_color global_col, global_bg_col, global_accent_col; // Theme colors
  page_ctx page_dat;
  uint8_t os_flags; // From MSB to LSB: first_time_setup,
  uint8_t password[6];
} os_state_ctx;

/*----------------------------------------------------------
                     Local Variables
----------------------------------------------------------*/
static const struct smf_state sos_states[] = {
  [HOME_SCREEN] = SMF_CREATE_STATE(home_screen_state_entry, home_screen_state_run, home_screen_state_exit, NULL, NULL),
  [ABOUT_PAGE] = SMF_CREATE_STATE(about_page_state_entry, about_page_state_run, about_page_state_exit, NULL, NULL),
  [LOCK_SCREEN] = SMF_CREATE_STATE(lock_screen_state_entry, lock_screen_state_run, lock_screen_state_exit, NULL, NULL),
};

static os_state_ctx sos_object;


void state_machine_init() {
  J_init(dev,dev_i2c,&spi_cfg,&dcx_gpio,bounds);
  J_LCD_init();
  draw_color_fs(BLACK);
  sos_object.global_col = WHITE;
  sos_object.global_bg_col = BLACK;
  sos_object.global_accent_col = GRAY;
  sos_object.os_flags = 0b10000000;
  smf_set_initial(SMF_CTX(&sos_object),&sos_states[LOCK_SCREEN]);
}

int state_machine_run() {
  return smf_run_state(SMF_CTX(&sos_object));
}


/*----------------------------------------------------------
                State Function Definitions
----------------------------------------------------------*/

static void lock_screen_state_entry(void* o) {
  os_state_ctx* ctx = (os_state_ctx*)o;
  memset(&ctx->page_dat,0,sizeof(os_lock_screen_ctx));
  os_lock_screen_ctx* page_dat = &ctx->page_dat.lock_screen_dat;

  page_dat->bg_col = ctx->global_bg_col;
  page_dat->button_dat = (j_button_data){.bg_col = ctx->global_col, .col = ctx->global_bg_col, .border_col = ctx->global_accent_col, .font_size = FONT_LARGE, .height = 35, .length = 35, .pressed_status = 0, .decal_dat = NULL, .border_width = 2};
  page_dat->button_dat_2 = (j_button_data){.bg_col = ctx->global_col, .col = ctx->global_bg_col, .border_col = ctx->global_accent_col, .font_size = FONT_LARGE, .height = 35, .length = 35, .pressed_status = 0, .decal_dat = icon_arrow_left_decal, .border_width = 2};
  page_dat->decal_dat = (j_decal_data){.animation_dat = NULL, .bg_col = ctx->global_bg_col, .col = ctx->global_col};
  page_dat->shape_dat = (j_shape_data){.centering = J_LEFT, .bg_col = ctx->global_bg_col, .col = ctx->global_col, .type = J_RECTANGLE, .height = 20, .length = 20};
  page_dat->text_dat = (j_text_data){.bg_col = ctx->global_bg_col, .col = ctx->global_accent_col, .centering = J_CENTER, .font_size = FONT_SMALL};

  char *pos_strings[] = {"Enter a new password...", "Enter password to unlock..."};
  char* status_string = pos_strings[!(ctx->os_flags & 0x80)];

  page_dat->cur_number_index = 0;

  j_component* FILL = create_component("bg_fill",J_FILL,0,0,&page_dat->bg_col,NULL);
  j_component* TEXT = create_component("status_text",J_TEXT,120,15,status_string,&page_dat->text_dat);
  uint8_t offset = 20;
  j_component* BUTTON0 = create_component("button0",J_BUTTON,80+offset,250,"0",&page_dat->button_dat);
  j_component* BUTTON1 = create_component("button1",J_BUTTON,30+offset,100,"1",&page_dat->button_dat);
  j_component* BUTTON2 = create_component("button2",J_BUTTON,80+offset,100,"2",&page_dat->button_dat);
  j_component* BUTTON3 = create_component("button3",J_BUTTON,130+offset,100,"3",&page_dat->button_dat);
  j_component* BUTTON4 = create_component("button4",J_BUTTON,30+offset,150,"4",&page_dat->button_dat);
  j_component* BUTTON5 = create_component("button5",J_BUTTON,80+offset,150,"5",&page_dat->button_dat);
  j_component* BUTTON6 = create_component("button6",J_BUTTON,130+offset,150,"6",&page_dat->button_dat);
  j_component* BUTTON7 = create_component("button7",J_BUTTON,30+offset,200,"7",&page_dat->button_dat);
  j_component* BUTTON8 = create_component("button8",J_BUTTON,80+offset,200,"8",&page_dat->button_dat);
  j_component* BUTTON9 = create_component("button9",J_BUTTON,130+offset,200,"9",&page_dat->button_dat);
  j_component* BUTTON10 = create_component("buttonA",J_BUTTON,130+offset,250,"",&page_dat->button_dat_2);
  j_component* SHAPE = create_component("confirm_shape",J_SHAPE,10,30,&page_dat->shape_dat,NULL);

  page_dat->shape = SHAPE;

  add_component_o(FILL);
  add_component_o(TEXT);
  add_component_o(BUTTON1);
  add_component_o(BUTTON2);
  add_component_o(BUTTON3);
  add_component_o(BUTTON4);
  add_component_o(BUTTON5);
  add_component_o(BUTTON6);
  add_component_o(BUTTON7);
  add_component_o(BUTTON8);
  add_component_o(BUTTON9);
  add_component_o(BUTTON0);
  add_component_o(BUTTON10);
  draw_screen_o(NULL, 0);
}

static enum smf_state_result lock_screen_state_run(void* o) {
  os_state_ctx* ctx = (os_state_ctx*)o;
  os_lock_screen_ctx* page_dat = &ctx->page_dat.lock_screen_dat;
  j_shape_data* shape_dat = (j_shape_data*)page_dat->shape->dat;
  uint16_t x, y;
  poll_touch(&x, &y);
  j_component* BUTTON_PRESSED = lcd_check_button_pressed(x, y, 5);
  if(press_button_visual(BUTTON_PRESSED)) {
    if(BUTTON_PRESSED->name[6] != 'A') {
      shape_dat->col = ctx->global_col;
      sos_object.password[page_dat->cur_number_index] = (uint8_t)(BUTTON_PRESSED->name[6] - 48);
      page_dat->shape->x = 30 + page_dat->cur_number_index*30;
      page_dat->cur_number_index++;
    } else {
      shape_dat->col = ctx->global_bg_col;
      page_dat->cur_number_index ? page_dat->cur_number_index-- : 0;
      page_dat->shape->x = 30 + page_dat->cur_number_index*30;
    }
    draw_component(page_dat->shape);
  }
  while(page_dat->cur_number_index == 6) { }
  return SMF_EVENT_HANDLED;
}

static void lock_screen_state_exit(void* o) {
  os_state_ctx* ctx = (os_state_ctx*)o;
  os_lock_screen_ctx* page_dat = &ctx->page_dat.lock_screen_dat;
  free(page_dat->shape);
  clear_draw_buffer();
}

static void about_page_state_entry(void* o) {
  os_state_ctx* ctx = (os_state_ctx*)o;
  memset(&ctx->page_dat,0,sizeof(os_about_page_ctx));
  os_about_page_ctx* page_dat = &ctx->page_dat.about_page_dat;
  page_dat->bg_col = ctx->global_col;
  page_dat->button_dat_1 = (j_button_data){.bg_col = ctx->global_col, .border_col = ctx->global_col, .col = ctx->global_bg_col, .border_width = 0,
    .decal_dat = icon_shutdown_decal,
    .font_size = FONT_SMALL, .height = 31, .length = 31, .pressed_status = 0};
  
  page_dat->button_dat_2 = (j_button_data){.bg_col = ctx->global_col, .border_col = ctx->global_accent_col, .col = ctx->global_bg_col, .border_width = 0,
    .decal_dat = icon_config_decal,
    .font_size = FONT_SMALL, .height = 31, .length = 31, .pressed_status = 0};
  
  page_dat->decal_dat = (j_decal_data){.animation_dat = NULL, .bg_col = ctx->global_bg_col, .col = ctx->global_col};

  ctx->page_dat.about_page_dat.bg_col = BLACK;


  j_component* FILL = create_component("bg_col",J_FILL,0,0,&page_dat->bg_col,NULL);
  j_component* BANNER_DECAL = create_component("banner_decal",J_DECAL,0,0,banner_gui_decal,NULL);
  j_component* PLANET_DECAL = create_component("planet_decal",J_DECAL,0,220,planet_gui_decal,NULL);
  j_component* SOJOURN_DECAL = create_component("sojourn_logo",J_DECAL,30,100,logo_text_gui_decal,&page_dat->decal_dat);
  j_component* CONFIG_BUTTON = create_component("button1",J_BUTTON,10,4,"",&page_dat->button_dat_2);
  j_component* POWER_BUTTON = create_component("button2",J_BUTTON,200,4,"",&page_dat->button_dat_1);

  add_component_o(FILL);
  add_component_o(BANNER_DECAL);
  add_component_o(PLANET_DECAL);
  add_component_o(SOJOURN_DECAL);
  add_component_o(CONFIG_BUTTON);
  add_component_o(POWER_BUTTON);
  draw_screen_o(NULL,0);
}

static enum smf_state_result about_page_state_run(void* o) {
  uint16_t x_touch, y_touch;
  poll_touch(&x_touch,&y_touch);
  j_component* button = lcd_check_button_pressed(x_touch,y_touch,10);
  if(press_button_visual(button)) {
    j_button_data* b_dat = (j_button_data*)button->dat2;
    printk("%s pressed! x, y, len, hei: %d %d %d %d\n\n",button->name, button->x, button->y, b_dat->length, b_dat->height);
  }
  printk("%d, %d\n", x_touch, y_touch);
  return SMF_EVENT_HANDLED;
}

static void about_page_state_exit(void* o) {
  clear_draw_buffer();
}


static void home_screen_state_entry(void* o) {
  return;
}

static enum smf_state_result home_screen_state_run(void* o) {
  return SMF_EVENT_HANDLED;
}

static void home_screen_state_exit(void* o) {
  clear_draw_buffer();
}