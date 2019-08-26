//
// tilemap_export.h
//

#ifndef __TILEMAP_EXPORT_H_
#define __TILEMAP_EXPORT_H_

    #include <stdio.h>
    #include <string.h>

    #include "lib_tilemap.h"
    //#include "tilemap_io.h"
    //#include "tilemap_path_ops.h"

    uint32_t tilemap_export_c_source_to_string(char * p_dest_str, uint32_t max_len, tile_map_data * tile_map, tile_set_data * tile_set);

#endif