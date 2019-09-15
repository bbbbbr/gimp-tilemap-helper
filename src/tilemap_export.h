//
// tilemap_export.h
//

#ifndef __TILEMAP_EXPORT_H_
#define __TILEMAP_EXPORT_H_

    #include <stdio.h>
    #include <string.h>

    #include "lib_tilemap.h"

    uint32_t tilemap_export_c_source_to_string(char * p_dest_str, uint32_t max_len,
                                               char * p_prefix_str,
                                               tile_map_data * p_map, tile_set_data * p_tile_set);

    uint32_t tilemap_export_asm_rgbds_source_to_string(char * p_dest_str, uint32_t max_len,
                                                       char * p_prefix_str,
                                                       tile_map_data * p_map, tile_set_data * p_tile_set);

#endif