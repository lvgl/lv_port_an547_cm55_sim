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


/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void lv_draw_arm2d_fill_colour(lv_color_t * dest_buf, 
                                     lv_area_t *des_area, 
                                     const lv_area_t * fill_area,
                                     lv_color_t color,
                                     lv_opa_t opa,
                                     arm_2d_tile_t *mask_tile);


static void lv_draw_arm2d_tile_copy(lv_color_t * dest_buf, 
                                    const lv_area_t * dest_area, 
                                    lv_coord_t dest_stride,
                                    const lv_color_t * src_buf, 
                                    lv_coord_t src_stride, 
                                    lv_opa_t opa);


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

void lv_draw_arm2d_blend(lv_draw_ctx_t * draw_ctx, const lv_draw_sw_blend_dsc_t * dsc)
{
    const lv_opa_t * mask;
    if(dsc->mask_buf == NULL) mask = NULL;
    if(dsc->mask_buf && dsc->mask_res == LV_DRAW_MASK_RES_TRANSP) return;
    else if(dsc->mask_res == LV_DRAW_MASK_RES_FULL_COVER) mask = NULL;
    else mask = dsc->mask_buf;
    
    
    lv_area_t blend_area;
    if(!_lv_area_intersect(&blend_area, dsc->blend_area, draw_ctx->clip_area)) 
        return;

    bool done = false;

    if(dsc->blend_mode == LV_BLEND_MODE_NORMAL && lv_area_get_size(&blend_area) > 100) {
        lv_coord_t dest_stride = lv_area_get_width(draw_ctx->buf_area);

        lv_color_t * dest_buf = draw_ctx->buf;
        //dest_buf += dest_stride * (blend_area.y1 - draw_ctx->buf_area->y1) + (blend_area.x1 - draw_ctx->buf_area->x1);

        const lv_color_t * src_buf = dsc->src_buf;
        if(src_buf) {
            lv_draw_sw_blend_basic(draw_ctx, dsc);
            // lv_coord_t src_stride;
            //src_stride = lv_area_get_width(dsc->blend_area);
            //src_buf += src_stride * (blend_area.y1 - dsc->blend_area->y1) + (blend_area.x1 -  dsc->blend_area->x1);
            //lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);
            //lv_draw_arm2d_tile_copy(dest_buf, &blend_area, dest_stride, src_buf, src_stride, dsc->opa);
            done = true;
        }
        else {
            arm_2d_tile_t mask_tile = {
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
            lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);
            lv_draw_arm2d_fill_colour(  dest_buf, 
                                        draw_ctx->buf_area, 
                                        &blend_area, 
                                        dsc->color, 
                                        dsc->opa,
                                        (NULL == mask) ? NULL : &mask_tile);
            done = true;
        }
    }

    if(!done) lv_draw_sw_blend_basic(draw_ctx, dsc);
}



static void lv_draw_arm2d_fill_colour(  lv_color_t * dest_buf, 
                                        lv_area_t *des_area, 
                                        const lv_area_t * fill_area,
                                        lv_color_t color,
                                        lv_opa_t opa,
                                        arm_2d_tile_t *mask_tile)
{
    arm_2d_tile_t target_tile = {
        
        .tRegion = {
            .tSize = {
                .iWidth = lv_area_get_width(des_area),
                .iHeight = lv_area_get_height(des_area),
            },
        },
        .tInfo.bIsRoot = true,
        .phwBuffer = (uint16_t*)dest_buf,
    };
    
    arm_2d_region_t target_region = {
        .tLocation = {
            .iX = fill_area->x1,
            .iY = fill_area->y1,
        },
        .tSize = {
            .iWidth = lv_area_get_width(fill_area),
            .iHeight = lv_area_get_height(fill_area),
        },
    };

    if (NULL == mask_tile) {
        if(opa >= LV_OPA_MAX) {
            arm_2d_rgb16_fill_colour( &target_tile, &target_region, color.full);
        } 
        else {
            arm_2d_rgb565_fill_colour_with_alpha(   
                                            &target_tile,
                                            &target_region,
                                            (arm_2d_color_rgb565_t){color.full},
                                            opa);
        }
    }
    else {
        if(opa >= LV_OPA_MAX) {
            arm_2d_rgb565_fill_colour_with_mask(
                                            &target_tile,
                                            &target_region,
                                            mask_tile,
                                            (arm_2d_color_rgb565_t){color.full});
        } 
        else {
            arm_2d_rgb565_fill_colour_with_mask_and_opacity(   
                                            &target_tile, 
                                            &target_region, 
                                            mask_tile,
                                            (arm_2d_color_rgb565_t){color.full}, 
                                            opa);
        }
    }

}


static void lv_draw_arm2d_tile_copy(lv_color_t * dest_buf, 
                                    const lv_area_t * dest_area, 
                                    lv_coord_t dest_stride,
                                    const lv_color_t * src_buf, 
                                    lv_coord_t src_stride, 
                                    lv_opa_t opa)
{
    arm_2d_tile_t target_tile = {
        
        .tRegion = {
            .tSize = {
                .iWidth = dest_stride,
                .iHeight = lv_area_get_height(dest_area),
            },
        },
        .tInfo.bIsRoot = true,
        .phwBuffer = (uint16_t*)dest_buf,
    };
    
    arm_2d_region_t target_region = {
        .tLocation = {
            .iX = dest_area->x1,
            .iY = dest_area->y1,
        },
        .tSize = {
            .iWidth = lv_area_get_width(dest_area),
            .iHeight = lv_area_get_height(dest_area),
        },
    };
    
    arm_2d_tile_t source_tile = {
        
        .tRegion = {
            .tSize = {
                .iWidth = src_stride,
                .iHeight = lv_area_get_height(dest_area),
            },
        },
        .tInfo.bIsRoot = true,
        .phwBuffer = (uint16_t*)src_buf,
    };

    if (opa >= LV_OPA_MAX) {
        arm_2d_rgb16_tile_copy( &target_tile,  
                                &source_tile,
                                &target_region,        
                                ARM_2D_CP_MODE_COPY);
    }
    else {
        arm_2d_rgb565_alpha_blending(   &target_tile,  
                                        &source_tile,
                                        &target_region,        
                                        opa);
    }
    


}

void lv_gpu_arm2d_wait_cb(lv_draw_ctx_t * draw_ctx)
{
    lv_disp_t * disp = _lv_refr_get_disp_refreshing();
    
    arm_2d_op_wait_async(NULL);
    if(disp->driver && disp->driver->wait_cb) {
        disp->driver->wait_cb(disp->driver);
    }
    lv_draw_sw_wait_for_finish(draw_ctx);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

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
