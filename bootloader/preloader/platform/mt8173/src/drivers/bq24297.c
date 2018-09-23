#include "platform.h"
#include "i2c.h"
#include "pmic.h"
#include "bq24297.h"
#include "cust_i2c.h"

/**********************************************************
  *   I2C Slave Setting
  *********************************************************/
#define bq24297_SLAVE_ADDR_WRITE   0xD6
#define bq24297_SLAVE_ADDR_READ    0xD7

/**********************************************************
  *   Global Variable 
  *********************************************************/
#ifdef I2C_SWITHING_CHARGER_CHANNEL
#define bq24297_I2C_ID I2C_SWITHING_CHARGER_CHANNEL
#else
#define bq24297_I2C_ID 0//2
#endif

static struct mt_i2c_t bq24297_i2c;
kal_uint8 bq24297_reg[bq24297_REG_NUM] = { 0 };

#define bq24297_print(fmt, args...)   \
do {									\
    print(fmt, ##args); \
} while(0)

/**********************************************************
  *
  *   [I2C Function For Read/Write bq24297]
  *
  *********************************************************/
kal_uint32 bq24297_write_byte(kal_uint8 addr, kal_uint8 value)
{
    int ret_code = I2C_OK;
    kal_uint8 write_data[2];
    kal_uint16 len;

    write_data[0]= addr;
    write_data[1] = value;

    bq24297_i2c.id = bq24297_I2C_ID;
    /* Since i2c will left shift 1 bit, we need to set I2C address to >>1 */
    bq24297_i2c.addr = (bq24297_SLAVE_ADDR_WRITE >> 1);
    bq24297_i2c.mode = ST_MODE;
    bq24297_i2c.speed = 100;
    len = 2;
    
    ret_code = i2c_write(&bq24297_i2c, write_data, len);
    //bq24297_print("%s: i2c_write: ret_code: %d\n", __func__, ret_code);

    if(ret_code == 0)
        return 1; // ok
    else
        return 0; // fail
}

kal_uint32 bq24297_read_byte (kal_uint8 addr, kal_uint8 *dataBuffer) 
{
    int ret_code = I2C_OK;
    kal_uint16 len;
    *dataBuffer = addr;

    bq24297_i2c.id = bq24297_I2C_ID;
    /* Since i2c will left shift 1 bit, we need to set I2C address to >>1 */
    bq24297_i2c.addr = (bq24297_SLAVE_ADDR_WRITE >> 1);
    bq24297_i2c.mode = ST_MODE;
    bq24297_i2c.speed = 100;
    len = 1;

    ret_code = i2c_write_read(&bq24297_i2c, dataBuffer, len, len);    
    //bq24297_print("%s: i2c_read: ret_code: %d\n", __func__, ret_code);

    if(ret_code == 0)
        return 1; // ok
    else
        return 0; // fail
}

/**********************************************************
  *
  *   [Read / Write Function]
  *
  *********************************************************/
kal_uint32 bq24297_read_interface(kal_uint8 RegNum, kal_uint8 *val, kal_uint8 MASK,
				  kal_uint8 SHIFT)
{
	kal_uint8 bq24297_reg = 0;
	int ret = 0;

	/* bq24297_print("--------------------------------------------------\n"); */

	ret = bq24297_read_byte(RegNum, &bq24297_reg);
	/* bq24297_print("[bq24297_read_interface] Reg[%x]=0x%x\n", RegNum, bq24297_reg); */

	bq24297_reg &= (MASK << SHIFT);
	*val = (bq24297_reg >> SHIFT);
	/* bq24297_print("[bq24297_read_interface] Val=0x%x\n", *val); */

	return ret;
}

kal_uint32 bq24297_config_interface(kal_uint8 RegNum, kal_uint8 val, kal_uint8 MASK,
				    kal_uint8 SHIFT)
{
	kal_uint8 bq24297_reg = 0;
	int ret = 0;

	/* bq24297_print("--------------------------------------------------\n"); */

	ret = bq24297_read_byte(RegNum, &bq24297_reg);
	/* bq24297_print("[bq24297_config_interface] Reg[%x]=0x%x\n", RegNum, bq24297_reg); */

	bq24297_reg &= ~(MASK << SHIFT);
	bq24297_reg |= (val << SHIFT);

	ret = bq24297_write_byte(RegNum, bq24297_reg);
	/* bq24297_print("[bq24297_config_interface] Write Reg[%x]=0x%x\n", RegNum, bq24297_reg); */

	/* Check */
	/* bq24297_read_byte(RegNum, &bq24297_reg); */
	/* bq24297_print("[bq24297_config_interface] Check Reg[%x]=0x%x\n", RegNum, bq24297_reg); */

	return ret;
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
/* CON0---------------------------------------------------- */

void bq24297_set_en_hiz(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON0),
				       (kal_uint8) (val),
				       (kal_uint8) (CON0_EN_HIZ_MASK),
				       (kal_uint8) (CON0_EN_HIZ_SHIFT)
	    );
}

void bq24297_set_vindpm(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON0),
				       (kal_uint8) (val),
				       (kal_uint8) (CON0_VINDPM_MASK),
				       (kal_uint8) (CON0_VINDPM_SHIFT)
	    );
}

void bq24297_set_iinlim(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON0),
				       (kal_uint8) (val),
				       (kal_uint8) (CON0_IINLIM_MASK),
				       (kal_uint8) (CON0_IINLIM_SHIFT)
	    );
}

kal_uint32 bq24297_get_iinlim(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;
	ret = bq24297_read_interface((kal_uint8) (bq24297_CON0),
				     (&val),
				     (kal_uint8) (CON0_IINLIM_MASK),
				     (kal_uint8) (CON0_IINLIM_SHIFT)
	    );
	return val;
}

/* CON1---------------------------------------------------- */

void bq24297_set_reg_rst(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON1),
				       (kal_uint8) (val),
				       (kal_uint8) (CON1_REG_RST_MASK),
				       (kal_uint8) (CON1_REG_RST_SHIFT)
	    );
}

void bq24297_set_wdt_rst(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON1),
				       (kal_uint8) (val),
				       (kal_uint8) (CON1_WDT_RST_MASK),
				       (kal_uint8) (CON1_WDT_RST_SHIFT)
	    );
}

void bq24297_set_otg_config(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON1),
				       (kal_uint8) (val),
				       (kal_uint8) (CON1_OTG_CONFIG_MASK),
				       (kal_uint8) (CON1_OTG_CONFIG_SHIFT)
	    );
}

void bq24297_set_chg_config(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON1),
				       (kal_uint8) (val),
				       (kal_uint8) (CON1_CHG_CONFIG_MASK),
				       (kal_uint8) (CON1_CHG_CONFIG_SHIFT)
	    );
}

void bq24297_set_sys_min(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON1),
				       (kal_uint8) (val),
				       (kal_uint8) (CON1_SYS_MIN_MASK),
				       (kal_uint8) (CON1_SYS_MIN_SHIFT)
	    );
}

void bq24297_set_boost_lim(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON1),
				       (kal_uint8) (val),
				       (kal_uint8) (CON1_BOOST_LIM_MASK),
				       (kal_uint8) (CON1_BOOST_LIM_SHIFT)
	    );
}

/* CON2---------------------------------------------------- */

void bq24297_set_ichg(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON2),
				       (kal_uint8) (val),
				       (kal_uint8) (CON2_ICHG_MASK), (kal_uint8) (CON2_ICHG_SHIFT)
	    );
}

void bq24297_set_force_20pct(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON2),
				       (kal_uint8) (val),
				       (kal_uint8) (CON2_FORCE_20PCT_MASK),
				       (kal_uint8) (CON2_FORCE_20PCT_SHIFT)
	    );
}

/* CON3---------------------------------------------------- */

void bq24297_set_iprechg(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON3),
				       (kal_uint8) (val),
				       (kal_uint8) (CON3_IPRECHG_MASK),
				       (kal_uint8) (CON3_IPRECHG_SHIFT)
	    );
}

void bq24297_set_iterm(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON3),
				       (kal_uint8) (val),
				       (kal_uint8) (CON3_ITERM_MASK), (kal_uint8) (CON3_ITERM_SHIFT)
	    );
}

/* CON4---------------------------------------------------- */

void bq24297_set_vreg(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON4),
				       (kal_uint8) (val),
				       (kal_uint8) (CON4_VREG_MASK), (kal_uint8) (CON4_VREG_SHIFT)
	    );
}

void bq24297_set_batlowv(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON4),
				       (kal_uint8) (val),
				       (kal_uint8) (CON4_BATLOWV_MASK),
				       (kal_uint8) (CON4_BATLOWV_SHIFT)
	    );
}

void bq24297_set_vrechg(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON4),
				       (kal_uint8) (val),
				       (kal_uint8) (CON4_VRECHG_MASK),
				       (kal_uint8) (CON4_VRECHG_SHIFT)
	    );
}

/* CON5---------------------------------------------------- */

void bq24297_set_en_term(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON5),
				       (kal_uint8) (val),
				       (kal_uint8) (CON5_EN_TERM_MASK),
				       (kal_uint8) (CON5_EN_TERM_SHIFT)
	    );
}

void bq24297_set_term_stat(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON5),
				       (kal_uint8) (val),
				       (kal_uint8) (CON5_TERM_STAT_MASK),
				       (kal_uint8) (CON5_TERM_STAT_SHIFT)
	    );
}

void bq24297_set_watchdog(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON5),
				       (kal_uint8) (val),
				       (kal_uint8) (CON5_WATCHDOG_MASK),
				       (kal_uint8) (CON5_WATCHDOG_SHIFT)
	    );
}

void bq24297_set_en_timer(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON5),
				       (kal_uint8) (val),
				       (kal_uint8) (CON5_EN_TIMER_MASK),
				       (kal_uint8) (CON5_EN_TIMER_SHIFT)
	    );
}

void bq24297_set_chg_timer(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON5),
				       (kal_uint8) (val),
				       (kal_uint8) (CON5_CHG_TIMER_MASK),
				       (kal_uint8) (CON5_CHG_TIMER_SHIFT)
	    );
}

/* CON6---------------------------------------------------- */

void bq24297_set_treg(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON6),
				       (kal_uint8) (val),
				       (kal_uint8) (CON6_TREG_MASK), (kal_uint8) (CON6_TREG_SHIFT)
	    );
}

/* CON7---------------------------------------------------- */
kal_uint32 bq24297_get_dpdm_status(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24297_read_interface((kal_uint8) (bq24297_CON7),
				     (&val),
				     (kal_uint8) (CON7_DPDM_EN_MASK),
				     (kal_uint8) (CON7_DPDM_EN_SHIFT)
	    );
	return val;
}

void bq24297_set_dpdm_en(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON7),
				       (kal_uint8) (val),
				       (kal_uint8) (CON7_DPDM_EN_MASK),
				       (kal_uint8) (CON7_DPDM_EN_SHIFT)
	    );
}

void bq24297_set_tmr2x_en(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON7),
				       (kal_uint8) (val),
				       (kal_uint8) (CON7_TMR2X_EN_MASK),
				       (kal_uint8) (CON7_TMR2X_EN_SHIFT)
	    );
}

void bq24297_set_batfet_disable(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON7),
				       (kal_uint8) (val),
				       (kal_uint8) (CON7_BATFET_Disable_MASK),
				       (kal_uint8) (CON7_BATFET_Disable_SHIFT)
	    );
}

void bq24297_set_int_mask(kal_uint32 val)
{
	kal_uint32 ret = 0;

	ret = bq24297_config_interface((kal_uint8) (bq24297_CON7),
				       (kal_uint8) (val),
				       (kal_uint8) (CON7_INT_MASK_MASK),
				       (kal_uint8) (CON7_INT_MASK_SHIFT)
	    );
}

/* CON8---------------------------------------------------- */

kal_uint32 bq24297_get_system_status(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24297_read_interface((kal_uint8) (bq24297_CON8),
				     (&val), (kal_uint8) (0xFF), (kal_uint8) (0x0)
	    );
	return val;
}

kal_uint32 bq24297_get_vbus_stat(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24297_read_interface((kal_uint8) (bq24297_CON8),
				     (&val),
				     (kal_uint8) (CON8_VBUS_STAT_MASK),
				     (kal_uint8) (CON8_VBUS_STAT_SHIFT)
	    );
	return val;
}

kal_uint32 bq24297_get_chrg_stat(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24297_read_interface((kal_uint8) (bq24297_CON8),
				     (&val),
				     (kal_uint8) (CON8_CHRG_STAT_MASK),
				     (kal_uint8) (CON8_CHRG_STAT_SHIFT)
	    );
	return val;
}

kal_uint32 bq24297_get_pg_stat(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24297_read_interface((kal_uint8) (bq24297_CON8),
				     (&val),
				     (kal_uint8) (CON8_PG_STAT_MASK),
				     (kal_uint8) (CON8_PG_STAT_SHIFT)
	    );
	return val;
}

kal_uint32 bq24297_get_vsys_stat(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;

	ret = bq24297_read_interface((kal_uint8) (bq24297_CON8),
				     (&val),
				     (kal_uint8) (CON8_VSYS_STAT_MASK),
				     (kal_uint8) (CON8_VSYS_STAT_SHIFT)
	    );
	return val;
}

/* CON10---------------------------------------------------- */
kal_uint32 bq24297_get_pn(void)
{
	kal_uint8 val = 0;
	kal_uint32 ret = 0;
	ret = bq24297_read_interface((kal_uint8) (bq24297_CON10),
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
  *   [Internal Function]
  *
  *********************************************************/
void bq24297_dump_register(void)
{
	int i = 0;
	for (i = 0; i < bq24297_REG_NUM; i++) {
		bq24297_read_byte(i, &bq24297_reg[i]);
		bq24297_print("[bq24297_dump_register] Reg[0x%X]=0x%X\n", i, bq24297_reg[i]);
	}
}

void check_usb_input_limit(void)
{
	kal_uint32 ret_val;

    if (pmic_IsUsbCableIn() == PMIC_CHRDET_NOT_EXIST)
        return;

	ret_val = bq24297_get_vbus_stat();
	if (ret_val == 0x3) {
		bq24297_print("charger OTG mode is on during preloader power-on!\n");
		bq24297_set_otg_config(0);
		bq24297_print("charger OTG mode is cleared!\n");
		return;
	}

    /* check if input limit is 100mA */
    if (0x0 == bq24297_get_iinlim())
    {
        /* increase input limit to 500mA in case of dead battery */
        bq24297_set_iinlim(0x2);
        printf("BQ24297: update input limit to 500mA.\n");
    }
}

static void charging_hw_init(void *data)
{
	upmu_set_rg_bc11_bb_ctrl(1);	/* BC11_BB_CTRL */
	upmu_set_rg_bc11_rst(1);	/* BC11_RST */

	bq24297_set_en_hiz(0x0);
	bq24297_set_vindpm(0x9);	/* VIN DPM check 4.60V */
	bq24297_set_reg_rst(0x0);
	bq24297_set_wdt_rst(0x1);	/* Kick watchdog */
	bq24297_set_sys_min(0x5);	/* Minimum system voltage 3.5V */
#if defined(MTK_JEITA_STANDARD_SUPPORT)
	bq24297_set_iprechg(0x1);	/* Precharge current 256mA */
#else
	bq24297_set_iprechg(0x3);	/* Precharge current 512mA */
#endif
	bq24297_set_iterm(0x0);	/* Termination current 128mA */
	bq24297_set_vreg(0x2C);	/* VREG 4.208V */
	bq24297_set_batlowv(0x1);	/* BATLOWV 3.0V */
	bq24297_set_vrechg(0x0);	/* VRECHG 0.1V (4.108V) */
	bq24297_set_en_term(0x1);	/* Enable termination */
	bq24297_set_term_stat(0x0);	/* Match ITERM */
	bq24297_set_watchdog(0x1);	/* WDT 40s */
#if !defined(MTK_JEITA_STANDARD_SUPPORT)
	bq24297_set_en_timer(0x0);	/* Disable charge timer */
#endif
	bq24297_set_int_mask(0x1);	/* Disable fault interrupt */
}

bool check_bq24297_exist(void)
{
	int part_num;
	part_num = bq24297_get_pn();
	if (part_num == 0x3) {
		bq24297_print("find BQ24297 device.\n");
		return true;
	}

	bq24297_print("can't find BQ24297 device.\n");
	return false;
}

CHARGER_TYPE bq24297_charger_type_detection(void)
{
	CHARGER_TYPE charger_type = CHARGER_UNKNOWN;
	kal_uint32 ret_val;
	kal_uint32 vbus_state;
	kal_uint8 reg_val = 0;
	kal_uint32 count = 0;

	bq24297_print("use BQ24297 charger detection\n");

	ret_val = bq24297_get_vbus_stat();
	if (ret_val == 0x3) {
		bq24297_print("charger OTG mode is on during type detection!\n");
		charger_type = CHARGER_UNKNOWN;
		bq24297_set_otg_config(0);
		bq24297_print("charger OTG mode is cleared!\n");
		return charger_type;
	}

	charger_detect_init();
	
	while (bq24297_get_pg_stat() == 0) {
		bq24297_print("wait pg_state ready.\n");
		count++;
		platform_wdt_kick();
		mdelay(1);
		if (count > 500) {
			bq24297_print("wait BQ24297 pg_state ready timeout!\n");
			break;
		}
		
		if (pmic_IsUsbCableIn() == PMIC_CHRDET_NOT_EXIST) {
			charger_detect_release();
			return CHARGER_UNKNOWN;
		}
	}

	ret_val = bq24297_get_vbus_stat();

	/* if detection is not finished or non-standard charger detected. */
	if (ret_val == 0x0) {
		count = 0;
		bq24297_set_dpdm_en(1);
		while (bq24297_get_dpdm_status() == 1) {
			count++;
			platform_wdt_kick();
			bq24297_print("polling BQ24297 charger detection\n");
			mdelay(1);
			if (count > 1000)
				break;
			if (pmic_IsUsbCableIn() == PMIC_CHRDET_NOT_EXIST) {
				charger_detect_release();
				return CHARGER_UNKNOWN;
			}
		}
	}

	vbus_state = bq24297_get_vbus_stat();

	switch (vbus_state) {
		case 0x1:
			charger_type = STANDARD_HOST;
			break;
		case 0x2:
			charger_type = STANDARD_CHARGER;
			break;
		default:
			charger_type = NONSTANDARD_CHARGER;
			break;
	}

	bq24297_print("type = %d\n",charger_type);

	/* do detection again due to slimport might not be able to switch DP/DM on-time */
	if (pmic_IsUsbCableIn() == PMIC_CHRDET_EXIST) {
		count = 0;
		bq24297_set_dpdm_en(1);
		while (bq24297_get_dpdm_status() == 1) {
			count++;
			platform_wdt_kick();
			mdelay(1);
			bq24297_print("polling again BQ24297 charger detection\n");
			if (count > 1000)
				break;
			if (pmic_IsUsbCableIn() == PMIC_CHRDET_NOT_EXIST) {
				charger_detect_release();
				return CHARGER_UNKNOWN;
			}
		}
	}

	ret_val = bq24297_get_vbus_stat();

	if (ret_val != vbus_state)
		bq24297_print("Update VBUS state from %d to %d!\n", vbus_state, ret_val);

	switch (ret_val) {
	case 0x1:
		charger_type = STANDARD_HOST;
		break;
	case 0x2:
		charger_type = STANDARD_CHARGER;
		break;
	default:
		charger_type = NONSTANDARD_CHARGER;
		break;
	}

#if 0
	if (charger_type == STANDARD_CHARGER) {
		reg_val = bq24297_get_iinlim();
		if (reg_val < 0x4)
			charger_type = NONSTANDARD_CHARGER;
		else if (reg_val == 0x4)
			charger_type = APPLE_1_0A_CHARGER;
		else if (reg_val == 0x6)
			charger_type = APPLE_2_1A_CHARGER;
	}
#endif
	charger_detect_release();
	bq24297_print("charging_get_charger_type = %d\n", charger_type);

	//disable watchdog here in case of long period of idle when F/W download.
	if (charger_type == STANDARD_HOST)
		bq24297_set_watchdog(0x0);

	return charger_type;
}

