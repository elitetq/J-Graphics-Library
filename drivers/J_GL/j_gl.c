#include "J_GL.h"
static struct J_CONTAINER J_CONTAINER_t = {0};
static volatile uint8_t color_data_2[RAM_DATA_SIZE];
static volatile uint8_t color_data[(LCD_MAX_LENGTH+1)*(LCD_MAX_HEIGHT+1)*3/LCD_BUF_DIV];
static struct spi_buf color_data_buf = {};
static struct spi_buf_set color_data_set = {.buffers = &color_data_buf, .count = 1};
static uint8_t pixel_color_shape[3] = {0xFF,0x00,0x00};
static uint16_t bound_buff[4] = {0,0,0,0};

typedef struct {
  uint8_t* buf_ptr;
  uint8_t* buf_ptr_2;
  uint8_t flags; // This is a bitmask (MSB first): Write flag [0], Error flag [1]
} j_spi_ctx;

static j_spi_ctx J_CTX1 = {color_data_2,color_data,0x00};

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
    y_pos = (uint16_t)(((x_pos_h & TOUCH_POS_MSB_MASK) << 8) + x_pos_l);
    x_pos = LCD_MAX_LENGTH - (uint16_t)(((y_pos_h & TOUCH_POS_MSB_MASK) << 8) + y_pos_l);
    if(x_pos > LCD_MAX_LENGTH) {
      x_pos = LCD_MAX_LENGTH;
    }
    if(y_pos > LCD_MAX_HEIGHT) {
      y_pos = LCD_MAX_HEIGHT;
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
  if(user_list[0] > user_list[1] || user_list[2] > user_list[3]) {
    printk("Invalid bounds xlo, xhi, ylo, yhi = %d %d %d %d\n",user_list[0],user_list[1],user_list[2],user_list[3]);
    return;
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

// FIX BUG HERE
void draw_square(uint16_t x, uint16_t y, uint16_t size) {
  bound_buff[2] = y > size ? y - size : 0;
  bound_buff[3] = (y + size) < LCD_MAX_HEIGHT ? y + size : LCD_MAX_HEIGHT;
  bound_buff[0] = x > size ? x - size : 0;
  bound_buff[1] = (x + size) < LCD_MAX_LENGTH ? x + size : LCD_MAX_LENGTH;
  set_bounds(bound_buff);
  draw_color_fs(RED);
}

void draw_image(uint16_t x, uint16_t y, const uint8_t* img_data) {
  // Obtain position of image
  uint16_t height = (img_data[0] << 8) | img_data[1];
  uint16_t length = (img_data[2] << 8) | img_data[3];

  // Set size and chunk_size correspondingly
  size_t size = length * height * 3;
  size_t chunk_size = MAX_SPI_CHUNK_SIZE;
  if(size < MAX_SPI_CHUNK_SIZE)
    chunk_size = size;
  img_data = img_data + 4; // Image data actually starts here
  size_t repeats = size / chunk_size;
  // Boom boom pretty cool set the bounds
  set_bounds((uint16_t[]){y, y + length - 1, x, x + height - 1});
  cmd_bounds();
  printk("Drawing image with %d chunk size, %d size, (%dx%d)\n",chunk_size,size,length,height);
  k_msleep(10);
  color_data_buf.buf = img_data;
  color_data_buf.len = chunk_size;
  k_msleep(5);
  lcd_cmd(CMD_MEMORY_WRITE,NULL);
  
  gpio_pin_set_dt(J_CONTAINER_t.dcx_gpio,1);
  for(int i = 0; i < repeats; i++) {
    int ret = spi_write(J_CONTAINER_t.dev_spi,J_CONTAINER_t.spi_cfg,&color_data_set);
    img_data += chunk_size;
    color_data_buf.buf = img_data;
    printk("%d %d\n",i,ret);
  }
  chunk_size = size - chunk_size*repeats;
  color_data_buf.len = chunk_size;
  spi_write(J_CONTAINER_t.dev_spi,J_CONTAINER_t.spi_cfg,&color_data_set);
  printk("Donezo\n");
}

int draw_text(char ch, uint8_t font_size, j_color FILL_COL) {

}

int ram_load(uint8_t* ram_data, const uint8_t* data, size_t len, uint16_t* ram_crop) {
  // ram_crop has the following data: [OG_X, OG_Y, CROP_X, CROP_Y]
  if(ram_crop[0] == NULL) {
    for(size_t i = 0; i < len; i++) { 
      ram_data[i] = data[i];
    }
    return len;
  } else { // Crop algorithm
    uint16_t OG_X, OG_Y, CROP_X, CROP_Y;
    OG_X = ram_crop[0];
    OG_Y = ram_crop[1];
    CROP_X = ram_crop[2];
    CROP_Y = ram_crop[3];
    size_t i = 0;
    size_t delta = 0;
    for(; i < len; i++) {
      ram_data[i] = data[i + ((i/(CROP_X * 3))+1)*(OG_X-CROP_X)*3];
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

void ram_draw_image(uint16_t x, uint16_t y, const uint8_t* img_data) {  
  uint16_t height = (img_data[0] << 8) | img_data[1];
  uint16_t length = (img_data[2] << 8) | img_data[3];


  img_data = img_data + 4; // Image data actually starts here

  uint16_t upper_height = x + length - 1 < LCD_MAX_HEIGHT ? x + length - 1 : LCD_MAX_HEIGHT;
  uint16_t upper_length = y + height - 1 < LCD_MAX_LENGTH ? x + height - 1 : LCD_MAX_LENGTH;

  uint16_t ram_cmd[4] = {length, height, upper_height - x + 1, upper_length - y + 1};
  size_t ram_cmd_ret;
  size_t chunk_size;

  if(ram_cmd[2] == ram_cmd[0] && ram_cmd[3] == ram_cmd[1]) {
    ram_cmd[0] = NULL; // No cropping needed, simplify spi write.
    chunk_size = RAM_DATA_SIZE;
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
  printk("RAMLOAD: Drawing image with %d repeats, %d chunk size, %d size, (%dx%d)\n",repeats,chunk_size,size,length,height);
  k_msleep(10);
  color_data_buf.buf = color_data;
  //color_data_buf.len = chunk_size;
  color_data_buf.len = chunk_size;
  k_msleep(5);
  lcd_cmd(CMD_MEMORY_WRITE,NULL);
  
  gpio_pin_set_dt(J_CONTAINER_t.dcx_gpio,1);
  ram_cmd_ret = ram_load(color_data,img_data,chunk_size,ram_cmd);
  spi_transceive_cb(J_CONTAINER_t.dev_spi,J_CONTAINER_t.spi_cfg,&color_data_set,NULL,ram_draw_cb,NULL);
  if(offset >= size) {
    while(!(J_CTX1.flags) & 0x80) {
      k_msleep(1);
    }
  }
  while(offset < size) {
    if(offset + chunk_size > size)
      chunk_size = size - offset;
    img_data += ram_cmd_ret;
    offset += chunk_size;
    ram_load(J_CTX1.buf_ptr,img_data,chunk_size,ram_cmd);
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
  J_CTX1.buf_ptr = color_data_2;
  J_CTX1.buf_ptr_2 = color_data;
  J_CTX1.flags &= 0x00;
  printk("RAMLOAD: Image written\n");
}

