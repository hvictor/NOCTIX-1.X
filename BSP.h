/******************************************************
 * NOCTIX-1 Module BSP (Board Support Package)
 * ****************************************************
 * File:    BSP.h
 * Date:    11.08.2021
 * Author:  Victor Huerlimann, Ribes Microsystems
 ******************************************************/

#ifndef BSP_PIN_H_
#define BSP_PIN_H_

#include <xc.h>
#include "tft_st7789.h"

/******************************************************
 * System Clock Constants
 ******************************************************/
#define SYSCLK                          (200000000L)

/******************************************************
 * Pin Definitions
 * 
 *	FLIR SCLK       = Pin 49
 *	FLIR CS         = Pin 27    = RB12
 *	FLIR SDI1       = Pin 6     = RG8 => PPS: SDI1R = 0001
 *	FLIR MOSI       = Pin 23    = RB10
 * 	
 *	TFT SCLK        = Pin 4
 *	TFT CS          = Pin 29 = RB14
 *	TFT DC          = Pin 30 = RB15
 *	TFT MISO        = SDI2 = RG7 = Pin 5    => PPS: SDI2R = 0001
 *	TFT MOSI        = SDO2 = Pin 22 = RB9   => PPS: RPB9R = 0110
 ******************************************************/

// LED Pins
#define BSP_Pin_LED1                    LATGbits.LATG7

// SPI Pins
#define BSP_Pin_TFT_CS                  LATBbits.LATB14
#define BSP_Pin_TFT_DC                  LATBbits.LATB15
#define BSP_Register_TFT_SPIBUF         SPI2BUF
#define BSP_Register_TFT_SPISTAT        SPI2STATbits
#define BSP_Register_TFT_SPICON         SPI2CONbits

#define BSP_Pin_FLIR_CS                 LATBbits.LATB12
#define BSP_Register_FLIR_SPIBUF        SPI1BUF
#define BSP_Register_FLIR_SPISTAT       SPI1STATbits
#define BSP_Register_FLIR_SPICON        SPI1CONbits

/******************************************************
 * LED Control
 ******************************************************/
#define BSP_LED1_On()       BSP_Pin_LED1 = 1;
#define BSP_LED1_Off()      BSP_Pin_LED1 = 0;
#define BSP_LED1_Toggle()   BSP_Pin_LED1 ^= 1;
void BSP_Initialize_LEDs();

/******************************************************
 * SPI Peripherals Control
 ******************************************************/
#define BSP_SPI1_CS_Low()   BSP_Pin_FLIR_CS = 0;
#define BSP_SPI1_CS_High()  BSP_Pin_FLIR_CS = 1;
#define BSP_SPI1_On()       BSP_Register_FLIR_SPICON.ON = 1;
#define BSP_SPI1_Off()      BSP_Register_FLIR_SPICON.ON = 0;

#define BSP_SPI2_CS_Low()   BSP_Pin_TFT_CS = 0;
#define BSP_SPI2_CS_High()  BSP_Pin_TFT_CS = 1;
#define BSP_SPI2_On()       BSP_Register_TFT_SPICON.ON = 1;
#define BSP_SPI2_Off()      BSP_Register_TFT_SPICON.ON = 0;

/******************************************************
 * BSP Initialization
 ******************************************************/
void BSP_Initialize();

/******************************************************
 * BSP Timing Functions
 ******************************************************/
void BSP_Delay_us(unsigned int us);
void BSP_Delay_ms(int ms);

#endif /* BSP_H_ */