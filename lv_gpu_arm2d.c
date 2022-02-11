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

static bool lv_draw_arm2d_fill_colour(
                                     const arm_2d_tile_t *target_tile,
                                     const arm_2d_region_t *region,
                                     lv_color_t color,
                                     lv_opa_t opa,
                                     const arm_2d_tile_t *mask_tile);


static bool lv_draw_arm2d_tile_copy(const arm_2d_tile_t *target_tile,
                                    const arm_2d_region_t *region,
                                    arm_2d_tile_t *source_tile, 
                                    lv_opa_t opa,
                                    arm_2d_tile_t *mask_tile);



static void lv_draw_arm2d_blend(lv_draw_ctx_t * draw_ctx, const lv_draw_sw_blend_dsc_t * dsc);

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

    bool is_accelerated = false;

    if(     dsc->blend_mode == LV_BLEND_MODE_NORMAL
      &&    lv_area_get_size(&blend_area) > 100) {

        lv_color_t * dest_buf = draw_ctx->buf;
        

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



static bool lv_draw_arm2d_fill_colour(  const arm_2d_tile_t *target_tile,
                                        const arm_2d_region_t *region,
                                        lv_color_t color,
                                        lv_opa_t opa,
                                        const arm_2d_tile_t *mask_tile)
{
    arm_fsm_rt_t result = (arm_fsm_rt_t)ARM_2D_ERR_NONE;

    if (NULL == mask_tile) {
        if(opa >= LV_OPA_MAX) {
            result = arm_2d_rgb16_fill_colour( target_tile, region, color.full);
        } 
        else {
#if LV_COLOR_SCREEN_TRANSP
            return false;
#else
            result = arm_2d_rgb565_fill_colour_with_alpha(
                                            target_tile,
                                            region,
                                            (arm_2d_color_rgb565_t){color.full},
                                            opa);
#endif
        }
    }
    else {
         
        if(opa >= LV_OPA_MAX) {
            result = arm_2d_rgb565_fill_colour_with_mask(
                                            target_tile,
                                            region,
                                            mask_tile,
                                            (arm_2d_color_rgb565_t){color.full});
        } 
        else {
#if LV_COLOR_SCREEN_TRANSP
            return false;
#else
            result = arm_2d_rgb565_fill_colour_with_mask_and_opacity(
                                            target_tile,
                                            region,
                                            mask_tile,
                                            (arm_2d_color_rgb565_t){color.full},
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


static bool lv_draw_arm2d_tile_copy(const arm_2d_tile_t *target_tile,
                                    const arm_2d_region_t *region,
                                    arm_2d_tile_t *source_tile,
                                    lv_opa_t opa,
                                    arm_2d_tile_t *mask_tile)
{
    arm_fsm_rt_t result = (arm_fsm_rt_t)ARM_2D_ERR_NONE;
    
    if(NULL == mask_tile) {
        if(opa >= LV_OPA_MAX) {
            return false;
            result = arm_2d_rgb16_tile_copy( target_tile,
                                    source_tile,
                                    region,
                                    ARM_2D_CP_MODE_COPY);
        }
#if LV_COLOR_SCREEN_TRANSP
        else {
            return false;  /* not supported */
        }
#else
        else {
            return false;
            result = arm_2d_rgb565_alpha_blending(   target_tile,
                                            source_tile,
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
            result = arm_2d_rgb565_tile_copy_with_src_mask( source_tile,
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
