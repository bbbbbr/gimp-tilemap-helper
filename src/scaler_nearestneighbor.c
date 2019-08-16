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


// TODO: should this always operate in RGBA so that overlay is easier to do?
// convert indexed/grayscale -> RGBA
// Have to pass in cmap





// For RGB and RGB+ALPHA
//
// Upscale by a factor of N from source (sp) to dest (dp)
// NOTE: Expects *dp to have a buffer size = width * height * (RGB or RGBA / 3 or 4 bytes)
//
void scaler_nearest_bpp_rgb(uint8_t * sp, uint8_t * dp,
                       int Xres, int Yres,
                       int scale_factor,
                       int src_bpp)
{
    int       x, y, sx, sb;
    uint8_t * p_dst_pixel;
    long      line_width_scaled_bpp;

    if (scale_factor == 1) {
        // Quick 1:1 copy if scale factor is 1x
        memcpy(dp, sp, Xres * Yres * src_bpp);
    }
    else {
        line_width_scaled_bpp = (Xres * scale_factor * src_bpp);

        for (y=0; y < Yres; y++) {
            // Copy each line from the source image
            for (x=0; x < Xres; x++) {

                // Copy each u8 byte of the pixel to the dest buffer
                for (sb=0; sb < src_bpp; sb++) {

                    // Temp copy of dest pointer to iterate through
                    // each pixel's multiple bytes
                    p_dst_pixel = dp;

                    // Copy N (scale factor) pixels from source pixel
                    for (sx=0; sx < scale_factor; sx++) {

                        // Copy pixel from source
                        *p_dst_pixel = *sp;
                        // Move to next pixel using bpp size
                        p_dst_pixel += src_bpp;

                    }

                    // Move to next byte in pixel
                    dp++;
                    sp++;

                }

                // Advance dest pointer to next upscaled pixel destination
                // (1x copy of pixel -> n+ upscaled copy/ies -> next pixel)
                // (it won't be at next pixel since p_dst_pixel is the pointer
                //  used to do the writes at the upscaled locations above)
                dp += ((scale_factor -1)* src_bpp);
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





// For INDEXED+ALPHA (BPP == 1 or == 2)
//
// Upscale by a factor of N from source (sp) to dest (dp)
// NOTE: Promotes image to RGB/A for display purposes
//       Expects *dp to have a buffer size = width * height * (RGB or RGBA / 3 or 4 bytes)
//
void scaler_nearest_bpp_indexed(uint8_t * sp, uint8_t * dp,
                       int Xres, int Yres,
                       int scale_factor,
                       int src_bpp,
                       uint8_t * p_color_map,
                       int num_colors)
{
    int       x, y, sx; // , sb;
    long      line_width_scaled_bpp;
    int       dest_bpp;
    int       color_index;

    // TODO: Instead of recalculating here, pass in the pre-promoted bpp value?
    // Promote Image: 1 bpp -> RGB 3 bpp, 2 bpp (alpha) -> RGBA 4bpp
    dest_bpp = (3 + (src_bpp - 1));

    line_width_scaled_bpp = (Xres * scale_factor * dest_bpp);

    for (y=0; y < Yres; y++) {
        // Copy each line from the source image
        for (x=0; x < Xres; x++) {

            // Copy lookup color from source image
            // Then move source pointer to next pixel
            color_index = *sp++;

            // Don't allow color map requests out of bounds (zero indexed map)
            if (color_index > (num_colors - 1))
                color_index = (num_colors - 1);

            // scale up to RGB indexing for color map
            color_index *= 3;

            // Copy N (scale factor) pixels from source pixel
            for (sx=0; sx < scale_factor; sx++) {

                // Copy Red / Green / Blue Pixel values
                // from color map into dest buffer
                *dp++ = p_color_map[color_index    ]; // Red
                *dp++ = p_color_map[color_index + 1]; // Green
                *dp++ = p_color_map[color_index + 1]; // Blue

                // Copy Alpha byte if present in source
                if (src_bpp == 2)
                    *dp++ = *sp;
            }

            // Move source pointer past Alpha byte if present
            if (src_bpp == 2)
                sp++;
        }

        // Duplicate the preceding line (at scaled size) N times if needed
        for (sx=0; sx < (scale_factor -1); sx++) {
            // memcopy operates on 8 bits
            memcpy(dp, dp - line_width_scaled_bpp, line_width_scaled_bpp);
            dp+= line_width_scaled_bpp;
        }

    }
}

