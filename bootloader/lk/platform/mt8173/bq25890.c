#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_pmic.h>
#include <platform/mt_i2c.h>
#include <platform/bq25890.h>
#include <platform/upmu_common.h>
#include <platform/mt_usbphy.h>
#include <platform/mt_gpt.h>
#include <printf.h>
#include "cust_i2c.h"
#include <platform/mt_battery.h>


/**********************************************************
 *
 *    [Define]
 *
 **********************************************************/
#if defined(MTK_BQ25890_SUPPORT)//BQ25892
#define bq25890_SLAVE_ADDR_WRITE   0xD6 
#define bq25890_SLAVE_ADDR_READ    0xD7
#endif

#if defined(MTK_BQ25896_SUPPORT)
#define bq25890_SLAVE_ADDR_WRITE   0xD6
#define bq25890_SLAVE_ADDR_READ    0xD7
#endif

/**********************************************************
  *
  *   [Global Variable]
  *
  *********************************************************/
#ifdef I2C_SWITHING_CHARGER_CHANNEL
#define bq25890_I2C_ID I2C_SWITHING_CHARGER_CHANNEL
#else
#define bq25890_I2C_ID 0
#endif

static struct mt_i2c_t bq25890_i2c;
static kal_uint8 bq25890_reg[bq25890_REG_NUM] = { 0 };

#define bq25890_print(fmt, args...)   \
do {                                    \
    printf(fmt, ##args); \
} while(0)

/**********************************************************
  *
  *   [I2C Function For Read/Write bq25890]
  *
  *********************************************************/
kal_uint32 bq25890_write_byte(kal_uint8 addr, kal_uint8 value)
{
	int ret_code = I2C_OK;
	kal_uint8 write_data[2];
	kal_uint16 len;

	write_data[0]= addr;
	write_data[1] = value;

	bq25890_i2c.id = bq25890_I2C_ID;
	/* Since i2c will left shift 1 bit, we need to set I2C address to >>1 */
	bq25890_i2c.addr = (bq25890_SLAVE_ADDR_WRITE >> 1);
	bq25890_i2c.mode = ST_MODE;
	bq25890_i2c.speed = 100;
	len = 2;

	ret_code = i2c_write(&bq25890_i2c, write_data, len);
	if (ret_code == 0)
		return 1; // ok
	else
		return 0; // fail
}

kal_uint32 bq25890_read_byte (kal_uint8 addr, kal_uint8 *dataBuffer)
{
	int ret_code = I2C_OK;
	kal_uint16 len;
	*dataBuffer = addr;

	bq25890_i2c.id = bq25890_I2C_ID;
	/* Since i2c will left shift 1 bit, we need to set I2C address to >>1 */
	bq25890_i2c.addr = (bq25890_SLAVE_ADDR_WRITE >> 1);
	bq25890_i2c.mode = ST_MODE;
	bq25890_i2c.speed = 100;
	len = 1;

	ret_code = i2c_write_read(&bq25890_i2c, dataBuffer, len, len);
	if (ret_code == 0)
		return 1; // ok
	else
		return 0; // fail
}


kal_uint32 bq25890_read_interface(kal_uint8 RegNum, kal_uint8 *val, kal_uint8 MASK,
                                  kal_uint8 SHIFT)
{
	kal_uint8 bq25890_reg = 0;
	int ret = 0;

	ret = bq25890_read_byte(RegNum, &bq25890_reg);

	bq25890_reg &= (MASK << SHIFT);
	*val = (bq25890_reg >> SHIFT);

	return ret;
}

kal_uint32 bq25890_config_interface(kal_uint8 RegNum, kal_uint8 val, kal_uint8 MASK,
                                    kal_uint8 SHIFT)
{
	kal_uint8 bq25890_reg = 0;
	int ret = 0;

	ret = bq25890_read_byte(RegNum, &bq25890_reg);

	bq25890_reg &= ~(MASK << SHIFT);
	bq25890_reg |= (val << SHIFT);

	ret = bq25890_write_byte(RegNum, bq25890_reg);
	return ret;
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/

/* CON0---------------------------------------------------- */
void bq25890_set_en_hiz(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON0),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON0_EN_HIZ_MASK),
	                         (kal_uint8) (CON0_EN_HIZ_SHIFT)
	                        );
}

void bq25890_set_iinlim(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON0),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON0_IINLIM_MASK),
	                         (kal_uint8) (CON0_IINLIM_SHIFT)
	                        );
}

kal_uint32 bq25890_get_iinlim(void)
{
	kal_uint8 val = 0;
	bq25890_read_interface((kal_uint8) (bq25890_CON0),
	                       (&val),
	                       (kal_uint8) (CON0_IINLIM_MASK), (kal_uint8) (CON0_IINLIM_SHIFT)
	                      );
	return val;
}

/* CON1---------------------------------------------------- */

/* CON2---------------------------------------------------- */
void bq25890_set_force_dpdm(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON2),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON2_FORCE_DPDM_MASK),
	                         (kal_uint8) (CON2_FORCE_DPDM_SHIFT)
	                        );
}

void bq25890_set_auto_dpdm_en(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON2),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON2_AUTO_DPDM_MASK),
	                         (kal_uint8) (CON2_AUTO_DPDM_SHIFT)
	                        );
}

kal_uint32 bq25890_get_dpdm_status(void)
{
	kal_uint8 val = 0;
	bq25890_read_interface((kal_uint8) (bq25890_CON2),
	                       (&val),
	                       (kal_uint8) (CON2_FORCE_DPDM_MASK),
	                       (kal_uint8) (CON2_FORCE_DPDM_SHIFT)
	                      );
	return val;
}

/* CON3---------------------------------------------------- */
void bq25890_set_wdt_rst(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON3),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON3_WDT_RST_MASK),
	                         (kal_uint8) (CON3_WDT_RST_SHIFT)
	                        );
}

void bq25890_set_otg_config(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON3),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON3_OTG_CONFIG_MASK),
	                         (kal_uint8) (CON3_OTG_CONFIG_SHIFT)
	                        );
}

void bq25890_set_chg_config(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON3),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON3_CHG_CONFIG_MASK),
	                         (kal_uint8) (CON3_CHG_CONFIG_SHIFT)
	                        );
}

void bq25890_set_sys_min(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON3),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON3_SYS_MIN_MASK),
	                         (kal_uint8) (CON3_SYS_MIN_SHIFT)
	                        );
}

/* CON4---------------------------------------------------- */
void bq25890_set_ichg(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON4),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON4_ICHG_MASK),
	                         (kal_uint8) (CON4_ICHG_SHIFT)
	                        );
}

kal_uint32 bq25890_get_ichg(void)
{
	kal_uint8 val = 0;
	bq25890_read_interface((kal_uint8) (bq25890_CON4),
	                       (&val),
	                       (kal_uint8) (CON4_ICHG_MASK),
	                       (kal_uint8) (CON4_ICHG_SHIFT)
	                      );
	return val;
}
/* CON5---------------------------------------------------- */
void bq25890_set_iprechg(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON5),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON5_IPRECHG_MASK),
	                         (kal_uint8) (CON5_IPRECHG_SHIFT)
	                        );
}

void bq25890_set_iterm(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON5),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON5_ITERM_MASK),
	                         (kal_uint8) (CON5_ITERM_SHIFT)
	                        );
}

/* CON6---------------------------------------------------- */
void bq25890_set_vreg(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON6),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON6_VREG_MASK),
	                         (kal_uint8) (CON6_VREG_SHIFT)
	                        );
}

void bq25890_set_batlowv(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON6),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON6_BATLOWV_MASK),
	                         (kal_uint8) (CON6_BATLOWV_SHIFT)
	                        );
}

void bq25890_set_vrechg(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON6),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON6_VRECHG_MASK),
	                         (kal_uint8) (CON6_VRECHG_SHIFT)
	                        );
}

/* CON7---------------------------------------------------- */
void bq25890_set_en_term(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON7),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON7_EN_TERM_MASK),
	                         (kal_uint8) (CON7_EN_TERM_SHIFT)
	                        );
}

void bq25890_set_watchdog(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON7),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON7_WATCHDOG_MASK),
	                         (kal_uint8) (CON7_WATCHDOG_SHIFT)
	                        );
}

void bq25890_set_en_timer(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON7),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON7_EN_TIMER_MASK),
	                         (kal_uint8) (CON7_EN_TIMER_SHIFT)
	                        );
}

void bq25890_set_chg_timer(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON7),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON7_CHG_TIMER_MASK),
	                         (kal_uint8) (CON7_CHG_TIMER_SHIFT)
	                        );
}

/* CON8---------------------------------------------------- */
void bq25890_set_treg(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON8),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON8_TREG_MASK),
	                         (kal_uint8) (CON8_TREG_SHIFT)
	                        );
}

/* CON9---------------------------------------------------- */
void bq25890_set_batfet_disable(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON9),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON9_BATFET_DIS_MASK),
	                         (kal_uint8) (CON9_BATFET_DIS_SHIFT)
	                        );
}

/* CON10---------------------------------------------------- */
void bq25890_set_boost_lim(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON10),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON10_BOOST_LIM_MASK),
	                         (kal_uint8) (CON10_BOOST_LIM_SHIFT)
	                        );
}

/* CON11---------------------------------------------------- */
kal_uint32 bq25890_get_vbus_stat(void)
{
	kal_uint8 val = 0;
	bq25890_read_interface((kal_uint8) (bq25890_CON11),
	                       (&val),
	                       (kal_uint8) (CON11_VBUS_STAT_MASK),
	                       (kal_uint8) (CON11_VBUS_STAT_SHIFT)
	                      );
	return val;
}

kal_uint32 bq25890_get_chrg_stat(void)
{
	kal_uint8 val = 0;
	bq25890_read_interface((kal_uint8) (bq25890_CON11),
	                       (&val),
	                       (kal_uint8) (CON11_CHRG_STAT_MASK),
	                       (kal_uint8) (CON11_CHRG_STAT_SHIFT)
	                      );
	return val;
}

kal_uint32 bq25890_get_pg_stat(void)
{
	kal_uint8 val = 0;
	bq25890_read_interface((kal_uint8) (bq25890_CON11),
	                       (&val),
	                       (kal_uint8) (CON11_PG_STAT_MASK),
	                       (kal_uint8) (CON11_PG_STAT_SHIFT)
	                      );
	return val;
}

kal_uint32 bq25890_get_vsys_stat(void)
{
	kal_uint8 val = 0;
	bq25890_read_interface((kal_uint8) (bq25890_CON11),
	                       (&val),
	                       (kal_uint8) (CON11_VSYS_STAT_MASK),
	                       (kal_uint8) (CON11_VSYS_STAT_SHIFT)
	                      );
	return val;
}

kal_uint32 bq25890_get_system_status(void)
{
	kal_uint8 val = 0;
	bq25890_read_interface((kal_uint8) (bq25890_CON11),
	                       (&val),
	                       (kal_uint8) (0xFF),
	                       (kal_uint8) (0x0)
	                      );
	return val;
}

/* CON13---------------------------------------------------- */
void bq25890_set_force_vindpm(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON13),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON13_FORCE_VINDPM_MASK),
	                         (kal_uint8) (CON13_FORCE_VINDPM_SHIFT)
	                        );
}

void bq25890_set_vindpm(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON13),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON13_VINDPM_MASK),
	                         (kal_uint8) (CON13_VINDPM_SHIFT)
	                        );
}

/* CON19---------------------------------------------------- */
kal_uint32 bq25890_get_vdpm_stat(void)
{
	kal_uint8 val = 0;
	bq25890_read_interface((kal_uint8) (bq25890_CON19),
	                       (&val),
	                       (kal_uint8) (CON19_VDPM_STAT_MASK),
	                       (kal_uint8) (CON19_VDPM_STAT_SHIFT)
	                      );
	return val;
}

kal_uint32 bq25890_get_idpm_stat(void)
{
	kal_uint8 val = 0;
	bq25890_read_interface((kal_uint8) (bq25890_CON19),
	                       (&val),
	                       (kal_uint8) (CON19_IDPM_STAT_MASK),
	                       (kal_uint8) (CON19_IDPM_STAT_SHIFT)
	                      );
	return val;
}

kal_uint32 bq25890_get_current_iinlim(void)
{
	kal_uint8 val = 0;
	bq25890_read_interface((kal_uint8) (bq25890_CON19),
	                       (&val),
	                       (kal_uint8) (CON19_IDPM_LIM_MASK),
	                       (kal_uint8) (CON19_IDPM_LIM_SHIFT)
	                      );
	return val;
}

/* CON20---------------------------------------------------- */
void bq25890_set_reg_rst(kal_uint32 val)
{
	bq25890_config_interface((kal_uint8) (bq25890_CON20),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON20_REG_RST_MASK),
	                         (kal_uint8) (CON20_REG_RST_SHIFT)
	                        );
}

kal_uint32 bq25890_get_pn(void)
{
	kal_uint8 val = 0;
	kal_uint32 ret = 0;
	ret = bq25890_read_interface((kal_uint8) (bq25890_CON20),
	                             (&val),
	                             (kal_uint8) (CON20_PN_MASK),
	                             (kal_uint8) (CON20_PN_SHIFT)
	                            );

	if (ret)
	{
			bq25890_print("--->ret=%d\n",ret);
			return val;
	}
	else
		return 0;
}

kal_uint32 bq25890_get_rev(void)
{
	kal_uint8 val = 0;

	bq25890_read_interface((kal_uint8) (bq25890_CON20),
	                       (&val),
	                       (kal_uint8) (CON20_REV_MASK),
	                       (kal_uint8) (CON20_REV_SHIFT)
	                      );

	return val;
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/

void bq25890_dump_register(void)
{
	int i = 0;
	for (i = 0; i < bq25890_REG_NUM; i++) {
		bq25890_read_byte(i, &bq25890_reg[i]);
		bq25890_print("[bq25890_dump_register] Reg[0x%X]=0x%X\n", i, bq25890_reg[i]);
	}
}

CHARGER_TYPE bq25890_charger_type_detection(void)
{
	CHARGER_TYPE charger_type = CHARGER_UNKNOWN;
	kal_uint32 ret_val;
	kal_uint32 vbus_state;
	kal_uint32 count = 0;
	static kal_bool force_detect_done = KAL_FALSE;

	bq25890_print("[LK] use BQ25890 charger detection\r\n");

	charger_detect_init();

	/* disable MAXC_EN or HVDCP_EN */
	bq25890_config_interface(bq25890_CON2, 0x0, CON2_HVDCP_EN_MASK, CON2_HVDCP_EN_SHIFT);
	bq25890_config_interface(bq25890_CON2, 0x0, CON2_MAXC_EN_MASK, CON2_MAXC_EN_SHIFT);

	while (bq25890_get_pg_stat() == 0) {
		bq25890_print("wait pg_state ready.\n");
		count++;
		mtk_wdt_restart();
		mdelay(1);
		if (count > 500) {
			bq25890_print("wait BQ25890 pg_state ready timeout!\n");
			break;
		}

		if (upmu_is_chr_det() == KAL_FALSE) {
			charger_detect_release();
			return CHARGER_UNKNOWN;
		}
	}

	ret_val = bq25890_get_vbus_stat();

	/* if detection is not finished */
	if (ret_val == 0x0 && !force_detect_done) {
		count = 0;
		bq25890_set_force_dpdm(1);
		while (bq25890_get_dpdm_status() == 1) {
			count++;
			bq25890_print("polling BQ25890 charger detection\r\n");
			mtk_wdt_restart();
			mdelay(1);
			if (count > 1000)
				break;
			if (upmu_is_chr_det() == KAL_FALSE) {
				charger_detect_release();
				return CHARGER_UNKNOWN;
			}
		}
	}

	vbus_state = bq25890_get_vbus_stat();

	switch (vbus_state) {
		case 0x0:
			charger_type = CHARGER_UNKNOWN;
			break;
		case 0x1:
			charger_type = STANDARD_HOST;
			break;
		case 0x2:
			charger_type = CHARGING_HOST;
			break;
		case 0x3:
			charger_type = STANDARD_CHARGER;
			break;
		case 0x4:
			charger_type = STANDARD_CHARGER; /* MaxCharge DCP */
			break;
		case 0x5:
			charger_type = NONSTANDARD_CHARGER;
			break;
		case 0x6:
			charger_type = NONSTANDARD_CHARGER;
			break;
		case 0x7:
			charger_type = CHARGER_UNKNOWN; /* OTG */
			break;

		default:
			charger_type = CHARGER_UNKNOWN;
			break;
	}
#if 0
	if (charger_type == NONSTANDARD_CHARGER) {
		reg_val = bq25890_get_iinlim();
		if (reg_val < 0x12)
			charger_type = NONSTANDARD_CHARGER;
		else if (reg_val == 0x12) /* 1A charger */
			charger_type = APPLE_1_0A_CHARGER;
		else if (reg_val >= 0x26) /* 2A/2.1A/2.4A charger */
			charger_type = APPLE_2_1A_CHARGER;

	}
#endif
	charger_detect_release();
	force_detect_done = KAL_TRUE;
	bq25890_print("charging_get_charger_type = %d\n", charger_type);

	return charger_type;
}

static void bq_set_charging_current(kal_int32 cc_value)
{
	bq25890_set_ichg((cc_value/64) & 0x7F);
}

static void bq_set_input_current_limit(kal_int32 input_limit)
{
	switch (input_limit) {
		case 500:
			bq25890_set_iinlim(0x8);
			break;
		case 2000:
			bq25890_set_iinlim(0x26);
			break;
		default:
			bq25890_set_iinlim(0x8);
	}
}

static void bq_set_reg_voltage(kal_int32 cv)
{
	if (cv == 4200)
		bq25890_set_vreg(0x17); //VREG 4.208V
	else if (cv == 4000)
		bq25890_set_vreg(0x0A); //VREG 4.0V
	else
		bq25890_set_vreg(0x17); //VREG 4.208V
}

static void bq_charging_hw_init(void)
{
	upmu_set_rg_bc11_bb_ctrl(1);    /* BC11_BB_CTRL */
	upmu_set_rg_bc11_rst(1);    /* BC11_RST */

	/* ICO_EN default off */
	bq25890_config_interface(bq25890_CON2, 0x1, CON2_ICO_EN_MASK, CON2_ICO_EN_SHIFT);
	/* disable MAXC_EN and HVDCP_EN */
	bq25890_config_interface(bq25890_CON2, 0x0, CON2_HVDCP_EN_MASK, CON2_HVDCP_EN_SHIFT);
	bq25890_config_interface(bq25890_CON2, 0x0, CON2_MAXC_EN_MASK, CON2_MAXC_EN_SHIFT);

	/* AUTO_DPDM_EN default on */
	bq25890_config_interface(bq25890_CON2, 0x1, CON2_AUTO_DPDM_MASK, CON2_AUTO_DPDM_SHIFT);

	bq25890_set_en_hiz(0x0);
	bq25890_set_otg_config(0x0);
	bq25890_set_force_vindpm(0x1); /* Run absolute VINDPM */
	bq25890_set_vindpm(0x14);   /* VIN DPM check 4.60V */
	bq25890_set_reg_rst(0x0);
	bq25890_set_wdt_rst(0x1);   /* Kick watchdog */
	bq25890_set_sys_min(0x5);   /* Minimum system voltage 3.5V */
#if defined(MTK_JEITA_STANDARD_SUPPORT)
	bq25890_set_iprechg(0x3);   /* Precharge current 256mA */
#else
	bq25890_set_iprechg(0x7);   /* Precharge current 512mA */
#endif
	bq25890_set_iterm(0x1); /* Termination current 128mA */

	bq25890_set_vreg(0x17); /* VREG 4.208V */

	bq25890_set_batlowv(0x1);   /* BATLOWV 3.0V */
	bq25890_set_vrechg(0x0);    /* VRECHG 0.1V (4.108V) */
	bq25890_set_en_term(0x1);   /* Enable termination */
	bq25890_set_watchdog(0x1);  /* WDT 40s */
}

static void bq_enable_charging(kal_bool enable)
{
	if (enable == KAL_FALSE) {
		bq25890_set_wdt_rst(0x1);
		bq25890_set_chg_config(0x0);
	} else {
		bq25890_set_en_hiz(0x0);
		bq25890_set_chg_config(0x1);
	}
}

static void bq_charger_dump_register(void)
{
	bq25890_dump_register();
}

static void bq_kick_ext_charger_watchdog(void)
{
	bq25890_set_wdt_rst(0x1);
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

	/* BQ25890 supports BC1.2 */
#if defined(MTK_BQ25890_SUPPORT)
	//charger.charger_type_detection = bq25890_charger_type_detection;
#endif
}

void probe_bq25890(void)
{
	if (bq25890_get_pn() == 0x2 || bq25890_get_pn() == 0x0) {
		register_charger_control();
		bq25890_print("find BQ25890/BQ25896 device and register charger control.\n");
	} else
		bq25890_print("no BQ25890 device is found.\n");
	
}

