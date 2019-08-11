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

    gboolean tilemap_dialog_show(GimpDrawable *drawable);
    void     tilemap_dialog_processing_run (GimpDrawable *drawable, GimpPreview  *preview);

    void tilemap_dialog_settings_set(PluginTileMapVals plugin_config_vals);

#endif
