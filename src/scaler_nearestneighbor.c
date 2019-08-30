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
    uint8_t   src_R, src_G, src_B;
    uint32_t  line_width_scaled_bpp;

    if (scale_factor == 1) {
        // Quick 1:1 copy if scale factor is 1x
        memcpy(dp, sp, Xres * Yres * src_bpp);
    }
    else {
        line_width_scaled_bpp = (Xres * scale_factor * src_bpp);

        for (y=0; y < Yres; y++) {
            // Copy each line from the source image
            for (x=0; x < Xres; x++) {

                // Copy each RGB values for each pixel
                src_R = *sp++;
                src_G = *sp++;
                src_B = *sp++;

                // Make N (scale factor) copies of pixel to dest pointer from source values
                for (sx=0; sx < scale_factor; sx++) {

                    // Copy pixel from source
                    *dp++ = src_R;
                    *dp++ = src_G;
                    *dp++ = src_B;
                }
            }

            // NOTE: This benchmarks faster than calling one big memcpy with all the lines at once...
            // Duplicate the preceding line (at scaled size) N times if needed
            for (sx=0; sx < (scale_factor -1); sx++) {
                // memcopy operates on 8 bits
                memcpy(dp, dp - line_width_scaled_bpp, line_width_scaled_bpp);
                dp+= line_width_scaled_bpp;
            }

        }
    }
}



// For RGB and RGB+ALPHA
//
// Upscale by a factor of N from source (sp) to dest (dp)
// NOTE: Expects *dp to have a buffer size = width * height * (RGB or RGBA / 3 or 4 bytes)
//
void scaler_nearest_bpp_rgba(uint32_t * sp, uint32_t * dp,
                           int Xres, int Yres,
                           int scale_factor,
                           int src_bpp)
{
    int       x, y, sx;
    uint32_t  line_len_scaled_u8;
    uint32_t  line_len_scaled_u32;

    if (scale_factor == 1) {
        // Quick 1:1 copy if scale factor is 1x
        memcpy(dp, sp, Xres * Yres * src_bpp);
    }
    else {
        // Pre-calculate length of upscaled scanline
        line_len_scaled_u32    = Xres * scale_factor;
        line_len_scaled_u8    = line_len_scaled_u32 * src_bpp;

        for (y=0; y < Yres; y++) {
            // Copy each line from the source image
            for (x=0; x < Xres; x++) {

                // Make N (scale factor) copies of pixel to dest pointer from source values
                for (sx=0; sx < scale_factor; sx++) {

                    // Copy pixel from source
                    *dp++ = *sp;
                }
                sp++;
            }

            // NOTE: This benchmarks faster than calling one big memcpy with all the lines at once...
            // Duplicate the preceding line (at scaled size) N times if needed
            for (sx=0; sx < (scale_factor -1); sx++) {
                // memcopy operates on 8 bits
                memcpy(dp, (dp - line_len_scaled_u32), line_len_scaled_u8);
                dp+= line_len_scaled_u32;
            }

        }
    }
}




// For INDEXED+ALPHA (BPP == 1 or == 2)
//
// Upscale by a factor of N from source (sp) to dest (dp)
// NOTE: Promotes image to RGB/A for display purposes
//       Expects *dp to have a buffer size = width * height * (RGB or RGBA / 3 or 4 bytes)
//       Expects a color map to be passed in
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
                *dp++ = p_color_map[color_index + 2]; // Blue

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


