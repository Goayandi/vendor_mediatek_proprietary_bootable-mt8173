#include "platform.h"
#include "i2c.h"
#include "pmic.h"
#include "bq25892.h"
#include "cust_i2c.h"

/**********************************************************
 *
 *    [Define]
 *
 **********************************************************/
#if defined(MTK_BQ25892_SUPPORT)
#define bq25892_SLAVE_ADDR_WRITE   0xD4
#define bq25892_SLAVE_ADDR_READ    0xD5
#endif

#if defined(MTK_BQ25896_SUPPORT)
#define bq25892_SLAVE_ADDR_WRITE   0xD6
#define bq25892_SLAVE_ADDR_READ    0xD7
#endif

/**********************************************************
  *
  *   [Global Variable]
  *
  *********************************************************/
#ifdef I2C_SWITHING_CHARGER_CHANNEL
#define bq25892_I2C_ID I2C_SWITHING_CHARGER_CHANNEL
#else
#define bq25892_I2C_ID 0
#endif

static struct mt_i2c_t bq25892_i2c;
static kal_uint8 bq25892_reg[bq25892_REG_NUM] = { 0 };

#define bq25892_print(fmt, args...)   \
do {                                    \
    print(fmt, ##args); \
} while(0)

/**********************************************************
  *
  *   [I2C Function For Read/Write bq25892]
  *
  *********************************************************/
kal_uint32 bq25892_write_byte(kal_uint8 addr, kal_uint8 value)
{
	int ret_code = I2C_OK;
	kal_uint8 write_data[2];
	kal_uint16 len;

	write_data[0]= addr;
	write_data[1] = value;

	bq25892_i2c.id = bq25892_I2C_ID;
	/* Since i2c will left shift 1 bit, we need to set I2C address to >>1 */
	bq25892_i2c.addr = (bq25892_SLAVE_ADDR_WRITE >> 1);
	bq25892_i2c.mode = ST_MODE;
	bq25892_i2c.speed = 100;
	len = 2;

	ret_code = i2c_write(&bq25892_i2c, write_data, len);
	if (ret_code == 0)
		return 1; // ok
	else
		return 0; // fail
}

kal_uint32 bq25892_read_byte (kal_uint8 addr, kal_uint8 *dataBuffer)
{
	int ret_code = I2C_OK;
	kal_uint16 len;
	*dataBuffer = addr;

	bq25892_i2c.id = bq25892_I2C_ID;
	/* Since i2c will left shift 1 bit, we need to set I2C address to >>1 */
	bq25892_i2c.addr = (bq25892_SLAVE_ADDR_WRITE >> 1);
	bq25892_i2c.mode = ST_MODE;
	bq25892_i2c.speed = 100;
	len = 1;

	ret_code = i2c_write_read(&bq25892_i2c, dataBuffer, len, len);
	if (ret_code == 0)
		return 1; // ok
	else
		return 0; // fail
}


kal_uint32 bq25892_read_interface(kal_uint8 RegNum, kal_uint8 *val, kal_uint8 MASK,
                                  kal_uint8 SHIFT)
{
	kal_uint8 bq25892_reg = 0;
	int ret = 0;

	ret = bq25892_read_byte(RegNum, &bq25892_reg);

	bq25892_reg &= (MASK << SHIFT);
	*val = (bq25892_reg >> SHIFT);

	return ret;
}

kal_uint32 bq25892_config_interface(kal_uint8 RegNum, kal_uint8 val, kal_uint8 MASK,
                                    kal_uint8 SHIFT)
{
	kal_uint8 bq25892_reg = 0;
	int ret = 0;

	ret = bq25892_read_byte(RegNum, &bq25892_reg);

	bq25892_reg &= ~(MASK << SHIFT);
	bq25892_reg |= (val << SHIFT);

	ret = bq25892_write_byte(RegNum, bq25892_reg);
	return ret;
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/

/* CON0---------------------------------------------------- */
void bq25892_set_en_hiz(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON0),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON0_EN_HIZ_MASK),
	                         (kal_uint8) (CON0_EN_HIZ_SHIFT)
	                        );
}

void bq25892_set_iinlim(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON0),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON0_IINLIM_MASK),
	                         (kal_uint8) (CON0_IINLIM_SHIFT)
	                        );
}

kal_uint32 bq25892_get_iinlim(void)
{
	kal_uint8 val = 0;
	bq25892_read_interface((kal_uint8) (BQ25892_CON0),
	                       (&val),
	                       (kal_uint8) (CON0_IINLIM_MASK), (kal_uint8) (CON0_IINLIM_SHIFT)
	                      );
	return val;
}

/* CON1---------------------------------------------------- */

/* CON2---------------------------------------------------- */
void bq25892_set_force_dpdm(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON2),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON2_FORCE_DPDM_MASK),
	                         (kal_uint8) (CON2_FORCE_DPDM_SHIFT)
	                        );
}

void bq25892_set_auto_dpdm_en(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON2),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON2_AUTO_DPDM_MASK),
	                         (kal_uint8) (CON2_AUTO_DPDM_SHIFT)
	                        );
}

kal_uint32 bq25892_get_dpdm_status(void)
{
	kal_uint8 val = 0;
	bq25892_read_interface((kal_uint8) (BQ25892_CON2),
	                       (&val),
	                       (kal_uint8) (CON2_FORCE_DPDM_MASK),
	                       (kal_uint8) (CON2_FORCE_DPDM_SHIFT)
	                      );
	return val;
}

/* CON3---------------------------------------------------- */
void bq25892_set_wdt_rst(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON3),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON3_WDT_RST_MASK),
	                         (kal_uint8) (CON3_WDT_RST_SHIFT)
	                        );
}

void bq25892_set_otg_config(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON3),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON3_OTG_CONFIG_MASK),
	                         (kal_uint8) (CON3_OTG_CONFIG_SHIFT)
	                        );
}

void bq25892_set_chg_config(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON3),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON3_CHG_CONFIG_MASK),
	                         (kal_uint8) (CON3_CHG_CONFIG_SHIFT)
	                        );
}

void bq25892_set_sys_min(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON3),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON3_SYS_MIN_MASK),
	                         (kal_uint8) (CON3_SYS_MIN_SHIFT)
	                        );
}

/* CON4---------------------------------------------------- */
void bq25892_set_ichg(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON4),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON4_ICHG_MASK),
	                         (kal_uint8) (CON4_ICHG_SHIFT)
	                        );
}

kal_uint32 bq25892_get_ichg(void)
{
	kal_uint8 val = 0;
	bq25892_read_interface((kal_uint8) (BQ25892_CON4),
	                       (&val),
	                       (kal_uint8) (CON4_ICHG_MASK),
	                       (kal_uint8) (CON4_ICHG_SHIFT)
	                      );
	return val;
}
/* CON5---------------------------------------------------- */
void bq25892_set_iprechg(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON5),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON5_IPRECHG_MASK),
	                         (kal_uint8) (CON5_IPRECHG_SHIFT)
	                        );
}

void bq25892_set_iterm(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON5),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON5_ITERM_MASK),
	                         (kal_uint8) (CON5_ITERM_SHIFT)
	                        );
}

/* CON6---------------------------------------------------- */
void bq25892_set_vreg(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON6),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON6_VREG_MASK),
	                         (kal_uint8) (CON6_VREG_SHIFT)
	                        );
}

void bq25892_set_batlowv(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON6),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON6_BATLOWV_MASK),
	                         (kal_uint8) (CON6_BATLOWV_SHIFT)
	                        );
}

void bq25892_set_vrechg(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON6),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON6_VRECHG_MASK),
	                         (kal_uint8) (CON6_VRECHG_SHIFT)
	                        );
}

/* CON7---------------------------------------------------- */
void bq25892_set_en_term(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON7),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON7_EN_TERM_MASK),
	                         (kal_uint8) (CON7_EN_TERM_SHIFT)
	                        );
}

void bq25892_set_watchdog(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON7),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON7_WATCHDOG_MASK),
	                         (kal_uint8) (CON7_WATCHDOG_SHIFT)
	                        );
}

void bq25892_set_en_timer(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON7),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON7_EN_TIMER_MASK),
	                         (kal_uint8) (CON7_EN_TIMER_SHIFT)
	                        );
}

void bq25892_set_chg_timer(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON7),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON7_CHG_TIMER_MASK),
	                         (kal_uint8) (CON7_CHG_TIMER_SHIFT)
	                        );
}

/* CON8---------------------------------------------------- */
void bq25892_set_treg(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON8),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON8_TREG_MASK),
	                         (kal_uint8) (CON8_TREG_SHIFT)
	                        );
}

/* CON9---------------------------------------------------- */
void bq25892_set_batfet_disable(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON9),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON9_BATFET_DIS_MASK),
	                         (kal_uint8) (CON9_BATFET_DIS_SHIFT)
	                        );
}

/* CON10---------------------------------------------------- */
void bq25892_set_boost_lim(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON10),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON10_BOOST_LIM_MASK),
	                         (kal_uint8) (CON10_BOOST_LIM_SHIFT)
	                        );
}

/* CON11---------------------------------------------------- */
kal_uint32 bq25892_get_vbus_stat(void)
{
	kal_uint8 val = 0;
	bq25892_read_interface((kal_uint8) (BQ25892_CON11),
	                       (&val),
	                       (kal_uint8) (CON11_VBUS_STAT_MASK),
	                       (kal_uint8) (CON11_VBUS_STAT_SHIFT)
	                      );
	return val;
}

kal_uint32 bq25892_get_chrg_stat(void)
{
	kal_uint8 val = 0;
	bq25892_read_interface((kal_uint8) (BQ25892_CON11),
	                       (&val),
	                       (kal_uint8) (CON11_CHRG_STAT_MASK),
	                       (kal_uint8) (CON11_CHRG_STAT_SHIFT)
	                      );
	return val;
}

kal_uint32 bq25892_get_pg_stat(void)
{
	kal_uint8 val = 0;
	bq25892_read_interface((kal_uint8) (BQ25892_CON11),
	                       (&val),
	                       (kal_uint8) (CON11_PG_STAT_MASK),
	                       (kal_uint8) (CON11_PG_STAT_SHIFT)
	                      );
	return val;
}

kal_uint32 bq25892_get_vsys_stat(void)
{
	kal_uint8 val = 0;
	bq25892_read_interface((kal_uint8) (BQ25892_CON11),
	                       (&val),
	                       (kal_uint8) (CON11_VSYS_STAT_MASK),
	                       (kal_uint8) (CON11_VSYS_STAT_SHIFT)
	                      );
	return val;
}

kal_uint32 bq25892_get_system_status(void)
{
	kal_uint8 val = 0;
	bq25892_read_interface((kal_uint8) (BQ25892_CON11),
	                       (&val),
	                       (kal_uint8) (0xFF),
	                       (kal_uint8) (0x0)
	                      );
	return val;
}

/* CON13---------------------------------------------------- */
void bq25892_set_force_vindpm(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON13),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON13_FORCE_VINDPM_MASK),
	                         (kal_uint8) (CON13_FORCE_VINDPM_SHIFT)
	                        );
}

void bq25892_set_vindpm(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON13),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON13_VINDPM_MASK),
	                         (kal_uint8) (CON13_VINDPM_SHIFT)
	                        );
}

/* CON19---------------------------------------------------- */
kal_uint32 bq25892_get_vdpm_stat(void)
{
	kal_uint8 val = 0;
	bq25892_read_interface((kal_uint8) (BQ25892_CON19),
	                       (&val),
	                       (kal_uint8) (CON19_VDPM_STAT_MASK),
	                       (kal_uint8) (CON19_VDPM_STAT_SHIFT)
	                      );
	return val;
}

kal_uint32 bq25892_get_idpm_stat(void)
{
	kal_uint8 val = 0;
	bq25892_read_interface((kal_uint8) (BQ25892_CON19),
	                       (&val),
	                       (kal_uint8) (CON19_IDPM_STAT_MASK),
	                       (kal_uint8) (CON19_IDPM_STAT_SHIFT)
	                      );
	return val;
}

kal_uint32 bq25892_get_current_iinlim(void)
{
	kal_uint8 val = 0;
	bq25892_read_interface((kal_uint8) (BQ25892_CON19),
	                       (&val),
	                       (kal_uint8) (CON19_IDPM_LIM_MASK),
	                       (kal_uint8) (CON19_IDPM_LIM_SHIFT)
	                      );
	return val;
}

/* CON20---------------------------------------------------- */
void bq25892_set_reg_rst(kal_uint32 val)
{
	bq25892_config_interface((kal_uint8) (BQ25892_CON20),
	                         (kal_uint8) (val),
	                         (kal_uint8) (CON20_REG_RST_MASK),
	                         (kal_uint8) (CON20_REG_RST_SHIFT)
	                        );
}

kal_uint32 bq25892_get_pn(void)
{
	kal_uint8 val = 0;
	kal_uint32 ret = 0;
	ret = bq25892_read_interface((kal_uint8) (BQ25892_CON20),
	                             (&val),
	                             (kal_uint8) (CON20_PN_MASK),
	                             (kal_uint8) (CON20_PN_SHIFT)
	                            );

	if (ret)
		return val;
	else
		return 0;
}

kal_uint32 bq25892_get_rev(void)
{
	kal_uint8 val = 0;

	bq25892_read_interface((kal_uint8) (BQ25892_CON20),
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

void bq25892_dump_register(void)
{
	int i = 0;
	for (i = 0; i < bq25892_REG_NUM; i++) {
		bq25892_read_byte(i, &bq25892_reg[i]);
		bq25892_print("[bq25892_dump_register] Reg[0x%X]=0x%X\n", i, bq25892_reg[i]);
	}
}

CHARGER_TYPE get_type_from_vbus_state(int vbus_state)
{
	CHARGER_TYPE charger_type;

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

	return charger_type;
}

CHARGER_TYPE bq25892_charger_type_detection(void)
{
	CHARGER_TYPE charger_type = CHARGER_UNKNOWN;
	kal_uint32 ret_val;
	kal_uint32 vbus_state;
	kal_uint32 count = 0;

	bq25892_print("[PL] use BQ25892 charger detection\r\n");

	/* ICO_EN default on */
	bq25892_config_interface(BQ25892_CON2, 0x1, CON2_ICO_EN_MASK, CON2_ICO_EN_SHIFT);
	/* disable MAXC_EN and HVDCP_EN */
	bq25892_config_interface(BQ25892_CON2, 0x0, CON2_HVDCP_EN_MASK, CON2_HVDCP_EN_SHIFT);
	bq25892_config_interface(BQ25892_CON2, 0x0, CON2_MAXC_EN_MASK, CON2_MAXC_EN_SHIFT);

	/* AUTO_DPDM_EN default on */
	bq25892_config_interface(BQ25892_CON2, 0x1, CON2_AUTO_DPDM_MASK, CON2_AUTO_DPDM_SHIFT);

	ret_val = bq25892_get_vbus_stat();

	if (ret_val == 0x7) {
		bq25892_print("charger OTG mode is on during type detection!\n");
		charger_type = CHARGER_UNKNOWN;
		bq25892_set_otg_config(0);
		bq25892_print("charger OTG mode is cleared!\n");
		return charger_type;
	}

	charger_detect_init();

	while (bq25892_get_pg_stat() == 0) {
		bq25892_print("wait pg_state ready.\n");
		count++;
		platform_wdt_kick();
		mdelay(1);
		if (count > 500) {
			bq25892_print("wait BQ25892 pg_state ready timeout!\n");
			break;
		}

		if (pmic_IsUsbCableIn() == PMIC_CHRDET_NOT_EXIST) {
			charger_detect_release();
			return CHARGER_UNKNOWN;
		}
	}

	ret_val = bq25892_get_vbus_stat();

	/* if initial detection is going, wait until detection done */
	if (ret_val == 0x0) {
		count = 0;
		bq25892_set_force_dpdm(1);
		while (bq25892_get_dpdm_status() == 1) {
			count++;
			bq25892_print("polling BQ25892 charger detection\r\n");
			platform_wdt_kick();
			mdelay(1);
			if (count > 1000)
				break;
			if (pmic_IsUsbCableIn() == PMIC_CHRDET_NOT_EXIST) {
				charger_detect_release();
				return CHARGER_UNKNOWN;
			}
		}
	}

	ret_val = bq25892_get_vbus_stat();

	bq25892_print("type = %d\n", get_type_from_vbus_state(ret_val));

	/* do detection again due to slimport might not be able to switch DP/DM on-time */
	if (pmic_IsUsbCableIn() == PMIC_CHRDET_EXIST) {
		count = 0;
		bq25892_set_force_dpdm(1);
		while (bq25892_get_dpdm_status() == 1) {
			count++;
			bq25892_print("polling BQ25892 charger detection\r\n");
			platform_wdt_kick();
			mdelay(1);
			if (count > 1000)
				break;
			if (pmic_IsUsbCableIn() == PMIC_CHRDET_NOT_EXIST) {
				charger_detect_release();
				return CHARGER_UNKNOWN;
			}
		}
	}

	vbus_state = bq25892_get_vbus_stat();

	if (ret_val != vbus_state)
		bq25892_print("Update VBUS state from %d to %d!\n", vbus_state, ret_val);

	charger_type = get_type_from_vbus_state(vbus_state);

	charger_detect_release();
	bq25892_print("charging_get_charger_type = %d\n", charger_type);

	//disable watchdog here in case of long period of idle when F/W download.
	if (charger_type == STANDARD_HOST || charger_type == CHARGING_HOST)
		bq25892_set_watchdog(0x0);

	return charger_type;
}

