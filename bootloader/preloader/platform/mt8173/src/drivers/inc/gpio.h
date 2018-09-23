/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 * 
 * MediaTek Inc. (C) 2010. All rights reserved.
 * 
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

/******************************************************************************
 * mt6516_gpio.h - The file is the gpio header file !
 * 
 * Copyright 2008-2009 MediaTek Co.,Ltd.
 * 
 * DESCRIPTION: - 
 *     This file expose an in-kernel gpio modules to other device drivers
 *
 * modification history
 * ----------------------------------------
 * 01a, 08 oct 2008, Koshi,Chiu written
 * ----------------------------------------
 ******************************************************************************/
#ifndef _GPIO_H_
#define _GPIO_H_

#include <cust_gpio_usage.h>

#define PRELOADER_SUPPORT_EXT_GPIO 1

/*----------------------------------------------------------------------------*/
//  Error Code No.
#define RSUCCESS        0
#define ERACCESS        1
#define ERINVAL         2
#define ERWRAPPER		3
/*----------------------------------------------------------------------------*/
/******************************************************************************
* Enumeration for GPIO pin
******************************************************************************/
typedef enum GPIO_PIN
{    
    GPIO_UNSUPPORTED = -1,    
        
    GPIO0  , GPIO1  , GPIO2  , GPIO3  , GPIO4  , GPIO5  , GPIO6  , GPIO7  ,
    GPIO8  , GPIO9  , GPIO10 , GPIO11 , GPIO12 , GPIO13 , GPIO14 , GPIO15 ,
    GPIO16 , GPIO17 , GPIO18 , GPIO19 , GPIO20 , GPIO21 , GPIO22 , GPIO23 ,
    GPIO24 , GPIO25 , GPIO26 , GPIO27 , GPIO28 , GPIO29 , GPIO30 , GPIO31 ,
    GPIO32 , GPIO33 , GPIO34 , GPIO35 , GPIO36 , GPIO37 , GPIO38 , GPIO39 ,
    GPIO40 , GPIO41 , GPIO42 , GPIO43 , GPIO44 , GPIO45 , GPIO46 , GPIO47 ,
    GPIO48 , GPIO49 , GPIO50 , GPIO51 , GPIO52 , GPIO53 , GPIO54 , GPIO55 ,
    GPIO56 , GPIO57 , GPIO58 , GPIO59 , GPIO60 , GPIO61 , GPIO62 , GPIO63 ,
    GPIO64 , GPIO65 , GPIO66 , GPIO67 , GPIO68 , GPIO69 , GPIO70 , GPIO71 ,
    GPIO72 , GPIO73 , GPIO74 , GPIO75 , GPIO76 , GPIO77 , GPIO78 , GPIO79 ,
    GPIO80 , GPIO81 , GPIO82 , GPIO83 , GPIO84 , GPIO85 , GPIO86 , GPIO87 ,
    GPIO88 , GPIO89 , GPIO90 , GPIO91 , GPIO92 , GPIO93 , GPIO94 , GPIO95 ,
    GPIO96 , GPIO97 , GPIO98 , GPIO99 , GPIO100, GPIO101, GPIO102, GPIO103,
    GPIO104, GPIO105, GPIO106, GPIO107, GPIO108, GPIO109, GPIO110, GPIO111,
    GPIO112, GPIO113, GPIO114, GPIO115, GPIO116, GPIO117, GPIO118, GPIO119,
    GPIO120, GPIO121, GPIO122, GPIO123, GPIO124, GPIO125, GPIO126, GPIO127,
    GPIO128, GPIO129, GPIO130, GPIO131, GPIO132, GPIO133, GPIO134, 
#if PRELOADER_SUPPORT_EXT_GPIO    
    GPIOEXT0, GPIOEXT1, GPIOEXT2, GPIOEXT3, GPIOEXT4, GPIOEXT5, GPIOEXT6, GPIOEXT7, 
    GPIOEXT8, GPIOEXT9, GPIOEXT10, GPIOEXT11, GPIOEXT12, GPIOEXT13, GPIOEXT14, GPIOEXT15,   
    GPIOEXT16, GPIOEXT17, GPIOEXT18, GPIOEXT19, GPIOEXT20, GPIOEXT21, GPIOEXT22, GPIOEXT23, 
    GPIOEXT24, GPIOEXT25, GPIOEXT26, GPIOEXT27, GPIOEXT28, GPIOEXT29, GPIOEXT30, GPIOEXT31, 
    GPIOEXT32, GPIOEXT33, GPIOEXT34, GPIOEXT35, GPIOEXT36, GPIOEXT37, GPIOEXT38, GPIOEXT39,
    GPIOEXT40, 
#endif
    GPIO_MAX
}GPIO_PIN;         

#define MAX_GPIO_PIN    (GPIO_MAX)

#if PRELOADER_SUPPORT_EXT_GPIO    
#define GPIO_EXTEND_START GPIOEXT0
#if 0//mark to avoid other module set gpio incorrectly, find problem in build phase
typedef enum GPIO_PIN_EXT
{    
    GPIO135 = GPIO_EXTEND_START,  
	GPIO136, GPIO137, GPIO138, GPIO139, 
	GPIO140, GPIO141, GPIO142, GPIO143, GPIO144, 
	GPIO145, GPIO146, GPIO147, GPIO148, GPIO149, 
	GPIO150, GPIO151, GPIO152, GPIO153, GPIO154,
	GPIO155, GPIO156, GPIO157, GPIO158, GPIO159, 
	GPIO160, GPIO161, GPIO162, GPIO163, GPIO164, 
    GPIO165, GPIO166, GPIO167, GPIO168, GPIO169,
    GPIO170, GPIO171, GPIO172, GPIO173, GPIO174,
    GPIO175
}GPIO_PIN_EXT;
#endif
#endif

/******************************************************************************
* Enumeration for Clock output
******************************************************************************/
/* GPIO MODE CONTROL VALUE*/
typedef enum {
    GPIO_MODE_UNSUPPORTED = -1,
    GPIO_MODE_GPIO  = 0,
    GPIO_MODE_00    = 0,
    GPIO_MODE_01    = 1,
    GPIO_MODE_02    = 2,
    GPIO_MODE_03    = 3,
    GPIO_MODE_04    = 4,
    GPIO_MODE_05    = 5,
    GPIO_MODE_06    = 6,
    GPIO_MODE_07    = 7,

    GPIO_MODE_MAX,
    GPIO_MODE_DEFAULT = GPIO_MODE_01,
} GPIO_MODE;
/*----------------------------------------------------------------------------*/
/* GPIO DIRECTION */
typedef enum {
    GPIO_DIR_UNSUPPORTED = -1,
    GPIO_DIR_IN     = 0,
    GPIO_DIR_OUT    = 1,

    GPIO_DIR_MAX,
    GPIO_DIR_DEFAULT = GPIO_DIR_IN,
} GPIO_DIR;
/*----------------------------------------------------------------------------*/
/* GPIO PULL ENABLE*/
typedef enum {
    GPIO_PULL_EN_UNSUPPORTED = -1,
    GPIO_PULL_DISABLE = 0,
    GPIO_PULL_ENABLE  = 1,

    GPIO_PULL_EN_MAX,
    GPIO_PULL_EN_DEFAULT = GPIO_PULL_ENABLE,
} GPIO_PULL_EN;
/*----------------------------------------------------------------------------*/
/* GPIO PULL-UP/PULL-DOWN*/
typedef enum {
    GPIO_PULL_UNSUPPORTED = -1,
    GPIO_PULL_DOWN  = 0,
    GPIO_PULL_UP    = 1,

    GPIO_PULL_MAX,
    GPIO_PULL_DEFAULT = GPIO_PULL_DOWN
} GPIO_PULL;
/*----------------------------------------------------------------------------*/
typedef enum {
    GPIO_DATA_INV_UNSUPPORTED = -1,
    GPIO_DATA_UNINV = 0,
    GPIO_DATA_INV   = 1,

    GPIO_DATA_INV_MAX,
    GPIO_DATA_INV_DEFAULT = GPIO_DATA_UNINV
} GPIO_INVERSION;
/*----------------------------------------------------------------------------*/
/* GPIO OUTPUT */
typedef enum {
    GPIO_OUT_UNSUPPORTED = -1,
    GPIO_OUT_ZERO = 0,
    GPIO_OUT_ONE  = 1,

    GPIO_OUT_MAX,
    GPIO_OUT_DEFAULT = GPIO_OUT_ZERO,
    GPIO_DATA_OUT_DEFAULT = GPIO_OUT_ZERO,  /*compatible with DCT*/
} GPIO_OUT;
/* GPIO INPUT */
typedef enum {
    GPIO_IN_UNSUPPORTED = -1,
    GPIO_IN_ZERO = 0,
    GPIO_IN_ONE  = 1,

    GPIO_IN_MAX,
} GPIO_IN;
    
/* GPIO POWER*/
typedef enum {
    GPIO_VIO28 = 0,
    GPIO_VIO18 = 1,

    GPIO_VIO_MAX,
} GPIO_POWER;
/*----------------------------------------------------------------------------*/
typedef struct {    
    u16 val;        
    u16 _align1;
    u16 set;
    u16 _align2;
    u16 rst;
    u16 _align3[3];
} VAL_REGS;
/*----------------------------------------------------------------------------*/
typedef struct {
	VAL_REGS dir[9];	        /*0x0000 ~ 0x008F: 144 bytes */
	unsigned char rsv00[112];	/*0x0090 ~ 0x00FF: 112 bytes */
	VAL_REGS pullen[9];	        /*0x0100 ~ 0x018F: 144 bytes */
	unsigned char rsv01[112];	/*0x0190 ~ 0x01FF: 112 bytes */
	VAL_REGS pullsel[9];	    /*0x0200 ~ 0x028F: 144 bytes */
	unsigned char rsv02[112];	/*0x0290 ~ 0x02FF: 112 bytes */
	unsigned char rsv03[256];	/*0x0300 ~ 0x03FF: 256 bytes */
	VAL_REGS dout[9];	        /*0x0400 ~ 0x048F: 144 bytes */
	unsigned char rsv04[112];	/*0x0490 ~ 0x04FF: 112 bytes */
	VAL_REGS din[9];	        /*0x0500 ~ 0x058F: 114 bytes */
	unsigned char rsv05[112];	/*0x0590 ~ 0x05FF: 112 bytes */
	VAL_REGS mode[27];	        /*0x0600 ~ 0x07AF: 432 bytes */
	unsigned char rsv06[336];	/*0x07B0 ~ 0x08FF: 336 bytes */
	VAL_REGS ies[3];	        /*0x0900 ~ 0x092F:  48 bytes */
	VAL_REGS smt[3];	        /*0x0930 ~ 0x095F:  48 bytes */
	unsigned char rsv07[160];	/*0x0960 ~ 0x09FF: 160 bytes */
	VAL_REGS tdsel[8];	        /*0x0A00 ~ 0x0A7F: 128 bytes */
	VAL_REGS rdsel[6];	        /*0x0A80 ~ 0x0ADF:  96 bytes */
	unsigned char rsv08[32];	/*0x0AE0 ~ 0x0AFF:  32 bytes */
    
	VAL_REGS drv_mode[10];	    /*0x0B00 ~ 0x0B9F: 160 bytes */
	unsigned char rsv09[96];	/*0x0BA0 ~ 0x0BFF:  96 bytes */
	VAL_REGS msdc0_ctrl0;	    /*0x0C00 ~ 0x0C0F:  16 bytes */
	VAL_REGS msdc0_ctrl1;		/*0x0C10 ~ 0x0C1F:  16 bytes */
	VAL_REGS msdc0_ctrl2;		/*0x0C20 ~ 0x0C2F:  16 bytes */
	VAL_REGS msdc0_ctrl5;		/*0x0C30 ~ 0x0C3F:  16 bytes */
	VAL_REGS msdc1_ctrl0;		/*0x0C40 ~ 0x0C4F:  16 bytes */
	VAL_REGS msdc1_ctrl1;		/*0x0C50 ~ 0x0C5F:  16 bytes */
	VAL_REGS msdc1_ctrl2;		/*0x0C60 ~ 0x0C6F:  16 bytes */
	VAL_REGS msdc1_ctrl5;		/*0x0C70 ~ 0x0C7F:  16 bytes */
	VAL_REGS msdc2_ctrl0;		/*0x0C80 ~ 0x0C8F:  16 bytes */
	VAL_REGS msdc2_ctrl1;		/*0x0C90 ~ 0x0C9F:  16 bytes */
	VAL_REGS msdc2_ctrl2;		/*0x0CA0 ~ 0x0CAF:  16 bytes */
	VAL_REGS msdc2_ctrl5;		/*0x0CB0 ~ 0x0CBF:  16 bytes */
	VAL_REGS msdc3_ctrl0;		/*0x0CC0 ~ 0x0CCF:  16 bytes */
	VAL_REGS msdc3_ctrl1;		/*0x0CD0 ~ 0x0CDF:  16 bytes */
	VAL_REGS msdc3_ctrl2;		/*0x0CE0 ~ 0x0CEF:  16 bytes */
	VAL_REGS msdc3_ctrl5;		/*0x0CF0 ~ 0x0CFF:  16 bytes */
	VAL_REGS msdc0_ctrl3;		/*0x0D00 ~ 0x0D0F:  16 bytes */
	VAL_REGS msdc0_ctrl4;		/*0x0D10 ~ 0x0D1F:  16 bytes */
	VAL_REGS msdc1_ctrl3;		/*0x0D20 ~ 0x0D2F:  16 bytes */
	VAL_REGS msdc1_ctrl4;		/*0x0D30 ~ 0x0D3F:  16 bytes */
	VAL_REGS msdc2_ctrl3;		/*0x0D40 ~ 0x0D4F:  16 bytes */
	VAL_REGS msdc2_ctrl4;		/*0x0D50 ~ 0x0D5F:  16 bytes */
	VAL_REGS msdc3_ctrl3;		/*0x0D60 ~ 0x0D6F:  16 bytes */
	VAL_REGS msdc3_ctrl4;		/*0x0D70 ~ 0x0D7F:  16 bytes */
	unsigned char rsv10[64];	/*0x0D80 ~ 0x0DBF:  64 bytes */
	VAL_REGS exmd_ctrl[1];		/*0x0DC0 ~ 0x0DCF:  16 bytes */
	unsigned char rsv11[48];	/*0x0DD0 ~ 0x0DFF:  48 bytes */
	VAL_REGS kpad_ctrl[2];		/*0x0E00 ~ 0x0E1F:  32 bytes */
	VAL_REGS hsic_ctrl[4];		/*0x0E20 ~ 0x0E5F:  64 bytes */
} GPIO_REGS;
/*----------------------------------------------------------------------------*/
#if PRELOADER_SUPPORT_EXT_GPIO
typedef struct {
    u16 val;
    u16 set;
    u16 rst;
    u16 _align;
} EXT_VAL_REGS;
/*----------------------------------------------------------------------------*/
typedef struct {
    EXT_VAL_REGS    dir[4];            /*0x0000 ~ 0x001F: 32 bytes*/
    EXT_VAL_REGS    pullen[4];         /*0x0020 ~ 0x003F: 32 bytes*/
    EXT_VAL_REGS    pullsel[4];        /*0x0040 ~ 0x005F: 32 bytes*/
    EXT_VAL_REGS    dinv[4];           /*0x0060 ~ 0x007F: 32 bytes*/
    EXT_VAL_REGS    dout[4];           /*0x0080 ~ 0x009F: 32 bytes*/
    EXT_VAL_REGS    din[4];            /*0x00A0 ~ 0x00BF: 32 bytes*/
    EXT_VAL_REGS    mode[10];          /*0x00C0 ~ 0x010F: 80 bytes*/
} GPIOEXT_REGS;
#endif
/*----------------------------------------------------------------------------*/
typedef struct {
    unsigned int no     : 16;
    unsigned int mode   : 3;    
    unsigned int pullsel: 1;
    unsigned int din    : 1;
    unsigned int dout   : 1;
    unsigned int pullen : 1;
    unsigned int dir    : 1;
    unsigned int dinv   : 1;
    unsigned int _align : 7; 
} GPIO_CFG; 
/******************************************************************************
* GPIO Driver interface 
******************************************************************************/
/*direction*/
s32 mt_set_gpio_dir(u32 pin, u32 dir);
s32 mt_get_gpio_dir(u32 pin);

/*pull enable*/
s32 mt_set_gpio_pull_enable(u32 pin, u32 enable);
s32 mt_get_gpio_pull_enable(u32 pin);
/*pull select*/
s32 mt_set_gpio_pull_select(u32 pin, u32 select);    
s32 mt_get_gpio_pull_select(u32 pin);

/*input/output*/
s32 mt_set_gpio_out(u32 pin, u32 output);
s32 mt_get_gpio_out(u32 pin);
s32 mt_get_gpio_in(u32 pin);

/*mode control*/
s32 mt_set_gpio_mode(u32 pin, u32 mode);
s32 mt_get_gpio_mode(u32 pin);

void mt_gpio_init(void);
#endif
