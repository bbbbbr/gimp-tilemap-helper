// tilemap_overlay.h

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#ifndef TILEMAP_OVERLAY_H
#define TILEMAP_OVERLAY_H

void tilemap_overlay_setparams(uint8_t * p_overlaybuf_new,
                               int bpp_new,
                               int width_new, int height_new,
                               int tile_width_new, int tile_height_new);

void tilemap_overlay_set_enables(int grid_enabled, int tilenums_enabled);

void tilemap_overlay_apply(uint32_t map_size, uint32_t * map_tilelist);

void overlay_redraw_invalidate(void);
void overlay_redraw_clear_flag(void);
int  overlay_redraw_needed(void);


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


#endif