#include <platform/sync_write.h>
#include <cust_leds.h>
#include <platform/ddp_reg.h>
#include <platform/ddp_pwm.h>
#include "platform/ddp_info.h"
#include <printf.h>

int hw_mutex_id_for_pwm = 4;
int hw_mutex_pwm_mod_id = 23;
int hw_mutex_pwm_mod_mode = 0;
#define PWM_REG_SET(reg, value) do { \
        mt65xx_reg_sync_writel(value, (volatile unsigned int*)(reg)); \
        PWM_MSG("set reg[0x%08x] = 0x%08x", reg, value); \
    } while (0)

#define PWM_REG_GET(reg) *((volatile unsigned int*)reg)

#define PWM_DEFAULT_DIV_VALUE 0x0

#define PWM_MSG(fmt, arg...) printf("[PWM] " fmt "\n", ##arg)

#define pwm_get_reg_base(id) ((id) == DISP_PWM0 ? DISPSYS_PWM0_BASE : DISPSYS_PWM1_BASE)

#define index_of_pwm(id) ((id == DISP_PWM0) ? 0 : 1)

static int g_pwm_inited = 0;
static disp_pwm_id_t g_pwm_main_id = DISP_PWM0;

void disp_pwm_init(disp_pwm_id_t id)
{
#if 0
	unsigned int ori_val;
	struct cust_mt65xx_led *cust_led_list;
	struct cust_mt65xx_led *cust;
	struct PWM_config *config_data;
#endif
	unsigned int pwm_div;
	unsigned int reg_base = pwm_get_reg_base(id);
	/* int index = index_of_pwm(id); */
	if (id == DISP_PWM0 || id == DISP_PWM_ALL)
		ddp_enable_module_clock(DISP_MODULE_PWM0);
	else if (id == DISP_PWM1 || id == DISP_PWM_ALL)
		ddp_enable_module_clock(DISP_MODULE_PWM1);



	if (g_pwm_inited & id)
		return;
	PWM_REG_SET(DISP_REG_CONFIG_MUTEX_MOD(hw_mutex_id_for_pwm),1<<hw_mutex_pwm_mod_id);
	PWM_REG_SET(DISP_REG_CONFIG_MUTEX_SOF(hw_mutex_id_for_pwm),hw_mutex_pwm_mod_mode);

	pwm_div = PWM_DEFAULT_DIV_VALUE;
#if 0 /* Not ported yet on LDVT */
	cust_led_list = get_cust_led_list();
	if (cust_led_list) {
		/* WARNING: may overflow if MT65XX_LED_TYPE_LCD not configured properly */
		cust = &cust_led_list[MT65XX_LED_TYPE_LCD];
		if ((strcmp(cust->name,"lcd-backlight") == 0) && (cust->mode == MT65XX_LED_MODE_CUST_BLS_PWM)) {
			config_data = &cust->config_data;
			if (config_data->clock_source >= 0 && config_data->clock_source <= 3) {
				unsigned int regVal = DISP_REG_GET(CLK_CFG_1);
				clkmux_sel(MT_MUX_PWM, config_data->clock_source, "DISP_PWM");
				PWM_MSG("disp_pwm_init : CLK_CFG_1 0x%x => 0x%x", regVal, DISP_REG_GET(CLK_CFG_1));
			}
			/* Some backlight chip/PMIC(e.g. MT6332) only accept slower clock */
			pwm_div = (config_data->div == 0) ? PWM_DEFAULT_DIV_VALUE : config_data->div;
			pwm_div &= 0x3FF;
			PWM_MSG("disp_pwm_init : PWM config data (%d,%d)", config_data->clock_source, config_data->div);
		}
	}
#endif

	PWM_REG_SET(reg_base + DISP_PWM_CON_0_OFF, pwm_div << 16);

	PWM_REG_SET(reg_base + DISP_PWM_CON_1_OFF, 1023); /* 1024 levels */

	g_pwm_main_id = id;

	g_pwm_inited |= id;
}


disp_pwm_id_t disp_pwm_get_main(void)
{
	return g_pwm_main_id;
}


int disp_pwm_is_enabled(disp_pwm_id_t id)
{
	unsigned int reg_base = pwm_get_reg_base(id);
	return (PWM_REG_GET(reg_base + DISP_PWM_EN_OFF) & 0x1);
}


static void disp_pwm_set_enabled(disp_pwm_id_t id, int enabled)
{
	unsigned int reg_base = pwm_get_reg_base(id);
	if (enabled) {
		if (!disp_pwm_is_enabled(id)) {
			PWM_REG_SET(reg_base + DISP_PWM_EN_OFF, 0x1);
			PWM_MSG("disp_pwm_set_enabled: PWN_EN = 0x1");
		}
	} else {
		PWM_REG_SET(reg_base + DISP_PWM_EN_OFF, 0x0);
	}
}


/* For backward compatible */
int disp_bls_set_backlight(int level_256)
{
	int level_1024 = 0;

	if (level_256 <= 0)
		level_1024 = 0;
	else if (level_256 >= 255)
		level_1024 = 1023;
	else {
		level_1024 = (level_256 << 2) + 2;
	}
	return disp_pwm_set_backlight(disp_pwm_get_main(), level_1024);
}


int disp_pwm_set_backlight(disp_pwm_id_t id, int level_1024)
{
	unsigned int reg_base = 0;
	unsigned int offset;

	if ((DISP_PWM_ALL & id) == 0) {
		PWM_MSG("[ERROR] disp_pwm_set_backlight: invalid PWM ID = 0x%x", id);
		return -1;
	}
	reg_base = pwm_get_reg_base(id);

	PWM_REG_SET(reg_base + DISP_PWM_COMMIT_OFF, 0);

	disp_pwm_init(id);

	PWM_MSG("disp_pwm_set_backlight(id = 0x%x, level_1024 = %d)", id, level_1024);


	PWM_REG_SET(reg_base + DISP_PWM_CON_1_OFF, (level_1024 << 16) | 0x3ff);

	if (level_1024 > 0) {
		mdelay(20);
		disp_pwm_set_enabled(id, 1);
	} else {
		mdelay(20);
		disp_pwm_set_enabled(id, 0); /* To save power */
	}

	PWM_REG_SET(reg_base + DISP_PWM_COMMIT_OFF, 1);
	PWM_REG_SET(reg_base + DISP_PWM_COMMIT_OFF, 0);

	for (offset = 0x0; offset <= 0x28; offset += 4) {
		PWM_MSG("reg[0x%08x] = 0x%08x", (reg_base + offset), PWM_REG_GET(reg_base + offset));
	}
	PWM_REG_SET(DISP_REG_CONFIG_MUTEX_EN(hw_mutex_id_for_pwm),1);

	return 0;
}

