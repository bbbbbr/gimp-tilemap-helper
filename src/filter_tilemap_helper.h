//
// filter_tilemap_helper.h
//

#ifndef __FILTER_TILEMAP_HELPER_H_
#define __FILTER_TILEMAP_HELPER_H_

    typedef struct
    {
        gint  tile_width;
        gint  tile_height;
        gint  scale_factor;
        gint overlay_grid_enabled;
        gint overlay_tileids_enabled;
    //  gint  offset_x;
    //  gint  offset_y;

    } PluginTileMapVals;

#endif