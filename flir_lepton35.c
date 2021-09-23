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

static const int colormap_ironblack[] = {255, 255, 255, 253, 253, 253, 251, 251, 251, 249, 249, 249, 247, 247, 247, 245, 245, 245, 243, 243, 243, 241, 241, 241, 239, 239, 239, 237, 237, 237, 235, 235, 235, 233, 233, 233, 231, 231, 231, 229, 229, 229, 227, 227, 227, 225, 225, 225, 223, 223, 223, 221, 221, 221, 219, 219, 219, 217, 217, 217, 215, 215, 215, 213, 213, 213, 211, 211, 211, 209, 209, 209, 207, 207, 207, 205, 205, 205, 203, 203, 203, 201, 201, 201, 199, 199, 199, 197, 197, 197, 195, 195, 195, 193, 193, 193, 191, 191, 191, 189, 189, 189, 187, 187, 187, 185, 185, 185, 183, 183, 183, 181, 181, 181, 179, 179, 179, 177, 177, 177, 175, 175, 175, 173, 173, 173, 171, 171, 171, 169, 169, 169, 167, 167, 167, 165, 165, 165, 163, 163, 163, 161, 161, 161, 159, 159, 159, 157, 157, 157, 155, 155, 155, 153, 153, 153, 151, 151, 151, 149, 149, 149, 147, 147, 147, 145, 145, 145, 143, 143, 143, 141, 141, 141, 139, 139, 139, 137, 137, 137, 135, 135, 135, 133, 133, 133, 131, 131, 131, 129, 129, 129, 126, 126, 126, 124, 124, 124, 122, 122, 122, 120, 120, 120, 118, 118, 118, 116, 116, 116, 114, 114, 114, 112, 112, 112, 110, 110, 110, 108, 108, 108, 106, 106, 106, 104, 104, 104, 102, 102, 102, 100, 100, 100, 98, 98, 98, 96, 96, 96, 94, 94, 94, 92, 92, 92, 90, 90, 90, 88, 88, 88, 86, 86, 86, 84, 84, 84, 82, 82, 82, 80, 80, 80, 78, 78, 78, 76, 76, 76, 74, 74, 74, 72, 72, 72, 70, 70, 70, 68, 68, 68, 66, 66, 66, 64, 64, 64, 62, 62, 62, 60, 60, 60, 58, 58, 58, 56, 56, 56, 54, 54, 54, 52, 52, 52, 50, 50, 50, 48, 48, 48, 46, 46, 46, 44, 44, 44, 42, 42, 42, 40, 40, 40, 38, 38, 38, 36, 36, 36, 34, 34, 34, 32, 32, 32, 30, 30, 30, 28, 28, 28, 26, 26, 26, 24, 24, 24, 22, 22, 22, 20, 20, 20, 18, 18, 18, 16, 16, 16, 14, 14, 14, 12, 12, 12, 10, 10, 10, 8, 8, 8, 6, 6, 6, 4, 4, 4, 2, 2, 2, 0, 0, 0, 0, 0, 9, 2, 0, 16, 4, 0, 24, 6, 0, 31, 8, 0, 38, 10, 0, 45, 12, 0, 53, 14, 0, 60, 17, 0, 67, 19, 0, 74, 21, 0, 82, 23, 0, 89, 25, 0, 96, 27, 0, 103, 29, 0, 111, 31, 0, 118, 36, 0, 120, 41, 0, 121, 46, 0, 122, 51, 0, 123, 56, 0, 124, 61, 0, 125, 66, 0, 126, 71, 0, 127, 76, 1, 128, 81, 1, 129, 86, 1, 130, 91, 1, 131, 96, 1, 132, 101, 1, 133, 106, 1, 134, 111, 1, 135, 116, 1, 136, 121, 1, 136, 125, 2, 137, 130, 2, 137, 135, 3, 137, 139, 3, 138, 144, 3, 138, 149, 4, 138, 153, 4, 139, 158, 5, 139, 163, 5, 139, 167, 5, 140, 172, 6, 140, 177, 6, 140, 181, 7, 141, 186, 7, 141, 189, 10, 137, 191, 13, 132, 194, 16, 127, 196, 19, 121, 198, 22, 116, 200, 25, 111, 203, 28, 106, 205, 31, 101, 207, 34, 95, 209, 37, 90, 212, 40, 85, 214, 43, 80, 216, 46, 75, 218, 49, 69, 221, 52, 64, 223, 55, 59, 224, 57, 49, 225, 60, 47, 226, 64, 44, 227, 67, 42, 228, 71, 39, 229, 74, 37, 230, 78, 34, 231, 81, 32, 231, 85, 29, 232, 88, 27, 233, 92, 24, 234, 95, 22, 235, 99, 19, 236, 102, 17, 237, 106, 14, 238, 109, 12, 239, 112, 12, 240, 116, 12, 240, 119, 12, 241, 123, 12, 241, 127, 12, 242, 130, 12, 242, 134, 12, 243, 138, 12, 243, 141, 13, 244, 145, 13, 244, 149, 13, 245, 152, 13, 245, 156, 13, 246, 160, 13, 246, 163, 13, 247, 167, 13, 247, 171, 13, 248, 175, 14, 248, 178, 15, 249, 182, 16, 249, 185, 18, 250, 189, 19, 250, 192, 20, 251, 196, 21, 251, 199, 22, 252, 203, 23, 252, 206, 24, 253, 210, 25, 253, 213, 27, 254, 217, 28, 254, 220, 29, 255, 224, 30, 255, 227, 39, 255, 229, 53, 255, 231, 67, 255, 233, 81, 255, 234, 95, 255, 236, 109, 255, 238, 123, 255, 240, 137, 255, 242, 151, 255, 244, 165, 255, 246, 179, 255, 248, 193, 255, 249, 207, 255, 251, 221, 255, 253, 235, 255, 255, 24,
-1};

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
    const int *colormap = colormap_ironblack;//colormap_grayscale;
    
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
