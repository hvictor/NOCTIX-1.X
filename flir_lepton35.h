/******************************************************
 * FLIR Lepton 3.5 Driver for PIC32 MZ
 * ****************************************************
 * File:    flir_lepton35.h
 * Date:    11.08.2021
 * Author:  Victor Huerlimann, Ribes Microsystems
 ******************************************************/


#ifndef FLIR_LEPTON35_H_
#define FLIR_LEPTON35_H_

#include <stdint.h>

/******************************************************
 * Data Structures
 ******************************************************/
typedef struct
{
    uint8_t Height;
    uint8_t Width;
    uint16_t Data[120][160];
} FLIR_Image;

/******************************************************
 * Frames Retrieval and Processing
 ******************************************************/
void FLIR_Process(void);

#endif /* FLIR_LEPTON35_H_ */