//
// filter_tilemap_helper.h
//

#ifndef __FILTER_TILEMAP_HELPER_H_
#define __FILTER_TILEMAP_HELPER_H_

    #define MAP_PREFIX_MAX_LEN 50

    typedef struct
    {
        gint  tile_width;
        gint  tile_height;
        gint  scale_factor;

        gint  overlay_grid_enabled;
        gint  overlay_tileids_enabled;

        gint  finalbpp;

        gint  flattened_image;

        gint  check_flip;

        gint  maptoclipboard_type;

        gchar maptoclipboard_prefix_str[MAP_PREFIX_MAX_LEN + 1];

    //  gint  offset_x;
    //  gint  offset_y;

    } PluginTileMapVals;

#endif