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


    #define BYTE_SIZE_RGBA_4BPP 4
    #define BYTE_SIZE_RGB_3BPP  3

    #define SCALE_FACTOR_MIN     1
    #define SCALE_FACTOR_DEFAULT SCALE_FACTOR_MIN
    #define SCALE_FACTOR_MAX     10


    typedef struct {
        gint       x,y;
        gint       width, height;
        gint       scale_factor;
        gint       bpp;
        glong      size_bytes; // scaledbuf_size;
        gboolean   valid_image;

        uint32_t * p_scaledbuf;
    } scaled_output_info;

    gint scale_factor_get();
    void scale_factor_set(gint);

    void scale_init(void);
    void scale_release_resources(void);
    void scale_apply(uint32_t *, uint32_t *, int, int);

    scaled_output_info * scaled_info_get(void);
    gint scaled_output_check_reapply_scale();
    void scaled_output_check_reallocate(gint, gint);

    void scaled_output_init(void);

#endif
