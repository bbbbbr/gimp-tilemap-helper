//
// scale.c
//

// ========================
//
// Scaler support
//
// ========================



#include "scale.h"
#include "scaler_nearestneighbor.h"

static scaled_output_info scaled_output;
static gint scale_factor;
static gint scale_bpp;


// Returns scale factor (2, 3, etc) of a scaler
//
gint scale_factor_get() {

    return (scale_factor);
}

/*
// Returns bpp (1, 2, 3, 4) of image
//
gint scale_bpp_get() {

    return (scale_bpp);
}
*/


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


/*
// Sets scale bits per pixel
//
// scaler_index: desired scaler (from the enum scaler_list)
//
void scale_bpp_set(gint scale_bpp_new) {

    // Update local scale factor setting
    scale_bpp = scale_bpp_new;

    // Enforce min/max bounds
    if      (scale_bpp < SCALE_BPP_MIN)
             scale_bpp = SCALE_BPP_MIN;

    else if (scale_bpp > SCALE_BPP_MAX)
             scale_bpp = SCALE_BPP_MAX;
}
*/


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
void scaled_output_invalidate() {
    scaled_output.valid_image = FALSE;
}


// scaled_output_check_reapply_scalers
//
// Checks whether the scaler needs to be re-applied
// depending on whether the scale factor has changed
//
// Used to assist with output caching
//
gint scaled_output_check_reapply_scale() {

    // If either the scale factor changed or there is no valid
    // image rendered at the moment, then signal TRUE to indicate
    // scaling should be re-applied
    return ((scaled_output.scale_factor != scale_factor) ||
            (scaled_output.valid_image == FALSE));
}


// scaled_output_check_reallocate
//
// Update output buffer size and re-allocate if needed
//
void scaled_output_check_reallocate(gint bpp_new, gint width_new, gint height_new)
{
    if ((scale_factor                != scaled_output.scale_factor) ||
        ((width_new  * scale_factor) != scaled_output.width) ||
        ((height_new * scale_factor) != scaled_output.height) ||
        (bpp_new                    != scaled_output.bpp) ||
        (scaled_output.p_scaledbuf == NULL)) {

        // Update the buffer size and re-allocate.
        // TODO: delete comment--- The x uint32_t is for RGBA buffer size
        scaled_output.bpp          = bpp_new;
        scaled_output.width        = width_new  * scale_factor;
        scaled_output.height       = height_new * scale_factor;
        scaled_output.scale_factor = scale_factor;
        scaled_output.size_bytes  = scaled_output.width * scaled_output.height * scaled_output.bpp; // BYTE_SIZE_RGBA_4BPP;

        if (scaled_output.p_scaledbuf) {
            g_free(scaled_output.p_scaledbuf);
            scaled_output.p_scaledbuf = NULL;
        }

        // 32 bit to ensure alignment, divide size since it's in BYTES
        scaled_output.p_scaledbuf = (uint8_t *) g_new (guint8, scaled_output.size_bytes);

        // Invalidate the image
        scaled_output.valid_image = FALSE;
    }
}


// scaled_output_init
//
// Initialize rendered output shared structure
//
void scaled_output_init(void)
{
      scaled_output.p_scaledbuf  = NULL;
      scaled_output.width        = 0;
      scaled_output.height       = 0;
      scaled_output.x            = 0;
      scaled_output.y            = 0;
      scaled_output.scale_factor = 0;
      scaled_output.size_bytes   = 0;
      scaled_output.bpp          = 0;
      scaled_output.valid_image  = FALSE;
}



// scaler_apply
//
// Calls selected scaler function
// Updates valid_image to assist with caching
//
void scale_apply(uint8_t * p_srcbuf, uint8_t * p_destbuf,
                 gint bpp,
                 gint width, gint height,
                 uint8_t * p_cmap_buf, int cmap_num_colors) {

    if ((p_srcbuf == NULL) || (p_destbuf == NULL))
        return;

printf("Scaling image now: %dx, bpp=%d, valid image = %d\n", scale_factor, bpp, scaled_output.valid_image);

    if (scale_factor) {

        switch(bpp) {
            case BPP_RGB:
            case BPP_RGBA:
                // Upscale by a factor of N from source (sp) to dest (dp)
                scaler_nearest_bpp_rgb(p_srcbuf, p_destbuf,
                                       width, height,
                                       scale_factor, bpp);
                break;

            case BPP_INDEXED:
            case BPP_INDEXEDA:
                // Upscale by a factor of N from source (sp) to dest (dp)
                scaler_nearest_bpp_indexed(p_srcbuf, p_destbuf,
                                           width, height,
                                           scale_factor, bpp,
                                           p_cmap_buf, cmap_num_colors);
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

  if (scaled_output.p_scaledbuf) {
      g_free(scaled_output.p_scaledbuf);
      scaled_output.p_scaledbuf = NULL;
  }
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
