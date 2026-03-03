#include "sojourn_os_smf.h"

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
                     Local Variables
----------------------------------------------------------*/
static const struct smf_state sos_states[] = {
  [HOME_SCREEN] = SMF_CREATE_STATE(home_screen_state_entry, home_screen_state_run, home_screen_state_exit, NULL, NULL),
  [ABOUT_PAGE] = SMF_CREATE_STATE(about_page_state_entry, about_page_state_run, about_page_state_exit, NULL, NULL),
  [LOCK_SCREEN] = SMF_CREATE_STATE(lock_screen_state_entry, lock_screen_state_run, lock_screen_state_exit, NULL, NULL),
};
static os_state_ctx sos_object;
static uint16_t x, y;
static sos_themes global_theme = VOID;


void state_machine_init() {
  J_init(dev,dev_i2c,&spi_cfg,&dcx_gpio,bounds);
  J_LCD_init();
  draw_color_fs(BLACK);
  set_theme(DUST);
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
  // Dereferencing OS context and making sure the union struct is empty before we make changes.
  os_state_ctx* ctx = (os_state_ctx*)o;
  memset(&ctx->page_dat,0,sizeof(os_lock_screen_ctx));
  os_lock_screen_ctx* page_dat = &ctx->page_dat.lock_screen_dat;

  // Styling page -- Lots of ugly code here
  page_dat->button_dat = J_STYLE_BUTTON_SQUARE_SMALL(NULL,FONT_LARGE);
  page_dat->button_dat_2 = J_STYLE_BUTTON_SQUARE_SMALL(icon_arrow_left_decal,FONT_LARGE);
  page_dat->decal_dat = J_STYLE_DECAL_DEFAULT;
  page_dat->shape_dat = (j_shape_data){.centering = J_LEFT, .bg_col = J_STYLE_BG_COL, .col = J_STYLE_COL, .type = J_RECTANGLE, .height = 15, .length = 15};
  page_dat->text_dat = J_STYLE_TEXT_DEFAULT(J_CENTER,FONT_SMALL);
  page_dat->message_dat = J_STYLE_BUTTON_STATUS_MESSAGE;
  // -------------------------------------
  
  char *pos_strings[] = {"Enter a new password...", "Enter password to unlock..."};
  bool is_locked = (ctx->os_flags & 0x80) != 0x50;
  char* status_string = pos_strings[!is_locked];

  page_dat->cur_number_index = 0; // Set the current number of the password number index to 0.

  j_component* FILL = create_component("bg_fill",J_FILL,0,0,&J_STYLE_BG_COL,NULL);
  j_component* TEXT = create_component("status_text",J_TEXT,120,15,status_string,&page_dat->text_dat);
  uint8_t offset = 20;
  j_component* SHAPE = create_component("confirm_shape",J_SHAPE,10,50,&page_dat->shape_dat,NULL);
  j_component* MESSAGE_COMP = create_component("message_status",J_BUTTON,120-page_dat->message_dat.length/2,160-page_dat->message_dat.height/2,page_dat->message_str,&page_dat->message_dat);
  // Pass these into the page_dat struct so the state machine run can access these variables.
  page_dat->message_comp_ptr = MESSAGE_COMP;
  page_dat->shape = SHAPE;
  page_dat->text1 = TEXT;

  add_component_o(FILL);
  add_component_o(TEXT);

  struct button_j {
    int x, y;
    char *str;
  };
  char str_but_name[] = "button0";
  struct button_j button_array[11] = {{80+offset,250,"button0"},{30+offset,100,"button1"},{80+offset,100,"button2"},{130+offset,100,"button3"},{30+offset,150,"button4"},{80+offset,150,"button5"},{130+offset,150,"button6"},{30+offset,200,"button7"},{80+offset,200,"button8"},{130+offset,200,"button9"},{130+offset,250,"10"}};
  for(int i = 0; i < 10; i++) {
    j_component* cur = create_component(button_array[i].str,J_BUTTON,button_array[i].x,button_array[i].y,button_array[i].str+6,&page_dat->button_dat);
    add_component_o(cur);
  }
  j_component* cur = create_component("Back",J_BUTTON,button_array[10].x,button_array[10].y,button_array[10].str,&page_dat->button_dat_2);
  add_component_o(cur);


  draw_screen_o(NULL, 0);
}

static enum smf_state_result lock_screen_state_run(void* o) {
  // State context setup
  os_state_ctx* ctx = (os_state_ctx*)o;
  os_lock_screen_ctx* page_dat = &ctx->page_dat.lock_screen_dat;
  j_shape_data* shape_dat = (j_shape_data*)page_dat->shape->dat;
  bool first_time_setup = ctx->os_flags & 0x80;
  static uint8_t local_password[6]; // Store local attempt

  char* str_status[3] = {"Password set...","Incorrect...","Correct..."};
  char *pos_strings[] = {"Enter a new password...", "Enter password to unlock..."};
  char* status_string = pos_strings[1];


  uint16_t x, y;
  poll_touch(&x, &y); // Checking when screen is touched, storing result in x and y
  j_component* BUTTON_PRESSED = lcd_check_button_pressed(x, y, 5); // Allowing for 5 cm of buffer space, checking pressed button
  if(press_button_visual(BUTTON_PRESSED)) {
    if(BUTTON_PRESSED->name[0] != 'B') { // Kinda scuffed, but will work when the button is not the back button
      shape_dat->col = J_STYLE_COL;
      local_password[page_dat->cur_number_index] = (uint8_t)(BUTTON_PRESSED->name[6] - 48);
      page_dat->shape->x = 30 + page_dat->cur_number_index*30;
      page_dat->cur_number_index++;
    } else { // When back button is pressed
      shape_dat->col = J_STYLE_BG_COL; // Writing over last index password block
      page_dat->cur_number_index ? page_dat->cur_number_index-- : 0; // Revert index unless at index 0
      page_dat->shape->x = 30 + page_dat->cur_number_index*30;
    }
    draw_component(page_dat->shape);
  }

  // Check passsword validity
  if(page_dat->cur_number_index == 6) {
    if(first_time_setup) {
      // Set password
      for(int i = 0; i < 6; i++) {
        ctx->password[i] = local_password[i];
      }
      ctx->os_flags &= 0x7F; // Set first_time flag to 0
      page_dat->message_dat.col = J_STYLE_CONFIRM_COL;
      page_dat->message_comp_ptr->dat = (void*)str_status[0]; // Set successfully!

      set_bounds((uint16_t[]){0,LCD_MAX_HEIGHT,50,90});
      cmd_bounds();
      draw_color_fs(J_STYLE_BG_COL);
      page_dat->cur_number_index = 0;
      
      draw_component(page_dat->message_comp_ptr);
      k_msleep(2000);
      update_text(page_dat->text1,pos_strings[1]);
      draw_screen_o(NULL,0);


    } else {
      for(int i = 0; i < 6; i++) {
        printk("Comparing %d with %d\n",local_password[i],ctx->password[i]);
        if(local_password[i] != ctx->password[i]) {
          page_dat->message_dat.col = J_STYLE_ERROR_COL;
          page_dat->message_comp_ptr->dat = (void*)str_status[1]; // Set successfully!
          set_bounds((uint16_t[]){0,LCD_MAX_HEIGHT,50,90});
          cmd_bounds();
          draw_color_fs(J_STYLE_BG_COL);
          page_dat->cur_number_index = 0;

          draw_component(page_dat->message_comp_ptr);
          k_msleep(2000);
          draw_screen_o(NULL,0);

          printk("Entered incorrect\n");
          return SMF_EVENT_HANDLED;
        };
      }
      page_dat->message_dat.col = J_STYLE_CONFIRM_COL;
      page_dat->message_comp_ptr->dat = (void*)str_status[2]; // Password correct message display
      draw_component(page_dat->message_comp_ptr);
      k_msleep(2000);

      printk("Entered correct\n");
      // If not first time, and correct password is input, code will end up here
      smf_set_state(SMF_CTX(&sos_object),&sos_states[HOME_SCREEN]);
    }
  }
  return SMF_EVENT_HANDLED;
}

static void lock_screen_state_exit(void* o) {
  os_state_ctx* ctx = (os_state_ctx*)o;
  os_lock_screen_ctx* page_dat = &ctx->page_dat.lock_screen_dat;
  free(page_dat->shape);
  free(page_dat->message_comp_ptr);
  clear_draw_buffer();
  printk("EXITING LOCK SCREEN\n\n");
}

static void about_page_state_entry(void* o) {
  os_state_ctx* ctx = (os_state_ctx*)o;
  memset(&ctx->page_dat,0,sizeof(os_about_page_ctx));
  os_about_page_ctx* page_dat = &ctx->page_dat.about_page_dat;
  page_dat->button_dat_1 = (j_button_data){.bg_col = ctx->global_col, .border_col = ctx->global_col, .col = ctx->global_bg_col, .border_width = 0,
    .decal_dat = icon_shutdown_decal,
    .font_size = FONT_SMALL, .height = 31, .length = 31, .pressed_status = 0};
  
  page_dat->button_dat_2 = (j_button_data){.bg_col = ctx->global_col, .border_col = ctx->global_accent_col, .col = ctx->global_bg_col, .border_width = 0,
    .decal_dat = icon_config_decal,
    .font_size = FONT_SMALL, .height = 31, .length = 31, .pressed_status = 0};
  
  page_dat->decal_dat = J_STYLE_DECAL_DEFAULT;

  j_component* FILL = create_component("bg_col",J_FILL,0,0,&J_STYLE_COL,NULL);
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
  poll_touch(&x,&y);
  j_component* button = lcd_check_button_pressed(x,y,10);
  if(press_button_visual(button)) {
    j_button_data* b_dat = (j_button_data*)button->dat2;
    printk("%s pressed! x, y, len, hei: %d %d %d %d\n\n",button->name, button->x, button->y, b_dat->length, b_dat->height);
  }
  printk("%d, %d\n", x, y);
  return SMF_EVENT_HANDLED;
}

static void about_page_state_exit(void* o) {
  clear_draw_buffer();
}


static void home_screen_state_entry(void* o) {
  os_state_ctx* ctx = (os_state_ctx*)o;
  memset(&ctx->page_dat,0,sizeof(os_home_screen_ctx));
  os_home_screen_ctx* page_dat = &ctx->page_dat.home_screen_dat;

  page_dat->app1 = J_STYLE_BUTTON_DECAL(global_theme ? J_STYLE_ACCENT_COL : MAGENTA, J_STYLE_COL,app_whatsyapp_icon,64,64);
  page_dat->app2 = J_STYLE_BUTTON_DECAL(global_theme ? J_STYLE_ACCENT_COL : ORANGE, J_STYLE_COL,app_calc_icon,64,64);
  page_dat->app3 = J_STYLE_BUTTON_DECAL(global_theme ? J_STYLE_ACCENT_COL : GREEN, J_STYLE_COL,app_snake_icon,64,64);

  page_dat->button_dat_2 = (j_button_data){.bg_col = ctx->global_col, .border_col = ctx->global_col, .col = ctx->global_bg_col, .border_width = 0,
    .decal_dat = icon_shutdown_decal,
    .font_size = FONT_SMALL, .height = 31, .length = 31, .pressed_status = 0};
  
  page_dat->button_dat_1 = (j_button_data){.bg_col = ctx->global_col, .border_col = ctx->global_accent_col, .col = ctx->global_bg_col, .border_width = 0,
    .decal_dat = icon_config_decal,
    .font_size = FONT_SMALL, .height = 31, .length = 31, .pressed_status = 0};
  
  page_dat->banner_decal_dat = J_STYLE_DECAL_DEFAULT;

  // page_dat->ctx_menu

  j_component* FILL = create_component("bg_col",J_FILL,0,0,&J_STYLE_BG_COL,NULL);
  j_component* BANNER_DECAL = create_component("banner_decal",J_DECAL,0,0,banner_gui_decal,&page_dat->banner_decal_dat);
  j_component* PLANET_DECAL = create_component("planet_decal",J_DECAL,0,220,planet_gui_decal,&page_dat->banner_decal_dat);
  j_component* CONFIG_BUTTON = create_component("button1",J_BUTTON,10,4,"",&page_dat->button_dat_1);
  j_component* POWER_BUTTON = create_component("button2",J_BUTTON,200,4,"",&page_dat->button_dat_2);
  j_component* APP1 = create_component("whatsyapp",J_BUTTON,13,90,"",&page_dat->app3);
  j_component* APP2 = create_component("calc",J_BUTTON,88,90,"",&page_dat->app1);
  j_component* APP3 = create_component("snake",J_BUTTON,163,90,"",&page_dat->app2);

  add_component_o(FILL);
  add_component_o(BANNER_DECAL);
  add_component_o(PLANET_DECAL);
  add_component_o(CONFIG_BUTTON);
  add_component_o(POWER_BUTTON);
  add_component_o(APP1);
  add_component_o(APP2);
  add_component_o(APP3);
  draw_screen_o(NULL,0);

  printk("SOJOURN_OS // ENTERING HOME SCREEN\n");
}

static enum smf_state_result home_screen_state_run(void* o) {
  poll_touch(&x,&y);
  j_component* PRESSED_BUTTON = lcd_check_button_pressed(x,y,5);
  if(press_button_visual(PRESSED_BUTTON)) {
    k_msleep(50);
  }
  return SMF_EVENT_HANDLED;
}

static void home_screen_state_exit(void* o) {
  clear_draw_buffer();
}



void set_theme(sos_themes theme) {
  switch(theme) {
    case VOID:
      sos_object.global_col = WHITE; sos_object.global_bg_col = BLACK; sos_object.global_accent_col = WHITE;
      sos_object.confirm_col = GREEN; sos_object.error_col = RED;
      break;
    case DIAMOND:
      sos_object.global_col = WHITE; sos_object.global_bg_col = PASTEL_BLUE; sos_object.global_accent_col = DIAMOND_BLUE;
      sos_object.confirm_col = WHITE; sos_object.error_col = PINK;
      break;
    case DUST:
      sos_object.global_col = DARK_GRAY; sos_object.global_bg_col = SAND; sos_object.global_accent_col = TERRACOTTA;
      sos_object.confirm_col = TERRACOTTA; sos_object.error_col = TERRACOTTA;
      break;
    default:
      sos_object.global_col = WHITE; sos_object.global_bg_col = BLACK; sos_object.global_accent_col = WHITE;
      sos_object.confirm_col = GREEN; sos_object.error_col = RED;
      global_theme = VOID;
      return;
  }
  global_theme = theme;
}