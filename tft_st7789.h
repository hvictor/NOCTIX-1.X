/******************************************************
 * ST7789 TFT Driver for PIC32 MZ
 * ****************************************************
 * File:    tft_st7789.h
 * Date:    20.07.2021
 * Author:  Victor Huerlimann, Ribes Microsystems
 * Note:    Ported to PIC32 from Adafruit's driver
 ******************************************************/

#ifndef _TFT_ST7789_H
#define _TFT_ST7789_H

/*******************************************************
 * TFT module configuration
 *******************************************************/
//#define TFT_CONFIG_USE_SDCARD

/* End TFT module configuration */

#include <stdint.h>

// Default Colors
#define TFT_COLOR_BLACK     0x0000
#define TFT_COLOR_WHITE     0xFFFF
#define TFT_COLOR_RED       0xF800
#define TFT_COLOR_GREEN     0x07E0
#define TFT_COLOR_BLUE      0x001F
#define TFT_COLOR_CYAN      0x07FF
#define TFT_COLOR_MAGENTA   0xF81F
#define TFT_COLOR_YELLOW    0xFFE0
#define TFT_COLOR_ORANGE    0xFC00

#define TFT_COLOR_LUNAR_BLUE_DARK tft_color_u16(120, 120, 255)
#define TFT_COLOR_LUNAR_BLUE_LIGHT tft_color_u16(200, 200, 255)

/******************************************************
 * Data Structures
 ******************************************************/
typedef struct
{
    uint8_t Height;
    uint8_t Width;
    uint16_t **Data;
} TFT_Image;

void tft_init(uint16_t width, uint16_t height);
void tft_fill_screen(uint16_t color);
void tft_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void tft_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void tft_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void tft_fill_half_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void tft_fill_circle_slice(int16_t cx, int16_t cy);
uint8_t tft_write_char(uint8_t c);
void tft_set_text_color(uint16_t c);
void tft_set_text_bg_color(uint16_t c, uint16_t bg);
void tft_set_cursor(int16_t x, int16_t y);
void tft_set_text_size_independent(uint8_t s_x, uint8_t s_y);
void tft_set_text_size(uint8_t s);
void tft_test_bitmap();
uint16_t tft_color_u16(uint8_t r, uint8_t g, uint8_t b);
void tft_render_image_raw(uint8_t *data, int x, int y, int width, int height);
void tft_render_image(TFT_Image image, int x, int y);
void tft_printf(char *format, ...);
uint8_t tft_get_cursor_x();
uint8_t tft_get_cursor_y();
uint8_t tft_get_char_pixels_x();
uint8_t tft_get_char_pixels_y();
void tft_draw_pixel_buffer(int16_t x, int16_t y, uint16_t color);

#ifdef TFT_CONFIG_USE_SDCARD
void tft_render_image_sdcard(char *filename, int x, int y, int width, int height);
#endif

#endif
