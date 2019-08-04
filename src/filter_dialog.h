//
// filter_dialog.h
//

#ifndef __FILTER_DIALOG_H_
#define __FILTER_DIALOG_H_

    #include <stdint.h>


    gboolean tilemap_dialog_show(GimpDrawable *drawable);
    void     tilemap_dialog_processing_run (GimpDrawable *drawable, GimpPreview  *preview);

#endif
