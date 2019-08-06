//
// scaler_nearestneighbor.c
//

// ========================
//
// Nearest Neighbor pixel art scaler
//
// Simple upscaler
//
// ========================

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "scaler_nearestneighbor.h"

static const char TRUE  = 1;
static const char FALSE = 0;
// static const char BYTE_SIZE_RGBA_4BPP = 4; // RGBA 4BPP


// Upscale by a factor of N from source (sp) to dest (dp)
void scaler_nearest_nx(uint8_t * sp, uint8_t * dp,
                       int Xres, int Yres,
                       int scale_factor,
                       int bpp)
{
    int       x, y, sx, sb;
    uint8_t * src, * dst, * dst_pixel;
    long      line_width_scaled_bpp;

if (scale_factor == 1) {
    memcpy(dp, sp, Xres * Yres * bpp);
    printf("SCALING NOW (1x quick copy): w=%d,h=%d,bpp=%d,s=%d\n",Xres, Yres, bpp, scale_factor);
}
else {
printf("SCALING NOW: w=%d,h=%d,bpp=%d,s=%d\n",Xres, Yres, bpp, scale_factor);

    line_width_scaled_bpp = (Xres * scale_factor * bpp);
    src = sp;
    dst = dp;


    for (y=0; y < Yres; y++) {
        // Copy each line from the source image
        for (x=0; x < Xres; x++) {

            // Copy each u8 byte of the pixel to the dest buffer
            for (sb=0; sb < bpp; sb++) {

                // Temp copy of dest pointer to iterate through
                // each pixels multiple bytes
                dst_pixel = dp;

                // Copy N (scale factor) pixels from source pixel
                for (sx=0; sx < scale_factor; sx++) {

                    // Copy pixel from source
                    *dst_pixel = *sp;
                    // Move to next pixel using bpp size
                    dst_pixel += bpp;

                }

                // Move to next byte in pixel
                dp++;
                sp++;

            }
            // Advance dest pointer to next upscaled pixel destination
            dp += ((scale_factor -1)* bpp);


        }

        // Duplicate the preceding line (at scaled size) N times if needed
        for (sx=0; sx < (scale_factor -1); sx++) {
            // memcopy operates on 8 bits
            memcpy(dp, dp - line_width_scaled_bpp, line_width_scaled_bpp);
            dp+= line_width_scaled_bpp;
        }

    }
}
}



//            *dp++ = *sp++;

/*
            // Copy new pixels from left source pixel
            for (sx=0; sx < scale_factor; sx++) {
                for (sb=0; sb < bpp; sb++) {
                    // Copy each u8 byte of the pixel to the dest buffer
                    *dp++ = *sp;
                }
            }
            // Move to next source pixel
            sp++;
*/



/*
// Upscale by a factor of N from source (sp) to dest (dp)
// Expects 32 bit alignment (RGBA 4BPP) for both source and dest pointers
void scaler_nearest_nx(uint32_t * sp,  uint32_t * dp, int Xres, int Yres, int scale_factor)
{
    int       bpp;
    int       x, y, sx, sb;
    uint32_t * src, * dst;
    long      line_width_scaled;

    bpp = BYTE_SIZE_RGBA_4BPP;  // Assume 4BPP RGBA
    line_width_scaled = (Xres * scale_factor * (bpp/sizeof(uint32_t)) );
    src = (uint32_t *) sp;
    dst = (uint32_t *) dp;


    for (y=0; y < Yres; y++) {
        // Copy each line from the source image
        for (x=0; x < Xres; x++) {

            // Copy new pixels from left source pixel
            for (sx=0; sx < scale_factor; sx++) {
                // Copy each uint32 (RGBA 4BPP) pixel to the dest buffer
                    *dp++ = *sp;
                }
            // Move to next source pixel
            sp++;
        }

        // Duplicate the preceding line (at scaled size) N times if needed
        for (sx=0; sx < (scale_factor -1); sx++) {
            // memcopy operates on 8 bits, the data size is 32 bits
            memcpy(dp, dp - line_width_scaled, line_width_scaled * sizeof(uint32_t));
            dp+= line_width_scaled;
        }
    }
}

*/