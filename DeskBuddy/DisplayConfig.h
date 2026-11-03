#pragma once
// NM-TV-154 display wiring from NMTech's custom firmware guide.
#define USER_SETUP_LOADED 1
#define ST7789_DRIVER
#define TFT_WIDTH 240
#define TFT_HEIGHT 240
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS 15
#define TFT_DC 2
#define TFT_RST -1
#define TFT_MISO -1
#define USE_HSPI_PORT
#define CGRAM_OFFSET
// This panel expects BGR: RGB made yellow/orange artwork appear cyan/blue.
#define TFT_RGB_ORDER TFT_BGR
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 20000000
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define SMOOTH_FONT
