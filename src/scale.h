//
// scale.h
//

#ifndef SCALE_H_
#define SCALE_H_


    #include <stdio.h>
    #include <string.h>
    #include <stdint.h>

    #include <libgimp/gimp.h>
    #include <libgimp/gimpui.h>

    #define BPP_INDEXED   1
    #define BPP_INDEXEDA  2
    #define BPP_RGB       3
    #define BPP_RGBA      4

    #define BYTE_SIZE_RGBA_4BPP 4
    #define BYTE_SIZE_RGB_3BPP  3

    #define SCALE_FACTOR_MIN     1
    #define SCALE_FACTOR_DEFAULT SCALE_FACTOR_MIN
    #define SCALE_FACTOR_MAX     10

    #define SCALE_BPP_DEFAULT 0  // Don't assume bit depth
    #define SCALE_BPP_MIN     1
    #define SCALE_BPP_MAX     4

    typedef struct {
        gint       x,y;
        gint       width, height;
        gint       scale_factor;
        gint       bpp;
        glong      size_bytes; // scaledbuf_size;
        gboolean   valid_image;

        uint8_t * p_scaledbuf;
    } scaled_output_info;

    gint scale_factor_get();
    void scale_factor_set(gint);
//    gint scale_bpp_get();
//    void scale_bpp_set(gint);

    void scale_init(void);
    void scale_release_resources(void);
    void scale_apply(uint8_t *, uint8_t *, gint, gint, gint, uint8_t *, int );

    scaled_output_info * scaled_info_get(void);
    void scaled_output_invalidate();
    gint scaled_output_check_reapply_scale();
    void scaled_output_check_reallocate(gint, gint, gint);

    void scaled_output_init(void);

#endif
