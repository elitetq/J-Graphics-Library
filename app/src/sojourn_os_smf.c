#include "sojourn_os_smf.h"

extern const struct gpio_dt_spec dcx_gpio;
extern const struct spi_cs_control cs_ctrl;
extern const struct device *dev;
extern const struct device *dev_i2c;
extern const struct spi_config spi_cfg;
extern uint16_t bounds[4];




/*---------------------------------------------------------
                     Local Variables
----------------------------------------------------------*/
static const struct smf_state sos_states[] = {
  [HOME_SCREEN] = SMF_CREATE_STATE(home_screen_state_entry, home_screen_state_run, home_screen_state_exit, NULL, NULL),
  [ABOUT_PAGE] = SMF_CREATE_STATE(about_page_state_entry, about_page_state_run, about_page_state_exit, NULL, NULL),
  [LOCK_SCREEN] = SMF_CREATE_STATE(lock_screen_state_entry, lock_screen_state_run, lock_screen_state_exit, NULL, NULL),
  [SNAKE_GAME] = SMF_CREATE_STATE(snake_game_state_entry, snake_game_state_run, snake_game_state_exit, NULL, NULL),
};
static os_state_ctx sos_object;
static uint16_t x, y;
static sos_themes global_theme = VOID;


#define SG_COORDS_TO_INDEX(x_l, y_l) ((((x_l > 0) && (x_l <= 10)) ? x_l - 1 : 0) + ((((y_l > 0) && (y_l <= 10)) ? y_l - 1 : 0))*10)
#define SG_INDEX_TO_Y_COORD(index) ((index / 10)+1)
#define SG_INDEX_TO_X_COORD(index) ((index % 10)+1)

// Set these so it is a nice division;
#define SG_DELAY 50
#define SG_POLLING_RATE 50


void state_machine_init() {
  J_init(dev,dev_i2c,&spi_cfg,&dcx_gpio,bounds);
  J_LCD_init();
  draw_color_fs(BLACK);
  set_theme(VOID);
  sos_object.os_flags = 0b10000000;
  smf_set_initial(SMF_CTX(&sos_object),&sos_states[SNAKE_GAME]);
}

int state_machine_run() {
  return smf_run_state(SMF_CTX(&sos_object));
}


/*----------------------------------------------------------
                State Function Definitions
----------------------------------------------------------*/

static void snake_game_state_entry(void* o) {
  os_state_ctx* ctx = (os_state_ctx*)o;
  memset(&ctx->page_dat,0,sizeof(os_snake_game_ctx));
  os_snake_game_ctx* page_dat = &ctx->page_dat.lock_screen_dat;

  page_dat->bg_decal_dat1 = (j_decal_data){.animation_dat = NULL, .bg_col = DARK_GREEN, .col = GREEN};
  page_dat->pellet_decal_dat = (j_decal_data){.animation_dat = NULL, .bg_col = GREEN, .col = GREEN};
  page_dat->button_decal_left_dat = J_STYLE_BUTTON_SQUARE_MEDIUM(icon_arrow_left_decal,FONT_SMALL);
  page_dat->button_decal_right_dat = J_STYLE_BUTTON_SQUARE_MEDIUM(icon_arrow_right_decal,FONT_SMALL);
  page_dat->button_decal_up_dat = J_STYLE_BUTTON_SQUARE_MEDIUM(icon_arrow_up_decal,FONT_SMALL);
  page_dat->button_decal_down_dat = J_STYLE_BUTTON_SQUARE_MEDIUM(icon_arrow_down_decal,FONT_SMALL);
  page_dat->button_decal_right_dat.tag = 1;
  page_dat->button_decal_up_dat.tag = 2;
  page_dat->button_decal_down_dat.tag = 3;

  page_dat->shape_dat = (j_shape_data){.bg_col = J_STYLE_BG_COL, .col = WHITE, .centering = J_LEFT, .height = 19, .length = 19, .type = J_RECTANGLE};


  page_dat->x_board_offset = page_dat->y_board_offset = 20;
  page_dat->snake_shape_comp_dat = create_component("snake_shape",J_SHAPE,0,0,&page_dat->shape_dat,NULL);
  j_component* bg_fill = create_component("bg_col",J_FILL,0,0,&J_STYLE_BG_COL,NULL);
  page_dat->bg_comp_dat = create_component("snake_background",J_DECAL,page_dat->x_board_offset,page_dat->y_board_offset,snake_app_foreground_1_decal,&page_dat->bg_decal_dat1);
  page_dat->button_comp_left = create_component("button_left",J_BUTTON,49,244,"",&page_dat->button_decal_left_dat);
  page_dat->button_comp_right = create_component("button_right",J_BUTTON,147,244,"",&page_dat->button_decal_right_dat);
  page_dat->button_comp_up = create_component("button_up",J_BUTTON,98,224,"",&page_dat->button_decal_up_dat);
  page_dat->button_comp_down = create_component("button_down",J_BUTTON,98,273,"",&page_dat->button_decal_down_dat);
  page_dat->pellet_comp_dat = create_component("pellet",J_DECAL,0,0,icon_sg_pellet_decal,&page_dat->pellet_decal_dat);



  sg_init(page_dat);

  add_component_o(bg_fill);
  add_component_o(page_dat->bg_comp_dat);
  add_component_o(page_dat->button_comp_left);
  add_component_o(page_dat->button_comp_right);
  add_component_o(page_dat->button_comp_up);
  add_component_o(page_dat->button_comp_down);
  draw_screen_o(NULL, 0);
  k_msleep(1000);
  draw_screen_o(NULL, 0);
  printk("Entered snake!\n");
}

/*----------------------------------------------------------
                Snake Game Functions
----------------------------------------------------------*/

void sg_draw_coords(os_snake_game_ctx* dat_ptr, uint8_t *x_l, uint8_t *y_l, sg_tile tile_info) {
  if(*x_l < 1 || *x_l > 10 || *y_l < 1 || *y_l > 10) return;
  uint16_t x_r, y_r;
  x_r = dat_ptr->x_board_offset + 20*(*x_l-1);
  y_r = dat_ptr->y_board_offset + 20*(*y_l-1);
  j_component* comp;
  if(tile_info == SG_FILL) {
    comp = dat_ptr->snake_shape_comp_dat;
  } else {
    comp = dat_ptr->pellet_comp_dat;
    dat_ptr->pellet_decal_dat.col = (x_r + y_r) ? GREEN : DARK_GREEN;
  }
  dat_ptr->game_array[SG_COORDS_TO_INDEX(*x_l,*y_l)] = tile_info;
  comp->x = x_r; comp->y = y_r;
  draw_component(comp);
}

void sg_clear_coords(os_snake_game_ctx* dat_ptr, uint8_t *x_l, uint8_t *y_l) {
  if(*x_l < 1 || *x_l > 10 || *y_l < 1 || *y_l > 10) return;
  uint16_t x_r, y_r;
  j_component* comp = dat_ptr->snake_shape_comp_dat;
  j_shape_data* shape_dat = (j_shape_data*)comp->dat;

  x_r = dat_ptr->x_board_offset + 20*(*x_l-1); // Find local x and y coordinates
  y_r = dat_ptr->y_board_offset + 20*(*y_l-1);

  dat_ptr->game_array[SG_COORDS_TO_INDEX(*x_l,*y_l)] = SG_NFILL;
  j_color old_col = shape_dat->col;
  shape_dat->col = ((*x_l + *y_l) % 2) ? DARK_GREEN : GREEN;
  comp->x = x_r; comp->y = y_r;
  draw_component(comp);
  shape_dat->col = old_col;
}

void sg_generate_pellet(os_snake_game_ctx* dat_ptr) {
  j_color pellet_colors[4] = {BABY_BLUE,RED,MAGENTA,YELLOW};
  uint8_t pellet_index = sys_rand8_get();
  pellet_index = pellet_index % 100;
  printk("%d\n",pellet_index);
  dat_ptr->pellet_decal_dat.bg_col = pellet_colors[pellet_index%4]; // Randomize pellet colors

  uint8_t local_index = 0;
  uint8_t *array = dat_ptr->game_array;
  uint8_t local_i = 0;
  bool skipped = false;
  for(int i = 0; i <= pellet_index; i++) {
    local_i%=100;
    local_i++;
    while(array[local_i]) {
      local_i++;
      local_i%=100;
    }
  }
  while(dat_ptr->game_array[local_i]) {
    printk("FAILED\n");
  }
  dat_ptr->temp_pellet_x = SG_INDEX_TO_X_COORD(local_i);
  dat_ptr->temp_pellet_y = SG_INDEX_TO_Y_COORD(local_i);
  // sg_draw_coords(dat_ptr,&dat_ptr->temp_pellet_x,&dat_ptr->temp_pellet_y,SG_FOOD);
  sg_draw_coords(dat_ptr,&dat_ptr->temp_pellet_x,&dat_ptr->temp_pellet_y,SG_FOOD);
  dat_ptr->pellet_amt++;
}

void sg_init(os_snake_game_ctx* dat_ptr) {
  for(int i = 0; i < 100; i++) {
    dat_ptr->game_array[i] = 0;
    dat_ptr->dir_array[i] = 0;
  }
  dat_ptr->cur_x = dat_ptr->cur_y = dat_ptr->cur_yf = 5;
  dat_ptr->cur_xf = 4;
  dat_ptr->snake_len = 2;
  dat_ptr->user_score = 0;
  dat_ptr->cur_dir_index = 1;
  dat_ptr->dir_array[0] = SG_RIGHT;
  dat_ptr->next_dir = SG_RIGHT;
  dat_ptr->DELAY_PER_CPU_CYCLE = 5;
  dat_ptr->pellet_amt = 0;
}

void sg_update_coords(uint8_t *x_l, uint8_t *y_l, sg_directions dir) {
  if(dir <= 0) {
    return;
  }
  *x_l += dir <= 2 ? -1 + 2*(dir-1) : 0;
  *y_l += dir > 2 ? -1 + 2*(dir-3) : 0;
}

int sg_update_pos(os_snake_game_ctx* dat_ptr, sg_directions dir, bool increase_size) {
  uint8_t *x_l, *y_l, *x_fl, *y_fl; // Set up local x and y, as well as the snake tail x and y.
  x_fl = &dat_ptr->cur_xf;
  y_fl = &dat_ptr->cur_yf;
  x_l = &dat_ptr->cur_x;
  y_l = &dat_ptr->cur_y;
  uint8_t *index = &dat_ptr->cur_dir_index;
  int follow_index = (int)*index - dat_ptr->snake_len + 1;
  if(follow_index < 0) follow_index = 100 + follow_index; // Handle end of array, array loops back into itself
  dat_ptr->dir_array[(*index)++] = dir;
  *index %= 100;
  follow_index %= 100;

  sg_update_coords(x_l,y_l,dir);

  // Check if on same tile as pellet
  if(dat_ptr->game_array[SG_COORDS_TO_INDEX(*x_l,*y_l)] == SG_FOOD) {
    increase_size = true;
    sg_generate_pellet(dat_ptr);
  }

  sg_draw_coords(dat_ptr,x_l,y_l,SG_FILL);

  if(increase_size) { // Increase the length of the snake
    dat_ptr->snake_len++;
    increase_size = 0;
    return;
  }
  sg_directions follow_dir = dat_ptr->dir_array[follow_index];
  printk("Follow dir: %d\n", follow_index);
  if(!(*x_l == *x_fl && *y_l == *y_fl)) {
    sg_clear_coords(dat_ptr,x_fl,y_fl);
  }
  printk("SG_COORDS: %d %d\n\n", *x_fl, *y_fl);
  sg_update_coords(x_fl,y_fl,follow_dir);
  return 0;
}


static enum smf_state_result snake_game_state_run(void* o) {
  os_state_ctx* ctx = (os_state_ctx*)o;
  os_snake_game_ctx* page_dat = &ctx->page_dat.snake_game_dat;
  static sg_directions cur_dir = SG_RIGHT;
  static bool increase = true;
  static int p = 0;
  sg_update_pos(page_dat,cur_dir,increase);
  for(int m = 0; m < 10; m++) {
    for(int u = 0; u < 10; u++) {
      printk("%d ",page_dat->game_array[u + m*10]);
    }
    printk("\n");
  }
  p++;
  if(p > 4) increase = false;
  if(increase) {
    sg_generate_pellet(page_dat);
  }

  int ret_timeout;

  for(int k = 0; k < SG_POLLING_RATE; k++) {
    ret_timeout = poll_touch_timeout(&x,&y,page_dat->DELAY_PER_CPU_CYCLE);
    k_msleep(page_dat->DELAY_PER_CPU_CYCLE - ret_timeout);
    j_component* PRESSED_BUTTON = lcd_check_button_pressed(x,y,2);
    if(PRESSED_BUTTON != NULL) {
      j_button_data* button_dat = (j_button_data*)PRESSED_BUTTON->dat2;
      if(button_dat != NULL) {
        if(1) {
          switch(button_dat->tag) {
            case 0: // Left
              cur_dir = SG_LEFT;
              break;
            case 1: // Right
              cur_dir = SG_RIGHT;
              break;
            case 2: // Up
              cur_dir = SG_UP;
              break;
            default: // Down
              cur_dir = SG_DOWN;
              break;
          }
        } else {

        }
      }
    }
  }
  return SMF_EVENT_HANDLED;
}

static void snake_game_state_exit(void* o) {
  clear_draw_buffer();

}

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
  add_component_o(page_dat->shape);
  add_component_o(page_dat->message_comp_ptr);
  // free_component(page_dat->shape);
  // free_component(page_dat->message_comp_ptr);
  clear_draw_buffer();
  printk("EXITING LOCK SCREEN\n\n");
}

static void about_page_state_entry(void* o) {
  printk("Entering about page...");
  os_state_ctx* ctx = (os_state_ctx*)o;
  memset(&ctx->page_dat,0,sizeof(os_about_page_ctx));
  os_about_page_ctx* page_dat = &ctx->page_dat.about_page_dat;
  
  page_dat->button_dat_1 = (j_button_data){.bg_col = ctx->global_col, .border_col = ctx->global_accent_col, .col = ctx->global_bg_col, .border_width = 0,
    .decal_dat = icon_arrow_left_tail_decal,
    .font_size = FONT_SMALL, .height = 31, .length = 31, .pressed_status = 0, .tag = 1};

  page_dat->text_dat = J_STYLE_TEXT_DEFAULT(J_CENTER,FONT_SMALL);

  page_dat->decal_dat = J_STYLE_DECAL_DEFAULT;

  j_component* FILL = create_component("bg_col",J_FILL,0,0,&J_STYLE_BG_COL,NULL);
  j_component* BANNER_DECAL = create_component("banner_decal",J_DECAL,0,0,banner_gui_decal,NULL);
  j_component* PLANET_DECAL = create_component("planet_decal",J_DECAL,0,220,planet_gui_decal,NULL);
  j_component* SOJOURN_DECAL = create_component("sojourn_logo",J_DECAL,30,100,logo_text_gui_decal,&page_dat->decal_dat);
  j_component* BACK_BUTTON = create_component("back_button",J_BUTTON,10,4,"",&page_dat->button_dat_1);
  j_component* TEXT = create_component("credit text1",J_TEXT,120,160,"Everything you see is",&page_dat->text_dat);
  j_component* TEXT2 = create_component("credit text1",J_TEXT,120,175,"created by Jonart Bajraktari",&page_dat->text_dat);
  j_component* TEXT3 = create_component("credit text1",J_TEXT,120,205,"for Embedded in Embedded 2026",&page_dat->text_dat);

  add_component_o(FILL);
  add_component_o(BANNER_DECAL);
  add_component_o(PLANET_DECAL);
  add_component_o(SOJOURN_DECAL);
  add_component_o(BACK_BUTTON);
  add_component_o(TEXT);
  add_component_o(TEXT2);
  add_component_o(TEXT3);
  draw_screen_o(NULL,0);
}

static enum smf_state_result about_page_state_run(void* o) {
  poll_touch(&x,&y);
  j_component* button = lcd_check_button_pressed_filter(x,y,10,1);
  if(press_button_visual(button)) {
    smf_set_state(SMF_CTX(&sos_object),&sos_states[HOME_SCREEN]);
  }
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

  // Context menu data
  page_dat->ctx_menu_dat = (j_button_data){.bg_col = J_STYLE_BG_COL, .border_col = J_STYLE_ACCENT_COL, .col = J_STYLE_BG_COL, .border_width = 2, .decal_dat = NULL, .font_size = FONT_SMALL, .height = 80, .length = 140, .pressed_status = 0, .tag = 0};
  page_dat->ctx_menu_button_dat = (j_button_data){.bg_col = J_STYLE_BG_COL, .border_col = J_STYLE_ACCENT_COL, .col = J_STYLE_COL, .border_width = 2, .decal_dat = NULL, .font_size = FONT_MEDIUM, .height = 40, .length = 140, .pressed_status = 0, .tag = 1};

  page_dat->ctx_menu_background = create_component("ctx_bg",J_BUTTON,10,35,"",&page_dat->ctx_menu_dat);
  page_dat->ctx_button1 = create_component("ctx_themes_but",J_BUTTON,10,35,"",&page_dat->ctx_menu_button_dat);
  page_dat->ctx_button2 = create_component("ctx_about_but",J_BUTTON,10,35+page_dat->ctx_menu_button_dat.height,"",&page_dat->ctx_menu_button_dat);

  page_dat->ctx_pressed = 0;
  page_dat->ctx_offset = 0;

  page_dat->button_dat_2 = (j_button_data){.bg_col = ctx->global_col, .border_col = ctx->global_col, .col = ctx->global_bg_col, .border_width = 0,
    .decal_dat = icon_shutdown_decal,
    .font_size = FONT_SMALL, .height = 31, .length = 31, .pressed_status = 0, .tag = 2};
  
  page_dat->button_dat_1 = (j_button_data){.bg_col = ctx->global_col, .border_col = ctx->global_accent_col, .col = ctx->global_bg_col, .border_width = 0,
    .decal_dat = icon_config_decal,
    .font_size = FONT_SMALL, .height = 31, .length = 31, .pressed_status = 0, .tag = 3};
  
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
  // smf_set_state(SMF_CTX(&sos_object),&sos_states[ABOUT_PAGE]);

  printk("SOJOURN_OS // ENTERING HOME SCREEN\n");
}

static enum smf_state_result home_screen_state_run(void* o) {
  static char* ctx_strings[4] = {"Themes","About","Shut down","Sign out"};
  os_state_ctx* ctx = (os_state_ctx*)o;
  os_home_screen_ctx* page_dat = &ctx->page_dat.home_screen_dat;
  poll_touch(&x,&y);
  j_component* PRESSED_BUTTON = lcd_check_button_pressed_filter(x,y,5,page_dat->ctx_pressed ? 1 : -1);

  if(press_button_visual(PRESSED_BUTTON)) {
    j_button_data* button_dat = (j_button_data*)PRESSED_BUTTON->dat2;
    switch(button_dat->tag) {
      case 0: // Pressed apps
      printk("Pressed ctx\n");
        break;
      case 1: // Pressed ctx
      sos_smf_states state_array[4] = {THEME_PAGE,ABOUT_PAGE,SHUT_DOWN,RESTART};
      printk("Pressed power/config\n");
      smf_set_state(SMF_CTX(&sos_object),&sos_states[state_array[(PRESSED_BUTTON == page_dat->ctx_button2) + page_dat->ctx_offset]]);
      return SMF_EVENT_HANDLED;

        break;
      default: // Pressed power/config
        j_button_data* ctx_menu_bg_dat = (j_button_data*)page_dat->ctx_menu_background->dat2;
        j_button_data* pressed_button_dat = (j_button_data*)PRESSED_BUTTON->dat2;
        page_dat->ctx_menu_background->x = PRESSED_BUTTON->x + ctx_menu_bg_dat->length > LCD_MAX_HEIGHT ? 
          PRESSED_BUTTON->x + pressed_button_dat->length - ctx_menu_bg_dat->length :
          PRESSED_BUTTON->x;
        page_dat->ctx_button1->x = page_dat->ctx_menu_background->x;
        page_dat->ctx_button2->x = page_dat->ctx_menu_background->x;
        // Set content of CTX buttons
        if(button_dat->tag - 2) {
          page_dat->ctx_button1->dat = (void*)ctx_strings[0];
          page_dat->ctx_button2->dat = (void*)ctx_strings[1];
          page_dat->ctx_offset = 0;
        } else {
          page_dat->ctx_button1->dat = (void*)ctx_strings[2];
          page_dat->ctx_button2->dat = (void*)ctx_strings[3];
          page_dat->ctx_offset = 2;
        }
        
        add_component_o(page_dat->ctx_button1);
        add_component_o(page_dat->ctx_button2);

        draw_component(page_dat->ctx_menu_background);
        draw_component(page_dat->ctx_button1);
        draw_component(page_dat->ctx_button2);
        
        page_dat->ctx_pressed = 1;
        break;
    }
    k_msleep(50);
  } else if(page_dat->ctx_pressed) {
    remove_component_o(page_dat->ctx_button1);
    remove_component_o(page_dat->ctx_button2);
    page_dat->ctx_pressed = 0;
    draw_screen_o(NULL,0);
  }
  return SMF_EVENT_HANDLED;
}

static void home_screen_state_exit(void* o) {
  os_state_ctx* ctx = (os_state_ctx*)o;
  os_home_screen_ctx* page_dat = &ctx->page_dat.home_screen_dat;
  printk("\n\nPointer before %p\n",page_dat->ctx_button1->dat2);
  add_component_o(page_dat->ctx_menu_background);
  add_component_o(page_dat->ctx_button1);
  add_component_o(page_dat->ctx_button2);
  clear_draw_buffer();
  printk("All good!");
  printk("Exiting home...\n\n\n");
}



void set_theme(sos_themes theme) {
  switch(theme) {
    case VOID:
      sos_object.global_col = WHITE; sos_object.global_bg_col = BLACK; sos_object.global_accent_col = GRAY;
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