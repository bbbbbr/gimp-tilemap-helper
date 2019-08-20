//
// filter_dialog.h
//

#ifndef __FILTER_DIALOG_H_
#define __FILTER_DIALOG_H_

    #include <stdint.h>

    enum dialog_widgets {
        WIDGET_TILESIZE_WIDTH,
        WIDGET_TILESIZE_HEIGHT,
    };

    const gchar * const finalbpp_strs[] = { "Src Image", "1", "2", "3", "4", "8", "16", "24", "32"};
    const gchar * const srcbpp_str[] = {" ", "INDEXED", "INDEXED-A", "RGB", "RGB-A"};

    #define ARRAY_LEN(x)  (int)(sizeof(x) / sizeof((x)[0]))


    gboolean tilemap_dialog_show(GimpDrawable *drawable);
    void     tilemap_dialog_processing_run (GimpDrawable *drawable, GimpPreview  *preview);

    void tilemap_dialog_settings_set(PluginTileMapVals * plugin_config_vals);
    void tilemap_dialog_settings_get(PluginTileMapVals * plugin_config_vals);

    void tilemap_dialog_imageid_set(gint32 new_image_id);
#endif
