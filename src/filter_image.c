//
// filter_image.c
//

// ========================
//
// Image processing support tools
//
// ========================


#include <stdio.h>
#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "filter_image.h"
#include "lib_tilemap.h"


// Retrieve a GimpImageBaseType/GimpImageType for a given bpp
// * Assumes grayscale is never used
static void get_color_modes_from_bpp(gint bpp, gint * color_mode_image, gint * color_mode_layer) {

    switch (bpp) {
        case 1: *color_mode_layer = GIMP_INDEXED_IMAGE;
                *color_mode_image = GIMP_INDEXED;
                break;

        case 2: *color_mode_layer = GIMP_INDEXEDA_IMAGE;
                *color_mode_image = GIMP_INDEXED;
                break;

        case 3: *color_mode_layer = GIMP_RGB_IMAGE;
                *color_mode_image = GIMP_RGB;
                break;

        case 4: *color_mode_layer = GIMP_RGBA_IMAGE;
                *color_mode_image = GIMP_RGB;
                break;
    }
}


// Create a new image from the deduplicated tileset
//
// * Call this only after tileset processing has completed: tilemap_initialize() -> tilemap_export_process()
// * If the source image is indexed, tilemap_color_data_set() must be called first
gint tilemap_create_tileset_image(void) {

    gint            status;

    tile_map_data * p_map;
    tile_set_data * p_tile_set;
    color_data    * p_colormap;
    image_data      tile_set_deduped_image;

    gint           color_mode_image, color_mode_layer;
    gint           new_image_id, new_layer_id;
    GimpDrawable * new_drawable;
    GimpPixelRgn   rgn;


    // Retrieve the deduplicated map and tile set
    p_map      = tilemap_get_map();
    p_tile_set = tilemap_get_tile_set();
    p_colormap = tilemap_color_data_get();

    // Convert deduplicated map tile set to a single (tall) image
    status     = tilemap_get_image_of_deduped_tile_set(&tile_set_deduped_image);

    // Create a new gimp image, then add the tileset to it
    if (p_map && p_tile_set && status) {

    get_color_modes_from_bpp(tile_set_deduped_image.bytes_per_pixel, &color_mode_image, &color_mode_layer);

        // Create new image
        new_image_id = gimp_image_new(tile_set_deduped_image.width,
                                      tile_set_deduped_image.height,
                                      color_mode_image);

        if (!new_image_id)
            return -1;


        // == START UNDO GROUPING ==
        // (An image id is needed before the undo group can be started)
        gimp_image_undo_group_start(new_image_id);

        // Create the new layer
        new_layer_id = gimp_layer_new(new_image_id,
                                      "Tileset",
                                      tile_set_deduped_image.width,
                                      tile_set_deduped_image.height,
                                      color_mode_layer,
                                      100,  // opacity
                                      GIMP_NORMAL_MODE);
        if (!new_layer_id)
            return -1;

        // Get the drawable for the layer
        new_drawable = gimp_drawable_get(new_layer_id);

        if (!new_drawable)
            return -1;

        // Set up color map if it's indexed or indexed-alpha
        if (color_mode_image == GIMP_INDEXED) {
            if (p_colormap->color_count > 0)
                gimp_image_set_colormap(new_image_id, p_colormap->pal, p_colormap->color_count);
            else {
                // printf("  ERROR: no color data\n");
                return -1; // Abort if there aren't any colors when there should be
            }
        }

        // Get a pixel region from the layer
        gimp_pixel_rgn_init(&rgn,
                            new_drawable,
                            0, 0,
                            tile_set_deduped_image.width,
                            tile_set_deduped_image.height,
                            TRUE, FALSE); // dirty=true, shadow=false (write directly without shadow)

        // Now set the pixel data
        gimp_pixel_rgn_set_rect(&rgn,
                                (guchar *) tile_set_deduped_image.p_img_data, // app_gfx.p_data,
                                0, 0,
                                tile_set_deduped_image.width,
                                tile_set_deduped_image.height);


        // Done with the drawable
        gimp_drawable_flush(new_drawable);
        gimp_drawable_detach(new_drawable);

        // Add the layer to the image
	// Note: gimp_image_add_layer() has been deprecated
        // gimp_image_add_layer(new_image_id, new_layer_id, 0);
	gimp_image_insert_layer (new_image_id, 
                                 new_layer_id, 
                                 0,  // parent_ID = 0 (none)
                                 0); // position = 0

        // Set the filename
        gimp_image_set_filename(new_image_id, "Tileset");

        // Create a display view for the new image
        gimp_display_new (new_image_id);

        // == END GROUPING
        gimp_image_undo_group_end(new_image_id);

    }
    else {
        // printf("  ERROR: Data NOT found\n");
        return -1;
    }

    // Success, pass along ID for newly created image
    return new_image_id;
}

