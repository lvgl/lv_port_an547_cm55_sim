/**
 * @file lv_gpu_arm2d.h
 *
 */

#ifndef LV_GPU_ARM2D_H
#define LV_GPU_ARM2D_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
#include "src/draw/sw/lv_draw_sw.h"

#if LV_USE_GPU_ARM2D

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef lv_draw_sw_ctx_t lv_draw_arm2d_ctx_t;

struct _lv_disp_drv_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void lv_draw_arm2d_init(void);

void lv_draw_arm2d_ctx_init(struct _lv_disp_drv_t * drv, lv_draw_ctx_t * draw_ctx);

void lv_draw_arm2d_ctx_deinit(struct _lv_disp_drv_t * drv, lv_draw_ctx_t * draw_ctx);

void lv_draw_arm2d_blend(lv_draw_ctx_t * draw_ctx, const lv_draw_sw_blend_dsc_t * dsc);

void lv_gpu_arm2d_wait_cb(lv_draw_ctx_t * draw_ctx);

/**********************
 *      MACROS
 **********************/

#endif  /*LV_USE_GPU_ARM2D*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_GPU_ARM2D_H*/
