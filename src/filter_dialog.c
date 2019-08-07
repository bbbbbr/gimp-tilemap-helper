//
// filter_dialog.c
//

// ========================
//
// Creates and shows plug-in dialog window,
// displays preview of tile map/set info
//
// ========================

//#include "config.h"
//#include <string.h>

#include <stdio.h>
#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "filter_dialog.h"
#include "scale.h"



extern const char PLUG_IN_PROCEDURE[];
extern const char PLUG_IN_ROLE[];
extern const char PLUG_IN_BINARY[];

static void dialog_scaled_preview_check_resize(GtkWidget *, gint, gint, gint);
static void resize_image_and_apply_changes(GimpDrawable *, guchar *, guint);
// static void on_settings_scaler_combo_changed (GtkComboBox *, gpointer);
static void on_settings_scale_spinbutton_changed(GtkSpinButton *, gpointer);
gboolean preview_scaled_update(GtkWidget *, GdkEvent *, GtkWidget *);


// Widget for displaying the upscaled image preview
static GtkWidget * preview_scaled;


/*******************************************************/
/*               Main Plug-in Dialog                   */
/*******************************************************/
gboolean tilemap_dialog_show (GimpDrawable *drawable)
{
    GtkWidget * dialog;
    GtkWidget * main_vbox;
    GtkWidget * preview_hbox;

    GtkWidget * scaled_preview_window;

    GtkWidget * settings_table;
    GtkWidget * settings_scale_spinbutton;
    GtkWidget * settings_scale_label;
    GtkWidget * settings_tilesize_label;

    gboolean   run;
    gint       idx;


    gimp_ui_init (PLUG_IN_BINARY, FALSE);

    dialog = gimp_dialog_new ("Tilemap Helper", PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROCEDURE,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

    // Resize to show more of scaled preview by default (this sets MIN size)
    gtk_widget_set_size_request (dialog,
                               500,
                               400);


    gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

    gimp_window_set_transient (GTK_WINDOW (dialog));


    // Create a main vertical box for the preview
    main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
    gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
    gtk_widget_show (main_vbox);



    // ======== IMAGE PREVIEW ========

    // Create scaled preview area and a scrolled window area to hold it
    // TODO: gimp_drawable_preview_new() is deprecated as of 2.10 -> gimp_drawable_preview_new_from_drawable_id()
    preview_scaled = gimp_preview_area_new();
    scaled_preview_window = gtk_scrolled_window_new (NULL, NULL);


    // Automatic scrollbars for scrolled preview window
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scaled_preview_window),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);


    // Add the scaled preview to the scrolled window
    // and then display them both (with auto-resize)
    gtk_scrolled_window_add_with_viewport((GtkScrolledWindow *)scaled_preview_window,
                                              preview_scaled);
    gtk_box_pack_start (GTK_BOX (main_vbox), scaled_preview_window, TRUE, TRUE, 0);
    gtk_widget_show (scaled_preview_window);
    gtk_widget_show (preview_scaled);


    // Wire up scaled preview redraw to the size-allocate event (Window changed size/etc)
    // resize scaled preview -> destroys scaled preview buffer -> resizes scroll window -> size-allocate -> redraw preview buffer
    // This fixes the scrolled window inhibiting the redraw when the size changed
    g_signal_connect_swapped(preview_scaled,
                             "size-allocate",
                             G_CALLBACK(tilemap_dialog_processing_run), drawable);


    // ======== UI CONTROLS ========

    // Create 2 x 3 table for Settings, non-homogonous sizing, attach to main vbox
    // TODO: Consider changing from a table to a grid (tables are deprecated)
    settings_table = gtk_table_new (2, 3, FALSE);
    gtk_box_pack_start (GTK_BOX (main_vbox), settings_table, FALSE, FALSE, 0);
    gtk_table_set_homogeneous(GTK_TABLE (settings_table), TRUE);

    // Create label and right-align it
    settings_tilesize_label = gtk_label_new ("Tile size (w x h):  " );
    gtk_misc_set_alignment(GTK_MISC(settings_tilesize_label), 1.0f, 0.5f);


    // Add a spin button for the zoom scale factor
    settings_scale_label = gtk_label_new ("Zoom:  " );
    gtk_misc_set_alignment(GTK_MISC(settings_scale_label), 1.0f, 0.5f); // Right-align
    settings_scale_spinbutton = gtk_spin_button_new_with_range(1,10,1); // Min/Max/Step


    // Attach the UI WIdgets to the table and show them all
    // gtk_table_attach_defaults (*attach_to, *widget, left_attach, right_attach, top_attach, bottom_attach)
    //
    gtk_table_attach_defaults (GTK_TABLE (settings_table), settings_tilesize_label,   1, 2, 0, 1); // Middle of table

    gtk_table_attach_defaults (GTK_TABLE (settings_table), settings_scale_label,      1, 2, 1, 2); // Middle of table
    gtk_table_attach_defaults (GTK_TABLE (settings_table), settings_scale_spinbutton, 2, 3, 1, 2); // Right side of table

    gtk_widget_show (settings_table);
    gtk_widget_show (settings_tilesize_label);

    gtk_widget_show (settings_scale_label);
    gtk_widget_show (settings_scale_spinbutton);



    // Connect the changed signal to update the scaler mode
    g_signal_connect (settings_scale_spinbutton,
                      "value-changed",
                      G_CALLBACK (on_settings_scale_spinbutton_changed),
                      NULL);

    // Then connect a second signal to trigger a preview update
    g_signal_connect_swapped (settings_scale_spinbutton,
                              "value-changed",
                              G_CALLBACK(tilemap_dialog_processing_run), drawable);



    // ======== SHOW THE DIALOG AND RUN IT ========

    gtk_widget_show (dialog);

    run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

    gtk_widget_destroy (dialog);

    return run;
}



// Handler for "changed" signal of SCALER MODE combo box
// When the user changes the scaler type -> Update the scaler mode
//
//   callback_data not used currently
//
static void on_settings_scale_spinbutton_changed(GtkSpinButton *spinbutton, gpointer callback_data)
{
    scale_factor_set( gtk_spin_button_get_value_as_int(spinbutton) );
}



// Checks to see whether the scaled preview area needs
// to be resized. Handles resizing if needed.
//
// Called from pixel_art_scalers_run() which is used for
// previewing and final rendering of the selected scaler mode
//
static void dialog_scaled_preview_check_resize(GtkWidget * preview_scaled, gint width_new, gint height_new, gint scale_factor_new)
{
    gint width_current, height_current;

    // Get current size for scaled preview area
    gtk_widget_get_size_request (preview_scaled, &width_current, &height_current);

    // Only resize if the width, height or scaling changed
    if ( (width_current  != (width_new  * scale_factor_new)) ||
         (height_current != (height_new * scale_factor_new)) )
    {
        // TODO: This queues a second redraw event... it seems to work fine. Does it need to be fixed?
        printf("Check size... Resize applied\n");

        // Resize scaled preview area
        gtk_widget_set_size_request (preview_scaled, width_new * scale_factor_new, height_new * scale_factor_new);

        // when set_size_request and then draw are called repeatedly on a preview_area
        // it causes redraw glitching in the surrounding scrolled_window region
        // Calling set_max_size appears to fix this
        // (though it may be treating the symptom and not the cause of the glitching)
        gimp_preview_area_set_max_size(GIMP_PREVIEW_AREA (preview_scaled),
                                       width_new * scale_factor_new,
                                       height_new * scale_factor_new);
    }
}



/*******************************************************/
/*   Create the dialog preview image or output layer   */
/*******************************************************/
//
//
// Previews and performs the final output rendering of
// the selected scaler.
//
// Called from:
// * Signals...
//   -> preview_scaled -> size_allocate
//   -> spin button -> value-changed
// * The end of filter_pixel_art_scalers.c (if user pressed "OK" to apply)
//
void tilemap_dialog_processing_run(GimpDrawable *drawable, GimpPreview  *preview)
{
    GimpPixelRgn src_rgn;
    gint         bpp;
    gint         width, height;
    gint         x, y;
    guint        scale_factor;
    uint8_t    * p_srcbuf = NULL;
    glong        srcbuf_size = 0;
    scaled_output_info * scaled_output;



    scaled_output = scaled_info_get();
    scale_factor = scale_factor_get();  // TODO: Set scale factor here? Used to pull scale factor from mode

    printf("Redraw queued at %dx\n", scale_factor);

    // TODO: Always use the entire image?

    // Get the working image area for either the preview sub-window or the entire image
    if (preview) {
        // gimp_preview_get_position (preview, &x, &y);
        // gimp_preview_get_size (preview, &width, &height);
        if (! gimp_drawable_mask_intersect (drawable->drawable_id,
                                                 &x, &y, &width, &height)) {
            return;
        }

        dialog_scaled_preview_check_resize( preview_scaled, width, height, scale_factor);
    } else if (! gimp_drawable_mask_intersect (drawable->drawable_id,
                                             &x, &y, &width, &height)) {
        // TODO: DO STUFF WHEN CALLED AFTER DIALOG CLOSED
        return;
    }


    // Get bit depth and alpha mask status
    bpp = drawable->bpp;

    // Allocate output buffer for upscaled image
    scaled_output_check_reallocate(bpp, width, height);

    if (scaled_output_check_reapply_scale()) {

        // ====== GET THE SOURCE IMAGE ======
        // Allocate a working buffer to copy the source image into
        srcbuf_size = width * height * bpp;
        p_srcbuf = (uint8_t *) g_new (guint8, srcbuf_size);


        // FALSE, FALSE : region will be used to read the actual drawable data
        // Initialize source pixel region with drawable
        gimp_pixel_rgn_init (&src_rgn,
                             drawable,
                             x, y,
                             width, height,
                             FALSE, FALSE);

        // Copy source image to working buffer
        gimp_pixel_rgn_get_rect (&src_rgn,
                                 (guchar *) p_srcbuf,
                                 x, y, width, height);


        // ====== APPLY THE SCALER ======

        // TODO: remove comment ---Expects 4BPP RGBA in p_srcbuf, outputs same to p_scaledbuf
        scale_apply(p_srcbuf,
                    scaled_output->p_scaledbuf,
                    bpp,
                    width, height);
    }
    else

    // Filter is done, apply the update
    if (preview) {


        // Redraw the scaled preview if it's available (it ought to be at this point)
        if ( (scaled_output->p_scaledbuf != NULL) &&
             (scaled_output->valid_image == TRUE) ) {

            // TODO: ? use gimp_preview_area_blend() to mix overlay and source image?
            // Calling widget should be: preview_scaled
            gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview_scaled),
                                    0, 0,                  // x,y
                                    scaled_output->width,  // width, height
                                    scaled_output->height,
                                    gimp_drawable_type (drawable->drawable_id),             // GimpImageType (source image)
                                    (guchar *) scaled_output->p_scaledbuf,      // Source buffer
                                    scaled_output->width * scaled_output->bpp); // Row-stride
        }
    }
    else
    {
/*
        // Apply image result with full resize
        resize_image_and_apply_changes(drawable,
                                       (guchar *) scaled_output->p_scaledbuf,
                                       scaled_output->scale_factor);
*/
    }


    // Free the working buffer
    if (p_srcbuf)
        g_free (p_srcbuf);
}




// resize_image_and_apply_changes
//
// Resizes image and then draws the newly scaled output onto it.
// This is only for FINAL, NON-PREVIEW rendered output
//
// Called from pixel_art_scalers_run()
//
// Params:
// * GimpDrawable          : from source image
// * guchar * buffer       : the previously rendered scaled output
// * guint    scale_factor : image scale multiplier
//
/*
void resize_image_and_apply_changes(GimpDrawable * drawable, guchar * p_scaledbuf, guint scale_factor)
{
    GimpPixelRgn  dest_rgn;
    gint          x,y, width, height;
    GimpDrawable  * resized_drawable;

    if (! gimp_drawable_mask_intersect (drawable->drawable_id,
                                         &x, &y, &width, &height))
        return;

    // == START UNDO GROUPING
    gimp_image_undo_group_start(gimp_item_get_image(drawable->drawable_id));

    // Resize source image
    if (gimp_image_resize(gimp_item_get_image(drawable->drawable_id),
                          width * scale_factor,
                          height * scale_factor,
                          0,0))
    {

        // Resize the current layer to match the resized image
        gimp_layer_resize_to_image_size( gimp_image_get_active_layer(
                                           gimp_item_get_image(drawable->drawable_id) ) );


        // Get a new drawable handle from the resized layer/image
        resized_drawable = gimp_drawable_get( gimp_image_get_active_drawable(
                                                gimp_item_get_image(drawable->drawable_id) ) );

        // Initialize destination pixel region with drawable
        // TRUE,  TRUE  : region will be used to write to the shadow tiles
        //                i.e. make changes that will be written back to source tiles
        gimp_pixel_rgn_init (&dest_rgn,
                             resized_drawable,
                             0, 0,
                             width * scale_factor,
                             height * scale_factor,
                             TRUE, TRUE);

        // Copy the previously rendered scaled output buffer
        // to the shadow image buffer in the drawable
        gimp_pixel_rgn_set_rect (&dest_rgn,
                                 (guchar *) p_scaledbuf,
                                 0,0,
                                 width * scale_factor,
                                 height * scale_factor);


        // Apply the changes to the image (merge shadow, update drawable)
        gimp_drawable_flush (resized_drawable);
        gimp_drawable_merge_shadow (resized_drawable->drawable_id, TRUE);
        gimp_drawable_update (resized_drawable->drawable_id, 0, 0, width * scale_factor, height * scale_factor);

        // Free the extra resized drawable
        gimp_drawable_detach (resized_drawable);
    }

    // == END GROUPING
    gimp_image_undo_group_end(gimp_item_get_image(drawable->drawable_id));
}
*/
