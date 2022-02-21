
/*
 * Auto generated Run-Time-Environment Configuration File
 *      *** Do not modify ! ***
 *
 * Project: 'lv_template' 
 * Target:  'Cortex-M55_FVP' 
 */

#ifndef RTE_COMPONENTS_H
#define RTE_COMPONENTS_H


/*
 * Define the Device Header File: 
 */
#define CMSIS_device_header "SSE300MPS3.h"

/* ARM::CMSIS Driver:USART:1.0.0 */
#define RTE_Drivers_USART
/* Arm::Acceleration:Arm-2D:Core:0.9.10 */
#define __RTE_ACCELERATION_ARM_2D__
/* GorgonMeducer.Performance Counter::Utilities:perf_counter:Core:Library:1.9.2 */
#define __PERF_COUNTER__
/* Keil.ARM Compiler::Compiler:I/O:STDOUT:User:1.2.0 */
#define RTE_Compiler_IO_STDOUT          /* Compiler I/O: STDOUT */
          #define RTE_Compiler_IO_STDOUT_User     /* Compiler I/O: STDOUT User */
/* LVGL.LVGL::LVGL:lvgl:Benchmark:8.3.0-dev */
/*! \brief enable demo:bencharmk */
#define LV_USE_DEMO_BENCHMARK         1
/* LVGL.LVGL::LVGL:lvgl:Essential:8.3.0-dev */
/*! \brief Enable LVGL */
#define RTE_GRAPHICS_LVGL
/* LVGL.LVGL::LVGL:lvgl:Extra Themes:8.3.0-dev */
/*! \brief use extra themes, widgets and layouts */
#define RTE_GRAPHICS_LVGL_USE_EXTRA_THEMES


#endif /* RTE_COMPONENTS_H */
