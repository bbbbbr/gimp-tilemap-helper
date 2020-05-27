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
        int       x,y;
        int       width, height;
        int       scale_factor;
        int       bpp;
        uint32_t  size_bytes; // scaledbuf_size;
        int       valid_image;

        uint8_t * p_scaledbuf;
        uint8_t * p_overlaybuf;
    } scaled_output_info;

    gint scale_factor_get(void);
    void scale_factor_set(gint);

    void scale_init(void);
    void scale_release_resources(void);
    void scale_apply(uint8_t *, uint8_t *, gint, gint, gint, uint8_t *, gint, gint);

    void scale_output_get_rgb_at_xy(int, int, uint8_t *, uint8_t *, uint8_t *);

    scaled_output_info * scaled_info_get(void);
    void scaled_output_invalidate(void);
    gint scaled_output_check_reapply_scale(void);
    void scaled_output_check_reallocate(gint, gint, gint);

    void scaled_output_init(void);

#endif
