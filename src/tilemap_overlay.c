//
// tilemap_overlay.c
//

// ========================
//
// Overlay tilemap info on a preview image buffer
//
// ========================


#include "tilemap_overlay.h"



static uint8_t * p_overlaybuf; // TODO: fixme - probs better to always pass this in than static global
static int bpp;
static int width;
static int height;
static int tile_width;
static int tile_height;

static int grid_enabled;
static int tilenums_enabled;


void font_render_number(int x, int y, uint16_t num, uint8_t * p_buf );
void font_render_digit(int x, int y, uint8_t digit, uint8_t * p_buf );
void pixel_draw_contrast(int x, int y, uint8_t * p_buf);
void pixel_draw_color(int x, int y, uint8_t * p_buf, uint8_t r, uint8_t g, uint8_t b);

void render_grid (uint8_t * p_buf);

// TODO: ? better to create two buffers and overlay the text one using gimp_preview_area_mask() ?


// Called from main dialog to toggle individual overlays on and off
void tilemap_overlay_set_enables(int grid_enabled_new, int tilenums_enabled_new) {
    grid_enabled = grid_enabled_new;
    tilenums_enabled = tilenums_enabled_new;
}

// NOTE: expects scale_factor to be pre-multipled against width, height, tile_width, tile_height before being fed in
void tilemap_overlay_setparams(uint8_t * p_overlaybuf_new,
                               int bpp_new,
                               int width_new, int height_new,
                               int tile_width_new, int tile_height_new) {

    p_overlaybuf = p_overlaybuf_new;
    bpp = bpp_new;
    width = width_new;
    height = height_new;
    tile_width = tile_width_new;
    tile_height = tile_height_new;
}



// Render a font digit
void font_render_number(int x, int y, uint16_t num, uint8_t * p_buf ) {
    int digits[5];       // Store At most 5 digits
    int digit_count = 0; // Initialize digit count

    // Store individual digits of n in reverse order
    // (using do-while to handle when initial value == 0)
    do {
        digits[digit_count] = num % 10;
        num = num / 10;
        digit_count++;
    } while (num != 0);

    // Print digits
    while (digit_count--) {
        font_render_digit(x, y, digits[digit_count], p_buf);
        x += 4; // TODO: #define FONT_WIDTH
    }
}


// Render a font digit
void font_render_digit(int x, int y, uint8_t digit, uint8_t * p_buf ) {
    int pix;

    if (digit <= 9) {
        // Load number of pixels in font character
        pix = font[digit][0];

        // Draw each pixel pair until none are left (array starts at 1, not zero)
        while (pix) {
            pixel_draw_contrast(x + font[digit][(pix*2) - 1],  // x location + x font pixel offset
                                y + font[digit][(pix*2)    ],  // y location + y font pixel offset
                                p_buf);
/*
            pixel_draw_color(x + font[digit][(pix*2) - 1],  // x location + x font pixel offset
                             y + font[digit][(pix*2)    ],  // y location + y font pixel offset
                             p_buf,
                             0,0,0);  // Black
//                             255,255,255);  // White
*/
            pix--;
        }
    }
}


// Draw a pixel by semi-inverting the current pixel value (roll it 128 bytes upward + wraparound)
// Expects BPP to only = 3 or 4
void pixel_draw_contrast(int x, int y, uint8_t * p_buf) {

    // Don't draw outside the image buffer
    if ((x < width) && (y < height)) {

        // Move to pixel location
        p_buf += (x + (y * width)) * bpp;

        // Handle mostly alpha transparent pixels differently (fixed color)
        if ((bpp == 4) && (*(p_buf + 3) < 192)) {
            // Set pixel to fixed color value
            *p_buf++ = 255; // Red
            *p_buf++ = 255; // Green
            *p_buf++ = 255; // Blue
        }
        else {

            // Set pixel to new contrasted value
            *p_buf++ = *p_buf + 128; // Red
            *p_buf++ = *p_buf + 128; // Green
            *p_buf++ = *p_buf + 128; // Blue
        }

        // handle opacity if needed
        if (bpp == 4)
            *p_buf++ = 255; //*p_buf + 128; // Blue
    }
}


// Draw a pixel by semi-inverting the current pixel value (roll it 128 bytes upward + wraparound)
// Expects BPP to only = 3 or 4
void pixel_draw_color(int x, int y, uint8_t * p_buf, uint8_t r, uint8_t g, uint8_t b) {

    // Don't draw outside the image buffer
    if ((x < width) && (y < height)) {

        // Move to pixel location
        p_buf += (x + (y * width)) * bpp;

        // Set pixel to new contrasted value
        *p_buf++ = r; // Red
        *p_buf++ = g; // Green
        *p_buf++ = b; // Blue

        // handle opacity if needed
        if (bpp == 4)
            *p_buf++ = 255;
    }
}


void render_grid (uint8_t * p_buf) {

    int x,y;

    // Draw a grid using the tile size
    for (y=0; y < height; y++) {
        for (x=0; x < width; x++) {
            if (((x % tile_width) == 0) ||
                ((y % tile_height) == 0)) {

                pixel_draw_contrast(x, y, p_buf);
            }
        }
    }
}


void render_tilenums (uint8_t * p_buf, uint32_t map_size, uint8_t * map_tilelist) {

    int x,y;
    int tile_index;

    tile_index = 0;

    if (map_size != ((width / tile_width) * (height / tile_height)))
        printf("Overlay: Render tilenums -> WRONG MAP SIZE!\n");
    else {
        for (y=0; y < height; y+= tile_height) {
            for (x=0; x < width; x+= tile_width) {

                font_render_number(x + 2,
                                   y + 2,
                                   map_tilelist[ tile_index++ ],
                                   p_buf);
            }
        }
    }
}


void tilemap_overlay_apply(uint32_t map_size, uint8_t * map_tilelist) {

    printf("Overlay: Drawing now...\n");

    if (p_overlaybuf == NULL)
        return;

    // Draw the tile grid
    if (grid_enabled)
        render_grid (p_overlaybuf);

    // Draw the tile numbers
    if (tilenums_enabled)
        render_tilenums ( p_overlaybuf, map_size, map_tilelist);
}


