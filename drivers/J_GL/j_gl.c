#include "J_GL.h"
/*----------------------------------------------------------
                Global Zephyr Variables Struct
----------------------------------------------------------*/
static struct J_CONTAINER J_CONTAINER_t = {0};

/*----------------------------------------------------------
                          Buffers
----------------------------------------------------------*/
// Here I use two different buffers, the first buffer is mainly used for all
// the regular draws, but the second (color_data_2) is an aux buffer used in
// the ram draw function to bounce between the two buffers for maximum refresh
// speed during large arrays like images.
static volatile uint8_t color_data[(LCD_MAX_LENGTH+1)*(LCD_MAX_HEIGHT+1)*3/LCD_BUF_DIV];
static volatile uint8_t color_data_2[RAM_DATA_SIZE];

static volatile uint8_t font_buf[16*26*3];
static struct spi_buf color_data_buf = {};
static struct spi_buf_set color_data_set = {.buffers = &color_data_buf, .count = 1};
static uint16_t bound_buff[4] = {0,0,0,0};

#define IS_POINT_IN_BOX(x, y, x_l, y_l, x_len, y_len) ((x > x_l) && (y > y_l) && (x < (x_l + x_len)) && (y < (y_l + y_len)))
#define IS_POINT_IN_BOX_BUFFER(x, y, x_l, y_l, x_len, y_len, buffer) ((x > (x_l-buffer)) && (y > (y_l-buffer)) && (x < (x_l + x_len + buffer)) && (y < (y_l + y_len + buffer)))

/*----------------------------------------------------------
                Component and Context Structs
----------------------------------------------------------*/
static j_linked_list comp_linked_list = {.start = NULL, .end = NULL};

static j_spi_ctx J_CTX1 = {color_data_2,color_data,0x00}; // Used in ram_draw function

/*----------------------------------------------------------
                    Shape Draw Handlers
----------------------------------------------------------*/

void shape_draw_rectangle(j_component* comp, j_shape_data* shape_dat, j_animation_data* anim_dat) {
  j_centering CENTERING = shape_dat->centering;
  uint16_t x, y, x2, y2;
  x = comp->x - (CENTERING == J_LEFT ? 0 : shape_dat->length / (3-CENTERING));
  y = comp->y - (CENTERING == J_CENTER ? shape_dat->height/2 : 0);
  x2 = x+shape_dat->length;
  y2 = y+shape_dat->height;
  if(x > LCD_MAX_HEIGHT) return;
  if(x2 > LCD_MAX_HEIGHT) x2 = LCD_MAX_HEIGHT;
  if(y2 > LCD_MAX_LENGTH) y2 = LCD_MAX_LENGTH;
  set_bounds((uint16_t[]){x,x2,y,y2});
  cmd_bounds();

  if(anim_dat != NULL) { // Animations here
    uint8_t percentage = anim_dat->percentage;
    uint8_t increment = anim_dat->increment_speed ? anim_dat->increment_speed : 1;
    if(anim_dat->type <= FADE_OUT) { // Fade in / Fade out
      uint8_t bg_col_B, bg_col_G, bg_col_R, col_B, col_G, col_R;
      j_color draw_col, shape_col, bg_col;

      bg_col = anim_dat->type ? shape_dat->col : shape_dat->bg_col;
      shape_col = anim_dat->type ? shape_dat->bg_col : shape_dat->col;
      
      col_B = shape_col;
      bg_col_B = bg_col;
      col_G = shape_col >> 8;
      bg_col_G = bg_col >> 8;
      col_R = shape_col >> 16;
      bg_col_R = bg_col >> 16;
      while(percentage <= 100) {
        draw_col = (percentage * col_B + (100 - percentage) * bg_col_B)/100 + 
                  ((percentage * col_G + (100 - percentage) * bg_col_G)/100 << 8) +
                  ((percentage * col_R + (100 - percentage) * bg_col_R)/100 << 16);
        printk("%d\n",draw_col);
        draw_color_fs(draw_col);
        k_msleep(101 - increment);
        percentage += 1;
      }
      draw_color_fs(shape_dat->col);
      k_msleep(200);
      return;
    } else { // Enlarge / Shrink
      draw_color_fs(shape_dat->col);
      uint16_t x_l, y_l, x2_l, y2_l, prev_x_l, prev_y_l, prev_x2_l, prev_y2_l, temp;
      prev_x_l = x;
      prev_y_l = y;
      prev_x2_l = x2;
      prev_y2_l = y2;
      while(1) {
        x_l = (x*(100-percentage) + anim_dat->x_low*(percentage))/100;
        y_l = (y*(100-percentage) + anim_dat->y_low*(percentage))/100;
        x2_l = (x2*(100-percentage) + anim_dat->x_high*(percentage))/100;
        y2_l = (y2*(100-percentage) + anim_dat->y_high*(percentage))/100;

        set_bounds((uint16_t[]){x_l,prev_x_l,prev_y_l,prev_y2_l}); // Left side
        cmd_bounds();
        prev_x_l > x_l ? draw_color_fs(shape_dat->col) : draw_color_fs(shape_dat->bg_col);
        

        set_bounds((uint16_t[]){prev_x2_l,x2_l,prev_y_l,prev_y2_l}); // Right side
        cmd_bounds();
        prev_x2_l < x2_l ? draw_color_fs(shape_dat->col) : draw_color_fs(shape_dat->bg_col);

        set_bounds((uint16_t[]){x_l,x2_l,y_l,prev_y_l}); // Top side
        cmd_bounds();
        prev_y_l > y_l ? draw_color_fs(shape_dat->col) : draw_color_fs(shape_dat->bg_col);

        set_bounds((uint16_t[]){x_l,x2_l,prev_y2_l,y2_l}); // Bottom side
        cmd_bounds();
        prev_y2_l < y2_l ? draw_color_fs(shape_dat->col) : draw_color_fs(shape_dat->bg_col);

        prev_x_l = x_l;
        prev_y_l = y_l;
        prev_x2_l = x2_l;
        prev_y2_l = y2_l;

        k_msleep(10);
        percentage += increment;
        if(percentage > 100) {
          if(100 % increment == 0) break;
          increment = 1;
          percentage = 100;
        }
      }
      k_msleep(10);
      return;
    }
  }
  
  draw_color_fs(shape_dat->col);
}

void shape_draw_circle(j_component* comp, j_shape_data* shape_dat, j_animation_data* anim_dat) {
  return;
}

void shape_draw_triangle(j_component* comp, j_shape_data* shape_dat, j_animation_data* anim_dat) {
  return;
}

void (*shape_draw_funcs[])(j_component*, j_shape_data*, j_animation_data*) = {
  shape_draw_rectangle,
  shape_draw_circle,
  shape_draw_triangle,
  shape_draw_triangle,
  shape_draw_triangle
};

/*----------------------------------------------------------
                        Draw Handlers
----------------------------------------------------------*/
void draw_handle_button(j_component* comp) {
  j_button_data button_default = {.border_width = 1, .bg_col = WHITE, .border_col = GRAY, .col = BLACK, .height = 30, .length = 80, .font_size = FONT_MEDIUM, .pressed_status = 0};
  j_button_data* button_dat = comp->dat2 == NULL ? &button_default : (j_button_data*)comp->dat2;
  uint8_t* decal_dat = button_dat->decal_dat;
  uint8_t len = 0;
  char* str = (char*)comp->dat;
  while(str[len]) {
    len++;
  }
  uint16_t x_len, y_len; // Get x and y lengths
  x_len = comp->x + button_dat->length > LCD_MAX_HEIGHT ? LCD_MAX_HEIGHT - comp->x : button_dat->length;
  y_len = comp->y + button_dat->height > LCD_MAX_LENGTH ? LCD_MAX_LENGTH - comp->y : button_dat->height;


  set_bounds((uint16_t[]){
    comp->x,
    comp->x+x_len,
    comp->y,
    comp->y+y_len
  });
  cmd_bounds();
  draw_color_fs(button_dat->border_col);

  set_bounds((uint16_t[]){
    comp->x+button_dat->border_width,
    comp->x+x_len-button_dat->border_width,
    comp->y+button_dat->border_width,
    comp->y+y_len-button_dat->border_width
  });
  cmd_bounds();
  draw_color_fs(button_dat->pressed_status ? button_dat->col : button_dat->bg_col);

  if(decal_dat == NULL) {
    draw_text(
      comp->x+((x_len)/2)-((len*(j_fonts_x_len[button_dat->font_size]+TEXT_KERNING))/2),
      comp->y+((y_len*11)/20)-j_fonts_y_len[button_dat->font_size]/2,
      str,
      button_dat->font_size,
      button_dat->pressed_status ? button_dat->bg_col : button_dat->col,
      button_dat->pressed_status ? button_dat->col : button_dat->bg_col
    );
  } else {
    j_decal_data decal_specific_dat = {
      .animation_dat = NULL,
      .bg_col = button_dat->pressed_status ? button_dat->bg_col : button_dat->col,
      .col = button_dat->pressed_status ? button_dat->col : button_dat->bg_col};

    uint16_t decal_x_len = (decal_dat[0] << 8) | decal_dat[1];
    uint16_t decal_y_len = (decal_dat[2] << 8) | decal_dat[3];
    j_component* decal = create_component("",J_DECAL,comp->x + (button_dat->length)/2 - decal_x_len/2,comp->y + (button_dat->height)/2 - decal_y_len/2,decal_dat,&decal_specific_dat);
    ram_draw_image_helper(decal->x, decal->y, decal, NULL);
  }
}

void draw_handle_shape(j_component* comp) {
  j_shape_data* shape_dat = (j_shape_data*)comp->dat;
  j_animation_data* anim_dat = (j_animation_data*)comp->dat2;
  shape_draw_funcs[shape_dat->type](comp,shape_dat,anim_dat);
}

void draw_handle_text(j_component* comp) {
  char* text_str = (char*)(comp->dat);
  uint16_t len = 0;
  while(text_str[len]) len++;
  j_text_data text_default = {.font_size = FONT_MEDIUM, .col = BLACK, .bg_col = WHITE, .centering = J_LEFT};
  j_text_data* text_dat = comp->dat2 == NULL ? &text_default : (j_text_data*)comp->dat2;
  uint16_t x_l = comp->x;
  if(text_dat->centering > J_LEFT) {
    uint16_t shift =  (len*j_fonts_x_len[text_dat->font_size])/(text_dat->centering - 1 ? 1 : 2);
    x_l -= shift > x_l ? x_l : shift;
  }
  draw_text(x_l,comp->y,text_str,text_dat->font_size,text_dat->col,text_dat->bg_col);
}

void draw_handle_image(j_component* comp) {
  j_animation_data* anim_dat = NULL;
  if(comp->type == J_IMAGE) {
    if(comp->dat2 != NULL) anim_dat = (j_animation_data*)comp->dat2;
  } else {
    j_decal_data* decal_dat = (j_decal_data*)comp->dat2;
    if(decal_dat != NULL) anim_dat = decal_dat->animation_dat;
  }
  ram_draw_image_helper(comp->x, comp->y, comp, anim_dat);
}

// dat is a uint8_t value from 0 to 100
// dat2 is a j_bar_data value
void draw_handle_bar(j_component* comp) {
  uint8_t* val = (uint8_t*)(comp->dat);
  j_bar_data default_data = {.bg_col = WHITE, .col = GREEN, .height = 15, .length = 100, .type = J_BAR_LR};
  j_bar_data* bar_dat = comp->dat2 == NULL ? &default_data : (j_bar_data*)(comp->dat2);

  uint16_t x_len, y_len, len_fill; // Get x and y lengths
  x_len = comp->x + bar_dat->length > LCD_MAX_HEIGHT ? LCD_MAX_HEIGHT - comp->x : bar_dat->length;
  y_len = comp->y + bar_dat->height > LCD_MAX_LENGTH ? LCD_MAX_LENGTH - comp->y : bar_dat->height;


  if(bar_dat->type <= J_BAR_RL) {
    len_fill = (x_len * *val)/100;
    uint16_t delta = x_len - len_fill;
    set_bounds((uint16_t[]){
      comp->x + len_fill - len_fill*bar_dat->type,
      comp->x + x_len - len_fill*bar_dat->type,
      comp->y,
      comp->y + y_len
    });
    cmd_bounds();
    draw_color_fs(bar_dat->bg_col);

    set_bounds((uint16_t[]){
      comp->x + (x_len - len_fill)*bar_dat->type,
      comp->x + len_fill + (x_len - len_fill)*bar_dat->type,
      comp->y,
      comp->y + y_len
    });
    cmd_bounds();
  } else {
    len_fill = (y_len * *val)/100;
    uint16_t delta = y_len - len_fill;
    set_bounds((uint16_t[]){
      comp->x,
      comp->x + x_len,
      comp->y + len_fill - len_fill*bar_dat->type,
      comp->y + y_len - len_fill*bar_dat->type
    });
    cmd_bounds();
    draw_color_fs(bar_dat->bg_col);

    set_bounds((uint16_t[]){
      comp->x,
      comp->x + x_len,
      comp->y + len_fill + (y_len - len_fill)*(bar_dat->type - 2)
    });
    cmd_bounds();
  }
  if(len_fill) draw_color_fs(bar_dat->col);
}

// dat is a j_color pointer
void draw_handle_fill(j_component* comp) {
  set_bounds((uint16_t[]){0,LCD_MAX_HEIGHT,0,LCD_MAX_LENGTH});
  cmd_bounds();
  j_color* COL = (j_color*)(comp->dat);
  draw_color_fs(*COL);
}


void (*draw_handle_funcs[])(j_component*) = {
  draw_handle_button,
  draw_handle_shape,
  draw_handle_text,
  draw_handle_image,
  draw_handle_bar,
  draw_handle_fill,
  draw_handle_image
};


/*----------------------------------------------------------
                    Function Definitions
----------------------------------------------------------*/

void J_init(const struct device* dev_spi, const struct device* dev_i2c, const struct spi_config* spi_cfg, const struct gpio_dt_spec* dcx_gpio, uint16_t* bounds) {
  J_CONTAINER_t.dev_spi = dev_spi;
  J_CONTAINER_t.dev_i2c = dev_i2c;
  J_CONTAINER_t.spi_cfg = spi_cfg;
  J_CONTAINER_t.dcx_gpio = dcx_gpio;
  J_CONTAINER_t.bounds = bounds;
  size_t top_bound = RAM_DATA_SIZE;
  for(size_t i = 0; i < top_bound; i++) {
    color_data_2[i] = 0xFF;
  }
}

void J_LCD_init() {
  lcd_cmd(CMD_SOFTWARE_RESET,NULL);
  k_msleep(120);
  lcd_cmd(CMD_SLEEP_OUT,NULL);
  lcd_cmd(CMD_DISPLAY_ON,NULL);
  set_bounds((uint16_t[]){0,LCD_MAX_HEIGHT,0,LCD_MAX_LENGTH});
}

void cmd_bounds() {
  struct spi_buf column_data = {
    .buf = (uint8_t[]){(J_CONTAINER_t.bounds[0]>>8),J_CONTAINER_t.bounds[0],(J_CONTAINER_t.bounds[1]>>8),J_CONTAINER_t.bounds[1]},
    .len = 4
  };
  struct spi_buf row_data = {
    .buf = (uint8_t[]){(J_CONTAINER_t.bounds[2]>>8),J_CONTAINER_t.bounds[2],(J_CONTAINER_t.bounds[3]>>8),J_CONTAINER_t.bounds[3]},
    .len = 4
  };
  lcd_cmd(CMD_ROW_ADDRESS_SET,&row_data);
  lcd_cmd(CMD_COLUMN_ADDRESS_SET,&column_data);
}

uint16_t cnv_float(float num) {
  if(num > 65535.0)
    return 0xFFFF;

  if(num < 0)
    return (uint16_t)(-1*num);

  return (uint16_t)num;
}

uint32_t get_pos() {
    uint16_t x_pos, y_pos;
    uint8_t x_pos_l, y_pos_l, x_pos_h, y_pos_h;
    touch_control_cmd_rsp(P1_XL,&x_pos_l);
    touch_control_cmd_rsp(P1_YL,&y_pos_l);
    touch_control_cmd_rsp(P1_XH,&x_pos_h);
    touch_control_cmd_rsp(P1_YH,&y_pos_h);
    x_pos = LCD_MAX_HEIGHT - (uint16_t)(((x_pos_h & TOUCH_POS_MSB_MASK) << 8) + x_pos_l);
    y_pos = LCD_MAX_LENGTH - (uint16_t)(((y_pos_h & TOUCH_POS_MSB_MASK) << 8) + y_pos_l);
    if(y_pos > LCD_MAX_LENGTH) {
      y_pos = LCD_MAX_LENGTH;
    }
    if(x_pos > LCD_MAX_HEIGHT) {
      x_pos = LCD_MAX_HEIGHT;
    }
    printk("%d %d\n", x_pos, y_pos);
    return ((uint32_t)(x_pos) << 16) + (uint32_t)(y_pos);
}

int draw_color_fs(j_color COLOR) {
  cmd_bounds();
  k_msleep(5);

  size_t buf_len = (J_CONTAINER_t.bounds[1]-J_CONTAINER_t.bounds[0]+1)*(J_CONTAINER_t.bounds[3]-J_CONTAINER_t.bounds[2]+1);
  for(int i = 0; i < buf_len*3/LCD_BUF_DIV; i += 3) {
    color_data[i] = COLOR; // Blue
    color_data[i+1] = COLOR >> 8; // Green
    color_data[i+2] = COLOR >> 16; // Red
  }
  struct spi_buf color_data_buf = {.buf = color_data, .len = buf_len/LCD_BUF_DIV * 3};
  struct spi_buf_set color_data_set = {.buffers = &color_data_buf, .count = 1};
  lcd_cmd(CMD_MEMORY_WRITE,NULL);
  
  gpio_pin_set_dt(J_CONTAINER_t.dcx_gpio,1);
  for(int i = 0; i < LCD_BUF_DIV; i++)
    spi_write(J_CONTAINER_t.dev_spi,J_CONTAINER_t.spi_cfg,&color_data_set);
  return 0;
}

void set_bounds(uint16_t* user_list) {
  // xlo, xhi, ylo, yhi -> ylo, yhi, xlo, xhi
  uint16_t temp;
  if(user_list[0] > user_list[1]) {
    temp = user_list[0];
    user_list[0] = user_list[1];
    user_list[1] = temp;
  }
  if(user_list[2] > user_list[3]) {
    temp = user_list[2];
    user_list[2] = user_list[3];
    user_list[3] = temp;
  }
  uint16_t cnv_list[4];
  cnv_list[3] = user_list[3];
  cnv_list[2] = user_list[2];
  cnv_list[0] = LCD_MAX_HEIGHT - user_list[1];
  cnv_list[1] = LCD_MAX_HEIGHT - user_list[0];
  for(int i = 0; i < 4; i++) {
    J_CONTAINER_t.bounds[i] = cnv_list[i];
  }
}


void touch_control_cmd_rsp(uint8_t cmd, uint8_t* rsp) {
  struct i2c_msg cmd_rsp_msg[2] = {
    {&cmd, 1, I2C_MSG_WRITE},
    {rsp, 1, I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP}
  };
  i2c_transfer(J_CONTAINER_t.dev_i2c,cmd_rsp_msg,2,TD_ADDR);
}

void lcd_cmd(uint8_t cmd, struct spi_buf * data) {
  struct spi_buf cmd_buf[1] = {[0] = {
    .buf=&cmd,
    .len=1}
  };
  struct spi_buf_set cmd_set = {
    .buffers=cmd_buf,
    .count=1,
  };
  
  gpio_pin_set_dt(J_CONTAINER_t.dcx_gpio,0); // Set pin to low to let SPI know its a command being sent
  spi_write(J_CONTAINER_t.dev_spi,J_CONTAINER_t.spi_cfg,&cmd_set);

  if(data != NULL) {
    struct spi_buf_set data_set = {data,1};
    gpio_pin_set_dt(J_CONTAINER_t.dcx_gpio,1); // Set pin to high to let SPI know its data being sent
    spi_write(J_CONTAINER_t.dev_spi,J_CONTAINER_t.spi_cfg,&data_set);
  }
}

void draw_square(uint16_t x, uint16_t y, uint16_t size) {
  bound_buff[2] = y > size ? y - size : 0;
  bound_buff[3] = (y + size) < LCD_MAX_LENGTH ? y + size : LCD_MAX_LENGTH;
  bound_buff[0] = x > size ? x - size : 0;
  bound_buff[1] = (x + size) < LCD_MAX_HEIGHT ? x + size : LCD_MAX_HEIGHT;
  set_bounds(bound_buff);
  cmd_bounds();
  draw_color_fs(RED);
}

int draw_char(uint16_t x, uint16_t y, char ch, uint8_t font_size, j_color FILL_COL, j_color BG_COL) {
  uint8_t x_len = j_fonts_x_len[font_size];
  uint8_t y_len = j_fonts_y_len[font_size];
  uint16_t *font_dat = j_fonts[font_size] + (ch - 32)*y_len;
  int buf_size = y_len*x_len*3;
  set_bounds((uint16_t[]){x, x + x_len - 1, y, y + y_len - 1});
  cmd_bounds();

  uint32_t color_array[2] = {BG_COL,FILL_COL};

  lcd_cmd(CMD_MEMORY_WRITE,NULL);
  gpio_pin_set_dt(J_CONTAINER_t.dcx_gpio,1);
  int j = 0;
  uint32_t cur_col = BLACK;
  for(int i = 0; i < buf_size; i+=3) {
    cur_col = color_array[!!((font_dat[j / x_len] >> (j % x_len)) & (0x8000 >> (x_len - 1)))];
    font_buf[i] = cur_col;
    font_buf[i+1] = cur_col >> 8;
    font_buf[i+2] = cur_col >> 16;
    j++;
  }
  color_data_buf.buf = font_buf;
  color_data_buf.len = buf_size;

  spi_write(J_CONTAINER_t.dev_spi,J_CONTAINER_t.spi_cfg,&color_data_set);
}

int draw_text(uint16_t x, uint16_t y, char* str, uint8_t font_size, j_color FILL_COL, j_color BG_COL) {
  uint8_t x_len = j_fonts_x_len[font_size];
  uint8_t y_len = j_fonts_y_len[font_size];
  uint16_t x_l, y_l;
  x_l = x;
  y_l = y;
  
  size_t len = 0;
  while(str[len]) {
    len++;
  }
  for(int i = 0; i < len; i++) {
    draw_char(x_l,y_l,str[i],font_size,FILL_COL,BG_COL);
    x_l += x_len + TEXT_KERNING;
    if(x_l + x_len > LCD_MAX_HEIGHT) {
      x_l = x;
      y_l += y_len + TEXT_SPACING;
    }
  }
}

int ram_load_decal(uint8_t* ram_data, const uint8_t* data, size_t len, uint16_t* ram_crop, j_animation_data* anim_dat, j_component* comp) {
  // ram_crop has the following data: [OG_X, OG_Y, CROP_X, CROP_Y, LR]
  size_t pixel_count, k;
  k = pixel_count = 0;
  j_decal_data* decal_dat = (j_decal_data*)comp->dat2;
  j_color col = decal_dat == NULL ? WHITE : decal_dat->col;
  j_color bg_col = decal_dat == NULL ? BLACK : decal_dat->bg_col;
  j_color ret_byte = 0x00;
  if(ram_crop[0] == NULL) {
    printk("Hello?\n");
    if(anim_dat != NULL) {
      uint8_t *percentage = &(anim_dat->percentage);
      j_color bg_col_user = anim_dat->bg_col;
      j_color bg_col_weighted = (100 - *percentage) * bg_col_user;
      j_color col_weighted;
      for(size_t i = 0; i < len; i+=3) { 
        ret_byte = data[k] & ((0x80) >> pixel_count%8) ? col : bg_col;
        col_weighted = *percentage * ret_byte;
        ram_data[i] = (col_weighted + bg_col_weighted)/100;
        ram_data[i+1] = (col_weighted + bg_col_weighted)/100;
        ram_data[i+2] = (col_weighted + bg_col_weighted)/100;
        if(pixel_count%8 == 7) k++;
        pixel_count++;
      }
    } else {
      printk("Entered!");
      size_t i;
      k = i = pixel_count = 0;
      for(; i < len; i+=3) { 
        ret_byte = data[k] & ((0x80) >> pixel_count%8) ? col : bg_col;
        ram_data[i] = ret_byte;
        ram_data[i+1] = ret_byte >> 8;
        ram_data[i+2] = ret_byte >> 16;
        if(pixel_count%8 == 7) k++;
        pixel_count++;
      }
    }
    return k;
  }
  //NOTE: NO crop algorithm for decals.
}

int ram_load(uint8_t* ram_data, const uint8_t* data, size_t len, uint16_t* ram_crop, j_animation_data* anim_dat, j_component* comp) {
  // ram_crop has the following data: [OG_X, OG_Y, CROP_X, CROP_Y, LR]
  if(ram_crop[0] == NULL) {
    if(anim_dat != NULL) {
      uint8_t bg_col_B, bg_col_G, bg_col_R;
      uint8_t *percentage = &(anim_dat->percentage);
      bg_col_B = anim_dat->bg_col;
      bg_col_G = (anim_dat->bg_col & 0x00FF00) >> 8;
      bg_col_R = (anim_dat->bg_col & 0x0000FF) >> 16;
      for(size_t i = 0; i < len; i+=3) { 
        ram_data[i] = (*percentage * data[i] + (100 - *percentage) * bg_col_B)/100;
        ram_data[i+1] = (*percentage * data[i+1] + (100 - *percentage) * bg_col_G)/100;
        ram_data[i+2] = (*percentage * data[i+2] + (100 - *percentage) * bg_col_R)/100;
      }
    } else {
      for(size_t i = 0; i < len; i++) { 
        ram_data[i] = data[i];
      }
    }
    return len;
  } else { // Crop algorithm
    uint16_t OG_X, OG_Y, CROP_X, CROP_Y, LR;
    OG_X = ram_crop[0];
    OG_Y = ram_crop[1];
    CROP_X = ram_crop[2];
    CROP_Y = ram_crop[3];
    LR = ram_crop[4];
    size_t i = 0;
    size_t delta = 0;
    for(; i < len; i++) {
      ram_data[i] = data[i + ((i/(CROP_X * 3))+LR)*(OG_X-CROP_X)*3];
    }
    return len + ((len/(CROP_X*3)))*(OG_X-CROP_X)*3;
  }
}

void ram_draw_cb(const struct device *dev, int result, void *data) {
  if(result != 0) {
    //printk("ram_draw_cb: callback result error.\n");
    J_CTX1.flags |= 0x40; // Error flag
    return;
  }
  J_CTX1.flags |= 0x80; // Write flag
  //printk("Write flag is set to 1\n");
}

void ram_draw_image(int x_coord, int y_coord, j_component* component, j_animation_data* anim) {  
  const uint8_t* img_data = (uint8_t*)component->dat;
  uint16_t height = (img_data[0] << 8) | img_data[1];
  uint16_t length = (img_data[2] << 8) | img_data[3];

  int (*ram_draw_func)(uint8_t*, const uint8_t*, size_t, uint16_t*, j_animation_data*, j_component*) = component->type - J_IMAGE ? ram_load_decal : ram_load; // Load decal or image
  
  uint16_t x, y;
  if(x_coord > 0)
    x = x_coord;
  else
    x = 0;

  if(y_coord > 0)
    y = y_coord;
  else
    y = 0;

  uint16_t LR = x > 0 ? 1 : 0; // Left/Right crop (1 for Right, 0 for Left)
  uint16_t upper_height = x_coord + length - 1 < LCD_MAX_HEIGHT ? x_coord + length - 1 : LCD_MAX_HEIGHT;
  uint16_t upper_length = y_coord + height - 1 < LCD_MAX_LENGTH ? y_coord + height - 1 : LCD_MAX_LENGTH;

  uint16_t ram_cmd[5] = {length, height, upper_height - x + 1, upper_length - y + 1, LR};
  size_t ram_cmd_ret;
  size_t chunk_size;

  const uint8_t* old_img_data = img_data;
  img_data = img_data + 4 + (y_coord < 0 ? length * -3 * y_coord : 0); // Image data actually starts here
  

  if(ram_cmd[2] == ram_cmd[0] && ram_cmd[3] == ram_cmd[1]) {
    ram_cmd[0] = NULL; // No cropping needed, simplify spi write.
    chunk_size = (RAM_DATA_SIZE/(length*8)) * (length*8);
  } else {
    height = ram_cmd[3];
    length = ram_cmd[2];
    chunk_size = (RAM_DATA_SIZE/(length*3)) * (length*3);
  }

  size_t size = length * height * 3;
  if(size < chunk_size)
    chunk_size = size;

  size_t offset = chunk_size;
  size_t repeats = (size / chunk_size);
  set_bounds((uint16_t[]){x, upper_height, y, upper_length});
  cmd_bounds();
  printk("RAM_DRAW_IMAGE: Image draw w/ %d repeats, %d chunk size, %d size, (%dx%d)\n\n",repeats,chunk_size,size,length,height);
  k_msleep(10);
  color_data_buf.buf = color_data;
  color_data_buf.len = chunk_size;
  k_msleep(5);
  lcd_cmd(CMD_MEMORY_WRITE,NULL);
  
  gpio_pin_set_dt(J_CONTAINER_t.dcx_gpio,1);
  ram_cmd_ret = ram_draw_func(color_data,img_data,chunk_size,ram_cmd,anim,component); // Will return the correct location where ram_load left off
  spi_transceive_cb(J_CONTAINER_t.dev_spi,J_CONTAINER_t.spi_cfg,&color_data_set,NULL,ram_draw_cb,NULL);
  while(offset < size) {
    if(offset + chunk_size > size)
      chunk_size = size - offset;
    img_data += ram_cmd_ret;
    offset += chunk_size;
    ram_draw_func(J_CTX1.buf_ptr,img_data,chunk_size,ram_cmd,anim,component);
    color_data_buf.buf = J_CTX1.buf_ptr;
    color_data_buf.len = chunk_size;

    while(!(J_CTX1.flags & 0x80)) { // Wait for spi
      k_msleep(1);
      if(J_CTX1.flags & 0x40) { // Check for error in callback while waiting
        offset = size;
        break;
      }
    }
    spi_transceive_cb(J_CONTAINER_t.dev_spi,J_CONTAINER_t.spi_cfg,&color_data_set,NULL,ram_draw_cb,NULL);
    J_CTX1.flags &= ~0x80;
    // Switch the buffs around
    uint8_t* temp = J_CTX1.buf_ptr_2;
    J_CTX1.buf_ptr_2 = J_CTX1.buf_ptr;
    J_CTX1.buf_ptr = temp;
  }
  while(!(J_CTX1.flags & 0x80)) { // Wait for spi
    k_msleep(1);
  }
  J_CTX1.buf_ptr = color_data_2;
  J_CTX1.buf_ptr_2 = color_data;
  J_CTX1.flags &= 0x00;
  printk("RAMLOAD: Image written\n");
}

void ram_draw_image_helper(int x_coord, int y_coord, j_component* comp, j_animation_data* anim) {
  if(anim != NULL) {
    uint8_t increment = anim->increment_speed ? anim->increment_speed : 1;
    while(1) {
      if(anim->percentage == 100) {
        k_msleep(1000);
        anim->percentage = 0;
        break;
      }
      anim->percentage += increment;
      if(anim->percentage > 100) anim->percentage = 100;
      ram_draw_image(x_coord,y_coord,comp,anim);
    }
    return;
  }
  ram_draw_image(x_coord,y_coord,comp,anim);
}

j_component* create_component(char* name, j_type type, uint16_t x, uint16_t y, void* dat, void* dat2) {
  j_component* ret = malloc(sizeof(j_component));
  if(ret == NULL) return NULL;
  ret->name = name;
  ret->type = type;
  ret->x = x;
  ret->y = y;
  ret->dat = dat;
  ret->dat2 = dat2;
  ret->index = 100;
  ret->index2 = 25;
  ret->next_ptr = ret->prev_ptr = NULL;
  return ret;
}

// void add_component(j_component* component) {
//   comp_arr[h_index] = component;
//   component->index = h_index;
//   if(component->type == J_BUTTON && component->dat2 != NULL) add_button_component(component);
//   h_index++;
// }

// void add_button_component(j_component* component) {
//   button_arr[h_index_b] = component;
//   component->index2 = h_index_b;
//   h_index_b++;
// }

// void remove_component(j_component* component) {
//   uint8_t index = component->index;
//   if(index > 99)
//     return;
//   printk("\"%s\" removed from index [%d]...\n\n",component->name,index);
//   component->index = 100;
//   if(index == 99 || comp_arr[index+1] == NULL) {
//     comp_arr[index] = NULL;
//     return;
//   }
//   while(index < 99) {
//     comp_arr[index] = comp_arr[index+1];
//     if(comp_arr[index] == NULL)
//       break;
//     comp_arr[index]->index = index;
//     index++;
//   }
//   if(component->type == J_BUTTON) { // Remove button from special array
//     button_arr[component->index2] = NULL;
//     uint8_t index2 = component->index2;
//     component->index2 = 25;

//     if(index2 == 24 || button_arr[index2+1] == NULL) {
//       button_arr[index2] = NULL;
//       return;
//     }

//     while(index2 < 25) {
//       button_arr[index2] = button_arr[index2+1];
//       if(button_arr[index2] == NULL)
//         break;
//       button_arr[index2]->index2 = index2;
//       index2++;
//     }
//   }
//   h_index--;
//   h_index_b--;
//   comp_arr[index] = NULL;
//   free(component);
// }


// void change_component_index(j_component* component, uint8_t new_index) {
//   uint8_t index = component->index;
//   if(new_index > 99 || new_index >= h_index || new_index == index) return;
//   int8_t increment = new_index < component->index ? -1 : 1;

//   printk("\"%s\" switched from index %d to %d...\n\n", component->name,component->index,new_index);

//   while((index * increment) < (new_index * increment)) {
//     comp_arr[index] = comp_arr[index+increment];
//     comp_arr[index]->index = index;
//     index += increment;
//   }

//   component->index = new_index;
//   comp_arr[index] = component;
// }

// void print_components() {
//   printk("Printing component array [%d components]...\n",h_index);
//   char* strings[] = {"J_BUTTON","J_SHAPE","J_TEXT","J_IMAGE","J_BAR","J_FILL"};
//   for(int i = 0; i < h_index; i++) {
//     j_component* cur = comp_arr[i];
//     printk("[%d] \"%s\"\t[x: %d, y: %d, type: %s, data: %s]\n", cur->index, cur->name, cur->x, cur->y, strings[cur->type], comp_arr[i]->dat == NULL ? "NULL" : "OK");
//   }
//   printk("\nPrinting button array [%d components]...\n",h_index_b);
//   for(int i = 0; i < h_index_b; i++) {
//     j_component* cur = button_arr[i];
//     j_button_data* data = (j_button_data*)button_arr[i]->dat2;
//     printk("[%d] \"%s\"\t[x: %d, y: %d, pressed?: %s]\n", cur->index2, cur->name, cur->x, cur->y, data->pressed_status ? "YES" : "NO");
//   }
//   printk("\n");
// }


// void draw_screen(int8_t* exclude_list, size_t len) {
//   printk("DRAW_SCREEN: Redrawing screen with %d components...",h_index);
//   uint8_t exclude_flag = 0;
//   for(int i = 0; i < h_index; i++) {
//     for(int j = 0; j < len; j++) {
//       if(comp_arr[i]->type == exclude_list[j]) {
//         exclude_flag = 1;
//         break;
//       } // Check excludes
//     }

//     if(comp_arr[i] != NULL && comp_arr[i]->dat != NULL && !exclude_flag) {
//       draw_handle_funcs[comp_arr[i]->type](comp_arr[i]); // Draw item
//     }
//     exclude_flag = 0;
//   }
// }

void draw_component(j_component* component) {
  //printk("DRAW_COMPONENT: Drawing component \"%s\"...\n\n",component->name);
  draw_handle_funcs[component->type](component);
}


j_component* lcd_check_button_pressed(uint16_t x, uint16_t y, uint8_t buffer_amt) {
  j_component* cur = comp_linked_list.start;
  for(int i = 0; i < MAX_LINKED_LIST_SIZE; i++) {
    if(cur == NULL) return NULL;
    if(cur->type == J_BUTTON && cur->dat2 != NULL) {
      j_button_data* button_dat = (j_button_data*)(cur->dat2);
      if(IS_POINT_IN_BOX_BUFFER(x,y,cur->x,cur->y,button_dat->length,button_dat->height,buffer_amt)) {
        return cur;
      }
    }
    cur = cur->next_ptr;
  }
  return NULL;
}

uint8_t press_button_visual(j_component* button) {
  if(button != NULL) {
    j_button_data* button_dat = (j_button_data*)(button->dat2);
    printk("Entered visual %d\n",button_dat->height);
    button_dat->pressed_status = 1;
    draw_component(button);
    k_msleep(200);
    button_dat->pressed_status = 0;
    draw_component(button);
    printk("Exited visual\n");
    return 1;
  }
  printk("invalid\n");
  return 0;
}

void update_text(j_component* text_component, char* new_str) {
  if(text_component->dat2 == NULL || (text_component->prev_ptr == NULL && comp_linked_list.start != text_component)) return;
  j_text_data* text_dat = (j_text_data*)text_component->dat2;

  uint16_t len = 0;
  char* old_str = (char*)text_component->dat; // Keep this for later
  
  while(old_str[len]) len++;
  uint16_t x_l = text_component->x;
  if(text_dat->centering > J_LEFT) {
    uint16_t shift =  (len*j_fonts_x_len[text_dat->font_size])/(text_dat->centering - 1 ? 1 : 2); // Shift value for X for centering
    x_l -= shift > x_l ? x_l : shift;
  }
  set_bounds((uint16_t[]){x_l,x_l+len*(j_fonts_x_len[text_dat->font_size]+TEXT_KERNING),text_component->y,text_component->y+j_fonts_y_len[text_dat->font_size]});
  cmd_bounds();
  draw_color_fs(text_dat->bg_col);
  text_component->dat = new_str;
  draw_handle_text(text_component);
}

void add_component_o(j_component* component) {
  if(comp_linked_list.start == NULL) {
    comp_linked_list.start = comp_linked_list.end = component;
    return;
  }
  comp_linked_list.end->next_ptr = component;
  component->prev_ptr = comp_linked_list.end;
  comp_linked_list.end = component;
}

uint8_t remove_component_o(j_component* component) {
  // Check if the next and previous pointer is NULL/non-NULL, and update values accordingly.
  uint8_t ret = 1;
  component->next_ptr == NULL ?
    (comp_linked_list.end == component ? (comp_linked_list.end = component->prev_ptr) : (ret = 0)) :
    (component->next_ptr->prev_ptr = component->prev_ptr);

  component->prev_ptr == NULL ?
    (comp_linked_list.start == component ? (comp_linked_list.start = component->next_ptr) : (ret = 0)) :
    (component->prev_ptr->next_ptr = component->next_ptr);
  
  component->prev_ptr = component->next_ptr = NULL;

  return 1;
}

void insert_component_o(j_component* component, j_component* og_component) {
  component->prev_ptr = og_component->prev_ptr;
  component->prev_ptr == NULL ? (comp_linked_list.start = component) : (component->prev_ptr->next_ptr = component);
  og_component->prev_ptr = component;
  component->next_ptr = og_component;
}

void change_component_index_o(j_component* component, uint8_t new_index) {
  if(!remove_component_o(component)) return;
  j_component* cur = comp_linked_list.start;
  if(cur == NULL) return;
  for(int i = 0; i < MAX_LINKED_LIST_SIZE; i++) { // Safeguard in case of weird loop pointers, infinite loop.
    if(i == new_index || cur->next_ptr == NULL) {
      insert_component_o(component,cur);
      return;
    }
    cur = cur->next_ptr;
  }
  return;
}

void print_components_o() {
  j_component* cur = comp_linked_list.start;
  printk("Printing components...\n\n");
  char* strings[] = {"J_BUTTON","J_SHAPE","J_TEXT","J_IMAGE","J_BAR","J_FILL"};
  int i = 0;
  for(; i < MAX_LINKED_LIST_SIZE; i++) {
    if(cur == NULL) break;
    printk("[%d] \"%s\"\t[x: %d, y: %d, type: %s, data: %s]\n", i, cur->name, cur->x, cur->y, strings[cur->type], cur->dat == NULL ? "NULL" : "OK");
    cur = cur->next_ptr;
  }
  printk("\nFound %d items...\n\n", i);
}

void draw_screen_o(int8_t* exclude_list, size_t len) {
  printk("DRAW_SCREEN: Redrawing screen...\n\n");
  uint8_t exclude_flag = 1;
  j_component* cur = comp_linked_list.start;
  for(int i = 0; i < MAX_LINKED_LIST_SIZE; i++) {
    if(cur == NULL) break;
    for(int j = 0; j < len; j++) (cur->type == exclude_list[j] ? (exclude_flag = 0) : 0);
    if(exclude_flag) draw_handle_funcs[cur->type](cur);
    exclude_flag = 1;
    cur = cur->next_ptr;
  }
  k_msleep(5);
}

void poll_touch(uint16_t* x, uint16_t* y) {
  uint16_t x_pos, y_pos;
  x_pos = y_pos = -1;
  while(1) {
    uint8_t touch_response;
    uint32_t position;
    touch_control_cmd_rsp(TD_STATUS,&touch_response);
    if(touch_response == 1) {
      position = get_pos();
      x_pos = (uint16_t)(position >> 16);
      y_pos = (uint16_t)(position);
      break;
    }
  }
  *x = x_pos;
  *y = y_pos;
}

void clear_draw_buffer() {
  j_component* cur = comp_linked_list.start;
  j_component* next_cur;
  for(int i = 0; i < MAX_LINKED_LIST_SIZE; i++) {
    if(cur == NULL) break;
    next_cur = cur->next_ptr;
    cur->next_ptr = NULL;
    cur->prev_ptr = NULL;
    free(cur);
    cur = next_cur;
  }
  comp_linked_list.start = comp_linked_list.end = NULL;
}