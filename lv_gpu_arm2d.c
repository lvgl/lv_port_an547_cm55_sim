/**
 * @file lv_gpu_arm2d.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_gpu_arm2d.h"
#include "lvgl.h"

#if LV_USE_GPU_ARM2D
#include "arm_2d.h"

/*********************
 *      DEFINES
 *********************/

#if LV_COLOR_DEPTH == 16
#define arm_2d_fill_colour              arm_2d_rgb16_fill_colour
#define arm_2d_fill_colour_with_alpha   arm_2d_rgb565_fill_colour_with_alpha
#define arm_2d_fill_colour_with_mask    arm_2d_rgb565_fill_colour_with_mask
#define arm_2d_fill_colour_with_mask_and_opacity                                \
            arm_2d_rgb565_fill_colour_with_mask_and_opacity
#define arm_2d_tile_copy                arm_2d_rgb16_tile_copy
#define arm_2d_alpha_blending           arm_2d_rgb565_alpha_blending
#define arm_2d_tile_copy_with_src_mask  arm_2d_rgb565_tile_copy_with_src_mask
#define arm_2d_color_t                  arm_2d_color_rgb565_t

#elif LV_COLOR_DEPTH == 32
#define arm_2d_fill_colour              arm_2d_rgb32_fill_colour
#define arm_2d_fill_colour_with_alpha   arm_2d_cccn888_fill_colour_with_alpha
#define arm_2d_fill_colour_with_mask    arm_2d_cccn888_fill_colour_with_mask
#define arm_2d_fill_colour_with_mask_and_opacity                                \
            arm_2d_cccn888_fill_colour_with_mask_and_opacity
#define arm_2d_tile_copy                arm_2d_rgb32_tile_copy
#define arm_2d_alpha_blending           arm_2d_cccn888_alpha_blending
#define arm_2d_tile_copy_with_src_mask  arm_2d_cccn888_tile_copy_with_src_mask
#define arm_2d_color_t                  arm_2d_color_cccn888_t

#else
#error The specified LV_COLOR_DEPTH is not supported by this version of lv_gpu_arm2d.c. 
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

#if __ARM_2D_HAS_ASYNC__
LV_ATTRIBUTE_FAST_MEM
static bool lv_draw_arm2d_fill_colour(  const arm_2d_tile_t *target_tile,
                                        const arm_2d_region_t *region,
                                        lv_color_t color,
                                        lv_opa_t opa,
                                        const arm_2d_tile_t *mask_tile);

LV_ATTRIBUTE_FAST_MEM
static bool lv_draw_arm2d_tile_copy(const arm_2d_tile_t *target_tile,
                                    const arm_2d_region_t *region,
                                    arm_2d_tile_t *source_tile, 
                                    lv_opa_t opa,
                                    arm_2d_tile_t *mask_tile);
#else
LV_ATTRIBUTE_FAST_MEM
static void arm_2d_fill_normal( lv_color_t * dest_buf,
                                const lv_area_t * dest_area,
                                lv_coord_t dest_stride,
                                lv_color_t color,
                                lv_opa_t opa,
                                const lv_opa_t * mask,
                                lv_coord_t mask_stride);
#endif

LV_ATTRIBUTE_FAST_MEM
static void lv_draw_arm2d_blend(lv_draw_ctx_t * draw_ctx, const lv_draw_sw_blend_dsc_t * dsc);
LV_ATTRIBUTE_FAST_MEM
static void lv_gpu_arm2d_wait_cb(lv_draw_ctx_t * draw_ctx);
                                    

                                    
/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_arm2d_init(void)
{
    arm_2d_init();
}


void lv_draw_arm2d_ctx_init(lv_disp_drv_t * drv, lv_draw_ctx_t * draw_ctx)
{
    lv_draw_sw_init_ctx(drv, draw_ctx);

    lv_draw_arm2d_ctx_t * arm2d_draw_ctx = (lv_draw_sw_ctx_t *)draw_ctx;

    arm2d_draw_ctx->blend = lv_draw_arm2d_blend;
    arm2d_draw_ctx->base_draw.wait_for_finish = lv_gpu_arm2d_wait_cb;

}

void lv_draw_arm2d_ctx_deinit(lv_disp_drv_t * drv, lv_draw_ctx_t * draw_ctx)
{
    LV_UNUSED(drv);
    LV_UNUSED(draw_ctx);
}

extern void test_flush(lv_color_t * color_p);

#if __ARM_2D_HAS_ASYNC__
LV_ATTRIBUTE_FAST_MEM
static void lv_draw_arm2d_blend(lv_draw_ctx_t * draw_ctx, const lv_draw_sw_blend_dsc_t * dsc)
{
    const lv_opa_t * mask;
    if(dsc->mask_buf == NULL) mask = NULL;
    if(dsc->mask_buf && dsc->mask_res == LV_DRAW_MASK_RES_TRANSP) return;
    else if(dsc->mask_res == LV_DRAW_MASK_RES_FULL_COVER) mask = NULL;
    else mask = dsc->mask_buf;


    lv_area_t blend_area;
    if(!_lv_area_intersect(&blend_area, dsc->blend_area, draw_ctx->clip_area)) {
        return;
    }
    
    static arm_2d_tile_t target_tile;
    static arm_2d_tile_t source_tile_orig;
    static arm_2d_tile_t source_tile;
    static arm_2d_tile_t mask_tile_orig;
    static arm_2d_tile_t mask_tile;
    static arm_2d_region_t target_region;

    bool is_accelerated = false;

    if(     dsc->blend_mode == LV_BLEND_MODE_NORMAL
      &&    lv_area_get_size(&blend_area) > 100) {

        lv_color_t * dest_buf = draw_ctx->buf;
        
          
        target_tile = (arm_2d_tile_t ){
            .tRegion = {
                .tSize = {
                    .iWidth = lv_area_get_width(draw_ctx->buf_area),
                    .iHeight = lv_area_get_height(draw_ctx->buf_area),
                },
            },
            .tInfo.bIsRoot = true,
            .phwBuffer = (uint16_t*)draw_ctx->buf,
        };
        
        target_region = (arm_2d_region_t){
            .tLocation = {
                .iX = blend_area.x1 - draw_ctx->buf_area->x1,
                .iY = blend_area.y1 - draw_ctx->buf_area->y1,
            },
            .tSize = {
                .iWidth = lv_area_get_width(&blend_area),
                .iHeight = lv_area_get_height(&blend_area),
            },
        };

        if (NULL != mask) {
            mask_tile_orig = (arm_2d_tile_t){
                .tRegion = {
                    .tSize = {
                        .iWidth = lv_area_get_width(dsc->mask_area),
                        .iHeight = lv_area_get_height(dsc->mask_area),
                    },
                },
                .tInfo = {
                    .bIsRoot = true,
                    .bHasEnforcedColour = true,
                    .tColourInfo = {
                        .chScheme = ARM_2D_COLOUR_8BIT,
                    },
                },
                .pchBuffer = (uint8_t *)mask,
            };
            
            arm_2d_tile_generate_child( 
                &mask_tile_orig,
                (arm_2d_region_t []) {
                    {
                        .tLocation = {
                            .iX = dsc->mask_area->x1 - blend_area.x1,
                            .iY = dsc->mask_area->y1 - blend_area.y1,
                        },
                        .tSize = mask_tile_orig.tRegion.tSize,
                    }
                },
                &mask_tile,
                false);
            mask_tile.tInfo.bDerivedResource = true;
        }
          

        const lv_color_t * src_buf = dsc->src_buf;
        if(src_buf) {
            source_tile_orig = (arm_2d_tile_t ){
                .tRegion = {
                    .tSize = {
                        .iWidth = lv_area_get_width(dsc->blend_area),
                        .iHeight = lv_area_get_height(dsc->blend_area),
                    },
                },
                .tInfo.bIsRoot = true,
                .phwBuffer = (uint16_t*)src_buf,
            };
            
            arm_2d_tile_generate_child( 
                &source_tile_orig,
                (arm_2d_region_t []) {
                    {
                        .tLocation = {
                            .iX = blend_area.x1 - dsc->blend_area->x1,
                            .iY = blend_area.y1 - dsc->blend_area->y1,
                        },
                        .tSize = source_tile_orig.tRegion.tSize,
                    }
                },
                &source_tile,
                false);
            source_tile.tInfo.bDerivedResource = true;
            
            is_accelerated = lv_draw_arm2d_tile_copy(
                                            &target_tile,
                                            &target_region,
                                            &source_tile,
                                            dsc->opa,
                                            (NULL == mask) ? NULL : &mask_tile);
        }
        else {
            is_accelerated = lv_draw_arm2d_fill_colour(  
                                            &target_tile,
                                            &target_region,
                                            dsc->color,
                                            dsc->opa,
                                            (NULL == mask) ? NULL : &mask_tile);
        }
    }

    if(!is_accelerated) {
        lv_draw_sw_blend_basic(draw_ctx, dsc);
    }
}


LV_ATTRIBUTE_FAST_MEM
static bool lv_draw_arm2d_fill_colour(  const arm_2d_tile_t *target_tile,
                                        const arm_2d_region_t *region,
                                        lv_color_t color,
                                        lv_opa_t opa,
                                        const arm_2d_tile_t *mask_tile)
{
    arm_fsm_rt_t result = (arm_fsm_rt_t)ARM_2D_ERR_NONE;

    if (NULL == mask_tile) {
        if(opa >= LV_OPA_MAX) {
            result = arm_2d_fill_colour( target_tile, region, color.full);
        } 
        else {
#if LV_COLOR_SCREEN_TRANSP
            return false;
#else
            result = arm_2d_fill_colour_with_alpha(
                                            target_tile,
                                            region,
                                            (arm_2d_color_t){color.full},
                                            opa);
#endif
        }
    }
    else {
         
        if(opa >= LV_OPA_MAX) {
            result = arm_2d_fill_colour_with_mask(
                                            target_tile,
                                            region,
                                            mask_tile,
                                            (arm_2d_color_t){color.full});
        } 
        else {
#if LV_COLOR_SCREEN_TRANSP
            return false;
#else
            result = arm_2d_fill_colour_with_mask_and_opacity(
                                            target_tile,
                                            region,
                                            mask_tile,
                                            (arm_2d_color_t){color.full},
                                            opa);
#endif
        }
    }
    
    if (result < 0) {
        /* error detected */
        return false;
    }

    return true;

}

LV_ATTRIBUTE_FAST_MEM
static bool lv_draw_arm2d_tile_copy(const arm_2d_tile_t *target_tile,
                                    const arm_2d_region_t *region,
                                    arm_2d_tile_t *source_tile,
                                    lv_opa_t opa,
                                    arm_2d_tile_t *mask_tile)
{
    arm_fsm_rt_t result = (arm_fsm_rt_t)ARM_2D_ERR_NONE;
    
    if(NULL == mask_tile) {
        if(opa >= LV_OPA_MAX) {
            result = arm_2d_tile_copy(  source_tile,
                                        target_tile,
                                        region,
                                        ARM_2D_CP_MODE_COPY);
        }
#if LV_COLOR_SCREEN_TRANSP
        else {
            return false;  /* not supported */
        }
#else
        else {
            result = arm_2d_alpha_blending( source_tile,
                                            target_tile,
                                            region,
                                            opa);
        }
#endif
    }
    else {
#if LV_COLOR_SCREEN_TRANSP
        return false;       /* not support */
#else
        
        if(opa >= LV_OPA_MAX) {
            result = arm_2d_tile_copy_with_src_mask(source_tile,
                                                    mask_tile,
                                                    target_tile,
                                                    region,
                                                    ARM_2D_CP_MODE_COPY);
        } else {
            return false;
        }
#endif
    }
    
    if (result < 0) {
        /* error detected */
        return false;
    }

    return true;
}

static void lv_gpu_arm2d_wait_cb(lv_draw_ctx_t * draw_ctx)
{
    lv_disp_t * disp = _lv_refr_get_disp_refreshing();

    arm_2d_op_wait_async(NULL);
    if(disp->driver && disp->driver->wait_cb) {
        disp->driver->wait_cb(disp->driver);
    }
    lv_draw_sw_wait_for_finish(draw_ctx);
}
#else


LV_ATTRIBUTE_FAST_MEM 
void lv_draw_arm2d_blend(lv_draw_ctx_t * draw_ctx, const lv_draw_sw_blend_dsc_t * dsc)
{
    const lv_opa_t * mask;
    if(dsc->mask_buf == NULL) mask = NULL;
    if(dsc->mask_buf && dsc->mask_res == LV_DRAW_MASK_RES_TRANSP) return;
    else if(dsc->mask_res == LV_DRAW_MASK_RES_FULL_COVER) mask = NULL;
    else mask = dsc->mask_buf;

    lv_coord_t dest_stride = lv_area_get_width(draw_ctx->buf_area);

    lv_area_t blend_area;
    if(!_lv_area_intersect(&blend_area, dsc->blend_area, draw_ctx->clip_area)) return;

    lv_disp_t * disp = _lv_refr_get_disp_refreshing();
    
    bool is_accelerated = false;
    do {
        if (NULL != disp->driver->set_px_cb) {
            break;
        }

        lv_color_t * dest_buf = draw_ctx->buf;
        if(disp->driver->set_px_cb == NULL) {
            dest_buf += dest_stride * (blend_area.y1 - draw_ctx->buf_area->y1) + (blend_area.x1 - draw_ctx->buf_area->x1);
        }

        const lv_color_t * src_buf = dsc->src_buf;
        lv_coord_t src_stride;
        if(src_buf) {
            src_stride = lv_area_get_width(dsc->blend_area);
            src_buf += src_stride * (blend_area.y1 - dsc->blend_area->y1) + (blend_area.x1 - dsc->blend_area->x1);
        }
        else {
            src_stride = 0;
        }

        lv_coord_t mask_stride;
        if(mask) {
            mask_stride = lv_area_get_width(dsc->mask_area);
            mask += mask_stride * (dsc->mask_area->y1 - blend_area.y1) + (dsc->mask_area->x1 - blend_area.x1);
        }
        else {
            mask_stride = 0;
        }

        lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);


        if(dsc->src_buf == NULL) {
            if(dsc->blend_mode == LV_BLEND_MODE_NORMAL) {
                arm_2d_fill_normal( dest_buf, 
                                    &blend_area, 
                                    dest_stride, 
                                    dsc->color, 
                                    dsc->opa, 
                                    mask, 
                                    mask_stride);
            }
    #if LV_DRAW_COMPLEX
            else {
                break;
            }
    #endif
        }
        else {
            
            if(dsc->blend_mode == LV_BLEND_MODE_NORMAL) {
                break;
            //    map_normal(dest_buf, &blend_area, dest_stride, src_buf, src_stride, dsc->opa, mask, mask_stride);
            }
    #if LV_DRAW_COMPLEX
            else {
                break;
            }
    #endif
        }

        is_accelerated = true;
    } while(0);

    if (!is_accelerated) lv_draw_sw_blend_basic(draw_ctx, dsc);
}

#if LV_COLOR_SCREEN_TRANSP == 0
#define FILL_NORMAL_MASK_PX(color)                                                          \
    if(*mask == LV_OPA_COVER) *dest_buf = color;                                 \
    else *dest_buf = lv_color_mix(color, *dest_buf, *mask);            \
    mask++;                                                         \
    dest_buf++;

#else
#define FILL_NORMAL_MASK_PX(color)                                               \
    if(*mask == LV_OPA_COVER) *dest_buf = color;                                 \
    else if(disp->driver->screen_transp) lv_color_mix_with_alpha(*dest_buf, dest_buf->ch.alpha, color, *mask, dest_buf, &dest_buf->ch.alpha);           \
    else *dest_buf = lv_color_mix(color, *dest_buf, *mask);            \
    mask++;                                                         \
    dest_buf++;
#endif

#define MAP_NORMAL_MASK_PX(x)                                                          \
    if(*mask_tmp_x) {          \
        if(*mask_tmp_x == LV_OPA_COVER) dest_buf[x] = src_buf[x];                                 \
        else dest_buf[x] = lv_color_mix(src_buf[x], dest_buf[x], *mask_tmp_x);            \
    }                                                                                               \
    mask_tmp_x++;

#define MAP_NORMAL_MASK_PX_SCR_TRANSP(x)                        \
    if(*mask_tmp_x) {          \
        if(*mask_tmp_x == LV_OPA_COVER) dest_buf[x] = src_buf[x];                                 \
        else if(disp->driver->screen_transp) lv_color_mix_with_alpha(dest_buf[x], dest_buf[x].ch.alpha,              \
                                                                         src_buf[x], *mask_tmp_x, &dest_buf[x], &dest_buf[x].ch.alpha);                  \
        else dest_buf[x] = lv_color_mix(src_buf[x], dest_buf[x], *mask_tmp_x);            \
    }                                                                                               \
    mask_tmp_x++;

LV_ATTRIBUTE_FAST_MEM 
static void arm_2d_fill_normal( lv_color_t * dest_buf,
                                const lv_area_t * dest_area,
                                lv_coord_t dest_stride,
                                lv_color_t color,
                                lv_opa_t opa,
                                const lv_opa_t * mask,
                                lv_coord_t mask_stride)
{
    lv_disp_t * disp = _lv_refr_get_disp_refreshing();
    int32_t w = lv_area_get_width(dest_area);
    int32_t h = lv_area_get_height(dest_area);

    int32_t x;
    int32_t y;

    /*No mask*/
    if(mask == NULL) {
        if(opa >= LV_OPA_MAX) {
        
            extern
            void __arm_2d_impl_rgb16_colour_filling( uint16_t *__RESTRICT pTarget,
                                    int16_t iTargetStride,
                                    arm_2d_size_t *__RESTRICT ptCopySize,
                                    uint16_t Colour);
            /*
            for(y = 0; y < h; y++) {
                lv_color_fill(dest_buf, color, w);
                dest_buf += dest_stride;
            }*/
            __arm_2d_impl_rgb16_colour_filling( (uint16_t *)dest_buf, 
                                                dest_stride, 
                                                (arm_2d_size_t[]) { {
                                                    .iWidth = w,
                                                    .iHeight = h,
                                                }},
                                                color.full);
        }
        /*Has opacity*/
        else {
            lv_color_t last_dest_color = lv_color_black();
            lv_color_t last_res_color = lv_color_mix(color, last_dest_color, opa);

            uint16_t color_premult[3];
            lv_color_premult(color, opa, color_premult);
            lv_opa_t opa_inv = 255 - opa;

            for(y = 0; y < h; y++) {
                for(x = 0; x < w; x++) {
                    if(last_dest_color.full != dest_buf[x].full) {
                        last_dest_color = dest_buf[x];

#if LV_COLOR_SCREEN_TRANSP
                        if(disp->driver->screen_transp) {
                            lv_color_mix_with_alpha(dest_buf[x], dest_buf[x].ch.alpha, color, opa, &last_res_color,
                                                    &last_res_color.ch.alpha);
                        }
                        else
#else
                        LV_UNUSED(disp);
#endif
                        {
                            last_res_color = lv_color_mix_premult(color_premult, dest_buf[x], opa_inv);
                        }
                    }
                    dest_buf[x] = last_res_color;
                }
                dest_buf += dest_stride;
            }
        }
    }
    /*Masked*/
    else {
#if LV_COLOR_DEPTH == 16
        uint32_t c32 = color.full + ((uint32_t)color.full << 16);
#endif
        /*Only the mask matters*/
        if(opa >= LV_OPA_MAX) {
            int32_t x_end4 = w - 4;
            for(y = 0; y < h; y++) {
                for(x = 0; x < w && ((lv_uintptr_t)(mask) & 0x3); x++) {
                    FILL_NORMAL_MASK_PX(color)
                }

                for(; x <= x_end4; x += 4) {
                    uint32_t mask32 = *((uint32_t *)mask);
                    if(mask32 == 0xFFFFFFFF) {
#if LV_COLOR_DEPTH == 16
                        if((lv_uintptr_t)dest_buf & 0x3) {
                            *(dest_buf + 0) = color;
                            uint32_t * d = (uint32_t *)(dest_buf + 1);
                            *d = c32;
                            *(dest_buf + 3) = color;
                        }
                        else {
                            uint32_t * d = (uint32_t *)dest_buf;
                            *d = c32;
                            *(d + 1) = c32;
                        }
#else
                        dest_buf[0] = color;
                        dest_buf[1] = color;
                        dest_buf[2] = color;
                        dest_buf[3] = color;
#endif
                        dest_buf += 4;
                        mask += 4;
                    }
                    else if(mask32) {
                        FILL_NORMAL_MASK_PX(color)
                        FILL_NORMAL_MASK_PX(color)
                        FILL_NORMAL_MASK_PX(color)
                        FILL_NORMAL_MASK_PX(color)
                    }
                    else {
                        mask += 4;
                        dest_buf += 4;
                    }
                }

                for(; x < w ; x++) {
                    FILL_NORMAL_MASK_PX(color)
                }
                dest_buf += (dest_stride - w);
                mask += (mask_stride - w);
            }
        }
        /*With opacity*/
        else {
            /*Buffer the result color to avoid recalculating the same color*/
            lv_color_t last_dest_color;
            lv_color_t last_res_color;
            lv_opa_t last_mask = LV_OPA_TRANSP;
            last_dest_color.full = dest_buf[0].full;
            last_res_color.full = dest_buf[0].full;
            lv_opa_t opa_tmp = LV_OPA_TRANSP;

            for(y = 0; y < h; y++) {
                for(x = 0; x < w; x++) {
                    if(*mask) {
                        if(*mask != last_mask) opa_tmp = *mask == LV_OPA_COVER ? opa :
                                                             (uint32_t)((uint32_t)(*mask) * opa) >> 8;
                        if(*mask != last_mask || last_dest_color.full != dest_buf[x].full) {
#if LV_COLOR_SCREEN_TRANSP
                            if(disp->driver->screen_transp) {
                                lv_color_mix_with_alpha(dest_buf[x], dest_buf[x].ch.alpha, color, opa_tmp, &last_res_color,
                                                        &last_res_color.ch.alpha);
                            }
                            else
#endif
                            {
                                if(opa_tmp == LV_OPA_COVER) last_res_color = color;
                                else last_res_color = lv_color_mix(color, dest_buf[x], opa_tmp);
                            }
                            last_mask = *mask;
                            last_dest_color.full = dest_buf[x].full;
                        }
                        dest_buf[x] = last_res_color;
                    }
                    mask++;
                }
                dest_buf += dest_stride;
                mask += (mask_stride - w);
            }
        }
    }
}

static void lv_gpu_arm2d_wait_cb(lv_draw_ctx_t * draw_ctx)
{
    lv_disp_t * disp = _lv_refr_get_disp_refreshing();

    if(disp->driver && disp->driver->wait_cb) {
        disp->driver->wait_cb(disp->driver);
    }
    lv_draw_sw_wait_for_finish(draw_ctx);
}


#endif


/**********************
 *   STATIC FUNCTIONS
 **********************/

#if 0
static void invalidate_cache(void)
{
    lv_disp_t * disp = _lv_refr_get_disp_refreshing();
    if(disp->driver->clean_dcache_cb) disp->driver->clean_dcache_cb(disp->driver);
    else {
#if __CORTEX_M >= 0x07
        if((SCB->CCR) & (uint32_t)SCB_CCR_DC_Msk)
            SCB_CleanInvalidateDCache();
#endif
    }
}
#endif

#endif
