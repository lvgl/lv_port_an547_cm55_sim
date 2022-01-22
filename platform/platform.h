/****************************************************************************
*  Copyright 2022 Gorgon Meducer (Email:embedded_zhuoran@hotmail.com)       *
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
#include "perf_counter.h"
#include "RTE_Components.h"
#include "led_port.h"
#include "serial.h"
#include "Board_GLCD.h"
#include "GLCD_Config.h"

/*============================ MACROS ========================================*/
#ifndef UNUSED_PARAM
#   define UNUSED_PARAM(__VAR)      do {__VAR = __VAR; } while(0)
#endif

/*============================ MACROFIED FUNCTIONS ===========================*/

#define __LL_LCD_PRINT_BANNER(__STR)                                            \
        do {                                                                    \
            GLCD_DrawString(    (GLCD_HEIGHT) / 2 - 8,                          \
                                (GLCD_WIDTH - sizeof(__STR) * 6) / 2,           \
                                __STR);                                         \
        } while(0)

/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/

extern GLCD_FONT GLCD_Font_6x8;
extern GLCD_FONT GLCD_Font_16x24;

/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/








