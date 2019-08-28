//
// lib_tilemap.c
//
#include <stdio.h>
#include <string.h>


#include "lib_tilemap.h"
#include "tilemap_tiles.h"

#include "hash.h"

#include "benchmark.h"

// Globals
tile_map_data tile_map;
tile_set_data tile_set;
color_data    colormap;



// TODO: Fix the dubious mixing of global and locals. Simplify code

// TODO: support configurable tile size
int tilemap_initialize(image_data * p_src_img, int tile_width, int tile_height) {

    // Tile Map
    tile_map.map_width   = p_src_img->width;
    tile_map.map_height  = p_src_img->height;

    tile_map.tile_width  = tile_width;
    tile_map.tile_height = tile_height;

    tile_map.width_in_tiles  = tile_map.map_width  / tile_map.tile_width;
    tile_map.height_in_tiles = tile_map.map_height / tile_map.tile_height;

    // Max space required to store Tile Map is
    // width x height in tiles (if every map tile is unique)
    tile_map.size = (tile_map.width_in_tiles * tile_map.height_in_tiles);

    //tile_map.tile_id_list = malloc(tile_map.size * sizeof(int32_t)); // Why was this uint32_t?
    tile_map.tile_id_list = malloc(tile_map.size);

    if (!tile_map.tile_id_list)
            return(false);


    // Tile Set
    tile_set.tile_bytes_per_pixel = p_src_img->bytes_per_pixel;
    tile_set.tile_width  = tile_width;
    tile_set.tile_height = tile_height;
    tile_set.tile_size   = tile_set.tile_width * tile_set.tile_height * tile_set.tile_bytes_per_pixel;
    tile_set.tile_count  = 0;

    return (true);
}



unsigned char tilemap_export_process(image_data * p_src_img, int tile_width, int tile_height) {

    if ( check_dimensions_valid(p_src_img, tile_width, tile_height) ) {
        if (!tilemap_initialize(p_src_img, tile_width, tile_height)) { // Success, prep for processing
            printf("Tilemap: Process: tilemap_initialize: failed\n");
            return (false); // Signal failure and exit
        }
    }
    else {
        printf("Tilemap: Process: check_dimensions_valid: failed\n" );
        return (false); // Signal failure and exit
    }

    if ( ! process_tiles(p_src_img) )
        return (false); // Signal failure and exit
}



unsigned char process_tiles(image_data * p_src_img) {

    int         img_x, img_y;

    uint32_t  * tile_buf_intermediary; // Needs to be 32 bit aligned for hash function
    tile_data   tile;
    uint32_t    tile_size_bytes;
    uint32_t    tile_size_bytes_hash_padding; // Make sure hashed data is multiple of 32 bits
    uint32_t    img_buf_offset;
    int32_t     tile_id;
    int32_t     map_slot;

benchmark_slot_resetall();
printf("Tilemap: Start -> Process..  ");
benchmark_start();

    map_slot = 0;

    // TODO: why isn't this using pre-initialized values in tile_set ?
    tile.raw_bytes_per_pixel = p_src_img->bytes_per_pixel;
    tile.raw_width           = tile_map.tile_width;
    tile.raw_height          = tile_map.tile_height;
    tile.raw_size_bytes      = tile.raw_height * tile.raw_width * tile.raw_bytes_per_pixel;

    // Make sure buffer is an even multiple of 32 bits (for hash function)
    tile_size_bytes_hash_padding = tile_size_bytes % sizeof(uint32_t);

    // Allocate buffer for temporary working tile raw image
    // Use a uint32 for initial allocation, then hand it off to the uint8
    // TODO: fix this hack. rumor is that in PC world uint8 buffers always get 32 bit alligned?
    tile_buf_intermediary = malloc((tile_size_bytes + tile_size_bytes_hash_padding) / sizeof(uint32_t));
    tile.p_img_raw        = (uint8_t *)tile_buf_intermediary;

    // Make sure padding bytes are zeroed
    memset(tile.p_img_raw, 0x00, tile_size_bytes_hash_padding);



    if (tile.p_img_raw) {

        // Iterate over the map, top -> bottom, left -> right
        img_buf_offset = 0;

        for (img_y = 0; img_y < tile_map.map_height; img_y += tile_map.tile_height) {
            for (img_x = 0; img_x < tile_map.map_width; img_x += tile_map.tile_width) {

                // Set buffer offset to upper left of current tile
                img_buf_offset = (img_x + (img_y * tile_map.map_width)) * p_src_img->bytes_per_pixel;

benchmark_slot_start(0);
                tile_copy_tile_from_image(p_src_img,
                                          &tile,
                                          img_buf_offset);
benchmark_slot_update(0);


benchmark_slot_start(9);
                // TODO! Don't hash transparent pixels? Have to overwrite second byte?
                tile.hash = MurmurHash2( tile.p_img_raw, tile.raw_size_bytes, 0xF0A5); // len is u8count
benchmark_slot_update(9);


benchmark_slot_start(2);
                // TODO: search could be optimized with a hash array
                tile_id = tile_find_matching(tile.hash, &tile_set);
//printf("New Tile: (%3d, %3d) tile_id=%4d, tile_hash = %8lx \n", img_x, img_y, tile_id, tile.hash);
benchmark_slot_update(2);

                // Tile not found, create a new entry
                if (tile_id == TILE_ID_NOT_FOUND) {

benchmark_slot_start(3);
                    tile_id = tile_register_new(&tile, &tile_set);
benchmark_slot_update(3);

                    if (tile_id <= TILE_ID_OUT_OF_SPACE) {
                        // Free using the original pointer, not tile.p_img_raw
                        free(tile_buf_intermediary);
                        tile_buf_intermediary = NULL;
                        printf("Tilemap: Process: FAIL -> Too Many Tiles\n");
                        return (false); // Ran out of tile space, exit
                    }
                }

                int32_t test;
                test = tile_id;

                tile_map.tile_id_list[map_slot] = test; // = tile_id; // TODO: IMPORTANT, SOMETHING IS VERY WRONG

// printf("Map Slot %d: tile_id=%d tilemap[]=%d, %08lx\n",map_slot, tile_id, tile_map.tile_id_list[map_slot], tile.hash);
                map_slot++;
            }
        }

    } else { // else if (tile.p_img_raw) {
        if (tile_map.tile_id_list)
            free(tile_map.tile_id_list);
        return (false); // Failed to allocate buffer, exit
    }

    // Free using the original pointer, not tile.p_img_raw
    if (tile_buf_intermediary) {
        free(tile_buf_intermediary);
        tile_buf_intermediary = NULL;
    }

benchmark_elapsed();
benchmark_slot_printall();


//    printf("Tilemap: Process: Total Tiles=%d\n", tile_set.tile_count);
}


static int32_t check_dimensions_valid(image_data * p_src_img, int tile_width, int tile_height) {

    // TODO: propagate error up to user dialog

    // Image dimensions must be exact multiples of tile size
    if ( ((p_src_img->width % tile_width) != 0) ||
         ((p_src_img->height % tile_height ) != 0))
        return false; // Fail
    else
        return true;  // Success
}



void tilemap_free_resources() {

    int c;

    // Free all the tile set data
    for (c = 0; c < tile_set.tile_count; c++) {

        if (tile_set.tiles[c].p_img_encoded)
            free(tile_set.tiles[c].p_img_encoded);

        if (tile_set.tiles[c].p_img_raw)
            free(tile_set.tiles[c].p_img_raw);
    }

    // Free tile map data
    if (tile_map.tile_id_list)
        free(tile_map.tile_id_list);

}




tile_map_data * tilemap_get_map(void) {
    return (&tile_map);
}



tile_set_data * tilemap_get_tile_set(void) {
    return (&tile_set);
}



// TODO: Consider moving this to a different location
//
// Returns an image which is a composite of all the
// tiles in a tile map, in order.
int32_t tilemap_get_image_of_deduped_tile_set(image_data * p_img) {

    uint32_t c;
    uint32_t img_offset;

    // Set up image to store deduplicated tile set
    p_img->width  = tile_map.tile_width;
    p_img->height = tile_map.tile_height * tile_set.tile_count;
    p_img->size   = tile_set.tile_size   * tile_set.tile_count;
    p_img->bytes_per_pixel = tile_set.tile_bytes_per_pixel;

    // printf("== COPY TILES INTO COMPOSITE BUF %d x %d, total size=%d\n", p_img->width, p_img->height, p_img->size);

    // Allocate a buffer for the image
    p_img->p_img_data = malloc(p_img->size);

    if (p_img->p_img_data) {

        img_offset = 0;

        for (c = 0; c < tile_set.tile_count; c++) {

            if (tile_set.tiles[c].p_img_raw) {
                // Copy from the tile's raw image buffer (indexed)
                // into the composite image
                memcpy(p_img->p_img_data + img_offset,
                       tile_set.tiles[c].p_img_raw,
                       tile_set.tile_size);

                // tile_print_buffer_raw(tile_set.tiles[c]); // TODO: remove
            }
            else
                return false;

            img_offset += tile_set.tile_size;
        }
    }
    else
        return false;

    return true;
}


// Set local indexed color map for later retrieval
void tilemap_color_data_set(color_data * p_color_data) {
    memcpy(&colormap, p_color_data, sizeof(color_data));
}

// Return pointer to locally stored indexed color map
color_data * tilemap_color_data_get(void) {
    return &colormap;
}
