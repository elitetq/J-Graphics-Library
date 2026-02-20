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
  draw_color_fs(BLUE);

  //k_msleep(1000);
  //ram_draw_image(0,50,img2);
  // draw_char(0,0,'B',FONT_LARGE,BLACK,BLUE);
  //draw_text(0,0,"100.567",FONT_LARGE,MAGENTA,BLUE);
  j_component* TEXT = create_component("text1",J_TEXT,0,0,NULL);
  j_component* BUTTON = create_component("button1",J_BUTTON,0,0,NULL);
  j_component* IMG = create_component("image1",J_IMAGE,0,0,NULL);
  j_component* IMG1 = create_component("image2",J_IMAGE,0,0,NULL);
  j_component* BUTTON1 = create_component("button2",J_BUTTON,69,69,NULL);
  add_component(BUTTON);
  add_component(IMG);
  add_component(IMG1);
  add_component(TEXT);
  add_component(BUTTON1);
  print_components();
  change_component_index(TEXT,5);
  print_components();


  while(0) {
    uint8_t touch_response;
    uint16_t x_pos, y_pos;
    uint32_t position;
    touch_control_cmd_rsp(TD_STATUS,&touch_response);
    if(touch_response == 1) {
      position = get_pos();
      x_pos = (uint16_t)(position >> 16);
      y_pos = (uint16_t)(position);
      draw_square(x_pos,y_pos,8);
    }
  }
  return 0;
}
