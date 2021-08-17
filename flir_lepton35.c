/******************************************************
 * FLIR Lepton 3.5 Driver for PIC32 MZ
 * ****************************************************
 * File:    flir_lepton35.c
 * Date:    11.08.2021
 * Author:  Victor Huerlimann, Ribes Microsystems
 ******************************************************/

#include "flir_lepton35.h"
#include "BSP.h"
#include <string.h>
#include <stdlib.h>

/******************************************************
 * Constants
 ******************************************************/

#define FLIR_COLORMAP_SIZE                  3 * 256
#define PACKET_SIZE                         164
#define PACKET_SIZE_UINT16                  (PACKET_SIZE / 2)
#define PACKETS_PER_FRAME                   60
#define FRAME_SIZE_UINT16                   (PACKET_SIZE_UINT16 * PACKETS_PER_FRAME)
#define FPS                                 27;
#define true                                1
#define false                               0

static const int colormap_grayscale[] = 
{
    0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 10,
    11, 11, 11, 12, 12, 12, 13, 13, 13, 14, 14, 14, 15, 15, 15, 16, 16, 16, 17, 17, 17, 18, 18, 18, 19, 19,
    19, 20, 20, 20, 21, 21, 21, 22, 22, 22, 23, 23, 23, 24, 24, 24, 25, 25, 25, 26, 26, 26, 27, 27, 27, 28,
    28, 28, 29, 29, 29, 30, 30, 30, 31, 31, 31, 32, 32, 32, 33, 33, 33, 34, 34, 34, 35, 35, 35, 36, 36, 36,
    37, 37, 37, 38, 38, 38, 39, 39, 39, 40, 40, 40, 41, 41, 41, 42, 42, 42, 43, 43, 43, 44, 44, 44, 45, 45,
    45, 46, 46, 46, 47, 47, 47, 48, 48, 48, 49, 49, 49, 50, 50, 50, 51, 51, 51, 52, 52, 52, 53, 53, 53, 54,
    54, 54, 55, 55, 55, 56, 56, 56, 57, 57, 57, 58, 58, 58, 59, 59, 59, 60, 60, 60, 61, 61, 61, 62, 62, 62,
    63, 63, 63, 64, 64, 64, 65, 65, 65, 66, 66, 66, 67, 67, 67, 68, 68, 68, 69, 69, 69, 70, 70, 70, 71, 71,
    71, 72, 72, 72, 73, 73, 73, 74, 74, 74, 75, 75, 75, 76, 76, 76, 77, 77, 77, 78, 78, 78, 79, 79, 79, 80,
    80, 80, 81, 81, 81, 82, 82, 82, 83, 83, 83, 84, 84, 84, 85, 85, 85, 86, 86, 86, 87, 87, 87, 88, 88, 88,
    89, 89, 89, 90, 90, 90, 91, 91, 91, 92, 92, 92, 93, 93, 93, 94, 94, 94, 95, 95, 95, 96, 96, 96, 97, 97,
    97, 98, 98, 98, 99, 99, 99, 100, 100, 100, 101, 101, 101, 102, 102, 102, 103, 103, 103, 104, 104, 104,
    105, 105, 105, 106, 106, 106, 107, 107, 107, 108, 108, 108, 109, 109, 109, 110, 110, 110, 111, 111, 111,
    112, 112, 112, 113, 113, 113, 114, 114, 114, 115, 115, 115, 116, 116, 116, 117, 117, 117, 118, 118, 118,
    119, 119, 119, 120, 120, 120, 121, 121, 121, 122, 122, 122, 123, 123, 123, 124, 124, 124, 125, 125, 125,
    126, 126, 126, 127, 127, 127, 128, 128, 128, 129, 129, 129, 130, 130, 130, 131, 131, 131, 132, 132, 132,
    133, 133, 133, 134, 134, 134, 135, 135, 135, 136, 136, 136, 137, 137, 137, 138, 138, 138, 139, 139, 139,
    140, 140, 140, 141, 141, 141, 142, 142, 142, 143, 143, 143, 144, 144, 144, 145, 145, 145, 146, 146, 146,
    147, 147, 147, 148, 148, 148, 149, 149, 149, 150, 150, 150, 151, 151, 151, 152, 152, 152, 153, 153, 153,
    154, 154, 154, 155, 155, 155, 156, 156, 156, 157, 157, 157, 158, 158, 158, 159, 159, 159, 160, 160, 160,
    161, 161, 161, 162, 162, 162, 163, 163, 163, 164, 164, 164, 165, 165, 165, 166, 166, 166, 167, 167, 167,
    168, 168, 168, 169, 169, 169, 170, 170, 170, 171, 171, 171, 172, 172, 172, 173, 173, 173, 174, 174, 174,
    175, 175, 175, 176, 176, 176, 177, 177, 177, 178, 178, 178, 179, 179, 179, 180, 180, 180, 181, 181, 181,
    182, 182, 182, 183, 183, 183, 184, 184, 184, 185, 185, 185, 186, 186, 186, 187, 187, 187, 188, 188, 188,
    189, 189, 189, 190, 190, 190, 191, 191, 191, 192, 192, 192, 193, 193, 193, 194, 194, 194, 195, 195, 195,
    196, 196, 196, 197, 197, 197, 198, 198, 198, 199, 199, 199, 200, 200, 200, 201, 201, 201, 202, 202, 202,
    203, 203, 203, 204, 204, 204, 205, 205, 205, 206, 206, 206, 207, 207, 207, 208, 208, 208, 209, 209, 209,
    210, 210, 210, 211, 211, 211, 212, 212, 212, 213, 213, 213, 214, 214, 214, 215, 215, 215, 216, 216, 216,
    217, 217, 217, 218, 218, 218, 219, 219, 219, 220, 220, 220, 221, 221, 221, 222, 222, 222, 223, 223, 223,
    224, 224, 224, 225, 225, 225, 226, 226, 226, 227, 227, 227, 228, 228, 228, 229, 229, 229, 230, 230, 230,
    231, 231, 231, 232, 232, 232, 233, 233, 233, 234, 234, 234, 235, 235, 235, 236, 236, 236, 237, 237, 237,
    238, 238, 238, 239, 239, 239, 240, 240, 240, 241, 241, 241, 242, 242, 242, 243, 243, 243, 244, 244, 244,
    245, 245, 245, 246, 246, 246, 247, 247, 247, 248, 248, 248, 249, 249, 249, 250, 250, 250, 251, 251, 251,
    252, 252, 252, 253, 253, 253, 254, 254, 254, 255, 255, 255, -1
};

/******************************************************
 * Macros
 ******************************************************/
#define convert_flir_tft(f)          ((TFT_Image) { .Height = f.Height, .Width = f.Width, .Data = (uint16_t **)&(f.Data[0]) })

/******************************************************
 * Global Variables
 ******************************************************/
FLIR_Image thermal_frame;

static int ofs_r;
static int ofs_g;
static int ofs_b;
static uint8_t auto_range_min = 1;
static uint8_t auto_range_max = 1;
static uint16_t range_min = 30000;
static uint16_t range_max = 32000;
static int frame_width;
static int frame_height;
static uint8_t frame_data[PACKET_SIZE * PACKETS_PER_FRAME];
static uint8_t storage[4][PACKET_SIZE * PACKETS_PER_FRAME];
static uint16_t *frame_buffer;

static void FLIR_ReadFramePacket(int j)
{
    BSP_SPI1_CS_Low();
    
    for (int i = 0; i < PACKET_SIZE; i++) {
        BSP_Register_FLIR_SPIBUF = 'A';
        while (!BSP_Register_FLIR_SPISTAT.SPIRBF) ;
        frame_data[j * PACKET_SIZE + i] = (uint8_t)BSP_Register_FLIR_SPIBUF;
    }
    
    BSP_SPI1_CS_High();
}

/******************************************************
 * Frames Retrieval and Processing
 ******************************************************/
void FLIR_Process(void)
{    
    const int *colormap = colormap_grayscale;
    
	uint16_t min_value = range_min;
	uint16_t max_value = range_max;
	float diff = max_value - min_value;
	float scale = 255/diff;
	uint16_t n_wrong_segment = 0;
	uint16_t n_zero_value_drop_frame = 0;

    frame_width = 160;
    frame_height = 120;
    
    thermal_frame.Height = frame_height;
    thermal_frame.Width = frame_width;

	// Min-Max value for scaling
	auto_range_min = 1;
	auto_range_max = 1;

    while (true)
    {
        // Read frame packets over SPI1
        int resets = 0;
        int segment_number = -1;

        for (int j = 0; j < PACKETS_PER_FRAME; j++) {
            //if it's a drop packet, reset j to 0, set to -1 so he'll be at 0 again loop
            FLIR_ReadFramePacket(j);
            int packet_number = frame_data[j * PACKET_SIZE + 1];
            if (packet_number != j) {
                j = -1;
                resets += 1;
                BSP_Delay_us(1000);
                
                if (resets == 750)
                {
                    BSP_SPI1_Off();
                    
                    n_wrong_segment = 0;
                    n_zero_value_drop_frame = 0;
                    BSP_Delay_us(750000);
                    
                    BSP_SPI1_On();
                }
                continue;
            }
            if (packet_number == 20) {
                segment_number = (frame_data[j * PACKET_SIZE] >> 4) & 0x0f;
                if ((segment_number < 1) || (4 < segment_number)) {
                    // Wrong segment number
                    break;
                }
            }
        }
        
        int segment_start_index = 1;
        int segment_stop_index;

        if ((segment_number < 1) || (4 < segment_number)) {
            n_wrong_segment++;

            continue;
        }
        
        // Got wrong segment number continuously (n_wrong_segment) times, recovered.
        if (n_wrong_segment != 0) {
            n_wrong_segment = 0;
        }

        // Copy frame data to storage
        memcpy(storage[segment_number - 1], frame_data, sizeof (uint8_t) * PACKET_SIZE * PACKETS_PER_FRAME);
        
        if (segment_number != 4) {
            continue;
        }
        segment_stop_index = 4;

        if ((auto_range_min == true) || (auto_range_max == true))
        {
            if (auto_range_min == true) {
                max_value = 65535;
            }
            
            if (auto_range_max == true) {
                max_value = 0;
            }
            
            for (int index = segment_start_index; index <= segment_stop_index; index++) {
                for (int i = 0; i < FRAME_SIZE_UINT16; i++) {
                    
                    // Skip the first 2 UINT16 of every packet: they are header 4 header bytes
                    if (i % PACKET_SIZE_UINT16 < 2) {
                        continue;
                    }

                    // Flip the MSB and LSB
                    uint16_t value = (storage[index - 1][i * 2] << 8) + storage[index - 1][i * 2 + 1];
                    
                    if (value == 0) {
                        continue;
                    }
                    
                    if ((auto_range_max == true) && (value > max_value)) {
                        max_value = value;
                    }
                    
                    if ((auto_range_min == true) && (value < min_value)) {
                        min_value = value;
                    }
                }
            }
            
            diff = max_value - min_value;
            scale = 255.0f / (float)diff;
        }

        int row, column;
        uint16_t value;
        uint16_t value_frame_buffer;
        uint16_t color;
        
        for (int index = segment_start_index; index <= segment_stop_index; index++)
        {
            int offset_row = 30 * (index - 1);
            
            for (int i = 0; i < FRAME_SIZE_UINT16; i++) {
                
                // Skip the first 2 UINT16 of every packet: they are header 4 header bytes
                if (i % PACKET_SIZE_UINT16 < 2) {
                    continue;
                }

                // Flip the MSB and LSB
                value_frame_buffer = (storage[index - 1][i * 2] << 8) + storage[index - 1][i * 2 + 1];
                
                if (value_frame_buffer == 0)
                {
                    n_zero_value_drop_frame++;
                    break;
                }

                value = (uint16_t)(((float)value_frame_buffer - (float)min_value) * scale);
                
                ofs_r = 3 * value + 0;
                if (FLIR_COLORMAP_SIZE <= ofs_r) ofs_r = FLIR_COLORMAP_SIZE - 1;
                ofs_g = 3 * value + 1;
                if (FLIR_COLORMAP_SIZE <= ofs_g) ofs_g = FLIR_COLORMAP_SIZE - 1;
                ofs_b = 3 * value + 2;
                if (FLIR_COLORMAP_SIZE <= ofs_b) ofs_b = FLIR_COLORMAP_SIZE - 1;

                color = tft_color_u16((uint8_t)colormap[ofs_r], (uint8_t)colormap[ofs_g], (uint8_t)colormap[ofs_b]);

                column = (i % PACKET_SIZE_UINT16) - 2 + (frame_width / 2) * ((i % (PACKET_SIZE_UINT16 * 2)) / PACKET_SIZE_UINT16);
                row = i / PACKET_SIZE_UINT16 / 2 + offset_row;

                thermal_frame.Data[row][column] = color;
            }
        }

        if (n_zero_value_drop_frame != 0) {
            // Found zero-value. Drop the frame continuously (n_zero_value_drop_frame) times - Recovered
            n_zero_value_drop_frame = 0;
        }

        tft_render_image(convert_flir_tft(thermal_frame), 0, 0);
    }
}
