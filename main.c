/****************************************************************************
*  Copyright 2021 Gorgon Meducer (Email:embedded_zhuoran@hotmail.com)       *
*                                                                           *
*  Licensed under the Apache License, Version 2.0 (the "License");          *
*  you may not use this file except in compliance with the License.         *
*  You may obtain a copy of the License at                                  *
*                                                                           *
*     http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                           *
*  Unless required by applicable law or agreed to in writing, software      *
*  distributed under the License is distributed on an "AS IS" BASIS,        *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
*  See the License for the specific language governing permissions and      *
*  limitations under the License.                                           *
*                                                                           *
****************************************************************************/

/*============================ INCLUDES ======================================*/
#include "app_cfg.h"
#include "platform.h"
#include "cmsis_compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include "demos/lv_demos.h"

#include "lv_port_disp_template.h"
#include "lv_port_indev_template.h"

#if defined(__clang__)
#   pragma clang diagnostic ignored "-Wcompound-token-split-by-macro"
#endif

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/
/*============================ IMPLEMENTATION ================================*/

static 
volatile bool s_bTimerEvent = false;

void SysTick_Handler(void)
{
    //! every 5ms 
    s_bTimerEvent = true;
    
    
    /*! \note please do not put following code here
     *!
     *!  lv_tick_inc(5);
     *!  
     *!     Use a custom tick source that tells the elapsed time in milliseconds.
     *!     It removes the need to manually update the tick with `lv_tick_inc()`)
     *!     #define LV_TICK_CUSTOM 1
     *!     #if LV_TICK_CUSTOM
     *!         extern uint32_t SystemCoreClock;
     *!         #define LV_TICK_CUSTOM_INCLUDE "perf_counter.h"  
     *!         #define LV_TICK_CUSTOM_SYS_TIME_EXPR                            \
     *!                        (get_system_ticks() / (SystemCoreClock / 1000ul))
     *!     #endif
     */
}

static void test(void)
{
    lv_obj_t * btn = lv_btn_create(lv_scr_act());                   /*Add a button to the current screen*/
    lv_obj_set_pos(btn, 10, 10);                                    /*Set its position*/
    lv_obj_set_size(btn, 100, 50);                                  /*Set its size*/
    //lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL); /*Assign a callback to the button*/

    lv_obj_t * label = lv_label_create(btn);                        /*Add a label to the button*/
    lv_label_set_text(label, "Button");                             /*Set the labels text*/
    lv_obj_center(label);                                           /*Align the label to the center*/
}

int main(void)
{
    printf("Hello LVGL!!\r\n");
    
    __cycleof__("Draw strings on LCD") {
        __LL_LCD_PRINT_BANNER("Hello LVGL!!");
    }
    
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    
#if LV_USE_DEMO_BENCHMARK
    lv_demo_benchmark();
    
    //lv_demo_benchmark_run_scene(51);      // run scene no 31
#endif
    
#if LV_USE_DEMO_WIDGETS
    lv_demo_widgets();
#endif
    
    //test();
    
    
    while(1) {
        
        lv_timer_handler_run_in_period(1);
    }
    
}
