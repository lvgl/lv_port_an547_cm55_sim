
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

/* ARM::CMSIS Driver:USART@1.0.0 */
#define RTE_Drivers_USART
/* ARM::CMSIS-Compiler:STDOUT:Event Recorder@1.0.0 */
#define RTE_CMSIS_Compiler_STDOUT                /* CMSIS-Compiler STDOUT */
        #define RTE_CMSIS_Compiler_STDOUT_Event_Recorder /* CMSIS-Compiler STDOUT: Event Recorder */
/* ARM::CMSIS-View:Event Recorder&DAP@1.5.3 */
#define RTE_CMSIS_View_EventRecorder
        #define RTE_CMSIS_View_EventRecorder_DAP
/* GorgonMeducer::Utilities:perf_counter:Core&Source@2.3.0 */
#define __PERF_COUNTER__ 1


#endif /* RTE_COMPONENTS_H */
