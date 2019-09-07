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

int tilemap_needs_recalc;

void tilemap_free_tile_set();
void tile_calc_alternate_hashes(tile_data *);

void tilemap_recalc_invalidate(void) {

    printf("Tilemap: recalc invalidated\n");
    tilemap_needs_recalc = true;
    tilemap_free_tile_set(); // Free tiles in set since they will get overwritten

}


void tilemap_recalc_clear_flag(void) {
    printf("Tilemap: recalc flag cleared\n");
    tilemap_needs_recalc = false;
}


int tilemap_recalc_needed(void) {
    return tilemap_needs_recalc;
}


// Handles whether to check/calculate hash for flip x/y of tile hash
void tilemap_search_mask_set(uint16_t search_mask_new) {

    tile_map.search_mask = search_mask_new;

//    if (tile_map.search_mask != search_mask_new) {
//        tile_map.search_mask = search_mask_new;
//        tilemap_recalc_invalidate();
//    }
}


// TODO: some mixing of global and locals. Simplify code
int tilemap_initialize(image_data * p_src_img, int tile_width, int tile_height) {

    // Tile Map
    tile_map.map_width   = p_src_img->width;
    tile_map.map_height  = p_src_img->height;

    tile_map.tile_width  = tile_width;
    tile_map.tile_height = tile_height;

    tile_map.width_in_tiles  = tile_map.map_width  / tile_map.tile_width;
    tile_map.height_in_tiles = tile_map.map_height / tile_map.tile_height;

    // Normal orientation search only, no flip x/y by default
    tile_map.search_mask = TILE_FLIP_NONE;

    // Max space required to store Tile Map is
    // width x height in tiles (if every map tile is unique)
    tile_map.size = (tile_map.width_in_tiles * tile_map.height_in_tiles);

    tile_map.tile_id_list = malloc(tile_map.size * sizeof(uint32_t));
    if (!tile_map.tile_id_list)
            return(false);

    tile_map.tile_attribs_list = malloc(tile_map.size * sizeof(uint16_t));
    if (!tile_map.tile_attribs_list)
            return(false);

    // Tile Set
    tile_set.tile_bytes_per_pixel = p_src_img->bytes_per_pixel;
    tile_set.tile_width  = tile_width;
    tile_set.tile_height = tile_height;
    tile_set.tile_size   = tile_set.tile_width * tile_set.tile_height * tile_set.tile_bytes_per_pixel;
    tile_set.tile_count  = 0;

    tilemap_recalc_invalidate();

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

    tilemap_recalc_clear_flag();
}


unsigned char process_tiles(image_data * p_src_img) {

    int         img_x, img_y;

    tile_data      tile;
    tile_map_entry map_entry;
    uint32_t       img_buf_offset;
    int32_t        tile_ret;
    int32_t        map_slot;

benchmark_slot_resetall();
printf("Tilemap: Start -> Process..  ");
benchmark_start();

    map_slot = 0;

    // Use pre-initialized values in from tilemap_initialize()
    tile_initialize(&tile, &tile_map, &tile_set);

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
                // TODO: BUG? Is this missing the extra tile 32 bit padding bytes?
                tile.hash[0] = MurmurHash2( tile.p_img_raw, tile.raw_size_bytes, 0xF0A5); // len is u8count
                benchmark_slot_update(9);


                benchmark_slot_start(2);
                // TODO: search could be optimized with a hash array
                map_entry = tile_find_match(tile.hash[0], &tile_set, tile_map.search_mask);
                //printf("New Tile: (%3d, %3d) tile_id=%4d, tile_hash[0] = %8lx \n", img_x, img_y, tile_id, tile.hash[0]);
                benchmark_slot_update(2);

                // Tile not found, create a new entry
                if (map_entry.id == TILE_ID_NOT_FOUND) {

                    benchmark_slot_start(3);
                    // Calculate remaining hash flip variations
                    tile_calc_alternate_hashes(&tile);
                    benchmark_slot_update(3);

                    benchmark_slot_start(4);
                    map_entry = tile_register_new(&tile, &tile_set, tile_map.search_mask);
                    benchmark_slot_update(4);

                    if (map_entry.id == TILE_ID_OUT_OF_SPACE) {

                        if (tile.p_img_raw) {
                            free(tile.p_img_raw);
                            tile.p_img_raw = NULL;
                        }

                        tilemap_free_resources();

                        printf("Tilemap: Process: FAIL -> Too Many Tiles\n");
                        return (false); // Ran out of tile space, exit
                    }
                }
                else // if (map_entry.id == TILE_ID_NOT_FOUND)
                    tile_set.tiles[map_entry.id].map_entry_count++; // increment tile in map usage entry count

                tile_map.tile_id_list[map_slot]      = map_entry.id;
                tile_map.tile_attribs_list[map_slot] = map_entry.attribs;

                map_slot++;
            }
        }

    } else { // else if (tile.p_img_raw) {
        tilemap_free_resources();
        return (false); // Failed to allocate buffer, exit
    }

    // Free using the original pointer, not tile.p_img_raw
    if (tile.p_img_raw)
        free(tile.p_img_raw);
    tile.p_img_raw = NULL;

benchmark_elapsed();
benchmark_slot_printall();


//    printf("Tilemap: Process: Total Tiles=%d\n", tile_set.tile_count);
}


void tile_calc_alternate_hashes(tile_data * p_tile) {

    int h;
    // Calculate the remaining requested hash flip permutations
    for (h = TILE_FLIP_X; h <= TILE_FLIP_MAX; h++)
        if (h & tile_map.search_mask)
            p_tile->hash[h] = MurmurHash2( p_tile->p_img_raw, p_tile->raw_size_bytes, 0xF0A5); // len is u8count

//  TODO: CALCULATE THE FLIPPED TILE HASHES
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



void tilemap_free_tile_set() {
        int c;

    // Free all the tile set data
    for (c = 0; c < tile_set.tile_count; c++) {

        if (tile_set.tiles[c].p_img_encoded)
            free(tile_set.tiles[c].p_img_encoded);
        tile_set.tiles[c].p_img_encoded = NULL;

        if (tile_set.tiles[c].p_img_raw)
            free(tile_set.tiles[c].p_img_raw);
        tile_set.tiles[c].p_img_raw = NULL;
    }

    tile_set.tile_count  = 0;
}

void tilemap_free_resources() {

    tilemap_free_tile_set();

    // Free tile map data
    if (tile_map.tile_id_list) {
        free(tile_map.tile_id_list);
        tile_map.tile_id_list = NULL;
    }

    if (tile_map.tile_attribs_list) {
        free(tile_map.tile_attribs_list);
        tile_map.tile_id_list = NULL;
    }

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
