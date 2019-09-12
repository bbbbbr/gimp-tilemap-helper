//
// filter_dialog.h
//

#ifndef __FILTER_DIALOG_H_
#define __FILTER_DIALOG_H_

    // #pragma message ( "INCLUDED: filter_dialog.h" )

    #include <stdint.h>

    enum dialog_widgets {
        WIDGET_TILESIZE_WIDTH,
        WIDGET_TILESIZE_HEIGHT,
    };

    #define ARRAY_LEN(x)  (int)(sizeof(x) / sizeof((x)[0]))


    gint tilemap_dialog_show(GimpDrawable *drawable);
    void tilemap_dialog_processing_run (GimpDrawable *drawable, GimpPreview  *preview);

    void tilemap_dialog_settings_set(PluginTileMapVals * plugin_config_vals);
    void tilemap_dialog_settings_get(PluginTileMapVals * plugin_config_vals);

    void tilemap_dialog_imageid_set(gint32 new_image_id);

    void dialog_free_resources(void);
#endif
