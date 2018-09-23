#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_pmic.h>
#include <platform/mt_i2c.h>
#include <platform/bq24261.h>
#include <platform/upmu_common.h>
#include <platform/mt_usbphy.h>
#include <platform/mt_gpt.h>
#include <printf.h>
#include "cust_i2c.h"
#include <platform/mt_battery.h>

#define STATUS_OK   0
#define STATUS_UNSUPPORTED  -1

/**********************************************************
  *
  *   [I2C Slave Setting]
  *
  *********************************************************/
#define bq24261_SLAVE_ADDR_WRITE   0xD6
#define bq24261_SLAVE_ADDR_Read    0xD7

#ifdef I2C_SWITHING_CHARGER_CHANNEL
#define bq24261_I2C_ID I2C_SWITHING_CHARGER_CHANNEL
#else
#define bq24261_I2C_ID 0
#endif

#define bq24261_print(fmt, args...)   \
do {                                    \
    dprintf(CRITICAL, fmt, ##args); \
} while(0)

/**********************************************************
  *
  *   [Global Variable]
  *
  *********************************************************/
static struct mt_i2c_t bq24261_i2c;
static kal_uint8 bq24261_reg[bq24261_REG_NUM] = { 0 };
static const kal_uint32 INPUT_CS_VTH[] = { 100, 150, 500, 900, 1500, 1950, 2500, 2000 };

/**********************************************************
  *
  *   [I2C Function For Read/Write bq24261]
  *
  *********************************************************/
kal_uint32 bq24261_write_byte(kal_uint8 addr, kal_uint8 value)
{
	int ret_code = I2C_OK;
	kal_uint8 write_data[2];
	kal_uint16 len;

	write_data[0]= addr;
	write_data[1] = value;

	bq24261_i2c.id = bq24261_I2C_ID;
	bq24261_i2c.addr = (bq24261_SLAVE_ADDR_WRITE >> 1);
	bq24261_i2c.mode = ST_MODE;
	bq24261_i2c.speed = 100;
	len = 2;

	ret_code = i2c_write(&bq24261_i2c, write_data, len);

	if (ret_code == 0)
		return 1; // ok
	else
		return 0; // fail
}

kal_uint32 bq24261_read_byte (kal_uint8 addr, kal_uint8 *dataBuffer)
{
	int ret_code = I2C_OK;
	kal_uint16 len;
	*dataBuffer = addr;

	bq24261_i2c.id = bq24261_I2C_ID;
	bq24261_i2c.addr = (bq24261_SLAVE_ADDR_WRITE >> 1);
	bq24261_i2c.mode = ST_MODE;
	bq24261_i2c.speed = 100;
	len = 1;

	ret_code = i2c_write_read(&bq24261_i2c, dataBuffer, len, len);

	if (ret_code == 0)
		return 1; // ok
	else
		return 0; // fail
}

kal_uint32 bq24261_read_interface(kal_uint8 RegNum, kal_uint8 *val, kal_uint8 MASK,
                                  kal_uint8 SHIFT)
{
	kal_uint8 bq24261_reg = 0;
	kal_uint32 ret = 0;

	ret = bq24261_read_byte(RegNum, &bq24261_reg);

	bq24261_reg &= (MASK << SHIFT);
	*val = (bq24261_reg >> SHIFT);

	return ret;
}

kal_uint32 bq24261_config_interface(kal_uint8 RegNum, kal_uint8 val, kal_uint8 MASK,
                                    kal_uint8 SHIFT)
{
	kal_uint8 bq24261_reg = 0;
	kal_uint32 ret = 0;

	ret = bq24261_read_byte(RegNum, &bq24261_reg);

	bq24261_reg &= ~(MASK << SHIFT);
	bq24261_reg |= (val << SHIFT);

	if (RegNum == bq24261_CON1 && val == 1 && MASK == CON1_RESET_MASK
	        && SHIFT == CON1_RESET_SHIFT) {
		/* read RESET bit */
	} else if (RegNum == bq24261_CON1)
		bq24261_reg &= ~0x80;   /* RESET bit read always return 1, need clear it */

	ret = bq24261_write_byte(RegNum, bq24261_reg);

	return ret;
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
/* CON0---------------------------------------------------- */

void bq24261_set_tmr_rst(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24261_config_interface((kal_uint8) (bq24261_CON0),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON0_TMR_RST_MASK),
	                               (kal_uint8) (CON0_TMR_RST_SHIFT)
	                              );
}

void bq24261_set_en_boost(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24261_config_interface((kal_uint8) (bq24261_CON0),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON0_EN_BOOST_MASK),
	                               (kal_uint8) (CON0_EN_BOOST_SHIFT)
	                              );
}

kal_uint32 bq24261_get_stat(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24261_read_interface((kal_uint8) (bq24261_CON0),
	                             (&val),
	                             (kal_uint8) (CON0_STAT_MASK), (kal_uint8) (CON0_STAT_SHIFT)
	                            );
	return val;
}

void bq24261_set_en_shipmode(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24261_config_interface((kal_uint8) (bq24261_CON0),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON0_EN_SHIPMODE_MASK),
	                               (kal_uint8) (CON0_EN_SHIPMODE_SHIFT)
	                              );
}

kal_uint32 bq24261_get_fault(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24261_read_interface((kal_uint8) (bq24261_CON0),
	                             (&val),
	                             (kal_uint8) (CON0_FAULT_MASK), (kal_uint8) (CON0_FAULT_SHIFT)
	                            );
	return val;
}

/* CON1---------------------------------------------------- */

void bq24261_set_reset(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24261_config_interface((kal_uint8) (bq24261_CON1),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON1_RESET_MASK), (kal_uint8) (CON1_RESET_SHIFT)
	                              );
}

kal_uint32 bq24261_get_in_limit(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24261_read_interface((kal_uint8) (bq24261_CON1),
	                             (&val),
	                             (kal_uint8) (CON1_IN_LIMIT_MASK),
	                             (kal_uint8) (CON1_IN_LIMIT_SHIFT)
	                            );
	return val;
}

void bq24261_set_in_limit(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24261_config_interface((kal_uint8) (bq24261_CON1),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON1_IN_LIMIT_MASK),
	                               (kal_uint8) (CON1_IN_LIMIT_SHIFT)
	                              );
}

void bq24261_set_en_stat(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24261_config_interface((kal_uint8) (bq24261_CON1),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON1_EN_STAT_MASK),
	                               (kal_uint8) (CON1_EN_STAT_SHIFT)
	                              );
}

void bq24261_set_te(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24261_config_interface((kal_uint8) (bq24261_CON1),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON1_TE_MASK), (kal_uint8) (CON1_TE_SHIFT)
	                              );
}

void bq24261_set_dis_ce(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24261_config_interface((kal_uint8) (bq24261_CON1),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON1_DIS_CE_MASK),
	                               (kal_uint8) (CON1_DIS_CE_SHIFT)
	                              );
}

void bq24261_set_hz_mode(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24261_config_interface((kal_uint8) (bq24261_CON1),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON1_HZ_MODE_MASK),
	                               (kal_uint8) (CON1_HZ_MODE_SHIFT)
	                              );
}

/* CON2---------------------------------------------------- */

void bq24261_set_vbreg(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24261_config_interface((kal_uint8) (bq24261_CON2),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON2_VBREG_MASK), (kal_uint8) (CON2_VBREG_SHIFT)
	                              );
}

void bq24261_set_mod_freq(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24261_config_interface((kal_uint8) (bq24261_CON2),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON2_MOD_FREQ_MASK),
	                               (kal_uint8) (CON2_MOD_FREQ_SHIFT)
	                              );
}

/* CON3---------------------------------------------------- */

kal_uint32 bq24261_get_vender_code(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24261_read_interface((kal_uint8) (bq24261_CON3),
	                             (&val),
	                             (kal_uint8) (CON3_VENDER_CODE_MASK),
	                             (kal_uint8) (CON3_VENDER_CODE_SHIFT)
	                            );
	return val;
}

kal_uint32 bq24261_get_pn(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24261_read_interface((kal_uint8) (bq24261_CON3),
	                             (&val), (kal_uint8) (CON3_PN_MASK), (kal_uint8) (CON3_PN_SHIFT)
	                            );
	return val;
}

/* CON4---------------------------------------------------- */

kal_uint32 bq24261_get_ichg(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24261_read_interface((kal_uint8) (bq24261_CON4),
	                             (&val),
	                             (kal_uint8) (CON4_ICHRG_MASK), (kal_uint8) (CON4_ICHRG_SHIFT)
	                            );
	return val;
}

void bq24261_set_ichg(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24261_config_interface((kal_uint8) (bq24261_CON4),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON4_ICHRG_MASK), (kal_uint8) (CON4_ICHRG_SHIFT)
	                              );
}

void bq24261_set_iterm(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24261_config_interface((kal_uint8) (bq24261_CON4),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON4_ITERM_MASK), (kal_uint8) (CON4_ITERM_SHIFT)
	                              );
}

/* CON5---------------------------------------------------- */

kal_uint32 bq24261_get_minsys_status(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24261_read_interface((kal_uint8) (bq24261_CON5),
	                             (&val),
	                             (kal_uint8) (CON5_MINSYS_STATUS_MASK),
	                             (kal_uint8) (CON5_MINSYS_STATUS_SHIFT)
	                            );
	return val;
}

kal_uint32 bq24261_get_vindpm_status(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24261_read_interface((kal_uint8) (bq24261_CON5),
	                             (&val),
	                             (kal_uint8) (CON5_VINDPM_STATUS_MASK),
	                             (kal_uint8) (CON5_VINDPM_STATUS_SHIFT)
	                            );
	return val;
}

kal_uint32 bq24261_get_low_chg(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24261_read_interface((kal_uint8) (bq24261_CON5),
	                             (&val),
	                             (kal_uint8) (CON5_LOW_CHG_MASK),
	                             (kal_uint8) (CON5_LOW_CHG_SHIFT)
	                            );
	return val;
}

void bq24261_set_low_chg(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24261_config_interface((kal_uint8) (bq24261_CON5),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON5_LOW_CHG_MASK),
	                               (kal_uint8) (CON5_LOW_CHG_SHIFT)
	                              );
}

void bq24261_set_dpdm_en(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24261_config_interface((kal_uint8) (bq24261_CON5),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON5_DPDM_EN_MASK),
	                               (kal_uint8) (CON5_DPDM_EN_SHIFT)
	                              );
}

kal_uint32 bq24261_get_cd_status(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24261_read_interface((kal_uint8) (bq24261_CON5),
	                             (&val),
	                             (kal_uint8) (CON5_CD_STATUS_MASK),
	                             (kal_uint8) (CON5_CD_STATUS_SHIFT)
	                            );
	return val;
}

void bq24261_set_vindpm(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24261_config_interface((kal_uint8) (bq24261_CON5),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON5_VINDPM_MASK),
	                               (kal_uint8) (CON5_VINDPM_SHIFT)
	                              );
}

/* CON6---------------------------------------------------- */

void bq24261_set_2xtmr_en(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24261_config_interface((kal_uint8) (bq24261_CON6),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON6_2XTMR_EN_MASK),
	                               (kal_uint8) (CON6_2XTMR_EN_SHIFT)
	                              );
}

void bq24261_set_tmr(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24261_config_interface((kal_uint8) (bq24261_CON6),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON6_TMR_MASK), (kal_uint8) (CON6_TMR_SHIFT)
	                              );
}

void bq24261_set_boost_ilim(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24261_config_interface((kal_uint8) (bq24261_CON6),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON6_BOOST_ILIM_MASK),
	                               (kal_uint8) (CON6_BOOST_ILIM_SHIFT)
	                              );
}

void bq24261_set_ts_en(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24261_config_interface((kal_uint8) (bq24261_CON6),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON6_TS_EN_MASK), (kal_uint8) (CON6_TS_EN_SHIFT)
	                              );
}

kal_uint32 bq24261_get_ts_fault(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24261_read_interface((kal_uint8) (bq24261_CON6),
	                             (&val),
	                             (kal_uint8) (CON6_TS_FAULT_MASK),
	                             (kal_uint8) (CON6_TS_FAULT_SHIFT)
	                            );
	return val;
}

void bq24261_set_vindpm_off(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24261_config_interface((kal_uint8) (bq24261_CON6),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON6_VINDPM_OFF_MASK),
	                               (kal_uint8) (CON6_VINDPM_OFF_SHIFT)
	                              );
}

/**********************************************************
  *
  *   [Utility Function]
  *
  *********************************************************/

void bq24261_dump_register(void)
{
	int i = 0;
	for (i = 0; i < bq24261_REG_NUM; i++) {
		bq24261_read_byte(i, &bq24261_reg[i]);
		bq24261_print("[bq24261_dump_register] Reg[0x%X]=0x%X\n", i, bq24261_reg[i]);
	}
}

static void bq_set_charging_current(kal_int32 cc_value)
{
	kal_uint32 register_value;
	if (cc_value <= 500)
		register_value = 0;
	else
		register_value = (cc_value - 500) / 100;
	bq24261_set_ichg(register_value);
}

static void bq_set_input_current_limit(kal_int32 input_limit)
{
	kal_uint32 array_size;
	kal_uint32 register_value;

	if (input_limit >= 2500)
		register_value = 0x6;
	else if (input_limit == 2000)
		register_value = 0x7;
	else {
		array_size = ARRAY_SIZE(INPUT_CS_VTH);
		for (register_value = 0; register_value < array_size; register_value++)
			if (INPUT_CS_VTH[register_value] >= input_limit)
				break;
	}

	bq24261_set_in_limit(register_value);
}

static void bq_set_reg_voltage(kal_int32 cv)
{
	kal_uint16 register_value;

	register_value = (cv - 3500) / 20;
	bq24261_set_vbreg(register_value);
}

static void bq_charging_hw_init(void)
{
	upmu_set_rg_bc11_bb_ctrl(1);    /* BC11_BB_CTRL */
	upmu_set_rg_bc11_rst(1);    /* BC11_RST */

	bq24261_set_tmr_rst(1); /* wdt reset */
	bq24261_set_en_boost(0);    /* OTG boost */
	bq24261_set_tmr(0x3);   /* default disable safty timer */
	bq24261_set_iterm(0x3); /* iterm 200mA */

	bq24261_set_vindpm_off(0);  /* 4.2V offset */
	bq24261_set_vindpm(0x3);    /* 4.452 VINDPM */
	bq24261_set_ts_en(0);   /* disable TS function */
}

static void bq_enable_charging(kal_bool enable)
{
	if (KAL_TRUE == enable) {
		bq24261_set_hz_mode(0x0);
		bq24261_set_dis_ce(0);
	} else
		bq24261_set_dis_ce(0x1);
}

static void bq_charger_dump_register(void)
{
	bq24261_dump_register();
}

static void bq_kick_ext_charger_watchdog(void)
{
	bq24261_set_tmr_rst(1);
}

static void register_charger_control(void)
{
	charger.set_charging_current = bq_set_charging_current;
	charger.set_input_current_limit = bq_set_input_current_limit;
	charger.set_reg_voltage = bq_set_reg_voltage;
	charger.charging_hw_init = bq_charging_hw_init;
	charger.enable_charging = bq_enable_charging;
	charger.charger_dump_register = bq_charger_dump_register;
	charger.kick_ext_charger_watchdog = bq_kick_ext_charger_watchdog;
}

void probe_bq24261(void)
{
	if (bq24261_get_pn() == 0x0 && bq24261_get_vender_code() == 0x2) {
		register_charger_control();
		bq24261_print("find BQ24261 device and register charger control.\n");
	} else
		bq24261_print("no BQ24261 device is found.\n");
}

