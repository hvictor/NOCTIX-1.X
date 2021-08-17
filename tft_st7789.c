/******************************************************
 * ST7789 TFT Driver for PIC32 MZ
 * ****************************************************
 * File:    tft_st7789.c
 * Date:    20.07.2021
 * Author:  Victor Huerlimann, Ribes Microsystems
 * Note:    Ported to PIC32 from Adafruit's driver
 ******************************************************/

#include <stdint.h>
#include <xc.h>
#include <stdarg.h>
#include "tft_st7789.h"
#include "BSP.h"

#ifdef TFT_CONFIG_USE_SDCARD
#include "ff.h"
#include "diskio.h"
#endif

#define ST_CMD_DELAY 0x80 // special signifier for command lists

#define ST77XX_NOP 0x00
#define ST77XX_SWRESET 0x01
#define ST77XX_RDDID 0x04
#define ST77XX_RDDST 0x09

#define ST77XX_SLPIN 0x10
#define ST77XX_SLPOUT 0x11
#define ST77XX_PTLON 0x12
#define ST77XX_NORON 0x13

#define ST77XX_INVOFF 0x20
#define ST77XX_INVON 0x21
#define ST77XX_DISPOFF 0x28
#define ST77XX_DISPON 0x29
#define ST77XX_CASET 0x2A
#define ST77XX_RASET 0x2B
#define ST77XX_RAMWR 0x2C
#define ST77XX_RAMRD 0x2E

#define ST77XX_PTLAR 0x30
#define ST77XX_TEOFF 0x34
#define ST77XX_TEON 0x35
#define ST77XX_MADCTL 0x36
#define ST77XX_COLMOD 0x3A

#define ST77XX_MADCTL_MY 0x80
#define ST77XX_MADCTL_MX 0x40
#define ST77XX_MADCTL_MV 0x20
#define ST77XX_MADCTL_ML 0x10
#define ST77XX_MADCTL_RGB 0x00

#define ST77XX_RDID1 0xDA
#define ST77XX_RDID2 0xDB
#define ST77XX_RDID3 0xDC
#define ST77XX_RDID4 0xDD

// Externals
void BSP_Delay_ms(int ms);

// Macros
#define _swap_int16_t(a, b)                                                    \
  {                                                                            \
    int16_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }

static int TFT_CS;
static int TFT_DC;
static int TFT_RST;

uint16_t _colstart = 0;
uint16_t _rowstart = 0;
uint16_t _colstart2 = 0;
uint16_t _rowstart2 = 0; 

uint16_t _xstart;
uint16_t _ystart;
uint16_t _width;
uint16_t _height;

uint16_t windowWidth;
uint16_t windowHeight;
uint16_t invertOnCommand;
uint16_t invertOffCommand;
uint8_t rotation;

void SPI_CS_LOW()
{
    BSP_Pin_TFT_CS = 0;
}

void SPI_CS_HIGH()
{
    BSP_Pin_TFT_CS = 1;
}

void SPI_DC_LOW()
{
    BSP_Pin_TFT_DC = 0;
}

void SPI_DC_HIGH()
{
    BSP_Pin_TFT_DC = 1;
}    

uint8_t spiWrite(uint8_t b) {
    BSP_Register_TFT_SPIBUF = b;
    while(!BSP_Register_TFT_SPISTAT.SPIRBF) ;
    return BSP_Register_TFT_SPIBUF;    
}

void sendCommand(uint8_t commandByte, uint8_t *dataBytes, uint8_t numDataBytes)
{
    SPI_CS_LOW();

    SPI_DC_LOW(); // Command mode
    spiWrite(commandByte); // Send the command byte

    SPI_DC_HIGH();
    for (int i = 0; i < numDataBytes; i++) {
        spiWrite(*dataBytes); // Send the data bytes
        dataBytes++;
    }

    SPI_CS_HIGH();
}

void __tft_set_rotation(uint8_t m) {
    uint8_t madctl = 0;

    rotation = m & 3; // can't be higher than 3

    switch (rotation) {
        case 0:
            madctl = ST77XX_MADCTL_MX | ST77XX_MADCTL_MY | ST77XX_MADCTL_RGB;
            _xstart = _colstart;
            _ystart = _rowstart;
            _width = windowWidth;
            _height = windowHeight;
            break;
        case 1:
            madctl = ST77XX_MADCTL_MY | ST77XX_MADCTL_MV | ST77XX_MADCTL_RGB;
            _xstart = _rowstart;
            _ystart = _colstart;
            _height = windowWidth;
            _width = windowHeight;
            break;
        case 2:
            madctl = ST77XX_MADCTL_RGB;
            _xstart = _colstart2;
            _ystart = _rowstart2;
            _width = windowWidth;
            _height = windowHeight;
            break;
        case 3:
            madctl = ST77XX_MADCTL_MX | ST77XX_MADCTL_MV | ST77XX_MADCTL_RGB;
            _xstart = _rowstart2;
            _ystart = _colstart2;
            _height = windowWidth;
            _width = windowHeight;
            break;
    }

    sendCommand(ST77XX_MADCTL, &madctl, 1);
}
/*!
 @brief   Adafruit_SPITFT Send Command handles complete sending of commands and
 data
 @param   commandByte       The Command Byte
 @param   dataBytes         A pointer to the Data bytes to send
 @param   numDataBytes      The number of bytes we should send
 */

void sendConstCommand(uint8_t commandByte, const uint8_t *dataBytes, uint8_t numDataBytes)
{
    //SPI_BEGIN_TRANSACTION();
    SPI_CS_LOW();

    SPI_DC_LOW(); // Command mode
    spiWrite(commandByte); // Send the command byte

    SPI_DC_HIGH();
    for (int i = 0; i < numDataBytes; i++) {
        spiWrite(*(dataBytes++));
    }

    SPI_CS_HIGH();
    //SPI_END_TRANSACTION();
}

static const uint8_t st7789_init_sequence[] =
{// Init commands for 7789 screens
    9, //  9 commands in list:
    ST77XX_SWRESET, ST_CMD_DELAY, //  1: Software reset, no args, w/delay
    150, //     ~150 ms delay
    ST77XX_SLPOUT, ST_CMD_DELAY, //  2: Out of sleep mode, no args, w/delay
    10, //      10 ms delay
    ST77XX_COLMOD, 1 + ST_CMD_DELAY, //  3: Set color mode, 1 arg + delay:
    0x55, //     16-bit color
    10, //     10 ms delay
    ST77XX_MADCTL, 1, //  4: Mem access ctrl (directions), 1 arg:
    0x08, //     Row/col addr, bottom-top refresh
    ST77XX_CASET, 4, //  5: Column addr set, 4 args, no delay:
    0x00,
    0, //     XSTART = 0
    0,
    240, //     XEND = 240
    ST77XX_RASET, 4, //  6: Row addr set, 4 args, no delay:
    0x00,
    0, //     YSTART = 0
    320 >> 8,
    320 & 0xFF, //     YEND = 320
    ST77XX_INVON, ST_CMD_DELAY, //  7: hack
    10,
    ST77XX_NORON, ST_CMD_DELAY, //  8: Normal display on, no args, w/delay
    10, //     10 ms delay
    ST77XX_DISPON, ST_CMD_DELAY, //  9: Main screen turn on, no args, delay
    10
};  

void begin() {
  invertOnCommand = ST77XX_INVON;
  invertOffCommand = ST77XX_INVOFF;
}

void __tft_display_init(const uint8_t *addr) {

    uint8_t numCommands, cmd, numArgs;
    uint16_t ms;

    numCommands = *(addr++); // Number of commands to follow
    while (numCommands--) { // For each command...
        cmd = *(addr++); // Read command
        numArgs = *(addr++); // Number of args to follow
        ms = numArgs & ST_CMD_DELAY; // If hibit set, delay follows args
        numArgs &= ~ST_CMD_DELAY; // Mask out delay bit
        sendConstCommand(cmd, addr, numArgs);
        addr += numArgs;

        if (ms) {
            ms = *(addr++); // Read post-command delay time (ms)
            if (ms == 255)
                ms = 500; // If 255, delay for 500 ms
            BSP_Delay_ms(ms);
        }
    }
}

void __tft_common_init(const uint8_t *cmdList) {
  begin();

  if (cmdList) {
    __tft_display_init(cmdList);
  }
}

void tft_init(uint16_t width, uint16_t height)
{
    __tft_common_init(0);

    _rowstart = (320 - height);
    _rowstart2 = 0;
    _colstart = _colstart2 = (240 - width);

    windowWidth = width;
    windowHeight = height;

    __tft_display_init(st7789_init_sequence);
    __tft_set_rotation(0);
}

void startWrite(void) {
    SPI_CS_LOW();
}

/*!
    @brief  Call after issuing command(s) or data to display. Performs
            chip-deselect (if required) and ends an SPI transaction (if
            using hardware SPI and transactions are supported). Required
            for all display types; not an SPI-specific function.
*/
void endWrite(void) {
    SPI_CS_HIGH();
}

void writeCommand(uint8_t cmd)
{
  SPI_DC_LOW();
  spiWrite(cmd);
  SPI_DC_HIGH();
}

void SPI_WRITE32(uint32_t l) {
    spiWrite(l >> 24);
    spiWrite(l >> 16);
    spiWrite(l >> 8);
    spiWrite(l);
}

void SPI_WRITE16(uint16_t w) {
    spiWrite(w >> 8);
    spiWrite(w);
}

void writeColor(uint16_t color, uint32_t len) {

  if (!len)
    return; // Avoid 0-byte transfers

  uint8_t hi = color >> 8, lo = color;
    while (len--) {
      spiWrite(hi);
      spiWrite(lo);
    }
}

/**************************************************************************/
/*!
  @brief  SPI displays set an address window rectangle for blitting pixels
  @param  x  Top left corner x coordinate
  @param  y  Top left corner x coordinate
  @param  w  Width of window
  @param  h  Height of window
*/
/**************************************************************************/
void setAddrWindow(uint16_t x, uint16_t y, uint16_t w,
                                    uint16_t h) {
  x += _xstart;
  y += _ystart;
  uint32_t xa = ((uint32_t)x << 16) | (x + w - 1);
  uint32_t ya = ((uint32_t)y << 16) | (y + h - 1);

  writeCommand(ST77XX_CASET); // Column addr set
  SPI_WRITE32(xa);

  writeCommand(ST77XX_RASET); // Row addr set
  SPI_WRITE32(ya);

  writeCommand(ST77XX_RAMWR); // write to RAM
}

void writeFillRectPreclipped(int16_t x, int16_t y,
                                                     int16_t w, int16_t h,
                                                     uint16_t color)
{
  setAddrWindow(x, y, w, h);
  writeColor(color, (uint32_t)w * h);
}

void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    if ((x >= 0) && (x < _width) && h) { // X on screen, nonzero height
        if (h < 0) { // If negative height...
            y += h + 1; //   Move Y to top edge
            h = -h; //   Use positive height
        }
        if (y < _height) { // Not off bottom
            int16_t y2 = y + h - 1;
            if (y2 >= 0) { // Not off top
                // Line partly or fully overlaps screen
                if (y < 0) {
                    y = 0;
                    h = y2 + 1;
                } // Clip top
                if (y2 >= _height) {
                    h = _height - y;
                } // Clip bottom
                writeFillRectPreclipped(x, y, 1, h, color);
            }
        }
    }
}

void tft_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  startWrite();
  for (int16_t i = x; i < x + w; i++) {
    writeFastVLine(i, y, h, color);
  }
  endWrite();
}

void tft_fill_screen(uint16_t color) {
  tft_fill_rect(0, 0, _width, _height, color);
}


/*
 ********************************************************
 * Ribes GFX
 * 
 * 
 * 
 * 
 * ******************************************************
 */

void __tft_draw_pixel(int16_t x, int16_t y, uint16_t color) {
    // Clipping
    if ((x >= 0) && (x < _width) && (y >= 0) && (y < _height)) {
        startWrite();
        setAddrWindow(x, y, 1, 1);
        SPI_WRITE16(color);
        endWrite();
    }
}

void __tft_write_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        _swap_int16_t(x0, y0);
        _swap_int16_t(x1, y1);
    }

    if (x0 > x1) {
        _swap_int16_t(x0, x1);
        _swap_int16_t(y0, y1);
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    for (; x0 <= x1; x0++) {
        if (steep) {
            __tft_draw_pixel(y0, x0, color);
        } else {
            __tft_draw_pixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}
 
void drawFastVLine(int16_t x, int16_t y, int16_t h,
                                 uint16_t color) {
  startWrite();
  __tft_write_line(x, y, x, y + h - 1, color);
  endWrite();
}

void drawFastHLine(int16_t x, int16_t y, int16_t w,
                                    uint16_t color) {
  if ((y >= 0) && (y < _height) && w) { // Y on screen, nonzero width
    if (w < 0) {                        // If negative width...
      x += w + 1;                       //   Move X to left edge
      w = -w;                           //   Use positive width
    }
    if (x < _width) { // Not off right
      int16_t x2 = x + w - 1;
      if (x2 >= 0) { // Not off left
        // Line partly or fully overlaps screen
        if (x < 0) {
          x = 0;
          w = x2 + 1;
        } // Clip left
        if (x2 >= _width) {
          w = _width - x;
        } // Clip right
        startWrite();
        writeFillRectPreclipped(x, y, w, 1, color);
        endWrite();
      }
    }
  }
}

void tft_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    if (x0 == x1) {
        if (y0 > y1)
            _swap_int16_t(y0, y1);
        drawFastVLine(x0, y0, y1 - y0 + 1, color);
    } else if (y0 == y1) {
        if (x0 > x1)
            _swap_int16_t(x0, x1);
        drawFastHLine(x0, y0, x1 - x0 + 1, color);
    } else {
        startWrite();
        __tft_write_line(x0, y0, x1, y1, color);
        endWrite();
    }
}

void tft_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    tft_draw_line(x, y, x + w, y, color);
    tft_draw_line(x + w, y, x + w, y + h, color);
    tft_draw_line(x + w, y + h, x, y + h, color);
    tft_draw_line(x, y + h, x, y, color);
}

void tft_draw_pixel_buffer(int16_t x, int16_t y, uint16_t color) {
    // Clipping
    if ((x >= 0) && (x < _width) && (y >= 0) && (y < _height)) {
        startWrite();
        setAddrWindow(x, y, 1, 1);
        SPI_WRITE16(color);
        endWrite();
    }
}

void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    startWrite();
    __tft_draw_pixel(x0, y0 + r, color);
    __tft_draw_pixel(x0, y0 - r, color);
    __tft_draw_pixel(x0 + r, y0, color);
    __tft_draw_pixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        __tft_draw_pixel(x0 + x, y0 + y, color);
        __tft_draw_pixel(x0 - x, y0 + y, color);
        __tft_draw_pixel(x0 + x, y0 - y, color);
        __tft_draw_pixel(x0 - x, y0 - y, color);
        __tft_draw_pixel(x0 + y, y0 + x, color);
        __tft_draw_pixel(x0 - y, y0 + x, color);
        __tft_draw_pixel(x0 + y, y0 - x, color);
        __tft_draw_pixel(x0 - y, y0 - x, color);
    }
    endWrite();
}

void __tft_half_circle_helper(int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    int16_t px = x;
    int16_t py = y;

    delta++; // Avoid some +1's in the loop
    int z = 0;

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        // These checks avoid double-drawing certain lines, important
        // for the SSD1306 library which has an INVERT drawing mode.
        if (x < (y + 1)) {
            if (corners & 1)
                writeFastVLine(x0 + x, y0 - y, 1 * y + delta, color);
            if (corners & 2)
                writeFastVLine(x0 - x, y0 - y, 1 * y + delta, color);
        }
        if (y != py) {
            if (corners & 1)
                writeFastVLine(x0 + py, y0 - px, 1 * px + delta, color);
            if (corners & 2)
                writeFastVLine(x0 - py, y0 - px, 1 * px + delta, color);
            py = y;
        }
        px = x;
        
        z++;
    }
}


void tft_fill_half_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
  startWrite();
  writeFastVLine(x0, y0 - r, r + 1, color);
  __tft_half_circle_helper(x0, y0, r, 3, 0, color);
  endWrite();
}

void tft_fill_circle_slice(int16_t cx, int16_t cy)
{
    int16_t offset = 40;
    int16_t startx = cx - offset;
    int16_t endx = cx + offset;
    int16_t x = startx;
    int16_t y = cy;
    
    startWrite();
    
    while (x < endx) {
        writeFastVLine(x, y, 40, TFT_COLOR_GREEN);
        
        if (x < cx) y++;
        else if (x > cx) y--;
        x++;
    }
    
    endWrite();
}

/*
 ********************************************************
 * Text Rendering
 * 
 * ******************************************************
 */
static const unsigned char font[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x5B, 0x4F, 0x5B, 0x3E, 0x3E, 0x6B,
    0x4F, 0x6B, 0x3E, 0x1C, 0x3E, 0x7C, 0x3E, 0x1C, 0x18, 0x3C, 0x7E, 0x3C,
    0x18, 0x1C, 0x57, 0x7D, 0x57, 0x1C, 0x1C, 0x5E, 0x7F, 0x5E, 0x1C, 0x00,
    0x18, 0x3C, 0x18, 0x00, 0xFF, 0xE7, 0xC3, 0xE7, 0xFF, 0x00, 0x18, 0x24,
    0x18, 0x00, 0xFF, 0xE7, 0xDB, 0xE7, 0xFF, 0x30, 0x48, 0x3A, 0x06, 0x0E,
    0x26, 0x29, 0x79, 0x29, 0x26, 0x40, 0x7F, 0x05, 0x05, 0x07, 0x40, 0x7F,
    0x05, 0x25, 0x3F, 0x5A, 0x3C, 0xE7, 0x3C, 0x5A, 0x7F, 0x3E, 0x1C, 0x1C,
    0x08, 0x08, 0x1C, 0x1C, 0x3E, 0x7F, 0x14, 0x22, 0x7F, 0x22, 0x14, 0x5F,
    0x5F, 0x00, 0x5F, 0x5F, 0x06, 0x09, 0x7F, 0x01, 0x7F, 0x00, 0x66, 0x89,
    0x95, 0x6A, 0x60, 0x60, 0x60, 0x60, 0x60, 0x94, 0xA2, 0xFF, 0xA2, 0x94,
    0x08, 0x04, 0x7E, 0x04, 0x08, 0x10, 0x20, 0x7E, 0x20, 0x10, 0x08, 0x08,
    0x2A, 0x1C, 0x08, 0x08, 0x1C, 0x2A, 0x08, 0x08, 0x1E, 0x10, 0x10, 0x10,
    0x10, 0x0C, 0x1E, 0x0C, 0x1E, 0x0C, 0x30, 0x38, 0x3E, 0x38, 0x30, 0x06,
    0x0E, 0x3E, 0x0E, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5F,
    0x00, 0x00, 0x00, 0x07, 0x00, 0x07, 0x00, 0x14, 0x7F, 0x14, 0x7F, 0x14,
    0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x23, 0x13, 0x08, 0x64, 0x62, 0x36, 0x49,
    0x56, 0x20, 0x50, 0x00, 0x08, 0x07, 0x03, 0x00, 0x00, 0x1C, 0x22, 0x41,
    0x00, 0x00, 0x41, 0x22, 0x1C, 0x00, 0x2A, 0x1C, 0x7F, 0x1C, 0x2A, 0x08,
    0x08, 0x3E, 0x08, 0x08, 0x00, 0x80, 0x70, 0x30, 0x00, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x00, 0x00, 0x60, 0x60, 0x00, 0x20, 0x10, 0x08, 0x04, 0x02,
    0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00, 0x42, 0x7F, 0x40, 0x00, 0x72, 0x49,
    0x49, 0x49, 0x46, 0x21, 0x41, 0x49, 0x4D, 0x33, 0x18, 0x14, 0x12, 0x7F,
    0x10, 0x27, 0x45, 0x45, 0x45, 0x39, 0x3C, 0x4A, 0x49, 0x49, 0x31, 0x41,
    0x21, 0x11, 0x09, 0x07, 0x36, 0x49, 0x49, 0x49, 0x36, 0x46, 0x49, 0x49,
    0x29, 0x1E, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x40, 0x34, 0x00, 0x00,
    0x00, 0x08, 0x14, 0x22, 0x41, 0x14, 0x14, 0x14, 0x14, 0x14, 0x00, 0x41,
    0x22, 0x14, 0x08, 0x02, 0x01, 0x59, 0x09, 0x06, 0x3E, 0x41, 0x5D, 0x59,
    0x4E, 0x7C, 0x12, 0x11, 0x12, 0x7C, 0x7F, 0x49, 0x49, 0x49, 0x36, 0x3E,
    0x41, 0x41, 0x41, 0x22, 0x7F, 0x41, 0x41, 0x41, 0x3E, 0x7F, 0x49, 0x49,
    0x49, 0x41, 0x7F, 0x09, 0x09, 0x09, 0x01, 0x3E, 0x41, 0x41, 0x51, 0x73,
    0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00, 0x41, 0x7F, 0x41, 0x00, 0x20, 0x40,
    0x41, 0x3F, 0x01, 0x7F, 0x08, 0x14, 0x22, 0x41, 0x7F, 0x40, 0x40, 0x40,
    0x40, 0x7F, 0x02, 0x1C, 0x02, 0x7F, 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x3E,
    0x41, 0x41, 0x41, 0x3E, 0x7F, 0x09, 0x09, 0x09, 0x06, 0x3E, 0x41, 0x51,
    0x21, 0x5E, 0x7F, 0x09, 0x19, 0x29, 0x46, 0x26, 0x49, 0x49, 0x49, 0x32,
    0x03, 0x01, 0x7F, 0x01, 0x03, 0x3F, 0x40, 0x40, 0x40, 0x3F, 0x1F, 0x20,
    0x40, 0x20, 0x1F, 0x3F, 0x40, 0x38, 0x40, 0x3F, 0x63, 0x14, 0x08, 0x14,
    0x63, 0x03, 0x04, 0x78, 0x04, 0x03, 0x61, 0x59, 0x49, 0x4D, 0x43, 0x00,
    0x7F, 0x41, 0x41, 0x41, 0x02, 0x04, 0x08, 0x10, 0x20, 0x00, 0x41, 0x41,
    0x41, 0x7F, 0x04, 0x02, 0x01, 0x02, 0x04, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x00, 0x03, 0x07, 0x08, 0x00, 0x20, 0x54, 0x54, 0x78, 0x40, 0x7F, 0x28,
    0x44, 0x44, 0x38, 0x38, 0x44, 0x44, 0x44, 0x28, 0x38, 0x44, 0x44, 0x28,
    0x7F, 0x38, 0x54, 0x54, 0x54, 0x18, 0x00, 0x08, 0x7E, 0x09, 0x02, 0x18,
    0xA4, 0xA4, 0x9C, 0x78, 0x7F, 0x08, 0x04, 0x04, 0x78, 0x00, 0x44, 0x7D,
    0x40, 0x00, 0x20, 0x40, 0x40, 0x3D, 0x00, 0x7F, 0x10, 0x28, 0x44, 0x00,
    0x00, 0x41, 0x7F, 0x40, 0x00, 0x7C, 0x04, 0x78, 0x04, 0x78, 0x7C, 0x08,
    0x04, 0x04, 0x78, 0x38, 0x44, 0x44, 0x44, 0x38, 0xFC, 0x18, 0x24, 0x24,
    0x18, 0x18, 0x24, 0x24, 0x18, 0xFC, 0x7C, 0x08, 0x04, 0x04, 0x08, 0x48,
    0x54, 0x54, 0x54, 0x24, 0x04, 0x04, 0x3F, 0x44, 0x24, 0x3C, 0x40, 0x40,
    0x20, 0x7C, 0x1C, 0x20, 0x40, 0x20, 0x1C, 0x3C, 0x40, 0x30, 0x40, 0x3C,
    0x44, 0x28, 0x10, 0x28, 0x44, 0x4C, 0x90, 0x90, 0x90, 0x7C, 0x44, 0x64,
    0x54, 0x4C, 0x44, 0x00, 0x08, 0x36, 0x41, 0x00, 0x00, 0x00, 0x77, 0x00,
    0x00, 0x00, 0x41, 0x36, 0x08, 0x00, 0x02, 0x01, 0x02, 0x04, 0x02, 0x3C,
    0x26, 0x23, 0x26, 0x3C, 0x1E, 0xA1, 0xA1, 0x61, 0x12, 0x3A, 0x40, 0x40,
    0x20, 0x7A, 0x38, 0x54, 0x54, 0x55, 0x59, 0x21, 0x55, 0x55, 0x79, 0x41,
    0x22, 0x54, 0x54, 0x78, 0x42, // a-umlaut
    0x21, 0x55, 0x54, 0x78, 0x40, 0x20, 0x54, 0x55, 0x79, 0x40, 0x0C, 0x1E,
    0x52, 0x72, 0x12, 0x39, 0x55, 0x55, 0x55, 0x59, 0x39, 0x54, 0x54, 0x54,
    0x59, 0x39, 0x55, 0x54, 0x54, 0x58, 0x00, 0x00, 0x45, 0x7C, 0x41, 0x00,
    0x02, 0x45, 0x7D, 0x42, 0x00, 0x01, 0x45, 0x7C, 0x40, 0x7D, 0x12, 0x11,
    0x12, 0x7D, // A-umlaut
    0xF0, 0x28, 0x25, 0x28, 0xF0, 0x7C, 0x54, 0x55, 0x45, 0x00, 0x20, 0x54,
    0x54, 0x7C, 0x54, 0x7C, 0x0A, 0x09, 0x7F, 0x49, 0x32, 0x49, 0x49, 0x49,
    0x32, 0x3A, 0x44, 0x44, 0x44, 0x3A, // o-umlaut
    0x32, 0x4A, 0x48, 0x48, 0x30, 0x3A, 0x41, 0x41, 0x21, 0x7A, 0x3A, 0x42,
    0x40, 0x20, 0x78, 0x00, 0x9D, 0xA0, 0xA0, 0x7D, 0x3D, 0x42, 0x42, 0x42,
    0x3D, // O-umlaut
    0x3D, 0x40, 0x40, 0x40, 0x3D, 0x3C, 0x24, 0xFF, 0x24, 0x24, 0x48, 0x7E,
    0x49, 0x43, 0x66, 0x2B, 0x2F, 0xFC, 0x2F, 0x2B, 0xFF, 0x09, 0x29, 0xF6,
    0x20, 0xC0, 0x88, 0x7E, 0x09, 0x03, 0x20, 0x54, 0x54, 0x79, 0x41, 0x00,
    0x00, 0x44, 0x7D, 0x41, 0x30, 0x48, 0x48, 0x4A, 0x32, 0x38, 0x40, 0x40,
    0x22, 0x7A, 0x00, 0x7A, 0x0A, 0x0A, 0x72, 0x7D, 0x0D, 0x19, 0x31, 0x7D,
    0x26, 0x29, 0x29, 0x2F, 0x28, 0x26, 0x29, 0x29, 0x29, 0x26, 0x30, 0x48,
    0x4D, 0x40, 0x20, 0x38, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x38, 0x2F, 0x10, 0xC8, 0xAC, 0xBA, 0x2F, 0x10, 0x28, 0x34, 0xFA, 0x00,
    0x00, 0x7B, 0x00, 0x00, 0x08, 0x14, 0x2A, 0x14, 0x22, 0x22, 0x14, 0x2A,
    0x14, 0x08, 0x55, 0x00, 0x55, 0x00, 0x55, // #176 (25% block) missing in old
                                              // code
    0xAA, 0x55, 0xAA, 0x55, 0xAA,             // 50% block
    0xFF, 0x55, 0xFF, 0x55, 0xFF,             // 75% block
    0x00, 0x00, 0x00, 0xFF, 0x00, 0x10, 0x10, 0x10, 0xFF, 0x00, 0x14, 0x14,
    0x14, 0xFF, 0x00, 0x10, 0x10, 0xFF, 0x00, 0xFF, 0x10, 0x10, 0xF0, 0x10,
    0xF0, 0x14, 0x14, 0x14, 0xFC, 0x00, 0x14, 0x14, 0xF7, 0x00, 0xFF, 0x00,
    0x00, 0xFF, 0x00, 0xFF, 0x14, 0x14, 0xF4, 0x04, 0xFC, 0x14, 0x14, 0x17,
    0x10, 0x1F, 0x10, 0x10, 0x1F, 0x10, 0x1F, 0x14, 0x14, 0x14, 0x1F, 0x00,
    0x10, 0x10, 0x10, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x10, 0x10, 0x10,
    0x10, 0x1F, 0x10, 0x10, 0x10, 0x10, 0xF0, 0x10, 0x00, 0x00, 0x00, 0xFF,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0xFF, 0x10, 0x00,
    0x00, 0x00, 0xFF, 0x14, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x1F,
    0x10, 0x17, 0x00, 0x00, 0xFC, 0x04, 0xF4, 0x14, 0x14, 0x17, 0x10, 0x17,
    0x14, 0x14, 0xF4, 0x04, 0xF4, 0x00, 0x00, 0xFF, 0x00, 0xF7, 0x14, 0x14,
    0x14, 0x14, 0x14, 0x14, 0x14, 0xF7, 0x00, 0xF7, 0x14, 0x14, 0x14, 0x17,
    0x14, 0x10, 0x10, 0x1F, 0x10, 0x1F, 0x14, 0x14, 0x14, 0xF4, 0x14, 0x10,
    0x10, 0xF0, 0x10, 0xF0, 0x00, 0x00, 0x1F, 0x10, 0x1F, 0x00, 0x00, 0x00,
    0x1F, 0x14, 0x00, 0x00, 0x00, 0xFC, 0x14, 0x00, 0x00, 0xF0, 0x10, 0xF0,
    0x10, 0x10, 0xFF, 0x10, 0xFF, 0x14, 0x14, 0x14, 0xFF, 0x14, 0x10, 0x10,
    0x10, 0x1F, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x10, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x38, 0x44, 0x44,
    0x38, 0x44, 0xFC, 0x4A, 0x4A, 0x4A, 0x34, // sharp-s or beta
    0x7E, 0x02, 0x02, 0x06, 0x06, 0x02, 0x7E, 0x02, 0x7E, 0x02, 0x63, 0x55,
    0x49, 0x41, 0x63, 0x38, 0x44, 0x44, 0x3C, 0x04, 0x40, 0x7E, 0x20, 0x1E,
    0x20, 0x06, 0x02, 0x7E, 0x02, 0x02, 0x99, 0xA5, 0xE7, 0xA5, 0x99, 0x1C,
    0x2A, 0x49, 0x2A, 0x1C, 0x4C, 0x72, 0x01, 0x72, 0x4C, 0x30, 0x4A, 0x4D,
    0x4D, 0x30, 0x30, 0x48, 0x78, 0x48, 0x30, 0xBC, 0x62, 0x5A, 0x46, 0x3D,
    0x3E, 0x49, 0x49, 0x49, 0x00, 0x7E, 0x01, 0x01, 0x01, 0x7E, 0x2A, 0x2A,
    0x2A, 0x2A, 0x2A, 0x44, 0x44, 0x5F, 0x44, 0x44, 0x40, 0x51, 0x4A, 0x44,
    0x40, 0x40, 0x44, 0x4A, 0x51, 0x40, 0x00, 0x00, 0xFF, 0x01, 0x03, 0xE0,
    0x80, 0xFF, 0x00, 0x00, 0x08, 0x08, 0x6B, 0x6B, 0x08, 0x36, 0x12, 0x36,
    0x24, 0x36, 0x06, 0x0F, 0x09, 0x0F, 0x06, 0x00, 0x00, 0x18, 0x18, 0x00,
    0x00, 0x00, 0x10, 0x10, 0x00, 0x30, 0x40, 0xFF, 0x01, 0x01, 0x00, 0x1F,
    0x01, 0x01, 0x1E, 0x00, 0x19, 0x1D, 0x17, 0x12, 0x00, 0x3C, 0x3C, 0x3C,
    0x3C, 0x00, 0x00, 0x00, 0x00, 0x00 // #255 NBSP
};

int16_t cursor_x = 0;
int16_t cursor_y = 0; 
uint8_t textsize_x = 1;   // Desired magnification in X-axis of text to print()
uint8_t textsize_y = 1;   // Desired magnification in Y-axis of text to print()
uint16_t textcolor = 0xFFFF;
uint16_t textbgcolor = 0xFFFF;  
uint8_t wrap = 1;
uint8_t _cp437 = 0;

void drawChar(int16_t x, int16_t y, unsigned char c,
                            uint16_t color, uint16_t bg, uint8_t size_x,
                            uint8_t size_y) {


    if ((x >= _width) ||              // Clip right
        (y >= _height) ||             // Clip bottom
        ((x + 6 * size_x - 1) < 0) || // Clip left
        ((y + 8 * size_y - 1) < 0))   // Clip top
      return;

    if (!_cp437 && (c >= 176))
      c++; // Handle 'classic' charset behavior

    startWrite();
    for (int8_t i = 0; i < 5; i++) { // Char bitmap = 5 columns
      uint8_t line = font[c * 5 + i];
      for (int8_t j = 0; j < 8; j++, line >>= 1) {
        if (line & 1) {
          if (size_x == 1 && size_y == 1)
            __tft_draw_pixel(x + i, y + j, color);
          else
            tft_fill_rect(x + i * size_x, y + j * size_y, size_x, size_y,
                          color);
        } else if (bg != color) {
          if (size_x == 1 && size_y == 1)
            __tft_draw_pixel(x + i, y + j, bg);
          else
            tft_fill_rect(x + i * size_x, y + j * size_y, size_x, size_y, bg);
        }
      }
    }
    if (bg != color) { // If opaque, draw vertical line for last column
      if (size_x == 1 && size_y == 1)
        writeFastVLine(x + 5, y, 8, bg);
      else
        tft_fill_rect(x + 5 * size_x, y, size_x, 8 * size_y, bg);
    }
    endWrite();

}

uint8_t tft_get_cursor_x() { return cursor_x; }
uint8_t tft_get_cursor_y() { return cursor_y; }
uint8_t tft_get_char_pixels_x() { return textsize_x * 6; }
uint8_t tft_get_char_pixels_y() { return textsize_y * 8; }

uint8_t tft_write_char(uint8_t c)
{
    if (c == '\n') { // Newline?
        cursor_x = 0; // Reset x to zero,
        cursor_y += textsize_y * 8; // advance y one line
    } else if (c != '\r') { // Ignore carriage returns
        if (wrap && ((cursor_x + textsize_x * 6) > _width)) { // Off right?
            cursor_x = 0; // Reset x to zero,
            cursor_y += textsize_y * 8; // advance y one line
        }
        drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize_x,
                textsize_y);
        cursor_x += textsize_x * 6; // Advance x one char
    }
    
    if (cursor_y > 320 - textsize_y * 8) {
        tft_fill_screen(TFT_COLOR_BLACK);
        cursor_y = 0;
    }

    return 1;
}

void tft_set_text_color(uint16_t c)
{
    textcolor = textbgcolor = c;
}

void tft_set_text_bg_color(uint16_t c, uint16_t bg)
{
    textcolor = c;
    textbgcolor = bg;
}

void tft_set_cursor(int16_t x, int16_t y)
{
    cursor_x = x;
    cursor_y = y;
}

void tft_set_text_size_independent(uint8_t s_x, uint8_t s_y)
{
    textsize_x = (s_x > 0) ? s_x : 1;
    textsize_y = (s_y > 0) ? s_y : 1;
}

void tft_set_text_size(uint8_t s)
{
    tft_set_text_size_independent(s, s);
}

void tft_printf(char *format, ...)
{
    va_list argp;
    va_start(argp, format);
    
    while (*format != '\0')
    {
        if (*format == '%')
        {
            format++;
            if (*format == '%')
            {
                tft_write_char('%');
            } else if (*format == 'c')
            {
                char char_to_print = va_arg(argp, int);
                tft_write_char(char_to_print);
            }
        } else
        {
            tft_write_char(*format);
        }
        
        format++;
    }
    
    va_end(argp);
}

/*******************************************************
 * Image Rendering
 *******************************************************/

uint16_t tft_color_u16(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void __tft_write_pixel_buffer(uint16_t *colors, uint32_t len) {

    if (!len)
        return; // Avoid 0-byte transfers

    while (len--) {
        SPI_WRITE16(*colors++);
    }
}

void tft_render_image_raw(uint8_t *data, int x, int y, int width, int height)
{
    /*
    uint8_t r;
    uint8_t g;
    uint8_t b;
    
    uint16_t *color_buffer = (uint16_t *)malloc(sizeof(uint16_t) * width * height);
    
    startWrite();
    setAddrWindow(x, y, width, height);

    int pos = 0;
    for (int i = 0; i < 3 * height * width; i += 3) {
        r = data[i];
        g = data[i + 1];
        b = data[i + 2];
        
        color_buffer[pos++] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
    
    __tft_write_pixel_buffer(color_buffer, width * height);
    endWrite();
    
    free(color_buffer);
     */
}

void tft_render_image(TFT_Image image, int x, int y)
{
    static uint16_t pixbuf[240 * 320];
    
    startWrite();
    setAddrWindow(x, y, image.Width, image.Height);

    uint16_t *pix_ptr = pixbuf;
    uint16_t* data_ptr = (uint16_t *)image.Data;
    uint16_t *data_end = data_ptr + image.Height * image.Width; 
    
    while (data_ptr != data_end) {
        *pix_ptr++ = *data_ptr++;
    }

    __tft_write_pixel_buffer(pixbuf, image.Height * image.Width);
    
    endWrite();
}

#ifdef TFT_CONFIG_USE_SDCARD

void tft_render_image_sdcard(char *filename, int x, int y, int width, int height)
{
    FIL file;
    uint8_t buffer[3 * 240];        // One RGB row
    int bytes_read = 0;             // Bytes read until now
    
    FRESULT frame_data = f_open(&file, filename, FA_READ);
    
    if (frame_data != FR_OK) {
        return;
    }
        
    for (int i = 0; i < height; i++) {
        do {
            f_read(&file, buffer + bytes_read, 3 * width, &bytes_read);
        } while (bytes_read > 0 && bytes_read < 3 * width);
        bytes_read = 0;

        tft_render_image(buffer, x, y+i, width, 1);
    }
    
    f_close(&file);
}

#endif

