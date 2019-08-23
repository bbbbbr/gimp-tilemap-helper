//
// filter_image.c
//

// ========================
//
// Image processing support tools
//
// ========================

/*

#include <stdio.h>
#include <string.h>

#include "filter_image.h"
#include "lib_tilemap.h"


// Create a new image from the deduplicated tileset
//
// * Call this only after tileset processing has completed
gint tilemap_create_tileset_image(guchar * p_colormap_buf,
                                    gint   colormap_numcolors) {

    gint            status;

    tile_map_data * p_map;
    tile_set_data * p_tile_set;
    image_data      tile_set_deduped_image;

    gint           new_image_id, new_layer_id;
    GimpDrawable * drawable;
    GimpPixelRgn   rgn;


    // Retrieve the deduplicated map and tile set
    p_map      = tilemap_get_map();
    p_tile_set = tilemap_get_tile_set();

    // Convert deduplicated map tile set to a single (tall) image
    status     = tilemap_get_image_of_deduped_tile_set(&tile_set_deduped_image);

    // Create a new gimp image, then add the tileset to it
    if (p_map && p_tile_set && status) {

        // == START UNDO GROUPING ==
//        gimp_image_undo_group_start(gimp_item_get_image(drawable->drawable_id));

        // Create new image
        gint new_image_id = gimp_image_new(tile_set_deduped_image.width,
                                           tile_set_deduped_image.height,
                                           tile_set_deduped_image.bytes_per_pixel);

        // Create the new layer
        new_layer_id = gimp_layer_new(new_image_id,
                                      "Background",
                                      tile_set_deduped_image.width,
                                      tile_set_deduped_image.height,
                                      tile_set_deduped_image.bytes_per_pixel,
                                      100,  // opacity
                                      GIMP_NORMAL_MODE);

        // Get the drawable for the layer
        drawable = gimp_drawable_get(new_layer_id);

        // Set up color map if it's indexed or indexed-alpha
        if (tile_set_deduped_image.bytes_per_pixel <= 2) //
            gimp_image_set_colormap(new_image_id, p_colormap_buf, colormap_numcolors);

        // Get a pixel region from the layer
        gimp_pixel_rgn_init(&rgn,
                            drawable,
                            0, 0,
                            tile_set_deduped_image.width,
                            tile_set_deduped_image.height,
                            TRUE, FALSE); // dirty=true, shadow=false (write directly without shadow)

        // Now set the pixel data
        gimp_pixel_rgn_set_rect(&rgn,
                                tile_set_deduped_image.p_img_data, // app_gfx.p_data,
                                0, 0,
                                tile_set_deduped_image.width,
                                tile_set_deduped_image.width);

        // Done with the drawable
        gimp_drawable_flush(drawable);
        gimp_drawable_detach(drawable);

        // Add the layer to the image
        gimp_image_add_layer(new_image_id, new_layer_id, 0);

        // Set the filename
        gimp_image_set_filename(new_image_id, "tile set");


        // == END GROUPING
  //      gimp_image_undo_group_end(gimp_item_get_image(drawable->drawable_id));

    }
    else
        return FALSE;

}
*/