/**
 * @file lv_draw_sw_helium.h
 *
 */

#ifndef LV_DRAW_SW_HELIUM_H
#define LV_DRAW_SW_HELIUM_H

#ifdef __cplusplus
extern "C" {
#endif

/* *INDENT-OFF* */

/*********************
 *      INCLUDES
 *********************/

//#include "../../../lv_conf_internal.h"
#include "lvgl.h"

#ifdef LV_DRAW_SW_HELIUM_CUSTOM_INCLUDE
#include LV_DRAW_SW_HELIUM_CUSTOM_INCLUDE
#endif

#if LV_USE_DRAW_ARM2D

#define __ARM_2D_IMPL__
#include "arm_2d.h"
#include "__arm_2d_impl.h"
#include "arm_2d_helper.h"

#if defined(__IS_COMPILER_ARM_COMPILER_5__)
#pragma diag_suppress 174,177,188,68,513,144,1296
#elif defined(__IS_COMPILER_IAR__)
#pragma diag_suppress=Pa093
#elif defined(__IS_COMPILER_GCC__)
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
#endif

/*********************
 *      DEFINES
 *********************/
#ifndef LV_DRAW_SW_RGB565_SWAP
#define LV_DRAW_SW_RGB565_SWAP(__buf_ptr, __buf_size_px)                        \
    _lv_draw_sw_rgb565_swap_helium((__buf_ptr), (__buf_size_px))
#endif

#ifndef LV_DRAW_SW_IMAGE
#define LV_DRAW_SW_IMAGE(__transformed,                                         \
                         __cf,                                                  \
                         __src_buf,                                             \
                         __img_coords,                                          \
                         __src_stride,                                          \
                         __blend_area,                                          \
                         __draw_unit,                                           \
                         __draw_dsc)                                            \
    _lv_draw_sw_image_helium(   (__transformed),                                \
                                (__cf),                                         \
                                (uint8_t *)(__src_buf),                         \
                                (__img_coords),                                 \
                                (__src_stride),                                 \
                                (__blend_area),                                 \
                                (__draw_unit),                                  \
                                (__draw_dsc))
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
extern void arm_2d_helper_swap_rgb16(uint16_t * phwBuffer, uint32_t wCount);

/**********************
 *      MACROS
 **********************/

#define __ARM_2D_PREPARE_TRANS_AND_TARGET_REGION(__TRANS_PREPARE, ...)          \
        do {                                                                    \
            __TRANS_PREPARE(                                                    \
                    NULL,                                                       \
                    __VA_ARGS__);                                               \
                                                                                \
            target_region = (arm_2d_region_t) {                                 \
                .tLocation = {                                                  \
                    .iX = (int16_t)(coords->x1 - draw_unit->clip_area->x1),     \
                    .iY = (int16_t)(coords->y1 - draw_unit->clip_area->y1),     \
                },                                                              \
                .tSize = src_size,                                              \
            };                                                                  \
                                                                                \
            arm_2d_size_t tTransSize                                            \
                = ARM_2D_CTRL.DefaultOP                                         \
                    .tTransform.Source.ptTile->tRegion.tSize;                   \
                                                                                \
            if (target_region.tSize.iWidth < tTransSize.iWidth) {               \
                int16_t iDelta = tTransSize.iWidth - target_region.tSize.iWidth;\
                target_region.tLocation.iX -= iDelta / 2;                       \
                target_region.tSize.iWidth = tTransSize.iWidth;                 \
            }                                                                   \
                                                                                \
            if (target_region.tSize.iHeight < tTransSize.iHeight) {             \
                int16_t iDelta                                                  \
                    = tTransSize.iHeight - target_region.tSize.iHeight;         \
                target_region.tLocation.iY -= iDelta / 2;                       \
                target_region.tSize.iHeight = tTransSize.iHeight;               \
            }                                                                   \
        } while(0)

static inline lv_result_t _lv_draw_sw_rgb565_swap_helium(void * buf, uint32_t buf_size_px)
{
    arm_2d_helper_swap_rgb16((uint16_t *)buf, buf_size_px);
    return LV_RESULT_OK;
}

static inline lv_result_t _lv_draw_sw_image_helium( 
                                        bool is_transform, 
                                        lv_color_format_t src_cf,
                                        uint8_t *src_buf,
                                        const lv_area_t * coords,
                                        int32_t src_stride,
                                        const lv_area_t * des_area,
                                        lv_draw_unit_t * draw_unit,
                                        const lv_draw_image_dsc_t * draw_dsc)
{
    lv_result_t result = LV_RESULT_INVALID;
    lv_layer_t * layer = draw_unit->target_layer;
    lv_color_format_t des_cf = layer->color_format;
    static bool arm_2d_initialized = false;
    
    if (!arm_2d_initialized) {
        arm_2d_initialized = true;
        arm_2d_init();
    }

    do {
        /* filter the unsupported colour format combination */
        if((LV_COLOR_FORMAT_RGB565 == des_cf)
        && !(  (LV_COLOR_FORMAT_RGB565 == src_cf)
           ||  (LV_COLOR_FORMAT_RGB565A8 == src_cf))) {
            break;
        }
        if((LV_COLOR_FORMAT_XRGB8888 == des_cf)
        && !(  (LV_COLOR_FORMAT_ARGB8888 == src_cf)
           ||  (LV_COLOR_FORMAT_XRGB8888 == src_cf))) {
            break;
        }

        /* ------------- prepare parameters for arm-2d APIs - BEGIN --------- */
        
        lv_area_t blend_area;
        if(!_lv_area_intersect(&blend_area, des_area, draw_unit->clip_area)) {
            break;
        }

        int32_t src_w = lv_area_get_width(coords);
        int32_t src_h = lv_area_get_height(coords);

        arm_2d_size_t src_size = {
            .iWidth = (int16_t)src_w,
            .iHeight = (int16_t)src_h,
        };

        arm_2d_size_t des_size;

        do{
            int32_t des_w = lv_area_get_width(&blend_area);
            int32_t des_h = lv_area_get_height(&blend_area);

            LV_ASSERT(des_w <= INT16_MAX);
            LV_ASSERT(des_h <= INT16_MAX);

            des_size.iWidth = (int16_t)des_w;
            des_size.iHeight = (int16_t)des_h;
        } while(0);
        
        arm_2d_size_t copy_size = {
            .iWidth = MIN(des_size.iWidth, src_size.iWidth),
            .iHeight = MIN(des_size.iHeight, src_size.iHeight),
        };
        
        int32_t des_stride = lv_draw_buf_width_to_stride(
                                lv_area_get_width(&layer->buf_area), 
                                des_cf);
        uint8_t *des_buf_moved = (uint8_t *)lv_draw_layer_go_to_xy(
                                            layer, 
                                            blend_area.x1 - layer->buf_area.x1,
                                            blend_area.y1 - layer->buf_area.y1);
        uint8_t *des_buf = (uint8_t *)layer->buf;
        uint8_t opa = draw_dsc->opa;
        
        /* ------------- prepare parameters for arm-2d APIs - END ----------- */

        if(is_transform) {
            /* transform with/without recolour */
            if(draw_dsc->scale_x != draw_dsc->scale_y) {
                break;
            }

            static arm_2d_tile_t target_tile_origin;
            static arm_2d_tile_t target_tile;
            arm_2d_region_t clip_region;
            static arm_2d_region_t target_region;
            
            target_tile_origin = (arm_2d_tile_t) {
                .tRegion = {
                    .tSize = {
                        .iWidth = (int16_t)lv_area_get_width(&layer->buf_area),
                        .iHeight = (int16_t)lv_area_get_height(&layer->buf_area),
                    },
                },
                .tInfo = {
                    .bIsRoot = true,
                },
                .phwBuffer = (uint16_t *)des_buf,
            };

            clip_region = (arm_2d_region_t) {
                .tLocation = {
                    .iX = (int16_t)(draw_unit->clip_area->x1 - layer->buf_area.x1),
                    .iY = (int16_t)(draw_unit->clip_area->y1 - layer->buf_area.y1),
                },
                .tSize = {
                    .iWidth = (int16_t)lv_area_get_width(draw_unit->clip_area),
                    .iHeight = (int16_t)lv_area_get_height(draw_unit->clip_area),
                },
            };

            arm_2d_tile_generate_child(&target_tile_origin,
                                       &clip_region,
                                       &target_tile,
                                       false);

            static arm_2d_tile_t source_tile;

            source_tile = (arm_2d_tile_t) {
                .tRegion = {
                    .tSize = {
                        .iWidth = (int16_t)src_w,
                        .iHeight = (int16_t)src_h,
                    },
                },
                .tInfo = {
                    .bIsRoot = true,
                },
                .pchBuffer = (uint8_t *)src_buf,
            };

            static arm_2d_location_t source_center, target_center;
            source_center.iX = draw_dsc->pivot.x;
            source_center.iY = draw_dsc->pivot.y;


            if(LV_COLOR_FORMAT_A8 == src_cf) {

                if(LV_COLOR_FORMAT_RGB565 == des_cf) {
                
                } else if(LV_COLOR_FORMAT_XRGB8888 == des_cf) {
                
                } else {
                    break;
                }

            }
            else if(LV_COLOR_FORMAT_RGB565A8 == src_cf) {
                LV_ASSERT(LV_COLOR_FORMAT_RGB565 == des_cf);

                /* mask_buf = src_buf + src_stride * src_w / header->w * src_h; */
                uint8_t *mask_buf = src_buf + src_stride * src_h;
                int32_t mask_stride = src_stride / 2;

                static arm_2d_tile_t mask_tile;
                mask_tile = source_tile;

                mask_tile.tInfo.bHasEnforcedColour = true;
                mask_tile.tInfo.tColourInfo.chScheme = ARM_2D_COLOUR_GRAY8;
                mask_tile.pchBuffer = mask_buf;

                if(opa >= LV_OPA_MAX) {
                    __ARM_2D_PREPARE_TRANS_AND_TARGET_REGION(
                        arm_2dp_rgb565_tile_transform_with_src_mask_prepare,
                        &source_tile,
                        &mask_tile,
                        source_center,
                        ARM_2D_ANGLE((draw_dsc->rotation / 10.0f)),
                        draw_dsc->scale_x / 256.0f
                        );
                }
                else {
                    __ARM_2D_PREPARE_TRANS_AND_TARGET_REGION(
                        arm_2dp_rgb565_tile_transform_with_src_mask_and_opacity_prepare,
                        &source_tile,
                        &mask_tile,
                        source_center,
                        ARM_2D_ANGLE((draw_dsc->rotation / 10.0f)),
                        draw_dsc->scale_x / 256.0f,
                        opa
                        );
                }

                arm_2d_tile_transform(
                    &target_tile,
                    &target_region,
                    NULL
                    );

            } else if(LV_COLOR_FORMAT_RGB565 == src_cf) {
                LV_ASSERT(LV_COLOR_FORMAT_RGB565 == des_cf);

            #if 1
                if(opa >= LV_OPA_MAX) {
                    __ARM_2D_PREPARE_TRANS_AND_TARGET_REGION(
                        arm_2dp_rgb565_tile_transform_only_prepare,
                        &source_tile,
                        source_center,
                        ARM_2D_ANGLE((draw_dsc->rotation / 10.0f)),
                        draw_dsc->scale_x / 256.0f
                        );
                    arm_2d_tile_transform(
                        &target_tile,
                        &target_region,
                        NULL
                        );
                }
                else {
                    __ARM_2D_PREPARE_TRANS_AND_TARGET_REGION(
                        arm_2dp_rgb565_tile_transform_only_with_opacity_prepare,
                        &source_tile,
                        source_center,
                        ARM_2D_ANGLE((draw_dsc->rotation / 10.0f)),
                        draw_dsc->scale_x / 256.0f,
                        opa
                        );
                    arm_2d_tile_transform(
                        &target_tile,
                        &target_region,
                        NULL
                        );
                }
            #else
//                __arm_2d_impl_rgb16_copy(
//                    (uint16_t *)src_buf,
//                    src_w,
//                    (uint16_t *)des_buf_moved,
//                    (int16_t)lv_area_get_width(&layer->buf_area),
//                    &copy_size
//                );

                arm_2d_rgb16_tile_copy_only(
                    &source_tile,
                    &target_tile,
                    NULL
                );

            #endif
            
            } else if(LV_COLOR_FORMAT_ARGB8888 == src_cf) {
                LV_ASSERT(LV_COLOR_FORMAT_XRGB8888 == des_cf);

            } else if(LV_COLOR_FORMAT_XRGB8888 == src_cf) {
                LV_ASSERT(LV_COLOR_FORMAT_XRGB8888 == des_cf);

            } else {
                break;
            }

        } else if(draw_dsc->recolor_opa > LV_OPA_MIN) {
            /* recolor-only */
            __NOP();
        }

        result = LV_RESULT_OK;
    } while(0);

    return result;
}



#endif /* LV_USE_DRAW_ARM2D */

/* *INDENT-ON* */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_SW_HELIUM_H*/
