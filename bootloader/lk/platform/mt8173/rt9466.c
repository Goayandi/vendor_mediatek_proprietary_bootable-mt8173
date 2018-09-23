/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2015. All rights reserved.
*
* BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
* THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
* RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
* AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
* NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
* SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
* SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
* THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
* THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
* CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
* SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
* STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
* CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
* AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
* OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
* MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*/

#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/errno.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <platform/rt9466.h>
#include <printf.h>
#include "cust_i2c.h"
#include <platform/mt_battery.h>
#include <platform/upmu_common.h>

#define RT9466_LK_DRV_VERSION "1.0.0_MTK"

/* ================= */
/* Internal variable */
/* ================= */

enum rt9466_charging_status {
	RT9466_CHG_STATUS_READY = 0,
	RT9466_CHG_STATUS_PROGRESS,
	RT9466_CHG_STATUS_DONE,
	RT9466_CHG_STATUS_FAULT,
	RT9466_CHG_STATUS_MAX,
};

#ifdef I2C_SWITHING_CHARGER_CHANNEL
#define RT9466_I2C_ID I2C_SWITHING_CHARGER_CHANNEL
#else
#define RT9466_I2C_ID I2C1
#endif

static struct mt_i2c_t rt9466_i2c;
int g_i2c_log_level = INFO;

/* Charging status name */
static const char *rt9466_chg_status_name[RT9466_CHG_STATUS_MAX] = {
	"ready", "progress", "done", "fault",
};

/* ======================= */
/* Address & Default value */
/* ======================= */


static const unsigned char rt9466_reg_addr[] = {
	RT9466_REG_CORE_CTRL0,
	RT9466_REG_CHG_CTRL1,
	RT9466_REG_CHG_CTRL2,
	RT9466_REG_CHG_CTRL3,
	RT9466_REG_CHG_CTRL4,
	RT9466_REG_CHG_CTRL5,
	RT9466_REG_CHG_CTRL6,
	RT9466_REG_CHG_CTRL7,
	RT9466_REG_CHG_CTRL8,
	RT9466_REG_CHG_CTRL9,
	RT9466_REG_CHG_CTRL10,
	RT9466_REG_CHG_CTRL11,
	RT9466_REG_CHG_CTRL12,
	RT9466_REG_CHG_CTRL13,
	RT9466_REG_CHG_CTRL14,
	RT9466_REG_CHG_CTRL15,
	RT9466_REG_CHG_CTRL16,
	RT9466_REG_CHG_ADC,
	RT9466_REG_CHG_CTRL17,
	RT9466_REG_CHG_CTRL18,
	RT9466_REG_DEVICE_ID,
	RT9466_REG_CHG_STAT,
	RT9466_REG_CHG_NTC,
	RT9466_REG_ADC_DATA_H,
	RT9466_REG_ADC_DATA_L,
	RT9466_REG_CHG_STATC,
	RT9466_REG_CHG_FAULT,
	RT9466_REG_TS_STATC,
	RT9466_REG_CHG_IRQ1,
	RT9466_REG_CHG_IRQ2,
	RT9466_REG_CHG_IRQ3,
	RT9466_REG_CHG_STATC_CTRL,
	RT9466_REG_CHG_FAULT_CTRL,
	RT9466_REG_TS_STATC_CTRL,
	RT9466_REG_CHG_IRQ1_CTRL,
	RT9466_REG_CHG_IRQ2_CTRL,
	RT9466_REG_CHG_IRQ3_CTRL,
};


enum rt9466_iin_limit_sel {
	RT9466_IIMLMTSEL_PSEL_OTG,
	RT9466_IINLMTSEL_AICR = 2,
	RT9466_IINLMTSEL_LOWER_LEVEL, /* lower of above two */
};


/* ========================= */
/* I2C operations */
/* ========================= */

static int rt9466_i2c_write_byte(u8 cmd, u8 data)
{
	unsigned int ret = I2C_OK;
	unsigned char write_buf[2] = {cmd, data};

	/* Set up I2C transaction */
	rt9466_i2c.id = RT9466_I2C_ID;
	rt9466_i2c.addr = RT9466_SLAVE_ADDR;
	rt9466_i2c.mode = ST_MODE;
	rt9466_i2c.speed = 100;

	ret = i2c_write(&rt9466_i2c, write_buf, 2);


	if (ret != I2C_OK)
		dprintf(CRITICAL,
			"%s: I2CW[0x%02X] = 0x%02X failed, code = %d\n",
			__func__, cmd, data, ret);
	else
		dprintf(g_i2c_log_level, "%s: I2CW[0x%02X] = 0x%02X\n",
			__func__, cmd, data);

	return ret;
}

static int rt9466_i2c_read_byte(u8 cmd, u8 *data)
{
	int ret = I2C_OK;
	u8 ret_data = cmd;

	/* Set up I2C transaction */
	rt9466_i2c.id = RT9466_I2C_ID;
	rt9466_i2c.addr = RT9466_SLAVE_ADDR;
	rt9466_i2c.mode = ST_MODE;
	rt9466_i2c.speed = 100;

	ret = i2c_write_read(&rt9466_i2c, &ret_data, 1, 1);

	if (ret != I2C_OK)
		dprintf(CRITICAL, "%s: I2CR[0x%02X] failed, code = %d\n",
			__func__, cmd, ret);
	else {
		dprintf(g_i2c_log_level, "%s: I2CR[0x%02X] = 0x%02X\n",
			__func__, cmd, ret_data);
		*data = ret_data;
	}

	return ret;
}

static int rt9466_i2c_test_bit(u8 cmd, u8 shift)
{
	int ret = 0;
	u8 data = 0;

	ret = rt9466_i2c_read_byte(cmd, &data);
	if (ret != I2C_OK)
		return ret;

	ret = data & (1 << shift);

	return ret;
}

static int rt9466_i2c_update_bits(u8 cmd, u8 data, u8 mask)
{
	int ret = 0;
	u8 reg_data = 0;

	ret = rt9466_i2c_read_byte(cmd, &reg_data);
	if (ret != I2C_OK)
		return ret;

	reg_data = reg_data & 0xFF;
	reg_data &= ~mask;
	reg_data |= (data & mask);

	return rt9466_i2c_write_byte(cmd, reg_data);
}

static inline int rt9466_set_bit(u8 reg, u8 mask)
{
    return rt9466_i2c_update_bits(reg, mask, mask);
}

static inline int rt9466_clr_bit(u8 reg, u8 mask)
{
    return rt9466_i2c_update_bits(reg, 0x00, mask);
}

/* ================== */
/* internal functions */
/* ================== */
static void rt_charger_set_ichg(kal_int32 cc);
static void rt_charger_set_aicr(kal_int32 limit);
static int rt_charger_set_mivr(void *data);
static int rt_charger_get_ichg(void *data);
static int rt_charger_get_aicr(void *data);

static u8 rt9466_find_closest_reg_value(const u32 min, const u32 max,
	const u32 step, const u32 num, const u32 target)
{
	u32 i = 0, cur_val = 0, next_val = 0;

	/* Smaller than minimum supported value, use minimum one */
	if (target < min)
		return 0;

	for (i = 0; i < num - 1; i++) {
		cur_val = min + i * step;
		next_val = cur_val + step;

		if (cur_val > max)
			cur_val = max;

		if (next_val > max)
			next_val = max;

		if (target >= cur_val && target < next_val)
			return i;
	}

	/* Greater than maximum supported value, use maximum one */
	return num - 1;
}

static u8 rt9466_find_closest_reg_value_via_table(const u32 *value_table,
	const u32 table_size, const u32 target_value)
{
	u32 i = 0;

	/* Smaller than minimum supported value, use minimum one */
	if (target_value < value_table[0])
		return 0;

	for (i = 0; i < table_size - 1; i++) {
		if (target_value >= value_table[i] && target_value < value_table[i + 1])
			return i;
	}

	/* Greater than maximum supported value, use maximum one */
	return table_size - 1;
}

static u32 rt9466_find_closest_real_value(const u32 min, const u32 max,
	const u32 step, const u8 reg_val)
{
	u32 ret_val = 0;

	ret_val = min + reg_val * step;
	if (ret_val > max)
		ret_val = max;

	return ret_val;
}

/* Hardware pin current limit */
static int rt9466_enable_ilim(u8 enable)
{
	int ret = 0;

	dprintf(CRITICAL, "%s: enable ilim = %d\n", __func__, enable);

	ret = (enable ? rt9466_set_bit : rt9466_clr_bit)
		(RT9466_REG_CHG_CTRL3, RT9466_MASK_ILIM_EN);

	return ret;
}

/* Select IINLMTSEL to use AICR */
static int rt9466_select_input_current_limit(enum rt9466_iin_limit_sel sel)
{
	int ret = 0;

	dprintf(CRITICAL, "%s: select input current limit = %d\n",
		__func__, sel);

	ret = rt9466_i2c_update_bits(
		RT9466_REG_CHG_CTRL2,
		sel << RT9466_SHIFT_IINLMTSEL,
		RT9466_MASK_IINLMTSEL);

	return ret;
}

static bool rt9466_is_hw_exist(void)
{
	int ret = 0;
	u8 revision = 0;
	u8 data = 0;

	ret = rt9466_i2c_read_byte(RT9466_REG_DEVICE_ID, &data);
	if (ret != I2C_OK)
		return false;

	revision = data & 0xFF;
	if (revision == RT9466_DEVICE_ID_E2)
		dprintf(CRITICAL, "%s: RT9466 E2 revision\n", __func__);
	else if (revision == RT9466_DEVICE_ID_E3)
		dprintf(CRITICAL, "%s: RT9466 E3 revision\n", __func__);
	else if (revision == RT9466_DEVICE_ID_E4)
		dprintf(CRITICAL, "%s: RT9466 E4 revision\n", __func__);
	else
		return false;

	return true;
}

/* Set register's value to default */
static int rt9466_hw_init(void)
{
	int ret = 0;

	dprintf(CRITICAL, "%s: starts\n", __func__);

	ret = rt9466_set_bit(RT9466_REG_CORE_CTRL0, RT9466_MASK_RST);

	return ret;
}

static void rt9466_set_battery_voreg(kal_int32 cv)
{
	int ret = 0;
	u8 reg_voreg = 0;

	reg_voreg = rt9466_find_closest_reg_value(RT9466_BAT_VOREG_MIN,
		RT9466_BAT_VOREG_MAX, RT9466_BAT_VOREG_STEP,
		RT9466_BAT_VOREG_NUM, cv);

	dprintf(CRITICAL, "%s: bat voreg = %d\n", __func__, cv);

	rt9466_i2c_update_bits(
		RT9466_REG_CHG_CTRL4,
		reg_voreg << RT9466_SHIFT_BAT_VOREG,
		RT9466_MASK_BAT_VOREG
	);
}

static int rt9466_mask_all_irq()
{
	int i = 0, ret = 0;

	dprintf(CRITICAL, "%s: starts\n", __func__);
	for (i = RT9466_REG_CHG_STATC_CTRL; i <= RT9466_REG_CHG_IRQ3_CTRL; i++) {
		ret = rt9466_i2c_write_byte(i, 0xFF);
		if (ret != I2C_OK)
			dprintf(CRITICAL,
				"%s: mask irq 0x%02X failed, ret = %d\n",
				__func__, i, ret);
	}

	return ret;
}

static int rt9466_enable_watchdog_timer(bool enable)
{
	int ret = 0;

	ret = (enable ? rt9466_set_bit : rt9466_clr_bit)
		(RT9466_REG_CHG_CTRL13, RT9466_MASK_WDT_EN);

	return ret;
}

static int rt9466_sw_init(void)
{
	int ret = 0;
	u32 ichg = 2000; /* mA */
	u32 aicr = 500; /* mA */
	u32 mivr = 4500; /* mV */

	dprintf(CRITICAL, "%s: starts\n", __func__);

	/* Mask all IRQs */
	ret = rt9466_mask_all_irq();
	if (ret < 0)
		dprintf(CRITICAL, "%s: mask all irq failed\n", __func__);

	/* Disable HW iinlimit, use SW */
	ret = rt9466_enable_ilim(0);
	if (ret < 0)
		dprintf(CRITICAL, "%s: disable ilim failed\n", __func__);

	/* Select input current limit to referenced from AICR */
	ret = rt9466_select_input_current_limit(RT9466_IINLMTSEL_AICR);
	if (ret < 0)
		dprintf(CRITICAL, "%s: select input current limit failed\n",
			__func__);

	rt_charger_set_ichg(2000);

	rt_charger_set_aicr(500);


	ret = rt_charger_set_mivr(&mivr);
	if (ret < 0)
		dprintf(CRITICAL, "%s: set mivr failed\n", __func__);

	rt9466_set_battery_voreg(4200);

	ret = rt9466_enable_watchdog_timer(true);
	if (ret < 0)
		dprintf(CRITICAL, "%s: enable wdt failed\n", __func__);

	return ret;
}


static int rt9466_get_charging_status(enum rt9466_charging_status *chg_stat)
{
	int ret = 0;
	u8 data = 0;

	ret = rt9466_i2c_read_byte(RT9466_REG_CHG_STAT, &data);
	if (ret != I2C_OK)
		return ret;

	*chg_stat = (data & RT9466_MASK_CHG_STAT) >> RT9466_SHIFT_CHG_STAT;

	return ret;
}

static int rt9466_get_mivr(u32 *mivr)
{
	int ret = 0;
	u8 reg_mivr = 0;
	u8 data = 0;

	ret = rt9466_i2c_read_byte(RT9466_REG_CHG_CTRL6, &data);
	if (ret != I2C_OK)
		return ret;
	reg_mivr = ((data & RT9466_MASK_MIVR) >> RT9466_SHIFT_MIVR) & 0xFF;

	*mivr = rt9466_find_closest_real_value(RT9466_MIVR_MIN, RT9466_MIVR_MAX,
		RT9466_MIVR_STEP, reg_mivr);


	return ret;
}

static int rt9466_is_charging_enable(bool *enable)
{
	int ret = 0;
	u8 data = 0;

	ret = rt9466_i2c_read_byte(RT9466_REG_CHG_CTRL2, &data);
	if (ret != I2C_OK)
		return ret;

	if (((data & RT9466_MASK_CHG_EN) >> RT9466_SHIFT_CHG_EN) & 0xFF)
		*enable = true;
	else
		*enable = false;

	return ret;
}

static int rt9466_get_ieoc(u32 *ieoc)
{
	int ret = 0;
	u8 reg_ieoc = 0;
	u8 data = 0;

	ret = rt9466_i2c_read_byte(RT9466_REG_CHG_CTRL9, &data);
	if (ret != I2C_OK)
		return ret;

	reg_ieoc = (data & RT9466_MASK_IEOC) >> RT9466_SHIFT_IEOC;
	*ieoc = rt9466_find_closest_real_value(RT9466_IEOC_MIN, RT9466_IEOC_MAX,
		RT9466_IEOC_STEP, reg_ieoc);

	return ret;
}


/* =========================================================== */
/* The following is implementation for interface of rt_charger */
/* =========================================================== */


static void rt_charger_dump_register(void)
{
	int i = 0, ret = 0;
	u32 ichg = 0, aicr = 0, mivr = 0, ieoc = 0;
	bool chg_enable = 0;
	enum rt9466_charging_status chg_status = RT9466_CHG_STATUS_READY;
	u8 data = 0;

	ret = rt9466_get_charging_status(&chg_status);
	if (chg_status == RT9466_CHG_STATUS_FAULT) {
		g_i2c_log_level = CRITICAL;
		for (i = 0; i < ARRAY_SIZE(rt9466_reg_addr); i++) {
			ret = rt9466_i2c_read_byte(rt9466_reg_addr[i], &data);
			if (ret != I2C_OK)
				return;
		}
	} else
		g_i2c_log_level = INFO;

	ret = rt_charger_get_ichg(&ichg);
	ret = rt9466_get_mivr(&mivr);
	ret = rt_charger_get_aicr(&aicr);
	ret = rt9466_get_ieoc(&ieoc);
	ret = rt9466_is_charging_enable(&chg_enable);

	dprintf(CRITICAL,
		"%s: ICHG = %dmA, AICR = %dmA, MIVR = %dmV, IEOC = %dmA\n",
		__func__, ichg, aicr, mivr, ieoc);

	dprintf(CRITICAL, "%s: CHG_EN = %d, CHG_STATUS = %s\n",
		__func__, chg_enable, rt9466_chg_status_name[chg_status]);

}

static void rt_charger_enable_charging(kal_bool enable)
{
	(enable ? rt9466_set_bit : rt9466_clr_bit)
		(RT9466_REG_CHG_CTRL2, RT9466_MASK_CHG_EN);
}

static int rt_charger_enable_power_path(void *data)
{
	int ret = 0;
	bool enable = *((bool *)data);

	dprintf(CRITICAL, "%s: enable = %d\n", __func__, enable);
	ret = (enable ? rt9466_clr_bit : rt9466_set_bit)
		(RT9466_REG_CHG_CTRL1, RT9466_MASK_HZ_EN);

	return ret;
}

static void rt_charger_set_ichg(kal_int32 cc)
{
	/* Find corresponding reg value */
	u8 reg_ichg = rt9466_find_closest_reg_value(RT9466_ICHG_MIN,
		RT9466_ICHG_MAX, RT9466_ICHG_STEP, RT9466_ICHG_NUM, cc);

	dprintf(CRITICAL, "%s: ichg = %d\n", __func__, cc);

	rt9466_i2c_update_bits(
	          RT9466_REG_CHG_CTRL7,
	          reg_ichg << RT9466_SHIFT_ICHG,
	          RT9466_MASK_ICHG
	      );
}

static void rt_charger_set_aicr(kal_int32 limit)
{
	/* Find corresponding reg value */
	u8 reg_aicr = rt9466_find_closest_reg_value(RT9466_AICR_MIN,
		RT9466_AICR_MAX, RT9466_AICR_STEP, RT9466_AICR_NUM, limit);

	dprintf(CRITICAL, "%s: aicr = %d\n", __func__, limit);

	rt9466_i2c_update_bits(
		RT9466_REG_CHG_CTRL3,
		reg_aicr << RT9466_SHIFT_AICR,
		RT9466_MASK_AICR
	);
}


static int rt_charger_set_mivr(void *data)
{
	int ret = 0;
	u32 mivr = *((u32 *)data);

	/* Find corresponding reg value */
	u8 reg_mivr = rt9466_find_closest_reg_value(RT9466_MIVR_MIN,
		RT9466_MIVR_MAX, RT9466_MIVR_STEP, RT9466_MIVR_NUM, mivr);

	dprintf(CRITICAL, "%s: mivr = %d\n", __func__, mivr);

	ret = rt9466_i2c_update_bits(
		RT9466_REG_CHG_CTRL6,
		reg_mivr << RT9466_SHIFT_MIVR,
		RT9466_MASK_MIVR
	);

	return ret;
}


static int rt_charger_get_ichg(void *data)
{
	int ret = 0;
	u8 reg_ichg = 0;
	u8 _data = 0;

	ret = rt9466_i2c_read_byte(RT9466_REG_CHG_CTRL7, &_data);
	if (ret != I2C_OK)
		return ret;

	reg_ichg = (_data & RT9466_MASK_ICHG) >> RT9466_SHIFT_ICHG;
	*((u32 *)data) = rt9466_find_closest_real_value(RT9466_ICHG_MIN,
		RT9466_ICHG_MAX, RT9466_ICHG_STEP, reg_ichg);

	return ret;
}

static int rt_charger_get_aicr(void *data)
{
	int ret = 0;
	u8 reg_aicr = 0;
	u8 _data = 0;

	ret = rt9466_i2c_read_byte(RT9466_REG_CHG_CTRL3, &_data);
	if (ret != I2C_OK)
		return ret;

	reg_aicr = (_data & RT9466_MASK_AICR) >> RT9466_SHIFT_AICR;
	*((u32 *)data) = rt9466_find_closest_real_value(RT9466_AICR_MIN,
		RT9466_AICR_MAX, RT9466_AICR_STEP, reg_aicr);

	return ret;
}

static void rt9466_charger_init(void)
{

}

static void rt9466_charger_kick_watchdog(void)
{
	/* Any I2C communication can reset watchdog timer */
	u8 data = 0;
	rt9466_i2c_read_byte(RT9466_REG_DEVICE_ID, &data);
}

static void rt9466_charger_intf_init(void)
{
	charger.set_charging_current = rt_charger_set_ichg;
	charger.set_input_current_limit = rt_charger_set_aicr;
	charger.set_reg_voltage = rt9466_set_battery_voreg;
	charger.charging_hw_init = rt9466_charger_init;
	charger.enable_charging = rt_charger_enable_charging;
	charger.charger_dump_register = rt_charger_dump_register;
	charger.kick_ext_charger_watchdog = rt9466_charger_kick_watchdog;
}

int rt9466_probe(void)
{
	int ret = 0;

	dprintf(CRITICAL, "%s: %s\n", __func__, RT9466_LK_DRV_VERSION);
	if (!rt9466_is_hw_exist())
		return -ENODEV;

	ret = rt9466_hw_init();
	ret = rt9466_sw_init();
	rt9466_charger_intf_init();

	return ret;
}
