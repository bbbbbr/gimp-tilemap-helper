//
// filter_tilemap.c
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

#include "filter_dialog.h"
// #include "filter_scalers.h"



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

typedef struct
{
  gint  tile_width;
  gint  tile_height;
//  gbool overlay_enabled;
//  gint  offset_x;
//  gint  offset_y;

} PluginTileMapVals;// PluginPixelArtScalerVals;


// Default settings for semi-persistant plugin config
static PluginTileMapVals plugin_config_vals = // PluginPixelArtScalerVals plugin_config_vals =
{
  8, // gint  tile_width;
  8 // gint  tile_height;
//  ,1  // gbool overlay_enabled;
};




MAIN()

// The query function
static void query(void)
{
    static const GimpParamDef args[] =
    {
        { GIMP_PDB_INT32,    "run-mode",    "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
        { GIMP_PDB_IMAGE,    "image",       "Input image (unused)" },
        { GIMP_PDB_DRAWABLE, "drawable",    "Input drawable" },
        { GIMP_PDB_INT32,    "tile_width",  "Tile Width(0-N)" },
        { GIMP_PDB_INT32,    "tile_height",  "Tile Width(0-N)" }
//        { GIMP_PDB_INT32,    "overlay_enabled", "Show overlay on plugin preview" }
    };


    gimp_install_procedure (PLUG_IN_PROCEDURE,
                            "Tilemap Helper",
                            "Tilemap Helper",
                            "--",
                            "Copyright --",
                            "2019",
                            "T_ilemap Helper ...",
                            "RGB*, INDEXED*",
                            GIMP_PLUGIN,
                            G_N_ELEMENTS (args),
                            0,
                            args,
                            NULL);

    gimp_plugin_menu_register (PLUG_IN_PROCEDURE, "<Image>/Filters/Map");
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

    run_mode      = param[0].data.d_int32;

    //  Get the specified drawable
    drawable = gimp_drawable_get (param[2].data.d_drawable);

    *nreturn_vals = 1;
    *return_vals  = return_values;

    // Set the return value to success by default
    return_values[0].type = GIMP_PDB_STATUS;
    // return_values[0].data.d_status = GIMP_PDB_SUCCESS;
    return_values[0].data.d_status = status;

printf("Filter Main: run mode=%d\n",run_mode);


    switch (run_mode) {
        case GIMP_RUN_INTERACTIVE:
            //  Try to retrieve plugin settings, then apply them
            gimp_get_data (PLUG_IN_PROCEDURE, &plugin_config_vals);
            // TODO: Set settings/config
            //      tilemap_setting_tilesize_set(plugin_config_vals.tile_width, plugin_config_vals.tile_height);

            //  Open the dialog
            // TODO: spawn dialog
                if (! tilemap_dialog_show (drawable))
                    return;
            break;

        case GIMP_RUN_NONINTERACTIVE:
            // Read in non-interactive mode plug settings, then apply them
            plugin_config_vals.tile_width  = param[3].data.d_int32;
            plugin_config_vals.tile_height = param[4].data.d_int32;
            // TODO: Set settings/config
            //      tilemap_setting_tilesize_set(plugin_config_vals.tile_width,
            //                                   plugin_config_vals.tile_height);
            break;

        case GIMP_RUN_WITH_LAST_VALS:
            //  Try to retrieve plugin settings, then apply them
            gimp_get_data (PLUG_IN_PROCEDURE, &plugin_config_vals);
            // TODO: Set settings/config
            //      tilemap_setting_tilesize_set(plugin_config_vals.tile_width,
            //                                   plugin_config_vals.tile_height);
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

        // Retrieve and then save plugin config settings
        if (run_mode == GIMP_RUN_INTERACTIVE) {
            // TODO: Get settings/config
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
}


