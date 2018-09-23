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
 * mt6577_gpio.c - MT6577 Linux GPIO Device Driver
 * 
 * Copyright 2008-2009 MediaTek Co.,Ltd.
 * 
 * DESCRIPTION:
 *     This file provid the other drivers GPIO relative functions
 *
 ******************************************************************************/

#include <typedefs.h>
#include <gpio.h>
#include <platform.h>
//#include <mt_pmic_wrap_init.h>

//#include <cust_power.h>
/******************************************************************************
 MACRO Definition
******************************************************************************/
//#define  GIO_SLFTEST            
#define GPIO_DEVICE "mt-gpio"
#define VERSION     "$Revision$"
/*---------------------------------------------------------------------------*/
#define GPIO_WR32(addr, data)   __raw_writel(data, addr)
#define GPIO_RD32(addr)         __raw_readl(addr)
#define GPIO_SET_BITS(BIT,REG)   ((*(volatile u32*)(REG)) = (u32)(BIT))
#define GPIO_CLR_BITS(BIT,REG)   ((*(volatile u32*)(REG)) &= ~((u32)(BIT)))
//S32 pwrap_read( U32  adr, U32 *rdata ){return 0;}
//S32 pwrap_write( U32  adr, U32  wdata ){return 0;}
#if PRELOADER_SUPPORT_EXT_GPIO	
#define GPIOEXT_WR(addr, data)   pwrap_write((u32)addr, data)
#define GPIOEXT_RD(addr)         ({ \
		u32 ext_data; \
		int ret; \
		ret = pwrap_read((u32)addr,&ext_data); \
		(ret != 0)?-1:ext_data;})
#define GPIOEXT_SET_BITS(BIT,REG)   (GPIOEXT_WR(REG, (u32)(BIT)))
#define GPIOEXT_CLR_BITS(BIT,REG)    ({ \
		u32 ext_data; \
		int ret; \
		ret = GPIOEXT_RD(REG);\
		ext_data = ret;\
		(ret < 0)?-1:(GPIOEXT_WR(REG,ext_data & ~((u32)(BIT))))})
#define GPIOEXT_BASE        (0xC000) 			//PMIC GPIO base.
#define RG_USBDL_EN_CTL_REG  (0x502)
#endif
/*---------------------------------------------------------------------------*/
#define TRUE                   1
#define FALSE                  0
/*---------------------------------------------------------------------------*/
#define MAX_GPIO_REG_BITS      16
#define MAX_GPIO_MODE_PER_REG  5
#define GPIO_MODE_BITS         3 
/*---------------------------------------------------------------------------*/
#define GPIOTAG                "[GPIO] "
#define GPIOLOG(fmt, arg...)   //printf(GPIOTAG fmt, ##arg)
#define GPIOMSG(fmt, arg...)   //printf(fmt, ##arg)
#define GPIOERR(fmt, arg...)   //printf(GPIOTAG "%5d: "fmt, __LINE__, ##arg)
#define GPIOFUC(fmt, arg...)   //printf(GPIOTAG "%s\n", __FUNCTION__)
#define GIO_INVALID_OBJ(ptr)   ((ptr) != gpio_obj)
/******************************************************************************
Enumeration/Structure
******************************************************************************/
/*-------for special kpad pupd-----------*/
struct kpad_pupd {
	unsigned char 	pin;
	unsigned char   reg;
	unsigned char   bit;
};
static struct kpad_pupd kpad_pupd_spec[] = {
	{GPIO119,	0,	2},     //KROW0
	{GPIO120,	0,	6},     //KROW1
	{GPIO121,	0,	10},    //KROW2
	{GPIO122,	1,	2},     //KCOL0
	{GPIO123,	1,	6},     //KCOL1
	{GPIO124,	1,	10}     //KCOL2
};
struct mt_gpio_obj {
    GPIO_REGS       *reg;
};
static struct mt_gpio_obj gpio_dat = {
    .reg  = (GPIO_REGS*)(GPIO_BASE),
};
static struct mt_gpio_obj *gpio_obj = &gpio_dat;

#if PRELOADER_SUPPORT_EXT_GPIO
struct mt_gpioext_obj {
	GPIOEXT_REGS	*reg;
};

static struct mt_gpioext_obj gpioext_dat = {
	.reg = (GPIOEXT_REGS*)(GPIOEXT_BASE),
};
static struct mt_gpioext_obj *gpioext_obj = &gpioext_dat;
#endif

/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_dir_chip(u32 pin, u32 dir)
{
    u32 pos;
    u32 bit;
    struct mt_gpio_obj *obj = gpio_obj;

    if (!obj)
        return -ERACCESS;

#if PRELOADER_SUPPORT_EXT_GPIO
    if (pin >= GPIO_EXTEND_START)
        return -ERINVAL;
#else
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;
#endif

    if (dir >= GPIO_DIR_MAX)
        return -ERINVAL;
    
    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;
    
    if (dir == GPIO_DIR_IN)
        GPIO_SET_BITS((1L << bit), &obj->reg->dir[pos].rst);
    else
        GPIO_SET_BITS((1L << bit), &obj->reg->dir[pos].set);
    return RSUCCESS;
    
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_dir_chip(u32 pin)
{    
    u32 pos;
    u32 bit;
    u32 reg;
    struct mt_gpio_obj *obj = gpio_obj;

    if (!obj)
        return -ERACCESS;
    
#if PRELOADER_SUPPORT_EXT_GPIO
    if (pin >= GPIO_EXTEND_START)
        return -ERINVAL;
#else
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;
#endif
    
    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;
    
    reg = GPIO_RD32(&obj->reg->dir[pos].val);
    return (((reg & (1L << bit)) != 0)? 1: 0);        
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_pull_enable_chip(u32 pin, u32 enable)
{
    u32 pos;
    u32 bit;
    u32 reg;
    u32 mask;
    u32 i;
    struct mt_gpio_obj *obj = gpio_obj;

    if (!obj)
        return -ERACCESS;
    
#if PRELOADER_SUPPORT_EXT_GPIO
    if (pin >= GPIO_EXTEND_START)
        return -ERINVAL;
#else
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;
#endif

    if (enable >= GPIO_PULL_EN_MAX)
        return -ERINVAL;

	/*****************for special kpad pupd, NOTE DEFINITION REVERSE!!!*****************/
	for(i = 0; i < sizeof(kpad_pupd_spec)/sizeof(kpad_pupd_spec[0]); i++){
		if (pin == kpad_pupd_spec[i].pin){
			if (enable == GPIO_PULL_DISABLE){
				GPIO_SET_BITS((3L << (kpad_pupd_spec[i].bit-2)), &obj->reg->kpad_ctrl[kpad_pupd_spec[i].reg].rst);
			} else {
				GPIO_SET_BITS((1L << (kpad_pupd_spec[i].bit-2)), &obj->reg->kpad_ctrl[kpad_pupd_spec[i].reg].set);    //single key: 75K 
			}
			return RSUCCESS;
		}
	}

	/********************************* MSDC special *********************************/
	if (pin == GPIO67) {         //ms0 DS
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS(3L, &obj->reg->msdc0_ctrl4.rst) : GPIO_SET_BITS(1L, &obj->reg->msdc0_ctrl4.set);   //1L:10K
		return RSUCCESS;
	} else if (pin == GPIO68) {  //ms0 RST
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS(3L, &obj->reg->msdc0_ctrl3.rst) : GPIO_SET_BITS(1L, &obj->reg->msdc0_ctrl3.set);
		return RSUCCESS;
	} else if (pin == GPIO66) {  //ms0 cmd
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS(3L, &obj->reg->msdc0_ctrl1.rst) : GPIO_SET_BITS(1L, &obj->reg->msdc0_ctrl1.set);
		return RSUCCESS;
	} else if (pin == GPIO65) {  //ms0 clk
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS(3L, &obj->reg->msdc0_ctrl0.rst) : GPIO_SET_BITS(1L, &obj->reg->msdc0_ctrl0.set);
		return RSUCCESS;
	} else if ( (pin >= GPIO57) && (pin <= GPIO64) ) {  //ms0 data0~7
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS(3L, &obj->reg->msdc0_ctrl2.rst) : GPIO_SET_BITS(1L, &obj->reg->msdc0_ctrl2.set);
		return RSUCCESS;

	} else if (pin == GPIO78) {  //ms1 cmd
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS(3L, &obj->reg->msdc1_ctrl1.rst) : GPIO_SET_BITS(1L, &obj->reg->msdc1_ctrl1.set);
		return RSUCCESS;
	} else if (pin == GPIO73) {  //ms1 dat0
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS(3L, &obj->reg->msdc1_ctrl3.rst) : GPIO_SET_BITS(1L, &obj->reg->msdc1_ctrl3.set);
		return RSUCCESS;
	} else if (pin == GPIO74) {  //ms1 dat1
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS((3L << 4), &obj->reg->msdc1_ctrl3.rst) : GPIO_SET_BITS((1L << 4), &obj->reg->msdc1_ctrl3.set);
		return RSUCCESS;
	} else if (pin == GPIO75) {  //ms1 dat2
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS((3L << 8), &obj->reg->msdc1_ctrl3.rst) : GPIO_SET_BITS((1L << 8), &obj->reg->msdc1_ctrl3.set);
		return RSUCCESS;
	} else if (pin == GPIO76) {  //ms1 dat3
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS((3L << 12), &obj->reg->msdc1_ctrl3.rst) : GPIO_SET_BITS((1L << 12), &obj->reg->msdc1_ctrl3.set);
		return RSUCCESS;
	} else if (pin == GPIO77) {  //ms1 clk
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS(3L, &obj->reg->msdc1_ctrl0.rst) : GPIO_SET_BITS(1L, &obj->reg->msdc1_ctrl0.set);
		return RSUCCESS;

	} else if (pin == GPIO100) {  //ms2 dat0
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS(3L, &obj->reg->msdc2_ctrl3.rst) : GPIO_SET_BITS(1L, &obj->reg->msdc2_ctrl3.set);
		return RSUCCESS;
	} else if (pin == GPIO101) {  //ms2 dat1
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS((3L << 4), &obj->reg->msdc2_ctrl3.rst) : GPIO_SET_BITS((1L << 4), &obj->reg->msdc2_ctrl3.set);
		return RSUCCESS;
	} else if (pin == GPIO102) {  //ms2 dat2
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS((3L << 8), &obj->reg->msdc2_ctrl3.rst) : GPIO_SET_BITS((1L << 8), &obj->reg->msdc2_ctrl3.set);
		return RSUCCESS;
	} else if (pin == GPIO103) {  //ms2 dat3
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS((3L << 12), &obj->reg->msdc2_ctrl3.rst) : GPIO_SET_BITS((1L << 12), &obj->reg->msdc2_ctrl3.set);
		return RSUCCESS;
	} else if (pin == GPIO104) {  //ms2 clk 
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS(3L, &obj->reg->msdc2_ctrl0.rst) : GPIO_SET_BITS(1L, &obj->reg->msdc2_ctrl0.set);
		return RSUCCESS;
	} else if (pin == GPIO105) {  //ms2 cmd
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS(3L, &obj->reg->msdc2_ctrl1.rst) : GPIO_SET_BITS(1L, &obj->reg->msdc2_ctrl1.set);
		return RSUCCESS;

	} else if (pin == GPIO22) {  //ms3 dat0
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS(3L, &obj->reg->msdc3_ctrl3.rst) : GPIO_SET_BITS(1L, &obj->reg->msdc3_ctrl3.set);
		return RSUCCESS;
	} else if (pin == GPIO23) {  //ms3 dat1
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS((3L << 4), &obj->reg->msdc3_ctrl3.rst) : GPIO_SET_BITS((1L << 4), &obj->reg->msdc3_ctrl3.set);
		return RSUCCESS;
	} else if (pin == GPIO24) {  //ms3 dat2
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS((3L << 8), &obj->reg->msdc3_ctrl3.rst) : GPIO_SET_BITS((1L << 8), &obj->reg->msdc3_ctrl3.set);
		return RSUCCESS;
	} else if (pin == GPIO25) {  //ms3 dat3
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS((3L << 12), &obj->reg->msdc3_ctrl3.rst) : GPIO_SET_BITS((1L << 12), &obj->reg->msdc3_ctrl3.set);
		return RSUCCESS;
	} else if (pin == GPIO26) {  //ms3 clk 
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS(3L, &obj->reg->msdc3_ctrl0.rst) : GPIO_SET_BITS(1L, &obj->reg->msdc3_ctrl0.set);
		return RSUCCESS;
	} else if (pin == GPIO27) {  //ms3 cmd
		(enable == GPIO_PULL_DISABLE)? GPIO_SET_BITS(3L, &obj->reg->msdc3_ctrl1.rst) : GPIO_SET_BITS(1L, &obj->reg->msdc3_ctrl1.set);
		return RSUCCESS;
	}

	if (0){
		return GPIO_PULL_EN_UNSUPPORTED;
	}else{
		pos = pin / MAX_GPIO_REG_BITS;
		bit = pin % MAX_GPIO_REG_BITS;

		if (enable == GPIO_PULL_DISABLE)
			GPIO_SET_BITS((1L << bit), &obj->reg->pullen[pos].rst);
		else
			GPIO_SET_BITS((1L << bit), &obj->reg->pullen[pos].set);
	}
    return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_pull_enable_chip(u32 pin)
{
    u32 pos;
    u32 bit;
    u32 reg;
    u32 i;
    struct mt_gpio_obj *obj = gpio_obj;

    if (!obj)
        return -ERACCESS;
    
#if PRELOADER_SUPPORT_EXT_GPIO
    if (pin >= GPIO_EXTEND_START)
        return -ERINVAL;
#else
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;
#endif
	
	/*****************for special kpad pupd, NOTE DEFINITION REVERSE!!!*****************/
	for(i = 0; i < sizeof(kpad_pupd_spec)/sizeof(kpad_pupd_spec[0]); i++){
        if (pin == kpad_pupd_spec[i].pin){
			return (((GPIO_RD32(&obj->reg->kpad_ctrl[kpad_pupd_spec[i].reg].val) & (3L << (kpad_pupd_spec[i].bit-2))) != 0)? 1: 0);        
        }
	}
    /*********************************MSDC special pupd*********************************/
	if (pin == GPIO67) {         //ms0 DS
        return (((GPIO_RD32(&obj->reg->msdc0_ctrl4.val) & (3L << 0)) != 0)? 1: 0); 
	} else if (pin == GPIO68) {  //ms0 RST
        return (((GPIO_RD32(&obj->reg->msdc0_ctrl3.val) & (3L << 0)) != 0)? 1: 0);  
	} else if (pin == GPIO66) {  //ms0 cmd
        return (((GPIO_RD32(&obj->reg->msdc0_ctrl1.val) & (3L << 0)) != 0)? 1: 0);  
	} else if (pin == GPIO65) {  //ms0 clk
        return (((GPIO_RD32(&obj->reg->msdc0_ctrl0.val) & (3L << 0)) != 0)? 1: 0);
	} else if ((pin >= GPIO57) && (pin <= GPIO64)) {	  //ms0 data0~7
        return (((GPIO_RD32(&obj->reg->msdc0_ctrl2.val) & (3L << 0)) != 0)? 1: 0);
	
	} else if (pin == GPIO78) {  //ms1 cmd
        return (((GPIO_RD32(&obj->reg->msdc1_ctrl1.val) & (3L << 0)) != 0)? 1: 0);       
	} else if (pin == GPIO73) {  //ms1 dat0
        return (((GPIO_RD32(&obj->reg->msdc1_ctrl3.val) & (3L << 0)) != 0)? 1: 0);    
	} else if (pin == GPIO74) {  //ms1 dat1
        return (((GPIO_RD32(&obj->reg->msdc1_ctrl3.val) & (3L << 4)) != 0)? 1: 0);        
	} else if (pin == GPIO75) {  //ms1 dat2
        return (((GPIO_RD32(&obj->reg->msdc1_ctrl3.val) & (3L << 8)) != 0)? 1: 0);        
	} else if (pin == GPIO76) {  //ms1 dat3
        return (((GPIO_RD32(&obj->reg->msdc1_ctrl3.val) & (3L << 12)) != 0)? 1: 0);        
	} else if (pin == GPIO77) {  //ms1 clk
        return (((GPIO_RD32(&obj->reg->msdc1_ctrl0.val) & (3L << 0)) != 0)? 1: 0);        

	} else if (pin == GPIO100) {  //ms2 dat0
        return (((GPIO_RD32(&obj->reg->msdc2_ctrl3.val) & (3L << 0)) != 0)? 1: 0);        
	} else if (pin == GPIO101) {  //ms2 dat1
        return (((GPIO_RD32(&obj->reg->msdc2_ctrl3.val) & (3L << 4)) != 0)? 1: 0);        
	} else if (pin == GPIO102) {  //ms2 dat2
        return (((GPIO_RD32(&obj->reg->msdc2_ctrl3.val) & (3L << 8)) != 0)? 1: 0);        
	} else if (pin == GPIO103) {  //ms2 dat3
        return (((GPIO_RD32(&obj->reg->msdc2_ctrl3.val) & (3L << 12)) != 0)? 1: 0);        
	} else if (pin == GPIO104) {  //ms2 clk 
        return (((GPIO_RD32(&obj->reg->msdc2_ctrl0.val) & (3L << 0)) != 0)? 1: 0);        
	} else if (pin == GPIO105) {  //ms2 cmd
        return (((GPIO_RD32(&obj->reg->msdc2_ctrl1.val) & (3L << 0)) != 0)? 1: 0);        

	} else if (pin == GPIO22) {  //ms3 dat0
        return (((GPIO_RD32(&obj->reg->msdc3_ctrl3.val) & (3L << 0)) != 0)? 1: 0);        
	} else if (pin == GPIO23) {  //ms3 dat1
        return (((GPIO_RD32(&obj->reg->msdc3_ctrl3.val) & (3L << 4)) != 0)? 1: 0);        
	} else if (pin == GPIO24) {  //ms3 dat2
        return (((GPIO_RD32(&obj->reg->msdc3_ctrl3.val) & (3L << 8)) != 0)? 1: 0);        
	} else if (pin == GPIO25) {  //ms3 dat3
        return (((GPIO_RD32(&obj->reg->msdc3_ctrl3.val) & (3L << 12)) != 0)? 1: 0);        
	} else if (pin == GPIO26) {  //ms3 clk 
        return (((GPIO_RD32(&obj->reg->msdc3_ctrl0.val) & (3L << 0)) != 0)? 1: 0);        
	} else if (pin == GPIO27) {  //ms3 cmd
        return (((GPIO_RD32(&obj->reg->msdc3_ctrl1.val) & (3L << 0)) != 0)? 1: 0);        
	}

	if (0){
		return GPIO_PULL_EN_UNSUPPORTED;
	}else{
		pos = pin / MAX_GPIO_REG_BITS;
		bit = pin % MAX_GPIO_REG_BITS;

		reg = GPIO_RD32(&obj->reg->pullen[pos].val);
	}
	return (((reg & (1L << bit)) != 0)? 1: 0);        
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_pull_select_chip(u32 pin, u32 select)
{
    u32 pos;
    u32 bit;
    u32 reg;
    u32 mask;
	u32 i;
    struct mt_gpio_obj *obj = gpio_obj;

    if (!obj)
        return -ERACCESS;

#if PRELOADER_SUPPORT_EXT_GPIO
    if (pin >= GPIO_EXTEND_START)
        return -ERINVAL;
#else
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;
#endif
    
    if (select >= GPIO_PULL_MAX)
        return -ERINVAL;

	/***********************for special kpad pupd, NOTE DEFINITION REVERSE!!!**************************/
	for(i = 0; i < sizeof(kpad_pupd_spec)/sizeof(kpad_pupd_spec[0]); i++){
		if (pin == kpad_pupd_spec[i].pin){
			if (select == GPIO_PULL_DOWN)
				GPIO_SET_BITS((1L << kpad_pupd_spec[i].bit), &obj->reg->kpad_ctrl[kpad_pupd_spec[i].reg].set);
			else
				GPIO_SET_BITS((1L << kpad_pupd_spec[i].bit), &obj->reg->kpad_ctrl[kpad_pupd_spec[i].reg].rst);
			return RSUCCESS;
		}
	}
	/*************************************MSDC special pupd*************************/
	if (pin == GPIO67) {         //ms0 DS
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L<<2), &obj->reg->msdc0_ctrl4.rst) : GPIO_SET_BITS((1L<<2), &obj->reg->msdc0_ctrl4.set);
	} else if (pin == GPIO66) {  //ms0 RST
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L<<2), &obj->reg->msdc0_ctrl3.rst) : GPIO_SET_BITS((1L<<2), &obj->reg->msdc0_ctrl3.set);
	} else if (pin == GPIO68) {  //ms0 cmd
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L<<2), &obj->reg->msdc0_ctrl1.rst) : GPIO_SET_BITS((1L<<2), &obj->reg->msdc0_ctrl1.set);
	} else if (pin == GPIO65) {  //ms0 clk
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L<<2), &obj->reg->msdc0_ctrl0.rst) : GPIO_SET_BITS((1L<<2), &obj->reg->msdc0_ctrl0.set);
	} else if ( (pin >= GPIO57) && (pin <= GPIO64) ) {  //ms0 data0~7
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L<<2), &obj->reg->msdc0_ctrl2.rst) : GPIO_SET_BITS((1L<<2), &obj->reg->msdc0_ctrl2.set);

	} else if (pin == GPIO78) {   //ms1 cmd
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L << 2), &obj->reg->msdc1_ctrl1.rst) : GPIO_SET_BITS((1L << 2), &obj->reg->msdc1_ctrl1.set);
	} else if (pin == GPIO73) {   //ms1 dat0
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L << 2), &obj->reg->msdc1_ctrl3.rst) : GPIO_SET_BITS((1L << 2), &obj->reg->msdc1_ctrl3.set);
	} else if (pin == GPIO74) {   //ms1 dat1
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L << 6), &obj->reg->msdc1_ctrl3.rst) : GPIO_SET_BITS((1L << 6), &obj->reg->msdc1_ctrl3.set);
	} else if (pin == GPIO75) {   //ms1 dat2
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L << 10), &obj->reg->msdc1_ctrl3.rst) : GPIO_SET_BITS((1L << 10), &obj->reg->msdc1_ctrl3.set);
	} else if (pin == GPIO76) {   //ms1 dat3
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L << 14), &obj->reg->msdc1_ctrl3.rst) : GPIO_SET_BITS((1L << 14), &obj->reg->msdc1_ctrl3.set);
	} else if (pin == GPIO77) {   //ms1 clk
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L << 2), &obj->reg->msdc1_ctrl0.rst) : GPIO_SET_BITS((1L << 2), &obj->reg->msdc1_ctrl0.set);

	} else if (pin == GPIO100) {   //ms2 dat0
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L << 2), &obj->reg->msdc2_ctrl3.rst) : GPIO_SET_BITS((1L << 2), &obj->reg->msdc2_ctrl3.set);
	} else if (pin == GPIO101) {   //ms2 dat1
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L << 6), &obj->reg->msdc2_ctrl3.rst) : GPIO_SET_BITS((1L << 6), &obj->reg->msdc2_ctrl3.set);
	} else if (pin == GPIO102) {   //ms2 dat2
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L << 10), &obj->reg->msdc2_ctrl3.rst) : GPIO_SET_BITS((1L << 10), &obj->reg->msdc2_ctrl3.set);
	} else if (pin == GPIO103) {   //ms2 dat3
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L << 14), &obj->reg->msdc2_ctrl3.rst) : GPIO_SET_BITS((1L << 14), &obj->reg->msdc2_ctrl3.set);
	} else if (pin == GPIO104) {   //ms2 clk 
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L << 2), &obj->reg->msdc2_ctrl0.rst) : GPIO_SET_BITS((1L << 2), &obj->reg->msdc2_ctrl0.set);
	} else if (pin == GPIO105) {   //ms2 cmd
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L << 2), &obj->reg->msdc2_ctrl1.rst) : GPIO_SET_BITS((1L << 2), &obj->reg->msdc2_ctrl1.set);

	} else if (pin == GPIO22) {   //ms3 dat0
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L << 2), &obj->reg->msdc3_ctrl3.rst) : GPIO_SET_BITS((1L << 2), &obj->reg->msdc3_ctrl3.set);
	} else if (pin == GPIO23) {   //ms3 dat1
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L << 6), &obj->reg->msdc3_ctrl3.rst) : GPIO_SET_BITS((1L << 6), &obj->reg->msdc3_ctrl3.set);
	} else if (pin == GPIO24) {   //ms3 dat2
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L << 10), &obj->reg->msdc3_ctrl3.rst) : GPIO_SET_BITS((1L << 10), &obj->reg->msdc3_ctrl3.set);
	} else if (pin == GPIO25) {   //ms3 dat3
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L << 14), &obj->reg->msdc3_ctrl3.rst) : GPIO_SET_BITS((1L << 14), &obj->reg->msdc3_ctrl3.set);
	} else if (pin == GPIO26) {   //ms3 clk 
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L << 2), &obj->reg->msdc3_ctrl0.rst) : GPIO_SET_BITS((1L << 2), &obj->reg->msdc3_ctrl0.set);
	} else if (pin == GPIO27) {   //ms3 cmd
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L << 2), &obj->reg->msdc3_ctrl1.rst) : GPIO_SET_BITS((1L << 2), &obj->reg->msdc3_ctrl1.set);
	}

	if (0){
		return GPIO_PULL_EN_UNSUPPORTED;
	}else{
		pos = pin / MAX_GPIO_REG_BITS;
		bit = pin % MAX_GPIO_REG_BITS;
		
		if (select == GPIO_PULL_DOWN)
			GPIO_SET_BITS((1L << bit), &obj->reg->pullsel[pos].rst);
		else
			GPIO_SET_BITS((1L << bit), &obj->reg->pullsel[pos].set);
	}
    return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_pull_select_chip(u32 pin)
{
    u32 pos;
    u32 bit;
    u32 reg;
	u32 i;
    struct mt_gpio_obj *obj = gpio_obj;

    if (!obj)
        return -ERACCESS;

#if PRELOADER_SUPPORT_EXT_GPIO
    if (pin >= GPIO_EXTEND_START)
        return -ERINVAL;
#else
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;
#endif

	/*********************************for special kpad pupd*********************************/
	for(i = 0; i < sizeof(kpad_pupd_spec)/sizeof(kpad_pupd_spec[0]); i++){
		if (pin == kpad_pupd_spec[i].pin){
			reg = GPIO_RD32(&obj->reg->kpad_ctrl[kpad_pupd_spec[i].reg].val);
			return (((reg & (1L << kpad_pupd_spec[i].bit)) != 0)? 0: 1);
		}
	}
    /********************************* MSDC special pupd *********************************/
	if (pin == GPIO67) {         //ms0 DS
        return (((GPIO_RD32(&obj->reg->msdc0_ctrl4.val) & (1L << 2)) != 0)? 0: 1); 
	} else if (pin == GPIO68) {  //ms0 RST
        return (((GPIO_RD32(&obj->reg->msdc0_ctrl3.val) & (1L << 2)) != 0)? 0: 1);  
	} else if (pin == GPIO66) {  //ms0 cmd
        return (((GPIO_RD32(&obj->reg->msdc0_ctrl1.val) & (1L << 2)) != 0)? 0: 1);  
	} else if (pin == GPIO65) {  //ms0 clk
        return (((GPIO_RD32(&obj->reg->msdc0_ctrl0.val) & (1L << 2)) != 0)? 0: 1);
	} else if ((pin >= GPIO57) && (pin <= GPIO64)) {	  //ms0 data0~7
        return (((GPIO_RD32(&obj->reg->msdc0_ctrl2.val) & (1L << 2)) != 0)? 0: 1);

	} else if (pin == GPIO78) {  //ms1 cmd
		return (((GPIO_RD32(&obj->reg->msdc1_ctrl1.val) & (1L << 2)) != 0)? 0: 1);
	} else if (pin == GPIO73) {  //ms1 dat0
		return (((GPIO_RD32(&obj->reg->msdc1_ctrl3.val) & (1L << 2)) != 0)? 0: 1);
	} else if (pin == GPIO74) {  //ms1 dat1
		return (((GPIO_RD32(&obj->reg->msdc1_ctrl3.val) & (1L << 6)) != 0)? 0: 1);
	} else if (pin == GPIO75) {  //ms1 dat2
		return (((GPIO_RD32(&obj->reg->msdc1_ctrl3.val) & (1L << 10)) != 0)? 0: 1);
	} else if (pin == GPIO76) {  //ms1 dat3
		return (((GPIO_RD32(&obj->reg->msdc1_ctrl3.val) & (1L << 14)) != 0)? 0: 1);
	} else if (pin == GPIO77) {  //ms1 clk
		return (((GPIO_RD32(&obj->reg->msdc1_ctrl0.val) & (1L << 2)) != 0)? 0: 1);

	} else if (pin == GPIO100) {  //ms2 dat0
		return (((GPIO_RD32(&obj->reg->msdc2_ctrl3.val) & (1L << 2)) != 0)? 0: 1);
	} else if (pin == GPIO101) {  //ms2 dat1
		return (((GPIO_RD32(&obj->reg->msdc2_ctrl3.val) & (1L << 6)) != 0)? 0: 1);
	} else if (pin == GPIO102) {  //ms2 dat2
		return (((GPIO_RD32(&obj->reg->msdc2_ctrl3.val) & (1L << 10)) != 0)? 0: 1);
	} else if (pin == GPIO103) {  //ms2 dat3
		return (((GPIO_RD32(&obj->reg->msdc2_ctrl3.val) & (1L << 14)) != 0)? 0: 1);
	} else if (pin == GPIO104) {  //ms2 clk 
		return (((GPIO_RD32(&obj->reg->msdc2_ctrl0.val) & (1L << 2)) != 0)? 0: 1);
	} else if (pin == GPIO105) {  //ms2 cmd
		return (((GPIO_RD32(&obj->reg->msdc2_ctrl1.val) & (1L << 2)) != 0)? 0: 1);

	} else if (pin == GPIO22) {  //ms3 dat0
		return (((GPIO_RD32(&obj->reg->msdc3_ctrl3.val) & (1L << 2)) != 0)? 0: 1);
	} else if (pin == GPIO23) {  //ms3 dat1
		return (((GPIO_RD32(&obj->reg->msdc3_ctrl3.val) & (1L << 6)) != 0)? 0: 1);
	} else if (pin == GPIO24) {  //ms3 dat2
		return (((GPIO_RD32(&obj->reg->msdc3_ctrl3.val) & (1L << 10)) != 0)? 0: 1);
	} else if (pin == GPIO25) {  //ms3 dat3
		return (((GPIO_RD32(&obj->reg->msdc3_ctrl3.val) & (1L << 14)) != 0)? 0: 1);
	} else if (pin == GPIO26) {  //ms3 clk 
		return ((GPIO_RD32(&obj->reg->msdc3_ctrl0.val) & (1L << 2)) != 0)? 0: 1;
	} else if (pin == GPIO27) {  //ms3 cmd
		return (((GPIO_RD32(&obj->reg->msdc3_ctrl1.val) & (1L << 2)) != 0)? 0: 1);
	}

	if (0){
		return GPIO_PULL_EN_UNSUPPORTED;
	}else{
		pos = pin / MAX_GPIO_REG_BITS;
		bit = pin % MAX_GPIO_REG_BITS;

		reg = GPIO_RD32(&obj->reg->pullsel[pos].val);
	}
    return (((reg & (1L << bit)) != 0)? 1: 0);        
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_out_chip(u32 pin, u32 output)
{
    u32 pos;
    u32 bit;
    struct mt_gpio_obj *obj = gpio_obj;

    if (!obj)
        return -ERACCESS;

#if PRELOADER_SUPPORT_EXT_GPIO
    if (pin >= GPIO_EXTEND_START)
        return -ERINVAL;
#else
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;
#endif

    if (output >= GPIO_OUT_MAX)
        return -ERINVAL;
    
    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;
    
    if (output == GPIO_OUT_ZERO)
        GPIO_SET_BITS((1L << bit), &obj->reg->dout[pos].rst);
    else
        GPIO_SET_BITS((1L << bit), &obj->reg->dout[pos].set);
    return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_out_chip(u32 pin)
{
    u32 pos;
    u32 bit;
    u32 reg;
    struct mt_gpio_obj *obj = gpio_obj;

    if (!obj)
        return -ERACCESS;

#if PRELOADER_SUPPORT_EXT_GPIO
    if (pin >= GPIO_EXTEND_START)
        return -ERINVAL;
#else
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;
#endif
    
    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;

    reg = GPIO_RD32(&obj->reg->dout[pos].val);
    return (((reg & (1L << bit)) != 0)? 1: 0);        
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_in_chip(u32 pin)
{
    u32 pos;
    u32 bit;
    u32 reg;
    struct mt_gpio_obj *obj = gpio_obj;

    if (!obj)
        return -ERACCESS;

#if PRELOADER_SUPPORT_EXT_GPIO
    if (pin >= GPIO_EXTEND_START)
        return -ERINVAL;
#else
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;
#endif
    
    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;

    reg = GPIO_RD32(&obj->reg->din[pos].val);
    return (((reg & (1L << bit)) != 0)? 1: 0);        
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_mode_chip(u32 pin, u32 mode)
{
    u32 pos;
    u32 bit;
    u32 reg;
    u32 mask = (1L << GPIO_MODE_BITS) - 1;    
    struct mt_gpio_obj *obj = gpio_obj;

    if (!obj)
        return -ERACCESS;

#if PRELOADER_SUPPORT_EXT_GPIO
    if (pin >= GPIO_EXTEND_START)
        return -ERINVAL;
#else
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;
#endif

    if (mode >= GPIO_MODE_MAX)
        return -ERINVAL;

	pos = pin / MAX_GPIO_MODE_PER_REG;
	bit = pin % MAX_GPIO_MODE_PER_REG;
   
	reg = GPIO_RD32(&obj->reg->mode[pos].val);

	reg &= ~(mask << (GPIO_MODE_BITS*bit));
	reg |= (mode << (GPIO_MODE_BITS*bit));
	
	GPIO_WR32(&obj->reg->mode[pos].val, reg);

    return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_mode_chip(u32 pin)
{
    u32 pos;
    u32 bit;
    u32 reg;
    u32 mask = (1L << GPIO_MODE_BITS) - 1;    
    struct mt_gpio_obj *obj = gpio_obj;

    if (!obj)
        return -ERACCESS;

#if PRELOADER_SUPPORT_EXT_GPIO
    if (pin >= GPIO_EXTEND_START)
        return -ERINVAL;
#else
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;
#endif
    
	pos = pin / MAX_GPIO_MODE_PER_REG;
	bit = pin % MAX_GPIO_MODE_PER_REG;

	reg = GPIO_RD32(&obj->reg->mode[pos].val);
	
	return ((reg >> (GPIO_MODE_BITS*bit)) & mask);
}
/*---------------------------------------------------------------------------*/
#if PRELOADER_SUPPORT_EXT_GPIO
s32 mt_set_gpio_dir_ext(u32 pin, u32 dir)
{
    u32 pos;
    u32 bit;
	int ret=0;
    struct mt_gpioext_obj *obj = gpioext_obj;

    if (!obj)
        return -ERACCESS;

    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

    if (dir >= GPIO_DIR_MAX)
        return -ERINVAL;
	pin -= GPIO_EXTEND_START;    
    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;
    
    if (dir == GPIO_DIR_IN)
        ret=GPIOEXT_SET_BITS((1L << bit), &obj->reg->dir[pos].rst);
    else
        ret=GPIOEXT_SET_BITS((1L << bit), &obj->reg->dir[pos].set);
	if(ret!=0) return -ERWRAPPER;
    return RSUCCESS;
    
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_dir_ext(u32 pin)
{    
    u32 pos;
    u32 bit;
    s64 reg;
    struct mt_gpioext_obj *obj = gpioext_obj;

    if (!obj)
        return -ERACCESS;
    
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;
    
	pin -= GPIO_EXTEND_START;    
    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;
    
    reg = GPIOEXT_RD(&obj->reg->dir[pos].val);
    if(reg < 0) return -ERWRAPPER;
    return (((reg & (1L << bit)) != 0)? 1: 0);        
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_pull_enable_ext(u32 pin, u32 enable)
{
    u32 pos;
    u32 bit;
	int ret=0;
    struct mt_gpioext_obj *obj = gpioext_obj;

    if (!obj)
        return -ERACCESS;
    
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

    if (enable >= GPIO_PULL_EN_MAX)
        return -ERINVAL;
    
	pin -= GPIO_EXTEND_START;    
    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;

    if (enable == GPIO_PULL_DISABLE)
        ret=GPIOEXT_SET_BITS((1L << bit), &obj->reg->pullen[pos].rst);
    else
        ret=GPIOEXT_SET_BITS((1L << bit), &obj->reg->pullen[pos].set);
	if(ret!=0) return -ERWRAPPER;
    return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_pull_enable_ext(u32 pin)
{
    u32 pos;
    u32 bit;
    s64 reg;
    struct mt_gpioext_obj *obj = gpioext_obj;

    if (!obj)
        return -ERACCESS;
    
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;
    
	pin -= GPIO_EXTEND_START;    
    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;

    reg = GPIOEXT_RD(&obj->reg->pullen[pos].val);
    if(reg < 0) return -ERWRAPPER;
    return (((reg & (1L << bit)) != 0)? 1: 0);        
}
s32 mt_set_gpio_pull_select_ext(u32 pin, u32 select)
{
    u32 pos;
    u32 bit;
	int ret=0;
    struct mt_gpioext_obj *obj = gpioext_obj;

    if (!obj)
        return -ERACCESS;

    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;
    
    if (select >= GPIO_PULL_MAX)
        return -ERINVAL;

	pin -= GPIO_EXTEND_START;    
    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;
    
    if (select == GPIO_PULL_DOWN)
        ret=GPIOEXT_SET_BITS((1L << bit), &obj->reg->pullsel[pos].rst);
    else
        ret=GPIOEXT_SET_BITS((1L << bit), &obj->reg->pullsel[pos].set);
	if(ret!=0) return -ERWRAPPER;
    return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_pull_select_ext(u32 pin)
{
    u32 pos;
    u32 bit;
    s64 reg;
    struct mt_gpioext_obj *obj = gpioext_obj;

    if (!obj)
        return -ERACCESS;

    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;
    
	pin -= GPIO_EXTEND_START;    
    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;

    reg = GPIOEXT_RD(&obj->reg->pullsel[pos].val);
    if(reg < 0) return -ERWRAPPER;
    return (((reg & (1L << bit)) != 0)? 1: 0);        
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_inversion_ext(u32 pin, u32 enable)
{
    u32 pos;
    u32 bit;
	int ret = 0;
    struct mt_gpioext_obj *obj = gpioext_obj;

    if (!obj)
        return -ERACCESS;

    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

    if (enable >= GPIO_DATA_INV_MAX)
        return -ERINVAL;

	pin -= GPIO_EXTEND_START;    
    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;
    
    if (enable == GPIO_DATA_UNINV)
        ret=GPIOEXT_SET_BITS((1L << bit), &obj->reg->dinv[pos].rst);
    else
        ret=GPIOEXT_SET_BITS((1L << bit), &obj->reg->dinv[pos].set);
	if(ret!=0) return -ERWRAPPER;
    return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_inversion_ext(u32 pin)
{
    u32 pos;
    u32 bit;
    s64 reg;
    struct mt_gpioext_obj *obj = gpioext_obj;

    if (!obj)
        return -ERACCESS;

    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;
    
	pin -= GPIO_EXTEND_START;    
    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;

    reg = GPIOEXT_RD(&obj->reg->dinv[pos].val);
    if(reg < 0) return -ERWRAPPER;
    return (((reg & (1L << bit)) != 0)? 1: 0);
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_out_ext(u32 pin, u32 output)
{
    u32 pos;
    u32 bit;
	int ret = 0;
    struct mt_gpioext_obj *obj = gpioext_obj;

    if (!obj)
        return -ERACCESS;

    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

    if (output >= GPIO_OUT_MAX)
        return -ERINVAL;
    
	pin -= GPIO_EXTEND_START;    
    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;
    
    if (output == GPIO_OUT_ZERO)
        ret=GPIOEXT_SET_BITS((1L << bit), &obj->reg->dout[pos].rst);
    else
        ret=GPIOEXT_SET_BITS((1L << bit), &obj->reg->dout[pos].set);
	if(ret!=0) return -ERWRAPPER;
    return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_out_ext(u32 pin)
{
    u32 pos;
    u32 bit;
    s64 reg;
    struct mt_gpioext_obj *obj = gpioext_obj;

    if (!obj)
        return -ERACCESS;

    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;
    
	pin -= GPIO_EXTEND_START;    
    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;

    reg = GPIOEXT_RD(&obj->reg->dout[pos].val);
    if(reg < 0) return -ERWRAPPER;
    return (((reg & (1L << bit)) != 0)? 1: 0);        
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_in_ext(u32 pin)
{
    u32 pos;
    u32 bit;
    s64 reg;
    struct mt_gpioext_obj *obj = gpioext_obj;

    if (!obj)
        return -ERACCESS;

    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;
    
	pin -= GPIO_EXTEND_START;    
    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;

    reg = GPIOEXT_RD(&obj->reg->din[pos].val);
    if(reg < 0) return -ERWRAPPER;
    return (((reg & (1L << bit)) != 0)? 1: 0);        
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_mode_ext(u32 pin, u32 mode)
{
    u32 pos;
    u32 bit;
    s64 reg;
	int ret=0;
    S32 data;
    u32 mask = (1L << GPIO_MODE_BITS) - 1;    
    struct mt_gpioext_obj *obj = gpioext_obj;

    if (!obj)
        return -ERACCESS;

    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

    if (mode >= GPIO_MODE_MAX)
        return -ERINVAL;
    
    if(pin == GPIOEXT12 || pin == GPIOEXT20) {
    	if(mode == GPIO_MODE_00) {
    		data = GPIOEXT_RD(RG_USBDL_EN_CTL_REG);
    		if(data < 0) return -ERWRAPPER;
    		data = data & 0xFFFFFFFE;
    		GPIOEXT_WR(RG_USBDL_EN_CTL_REG, data);
    	} else {
    		data = GPIOEXT_RD(RG_USBDL_EN_CTL_REG);
    		if(data < 0) return -ERWRAPPER;
    		data = data | 0x00000001;
    		GPIOEXT_WR(RG_USBDL_EN_CTL_REG, data);
    	}
    }
    
	pin -= GPIO_EXTEND_START;    
    pos = pin / MAX_GPIO_MODE_PER_REG;
    bit = pin % MAX_GPIO_MODE_PER_REG;

   
    reg = GPIOEXT_RD(&obj->reg->mode[pos].val);
    if(reg < 0) return -ERWRAPPER;

    reg &= ~(mask << (GPIO_MODE_BITS*bit));
    reg |= (mode << (GPIO_MODE_BITS*bit));
    
    ret = GPIOEXT_WR(&obj->reg->mode[pos].val, reg);
	if(ret!=0) return -ERWRAPPER;
    return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_mode_ext(u32 pin)
{
    u32 pos;
    u32 bit;
    s64 reg;
    u32 mask = (1L << GPIO_MODE_BITS) - 1;    
    struct mt_gpioext_obj *obj = gpioext_obj;

    if (!obj)
        return -ERACCESS;

    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;
    
	pin -= GPIO_EXTEND_START;    
    pos = pin / MAX_GPIO_MODE_PER_REG;
    bit = pin % MAX_GPIO_MODE_PER_REG;

    reg = GPIOEXT_RD(&obj->reg->mode[pos].val);
    if(reg < 0) return -ERWRAPPER;
    
    return ((reg >> (GPIO_MODE_BITS*bit)) & mask);
}
#endif
void mt_gpio_pin_decrypt(unsigned long *cipher)
{
	//just for debug, find out who used pin number directly
	if((*cipher & (0x80000000)) == 0){
		GPIOERR("Pin %u decrypt warning! \n",*cipher);	
		//dump_stack();
		//return;
	}

	//GPIOERR("Pin magic number is %x\n",*cipher);
	*cipher &= ~(0x80000000);
	return;
}
//set GPIO function in fact
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_dir(u32 pin, u32 dir)
{
	mt_gpio_pin_decrypt(&pin);

#if PRELOADER_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_set_gpio_dir_ext(pin,dir): mt_set_gpio_dir_chip(pin,dir);
#else
    return mt_set_gpio_dir_chip(pin,dir);
#endif
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_dir(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);

#if PRELOADER_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_get_gpio_dir_ext(pin): mt_get_gpio_dir_chip(pin);
#else
    return mt_get_gpio_dir_chip(pin);
#endif
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_pull_enable(u32 pin, u32 enable)
{
	mt_gpio_pin_decrypt(&pin);

#if PRELOADER_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_set_gpio_pull_enable_ext(pin,enable): mt_set_gpio_pull_enable_chip(pin,enable);
#else
    return mt_set_gpio_pull_enable_chip(pin,enable);
#endif
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_pull_enable(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);

#if PRELOADER_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_get_gpio_pull_enable_ext(pin): mt_get_gpio_pull_enable_chip(pin);
#else
    return mt_get_gpio_pull_enable_chip(pin);
#endif
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_pull_select(u32 pin, u32 select)
{
	mt_gpio_pin_decrypt(&pin);

#if PRELOADER_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_set_gpio_pull_select_ext(pin,select): mt_set_gpio_pull_select_chip(pin,select);
#else
    return mt_set_gpio_pull_select_chip(pin,select);
#endif
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_pull_select(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);

#if PRELOADER_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_get_gpio_pull_select_ext(pin): mt_get_gpio_pull_select_chip(pin);
#else
    return mt_get_gpio_pull_select_chip(pin);
#endif
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_inversion(u32 pin, u32 enable)
{
	mt_gpio_pin_decrypt(&pin);

#if PRELOADER_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_set_gpio_inversion_ext(pin,enable): mt_set_gpio_inversion_chip(pin,enable);
#else
    return mt_set_gpio_inversion_chip(pin,enable);
#endif
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_inversion(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);

#if PRELOADER_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_get_gpio_inversion_ext(pin): mt_get_gpio_inversion_chip(pin);
#else
    return mt_get_gpio_inversion_chip(pin);
#endif
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_out(u32 pin, u32 output)
{
	mt_gpio_pin_decrypt(&pin);
#if PRELOADER_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_set_gpio_out_ext(pin,output): mt_set_gpio_out_chip(pin,output);
#else
    return mt_set_gpio_out_chip(pin,output);
#endif
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_out(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);
#if PRELOADER_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_get_gpio_out_ext(pin): mt_get_gpio_out_chip(pin);
#else
    return mt_get_gpio_out_chip(pin);
#endif
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_in(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);

#if PRELOADER_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_get_gpio_in_ext(pin): mt_get_gpio_in_chip(pin);
#else
    return mt_get_gpio_in_chip(pin);
#endif
}
/*---------------------------------------------------------------------------*/
s32 mt_set_gpio_mode(u32 pin, u32 mode)
{
	mt_gpio_pin_decrypt(&pin);

#if PRELOADER_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_set_gpio_mode_ext(pin,mode): mt_set_gpio_mode_chip(pin,mode);
#else
    return mt_set_gpio_mode_chip(pin,mode);
#endif
}
/*---------------------------------------------------------------------------*/
s32 mt_get_gpio_mode(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);

#if PRELOADER_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_get_gpio_mode_ext(pin): mt_get_gpio_mode_chip(pin);
#else
    return mt_get_gpio_mode_chip(pin);
#endif
}
/*---------------------------------------------------------------------------*/
void mt_gpio_init(void)
{
#ifdef DUMMY_AP
	mt_gpio_set_default();
#endif

#ifdef TINY
	mt_gpio_set_default();
#endif
}
