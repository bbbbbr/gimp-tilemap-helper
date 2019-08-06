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

// void scaler_nearest_nx(uint32_t *, uint32_t *, int, int, int, int);
void scaler_nearest_nx(uint8_t * sp, uint8_t * dp,
                       int Xres, int Yres,
                       int scale_factor,
                       int bpp);