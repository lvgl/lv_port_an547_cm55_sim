# lv_port_mps3_an547_cm55
A LVGL porting for Cortex-M55 running on an [Arm official FPGA prototyping development board](https://developer.arm.com/tools-and-software/development-boards/fpga-prototyping-boards/mps3) called MPS3 ([AN547](https://developer.arm.com/tools-and-software/development-boards/fpga-prototyping-boards/download-fpga-images#)), see **Figure 1**. It is also possible to run the project template on an emulator called [Corstone-300-FVP](https://developer.arm.com/tools-and-software/open-source-software/arm-platforms-software/arm-ecosystem-fvps), which is free.



**Figure 1 Arm MPS3 FPGA prototyping board** 

![](https://developer.arm.com/-/media/Arm%20Developer%20Community/Images/Development%20Boards/Arm_MPS3_prototyping_system.png?revision=79a108aa-abe8-4a21-b453-ad6c2d6e82b2&h=409&w=400&hash=F7DEA4E2D2C854ECAC5BBCC8804D6107) 



## Features

- LVGL 8.2.0  (CMSIS-Pack)
- [**New**] Arm-2D (Helium) acceleration is added !!!
  - see `lv_gpu_arm2d.c` and `lv_gpu_arm2d.h`
  - Attach acceleration in `lv_port_disp_template.c`
  - [Arm-2D CMSIS-Pack](https://github.com/ARM-software/EndpointAI/tree/main-arm-2d-developing/Kernels/Research/Arm-2D/cmsis-pack)

- 320 * 240 RGB565 LCD Display connected with an Integrated Color LCD parallel interface. 
- Default System Clock: 32MHz (50MHz Max)
- CPU: Cortex-M55 with Helium
  - DCache
  - ICache
  - ITCM: 512KB
  - DTCM: 512KB
- NPU: Ethos-U55
- PRAM (Used for Code and RO-Data): 2MByte
- SRAM: 4MByte
- DDR4: 1GByte
- MDK Project with Arm Compiler 6



## Acceleration Using Arm-2D

[A benchmark report](./lvgl_benchmark.md) shows the advantage of using arm-2d to accelerate LVGL when Helium technology is available. 



## How To Use

### How to run the simulation

1. Download the [Corstone-300-FVP](https://developer.arm.com/tools-and-software/open-source-software/arm-platforms-software/arm-ecosystem-fvps) and install it.
2. Open the project in MDK
3. Select the '**Cortex-M55_FVP**' configuration.

![](./documents/pictures/configuration_selection) 

4. Open the 'Options for Target' window and select the 'Debug' panel

![](./documents/pictures/debug_panel)

5.  Press the 'Settings' button on the right corner

![](./documents/pictures/Model_Settings)

6.  Click the '...' button on the top-right corner, browse to the folder where Corstone-300-FVP is installed and select the executable file, i.e. FVP_Corstone_SSE-300_EThos-U55.exe. 

![](./documents/pictures/FVP)

 Usually the Corstone-300-FVP is located in the following path:

```
C:\Program Files\ARM\FVP_Corstone_SSE-300\models\Win64_VC2017\FVP_Corstone_SSE-300_Ethos-U55.exe
```

7. Click the '...' button next to the 'Target' textbox and select the cpu0 in the popup dialog. Click OK to confirm.

![](./documents/pictures/select_cpu0)

8. Click the OK button to close the 'Models Armv8-M Target Driver Setup' window.
9. Compile and Debug. You should be able to see the LVGL benchmark running on a FVP window as shown below:

![](./documents/pictures/fvp_demo) 

### NOTE: 

1. FVP can **ONLY** be used to verify the correctness of firmware functionality

2. **NO** performance data generated from FVP is trustworthy. FVP is simply **NOT** designed for performance evaluation. 

   For example, the '-O0' optimization might run as fast as -Ofast, in contrast, the '-Ofast+LTO (Link-Time-Optimization)' runs very slow.  In fact, in the real hardware, -Ofast+LTO > -Ofast >> -O0. 



### How to Run on MPS3

1. Open the project in MDK
2. Select the '**Cortex-M55_MPS3**' configuration
3. Compile and Debug



## LVGL Porting Exercise 

For people who want to learn and practice porting LVGL to a LCD-Ready MDK project using the [LVGL cmsis-pack](https://github.com/lvgl/lvgl/tree/master/env_support/cmsis-pack), a dedicated branch called "***lvgl_porting_exercise***" is introduced, which contains a clean project and provides:

- Low-level LCD APIs

```c
extern int32_t  GLCD_Initialize          (void);
extern int32_t  GLCD_Uninitialize        (void);
extern int32_t  GLCD_SetForegroundColor  (uint32_t color);
extern int32_t  GLCD_SetBackgroundColor  (uint32_t color);
extern int32_t  GLCD_ClearScreen         (void);
extern int32_t  GLCD_SetFont             (GLCD_FONT *font);
extern int32_t  GLCD_DrawPixel           (uint32_t x, uint32_t y);
extern int32_t  GLCD_DrawHLine           (uint32_t x, uint32_t y, uint32_t length);
extern int32_t  GLCD_DrawVLine           (uint32_t x, uint32_t y, uint32_t length);
extern int32_t  GLCD_DrawRectangle       (uint32_t x, uint32_t y, uint32_t width, uint32_t height);
extern int32_t  GLCD_DrawChar            (uint32_t x, uint32_t y, int32_t  ch);
extern int32_t  GLCD_DrawString          (uint32_t x, uint32_t y, const char *str);
extern int32_t  GLCD_DrawBitmap          (uint32_t x, uint32_t y, 
                                          uint32_t width, uint32_t height, const uint8_t *bitmap);
```



- A simple ***main()*** function

- A simple way to display string on LCD

```c
extern 
int32_t GLCD_DrawString(uint32_t x, uint32_t y, const char *str);

#define __LL_LCD_PRINT_BANNER(__STR)                                            \
        do {                                                                    \
            GLCD_DrawString(    (GLCD_HEIGHT) / 2 - 8,                          \
                                (GLCD_WIDTH - sizeof(__STR) * 6) / 2,           \
                                __STR);                                         \
        } while(0)
```



- **printf()** is retargeted to USART0 (telnet in FVP emulation).
- A function, ***GLCD_DrawBitmap()***,  for flushing display buffer to LCD, which supports window-mode (partial flush).

```c
extern 
int32_t  GLCD_DrawBitmap (uint32_t x, uint32_t y, 
                          uint32_t width, uint32_t height, 
                          const uint8_t *bitmap);
```

- Ready to Compile and Debug in FVP (Emulation) as shown in Figure 2.



**Figure 2 A Clean Project Template for practicing LVGL Porting using LVGL CMSIS-Pack** 

![image-20220117225044592](./documents/pictures/fvp_clean) 

- A new method, ***\_\_cycleof\_\_()***, for measuring cpu cycles consumed by specified code segment.

```c
    __cycleof__("Draw strings on LCD") {
        __LL_LCD_PRINT_BANNER("Hello LVGL!!");
    }
```



**Figure 3 *printf()* and *\_\_cycleof\_\_***

![image-20220117231318232](./documents/pictures/telnet_clean.png) 

## License

- LVGL used in this project is under MIT license.
- This project template is under Apache 2.0 license.
- Arm-2D used in this project is under Apache 2.0 license.



## Reference

1. [Arm Corstone SSE-300 with Cortex-M55 and Ethos-U55 Example Subsystem for MPS3](https://developer.arm.com/documentation/dai0547/c?_ga=2.157798205.688811587.1624957483-616249991.1623083451)



