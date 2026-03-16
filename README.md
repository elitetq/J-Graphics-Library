[SojournOS + Graphics Library]

# Overview
This GitHub features a rudimentary OS - named SojournOS - designed to showcase the custom J Graphics Library, developed by Jonart Bajraktari for the 2025-26 Embedded in Embedded club. This documentation will detail technical details and implementations of the graphics library, as well as the OS details and future roadmap.

# 1 | Graphics Library (J_GL)
The graphics library is entirely custom made, it utilizes the Zephyr RTOS for SPI and State Machines, and a custom pipeline for drawing different components.

## 1.1 Installation and Initialization
To install this graphics library in any zephyr project, you first need to enable SPI (Asynchronous and Synchronous), I2C, and GPIO in your `prj.conf` file.

```conf
# GPIO pins, needed for setting DC pins for LCD
CONFIG_GPIO=y
# SPI (Add both)
CONFIG_SPI=y
CONFIG_SPI_ASYNC=y
# I2C
CONFIG_I2C=y
# Heap size configuration. Depending on the size of your project, you may want to lower this or set it higher. But this is a good baseline
CONFIG_HEAP_MEM_POOL_SIZE=8192
```

Now, you can add the `J_GL` folder in `app/drivers` to your own project. Include it in the CMake file, and use all the functions in the `.h` file.

There are **three** important things to note:
+ This library is designed to be compatible with the Adafruit 2.8" 240x320 LCD TFT Display (With capacitive touch)
+ I am using an nrf52840 development kit from Nordic Semiconductors. This is designed to work for any other board, as long as you can run C. However, there are no guarantees. And this readme will focus on the initialization for the `Zephyr` framework.
+ This library is only tested with a 240x320 display. Theoretically, there are defines you can change in the `J_GL.h` file, and they should work by changing the `LCD_MAX_HEIGHT` and `LCD_MAX_LENGTH` defines, but there are no guarantees.

### Getting started (Initialization in code)
To initialize your display, you need to do a couple things.


First, open your `.overlay` file and enter in your dedicated DC (Data/Command) and CS (Chip Select) pin. This is vital for allowing SPI to work correctly. My code looks like this. These are arbitrary GPIO pins, but make sure they correspond to the right pins on the LCD display.

```overlay
zephyr,user {
    dcx_gpios = <&gpio1 11 GPIO_ACTIVE_HIGH>;
};

arduino_spi {
    cs-gpios = <&gpio1 12 GPIO_ACTIVE_LOW>;
};
```

Now add these includes to your main.c

```c
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/random/random.h>
#include <zephyr/drivers/i2c.h>
```

Third step is to define all your initialization variables, this part can get confusing, so I have provided some documentation with each function call to explain what is happening.

```c
// Set bound array to keep track of where we want to draw on the screen
uint16_t bounds[4] = {0x0000,LCD_MAX_HEIGHT,0x0000,LCD_MAX_LENGTH};

// Find DCX and CSX gpios and store them
const struct gpio_dt_spec dcx_gpio = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE,dcx_gpios);
const struct spi_cs_control cs_ctrl = (struct spi_cs_control) {
  .gpio = GPIO_DT_SPEC_GET(ARDUINO_SPI_NODE,cs_gpios),
  .delay = 0u,
};

// SPI device struct (used for screen draw)
const struct device * dev = DEVICE_DT_GET(ARDUINO_SPI_NODE);
const struct spi_config spi_cfg = {
  .frequency = 15000000,
  .operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
  .slave = 0,
  .cs = cs_ctrl
};
// I2C device struct (used for touch)
#define ARDUINO_I2C_NODE DT_NODELABEL(arduino_i2c)
const struct device * dev_i2c = DEVICE_DT_GET(ARDUINO_I2C_NODE);

```

After this step, you can run the following commands in your main function, before you call ANY commands. You should only run these once.

```c
// Check if everything is working correctly (NOTE: i am not working with a CS pin, as my code only uses SPI for the display writing, so it is unnecessary. If you need it, make sure to configure it here)
if(!device_is_ready(dev))
    return 0;
if(!gpio_is_ready_dt(&dcx_gpio))
    return 0;
if(gpio_pin_configure_dt(&dcx_gpio,GPIO_OUTPUT_LOW))
    return 0;
if(0 > i2c_configure(dev_i2c,I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER))
    return 0;

// And finally...
J_init(dev,dev_i2c,&spi_cfg,&dcx_gpio,bounds);
J_LCD_init();
```

And you are done, you may begin using the library and all of its functions!

The basics of this library are summed up below, for a more comprehensive guide, you can study the code of this github.

## 1.2 | How to create a component
Components can any of the following types: `J_BUTTON`, `J_SHAPE`, `J_TEXT`, `J_IMAGE`, `J_DECAL`, `J_BAR`, and `J_FILL`.

For `J_SHAPE`, the only currently working shape is RECTANGLE. So be aware that triangles or circles will not work (coming in a future update).

Each component has the same structure properties. That is shown below:
```c
struct j_component {
  char* name; // Name of component, used for keeping track
  j_type type; // Type of component, as mentioned above
  uint16_t x, y; // x and y coordinates
  void* dat; // Often used for direct data, such as text contents or image data.
  void* dat2; // Often used for cosmetic data, such as colors, size, etc...
  // Used for tracking the components in the linked list array, you do not need to worry about this, the code logic will work with these automatically
  j_component* next_ptr;
  j_component* prev_ptr;
};
```

To create a component, you use the command `j_component* create_component(char* name, j_type type, uint16_t x, uint16_t y, void* dat, void* dat2);`, this will return a pointer to a j_component, keep track of this, as you can use this to modify your components later, as well as draw them to the screen.

To add a component to the screen array, you can use `void add_component_o(j_component* component);`. It is important to note that when adding multiple components, the most recently added component will be prioritized on the foreground when drawn.

To remove a component from the screen array, you can use `uint8_t remove_component_o(j_component* component);` **NOTE:** This will not deallocate the component from memory, thus if you do this, you must also eventually call `void free_component(j_component* comp);`. This method of removing components is not recommended, a better method is described in **Section 1.4**. 

Further documentation can be found in the header file for `J_GL.h`

The majority of the work happens with the `dat` and `dat2` void pointers. These can take different types of data, and will be dereferenced in the graphics library to draw your components.

## 1.3 | Component data specifications

### J_BUTTON
+ `dat`: Needs to be a pointer to an array of characters, terminated by a null character. This will be your text shown on the button. If you wish to put an image on the button, you can simply set this to be the empty string `""` and leave it at that. Do not set this to `NULL`.
+ `dat2`: A pointer to a `j_button_data` struct. You can leave this set to `NULL`, which will then give the button a default data struct.

### J_SHAPE
+ `dat`: Needs to be a pointer to a `j_shape_data` struct. This cannot be set to `NULL`.
+ `dat2`: Needs to be a pointer to a `j_animation_data` struct, or `NULL` pointer. The only working animations for J_SHAPE is `RESIZE` as of now.

### J_TEXT
+ `dat`: Needs to be a pointer to an array of characters, terminated by a null character. This will be your text contents. If you want to update the text at runtime, it is recommended to use `void update_text(j_component* text_component, char* new_str);`
+ `dat2`: Needs to be a pointer to `j_text_data` or `NULL`. 

### J_IMAGE
+ `dat`: Needs to be a pointer to a `uint8_t` array of image data, generated using the custom `cnv_to_bit.py` script seen in this repo. This is explained further in **1.5**.
+ `dat2`: Needs to be a pointer to `j_animation_data` struct or a `NULL` pointer. The valid animations for images are `FADE_IN` and `FADE_OUT`

### J_DECAL
+ `dat`: Needs to be a pointer to a `uint8_t` array of decal data, generated using the custom `cnv_to_bitmap.py` script seen in this repo. This is explained further in **1.5**.
+ `dat2`: Needs to be a pointer to 'j_decal_data' struct or a 'NULL' pointer, which sets the data to the default decal data.

### J_BAR
+ `dat`: Needs to be a pointer to a `uint8_t` variable. This will be the value used to fill the bar. It needs to be within the range of 0 to 100 at all times.
+ `dat2`: Needs to be a pointer to a `j_bar_data` struct.

### J_FILL
+ `x` and `y`: Don't matter, J_FILL fills the entire screen with a color
+ `dat`: Needs to be a pointer to a `j_color` variable. This will be used as the color of the fill.
+ `dat2`: Always set to `NULL`.

## 1.4 | Screen Drawing

### Drawing components
To draw to the screen, you use the `void draw_screen_o(int8_t* exclude_list, size_t len);` function.

The function takes two arguments, `exclude_list` and `len`.
The exclude list is an array of the elements you don't want to be printed. (Ex. `J_BUTTON`)
The len variable is the length of the list.

If you want to draw everything, you don't have to worry about these arguments at all, and can just call `draw_screen_o(NULL,0)`.

If you want to draw just one component (very useful for when you update a components j_XX_data structs) you simply use `void draw_component(j_component* component)` (NOTE: Dont use this for J_TEXT, if you want to update J_TEXT, use the `update_text` function seen in **1.3**)

### Changing component layers
The screen drawing pipeline will iteratively go through the entire linked list component array (managed by the library) and draw each component added with `add_component_o` consecutively. Thus, it will draw the most recently added component on the foreground. If you ever wish to change the order. You can simply call the `void change_component_index_o(j_component* component, uint8_t new_index)` function. `new_index` has to be greater than or equal to 0 (which is the lowest layer, drawn first). It doesn't really matter if you go above the current component size for `new_index`, as the function will just stop when it reaches the end of the list and place the component on the foreground.

### Removing all components from screen. (Deallocation)
(See **1.2** for function definitions)
Every component you create is allocated on the heap. Thus it is very important that you deallocate them when not in use. The J_GL library automatically does this for you, provided you have added the components to the screen buffer via `add_component_o(j_component* component)`. You simply need to call the `void clear_draw_buffer();` command, and it will take care of deallocating every component on the heap.

If you want to remove a singular component, it is still recommended to use this method. A good strategy is to use the `remove_component_o(j_component* component)` and then redraw the screen. When you want to deallocate everything, you would use `add_component_o(j_component* component)` to bring this component back in and `clear_draw_buffer()` to deallocate everything.

### Recommendation
My recommendation is to use Zephyrs State Machines for this library, they work well with the State Machine pipeline and can be implemented generally like this for each state function.

```c

void state_1_enter(void* o) {
    // Define j_data structs
    // Create components using create_component(), and link their respective j_data structs
    // draw screen
}

void state_1_run(void* o) {
    // Main loop logic
}

void state_1_exit(void* o) {
    // IMPORTANT: Make sure you add EVERY initialized component (not on the draw buffer, and that you have also not already deallocated) to the draw array using add_component_o(j_component* component) and then use clear_draw_buffer() to deallocate everything. OR you can deallocate eavrything yourself manually using the library defined free_component(j_component* component)
}

```

Also note that as seen in **1.2**, each component data type `dat` and `dat2` is passed as a pointer, not as a scalar. So you have to make sure that your data is still in the program memory, and not deallocated when the function ends.

In my implementation, I used a union of different structs to store the data I needed for each page of my application. But the method may vary from person to person.

## 1.5 | Image conversion to J_IMAGE or J_DECAL format
As shown previously, the `dat` variable of the `J_IMAGE` and `J_DECAL` point to uint8_t arrays of data, which depict images converted into 6-bit or 1-bit images, respectively.

### J_IMAGE
+ Built for showing images with lots of color detail. It can support FADE_OUT and FADE_IN animation types.
+ Drawback is that it takes up a lot of space, therefore you are much more limited in how many `J_IMAGE`'s you can have in your project.
+ Not possible to modify direct color values for this data type.

### J_DECAL
+ Built for showing simple images (like icons) on the screen.
+ Uses each bit in a byte to show either an ON or OFF state on an image pixel.
+ Much more customizable in terms of colors, simply by changing the `col` and `bg_col` values of the `j_decal_data` struct, you can redraw the screen and the `J_DECAL` will have a different color
+ Since it is 1-bit color space, you cannot have detailed images.
+ Format for conversion is if the Red value of a pixel is greater than 120 (on a 0-255 scale), it will fill in with `col`, and `bg_col` if it is less than or equal to 120.

### Converting
To convert an image to `J_IMAGE` or `J_DECAL`, you need to use `cnv_to_bit.py` or `cnv_to_bitmap.py` respectively. 

Use the terminal to run these applications, as they both require two arguments, the general format is shown here:

`python cnv_to_bit.py <path_to_image> <name_of_generated_array>`
`python cnv_to_bitmap.py <path_to_image> <name_of_generated_array>`

Note that your path_to_image must end with .png or any image format.
Also, the name of your generated array can be anything, it just helps you set the name as you convert instead of needing to do it yourself everytime.

The python code will generate a `<page_to_image>-converted.txt` file in the same directory as where you run your python script. This will contain your converted array, which you can copy and paste into your code. It is important that you use the keyword `const` when declaring these variables (automatically done for you) because they will be stored in Flash memory. The J_GL library automatically loads these into ram for efficient drawing when necessary.

## Roadmap for J Graphics Library

+ SOON: Fix up the printk statements, currently your serial monitor is bombarded with random debug messages lol. Ill fix that as soon as I have time.
+ April 2026: Comprehensive shapes, such as circles, triangles, hexagons, etc...


# 2 | SojournOS (Application)

This is SojournOS, a passion project made by Jonart Bajraktari to showcase J_GL and its features. As of right now, it features a playable snake game, theme switching, a lock screen, an about page, and context buttons.

## Roadmap for SojournOS

+ May 2026: Whatsyapp - The bluetooth messaging app - and Calculator - do I need to explain this one?


Thanks for reading! I hope you enjoy the library and/or the basic OS I have made.
