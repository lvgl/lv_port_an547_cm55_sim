/*
* Copyright (c) 2017-2020 Arm Limited
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <stdio.h>
#include <string.h>
#include "device_cfg.h"
#include "Driver_USART.h"
#include "serial.h"
#include <assert.h>

#include "RTE_Components.h"
#if defined(RTE_Compiler_EventRecorder) || defined(RTE_CMSIS_View_EventRecorder)
#   include <EventRecorder.h>
#endif

extern int stdout_putchar(int ch);

extern ARM_DRIVER_USART Driver_USART0;

void serial_init(void)
{
#if defined(RTE_Compiler_EventRecorder) || defined(RTE_CMSIS_View_EventRecorder)
    EventRecorderInitialize(0, 1);
#else
    Driver_USART0.Initialize(NULL);
    Driver_USART0.PowerControl(ARM_POWER_FULL);
    Driver_USART0.Control(ARM_USART_MODE_ASYNCHRONOUS, DEFAULT_UART_BAUDRATE);
#endif
}

void serial_print(char *str)
{
    (void)Driver_USART0.Send(str, strlen(str));
}




/* Redirects gcc printf to UART0 */
int _write(int fd, char *str, int len)
{
    if (Driver_USART0.Send(str, len) == ARM_DRIVER_OK) {
        return len;
    }
    return 0;
}

#if defined(RTE_Compiler_EventRecorder) && defined(USE_EVR_FOR_STDOUR)
int stdout_putchar (int ch) {
  static uint32_t index = 0U;
  static uint8_t  buffer[8];
 
  assert(index < sizeof(buffer));

  buffer[index++] = (uint8_t)ch;
  if ((index == sizeof(buffer)) || (ch == '\n')) {
    EventRecordData(EventID(EventLevelOp, 0xFE, 0x00), buffer, index);
    index = 0U;
  }
  return (ch);
}
#elif !defined(RTE_CMSIS_View_EventRecorder) && defined(RTE_CMSIS_Compiler_STDOUT_Custom)
int stdout_putchar(int ch)
{
    if ('\n' == ch) {
        int temp = '\r';
        while(Driver_USART0.Send(&temp, 1) != ARM_DRIVER_OK);
    }
    
    if (Driver_USART0.Send(&ch, 1) == ARM_DRIVER_OK) {
        return ch;
    }
    
    return -1;
}
#endif

/* Redirects armclang printf to UART */
int fputc(int ch, FILE *f)
{
    return stdout_putchar(ch);
}
