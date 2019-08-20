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

#include "filter_tilemap_helper.h"
#include "filter_dialog.h"
#include "scale.h"
#include "lib_tilemap.h"
#include "tilemap_overlay.h"


extern const char PLUG_IN_PROCEDURE[];
extern const char PLUG_IN_ROLE[];
extern const char PLUG_IN_BINARY[];

static void dialog_scaled_preview_check_resize(GtkWidget *, gint, gint, gint);
static void resize_image_and_apply_changes(GimpDrawable *, guchar *, guint);
// static void on_setting_scaler_combo_changed (GtkComboBox *, gpointer);

static void on_scaled_preview_mouse_exited(GtkWidget * window, gpointer callback_data);
static void on_scaled_preview_mouse_moved(GtkWidget * window, gpointer callback_data);
static void on_setting_scale_spinbutton_changed(GtkSpinButton *, gpointer);
static void on_setting_tilesize_spinbutton_changed(GtkSpinButton *, gint);
static void on_setting_overlay_checkbutton_changed(GtkToggleButton *, gpointer);
static void on_setting_finalbpp_combo_changed(GtkComboBox *, gpointer);

static void dialog_settings_apply_to_ui();
static void dialog_settings_connect_signals(GimpDrawable *);

static void info_display_update();

gboolean preview_scaled_update(GtkWidget *, GdkEvent *, GtkWidget *);

static void tilemap_calculate(uint8_t *, gint, gint, gint);
static void tilemap_invalidate();

static void tilemap_render_overlay();
static void tilemap_preview_display_tilenum_on_mouseover(gint x, gint y, GtkAllocation widget_alloc);

// Widget for displaying the upscaled image preview
static GtkWidget * preview_scaled;
static GtkWidget * tile_info_display;
static GtkWidget * memory_info_display;
static GtkWidget * mouse_hover_display;


// TODO: consider passing parent vbox into widget creation so these can be moved to local vars (?)
static GtkWidget * setting_scale_label;
static GtkWidget * setting_scale_spinbutton;

static GtkWidget * setting_overlay_grid_checkbutton;
static GtkWidget * setting_overlay_tileids_checkbutton;

static GtkWidget * setting_finalbpp_combo;

static GtkWidget * setting_tilesize_label;
static GtkWidget * setting_tilesize_width_spinbutton;
static GtkWidget * setting_tilesize_height_spinbutton;

static GtkWidget * setting_checkmirror_checkbutton;
static GtkWidget * setting_checkrotation_checkbutton;


static PluginTileMapVals dialog_settings;


// TODO: move these out of global scope?
static image_data      app_image;
static color_data      app_colors;

static gint            tilemap_needs_recalc;

static gint32          image_id;

/*
Preview
x Zoom [1]^
x [x] Overlay

Processing:
[x] Deduplicate Tiles
    [ ] Check Rotation
    [ ] Check Mirroring

Tiles:
Width[ 8 ]
Height[ 8 ]
Size from Image Grid [ ]

Offset:
X[ 0 ]
Y[ 0 ]

Info:
Tile W x H
Image W x H
Tile Map W x H
Total Colors N / Max colors per tile N
Color Mode: Indexed / Etc
*/


/*******************************************************/
/*               Main Plug-in Dialog                   */
/*******************************************************/
gboolean tilemap_dialog_show (GimpDrawable *drawable)
{
    GtkWidget * dialog;
    GtkWidget * main_vbox;

    GtkWidget * scaled_preview_window;

    GtkWidget * setting_table;
    GtkWidget * setting_preview_label;
    GtkWidget * setting_processing_label;

    GtkWidget * setting_tilesize_hbox;

    GtkWidget * setting_finalbpp_label;
    GtkWidget * setting_finalbpp_hbox;

    GtkWidget * mouse_hover_frame;


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
                               650,
                               600);


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

    // Create n x n table for Settings, non-homogonous sizing, attach to main vbox
    // TODO: Consider changing from a table to a grid (tables are deprecated)
    setting_table = gtk_table_new (5, 6, FALSE);
    gtk_box_pack_start (GTK_BOX (main_vbox), setting_table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(setting_table), 2);
    gtk_table_set_col_spacings(GTK_TABLE(setting_table), 20);
    //gtk_table_set_homogeneous(GTK_TABLE (setting_table), TRUE);
    gtk_table_set_homogeneous(GTK_TABLE (setting_table), FALSE);


    // == Preview Settings ==
    setting_preview_label = gtk_label_new (NULL);
    gtk_label_set_markup(GTK_LABEL(setting_preview_label), "<b>Preview</b>");
    gtk_misc_set_alignment(GTK_MISC(setting_preview_label), 0.0f, 0.5f); // Left-align

        // Checkboxes for the image overlays
        setting_overlay_grid_checkbutton = gtk_check_button_new_with_label("Grid");
        // gtk_misc_set_alignment(GTK_MISC(setting_overlay_grid_checkbutton), 1.0f, 0.5f); // Right-align

        setting_overlay_tileids_checkbutton = gtk_check_button_new_with_label("Tile IDs");
        // gtk_misc_set_alignment(GTK_MISC(setting_overlay_tileids_checkbutton), 1.0f, 0.5f); // Right-align


        // Spin button for the zoom scale factor
        setting_scale_label = gtk_label_new ("Zoom:" );
        gtk_misc_set_alignment(GTK_MISC(setting_scale_label), 0.0f, 0.5f); // Left-align
        setting_scale_spinbutton = gtk_spin_button_new_with_range(1,10,1); // Min/Max/Step


    // == Processing Settings ==
    setting_processing_label = gtk_label_new (NULL);
    gtk_label_set_markup(GTK_LABEL(setting_processing_label), "<b>Processing</b>");
    gtk_misc_set_alignment(GTK_MISC(setting_processing_label), 0.0f, 0.5f); // Left-align

        // Create label and right-align it
        setting_tilesize_label = gtk_label_new ("Tile size (w x h):  " );
        gtk_misc_set_alignment(GTK_MISC(setting_tilesize_label), 0.0f, 0.5f); // Left-align
        // Tile width/height widgets
        setting_tilesize_width_spinbutton  = gtk_spin_button_new_with_range(2,256,1); // Min/Max/Step
        setting_tilesize_height_spinbutton = gtk_spin_button_new_with_range(2,256,1); // Min/Max/Step

        // Horizontal box for tile width/height widgets
        // (Forces them to a nicer looking, smaller size)
        setting_tilesize_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);
        gtk_container_set_border_width (GTK_CONTAINER (setting_tilesize_hbox), 3);
        gtk_box_pack_start (GTK_BOX (setting_tilesize_hbox), setting_tilesize_width_spinbutton, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (setting_tilesize_hbox), setting_tilesize_height_spinbutton, FALSE, FALSE, 0);


        // Checkboxes for mirroring and rotation on tile deduplication
        setting_checkmirror_checkbutton = gtk_check_button_new_with_label("Check Mirroring");
        setting_checkrotation_checkbutton = gtk_check_button_new_with_label("Check Rotation");

    // Info readout/display area
    tile_info_display = gtk_label_new (NULL);
    gtk_label_set_markup(GTK_LABEL(tile_info_display),
                         g_markup_printf_escaped("<b>Tiles:</b>"));
    gtk_label_set_max_width_chars(GTK_LABEL(tile_info_display), 29);
    gtk_label_set_ellipsize(GTK_LABEL(tile_info_display),PANGO_ELLIPSIZE_END);
    gtk_misc_set_alignment(GTK_MISC(tile_info_display), 0.0f, 0.0f);

    memory_info_display = gtk_label_new (NULL);
    gtk_label_set_markup(GTK_LABEL(memory_info_display),
                         g_markup_printf_escaped("<b>Memory:</b>"));
    gtk_label_set_max_width_chars(GTK_LABEL(memory_info_display), 29);
    gtk_label_set_ellipsize(GTK_LABEL(memory_info_display),PANGO_ELLIPSIZE_END);
    gtk_misc_set_alignment(GTK_MISC(memory_info_display), 0.0f, 0.0f);

        // Combo box to customize the final bits-per-pixel of the tile data
        setting_finalbpp_label = gtk_label_new("Bits-per-pixel:");
        gtk_misc_set_alignment(GTK_MISC(setting_finalbpp_label), 0.0f, 0.5f); // Left-align

        setting_finalbpp_combo = gtk_combo_box_text_new ();
        // Populate combo box values from const array
        for (idx = 0; idx < ARRAY_LEN(finalbpp_strs); idx++)
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(setting_finalbpp_combo), finalbpp_strs[idx]);
        gtk_combo_box_set_active(GTK_COMBO_BOX(setting_finalbpp_combo), 0);

        // Horizontal box for tile final bits-per-pixel selector
        // (Forces them to a nicer looking, smaller size)
        setting_finalbpp_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_container_set_border_width (GTK_CONTAINER (setting_finalbpp_hbox), 3);
        gtk_box_pack_start (GTK_BOX (setting_finalbpp_hbox), setting_finalbpp_label, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (setting_finalbpp_hbox), setting_finalbpp_combo, FALSE, FALSE, 0);




    // Info readout/display area for mouse hover on the scaled preview area
    mouse_hover_display = gtk_label_new (NULL);
    gtk_misc_set_alignment(GTK_MISC(mouse_hover_display), 0.0f, 0.5f); // Left-align
    // Put the label inside a frame
    mouse_hover_frame = gtk_frame_new(NULL);
    gtk_container_add (GTK_CONTAINER (mouse_hover_frame), mouse_hover_display);


    // Attach the UI WIdgets to the table and show them all
    // gtk_table_attach_defaults (*attach_to, *widget, left_attach, right_attach, top_attach, bottom_attach)
    //
    gtk_table_attach_defaults (GTK_TABLE (setting_table), setting_preview_label,            0, 2, 0, 1);
        gtk_table_attach_defaults (GTK_TABLE (setting_table), setting_scale_label,          0, 2, 1, 2);
        gtk_table_attach_defaults (GTK_TABLE (setting_table), setting_scale_spinbutton,     0, 1, 2, 3);
        gtk_table_attach_defaults (GTK_TABLE (setting_table), setting_overlay_grid_checkbutton,     0, 2, 3, 4);
        gtk_table_attach_defaults (GTK_TABLE (setting_table), setting_overlay_tileids_checkbutton,  0, 2, 4, 5);

    gtk_table_attach_defaults (GTK_TABLE (setting_table), setting_processing_label,                2, 3, 0, 1);
        gtk_table_attach_defaults (GTK_TABLE (setting_table), setting_tilesize_label,              2, 3, 1, 2);
        gtk_table_attach_defaults (GTK_TABLE (setting_table), setting_tilesize_hbox,               2, 3, 2, 3);
        gtk_table_attach_defaults (GTK_TABLE (setting_table), setting_checkmirror_checkbutton,     2, 3, 3, 4);
        gtk_table_attach_defaults (GTK_TABLE (setting_table), setting_checkrotation_checkbutton,   2, 3, 4, 5);

    gtk_table_attach_defaults (GTK_TABLE (setting_table), tile_info_display,        3, 4, 0, 5);  // Vertical Column
    gtk_table_attach_defaults (GTK_TABLE (setting_table), memory_info_display,      4, 5, 0, 6);  // Vertical Column
    gtk_table_attach_defaults (GTK_TABLE (setting_table), setting_finalbpp_hbox,    4, 5, 5, 6);  // Bottom right

    // Attach mouse hover info area to bottom of main vbox (below table)
    gtk_box_pack_start (GTK_BOX (main_vbox), mouse_hover_frame, FALSE, FALSE, 0);



    gtk_widget_show (setting_table);

    gtk_widget_show (setting_preview_label);
        gtk_widget_show (setting_overlay_grid_checkbutton);
        gtk_widget_show (setting_overlay_tileids_checkbutton);
        gtk_widget_show (setting_scale_label);
        gtk_widget_show (setting_scale_spinbutton);

    gtk_widget_show (setting_processing_label);
        gtk_widget_show (setting_tilesize_label);
        gtk_widget_show (setting_tilesize_hbox);
        gtk_widget_show (setting_tilesize_width_spinbutton);
        gtk_widget_show (setting_tilesize_height_spinbutton);
        gtk_widget_show (setting_checkmirror_checkbutton);
        gtk_widget_show (setting_checkrotation_checkbutton);

    gtk_widget_show (tile_info_display);

    gtk_widget_show (memory_info_display);
        gtk_widget_show (setting_finalbpp_hbox);
        gtk_widget_show (setting_finalbpp_label);
        gtk_widget_show (setting_finalbpp_combo);

    gtk_widget_show (mouse_hover_display);
    gtk_widget_show (mouse_hover_frame);

    dialog_settings_apply_to_ui();

    dialog_settings_connect_signals(drawable);


    // ======== SHOW THE DIALOG AND RUN IT ========

    tilemap_invalidate();

    gtk_widget_show (dialog);

    run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

    gtk_widget_destroy (dialog);

    return run;
}



// For calling plugin to set dialog settings, including in headless mode
//
void tilemap_dialog_settings_set(PluginTileMapVals * p_plugin_config_vals) {

    // Copy plugin settings to dialog settings
    memcpy (&dialog_settings, p_plugin_config_vals, sizeof(PluginTileMapVals));
}



// For calling plugin to retrieve dialog settings (to persist for next-run)
//
void tilemap_dialog_settings_get(PluginTileMapVals * p_plugin_config_vals) {

    // Copy dialog settings to plugin settings
    memcpy (p_plugin_config_vals, &dialog_settings, sizeof(PluginTileMapVals));
}



// For calling plugin to set dialog settings, including in headless mode
//
void tilemap_dialog_imageid_set(gint32 new_image_id) {

    // Set local copy of source image id
    image_id = new_image_id;
}






// Load dialog settings into UI (called on startup)
//
void dialog_settings_apply_to_ui() {

    gint idx;
    //gchar * compare_str[255];
    gchar incoming_val_str[255];


    //printf("==== Applying Dialog Settings to UI\n");
    // ======== UPDATE WIDGETS TO CURRENT SETTINGS ========

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(setting_scale_spinbutton),           dialog_settings.scale_factor);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(setting_tilesize_width_spinbutton),  dialog_settings.tile_width);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(setting_tilesize_height_spinbutton), dialog_settings.tile_height);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(setting_overlay_grid_checkbutton),    dialog_settings.overlay_grid_enabled);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(setting_overlay_tileids_checkbutton), dialog_settings.overlay_tileids_enabled);

    gtk_combo_box_set_active(GTK_COMBO_BOX(setting_finalbpp_combo), 0);

    // Loop through combo options to see if any match the string for the finalbpp value,
    // if there is a match then update combo box selection
    snprintf(incoming_val_str, 255, "%d", dialog_settings.finalbpp);

    for (idx = 0; idx < ARRAY_LEN(finalbpp_strs); idx++)
            if (!(g_strcmp0( incoming_val_str, finalbpp_strs[idx] )))
            gtk_combo_box_set_active(GTK_COMBO_BOX(setting_finalbpp_combo), idx);
}



void dialog_settings_connect_signals(GimpDrawable *drawable) {

    // ======== SCALED PREVIEW MOUSEOVER INFO ========

    // Add mouse movement event
    gtk_widget_add_events(preview_scaled, GDK_POINTER_MOTION_MASK);

    // Connect the mouse moved signal to a display function
    g_signal_connect (preview_scaled, "motion-notify-event",
                      G_CALLBACK (on_scaled_preview_mouse_moved), NULL);


    // Add event for when the mouse leaves the window (clear info display)
    gtk_widget_add_events(preview_scaled, GDK_LEAVE_NOTIFY_MASK);

    // Connect the mouse moved signal to a display function
    g_signal_connect (preview_scaled, "leave-notify-event",
                      G_CALLBACK (on_scaled_preview_mouse_exited), NULL);

    // ======== HANDLE UI CONTROL VALUE UPDATES ========

    // Connect the changed signal to update the scaler mode
    g_signal_connect (setting_scale_spinbutton, "value-changed",
                      G_CALLBACK (on_setting_scale_spinbutton_changed), NULL);

    // Connect the changed signal to update the scaler mode
    g_signal_connect (setting_tilesize_width_spinbutton, "value-changed",
                      G_CALLBACK (on_setting_tilesize_spinbutton_changed), GINT_TO_POINTER(WIDGET_TILESIZE_WIDTH));
    g_signal_connect (setting_tilesize_height_spinbutton, "value-changed",
                      G_CALLBACK (on_setting_tilesize_spinbutton_changed), GINT_TO_POINTER(WIDGET_TILESIZE_HEIGHT));

    // Overlay control changes (will require a re-render)
    g_signal_connect(G_OBJECT(setting_overlay_grid_checkbutton), "toggled",
                      G_CALLBACK(on_setting_overlay_checkbutton_changed), NULL);
    g_signal_connect(G_OBJECT(setting_overlay_tileids_checkbutton), "toggled",
                      G_CALLBACK(on_setting_overlay_checkbutton_changed), NULL);

    // Final bits-per-pixel change for Memory info readout (just needs a recalc)
    g_signal_connect (setting_finalbpp_combo, "changed",
                      G_CALLBACK (on_setting_finalbpp_combo_changed), NULL);

    // ======== HANDLE PROCESSING UPDATES VIA UI CONTROL VALUE CHANGES ========

    // Connect a second signal to trigger a preview update
    // TODO: just run display processing
    g_signal_connect_swapped (setting_scale_spinbutton, "value-changed",
                              G_CALLBACK(tilemap_dialog_processing_run), drawable);

    // TODO: wire this to a separate processing function to run both tile and display processing
    g_signal_connect_swapped (setting_tilesize_width_spinbutton, "value-changed",
                              G_CALLBACK(tilemap_dialog_processing_run), drawable);
    g_signal_connect_swapped (setting_tilesize_height_spinbutton, "value-changed",
                              G_CALLBACK(tilemap_dialog_processing_run), drawable);

    g_signal_connect_swapped (setting_overlay_grid_checkbutton, "toggled",
                              G_CALLBACK(tilemap_dialog_processing_run), drawable);
    g_signal_connect_swapped (setting_overlay_tileids_checkbutton, "toggled",
                              G_CALLBACK(tilemap_dialog_processing_run), drawable);
}





static void on_scaled_preview_mouse_exited(GtkWidget * widget, gpointer callback_data) {
    // Mouse Tile Display: Outside preview area, clear info
    gtk_label_set_markup(GTK_LABEL(mouse_hover_display),
                     g_markup_printf_escaped(" " ) );
}



static void on_scaled_preview_mouse_moved(GtkWidget * widget, gpointer callback_data) {

    GtkAllocation allocation;
    gint x,y;

    // Note: gtk_widget_get_pointer() is deprecated, eventually use...
    //   -> gdk_window_get_device_position (window, mouse, &x, &y, NULL);
    gtk_widget_get_pointer(widget, &x, &y);
    gtk_widget_get_allocation (widget, &allocation);

    // Display info about the tile the mouse is over
    tilemap_preview_display_tilenum_on_mouseover(x,y, allocation);
}



// Handler for "changed" signal of SCALER MODE combo box
// When the user changes the scaler type -> Update the scaler mode
//
//   callback_data not used currently
//
static void on_setting_scale_spinbutton_changed(GtkSpinButton * spinbutton, gpointer callback_data)
{
    //printf("        --> EVENT: on_setting_tilesize_spinbutton_changed\n");

    dialog_settings.scale_factor = gtk_spin_button_get_value_as_int(spinbutton);
}



// TODO: ?consolidate to a single spin button UI update handler?
static void on_setting_tilesize_spinbutton_changed(GtkSpinButton * spinbutton, gint callback_data)
{
    //printf("        --> EVENT: on_setting_tilesize_spinbutton_changed\n");

    switch (callback_data) {
        case WIDGET_TILESIZE_WIDTH:  dialog_settings.tile_width  = gtk_spin_button_get_value_as_int(spinbutton);
            tilemap_invalidate();
            break;
        case WIDGET_TILESIZE_HEIGHT: dialog_settings.tile_height = gtk_spin_button_get_value_as_int(spinbutton);
            tilemap_invalidate();
            break;
    }
}


static void on_setting_overlay_checkbutton_changed(GtkToggleButton * p_togglebutton, gpointer callback_data) {
    // Update settings for both checkboxes
    dialog_settings.overlay_grid_enabled    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(setting_overlay_grid_checkbutton));
    dialog_settings.overlay_tileids_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(setting_overlay_tileids_checkbutton));

//    printf("        --> EVENT: Setting Enables: %d, %d\n", dialog_settings.overlay_grid_enabled,
//                                dialog_settings.overlay_tileids_enabled);

    // Request a redraw of the scaled preview + overlay
    scaled_output_invalidate();
}


static void on_setting_finalbpp_combo_changed(GtkComboBox *combo, gpointer callback_data)
{
    dialog_settings.finalbpp = atoi(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo)));

    info_display_update();
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
//   -> spin buttons -> value-changed
// * The end of this file.c (if user pressed "OK" to apply)
//
void tilemap_dialog_processing_run(GimpDrawable *drawable, GimpPreview  *preview)
{
    GimpImageType drawable_type;
    GimpPixelRgn src_rgn;
    gint         src_bpp, dest_bpp;
    gint         width, height;
    gint         x, y;

    uint8_t    * p_srcbuf = NULL;
    glong        srcbuf_size = 0;
    scaled_output_info * scaled_output;

    guchar     * p_colormap_buf = NULL;
    gint         colormap_numcolors = 0;


printf("tilemap_dialog_processing_run 1 --> tilemap_needs_recalc = %d\n", tilemap_needs_recalc);

    // Apply dialog settings
    tilemap_overlay_set_enables(dialog_settings.overlay_grid_enabled,
                                dialog_settings.overlay_tileids_enabled);
    scale_factor_set( dialog_settings.scale_factor );
    printf("Redraw queued at %dx\n", dialog_settings.scale_factor);

    // Check for previously rendered output
    scaled_output = scaled_info_get();

    // TODO: Always use the entire image?

    // Get the working image area for either the preview sub-window or the entire image
    if (preview) {
        // gimp_preview_get_position (preview, &x, &y);
        // gimp_preview_get_size (preview, &width, &height);
        if (! gimp_drawable_mask_intersect (drawable->drawable_id,
                                                 &x, &y, &width, &height)) {
            return;
        }

        dialog_scaled_preview_check_resize( preview_scaled, width, height, dialog_settings.scale_factor);
    } else if (! gimp_drawable_mask_intersect (drawable->drawable_id,
                                             &x, &y, &width, &height)) {
        // TODO: DO STUFF WHEN CALLED AFTER DIALOG CLOSED
        return;
    }


    // Get bit depth and alpha mask status
    src_bpp = drawable->bpp;

    // If image is INDEXED or INDEXED ALPHA
    // Then promote dest image: 1 bpp -> RGB 3 bpp, 2 bpp (alpha) -> RGBA 4bpp
    if (src_bpp <= 2)
        dest_bpp = (3 + (src_bpp - 1));
    else
        dest_bpp = src_bpp;


    // Allocate output buffer for upscaled image
    // NOTE: This is feeding in the dest/scaled RGB/ALPHA 3/4 BPP that was promoted from INDEXED/ALPHA 1/2 BPP
    scaled_output_check_reallocate(dest_bpp, width, height);


    // TODO: switch this to an invalidate model like tilemap_needs_recalc? - then above could be merged in and nested
    if ((scaled_output_check_reapply_scale()) || (tilemap_needs_recalc)) {

        // ====== GET THE SOURCE IMAGE ======
        // Allocate a working buffer to copy the source image into
        srcbuf_size = width * height * src_bpp;
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


        // TODO: handle grayscale?
        // gimp_drawable_is_gray()
        // gimp_drawable_is_rgb()
        // Load color map if needed
        if (gimp_drawable_is_indexed(drawable->drawable_id)) {
            // Load the color map and copy it to a working buffer
            p_colormap_buf = gimp_image_get_colormap(image_id, &colormap_numcolors);
        }
        else
            colormap_numcolors = 0;


        // ====== CALCULATE TILE MAP & TILES ======

        printf("tilemap_dialog_processing_run 2 --> tilemap_needs_recalc = %d\n", tilemap_needs_recalc);

        if (tilemap_needs_recalc) {
            tilemap_calculate(p_srcbuf,
                              src_bpp,
                              width, height);
        }


        // ====== APPLY THE SCALER ======
        // NOTE: Promotes INDEXED/ALPHA 1/2 BPP to RGB/ALPHA 3/4 BPP
        //       Expects p_destbuf to be allocated with 3/4 BPP RGB/A number of bytes, not 1/2 if INDEXED
        //
        // TODO: FIXME For now, every time the tile size is changed, re-scale the image
//        if (scaled_output_check_reapply_scale()) {
        {
            scale_apply(p_srcbuf,
                        scaled_output->p_scaledbuf,
                        src_bpp,
                        width, height,
                        p_colormap_buf, colormap_numcolors);

            // Note: Drawing of individual overlays controlled via tilemap_overlay_set_enables()
            // For now, every time we change the tile size, we have to re-scale the image
            tilemap_overlay_setparams(scaled_output->p_scaledbuf,
                                      dest_bpp,
                                      scaled_output->width,
                                      scaled_output->height,
                                      dialog_settings.tile_width * dialog_settings.scale_factor,
                                      dialog_settings.tile_height * dialog_settings.scale_factor);

            tilemap_render_overlay();
        }
    }
//    else

    // Filter is done, apply the update
    if (preview) {


        // Redraw the scaled preview if it's available (it ought to be at this point)
        if ( (scaled_output->p_scaledbuf != NULL) &&
             (scaled_output->valid_image == TRUE) ) {


            // Select drawable render type based on BPP of upscaled image
            if      (dest_bpp == BPP_RGB)  drawable_type = GIMP_RGB_IMAGE;
            else if (dest_bpp == BPP_RGBA) drawable_type = GIMP_RGBA_IMAGE;
            else     drawable_type = GIMP_RGB_IMAGE; // fallback to RGB // TODO: should handle this better...

            // TODO: optional use gimp_preview_area_blend() to mix overlay and source image?
            // TODO: optional: // gimp_preview_area_set_colormap ()
            gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview_scaled),
                                    0, 0,                  // x,y
                                    scaled_output->width,  // width, height
                                    scaled_output->height,
                                    drawable_type,                              // GimpImageType (scaled image) // gimp_drawable_type (drawable->drawable_id),
                                    (guchar *) scaled_output->p_scaledbuf,      // Source buffer
                                    scaled_output->width * scaled_output->bpp); // Row-stride
        }

        // Update the info display area
        // update_text_readout();

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




static void tilemap_invalidate() {
    tilemap_needs_recalc = TRUE;
}




// TODO: variable tile size (push down via app settings?)
//  gint image_id, gint drawable_id, gint image_mode)
void tilemap_calculate(uint8_t * p_srcbuf, gint bpp, gint width, gint height) {

    gint status;

    tile_map_data * p_map;
    tile_set_data * p_tile_set;
    image_data      tile_set_deduped_image;


    status = TRUE; // Default to success

    /*
// TODO: FIXME Update getting triggered incorrectly by changes in scale
    // TODO: move into function
    // TODO: or tile size, mirror or rotate changed
    // Did any map related settings change? Then queue an update
    if ((app_image.bytes_per_pixel != bpp)             ||
        (app_image.width      != width)                ||
        (app_image.height     != height)               ||
        (app_image.size       != width * height * bpp) ||
        (p_map->tile_width    != dialog_settings.tile_width) ||
        (p_map->tile_height   != dialog_settings.tile_height))
    {
        tilemap_needs_recalc = TRUE;
        printf("Tilemap Recalc check = True\n");
    }
*/
    // Get the Bytes Per Pixel of the incoming app image
    app_image.bytes_per_pixel = bpp;

    // Determine the array size for the app's image then allocate it
    app_image.width      = width;
    app_image.height     = height;
    app_image.size       = width * height * bpp;
    app_image.p_img_data = p_srcbuf;


    // TODO: FIXME: invalidate model is failing to work if tilemap fails due to excessive tile count/etc -> it's causing multiple pointless recalculations in a row, bad for large images
    if (tilemap_needs_recalc) {
        printf("Tilemap: Starting Recalc: tilemap_needs_recalc = %d\n\n", tilemap_needs_recalc);
        status = tilemap_export_process(&app_image,
                                        dialog_settings.tile_width,
                                        dialog_settings.tile_height);

        // TODO: warn/notify on failure (invalid tile size, etc)

        if (status) {

            printf("Tilemap Recalc SUCCESS...\n");

            // Retrieve the deduplicated map and tile set
            p_map      = tilemap_get_map();
            p_tile_set = tilemap_get_tile_set();
            // status     = tilemap_get_image_of_deduped_tile_set(&tile_set_deduped_image);

            // Set tile map parameters, then convert the image to a map
            /*
            p_map->width_in_tiles;
            p_map->height_in_tiles;
            p_tile_set->tile_count;

            p_map->tile_id_list -> uint8_t * p_map_data
            p_map->size

            */
            tilemap_needs_recalc = FALSE;

            // TODO: consider moving this to call from somewhere else, maybe end of "Run" function (BUT ON DIALOG DISPLAY MODE ONLY!, NOT HEADLESS/STANDALONE)
            info_display_update();

            printf("tilemap:done --> tilemap_needs_recalc = %d\n\n", tilemap_needs_recalc);
        }
        else
            printf("Tilemap: Recalc FAILED: tilemap_needs_recalc = %d\n\n", tilemap_needs_recalc);
    }
    else
         printf("Tilemap: NO Recalc: tilemap_needs_recalc = %d\n\n", tilemap_needs_recalc);
}




static void info_display_update() {

    tile_map_data * p_map;
    tile_set_data * p_tile_set;

    gint final_bitsperpixel;
    gint tilemap_storage_size;
    const gchar * bpp_str;

    bpp_str = srcbpp_str[0]; // Default blank string

    // TODO: FIXME: implement better handling for valid map data (tilemap_is_valid()?)
    // TODO: maybe display "no valid data" when no valid tile map calculated (or maybe not, since it's less startling when comparing tile sizes)

    if (tilemap_needs_recalc == FALSE) {

        p_map      = tilemap_get_map();
        p_tile_set = tilemap_get_tile_set();

        // Use bpp from source image when finalbpp combo is "0", i.e "Src Image"
        // Otherwise use combo value directly
        if (dialog_settings.finalbpp == 0) {
            // Convert source image bytes into bits-per-pixel
            final_bitsperpixel = p_tile_set->tile_bytes_per_pixel * 8;

            // Load image mode string if it's coming from the source image
            if ((p_tile_set->tile_bytes_per_pixel >=1) &&
                (p_tile_set->tile_bytes_per_pixel < ARRAY_LEN(srcbpp_str)))
            bpp_str = srcbpp_str[p_tile_set->tile_bytes_per_pixel];
        }
        else
            final_bitsperpixel = dialog_settings.finalbpp;


        // Use u8 for tilemap array when possible, otherwise u16
        if ((p_tile_set->tile_count > 255) || (p_map->map_width > 255) || (p_map->map_height > 255))
            tilemap_storage_size = sizeof(uint16_t);
        else
            tilemap_storage_size = sizeof(uint8_t);


        gtk_label_set_markup(GTK_LABEL(tile_info_display),
             g_markup_printf_escaped("<b>Tile Info:</b>\n"
                                     "Size: %d x %d\n"
                                     "Tiled Map: %d x %d\n"
                                     "Image: %d x %d\n"
                                     "Map # Tiles: %d\n"
                                     "Unique # Tiles: %d\n",
                                     p_map->tile_width,     p_map->tile_height,
                                     p_map->width_in_tiles, p_map->height_in_tiles,
                                     p_map->map_width,      p_map->map_height,
                                     (p_map->width_in_tiles * p_map->height_in_tiles),
                                     p_tile_set->tile_count));

        gtk_label_set_markup(GTK_LABEL(memory_info_display),
             g_markup_printf_escaped("<b>Memory Info</b>\n"
                                     //"Color Mode: %d byte/pixel\n"
                                    "Bits-per-pixel: %d (%s)\n"
                                    "Tile Bytes: %d\n"
                                    "Tile Set Bytes: %d\n"
                                    "Tile Map Var Size: %d byte\n"
                                    "Tile Map Bytes: %d\n"
                                    "Total Bytes: %d\n",
                                    final_bitsperpixel, bpp_str,
                                    // Tile Bytes
                                    ((p_map->tile_width * p_map->tile_height) * final_bitsperpixel) / 8,  // / 8 bits per byte
                                    // Tile Set Bytes
                                    ((p_map->tile_width * p_map->tile_height) * final_bitsperpixel * p_tile_set->tile_count) / 8,  // / 8 bits per byte
                                    // Tile Map Var Size & Tile Map Bytes
                                    tilemap_storage_size,
                                    (p_map->width_in_tiles * p_map->height_in_tiles) * tilemap_storage_size,
                                    // Total Bytes
                                    (((p_map->tile_width * p_map->tile_height) * final_bitsperpixel * p_tile_set->tile_count) / 8)  // / 8 bits per byte
                                     + ((p_map->width_in_tiles * p_map->height_in_tiles) * tilemap_storage_size)));
    }
}



static void tilemap_render_overlay() {

    tile_map_data * p_map;
    tile_set_data * p_tile_set;

    p_map      = tilemap_get_map();
    p_tile_set = tilemap_get_tile_set();

    if (p_tile_set->tile_count > 0)
        tilemap_overlay_apply(p_map->size, p_map->tile_id_list);
    else
        printf("Overlay: Render tilenums -> NO TILES FOUND!\n");
}



static void tilemap_preview_display_tilenum_on_mouseover(gint x, gint y, GtkAllocation widget_alloc) {

    //
    #define PREVIEW_WIDGET_BORDER_X 1
    #define PREVIEW_WIDGET_BORDER_Y 2


    gint tile_num;
    gint tile_x, tile_y, tile_idx;
    gint img_x, img_y;

    scaled_output_info * scaled_output;

    tile_map_data * p_map;
    tile_set_data * p_tile_set;

    // Only display if there's valid data available (no recalc queued)
    if (!((scaled_output_check_reapply_scale()) || (tilemap_needs_recalc))) {

            p_map      = tilemap_get_map();
            p_tile_set = tilemap_get_tile_set();

            scaled_output = scaled_info_get();

            // * Mouse location is in preview window coordinates
            // * Scaled preview image may be smaller and centered in preview window
            // So: position on image = mouse.x - (alloc.width - scaled_output->width) / 2,

            img_x = x - ((widget_alloc.width - scaled_output->width) / 2) - PREVIEW_WIDGET_BORDER_X;
            img_y = y - ((widget_alloc.height - scaled_output->height) / 2) - PREVIEW_WIDGET_BORDER_Y;

        // Only process if it's within the bounds of the actual preview area
        if ((img_x >= 0) && (img_x < scaled_output->width) &&
            (img_y >= 0) && (img_y < scaled_output->height)) {

            if (p_tile_set->tile_count > 0) {

                // Get position on tile map and relevant info for tile
                tile_x = (img_x / scaled_output->scale_factor) / p_map->tile_width;
                tile_y = (img_y / scaled_output->scale_factor) / p_map->tile_height;

                tile_idx = tile_x + (tile_y * p_map->width_in_tiles );

                tile_num = p_map->tile_id_list[tile_idx];

                gtk_label_set_markup(GTK_LABEL(mouse_hover_display),
                            g_markup_printf_escaped("  Image x,y: (%4d ,%-4d)"
                                                    "        Tile x,y: (%4d , %-4d)"
                                                    "        Tile index: %-8d"
                                                    "        Tile ID: %-8d"
                                                    , img_x / scaled_output->scale_factor
                                                    , img_y / scaled_output->scale_factor
                                                    , tile_x, tile_y
                                                    , tile_idx, tile_num
                                                    ) );
            }
            else gtk_label_set_markup(GTK_LABEL(mouse_hover_display),
                     g_markup_printf_escaped("  ( No tiles available - check tile sizing )" ) );
                // printf("Mouse Tile Display: NO TILES FOUND!\n");

        } else gtk_label_set_markup(GTK_LABEL(mouse_hover_display),
                     g_markup_printf_escaped(" " ) );
            // else printf("Mouse Tile Display: Outside preview image bounds\n");
    }
    else gtk_label_set_markup(GTK_LABEL(mouse_hover_display),
                     g_markup_printf_escaped("  ( No tiles available - check tile sizing )" ) );
        // printf("Mouse Tile Display: Not yet ready for display!\n");

}


// TODO
// gint tilemap_check_needs_recalcualte()


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
