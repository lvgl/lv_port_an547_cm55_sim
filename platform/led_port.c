/*
 * Copyright (c) 2019-2020 Arm Limited. All rights reserved.
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

#include <stdint.h>
#include "led_port.h"
#include "arm_mps3_io_drv.h"
#include "device_cfg.h"
#include "device_definition.h"

#define MAX_PIN_FPGAIO_LED         10UL
#define MAX_LED_MASK               ((1U << MAX_PIN_FPGAIO_LED) - 1)
#define BIT_VALUE(v, bit)          (((v) >> (bit) & 1UL))

static unsigned int led_status;

unsigned int get_led_port_bit_length(void)
{
    return MAX_PIN_FPGAIO_LED;
}

void led_port_init(void)
{
    /* Turn off all LEDs at init */
    arm_mps3_io_write_leds(&MPS3_IO_DEV, ARM_MPS3_IO_ACCESS_PORT, 0, 0);
    led_status = 0x0;
}

unsigned int set_led_port(unsigned int led_mask)
{
    uint8_t i;
    uint32_t value;

    if (led_mask > MAX_LED_MASK) {
        return 1;
    }

    /* Set the LEDs to new value one by one */
    for (i = 0; i < MAX_PIN_FPGAIO_LED; i++) {
        if (BIT_VALUE((led_mask ^ led_status), i)) {
            /* led pin i has new value */
            value = BIT_VALUE(led_mask, i);
            arm_mps3_io_write_leds(&MPS3_IO_DEV,
                                   ARM_MPS3_IO_ACCESS_PIN,
                                   i, value);
        }
    }
    led_status = led_mask;

    return 0;
}

/**
 * \brief Get the value of the LED port.
 *
 * \return Value of the LED port
 *         Every bit represents one physical LED.
 */
unsigned int get_led_port(void)
{
    return arm_mps3_io_read_leds(&MPS3_IO_DEV,
                                 ARM_MPS3_IO_ACCESS_PORT,
                                 0);
}
