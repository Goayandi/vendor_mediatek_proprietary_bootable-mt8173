#include <target/board.h>
#ifdef MTK_KERNEL_POWER_OFF_CHARGING
#define CFG_POWER_CHARGING
#endif
#ifdef CFG_POWER_CHARGING
#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_pmic.h>
#include <platform/boot_mode.h>
#include <platform/mt_gpt.h>
#include <platform/mt_rtc.h>
#include <platform/mt_disp_drv.h>
#include <platform/mtk_wdt.h>
#include <platform/mtk_key.h>
#include <platform/mt_logo.h>
#include <platform/mt_leds.h>
#include <printf.h>
#include <sys/types.h>
#include <target/cust_battery.h>
#include <target/cust_usb.h>
#include <dev/udc.h>

#include <platform/upmu_common.h>
#include <kernel/thread.h>
#include <platform/mt_battery.h>

#if defined(MTK_BQ24297_SUPPORT) || defined(MTK_BQ24296_SUPPORT)
#include <platform/bq24297.h>
#endif

#if defined(MTK_BQ24196_SUPPORT)
#include <platform/bq24196.h>
#endif

#if defined(MTK_BQ24261_SUPPORT)
#include <platform/bq24261.h>
#endif

#if defined(MTK_RT9466_SUPPORT)
#include <platform/rt9466.h>
#endif

#if defined(MTK_BQ25890_SUPPORT) || defined(MTK_BQ25896_SUPPORT)
#include <platform/bq25890.h>
#endif

#if defined(MTK_BQ25892_SUPPORT)
#include <platform/bq25892.h>
#endif

#undef printf


/*****************************************************************************
 *  Type define
 ****************************************************************************/
typedef struct {
	kal_int32 bat_vol;
	kal_int32 temperature;
	CHARGER_TYPE charger_type;
	kal_int32 lowvol_threshold;
	kal_int32 lowvol_to_display;
} PMU_ChargerStruct;

typedef enum {
	PMU_STATUS_OK = 0,
	PMU_STATUS_FAIL = 1,
} PMU_STATUS;

/*****************************************************************************
 *  Global Variable
 ****************************************************************************/
bool g_boot_reason_change = false;
PMU_ChargerStruct BMT_status;
kal_int32 g_temp_status=TEMP_POS_10_TO_POS_45;
kal_int32 g_bat_temperature_pre = 22;

int g_R_BAT_SENSE = R_BAT_SENSE;
int g_R_I_SENSE = R_I_SENSE;
int g_R_CHARGER_1 = R_CHARGER_1;
int g_R_CHARGER_2 = R_CHARGER_2;

charger_control_interface charger;

/*****************************************************************************
 *  Externl Variable
 ****************************************************************************/
extern bool g_boot_menu;

/*****************************************************************************
 *  Move to PMIC
 ****************************************************************************/

// @@@ menghui: need confirm below setting again.
void pmic_init_for_charger(void)
{
	upmu_set_rg_chrwdt_td(0x0);           // CHRWDT_TD, 4s
	upmu_set_rg_chrwdt_int_en(0);         // CHRWDT_INT_EN
	upmu_set_rg_chrwdt_en(0);             // CHRWDT_EN
	upmu_set_rg_chrwdt_wr(0);             // CHRWDT_WR

	upmu_set_rg_csdac_mode(0);      //CSDAC_MODE

	upmu_set_rg_vcdt_mode(0);       //VCDT_MODE

	upmu_set_rg_bc11_bb_ctrl(1);    //BC11_BB_CTRL
	upmu_set_rg_bc11_rst(1);        //BC11_RST

	upmu_set_rg_csdac_mode(0);      //CSDAC_MODE
	upmu_set_rg_vbat_ov_en(0);      //VBAT_OV_EN
	upmu_set_rg_vbat_ov_vth(0x0);   //VBAT_OV_VTH, 4.3V
	upmu_set_rg_baton_en(1);        //BATON_EN

	//Tim, for TBAT
	upmu_set_rg_buf_pwd_b(1);       //RG_BUF_PWD_B
	upmu_set_rg_baton_ht_en(0);     //BATON_HT_EN

	upmu_set_rg_ulc_det_en(0);      // RG_ULC_DET_EN=1
	upmu_set_rg_low_ich_db(1);      // RG_LOW_ICH_DB=000001'b

	upmu_set_rg_chr_en(0);           // CHR_EN
	upmu_set_rg_hwcv_en(0);          // RG_HWCV_EN
}

kal_int32 get_battery_voltage(void)
{
	return get_i_sense_volt(5);
}

kal_int32 get_tbat_voltage(void)
{
	return get_tbat_volt(5);
}

void external_charger_init(void)
{
	static bool init_done = false;

	if (init_done == true)
		return;

	/* set default type detection handler to PMIC */
	charger.charger_type_detection = mt_charger_type_detection;

#if defined(MTK_BQ24297_SUPPORT) || defined(MTK_BQ24296_SUPPORT)
	probe_bq24297();
#endif

#if defined(MTK_RT9466_SUPPORT)
	rt9466_probe();
#endif

#if defined(MTK_BQ24196_SUPPORT)
	probe_bq24196();
#endif

#if defined(MTK_BQ24261_SUPPORT)
	probe_bq24261();
#endif

#if defined(MTK_BQ25890_SUPPORT) || defined(MTK_BQ25896_SUPPORT)
	probe_bq25890();
#endif

	init_done = true;
}
/*****************************************************************************
 *
 ****************************************************************************/
void set_charging_current(kal_int32 cc)
{
	if (charger.set_charging_current)
		charger.set_charging_current(cc);
}

void set_input_current_limit(kal_int32 limit)
{
	if (charger.set_input_current_limit)
		charger.set_input_current_limit(limit);
}

void set_reg_voltage(kal_int32 cv)
{
	if (charger.set_reg_voltage)
		charger.set_reg_voltage(cv);
}

void charging_hw_init(void)
{
	if (charger.charging_hw_init)
		charger.charging_hw_init();
}

void enable_charging(kal_bool enable)
{
	if (charger.enable_charging)
		charger.enable_charging(enable);
}

void charger_dump_register(void)
{
	if (charger.charger_dump_register)
		charger.charger_dump_register();
}

void kick_ext_charger_watchdog(void)
{
	if (charger.kick_ext_charger_watchdog)
		charger.kick_ext_charger_watchdog();
}

#if defined(MTK_JEITA_STANDARD_SUPPORT)
kal_int32 get_jeita_charging_current(void)
{
	if (g_temp_status == TEMP_NEG_10_TO_POS_0)
		return 0;
	else if (g_temp_status == TEMP_POS_0_TO_POS_10)
		return 512;
	else if (g_temp_status == TEMP_POS_10_TO_POS_45) {
		if (BMT_status.temperature <= 23)
			return 1536;
		else
			return 2112;
	} else
		return 1536;
}

void set_jeita_cv(void)
{
	if (g_temp_status == TEMP_NEG_10_TO_POS_0)
		set_reg_voltage(4000);
	else if (g_temp_status == TEMP_POS_10_TO_POS_45)
		set_reg_voltage(4200);
	else
		set_reg_voltage(4000);
}
#endif

void select_charging_current(void)
{
	kal_int32 cc_value, input_limit;

	if (BMT_status.charger_type == STANDARD_HOST) {
		cc_value = USB_CHARGER_CURRENT;
		input_limit = 500;
	} else if (BMT_status.charger_type == NONSTANDARD_CHARGER) {
		cc_value = AC_CHARGER_CURRENT;
		input_limit = 2000;
	} else if (BMT_status.charger_type == STANDARD_CHARGER) {
		cc_value = AC_CHARGER_CURRENT;
		input_limit = 2000;
	} else if (BMT_status.charger_type == CHARGING_HOST) {
		cc_value = AC_CHARGER_CURRENT;
		input_limit = 2000;
	} else {
		dprintf(CRITICAL, "Unknown charger type!\n");
		cc_value = 500;
		input_limit = 500;
	}

#if defined(MTK_JEITA_STANDARD_SUPPORT)
	cc_value = get_jeita_charging_current();
#endif
	set_charging_current(cc_value);
#if defined(MTK_BQ24297_SUPPORT) || defined(MTK_BQ25890_SUPPORT)
	dprintf(CRITICAL, "charger_type=%d,Use detected input current limit instead of %d,cc_value=%d\n",BMT_status.charger_type, input_limit,cc_value);
//#else
	set_input_current_limit(input_limit);
#endif
}

static int charger_watchdog_handler(void *arg)
{
	for (;;) {
		dprintf(CRITICAL, "kick charger IC watchdog.\n");
		kick_ext_charger_watchdog();
		thread_sleep(15000);
	}
	return 0;
}

/* convert register to temperature  */
kal_int32 BattThermistorConverTemp(kal_int32 Res)
{
	kal_int32 i = 0;
	kal_int32 RES1 = 0, RES2 = 0;
	kal_int32 TBatt_Value = -200, TMP1 = 0, TMP2 = 0;
	kal_int32 saddles = sizeof(Batt_Temperature_Table)/sizeof(BATT_TEMPERATURE);

	if (Res >= Batt_Temperature_Table[0].TemperatureR)
		TBatt_Value = Batt_Temperature_Table[0].BatteryTemp;
	else if (Res <= Batt_Temperature_Table[saddles-1].TemperatureR)
		TBatt_Value = Batt_Temperature_Table[saddles-1].BatteryTemp;
	else {
		RES1 = Batt_Temperature_Table[0].TemperatureR;
		TMP1 = Batt_Temperature_Table[0].BatteryTemp;

		for (i = 0; i <= saddles-1; i++) {
			if (Res >= Batt_Temperature_Table[i].TemperatureR) {
				RES2 = Batt_Temperature_Table[i].TemperatureR;
				TMP2 = Batt_Temperature_Table[i].BatteryTemp;
				break;
			} else {
				RES1 = Batt_Temperature_Table[i].TemperatureR;
				TMP1 = Batt_Temperature_Table[i].BatteryTemp;
			}
		}

		TBatt_Value = (((Res - RES2) * TMP1) + ((RES1 - Res) * TMP2)) / (RES1-RES2);
	}

	return TBatt_Value;
}

kal_int32 BattVoltToTemp(kal_int32 dwVolt)
{
	kal_int32 TRes;
	kal_int32 dwVCriBat = (TBAT_OVER_CRITICAL_LOW * RBAT_PULL_UP_VOLT) / (TBAT_OVER_CRITICAL_LOW + RBAT_PULL_UP_R);
	kal_int32 sBaTTMP = -100;

	if (dwVolt > dwVCriBat)
		TRes = TBAT_OVER_CRITICAL_LOW;
	else
		TRes = (RBAT_PULL_UP_R*dwVolt) / (RBAT_PULL_UP_VOLT-dwVolt);

	/* convert register to temperature */
	sBaTTMP = BattThermistorConverTemp(TRes);

	dprintf(CRITICAL, "BattVoltToTemp() : TBAT_OVER_CRITICAL_LOW = %d\n", TBAT_OVER_CRITICAL_LOW);
	dprintf(CRITICAL, "BattVoltToTemp() : RBAT_PULL_UP_VOLT = %d\n", RBAT_PULL_UP_VOLT);
	dprintf(CRITICAL, "BattVoltToTemp() : dwVolt = %d\n", dwVolt);
	dprintf(CRITICAL, "BattVoltToTemp() : TRes = %d\n", TRes);
	dprintf(CRITICAL, "BattVoltToTemp() : sBaTTMP = %d\n", sBaTTMP);

#ifdef BEFORE_TREF_REWORK
	if (upmu_get_cid() == 0x1020)
		sBaTTMP=22;
#endif
	return 25;
}

void updateBatTemperature(void)
{
	kal_int32 bat_temperature_volt = get_tbat_voltage();

#ifdef MTK_DISABLE_POWER_ON_OFF_VOLTAGE_LIMITATION
	BMT_status.temperature = 25;
	g_bat_temperature_pre = 25;
	return;
#endif

	if (bat_temperature_volt == 0) {
		BMT_status.temperature = g_bat_temperature_pre;
	} else {
		BMT_status.temperature = BattVoltToTemp(bat_temperature_volt);
		g_bat_temperature_pre = BMT_status.temperature;
	}
}

#if defined(MTK_JEITA_STANDARD_SUPPORT)
int do_jeita_state_machine(void)
{
	//JEITA battery temp Standard
	if (BMT_status.temperature >= TEMP_POS_60_THRESHOLD) {
		dprintf(CRITICAL, "[BATTERY] Battery Over high Temperature(%d) !!\n\r", TEMP_POS_60_THRESHOLD);
		g_temp_status = TEMP_ABOVE_POS_60;
		return PMU_STATUS_FAIL;
	} else if (BMT_status.temperature > TEMP_POS_45_THRESHOLD) {

		if ((g_temp_status == TEMP_ABOVE_POS_60) && (BMT_status.temperature >= TEMP_POS_60_THRES_MINUS_X_DEGREE)) {
			dprintf(CRITICAL, "[BATTERY] Battery Temperature between %d and %d,not allow charging yet!!\n\r", TEMP_POS_60_THRES_MINUS_X_DEGREE,TEMP_POS_60_THRESHOLD);
			return PMU_STATUS_FAIL;
		} else {
			dprintf(CRITICAL, "[BATTERY] Battery Temperature between %d and %d !!\n\r", TEMP_POS_45_THRESHOLD,TEMP_POS_60_THRESHOLD);
			g_temp_status = TEMP_POS_45_TO_POS_60;
		}
	} else if (BMT_status.temperature >= TEMP_POS_10_THRESHOLD) {
		if ( ((g_temp_status == TEMP_POS_45_TO_POS_60) && (BMT_status.temperature >= TEMP_POS_45_THRES_MINUS_X_DEGREE)) ||
		        ((g_temp_status == TEMP_POS_0_TO_POS_10 ) && (BMT_status.temperature <= TEMP_POS_10_THRES_PLUS_X_DEGREE ))) {
			dprintf(CRITICAL, "[BATTERY] Battery Temperature not recovery to normal temperature charging mode yet!!\n\r");
		} else {

			dprintf(CRITICAL, "[BATTERY] Battery Normal Temperature between %d and %d !!\n\r", TEMP_POS_10_THRESHOLD,TEMP_POS_45_THRESHOLD);

			g_temp_status = TEMP_POS_10_TO_POS_45;
		}
	} else if (BMT_status.temperature >= TEMP_POS_0_THRESHOLD) {
		if ((g_temp_status == TEMP_NEG_10_TO_POS_0 || g_temp_status == TEMP_BELOW_NEG_10) && (BMT_status.temperature <= TEMP_POS_0_THRES_PLUS_X_DEGREE)) {
			if (g_temp_status == TEMP_NEG_10_TO_POS_0) {
				dprintf(CRITICAL, "[BATTERY] Battery Temperature between %d and %d !!\n\r", TEMP_POS_0_THRES_PLUS_X_DEGREE,TEMP_POS_10_THRESHOLD);
			}
			if (g_temp_status == TEMP_BELOW_NEG_10) {
				dprintf(CRITICAL, "[BATTERY] Battery Temperature between %d and %d,not allow charging yet!!\n\r", TEMP_POS_0_THRESHOLD,TEMP_POS_0_THRES_PLUS_X_DEGREE);
				return PMU_STATUS_FAIL;
			}
		} else {
			dprintf(CRITICAL, "[BATTERY] Battery Temperature between %d and %d !!\n\r", TEMP_POS_0_THRESHOLD,TEMP_POS_10_THRESHOLD);
			g_temp_status = TEMP_POS_0_TO_POS_10;
		}
	} else if (BMT_status.temperature >= TEMP_NEG_10_THRESHOLD) {
		if ((g_temp_status == TEMP_BELOW_NEG_10) && (BMT_status.temperature <= TEMP_NEG_10_THRES_PLUS_X_DEGREE)) {
			dprintf(CRITICAL, "[BATTERY] Battery Temperature between %d and %d,not allow charging yet!!\n\r", TEMP_NEG_10_THRESHOLD,TEMP_NEG_10_THRES_PLUS_X_DEGREE);
			return PMU_STATUS_FAIL;
		} else {
			dprintf(CRITICAL, "[BATTERY] Battery Temperature between %d and %d !!\n\r", TEMP_NEG_10_THRESHOLD,TEMP_POS_0_THRESHOLD);
			g_temp_status = TEMP_NEG_10_TO_POS_0;
		}
	} else {
		dprintf(CRITICAL, "[BATTERY] Battery below low Temperature(%d) !!\n\r", TEMP_NEG_10_THRESHOLD);
		g_temp_status = TEMP_BELOW_NEG_10;
		return PMU_STATUS_FAIL;
	}
	return PMU_STATUS_OK;
}
#endif

int get_low_bat_by_tbat(void)
{
	updateBatTemperature();

	if (BMT_status.temperature >= 0) {
		BMT_status.lowvol_threshold = 3500;
		BMT_status.lowvol_to_display = 3300;
	} else {
		BMT_status.lowvol_threshold = 3650;
		BMT_status.lowvol_to_display = 3550;
	}

#if defined(MTK_JEITA_STANDARD_SUPPORT)
	if (BMT_status.temperature > 60 || BMT_status.temperature < -10) {
		dprintf(CRITICAL, "TBAT(%d) out of range. Need shutdown device.\n", BMT_status.temperature);
		mt6575_power_off();
	}
#endif

	return BMT_status.lowvol_threshold;
}

kal_bool is_low_battery(void)
{
	static kal_int32 is_low = 0xFF;

	if (is_low != 0xFF)
		return (is_low == 1 ? KAL_TRUE : KAL_FALSE);

	external_charger_init();

	enable_charging(KAL_FALSE);
	mdelay(100);
	BMT_status.bat_vol = get_battery_voltage();

	get_low_bat_by_tbat();

	dprintf(CRITICAL, "check VBAT=%d mV with %d mV\n", BMT_status.bat_vol, BMT_status.lowvol_threshold);

	if (BMT_status.bat_vol < BMT_status.lowvol_threshold) {
		dprintf(CRITICAL, "%s, TRUE\n", __FUNCTION__);
		is_low = 1;
		return KAL_TRUE;
	}

	dprintf(CRITICAL, "%s, FALSE\n", __FUNCTION__);
	is_low = 0;
	return KAL_FALSE;
}

void display_low_battery(int charged)
{
	long tmo2;
	if (BMT_status.bat_vol >= BMT_status.lowvol_to_display) {
		dprintf(CRITICAL, "%s\n", __FUNCTION__);
		//display low battery icon for 5s
		mt_disp_power(TRUE);
		if (charged)
			mt_disp_show_low_battery();
		else
			mt_disp_show_battery_capacity(1);
		//mt_disp_wait_idle();
		mt65xx_leds_brightness_set(MT65XX_LED_TYPE_LCD, 30);

		mtk_wdt_restart();

		tmo2 = get_timer(0);
		while (get_timer(tmo2) <= 5000);
		mt65xx_backlight_off();

		// clear framebuffer.
		mt_disp_fill_rect(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT, 0x0);
		mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
	}
}

void turn_on_charging(void)
{
	charging_hw_init();
#if defined(MTK_JEITA_STANDARD_SUPPORT)
	set_jeita_cv();
#endif
	select_charging_current();
	enable_charging(KAL_TRUE);
}

void check_bat_protect_status()
{
	int first_time_screen_on = 0;
	long tmo2;
	unsigned long charging_total_time = 0;
	unsigned long charging_start_time = 0;

	dprintf(CRITICAL, "%s\n", __FUNCTION__);

	if (BMT_status.charger_type == CHARGER_UNKNOWN) {
		dprintf(CRITICAL, "no trap, keep booting!\n");
		return;
	}

	enable_charging(KAL_FALSE);
	mdelay(50);
	BMT_status.bat_vol = get_battery_voltage();

	dprintf(CRITICAL, "battery voltage = %d mV\n", BMT_status.bat_vol);

	charging_start_time = get_rtc_time();

	while (BMT_status.bat_vol < get_low_bat_by_tbat()) {
		rtc_boot_check(false);
		mtk_wdt_restart();
		if (upmu_is_chr_det() == KAL_FALSE) {
			dprintf(CRITICAL, "no charger! power off!\n");
			enable_charging(KAL_FALSE);
			mt6575_power_off();
			while (1);
		}

#if defined(MTK_JEITA_STANDARD_SUPPORT)
		if (do_jeita_state_machine() == PMU_STATUS_FAIL)
#else
		if (BMT_status.temperature > MAX_CHARGE_TEMPERATURE ||
		        BMT_status.temperature < MIN_CHARGE_TEMPERATURE)
#endif
		{
			dprintf(CRITICAL, "[BATTERY] JEITA : out of charging temperature range\n");
			enable_charging(KAL_FALSE);
			kick_ext_charger_watchdog();

			tmo2 = get_timer(0);
			while (get_timer(tmo2) <= 5000 /* ms */);

			continue;
		}

		if (charging_total_time < 21600) /* check 6 hours limit */
			turn_on_charging();
		else {
			enable_charging(KAL_FALSE);
			dprintf(CRITICAL, "Charging time over 6 hours! Stop charging!\n");
		}
		charger_dump_register();

		tmo2 = get_timer(0);
		while (get_timer(tmo2) <= 5000 /* ms */) {
			if ((!first_time_screen_on || pmic_detect_powerkey())
			        && (BMT_status.bat_vol >= BMT_status.lowvol_to_display)) {

				first_time_screen_on = 1;
				display_low_battery(1);
			}
			mdelay(200); // A fine tuned value
			mtk_wdt_restart();
		}

		charging_total_time = get_rtc_time() - charging_start_time;
		BMT_status.bat_vol = get_battery_voltage();
		if (BMT_status.bat_vol > 3400) {
			enable_charging(KAL_FALSE);
			mdelay(100);
			BMT_status.bat_vol = get_battery_voltage();
		}
		dprintf(CRITICAL, "check battery voltage = %d mV against %d mV.\n", BMT_status.bat_vol, get_low_bat_by_tbat());
	}
	enable_charging(KAL_FALSE);
	mtk_wdt_restart();
}

/*
Some USB CDP port implementation requires USB connection after BC1.1/BC1.2 type
Otherwise the port falls back to SDP by re-asserting vbus.
Here we enable USB in case of CDP port.
*/
static void init_usb_for_cdp(void)
{
	static struct udc_device surf_udc_device = {
		.vendor_id = USB_VENDORID,
		.product_id = USB_PRODUCTID,
		.version_id = USB_VERSIONID,
		.manufacturer = USB_MANUFACTURER,
		.product = USB_PRODUCT_NAME,
	};

	dprintf(CRITICAL, "init usb for CDP port.\n");
	udc_init(&surf_udc_device);
	mt_usb_phy_recover();
}

void mt65xx_bat_init(void)
{
	external_charger_init();

	if ((upmu_is_chr_det() == KAL_TRUE)) {
		pmic_init_for_charger();

		if (charger.charger_type_detection)
			BMT_status.charger_type = charger.charger_type_detection();

		dprintf(CRITICAL, "detected charger type: %d.\n", BMT_status.charger_type);
		if (BMT_status.charger_type == CHARGING_HOST)
			init_usb_for_cdp();

		charging_hw_init();
		// enable power path but disable charging for meter to get precise battery voltage.
		enable_charging(KAL_FALSE);
	}

	if (g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT && (upmu_get_pwrkey_deb()==0) ) {
		dprintf(CRITICAL, "[mt65xx_bat_init] KPOC+PWRKEY => change boot mode\n");
		g_boot_reason_change = true;
	}

	rtc_boot_check(false);
	charger_dump_register();

	// kick watchdog for fastboot download
	if (g_boot_mode == FASTBOOT) {
		thread_t *thr;

		dprintf(CRITICAL, "[mt65xx_bat_init] create kick-watchdog thread.\n");
		thr = thread_create("charger_fb", charger_watchdog_handler, 0, DEFAULT_PRIORITY, 4096);
		if (!thr)
			dprintf(CRITICAL, "[mt65xx_bat_init] fail to create kick-watchdog thread.\n");
		else
			thread_resume(thr);
	}

#ifndef MTK_DISABLE_POWER_ON_OFF_VOLTAGE_LIMITATION
	if (is_low_battery() == KAL_TRUE) {
		if (g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT && upmu_is_chr_det() == KAL_TRUE) {
			check_bat_protect_status();
			return;
		} else {

#ifdef MTK_AUTO_POWER_ON_WITH_CHARGER
			if (upmu_is_chr_det() == KAL_TRUE && g_boot_mode == NORMAL_BOOT) {
				check_bat_protect_status();
				return;
			}
#endif

			if (BMT_status.charger_type == STANDARD_CHARGER) {
				dprintf(CRITICAL, "bq24297 with power path and use standard charger, keep booting!\n");
				return;
			} else {
				dprintf(CRITICAL, "NOT use standard charger, power path is forbidden!\n");
			}

			if (upmu_is_chr_det() == KAL_TRUE && BMT_status.charger_type != STANDARD_CHARGER) {
				check_bat_protect_status();
				g_boot_mode = KERNEL_POWER_OFF_CHARGING_BOOT;
				return;
			}

			dprintf(CRITICAL, "[BATTERY] battery voltage(%dmV) <= CLV ! Can not Boot Linux Kernel !! \n\r",BMT_status.bat_vol);

#ifdef MTK_AUTO_POWER_ON_WITH_CHARGER
			/* force display sign-of-life if VBAT > 3100mV. */
			BMT_status.lowvol_to_display = 3100;
			display_low_battery(0);
#endif

#ifndef NO_POWER_OFF
			mt6575_power_off();
#endif
			while (1) {
				dprintf(CRITICAL, "If you see the log, please check with RTC power off API\n\r");
			}
		}
	}
#endif
	return;
}

#else

#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <printf.h>
#include <target/cust_battery.h>

int g_R_BAT_SENSE = R_BAT_SENSE;
int g_R_I_SENSE = R_I_SENSE;
int g_R_CHARGER_1 = R_CHARGER_1;
int g_R_CHARGER_2 = R_CHARGER_2;

void mt65xx_bat_init(void)
{
	dprintf(CRITICAL, "[BATTERY] Skip mt65xx_bat_init !!\n\r");
	dprintf(CRITICAL, "[BATTERY] If you want to enable power off charging, \n\r");
	dprintf(CRITICAL, "[BATTERY] Please #define CFG_POWER_CHARGING!!\n\r");
}

kal_bool is_low_battery(void)
{
	return KAL_FALSE;
}
#endif
