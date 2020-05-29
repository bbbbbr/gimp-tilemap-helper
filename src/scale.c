//
// scale.c
//

// ========================
//
// Scaler support
//
// ========================

#include <stdlib.h>
#include <inttypes.h>

#include "win_aligned_alloc.h"

#include "scale.h"
#include "scaler_nearestneighbor.h"
#include "benchmark.h"

static scaled_output_info scaled_output;
static gint scale_factor;


// Returns scale factor (2, 3, etc) of a scaler
//
gint scale_factor_get(void) {

    return (scale_factor);
}



// Sets scale factor (SCALE_FACTOR_MIN .. 2, 3, .. SCALE_FACTOR_MAX)
//
// scaler_index: desired scaler (from the enum scaler_list)
//
void scale_factor_set(gint scale_factor_new) {

    // Update local scale factor setting
    scale_factor = scale_factor_new;

    // Enforce min/max bounds
    if      (scale_factor < SCALE_FACTOR_MIN)
             scale_factor = SCALE_FACTOR_MIN;

    else if (scale_factor > SCALE_FACTOR_MAX)
             scale_factor = SCALE_FACTOR_MAX;
}



// scaled_info_get
//
// Returns structure (type scaled_output_info)
// with details about the current rendered
// output image (scale mode, width, height, factor, etc)
//
// Used to assist with output caching
//
scaled_output_info * scaled_info_get(void) {
    return &scaled_output;
}


// scaled_output_invalidate
//
// Clears valid image flag
//
// Used to clear output caching and trigger a redraw
//
void scaled_output_invalidate(void) {

    printf("Scale: Invalidated\n");
    scaled_output.valid_image = FALSE;
}


// scaled_output_check_reapply_scalers
//
// Checks whether the scaler needs to be re-applied
// depending on whether the scale factor has changed
//
// Used to assist with output caching
//
gint scaled_output_check_reapply_scale(void) {

    // If either the scale factor changed or there is no valid
    // image rendered at the moment, then signal TRUE to indicate
    // scaling should be re-applied

    // printf("Scale: Check Reapply -> scale cached/new (%d/%d), valid=%d\n",
    //         scaled_output.scale_factor, scale_factor,
    //         scaled_output.valid_image);

    if ((scaled_output.scale_factor != scale_factor) ||
        (scaled_output.valid_image == FALSE)) {

        printf("Scale: Check Reapply -> *Required = YES* : scale cached/new (%d/%d), valid=%d\n",
               scaled_output.scale_factor, scale_factor,
               scaled_output.valid_image);

        return TRUE;
    }
    else
        return FALSE;
}


// scaled_output_check_reallocate
//
// Update output buffer size and re-allocate if needed
//
void scaled_output_check_reallocate(gint bpp_new, gint width_new, gint height_new)
{

    size_t alloc_size;

    printf("Scale: Check Realloc : (%d/%d) (%d/%d) (%d/%d) (%d/%d) (%"PRId32"/%d)..  ",
        scaled_output.bpp          , bpp_new,
        scaled_output.width        , width_new  * scale_factor,
        scaled_output.height       , height_new * scale_factor,
        scaled_output.scale_factor , scale_factor,
        scaled_output.size_bytes   , (width_new  * scale_factor) * (height_new * scale_factor) * bpp_new);

    if ((scale_factor                != scaled_output.scale_factor) ||
        ((width_new  * scale_factor) != scaled_output.width) ||
        ((height_new * scale_factor) != scaled_output.height) ||
        (bpp_new                    != scaled_output.bpp) ||
        (scaled_output.p_scaledbuf == NULL) ||
        (scaled_output.p_overlaybuf == NULL)) {

        // Update the buffer size and re-allocate.
        scaled_output.bpp          = bpp_new;
        scaled_output.width        = width_new  * scale_factor;
        scaled_output.height       = height_new * scale_factor;
        scaled_output.scale_factor = scale_factor;
        scaled_output.size_bytes  = scaled_output.width * scaled_output.height * scaled_output.bpp;

        if (scaled_output.p_scaledbuf) {
            free(scaled_output.p_scaledbuf);
            scaled_output.p_scaledbuf = NULL;
        }

        if (scaled_output.p_overlaybuf) {
            free(scaled_output.p_overlaybuf);
            scaled_output.p_overlaybuf = NULL;
        }

        // Allocate a working buffer to copy the source image into, 32 bit aligned
        // aligned_alloc expects SIZE to be a multiple of ALIGNMENT, so pad with a couple bytes if needed
        alloc_size = scaled_output.size_bytes + (scaled_output.size_bytes % sizeof(uint32_t));
        printf(" (allocating %zu bytes %" PRId32 " %zu) \n", alloc_size, scaled_output.size_bytes, (scaled_output.size_bytes % sizeof(uint32_t)));

        scaled_output.p_scaledbuf  = (uint8_t *)aligned_alloc(sizeof(uint32_t), alloc_size);
        scaled_output.p_overlaybuf = (uint8_t *)aligned_alloc(sizeof(uint32_t), alloc_size);
        // g_new allocation here is in u32, so no need to multiply by * BYTE_SIZE_RGBA_4BPP
        // Use matching g_free()
        // scaled_output.p_scaledbuf  = (uint8_t *) g_new (guint32, scaled_output.width * scaled_output.height);
        // scaled_output.p_overlaybuf = (uint8_t *) g_new (guint32, scaled_output.width * scaled_output.height);

        // Invalidate the image
        scaled_output.valid_image = FALSE;

        printf("Reallocated. Valid (scaled image) -> to 0 (false)\n");
    }
    else
        printf("No Change\n");
}


// scaled_output_init
//
// Initialize rendered output shared structure
//
void scaled_output_init(void)
{
      scaled_output.p_scaledbuf  = NULL;
      scaled_output.p_overlaybuf = NULL;
      scaled_output.width        = 0;
      scaled_output.height       = 0;
      scaled_output.x            = 0;
      scaled_output.y            = 0;
      scaled_output.scale_factor = 0;
      scaled_output.size_bytes   = 0;
      scaled_output.bpp          = 0;
      scaled_output.valid_image  = FALSE;
}


void scale_output_get_rgb_at_xy(int x, int y, uint8_t * p_r, uint8_t * p_g, uint8_t * p_b) {
    uint32_t offset;

    offset = ((y * scaled_output.width) + x) * scaled_output.bpp;
    if ((offset + 2) >= scaled_output.size_bytes)
        return; // beyond range of image buffer

    *p_r = *(scaled_output.p_scaledbuf + offset);
    *p_g = *(scaled_output.p_scaledbuf + offset + 1);
    *p_b = *(scaled_output.p_scaledbuf + offset + 2);
}


// scaler_apply
//
// Calls selected scaler function
// Updates valid_image to assist with caching
//
void scale_apply(uint8_t * p_srcbuf, uint8_t * p_destbuf,
                 gint bpp,
                 gint width, gint height,
                 uint8_t * p_cmap_buf, gint cmap_num_colors,
                 gint dest_bpp) {

    if ((p_srcbuf == NULL) || (p_destbuf == NULL))
        return;

printf("Scale: Scaling image now: %dx, bpp=%d, valid image = %d\n", scale_factor, bpp, scaled_output.valid_image);

    if (scale_factor) {

        switch(bpp) {
            case BPP_RGB:
                // Upscale by a factor of N from source (sp) to dest (dp)
                printf("Scale: Start -> RGB  ");
                benchmark_start();
                scaler_nearest_bpp_rgb(p_srcbuf, p_destbuf,
                                       width, height,
                                       scale_factor, bpp);
                benchmark_elapsed();
                break;

            case BPP_RGBA:
                // Upscale by a factor of N from source (sp) to dest (dp)
                printf("Scale: Start -> RGBA  ");
                benchmark_start();
                scaler_nearest_bpp_rgba((uint32_t*)p_srcbuf, (uint32_t*)p_destbuf,
                                       width, height,
                                       scale_factor, bpp);
                benchmark_elapsed();
                break;

            case BPP_INDEXED:
            case BPP_INDEXEDA:
                // Upscale by a factor of N from source (sp) to dest (dp)
                printf("Scale: Start -> INDEXED/A  ");
                benchmark_start();
                scaler_nearest_bpp_indexed(p_srcbuf, p_destbuf,
                                           width, height,
                                           scale_factor, bpp,
                                           p_cmap_buf, cmap_num_colors,
                                           dest_bpp);
                benchmark_elapsed();
                                break;
        }

        scaled_output.valid_image = TRUE;

    }
}



// pixel_art_scalers_release_resources
//
// Release the scaled output buffer.
// Should be called only at the very
// end of the plugin shutdown (not on dialog close)
//
void scale_release_resources(void) {

    if (scaled_output.p_scaledbuf)
        free(scaled_output.p_scaledbuf);
    scaled_output.p_scaledbuf = NULL;


    if (scaled_output.p_overlaybuf)
        free(scaled_output.p_overlaybuf);
    scaled_output.p_overlaybuf = NULL;
}


// scalers_init
//
// Populate the shared list of available scalers with their names
// calling functions and scale factors.
//
void scale_init(void) {

    scaled_output_init();

    // Now set the default scaler
    scale_factor = SCALE_FACTOR_DEFAULT;
 }

