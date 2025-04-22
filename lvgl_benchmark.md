# LVGL Benchmark

## 1 Overview

This is a benchmark report for comparing performance differences between LVGL baseline and LVGL plus arm-2d acceleration (taking advantage of Helium technology). 

**The result truly shows the advantage of using arm-2d to accelerate LVGL when Helium technology is available** but doesn't not reveal the right performance ratio between a C verison and a Helium accelerated version for a given 2D image processing algorithm. 

### 1.1 Test Environment

#### 1.1.1 Hardware

- MPS3
- Processor: **Cortex-M55 (r1p0)**
- System Frequency: **32MHz**
- LCD: 320*240 RGB565 8080 interface 

#### 1.1.2 Software

- IDE: [**MDK** **5.42** (Community Edition)](https://www.keil.com/pr/article/1299.htm)
- Compiler: **Arm Compiler 6.24 (built-in compiler in MDK)** with standard c library
- LVGL: **v9.3.0 (cmsis-pack v9.3.0)**
- Arm-2D: **v1.2.2-rc3**

#### 1.1.3 Configuration

- LVGL (configured in `lv_conf_cmsis.h`)

  - set `LV_COLOR_DEPTH` to `16`
  - set `LV_MEM_SIZE` to `(128U * 1024U)`
  - set `LV_DISP_DEF_REFR_PERIOD` to `2`
  
  
  
- Arm-2D (configured in `arm_2d_cfg.h`)
  - set `__ARM_2D_HAS_ASYNC__`to `0`
  - set `__ARM_2D_HAS_ANTI_ALIAS_TRANSFORM__` to `1`
  - set `__ARM_2D_CFG_SUPPORT_COLOUR_CHANNEL_ACCESS__` to `1`



## 2 Result



**Table 2-1 LVGL Benchmark (baseline vs arm-2d acceleration)**

| Name                      | Scaler | Helium | improve     |
| ------------------------- | ------ | ------ | ----------- |
| Empty screen              | 33     | 27     | 22.22%      |
| Moving wallpaper          | 60     | 44     | 36.36%      |
| Single rectangle          | 3      | 3      | 0.00%       |
| Multiple rectangles       | 29     | 26     | 11.54%      |
| Multiple RGB images       | 10     | 7      | **42.86%**  |
| Multiple ARGB images      | 27     | 12     | **125.00%** |
| Rotated ARGB images       | 172    | 94     | **82.98%**  |
| Multiple labels           | 39     | 35     | 11.43%      |
| Screen sized text         | 273    | 243    | 12.35%      |
| Multiple arcs             | 6      | 6      | 0.00%       |
| Containers                | 48     | 36     | 33.33%      |
| Containers with overlay   | 77     | 56     | 37.50%      |
| Containers with opa       | 73     | 53     | 37.74%      |
| Containers with opa_layer | 130    | 71     | **83.10%**  |
| Containers with scrolling | 104    | 82     | 26.83%      |
| Widgets demo              | 105    | 91     | 15.38%      |
| All scenes avg.           | 74     | 55     | **34.55%**  |



**Figure 2-1 Performance Ratio (Arm-2D acceleration over baseline)**

 ![](./documents/pictures/figure2-1)

**NOTE:** 

1. The unit of the histogram is millisecond (ms), which is lower the better. 