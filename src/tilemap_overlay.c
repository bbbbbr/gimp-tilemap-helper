//
// tilemap_overlay.c
//

// ========================
//
// Overlay tilemap info on a preview image buffer
//
// ========================


#include "tilemap_overlay.h"

#include "benchmark.h"

    // Overlay 3 x 5 font in pixel offset locations
    // first byte = number of pixel pairs
    // next bytes = x,y offsets of font pixels
    static int font[10][(8*2) + 1] =
    {
        { 8,  2,0,  1,1,  3,1,  1,2,  3,2,  1,3,  3,3,  2,4 },  // 0
        { 5,  2,0,  2,1,  2,2,  2,3,  2,4, },                    // 1
        { 8,  1,0,  2,0,  3,1,  2,2,  1,3,  1,4,  2,4,  3,4 },  // 2
        { 7,  1,0,  2,0,  3,1,  2,2,  3,3,  1,4,  2,4 },        // 3
        { 8,  1,0,  3,0,  1,1,  3,1,  2,2,  3,2,  3,3,  3,4 },  // 4
        { 8,  1,0,  2,0,  3,0,  1,1,  2,2,  3,3,  1,4,  2,4 },  // 5
        { 8,  2,0,  3,0,  1,1,  1,2,  2,2,  1,3,  3,3,  2,4 },  // 6
        { 7,  1,0,  2,0,  3,0,  3,1,  2,2,  2,3,  2,4 },        // 7
        { 7,  2,0,  1,1,  3,1,  2,2,  1,3,  3,3,  2,4 },        // 8
        { 7,  2,0,  1,1,  3,1,  2,2,  3,2,  3,3,  3,4 }         // 9
    };


static uint8_t * p_overlaybuf; // TODO: fixme - probs better to always pass this in than static global
static int bpp;
static int width;
static int height;
static int tile_width;
static int tile_height;

static int grid_enabled;
static int tilenums_enabled;

#define TILE_HIGHLIGHT_NONE -1
static int tile_to_hightlight = TILE_HIGHLIGHT_NONE;

static int redraw_required = true;


static void font_render_number(int x, int y, uint16_t num, uint8_t * p_buf );
static void font_render_digit(int x, int y, uint8_t digit, uint8_t * p_buf );
static void pixel_draw_contrast(int x, int y, uint8_t * p_buf);
// static void pixel_draw_color(int x, int y, uint8_t * p_buf, uint8_t r, uint8_t g, uint8_t b);

static void render_grid_rgb(uint8_t * p_buf);
static void render_grid_rgba(uint32_t * p_buf);


static void highlight_tile_rgb(uint8_t * p_buf, int tx, int ty);
static void highlight_tile_rgba(uint32_t * p_buf, int tx, int ty);
static void render_highlight_tilenum (uint8_t * p_buf, uint32_t map_size, uint32_t * map_tilelist);



// TODO: ? better to create two buffers and overlay the text one using gimp_preview_area_mask() ?



void overlay_redraw_invalidate(void) {
    printf("Overlay: invalidated\n");
    redraw_required = true;
}


void overlay_redraw_clear_flag(void) {
    printf("Overlay: redraw flag cleared\n");
    redraw_required = false;
}


int overlay_redraw_needed(void) {
    return redraw_required;
}


void tilemap_overlay_set_highlight_tile(int tile_id) {

    // Set tile number if it's different
    if (tile_to_hightlight != tile_id) {
        tile_to_hightlight = tile_id;
        printf("Overlay: Highlight: Set to %d\n", tile_to_hightlight);
    }
    else {
        // If it's the same tile as already set then de-select it
        tilemap_overlay_clear_highlight_tile();
    }
}

void tilemap_overlay_clear_highlight_tile(void) {
    tile_to_hightlight = TILE_HIGHLIGHT_NONE;
    printf("Overlay: Highlight: Set to %d\n", tile_to_hightlight);
}

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
static void font_render_number(int x, int y, uint16_t num, uint8_t * p_buf ) {
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
static void font_render_digit(int x, int y, uint8_t digit, uint8_t * p_buf ) {
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
static void pixel_draw_contrast(int x, int y, uint8_t * p_buf) {

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
            *p_buf++ ^= 0x80; // = *p_buf + 128; // Red
            *p_buf++ ^= 0x80; // = *p_buf + 128; // Green
            *p_buf++ ^= 0x80; // = *p_buf + 128; // Blue
        }

        // handle opacity if needed
        if (bpp == 4)
            *p_buf++ = 255; //*p_buf + 128; // Blue
    }
}


// Draw a pixel with a given color
// Expects BPP to only = 3 or 4
/*
static void pixel_draw_color(int x, int y, uint8_t * p_buf, uint8_t r, uint8_t g, uint8_t b) {

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
*/

// Render a solid tile inverted at x,y
static void highlight_tile_rgb(uint8_t * p_buf, int tx, int ty) {

    int       x,y;
    uint32_t  row_gap_u8;

    // Pre-calculate the buffer distance from the
    // end of one row to the start of the next
    row_gap_u8       = (width - tile_width) * bpp;

    // Move down to first pixel of first row of tile
    p_buf += ((width * ty) + tx) * bpp;

    for (y=0; y < tile_height; y++) {
        for (x=0; x < tile_width; x++) {

            // Semi-invert the pixel
            *p_buf++ ^= 0x80; // R
            *p_buf++ ^= 0x80; // G
            *p_buf++ ^= 0x80; // B
        }
        // Advance image buffer to next horizontal grid line
        p_buf += row_gap_u8;
    }
}


// Render a solid tile inverted at x,y
static void highlight_tile_rgba(uint32_t * p_buf, int tx, int ty) {

    int        x,y;
    uint32_t   row_gap_u32;

    // Pre-calculate the buffer distance from the
    // end of one grid-line to the start of the next
    row_gap_u32       = (width  - tile_width);

    // Move down to first pixel of first row of tile
    p_buf += ((width * ty) + tx);

    for (y=0; y < tile_height; y++) {
        for (x=0; x < tile_width; x++) {

            // If the pixel is mostly visible, semi-invert it
            // If it's mostly transparent then set it to red + fully visible
            if (*p_buf & 0xC0000000)
                *p_buf ^= 0x00808080;
            else
                *p_buf = 0xFF0000FF;

            // Move right by one pixel (col)
            p_buf++;
        }
        // Advance image buffer to next horizontal grid line
        p_buf += row_gap_u32;
    }
}


static void render_highlight_tilenum (uint8_t * p_buf,
                                      uint32_t map_size,
                                      uint32_t * map_tilelist) {
    int x,y;
    int tile_index;

    tile_index = 0;

    // Overlay doesn't have access to tile_count right now
    // if (tile_to_hightlight >= tile_count) {
    //     printf("Overlay: Render Highlight Tilenum -> invalid tile number!\n");
    //     return;
    // }

    if (map_size != ((width / tile_width) * (height / tile_height))) {
        printf("Overlay: Render Highlight Tilenum -> WRONG MAP SIZE!\n");
        return;
    }

    for (y=0; y < height; y+= tile_height) {
        for (x=0; x < width; x+= tile_width) {

            if (map_tilelist[ tile_index++ ] == tile_to_hightlight ) {
                if (bpp == 3)
                    highlight_tile_rgb(p_buf, x, y);
                else if (bpp == 4)
                    highlight_tile_rgba((uint32_t * )p_buf, x, y);
            }
        }
    }

}


// Render a tile spaced grid of semi-inverted pixels in the image buffer
static void render_grid_rgb(uint8_t * p_buf) {

    uint8_t * p_pix;
    int       x,y;
    uint32_t  row_gap_u8, col_increment_u8;

    // Pre-calculate the buffer distance from the
    // end of one grid-line to the start of the next
    row_gap_u8       = (width * (tile_height - 1)) * bpp;
    col_increment_u8 = (width * bpp) - bpp;

    // Draw horizontal grid lines using the tile size
    p_pix = p_buf;

    for (y=0; y < height; y += tile_height) {
        for (x=0; x < width; x++) {

// TODO: renger grid rgb: handle transparency better here (see RGBA)
            // Semi-invert the pixel
            *p_pix++ ^= 0x20; // R
            *p_pix++ ^= 0x20; // G
            *p_pix++ ^= 0x20; // B
        }
        // Advance image buffer to next horizontal grid line
        p_pix += row_gap_u8;
    }


    // Draw veritcal grid lines using the tile size
    p_pix = p_buf;

    for (x=0; x < width; x += tile_width) {

        p_pix = p_buf + (x * bpp);

        for (y=0; y < height; y++) {

            // Semi-invert the pixel
            *p_pix++ ^= 0x20; // R
            *p_pix++ ^= 0x20; // G
            *p_pix++ ^= 0x20; // B

            // Move down by one pixel (row)
            p_pix += col_increment_u8;
        }
    }
}


// Render a tile spaced grid of semi-inverted pixels in the image buffer
static void render_grid_rgba(uint32_t * p_buf) {

    uint32_t * p_pix;
    int        x,y;
    uint32_t   row_gap_u32, col_increment_u32;

    // Pre-calculate the buffer distance from the
    // end of one grid-line to the start of the next
    row_gap_u32       = width * (tile_height - 1);
    col_increment_u32 = width;

    // Draw horizontal grid lines using the tile size
    p_pix = p_buf;

    for (y=0; y < height; y += tile_height) {
        for (x=0; x < width; x++) {

            // If the pixel is mostly visible, semi-invert it
            // If it's mostly transparent then set it to red + fully visible
            if (*p_pix & 0xC0000000)
                *p_pix ^= 0x00202020;
            else
                *p_pix = 0xFF0000FF;

            // Move right by one pixel (col)
            p_pix++;
        }
        // Advance image buffer to next horizontal grid line
        p_pix += row_gap_u32;
    }


    // Draw veritcal grid lines using the tile size
    p_pix = p_buf;

    for (x=0; x < width; x += tile_width) {

        p_pix = p_buf + x;

        for (y=0; y < height; y++) {

            // If the pixel is mostly visible, semi-invert it
            // If it's mostly transparent then set it to red + fully visible
            if (*p_pix & 0xC0000000)
                *p_pix ^= 0x00202020;
            else
                *p_pix = 0xFF0000FF;

            // Move down by one pixel (row)
            p_pix += col_increment_u32;
        }
    }
}


static void render_tilenums (uint8_t * p_buf, uint32_t map_size, uint32_t * map_tilelist) {

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


void tilemap_overlay_apply(uint32_t map_size, uint32_t * map_tilelist) {

//    printf("Overlay: Drawing now...\n");

    if (p_overlaybuf == NULL)
        return;

    printf("Overlay: Start -> Grid  ");
    benchmark_start();

    // Draw the tile grid
    if (grid_enabled) {
        if (bpp == 3)
            render_grid_rgb (p_overlaybuf);
        else if (bpp == 4)
            render_grid_rgba ((uint32_t * )p_overlaybuf);
    }

    benchmark_elapsed();
    printf("Overlay: Start -> Tilenums  ");

    // Draw the tile numbers
    if (tilenums_enabled)
        render_tilenums ( p_overlaybuf, map_size, map_tilelist);

    benchmark_elapsed();
    printf("Overlay: Start -> Highlight (%d) ", tile_to_hightlight);

    if (tile_to_hightlight != TILE_HIGHLIGHT_NONE)
        render_highlight_tilenum(p_overlaybuf, map_size, map_tilelist);

    benchmark_elapsed();

    overlay_redraw_clear_flag();
}


