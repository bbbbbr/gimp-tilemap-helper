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

void tilemap_overlay_set_highlight_tile(int tile_id);
void tilemap_overlay_clear_highlight_tile(void);

void overlay_redraw_invalidate(void);
void overlay_redraw_clear_flag(void);
int  overlay_redraw_needed(void);

#endif
