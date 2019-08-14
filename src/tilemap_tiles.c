//
// tilemap_tiles.c
//

#include <stdio.h>
#include <string.h>


#include "lib_tilemap.h"
#include "tilemap_tiles.h"


int32_t tile_register_new(tile_data * p_src_tile, tile_set_data * tile_set) {

    int32_t     tile_id;
    tile_data * new_tile;

// printf("tile_register_new %d\n",tile_set->tile_count);

    if (tile_set->tile_count < TILES_MAX_DEFAULT) {

        // Set tile id to the current tile
        tile_id = tile_set->tile_count;

        // Use an easier to read name for the new tile entry
        new_tile = &tile_set->tiles[tile_id];

        // Store hash and encoded image data into tile
        new_tile->hash = p_src_tile->hash;
        new_tile->encoded_size_bytes = 0;
        new_tile->raw_bytes_per_pixel = p_src_tile->raw_bytes_per_pixel;
        new_tile->raw_width           = p_src_tile->raw_width;
        new_tile->raw_height          = p_src_tile->raw_height;

        // Copy raw tile data into tile image buffer
        new_tile->raw_size_bytes = p_src_tile->raw_size_bytes;
        new_tile->p_img_raw      = malloc(p_src_tile->raw_size_bytes);

        if (new_tile->p_img_raw) {

            memcpy(new_tile->p_img_raw,
                   p_src_tile->p_img_raw,
                   p_src_tile->raw_size_bytes);

            // TODO: DON'T ENCODE - just keep raw (above), so always increment
            tile_set->tile_count++;
/*

            // Copy encoded tile data into tile buffer
            tile_encode(&(tile_set->tiles[tile_id]),
                        BIN_MODE_SNESGB_2BPP);

            if (tile_set->tiles[tile_id].encoded_size_bytes) {
                // Move to new tile
                tile_set->tile_count++;

            } else // encoding failed
                tile_id = TILE_ID_FAILED_ENCODE;
*/

        } else // malloc failed
            tile_id = TILE_ID_OUT_OF_SPACE;
    }
    else
        tile_id = TILE_ID_OUT_OF_SPACE;

// printf("tile_register_new tile_id=%d\n",tile_id);

    return (tile_id);
}


int32_t tile_find_matching(uint64_t hash_sig, tile_set_data * tile_set) {

    int c;

    for (c = 0; c < tile_set->tile_count; c++) {
        if (hash_sig == tile_set->tiles[c].hash) {
            // found a matching tile, return it's ID
            return(c);
        }
    }

    // No matching tile found
    return(TILE_ID_NOT_FOUND);
}


void tile_copy_tile_from_image(image_data * p_src_img,
                              tile_data * p_tile,
                            uint32_t img_buf_offset) {

    int32_t tile_y;
    int32_t tile_img_offset;
    int32_t image_width_bytes;
    int32_t tile_width_bytes;

    tile_img_offset   = 0;
    image_width_bytes = p_src_img->width  * p_src_img->bytes_per_pixel;
    tile_width_bytes  = p_tile->raw_width * p_tile->raw_bytes_per_pixel;

    if (!p_tile->p_img_raw)
        return;

    // Iterate over each tile, top -> bottom, left -> right
    for (tile_y = 0; tile_y < p_tile->raw_height; tile_y++) {

        // Copy a row of tile data into the temp tile buffer
        memcpy(p_tile->p_img_raw + tile_img_offset,
               p_src_img->p_img_data + img_buf_offset,
               tile_width_bytes);

        // Move to the next row in image
        img_buf_offset += image_width_bytes;

        // Move to next row in tile buffer
        tile_img_offset += tile_width_bytes;
    }
}



// TODO: DEBUG: REMOVE ME
void tile_print_buffer_raw(tile_data tile) {

    int32_t tile_y;
    int32_t tile_x;

    printf("\n");

    // Iterate over each tile, top -> bottom, left -> right
    for (tile_y = 0; tile_y < tile.raw_height; tile_y++) {
        for (tile_x = 0; tile_x < tile.raw_width; tile_x++) {

            printf(" %2x", *(tile.p_img_raw));

            // Move to the next row
            tile.p_img_raw += tile.raw_bytes_per_pixel;
        }
        printf(" \n");
    }

    printf(" \n");
}
