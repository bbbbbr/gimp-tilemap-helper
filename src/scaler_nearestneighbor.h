//
// scaler_nearestneighbor.h
//

// ========================
//
// Nearest Neighbor pixel art scaler
//
// Simple upscaler
//
// ========================

void scaler_nearest_bpp_rgb(uint8_t * sp, uint8_t * dp,
                       int Xres, int Yres,
                       int scale_factor,
                       int src_bpp);

void scaler_nearest_bpp_rgba(uint32_t * sp, uint32_t * dp,
                       int Xres, int Yres,
                       int scale_factor,
                       int src_bpp);


void scaler_nearest_bpp_indexed(uint8_t * sp, uint8_t * dp,
                       int Xres, int Yres,
                       int scale_factor,
                       int src_bpp,
                       uint8_t * p_color_map,
                       int num_colors);
