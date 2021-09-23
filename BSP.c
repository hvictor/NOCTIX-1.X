/******************************************************
 * NOCTIX-1 Module BSP (Board Support Package)
 * ****************************************************
 * File:    BSP.c
 * Date:    11.08.2021
 * Author:  Victor Huerlimann, Ribes Microsystems
 ******************************************************/

#include "BSP.h"

void BSP_Initialize_LEDs()
{
    /* LED1 */
    //ANSELGbits.ANSG7 = 0;
    //TRISGbits.TRISG7 = 0;
}

/******************************************************
 * SPI1 Peripheral Configuration for FLIR Lepton 3.5
 ******************************************************/
void BSP_Initialize_SPI1()
{
    // FLIR SCLK     = Pin 49
    // FLIR CS       = Pin 27    = RB12
    // DC            = Pin 28    = RB13
    // FLIR SDI1     = Pin 6     = RG8 => PPS: SDI1R = 0001
    // FLIR MOSI     = Pin 23    = RB10
    
    SPI1CONbits.ON = 0;     // Turn off SPI1 before configuring
    SPI1CONbits.MSTEN = 1;  // Enable Master mode
    SPI1CONbits.CKP = 1;    // Clock signal is active low, idle state is high
    SPI1CONbits.CKE = 0;    // Data is shifted out/in on transition from idle (high) state to active (low) state
    SPI1CONbits.SMP = 1;    // Input data is sampled at the end of the clock signal
    SPI1CONbits.MODE16 = 0; // Do not use 16-bit mode
    SPI1CONbits.MODE32 = 0; // Do not use 32-bit mode (combines with the above line to activate 8-bit mode)
    SPI1BRG = 1;            
    SPI1CONbits.ENHBUF = 0; // Disables Enhanced Buffer mode
    SPI1CONbits.ON = 1;     // Configuration is done, turn on SPI1 peripheral
    
    // FLIR CS = Pin 27 = RB12
    TRISBbits.TRISB12 = 0;
    ANSELBbits.ANSB12 = 0;

    // FLIR DC = Pin 28 = RB13
    TRISBbits.TRISB13 = 0;
    ANSELBbits.ANSB13 = 0;
    
    // FLIR MISO = SDI1 = RG8 = Pin 6 => PPS: SDI1R = 0001
    // Set as Input
    ANSELGbits.ANSG8 = 0;
    SDI1R = 0b0001;
    TRISGbits.TRISG8 = 1;
    
    // FLIR MOSI = Pin 23 = RB10
    ANSELBbits.ANSB10 = 0;
    TRISBbits.TRISB10 = 0;
    RPB10R = 0b0101;
}

/******************************************************
 * SPI2 Peripheral Configuration for ST7789 TFT
 ******************************************************/
void BSP_Initialize_SPI2()
{
    // TFT SCLK = Pin 4 = RG6
    // TFT CS = Pin 29 = RB14
    // TFT DC = Pin 30 = RB15
    // TFT MISO = SDI2 = RG7 = Pin 5 => PPS: SDI2R = 0001
    // TFT MOSI = SDO2 = Pin 22 = RB9 => PPS: RPB9R = 0110
    
    SPI2CONbits.ON = 0;     // Turn off SPI1 before configuring
    SPI2CONbits.MSTEN = 1;  // Enable Master mode
    SPI2CONbits.CKP = 1;    // Clock signal is active low, idle state is high
    SPI2CONbits.CKE = 0;    // Data is shifted out/in on transition from idle (high) state to active (low) state
    SPI2CONbits.SMP = 1;    // Input data is sampled at the end of the clock signal
    SPI2CONbits.MODE16 = 0; // Do not use 16-bit mode
    SPI2CONbits.MODE32 = 0; // Do not use 32-bit mode (combines with the above line to activate 8-bit mode)
    SPI2BRG = 0;       // (BRG DISARMED) Set Baud Rate Generator to 0
    SPI2CONbits.ENHBUF = 0; // Disables Enhanced Buffer mode
    SPI2CONbits.ON = 1;     // Configuration is done, turn on SPI1 peripheral
    
    // TFT2 CS = Pin 29 = RB14
    ANSELBbits.ANSB14 = 0;
    TRISBbits.TRISB14 = 0;

    // TFT2 DC = Pin 30 = RB15
    ANSELBbits.ANSB15 = 0;
    TRISBbits.TRISB15 = 0;
    
    // TFT2 MISO = SDI2 = RG7 = Pin 5 => PPS: SDI2R = 0001
    ANSELGbits.ANSG7 = 0;
    TRISGbits.TRISG7 = 1;
    SDI2R = 0b0001;
    
    // TFT2 MOSI = SDO2 = Pin 22 = RB9 => PPS: RPB9R = 0110
    ANSELBbits.ANSB9 = 0;
    TRISBbits.TRISB9 = 0;
    RPB9R = 0b0110;
}

/******************************************************
 * BSP Initialization
 ******************************************************/
void BSP_Initialize()
{
    BSP_Initialize_SPI1();
    BSP_Initialize_SPI2();
    BSP_Initialize_LEDs();
    
    tft_init(240, 320);
    tft_fill_screen(TFT_COLOR_BLACK);
    tft_set_cursor(0, 320 - tft_get_char_pixels_y() - 1);
    tft_set_text_color(TFT_COLOR_RED);
    tft_printf("* ");
    tft_set_text_color(TFT_COLOR_WHITE);
    tft_printf("NOCTIX-1 Module Core Executing.");
    tft_set_cursor(0, 0);
}

/******************************************************
 * BSP Timing Functions
 ******************************************************/
void BSP_Delay_us(unsigned int us)
{
    // Convert microseconds us into how many clock ticks it will take
	us *= SYSCLK / 1000000 / 2; // Core Timer updates every 2 ticks
       
    _CP0_SET_COUNT(0); // Set Core Timer count to 0
    
    while (us > _CP0_GET_COUNT()); // Wait until Core Timer count reaches the number we calculated earlier
}

void BSP_Delay_ms(int ms)
{
    BSP_Delay_us(ms * 1000);
}