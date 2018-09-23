#ifndef __DISP_DRV_PLATFORM_H__
#define __DISP_DRV_PLATFORM_H__

//#include <linux/dma-mapping.h>
#include "mt_typedefs.h"
#include "mt_gpio.h"
#include "sync_write.h"
//#include "disp_drv_log.h"

#ifdef OUTREG32
#undef OUTREG32
#define OUTREG32(x, y) mt65xx_reg_sync_writel(y, x)
#endif

#ifndef OUTREGBIT
#define OUTREGBIT(TYPE,REG,bit,value)  \
                    do {    \
                        TYPE r = *((TYPE*)&INREG32(&REG));    \
                        r.bit = value;    \
                        OUTREG32(&REG, AS_UINT32(&r));    \
                    } while (0)
#endif

///LCD HW feature options for MT6575
#define MTK_LCD_HW_SIF_VERSION      2       ///for MT6575, we naming it is V2 because MT6516/73 is V1...
#define MTKFB_NO_M4U
#define MT65XX_NEW_DISP
//#define MTK_LCD_HW_3D_SUPPORT
#define ALIGN_TO(x, n)  \
    (((x) + ((n) - 1)) & ~((n) - 1))
#define MTK_FB_ALIGNMENT 32
#define MTK_FB_START_DSI_ISR

#define DFO_USE_NEW_API
#define MTKFB_FPGA_ONLY
#if defined(MTK_ALPS_BOX_SUPPORT)
#define HDMI_MAIN_PATH_LK 1
#define HDMI_SUB_PATH_LK 0
#define DISABLE_LCM_INIT 0
#define DISABLE_LCM_INIT_V1 0
#else
#define HDMI_MAIN_PATH_LK 0
#define HDMI_SUB_PATH_LK 0
#define DISABLE_LCM_INIT 0
#define DISABLE_LCM_INIT_V1 0
#endif

#define BOOT_RES_480P  0
#define BOOT_RES_720P  1
#define BOOT_RES_1080P  2
#define BOOT_RES_2160P  3
#define BOOT_RES_2161P  4

//#define BOOT_RES  BOOT_RES_1080P
//#define BOOT_RES  BOOT_RES_720P
#define BOOT_RES  BOOT_RES_1080P

#if BOOT_RES == BOOT_RES_480P
#define HDMI_DISP_WIDTH_LK 720
#define HDMI_DISP_HEIGHT_LK 480
#define BOX_FIX_RES 0x0

#elif BOOT_RES == BOOT_RES_720P
#define HDMI_DISP_WIDTH_LK 1280
#define HDMI_DISP_HEIGHT_LK 720
#define BOX_FIX_RES 0x2

#elif BOOT_RES == BOOT_RES_1080P
#define HDMI_DISP_WIDTH_LK 1920
#define HDMI_DISP_HEIGHT_LK 1080
#define BOX_FIX_RES 0xb

#elif BOOT_RES == BOOT_RES_2160P
#define HDMI_DISP_WIDTH_LK 3840
#define HDMI_DISP_HEIGHT_LK 2160
#define BOX_FIX_RES 0x17

#elif BOOT_RES == BOOT_RES_2161P
#define HDMI_DISP_WIDTH_LK 4096
#define HDMI_DISP_HEIGHT_LK 2160
#define BOX_FIX_RES 0x19
#endif


#endif //__DISP_DRV_PLATFORM_H__
