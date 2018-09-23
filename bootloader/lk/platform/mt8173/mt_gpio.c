/******************************************************************************
 * mt_gpio.c - MTKLinux GPIO Device Driver
 *
 * Copyright 2008-2009 MediaTek Co.,Ltd.
 *
 * DESCRIPTION:
 *     This file provid the other drivers GPIO relative functions
 *
 ******************************************************************************/

#include <platform/mt_reg_base.h>
#include <platform/mt_gpio.h>
#include <platform/mt_pmic_wrap_init.h>
#include <debug.h>


/******************************************************************************
 MACRO Definition
******************************************************************************/
//#define  GIO_SLFTEST
#define GPIO_DEVICE "mt-gpio"
#define VERSION     GPIO_DEVICE
/*---------------------------------------------------------------------------*/
#define GPIO_WR32(addr, data)   DRV_WriteReg16(addr,data)
#define GPIO_RD32(addr)         DRV_Reg16(addr)
#define GPIO_SET_BITS(BIT,REG)   ((*(volatile u16*)(REG)) = (u16)(BIT))
#define GPIO_CLR_BITS(BIT,REG)   ((*(volatile u16*)(REG)) &= ~((u16)(BIT)))

#if LK_SUPPORT_EXT_GPIO
#define GPIOEXT_WR(addr, data)   pwrap_write((u32)addr, data)
#define GPIOEXT_RD(addr)         ({ \
        u32 ext_data; \
        int ret; \
        ret = pwrap_read((u32)addr,&ext_data); \
        (ret != 0)?4294967295:ext_data;})
#define GPIOEXT_SET_BITS(BIT,REG)   (GPIOEXT_WR(REG, (u32)(BIT)))
#define GPIOEXT_CLR_BITS(BIT,REG)    ({ \
        u32 ext_data; \
        int ret; \
        ret = GPIOEXT_RD(REG);\
        ext_data = ret;\
        (ret < 0)?4294967295:(GPIOEXT_WR(REG,ext_data & ~((u32)(BIT))))})
#define GPIOEXT_BASE        (0xC000)            //PMIC GPIO base.
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
#define GPIOLOG(fmt, arg...)   dprintf(INFO,GPIOTAG fmt, ##arg)
#define GPIOMSG(fmt, arg...)   dprintf(INFO,fmt, ##arg)
#define GPIOERR(fmt, arg...)   dprintf(INFO,GPIOTAG "%5d: "fmt, __LINE__, ##arg)
#define GPIOFUC(fmt, arg...)   //dprintk(INFO,GPIOTAG "%s\n", __FUNCTION__)
#define GIO_INVALID_OBJ(ptr)   ((ptr) != gpio_obj)

/******************************************************************************
Enumeration/Structure
******************************************************************************/
#if  !defined(MT_GPIO_ENABLED)
S32 mt_set_gpio_dir(u32 pin, u32 dir)               {return RSUCCESS;}
S32 mt_get_gpio_dir(u32 pin)                        {return GPIO_DIR_UNSUPPORTED;}
S32 mt_set_gpio_pull_enable(u32 pin, u32 enable)    {return RSUCCESS;}
S32 mt_get_gpio_pull_enable(u32 pin)                {return GPIO_PULL_EN_UNSUPPORTED;}
S32 mt_set_gpio_pull_select(u32 pin, u32 select)    {return RSUCCESS;}
S32 mt_get_gpio_pull_select(u32 pin)                {return GPIO_PULL_UNSUPPORTED;}
S32 mt_set_gpio_out(u32 pin, u32 output)            {return RSUCCESS;}
S32 mt_get_gpio_out(u32 pin)                        {return GPIO_OUT_UNSUPPORTED;}
S32 mt_get_gpio_in(u32 pin)                         {return GPIO_IN_UNSUPPORTED;}
S32 mt_set_gpio_mode(u32 pin, u32 mode)             {return RSUCCESS;}
S32 mt_get_gpio_mode(u32 pin)                       {return GPIO_MODE_UNSUPPORTED;}
#else
/*-------for special kpad pupd-----------*/
struct kpad_pupd {
	unsigned char   pin;
	unsigned char   reg;
	unsigned char   bit;
};
static struct kpad_pupd kpad_pupd_spec[] = {
	{GPIO119,   0,  2},     //KROW0
	{GPIO120,   0,  6},     //KROW1
	{GPIO121,   0,  10},    //KROW2
	{GPIO122,   1,  2},     //KCOL0
	{GPIO123,   1,  6},     //KCOL1
	{GPIO124,   1,  10}     //KCOL2
};
/*---------------------------------------*/
struct mt_gpio_obj {
	GPIO_REGS       *reg;
};
static struct mt_gpio_obj gpio_dat = {
	.reg  = (GPIO_REGS*)(GPIO_BASE),
};
static struct mt_gpio_obj *gpio_obj = &gpio_dat;
/*---------------------------------------------------------------------------*/
#if LK_SUPPORT_EXT_GPIO
/*---------------------------------------*/
struct mt_gpioext_obj {
	GPIOEXT_REGS    *reg;
};
/*---------------------------------------*/
static struct mt_gpioext_obj gpioext_dat = {
	.reg = (GPIOEXT_REGS*)(GPIOEXT_BASE),
};
/*---------------------------------------*/
static struct mt_gpioext_obj *gpioext_obj = &gpioext_dat;
#endif


/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_dir_chip(u32 pin, u32 dir)
{
	u32 pos;
	u32 bit;
	struct mt_gpio_obj *obj = gpio_obj;

	if (!obj)
		return -ERACCESS;

#if LK_SUPPORT_EXT_GPIO
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
S32 mt_get_gpio_dir_chip(u32 pin)
{
	u32 pos;
	u32 bit;
	u32 reg;
	struct mt_gpio_obj *obj = gpio_obj;

	if (!obj)
		return -ERACCESS;

#if LK_SUPPORT_EXT_GPIO
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
S32 mt_set_gpio_pull_enable_chip(u32 pin, u32 enable)
{
	u32 pos;
	u32 bit;
	u32 i;

	struct mt_gpio_obj *obj = gpio_obj;

	if (!obj)
		return -ERACCESS;

#if LK_SUPPORT_EXT_GPIO
	if (pin >= GPIO_EXTEND_START)
		return -ERINVAL;
#else
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
#endif

	if (enable >= GPIO_PULL_EN_MAX)
		return -ERINVAL;

	/*****************for special kpad pupd, NOTE DEFINITION REVERSE!!!*****************/
	for (i = 0; i < sizeof(kpad_pupd_spec)/sizeof(kpad_pupd_spec[0]); i++) {
		if (pin == kpad_pupd_spec[i].pin) {
			if (enable == GPIO_PULL_DISABLE) {
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

	if (0) {
		return GPIO_PULL_EN_UNSUPPORTED;
	} else {
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
S32 mt_get_gpio_pull_enable_chip(u32 pin)
{
	u32 pos;
	u32 bit;
	u32 reg;
	u32 i;

	struct mt_gpio_obj *obj = gpio_obj;

	if (!obj)
		return -ERACCESS;

#if LK_SUPPORT_EXT_GPIO
	if (pin >= GPIO_EXTEND_START)
		return -ERINVAL;
#else
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
#endif

	/*****************for special kpad pupd, NOTE DEFINITION REVERSE!!!*****************/
	for (i = 0; i < sizeof(kpad_pupd_spec)/sizeof(kpad_pupd_spec[0]); i++) {
		if (pin == kpad_pupd_spec[i].pin) {
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
	} else if ((pin >= GPIO57) && (pin <= GPIO64)) {      //ms0 data0~7
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

	if (0) {
		return GPIO_PULL_EN_UNSUPPORTED;
	} else {
		pos = pin / MAX_GPIO_REG_BITS;
		bit = pin % MAX_GPIO_REG_BITS;

		reg = GPIO_RD32(&obj->reg->pullen[pos].val);
	}
	return (((reg & (1L << bit)) != 0)? 1: 0);
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_pull_select_chip(u32 pin, u32 select)
{
	u32 pos;
	u32 bit;
	u32 i;
	struct mt_gpio_obj *obj = gpio_obj;

	if (!obj)
		return -ERACCESS;

#if LK_SUPPORT_EXT_GPIO
	if (pin >= GPIO_EXTEND_START)
		return -ERINVAL;
#else
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
#endif

	if (select >= GPIO_PULL_MAX)
		return -ERINVAL;

	/***********************for special kpad pupd, NOTE DEFINITION REVERSE!!!**************************/
	for (i = 0; i < sizeof(kpad_pupd_spec)/sizeof(kpad_pupd_spec[0]); i++) {
		if (pin == kpad_pupd_spec[i].pin) {
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
	} else if (pin == GPIO68) {  //ms0 RST
		(select == GPIO_PULL_UP)? GPIO_SET_BITS((1L<<2), &obj->reg->msdc0_ctrl3.rst) : GPIO_SET_BITS((1L<<2), &obj->reg->msdc0_ctrl3.set);
	} else if (pin == GPIO66) {  //ms0 cmd
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

	if (0) {
		return GPIO_PULL_EN_UNSUPPORTED;
	} else {
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
S32 mt_get_gpio_pull_select_chip(u32 pin)
{
	u32 pos;
	u32 bit;
	u32 reg;
	u32 i;
	struct mt_gpio_obj *obj = gpio_obj;

	if (!obj)
		return -ERACCESS;

#if LK_SUPPORT_EXT_GPIO
	if (pin >= GPIO_EXTEND_START)
		return -ERINVAL;
#else
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
#endif

	/*********************************for special kpad pupd*********************************/
	for (i = 0; i < sizeof(kpad_pupd_spec)/sizeof(kpad_pupd_spec[0]); i++) {
		if (pin == kpad_pupd_spec[i].pin) {
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
	} else if ((pin >= GPIO57) && (pin <= GPIO64)) {      //ms0 data0~7
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
		return (((GPIO_RD32(&obj->reg->msdc3_ctrl0.val) & (1L << 2)) != 0)? 0: 1);
	} else if (pin == GPIO27) {  //ms3 cmd
		return (((GPIO_RD32(&obj->reg->msdc3_ctrl1.val) & (1L << 2)) != 0)? 0: 1);
	}

	if (0) {
		return GPIO_PULL_EN_UNSUPPORTED;
	} else {
		pos = pin / MAX_GPIO_REG_BITS;
		bit = pin % MAX_GPIO_REG_BITS;

		reg = GPIO_RD32(&obj->reg->pullsel[pos].val);
	}
	return (((reg & (1L << bit)) != 0)? 1: 0);
}

/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_out_chip(u32 pin, u32 output)
{
	u32 pos;
	u32 bit;
	struct mt_gpio_obj *obj = gpio_obj;

	if (!obj)
		return -ERACCESS;

#if LK_SUPPORT_EXT_GPIO
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
S32 mt_get_gpio_out_chip(u32 pin)
{
	u32 pos;
	u32 bit;
	u32 reg;
	struct mt_gpio_obj *obj = gpio_obj;

	if (!obj)
		return -ERACCESS;

#if LK_SUPPORT_EXT_GPIO
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
S32 mt_get_gpio_in_chip(u32 pin)
{
	u32 pos;
	u32 bit;
	u32 reg;
	struct mt_gpio_obj *obj = gpio_obj;

	if (!obj)
		return -ERACCESS;

#if LK_SUPPORT_EXT_GPIO
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
S32 mt_set_gpio_mode_chip(u32 pin, u32 mode)
{
	u32 pos;
	u32 bit;
	u32 reg;
	u32 mask = (1L << GPIO_MODE_BITS) - 1;
	struct mt_gpio_obj *obj = gpio_obj;

	if (!obj)
		return -ERACCESS;

#if LK_SUPPORT_EXT_GPIO
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
S32 mt_get_gpio_mode_chip(u32 pin)
{
	u32 pos;
	u32 bit;
	u32 reg;
	u32 mask = (1L << GPIO_MODE_BITS) - 1;
	struct mt_gpio_obj *obj = gpio_obj;

	if (!obj)
		return -ERACCESS;

#if LK_SUPPORT_EXT_GPIO
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
#if LK_SUPPORT_EXT_GPIO
S32 mt_set_gpio_dir_ext(u32 pin, u32 dir)
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
	if (ret!=0) return -ERWRAPPER;
	return RSUCCESS;

}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_dir_ext(u32 pin)
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
	if (reg < 0) return -ERWRAPPER;
	return (((reg & (1L << bit)) != 0)? 1: 0);
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_pull_enable_ext(u32 pin, u32 enable)
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
	if (ret!=0) return -ERWRAPPER;
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_pull_enable_ext(u32 pin)
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
	if (reg < 0) return -ERWRAPPER;
	return (((reg & (1L << bit)) != 0)? 1: 0);
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_pull_select_ext(u32 pin, u32 select)
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
	if (ret!=0) return -ERWRAPPER;
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_pull_select_ext(u32 pin)
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
	if (reg < 0) return -ERWRAPPER;
	return (((reg & (1L << bit)) != 0)? 1: 0);
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_inversion_ext(u32 pin, u32 enable)
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
	if (ret!=0) return -ERWRAPPER;
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_inversion_ext(u32 pin)
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
	if (reg < 0) return -ERWRAPPER;
	return (((reg & (1L << bit)) != 0)? 1: 0);
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_out_ext(u32 pin, u32 output)
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
	if (ret!=0) return -ERWRAPPER;
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_out_ext(u32 pin)
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
	if (reg < 0) return -ERWRAPPER;
	return (((reg & (1L << bit)) != 0)? 1: 0);
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_in_ext(u32 pin)
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
	if (reg < 0) return -ERWRAPPER;
	return (((reg & (1L << bit)) != 0)? 1: 0);
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_mode_ext(u32 pin, u32 mode)
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

	if (pin == GPIOEXT12 || pin == GPIOEXT20) {
		if (mode == GPIO_MODE_00) {
			data = GPIOEXT_RD(RG_USBDL_EN_CTL_REG);
			if (data < 0) return -ERWRAPPER;
			data = data & 0xFFFFFFFE;
			GPIOEXT_WR(RG_USBDL_EN_CTL_REG, data);
		} else {
			data = GPIOEXT_RD(RG_USBDL_EN_CTL_REG);
			if (data < 0) return -ERWRAPPER;
			data = data | 0x00000001;
			GPIOEXT_WR(RG_USBDL_EN_CTL_REG, data);
		}
	}

	pin -= GPIO_EXTEND_START;
	pos = pin / MAX_GPIO_MODE_PER_REG;
	bit = pin % MAX_GPIO_MODE_PER_REG;


	reg = GPIOEXT_RD(&obj->reg->mode[pos].val);
	if (reg < 0) return -ERWRAPPER;

	reg &= ~(mask << (GPIO_MODE_BITS*bit));
	reg |= (mode << (GPIO_MODE_BITS*bit));

	ret = GPIOEXT_WR(&obj->reg->mode[pos].val, reg);
	if (ret!=0) return -ERWRAPPER;
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_mode_ext(u32 pin)
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
	if (reg < 0) return -ERWRAPPER;

	return ((reg >> (GPIO_MODE_BITS*bit)) & mask);
}

#endif

void mt_gpio_pin_decrypt(u32 *cipher)
{
	//just for debug, find out who used pin number directly
	if ((*cipher & (0x80000000)) == 0) {
		//GPIOERR("GPIO%u HARDCODE warning!!! \n",(unsigned int)*cipher);
		//dump_stack();
		//return;
	}

	//GPIOERR("Pin magic number is %x\n",*cipher);
	*cipher &= ~(0x80000000);
	return;
}
//set GPIO function in fact
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_dir(u32 pin, u32 dir)
{
	mt_gpio_pin_decrypt(&pin);

#if LK_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_set_gpio_dir_ext(pin,dir): mt_set_gpio_dir_chip(pin,dir);
#else
	return mt_set_gpio_dir_chip(pin,dir);
#endif
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_dir(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);

#if LK_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_get_gpio_dir_ext(pin): mt_get_gpio_dir_chip(pin);
#else
	return mt_get_gpio_dir_chip(pin);
#endif
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_pull_enable(u32 pin, u32 enable)
{
	mt_gpio_pin_decrypt(&pin);

#if LK_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_set_gpio_pull_enable_ext(pin,enable): mt_set_gpio_pull_enable_chip(pin,enable);
#else
	return mt_set_gpio_pull_enable_chip(pin,enable);
#endif
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_pull_enable(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);

#if LK_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_get_gpio_pull_enable_ext(pin): mt_get_gpio_pull_enable_chip(pin);
#else
	return mt_get_gpio_pull_enable_chip(pin);
#endif
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_pull_select(u32 pin, u32 select)
{
	mt_gpio_pin_decrypt(&pin);
#if LK_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_set_gpio_pull_select_ext(pin,select): mt_set_gpio_pull_select_chip(pin,select);
#else
	return mt_set_gpio_pull_select_chip(pin,select);
#endif
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_pull_select(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);

#if LK_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_get_gpio_pull_select_ext(pin): mt_get_gpio_pull_select_chip(pin);
#else
	return mt_get_gpio_pull_select_chip(pin);
#endif
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_out(u32 pin, u32 output)
{
	mt_gpio_pin_decrypt(&pin);

#if LK_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_set_gpio_out_ext(pin,output): mt_set_gpio_out_chip(pin,output);
#else
	return mt_set_gpio_out_chip(pin,output);
#endif
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_out(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);

#if LK_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_get_gpio_out_ext(pin): mt_get_gpio_out_chip(pin);
#else
	return mt_get_gpio_out_chip(pin);
#endif
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_in(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);

#if LK_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_get_gpio_in_ext(pin): mt_get_gpio_in_chip(pin);
#else
	return mt_get_gpio_in_chip(pin);
#endif
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_mode(u32 pin, u32 mode)
{
	mt_gpio_pin_decrypt(&pin);

#if LK_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_set_gpio_mode_ext(pin,mode): mt_set_gpio_mode_chip(pin,mode);
#else
	return mt_set_gpio_mode_chip(pin,mode);
#endif
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_mode(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);

#if LK_SUPPORT_EXT_GPIO
	return (pin >= GPIO_EXTEND_START) ? mt_get_gpio_mode_ext(pin): mt_get_gpio_mode_chip(pin);
#else
	return mt_get_gpio_mode_chip(pin);
#endif
}
/*****************************************************************************/
/* sysfs operation                                                           */
/*****************************************************************************/
void mt_gpio_self_test(void)
{
	int i, val;
	for (i = 0; i < GPIO_MAX; i++) {
		S32 res,old;
		GPIOMSG("GPIO-%3d test\n", i);
		/*direction test*/
		old = mt_get_gpio_dir(i);
		if (old == 0 || old == 1) {
			GPIOLOG(" dir old = %d\n", old);
		} else {
			GPIOERR(" test dir fail: %d\n", old);
			break;
		}
		if ((res = mt_set_gpio_dir(i, GPIO_DIR_OUT)) != RSUCCESS) {
			GPIOERR(" set dir out fail: %d\n", res);
			break;
		} else if ((res = mt_get_gpio_dir(i)) != GPIO_DIR_OUT) {
			GPIOERR(" get dir out fail: %d\n", res);
			break;
		} else {
			/*output test*/
			S32 out = mt_get_gpio_out(i);
			if (out != 0 && out != 1) {
				GPIOERR(" get out fail = %d\n", old);
				break;
			}
			for (val = 0; val < GPIO_OUT_MAX; val++) {
				if ((res = mt_set_gpio_out(i,0)) != RSUCCESS) {
					GPIOERR(" set out[%d] fail: %d\n", val, res);
					break;
				} else if ((res = mt_get_gpio_out(i)) != 0) {
					GPIOERR(" get out[%d] fail: %d\n", val, res);
					break;
				}
			}
			if ((res = mt_set_gpio_out(i,out)) != RSUCCESS) {
				GPIOERR(" restore out fail: %d\n", res);
				break;
			}
		}

		if ((res = mt_set_gpio_dir(i, GPIO_DIR_IN)) != RSUCCESS) {
			GPIOERR(" set dir in fail: %d\n", res);
			break;
		} else if ((res = mt_get_gpio_dir(i)) != GPIO_DIR_IN) {
			GPIOERR(" get dir in fail: %d\n", res);
			break;
		} else {
			GPIOLOG(" input data = %d\n", res);
		}

		if ((res = mt_set_gpio_dir(i, old)) != RSUCCESS) {
			GPIOERR(" restore dir fail: %d\n", res);
			break;
		}
		for (val = 0; val < GPIO_PULL_EN_MAX; val++) {
			if ((res = mt_set_gpio_pull_enable(i,val)) != RSUCCESS) {
				GPIOERR(" set pullen[%d] fail: %d\n", val, res);
				break;
			} else if ((res = mt_get_gpio_pull_enable(i)) != val) {
				GPIOERR(" get pullen[%d] fail: %d\n", val, res);
				break;
			}
		}
		if ((res = mt_set_gpio_pull_enable(i, old)) != RSUCCESS) {
			GPIOERR(" restore pullen fail: %d\n", res);
			break;
		}

		/*pull select test*/
		old = mt_get_gpio_pull_select(i);
		if (old == 0 || old == 1)
			GPIOLOG(" pullsel old = %d\n", old);
		else {
			GPIOERR(" pullsel fail: %d\n", old);
			break;
		}
		for (val = 0; val < GPIO_PULL_MAX; val++) {
			if ((res = mt_set_gpio_pull_select(i,val)) != RSUCCESS) {
				GPIOERR(" set pullsel[%d] fail: %d\n", val, res);
				break;
			} else if ((res = mt_get_gpio_pull_select(i)) != val) {
				GPIOERR(" get pullsel[%d] fail: %d\n", val, res);
				break;
			}
		}
		if ((res = mt_set_gpio_pull_select(i, old)) != RSUCCESS) {
			GPIOERR(" restore pullsel fail: %d\n", res);
			break;
		}

		/*mode control*/
		old = mt_get_gpio_mode(i);
		if ((old >= GPIO_MODE_00) && (val < GPIO_MODE_MAX)) {
			GPIOLOG(" mode old = %d\n", old);
		} else {
			GPIOERR(" get mode fail: %d\n", old);
			break;
		}
		for (val = 0; val < GPIO_MODE_MAX; val++) {
			if ((res = mt_set_gpio_mode(i, val)) != RSUCCESS) {
				GPIOERR("set mode[%d] fail: %d\n", val, res);
				break;
			} else if ((res = mt_get_gpio_mode(i)) != val) {
				GPIOERR("get mode[%d] fail: %d\n", val, res);
				break;
			}
		}
		if ((res = mt_set_gpio_mode(i,old)) != RSUCCESS) {
			GPIOERR(" restore mode fail: %d\n", res);
			break;
		}
	}
	GPIOLOG("GPIO test done\n");
}
/*----------------------------------------------------------------------------*/
void mt_gpio_dump(void)
{
	GPIO_REGS *regs = (GPIO_REGS*)(GPIO_BASE);
	unsigned int idx;

	GPIOMSG("---# dir #-----------------------------------------------------------------\n");
	for (idx = 0; idx < sizeof(regs->dir)/sizeof(regs->dir[0]); idx++) {
		GPIOMSG("0x%04X ", regs->dir[idx].val);
		if (7 == (idx % 8)) GPIOMSG("\n");
	}
	GPIOMSG("\n---# pullen #--------------------------------------------------------------\n");
	for (idx = 0; idx < sizeof(regs->pullen)/sizeof(regs->pullen[0]); idx++) {
		GPIOMSG("0x%04X ", regs->pullen[idx].val);
		if (7 == (idx % 8)) GPIOMSG("\n");
	}
	GPIOMSG("\n---# pullsel #-------------------------------------------------------------\n");
	for (idx = 0; idx < sizeof(regs->pullsel)/sizeof(regs->pullsel[0]); idx++) {
		GPIOMSG("0x%04X ", regs->pullsel[idx].val);
		if (7 == (idx % 8)) GPIOMSG("\n");
	}
	GPIOMSG("\n---# dout #----------------------------------------------------------------\n");
	for (idx = 0; idx < sizeof(regs->dout)/sizeof(regs->dout[0]); idx++) {
		GPIOMSG("0x%04X ", regs->dout[idx].val);
		if (7 == (idx % 8)) GPIOMSG("\n");
	}
	GPIOMSG("\n---# din  #----------------------------------------------------------------\n");
	for (idx = 0; idx < sizeof(regs->din)/sizeof(regs->din[0]); idx++) {
		GPIOMSG("0x%04X ", regs->din[idx].val);
		if (7 == (idx % 8)) GPIOMSG("\n");
	}
	GPIOMSG("\n---# mode #----------------------------------------------------------------\n");
	for (idx = 0; idx < sizeof(regs->mode)/sizeof(regs->mode[0]); idx++) {
		GPIOMSG("0x%04X ", regs->mode[idx].val);
		if (7 == (idx % 8)) GPIOMSG("\n");
	}

	GPIOMSG("keypad0 0x%04X ", regs->kpad_ctrl[0].val);
	GPIOMSG("keypad1 0x%04X ", regs->kpad_ctrl[1].val);

	GPIOMSG("\n---------------------------------------------------------------------------\n");
}
/*---------------------------------------------------------------------------*/
void mt_gpio_read_pin(GPIO_CFG* cfg, int method)
{
	if (method == 0) {
		GPIO_REGS *cur = (GPIO_REGS*)GPIO_BASE;
		u32 mask = (1L << GPIO_MODE_BITS) - 1;
		int num, bit,idx=cfg->no;
		num = idx / MAX_GPIO_REG_BITS;
		bit = idx % MAX_GPIO_REG_BITS;
		cfg->pullsel= (cur->pullsel[num].val & (1L << bit)) ? (1) : (0);
		cfg->din    = (cur->din[num].val & (1L << bit)) ? (1) : (0);
		cfg->dout   = (cur->dout[num].val & (1L << bit)) ? (1) : (0);
		cfg->pullen = (cur->pullen[num].val & (1L << bit)) ? (1) : (0);
		cfg->dir    = (cur->dir[num].val & (1L << bit)) ? (1) : (0);
		num = idx / MAX_GPIO_MODE_PER_REG;
		bit = idx % MAX_GPIO_MODE_PER_REG;
		cfg->mode   = (cur->mode[num].val >> (GPIO_MODE_BITS*bit)) & mask;
	} else if (method == 1) {
		int idx=cfg->no;
		cfg->pullsel= mt_get_gpio_pull_select(idx);
		cfg->din    = mt_get_gpio_in(idx);
		cfg->dout   = mt_get_gpio_out(idx);
		cfg->pullen = mt_get_gpio_pull_enable(idx);
		cfg->dir    = mt_get_gpio_dir(idx);
		cfg->mode   = mt_get_gpio_mode(idx);
	}
}
/*---------------------------------------------------------------------------*/
void mt_gpio_dump_addr(void)
{
	unsigned int idx;
	struct mt_gpio_obj *obj = gpio_obj;
	GPIO_REGS *reg = obj->reg;

	GPIOMSG("# direction\n");
	for (idx = 0; idx < sizeof(reg->dir)/sizeof(reg->dir[0]); idx++)
		GPIOMSG("val[%2d] %p\nset[%2d] %p\nrst[%2d] %p\n", idx, &reg->dir[idx].val, idx, &reg->dir[idx].set, idx, &reg->dir[idx].rst);
	GPIOMSG("# pull enable\n");
	for (idx = 0; idx < sizeof(reg->pullen)/sizeof(reg->pullen[0]); idx++)
		GPIOMSG("val[%2d] %p\nset[%2d] %p\nrst[%2d] %p\n", idx, &reg->pullen[idx].val, idx, &reg->pullen[idx].set, idx, &reg->pullen[idx].rst);
	GPIOMSG("# pull select\n");
	for (idx = 0; idx < sizeof(reg->pullsel)/sizeof(reg->pullsel[0]); idx++)
		GPIOMSG("val[%2d] %p\nset[%2d] %p\nrst[%2d] %p\n", idx, &reg->pullsel[idx].val, idx, &reg->pullsel[idx].set, idx, &reg->pullsel[idx].rst);
	GPIOMSG("# data output\n");
	for (idx = 0; idx < sizeof(reg->dout)/sizeof(reg->dout[0]); idx++)
		GPIOMSG("val[%2d] %p\nset[%2d] %p\nrst[%2d] %p\n", idx, &reg->dout[idx].val, idx, &reg->dout[idx].set, idx, &reg->dout[idx].rst);
	GPIOMSG("# data input\n");
	for (idx = 0; idx < sizeof(reg->din)/sizeof(reg->din[0]); idx++)
		GPIOMSG("val[%2d] %p\n", idx, &reg->din[idx].val);
	GPIOMSG("# mode\n");
	for (idx = 0; idx < sizeof(reg->mode)/sizeof(reg->mode[0]); idx++)
		GPIOMSG("val[%2d] %p\nset[%2d] %p\nrst[%2d] %p\n", idx, &reg->mode[idx].val, idx, &reg->mode[idx].set, idx, &reg->mode[idx].rst);
}
#endif
