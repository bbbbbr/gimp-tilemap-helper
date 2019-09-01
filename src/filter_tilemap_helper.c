//
// filter_tilemap_helper.c
//

// ========================
//
// Registers filter with Gimp, calls
// dialog for processing and preview
//
// ========================


#include <string.h>
#include <stdint.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "filter_tilemap_helper.h"
#include "filter_dialog.h"
#include "scale.h"
#include "lib_tilemap.h"
#include "filter_image.h"


const char PLUG_IN_PROCEDURE[] = "filter-tilemap-proc";
const char PLUG_IN_ROLE[]      = "gimp-tilemap-helper";
const char PLUG_IN_BINARY[]    = "plugin-gimp-tilemap-helper";


// Predeclare entrypoints
static void query(void);
static void run(const gchar *, gint, const GimpParam *, gint *, GimpParam **);



// Declare plugin entry points
GimpPlugInInfo PLUG_IN_INFO = {
    NULL,
    NULL,
    query,
    run
};



// Default settings for semi-persistant plugin config
static PluginTileMapVals plugin_config_vals = // PluginPixelArtScalerVals plugin_config_vals =
{
  8,  // gint  tile_width;
  8,  // gint  tile_height;
  2,  // gint  scale_factor;
  1,  // gint overlay_grid_enabled;
  1,  // gint overlay_tileids_enabled;
  0,  // gint  finalbpp;
  1,  // gint flattened_image;
};




MAIN()

// The query function
static void query(void)
{
    static const GimpParamDef args[] =
    {
        { GIMP_PDB_INT32,    "run-mode",    "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
        { GIMP_PDB_IMAGE,    "image",       "Input image" },
        { GIMP_PDB_DRAWABLE, "drawable",    "Input drawable" },
        { GIMP_PDB_INT32,    "tile_width",  "Tile Width(0-N)" },
        { GIMP_PDB_INT32,    "tile_height",  "Tile Width(0-N)" }
//        { GIMP_PDB_BOOL,    "overlay_enabled", "Show overlay on plugin preview" }
    };


    gimp_install_procedure (PLUG_IN_PROCEDURE,
                            "Tilemap Helper",
                            "Tilemap Helper",
                            "--",
                            "Copyright --",
                            "2019",
                            "T_ilemap Helper ...",
//                            "RGB*",
                            "RGB*, INDEXED*", // TODO: greyscale* support
                            GIMP_PLUGIN,
                            G_N_ELEMENTS (args),
                            0,
                            args,
                            NULL);

    gimp_plugin_menu_register (PLUG_IN_PROCEDURE, "<Image>/Filters/Map");
}



// Create deduplicated tileset if requested when the user closed the dialog
static void handle_tileset_create(gint * nreturn_vals, GimpParam * return_values) {

    int                new_image_id; // used if a tile set image is created

    // TODO: export tile map as well?

    // Try to create new image from deduplicated tile set
    new_image_id = tilemap_create_tileset_image();

    if(new_image_id == -1)
        // create image failed
        return_values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
    else
    {
        // Indicate successful creation of image
        return_values[0].data.d_status = GIMP_PDB_SUCCESS;

        // Fill in the second return value
        *nreturn_vals = 2;
        return_values[1].type         = GIMP_PDB_IMAGE;
        return_values[1].data.d_image = new_image_id;
    }
}



// The run function
static void run(const gchar      * name,
                      gint         nparams,
                const GimpParam  * param,
                      gint       * nreturn_vals,
                      GimpParam ** return_vals)
{
    // Create the return value.
    static GimpParam   return_values[2];
    GimpRunMode        run_mode;
    GimpDrawable       *drawable;
    GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
    gint32             image_id, drawable_id;
    gint               dialog_response;

    run_mode      = param[0].data.d_int32;

    //  Get the specified drawable
    image_id    = param[1].data.d_int32;
    drawable_id = param[2].data.d_int32;
    drawable = gimp_drawable_get (param[2].data.d_drawable);

    *nreturn_vals = 1;
    *return_vals  = return_values;

    // Set the return value to success by default
    return_values[0].type = GIMP_PDB_STATUS;
    // return_values[0].data.d_status = GIMP_PDB_SUCCESS;
    return_values[0].data.d_status = status;

printf("================================= Filter Main: run mode=%d  image_id = %d   ================================= \n\n\n\n",run_mode, image_id);

    scale_init();
    tilemap_dialog_imageid_set(image_id);


    switch (run_mode) {
        case GIMP_RUN_INTERACTIVE:

            //  Try to retrieve plugin settings, then apply them
            gimp_get_data (PLUG_IN_PROCEDURE, &plugin_config_vals);

            // Set settings/config in dialog
            tilemap_dialog_settings_set(&plugin_config_vals);

            //  Open the dialog
            dialog_response = tilemap_dialog_show (drawable);

            // Handle response from dialog (which button the user pressed)
            switch (dialog_response) {
                case GTK_RESPONSE_CANCEL: // Do nothing, exit
                                          return;

                case GTK_RESPONSE_APPLY:  handle_tileset_create(nreturn_vals, return_values);
                                          return; // No more to do, exit plugin

                case GTK_RESPONSE_OK:     // Shim reponse.. continue below (TODO: move "keep settings" handling up here)
                                          break;
            }

            break;
/*
        case GIMP_RUN_NONINTERACTIVE:
            // Read in non-interactive mode plugin settings, then apply them
            plugin_config_vals.tile_width   = param[3].data.d_int32;
            plugin_config_vals.tile_height  = param[4].data.d_int32;
            plugin_config_vals.scale_factor = param[5].data.d_int32;

            // Set settings/config in dialog
            tilemap_dialog_settings_set(&plugin_config_vals);
            break;
*/
        case GIMP_RUN_WITH_LAST_VALS:
            //  Try to retrieve plugin settings, then apply them
            gimp_get_data (PLUG_IN_PROCEDURE, &plugin_config_vals);

            // Set settings/config in dialog
            tilemap_dialog_settings_set(&plugin_config_vals);

            break;

        default:
            break;
    }

    if (status == GIMP_PDB_SUCCESS) {
        // TODO: replace all this with export / etc post-dialog handling

        /*
        //  Make sure that the drawable is RGB color
        if (gimp_drawable_is_rgb (drawable->drawable_id))
        {
        gimp_progress_init ("Pixel-Art-Scalers");


        // Apply image filter (user confirmed in preview dialog)
        pixel_art_scalers_run(drawable, NULL);

        if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();
        */

        // TODO: move this to a function, then call it from the above dialog response handler for ..._OK
        // Retrieve and then save plugin config settings
        if (run_mode == GIMP_RUN_INTERACTIVE) {
            // Get settings/config from dialog
            // Set settings/config in dialog
            tilemap_dialog_settings_get(&plugin_config_vals);

            // tilemap_setting_tilesize_get(&(plugin_config_vals.tile_width),
            //                              &(plugin_config_vals.tile_height));
            gimp_set_data (PLUG_IN_PROCEDURE,
                           &plugin_config_vals,
                           sizeof (PluginTileMapVals));
        }
    } // end : if (status == GIMP_PDB_SUCCESS)
    else {
            status                         = GIMP_PDB_EXECUTION_ERROR;
            *nreturn_vals                  = 2;
            return_values[1].type          = GIMP_PDB_STRING;
            return_values[1].data.d_string = "Unable to process image";
            // TODO: better error messaging
    }

    // TODO:
    // tilemap_release_resources();

    return_values[0].data.d_status = status;

    gimp_drawable_detach (drawable);

    dialog_free_resources();
    tilemap_free_resources();
}


