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

#include "sojourn_os_smf.h"

#define SLEEP_MS 1

#define BOX_SIZE (uint16_t)20



// LOW_HEIGHT - MAX_HEIGHT - LOW_LENGTH - MAX_LENGTH
uint16_t bounds[4] = {0x0000,LCD_MAX_HEIGHT,0x0000,LCD_MAX_LENGTH};

const struct gpio_dt_spec dcx_gpio = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE,dcx_gpios);
const struct spi_cs_control cs_ctrl = (struct spi_cs_control) {
  .gpio = GPIO_DT_SPEC_GET(ARDUINO_SPI_NODE,cs_gpios),
  .delay = 0u,
};

const struct device * dev = DEVICE_DT_GET(ARDUINO_SPI_NODE);
#define ARDUINO_I2C_NODE DT_NODELABEL(arduino_i2c)
const struct device * dev_i2c = DEVICE_DT_GET(ARDUINO_I2C_NODE);
const struct spi_config spi_cfg = {
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

  state_machine_init();

  while(1) {
    int ret = state_machine_run();
    if(0>ret) {
      printk("Error occured\n");
      return 0;
    }
    k_msleep(SLEEP_MS);
  }
  
  

  return 0;
}
