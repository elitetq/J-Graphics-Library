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

/*----------------------------------------------------------
          Touch Sensor Registry/Variable Defines
----------------------------------------------------------*/
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

/*----------------------------------------------------------
          LCD Screen Registry/Variable Defines
----------------------------------------------------------*/
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
/*----------------------------------------------------------
                Buffer / Text Safety Defines
----------------------------------------------------------*/
#define CNV_8_TO_6(x) ((uint8_t)x << 2u)

#define SLEEP_MS 100
#define PI 3.14159
#define ARDUINO_SPI_NODE DT_NODELABEL(arduino_spi)
#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

#define RAM_DATA_DIV 4
#define RAM_DATA_SIZE (LCD_MAX_LENGTH+1)*(LCD_MAX_HEIGHT+1)*3/RAM_DATA_DIV
#define MAX_SPI_CHUNK_SIZE 30000
#define MAX_LINKED_LIST_SIZE 1000

#define TEXT_KERNING 0
#define TEXT_SPACING 3


/*----------------------------------------------------------
                Font Sizes / Context Struct
----------------------------------------------------------*/
static uint8_t font_length[3] = {7,11,16};
static uint8_t font_height[3] = {10,18,26};
static uint16_t* font_ptr[3] = {Font7x10,Font11x18,Font16x26};

typedef struct {
  uint8_t* buf_ptr;
  uint8_t* buf_ptr_2;
  uint8_t flags; // This is a bitmask (MSB first): Write flag [0], Error flag [1]
} j_spi_ctx;

/*----------------------------------------------------------
                      Global J Structs
----------------------------------------------------------*/
typedef enum {
  BLACK = 0x00000000,
  WHITE = 0x00FFFFFF,
  RED = 0x00FF0000,
  GREEN = 0x0000FF00,
  BLUE = 0x000000FF,
  MAGENTA = 0x00FF00FF,
  YELLOW = 0x00FFFF00,
  GRAY = 0x007F7F7F
} j_color;


typedef enum {
  FONT_SMALL = 0,
  FONT_MEDIUM,
  FONT_LARGE
} j_font_size;

typedef enum {
  J_LEFT,
  J_CENTER,
  J_RIGHT
} j_centering;

struct J_CONTAINER {
  const struct device* dev_spi;
  const struct device* dev_i2c;
  const struct spi_config* spi_cfg;
  const struct gpio_dt_spec* dcx_gpio;
  uint16_t* bounds;
};


typedef enum {
  J_BUTTON = 0,
  J_SHAPE,
  J_TEXT,
  J_IMAGE,
  J_BAR,
  J_FILL,
  J_DECAL
} j_type;

typedef enum {
  J_RECTANGLE,
  J_CIRCLE,
  J_ITRIANGLE, // Isoscoles
  J_RLTRIANGLE, // Right Triangle, Left Biased
  J_RRTRIANGLE // Right Triangle, Right biased
} j_shape_type;

typedef enum {
  J_BAR_LR = 0,
  J_BAR_RL,
  J_BAR_UD,
  J_BAR_DU
} j_bar_type;

typedef enum {
  FADE_IN,
  FADE_OUT,
  RESIZE
} j_animation_type;

typedef struct j_component j_component;

struct j_component {
  char* name;
  j_type type;
  uint16_t x, y;
  void* dat; // Often used for direct data, such as text contents or image data.
  void* dat2; // Often used for cosmetic data, such as colors, size, etc...
  uint8_t index, index2;
  j_component* next_ptr;
  j_component* prev_ptr;
};

typedef struct {
  j_component* start;
  j_component* end;
} j_linked_list;
// 
/*----------------------------------------------------------
                  Component Data Structs
----------------------------------------------------------*/
typedef struct {
  j_color col, bg_col;
  j_font_size font_size;
  j_centering centering;
} j_text_data;

typedef struct {
  j_color col, bg_col, border_col;
  j_font_size font_size;
  uint8_t border_width, pressed_status; // Pressed_status is not pressed (0) and pressed (1)
  uint16_t length, height;
  uint8_t* decal_dat; // set to decal for decal writing, or NULL for text
} j_button_data;

typedef struct {
  j_bar_type type;
  j_color col, bg_col;
  uint16_t length, height;
} j_bar_data;

typedef struct {
  j_animation_type type;
  j_color bg_col;
  uint8_t increment_speed;
  uint8_t percentage;
  uint16_t x_low, y_low, x_high, y_high; // Used for grow / shrink
} j_animation_data;

typedef struct {
  j_shape_type type;
  j_color col, bg_col;
  j_centering centering;
  uint16_t length, height;
} j_shape_data;

typedef struct {
  j_animation_data* animation_dat;
  j_color col, bg_col;
} j_decal_data;


/*----------------------------------------------------------
                    Function Prototypes
----------------------------------------------------------*/

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

/**
 * @brief Receive touch screen response information.
 * 
 * @param cmd Touch data commad for response
 * @param rsp Pointer to store response data
 */
void touch_control_cmd_rsp(uint8_t cmd, uint8_t* rsp);

/**
 * @brief Get position of last touch point.
 * 
 * @return 32-bit positional value (first 16 X, last 16 Y)
 */
uint32_t get_pos();
void draw_square(uint16_t x, uint16_t y, uint16_t size);
int draw_circle(uint16_t x, uint16_t y, uint16_t radius);

/**
 * @brief DEPRECATED: Draw image at given x and y positions, img_data should be J_IMG compatible. Assumes first 4 bytes read by img_data are the length and height of image.
 * 
 * @param x X position
 * @param y Y position
 * @param img_data pointer to image data to be read (J_IMG compatible)
 */
void draw_image(uint16_t x, uint16_t y, const uint8_t* img_data);


/**
 * @brief Same as draw_image, except faster at the cost of some extra ram usage (around 15kb on a 240x320 display), can be turned off with XX (not yet implemented) command.
 * 
 * @param x_coord X position
 * @param y_coord Y position
 * @param img_data pointer to image data to be read (J_IMG compatible)
 */
void ram_draw_image(int x_coord, int y_coord, j_component* component, j_animation_data* anim);

/**
 * @brief Draw text on the screen at given x and y coords, will wrap around if it leaves screen.
 * 
 * @param x X position
 * @param y Y position
 * @param font_size a j_font_size compatible font size.
 * @param FILL_COL color of text
 * @param BG_COL color to fill the empty space with
 */
int draw_text(uint16_t x, uint16_t y, char* str, uint8_t font_size, j_color FILL_COL, j_color BG_COL);

j_component* create_component(char* name, j_type type, uint16_t x, uint16_t y, void* dat, void* dat2);
void add_component(j_component* component);
void remove_component(j_component* component);
void change_component_index(j_component* component, uint8_t new_index);
void print_components();

void add_component_o(j_component* component);
uint8_t remove_component_o(j_component* component);
void change_component_index_o(j_component* component, uint8_t new_index);
void print_components_o();

/**
 * @brief Draw the current component queue onto the screen. Higher indices are prioritized on the foreground.
 * 
 * @param exclude_list The j_types to exclude when redrawing, useful when you want to not redraw certain components to save refresh time.
 * @param len Length of exclude list (set to 0 or less if no excludes).
 */
void draw_screen(int8_t* exclude_list, size_t len);
void draw_screen_o(int8_t* exclude_list, size_t len);

/**
 * @brief Draw singular component on screen.
 * 
 * @param component Component to draw.
 */
void draw_component(j_component* component);

j_component* lcd_check_button_pressed(uint16_t x, uint16_t y);

/**
 * @brief Will refresh the display to make the button look "clicked".
 * 
 * @param button Button component to be updated.
 * 
 * @return 0 for null button pointer, 1 otherwise
 */
uint8_t press_button_visual(j_component* button);

void update_text(j_component* text_component, char* new_str);


#endif