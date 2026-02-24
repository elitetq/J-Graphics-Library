#include <inttypes.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/random/random.h>
#include <zephyr/drivers/i2c.h>


#include "BTN.h"
#include "LED.h"

#include <math.h>
#include "J_GL.h"
#include "J_ASSETS.h"

#define BOX_SIZE (uint16_t)20



// LOW_HEIGHT - MAX_HEIGHT - LOW_LENGTH - MAX_LENGTH
static uint16_t bounds[4] = {0x0000,LCD_MAX_HEIGHT,0x0000,LCD_MAX_LENGTH};
static uint8_t column_array[4] = {(uint8_t)(0x0000>>8),(uint8_t)0x0000,(uint8_t)(LCD_MAX_HEIGHT>>8),(uint8_t)LCD_MAX_HEIGHT};
static uint8_t row_array[4] = {(uint8_t)(0x0000>>8),(uint8_t)0x0000,(uint8_t)(LCD_MAX_LENGTH>>8),(uint8_t)(LCD_MAX_LENGTH)};

static const struct gpio_dt_spec dcx_gpio = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE,dcx_gpios);
static const struct spi_cs_control cs_ctrl = (struct spi_cs_control) {
  .gpio = GPIO_DT_SPEC_GET(ARDUINO_SPI_NODE,cs_gpios),
  .delay = 0u,
};

static const struct device * dev = DEVICE_DT_GET(ARDUINO_SPI_NODE);
#define ARDUINO_I2C_NODE DT_NODELABEL(arduino_i2c)
static const struct device * dev_i2c = DEVICE_DT_GET(ARDUINO_I2C_NODE);
static const struct spi_config spi_cfg = {
  .frequency = 15000000,
  .operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
  .slave = 0,
  .cs = cs_ctrl
};


int main(void) {
  if(!device_is_ready(dev))
    return 0;
  if(!gpio_is_ready_dt(&dcx_gpio))
    return 0;
  if(gpio_pin_configure_dt(&dcx_gpio,GPIO_OUTPUT_LOW))
    return 0;
  
  if(0 > i2c_configure(dev_i2c,I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER))
    return 0;
  if(0 > BTN_init())
    return 0;
  if(0 > LED_init())
    return 0;
  
  J_init(dev,dev_i2c,&spi_cfg,&dcx_gpio,bounds);
  J_LCD_init();
  draw_color_fs(BLACK);

  //k_msleep(1000);
  //ram_draw_image(0,50,img2);
  // draw_char(0,0,'B',FONT_LARGE,BLACK,BLUE);
  //draw_text(0,0,"100.567",FONT_LARGE,MAGENTA,BLUE);
  j_color black_col = BLACK;

  j_text_data dat_text = {
    .font_size = FONT_SMALL,
    .col = WHITE,
    .bg_col = BLACK,
    .centering = J_CENTER
  };

  j_button_data but_profile = {
    .bg_col = MAGENTA,
    .border_col = WHITE,
    .border_width = 2,
    .col = YELLOW,
    .font_size = FONT_MEDIUM,
    .height = 35,
    .length = 100,
    .pressed_status = 0
  };

  j_bar_data bar_dat = {
    .bg_col = BLACK,
    .col = WHITE,
    .height = 2,
    .length = 100,
    .type = J_BAR_LR
  };

  j_shape_data shape_dat = {
    .bg_col = BLACK,
    .col = WHITE,
    .centering = J_CENTER,
    .height = 200,
    .length = 100,
    .type = J_RECTANGLE
  };

  uint8_t val = 0;

  j_animation_data anim_data = {.bg_col = BLACK, .increment_speed = 5, .percentage = 0, .type = FADE_IN};
  j_animation_data anim_data_2 = {.bg_col = BLACK, .increment_speed = 30, .percentage = 0, .type = RESIZE, .x_low = 0, .x_high = 239, .y_low = 0, .y_high = 319};
  j_component* TEXT = create_component("text1",J_TEXT,120,270,"",&dat_text);
  j_component* BUTTON = create_component("button1",J_BUTTON,120-but_profile.length/2,20,"Press!",&but_profile);
  j_component* IMG = create_component("image1",J_IMAGE,120-50,110,sojourn_logo_img,&anim_data);
  j_component* FILL = create_component("bg_col",J_FILL,0,0,&black_col,NULL);
  j_component* SHAPE = create_component("shape1",J_SHAPE,120,160,&shape_dat,&anim_data_2);
  j_component* SHAPE2 = create_component("shape2",J_SHAPE,240,160,&shape_dat,&anim_data_2);
  j_component* BAR = create_component("bar1",J_BAR,120-bar_dat.length/2,285,&val,&bar_dat);

  char* str_array[4] = {"Loading components...","Loading images...","Doing cool stuff...","Flipping pancakes..."};
  int j = -1;

  add_component_o(FILL);
  add_component_o(IMG);
  add_component_o(BAR);
  add_component_o(TEXT);


  while(1) {
    uint8_t touch_response = 0;
    uint16_t x_pos, y_pos;
    uint32_t position;
    touch_control_cmd_rsp(TD_STATUS,&touch_response);
    if(touch_response == 1) {
      break;
    }
  }

  draw_screen_o(NULL,0);

  //draw_screen(NULL,0);
  while(0) {
    val+= 5;
    if(!(val %= 105)) {
      j++;
      if(j >= 4) break;
      update_text(TEXT,str_array[j]);
    }
    draw_component(BAR);
    k_msleep(50);
  }
  k_msleep(50);
  draw_component(SHAPE);
  anim_data_2.increment_speed = 10;
  shape_dat.bg_col = WHITE;
  shape_dat.col = BLACK;
  draw_component(SHAPE);
  // draw_screen(NULL,0);

  while(0) {
    uint8_t touch_response;
    uint16_t x_pos, y_pos;
    uint32_t position;
    touch_control_cmd_rsp(TD_STATUS,&touch_response);
    if(touch_response == 1) {
      position = get_pos();
      x_pos = (uint16_t)(position >> 16);
      y_pos = (uint16_t)(position);
      j_component* button = lcd_check_button_pressed(x_pos,y_pos);
      if(press_button_visual(button)) {
        k_msleep(50);
      }
    }
  }
  return 0;
}
