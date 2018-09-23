#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_pmic.h>
#include <platform/mt_i2c.h>
#include <platform/bq24196.h>
#include <platform/upmu_common.h>
#include <platform/mt_usbphy.h>
#include <platform/mt_gpt.h>
#include <printf.h>
#include "cust_i2c.h"
#include <platform/mt_battery.h>

/**********************************************************
  *   I2C Slave Setting
  *********************************************************/
#define bq24196_SLAVE_ADDR_WRITE   0xD6
#define bq24196_SLAVE_ADDR_READ    0xD7

/**********************************************************
  *   Global Variable
  *********************************************************/
#ifdef I2C_SWITHING_CHARGER_CHANNEL
#define bq24196_I2C_ID I2C_SWITHING_CHARGER_CHANNEL
#else
#define bq24196_I2C_ID 0//2
#endif

static struct mt_i2c_t bq24196_i2c;
kal_uint8 bq24196_reg[bq24196_REG_NUM] = { 0 };

#define bq24196_print(fmt, args...)   \
do {                                    \
    dprintf(CRITICAL, fmt, ##args); \
} while(0)

#define CHECK_RET(ret) \
do { \
    if (ret == 0) \
        printf("bq24196 read/write error.\n"); \
} while(0)

/**********************************************************
  *
  *   [I2C Function For Read/Write bq24196]
  *
  *********************************************************/
kal_uint32 bq24196_write_byte(kal_uint8 addr, kal_uint8 value)
{
	int ret_code = I2C_OK;
	kal_uint8 write_data[2];
	kal_uint16 len;

	write_data[0]= addr;
	write_data[1] = value;

	bq24196_i2c.id = bq24196_I2C_ID;
	/* Since i2c will left shift 1 bit, we need to set I2C address to >>1 */
	bq24196_i2c.addr = (bq24196_SLAVE_ADDR_WRITE >> 1);
	bq24196_i2c.mode = ST_MODE;
	bq24196_i2c.speed = 100;
	len = 2;

	ret_code = i2c_write(&bq24196_i2c, write_data, len);
	//bq24196_print("%s: i2c_write: ret_code: %d\n", __func__, ret_code);

	if (ret_code == 0)
		return 1; // ok
	else
		return 0; // fail
}

kal_uint32 bq24196_read_byte (kal_uint8 addr, kal_uint8 *dataBuffer)
{
	int ret_code = I2C_OK;
	kal_uint16 len;
	*dataBuffer = addr;

	bq24196_i2c.id = bq24196_I2C_ID;
	/* Since i2c will left shift 1 bit, we need to set I2C address to >>1 */
	bq24196_i2c.addr = (bq24196_SLAVE_ADDR_WRITE >> 1);
	bq24196_i2c.mode = ST_MODE;
	bq24196_i2c.speed = 100;
	len = 1;

	ret_code = i2c_write_read(&bq24196_i2c, dataBuffer, len, len);
	//bq24196_print("%s: i2c_read: ret_code: %d\n", __func__, ret_code);

	if (ret_code == 0)
		return 1; // ok
	else
		return 0; // fail
}

/**********************************************************
  *
  *   [Read / Write Function]
  *
  *********************************************************/
kal_uint32 bq24196_read_interface(kal_uint8 RegNum, kal_uint8 *val, kal_uint8 MASK,
                                  kal_uint8 SHIFT)
{
	kal_uint8 bq24196_reg = 0;
	int ret = 0;

	/* bq24196_print("--------------------------------------------------\n"); */

	ret = bq24196_read_byte(RegNum, &bq24196_reg);
	/* bq24196_print("[bq24196_read_interface] Reg[%x]=0x%x\n", RegNum, bq24196_reg); */

	bq24196_reg &= (MASK << SHIFT);
	*val = (bq24196_reg >> SHIFT);
	/* bq24196_print("[bq24196_read_interface] Val=0x%x\n", *val); */

	return ret;
}

kal_uint32 bq24196_config_interface(kal_uint8 RegNum, kal_uint8 val, kal_uint8 MASK,
                                    kal_uint8 SHIFT)
{
	kal_uint8 bq24196_reg = 0;
	int ret = 0;

	/* bq24196_print("--------------------------------------------------\n"); */

	ret = bq24196_read_byte(RegNum, &bq24196_reg);
	/* bq24196_print("[bq24196_config_interface] Reg[%x]=0x%x\n", RegNum, bq24196_reg); */

	bq24196_reg &= ~(MASK << SHIFT);
	bq24196_reg |= (val << SHIFT);

	ret = bq24196_write_byte(RegNum, bq24196_reg);
	/* bq24196_print("[bq24196_config_interface] Write Reg[%x]=0x%x\n", RegNum, bq24196_reg); */

	/* Check */
	/* bq24196_read_byte(RegNum, &bq24196_reg); */
	/* bq24196_print("[bq24196_config_interface] Check Reg[%x]=0x%x\n", RegNum, bq24196_reg); */

	return ret;
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
/* CON0---------------------------------------------------- */

void bq24196_set_en_hiz(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON0),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON0_EN_HIZ_MASK),
	                               (kal_uint8) (CON0_EN_HIZ_SHIFT)
	                              );
	CHECK_RET(ret);
}

void bq24196_set_vindpm(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON0),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON0_VINDPM_MASK),
	                               (kal_uint8) (CON0_VINDPM_SHIFT)
	                              );
	CHECK_RET(ret);
}

void bq24196_set_iinlim(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON0),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON0_IINLIM_MASK),
	                               (kal_uint8) (CON0_IINLIM_SHIFT)
	                              );
	CHECK_RET(ret);
}

kal_uint32 bq24196_get_iinlim(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;
	ret = bq24196_read_interface((kal_uint8) (bq24196_CON0),
	                             (&val),
	                             (kal_uint8) (CON0_IINLIM_MASK),
	                             (kal_uint8) (CON0_IINLIM_SHIFT)
	                            );
	CHECK_RET(ret);
	return val;
}

/* CON1---------------------------------------------------- */

void bq24196_set_reg_rst(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON1),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON1_REG_RST_MASK),
	                               (kal_uint8) (CON1_REG_RST_SHIFT)
	                              );
	CHECK_RET(ret);
}

void bq24196_set_wdt_rst(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON1),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON1_WDT_RST_MASK),
	                               (kal_uint8) (CON1_WDT_RST_SHIFT)
	                              );
	CHECK_RET(ret);
}

void bq24196_set_otg_config(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON1),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON1_OTG_CONFIG_MASK),
	                               (kal_uint8) (CON1_OTG_CONFIG_SHIFT)
	                              );
	CHECK_RET(ret);
}

void bq24196_set_chg_config(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON1),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON1_CHG_CONFIG_MASK),
	                               (kal_uint8) (CON1_CHG_CONFIG_SHIFT)
	                              );
	CHECK_RET(ret);
}

void bq24196_set_sys_min(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON1),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON1_SYS_MIN_MASK),
	                               (kal_uint8) (CON1_SYS_MIN_SHIFT)
	                              );
	CHECK_RET(ret);
}

void bq24196_set_boost_lim(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON1),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON1_BOOST_LIM_MASK),
	                               (kal_uint8) (CON1_BOOST_LIM_SHIFT)
	                              );
	CHECK_RET(ret);
}

/* CON2---------------------------------------------------- */

void bq24196_set_ichg(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON2),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON2_ICHG_MASK), (kal_uint8) (CON2_ICHG_SHIFT)
	                              );
	CHECK_RET(ret);
}

void bq24196_set_force_20pct(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON2),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON2_FORCE_20PCT_MASK),
	                               (kal_uint8) (CON2_FORCE_20PCT_SHIFT)
	                              );
	CHECK_RET(ret);
}

/* CON3---------------------------------------------------- */

void bq24196_set_iprechg(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON3),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON3_IPRECHG_MASK),
	                               (kal_uint8) (CON3_IPRECHG_SHIFT)
	                              );
	CHECK_RET(ret);
}

void bq24196_set_iterm(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON3),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON3_ITERM_MASK), (kal_uint8) (CON3_ITERM_SHIFT)
	                              );
	CHECK_RET(ret);
}

/* CON4---------------------------------------------------- */

void bq24196_set_vreg(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON4),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON4_VREG_MASK), (kal_uint8) (CON4_VREG_SHIFT)
	                              );
	CHECK_RET(ret);
}

void bq24196_set_batlowv(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON4),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON4_BATLOWV_MASK),
	                               (kal_uint8) (CON4_BATLOWV_SHIFT)
	                              );
	CHECK_RET(ret);
}

void bq24196_set_vrechg(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON4),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON4_VRECHG_MASK),
	                               (kal_uint8) (CON4_VRECHG_SHIFT)
	                              );
	CHECK_RET(ret);
}

/* CON5---------------------------------------------------- */

void bq24196_set_en_term(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON5),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON5_EN_TERM_MASK),
	                               (kal_uint8) (CON5_EN_TERM_SHIFT)
	                              );
	CHECK_RET(ret);
}

void bq24196_set_term_stat(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON5),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON5_TERM_STAT_MASK),
	                               (kal_uint8) (CON5_TERM_STAT_SHIFT)
	                              );
	CHECK_RET(ret);
}

void bq24196_set_watchdog(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON5),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON5_WATCHDOG_MASK),
	                               (kal_uint8) (CON5_WATCHDOG_SHIFT)
	                              );
	CHECK_RET(ret);
}

void bq24196_set_en_timer(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON5),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON5_EN_TIMER_MASK),
	                               (kal_uint8) (CON5_EN_TIMER_SHIFT)
	                              );
	CHECK_RET(ret);
}

void bq24196_set_chg_timer(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON5),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON5_CHG_TIMER_MASK),
	                               (kal_uint8) (CON5_CHG_TIMER_SHIFT)
	                              );
	CHECK_RET(ret);
}

/* CON6---------------------------------------------------- */

void bq24196_set_treg(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON6),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON6_TREG_MASK), (kal_uint8) (CON6_TREG_SHIFT)
	                              );
	CHECK_RET(ret);
}

/* CON7---------------------------------------------------- */
kal_uint32 bq24196_get_dpdm_status(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24196_read_interface((kal_uint8) (bq24196_CON7),
	                             (&val),
	                             (kal_uint8) (CON7_DPDM_EN_MASK),
	                             (kal_uint8) (CON7_DPDM_EN_SHIFT)
	                            );
	CHECK_RET(ret);
	return val;
}

void bq24196_set_dpdm_en(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON7),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON7_DPDM_EN_MASK),
	                               (kal_uint8) (CON7_DPDM_EN_SHIFT)
	                              );
	CHECK_RET(ret);
}

void bq24196_set_tmr2x_en(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON7),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON7_TMR2X_EN_MASK),
	                               (kal_uint8) (CON7_TMR2X_EN_SHIFT)
	                              );
	CHECK_RET(ret);
}

void bq24196_set_batfet_disable(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON7),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON7_BATFET_Disable_MASK),
	                               (kal_uint8) (CON7_BATFET_Disable_SHIFT)
	                              );
	CHECK_RET(ret);
}

void bq24196_set_int_mask(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24196_config_interface((kal_uint8) (bq24196_CON7),
	                               (kal_uint8) (val),
	                               (kal_uint8) (CON7_INT_MASK_MASK),
	                               (kal_uint8) (CON7_INT_MASK_SHIFT)
	                              );
	CHECK_RET(ret);
}

/* CON8---------------------------------------------------- */

kal_uint32 bq24196_get_system_status(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24196_read_interface((kal_uint8) (bq24196_CON8),
	                             (&val), (kal_uint8) (0xFF), (kal_uint8) (0x0)
	                            );
	CHECK_RET(ret);
	return val;
}

kal_uint32 bq24196_get_vbus_stat(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24196_read_interface((kal_uint8) (bq24196_CON8),
	                             (&val),
	                             (kal_uint8) (CON8_VBUS_STAT_MASK),
	                             (kal_uint8) (CON8_VBUS_STAT_SHIFT)
	                            );
	CHECK_RET(ret);
	return val;
}

kal_uint32 bq24196_get_chrg_stat(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24196_read_interface((kal_uint8) (bq24196_CON8),
	                             (&val),
	                             (kal_uint8) (CON8_CHRG_STAT_MASK),
	                             (kal_uint8) (CON8_CHRG_STAT_SHIFT)
	                            );
	CHECK_RET(ret);
	return val;
}

kal_uint32 bq24196_get_pg_stat(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24196_read_interface((kal_uint8) (bq24196_CON8),
	                             (&val),
	                             (kal_uint8) (CON8_PG_STAT_MASK),
	                             (kal_uint8) (CON8_PG_STAT_SHIFT)
	                            );
	CHECK_RET(ret);
	return val;
}

kal_uint32 bq24196_get_vsys_stat(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24196_read_interface((kal_uint8) (bq24196_CON8),
	                             (&val),
	                             (kal_uint8) (CON8_VSYS_STAT_MASK),
	                             (kal_uint8) (CON8_VSYS_STAT_SHIFT)
	                            );
	CHECK_RET(ret);
	return val;
}

/* CON10---------------------------------------------------- */
kal_uint32 bq24196_get_pn(void)
{
	kal_uint8 val = 0;
	kal_uint32 ret = 0;
	ret = bq24196_read_interface((kal_uint8) (bq24196_CON10),
	                             (&val),
	                             (kal_uint8) (CON10_PN_MASK),
	                             (kal_uint8) (CON10_PN_SHIFT)
	                            );

	if (ret)
		return val;
	else
		return 0;
}

/**********************************************************
  *
  *   [Utility Function]
  *
  *********************************************************/
void bq24196_dump_register(void)
{
	int i = 0;
	for (i = 0; i < bq24196_REG_NUM; i++) {
		bq24196_read_byte(i, &bq24196_reg[i]);
		bq24196_print("[bq24196_dump_register] Reg[0x%X]=0x%X\n", i, bq24196_reg[i]);
	}
}

static void bq_set_charging_current(kal_int32 cc_value)
{
	if (cc_value > 2484 || cc_value < 500) {
		bq24196_print("invalid current selected (%d), use 500mA \r\n", cc_value);
		bq24196_set_ichg(0);
	} else
		bq24196_set_ichg((cc_value-512)/64);
}

static void bq_set_input_current_limit(kal_int32 input_limit)
{
	switch (input_limit) {
		case 500:
			bq24196_set_iinlim(0x2);
			break;
		case 2000:
			bq24196_set_iinlim(0x6);
			break;
		default:
			bq24196_set_iinlim(0x2);
	}
}

static void bq_set_reg_voltage(kal_int32 cv)
{
	if (cv == 4200)
		bq24196_set_vreg(0x2C); //VREG 4.208V
	else if (cv == 4000)
		bq24196_set_vreg(0x1F); //VREG 4.0V
	else
		bq24196_set_vreg(0x2C); //VREG 4.208V
}

static void bq_charging_hw_init(void)
{
	upmu_set_rg_bc11_bb_ctrl(1);    /* BC11_BB_CTRL */
	upmu_set_rg_bc11_rst(1);    /* BC11_RST */

	bq24196_set_en_hiz(0x0);
	bq24196_set_vindpm(0x9);    /* VIN DPM check 4.60V */
	bq24196_set_reg_rst(0x0);
	bq24196_set_wdt_rst(0x1);   /* Kick watchdog */
	bq24196_set_sys_min(0x5);   /* Minimum system voltage 3.5V */
	bq24196_set_iterm(0x0); /* Termination current 128mA */

#if defined(MTK_JEITA_STANDARD_SUPPORT)
	bq24196_set_iprechg(0x1);   /* Precharge current 256mA */
#else
	bq24196_set_iprechg(0x3);   /* Precharge current 512mA */
	bq24196_set_vreg(0x2C); //VREG 4.208V
	bq24196_set_en_timer(0x0);  /* Disable charge timer */
#endif

	bq24196_set_batlowv(0x1);   /* BATLOWV 3.0V */
	bq24196_set_vrechg(0x0);    /* VRECHG 0.1V (4.108V) */
	bq24196_set_en_term(0x1);   /* Enable termination */
	bq24196_set_term_stat(0x0); /* Match ITERM */
	bq24196_set_watchdog(0x1);  /* WDT 40s */
	bq24196_set_int_mask(0x1);  /* Disable fault interrupt */
}

static void bq_enable_charging(kal_bool enable)
{
	if (enable == KAL_FALSE) {
		bq24196_set_wdt_rst(0x1);
		bq24196_set_chg_config(0x0);
	} else {
		bq24196_set_en_hiz(0x0);
		bq24196_set_chg_config(0x1);
	}
}

static void bq_charger_dump_register(void)
{
	bq24196_dump_register();
}

static void bq_kick_ext_charger_watchdog(void)
{
	bq24196_set_wdt_rst(0x1);
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

void probe_bq24196(void)
{
	if (bq24196_get_pn() == 0x5) {
		register_charger_control();
		bq24196_print("find bq24196 device and register charger control.\n");
	} else
		bq24196_print("no bq24196 device is found.\n");
}

