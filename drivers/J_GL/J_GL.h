#ifndef J_GL
#define J_GL

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/random/random.h>
#include <zephyr/drivers/i2c.h>
#include <inttypes.h>
#include <math.h>
#include <J_ASSETS.h>
#include <stdlib.h>

// Touch Sensor Registry Address Defines
#define TD_ADDR 0x38
#define P1_XL 0x04 // 1st Touch X pos
#define P1_XH 0x03 // Event flag
#define P1_YL 0x06 // 1st Touch Y pos
#define P1_YH 0x05 // Touch ID
#define TD_STATUS 0x02 // num touch points

#define TOUCH_EVENT_MASK 0xC0
#define TOUCH_EVENT_SHIFT 6

#define TOUCH_POS_MSB_MASK 0x0F

#define J_LOAD_ASSETS

typedef enum {
  TOUCH_EVENT_PRESS_DOWN = 0b00u,
  TOUCH_EVENT_LIFT_UP = 0b01u,
  TOUCH_EVENT_CONTACT = 0b10u,
  TOUCH_EVENT_NO_EVENT = 0b11u
} touch_event_t;
// -----------------------

// LCD Screen Registry Address Defines
#define CMD_SOFTWARE_RESET 0x01
#define CMD_SLEEP_OUT 0x11
#define CMD_DISPLAY_ON 0x29
#define CMD_DISPLAY_OFF 0x00 // find the value for this
#define CMD_COLUMN_ADDRESS_SET 0x2A
#define CMD_ROW_ADDRESS_SET 0x2B
#define CMD_MEMORY_WRITE 0x2C
#define CMD_COL_MOD 0x3A

#define LCD_MAX_HEIGHT (uint16_t)0x00EF 
#define LCD_MAX_LENGTH (uint16_t)0x013F
#define LCD_BUF_DIV 2
// -----------------------
#define CNV_8_TO_6(x) ((uint8_t)x << 2u)

#define SLEEP_MS 100
#define PI 3.14159
#define ARDUINO_SPI_NODE DT_NODELABEL(arduino_spi)
#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

#define RAM_DATA_DIV 4
#define RAM_DATA_SIZE (LCD_MAX_LENGTH+1)*(LCD_MAX_HEIGHT+1)*3/RAM_DATA_DIV
#define MAX_SPI_CHUNK_SIZE 30000


// Font sizes
static uint8_t font_length[3] = {7,11,16};
static uint8_t font_height[3] = {10,18,26};
static uint16_t* font_ptr[3] = {Font7x10,Font11x18,Font16x26};

typedef enum {
  FONT_SMALL = 0,
  FONT_MEDIUM,
  FONT_LARGE
} j_font_size;
struct J_CONTAINER {
  const struct device* dev_spi;
  const struct device* dev_i2c;
  const struct spi_config* spi_cfg;
  const struct gpio_dt_spec* dcx_gpio;
  uint16_t* bounds;
};

typedef enum {
  BLACK = 0x00000000,
  WHITE = 0x00FFFFFF,
  RED = 0x00FF0000,
  GREEN = 0x0000FF00,
  BLUE = 0x000000FF,
  MAGENTA = 0x00FF00FF,
  YELLOW = 0x00FFFF00
} j_color;


/**
 * @brief J_GL initialization
 * 
 * @param dev_spi Development device for SPI serial protocol
 * @param dev_i2c Development device for I2C serial protocol
 * @param spi_cfg SPI configuration struct
 * @param dcx_gpio Data/Command pin struct
 * @param bounds Bounds array for color writing
 * 
 */
void J_init(const struct device* dev_spi, const struct device* dev_i2c, const struct spi_config* spi_cfg, const struct gpio_dt_spec* dcx_gpio, uint16_t* bounds);
void J_LCD_init();

void lcd_cmd(uint8_t cmd, struct spi_buf * data);

/**
 * @brief Draw color on screen, respects current bounds.
 */
int draw_color_fs(j_color COLOR);

/**
 * @brief Set the bounds of the LCD display
 * 
 * @param user_list 4 element list of users bounds (lo_h, hi_h, lo_l, hi_l)
 * @param bounds pointer to an equall sized bounds array
 */
void set_bounds(uint16_t* user_list);
void touch_control_cmd_rsp(uint8_t cmd, uint8_t* rsp);
uint32_t get_pos();
void draw_square(uint16_t x, uint16_t y, uint16_t size);
int draw_circle(uint16_t x, uint16_t y, uint16_t radius);
void draw_image(uint16_t x, uint16_t y, const uint8_t* img_data);
void ram_load(const uint8_t* data, size_t len);
void ram_draw_image(uint16_t x, uint16_t y, const uint8_t* img_data);
int draw_text(char ch, uint8_t font_size, j_color FILL_COL);


#endif