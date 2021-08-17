/******************************************************
 * NOCTIX-1 Module Core Firmware
 * ****************************************************
 * File:    main.c
 * Date:    11.08.2021
 * Author:  Victor Huerlimann, Ribes Microsystems
 ******************************************************/

#include "configs.h"
#include "BSP.h"
#include "flir_lepton35.h"
#include <proc/p32mz1024ech064.h>

int main(void)
{
    // Initialize the NOCTIX-1 module
    BSP_Initialize();
    
    // Process thermal video stream from the FLIR
    FLIR_Process();
    
    return 0;
}