#include <printf.h>
#include <platform/mt_typedefs.h>
#include <platform/mtk_key.h>
#include <platform/boot_mode.h>
#include <platform/mt_pmic.h>
#include <platform/mt_gpio.h>
#include <platform/mt_pmic_wrap_init.h>
#include <platform/sync_write.h>
#include <target/cust_key.h>

extern int pmic_detect_powerkey(void);
extern int pmic_detect_homekey(void);

void set_kpd_pmic_mode(void)
{
	return;
}

void disable_PMIC_kpd_clock(void)
{
	return;
}

void enable_PMIC_kpd_clock(void)
{
	return;
}

BOOL mtk_detect_key(unsigned short key) /* key: HW keycode */
{
	/* Because 6572 FPGA doesn't include keypad HW,
	 * add FPGA macro to avoid misjudgment
	 */
#ifdef MACH_FPGA
	return FALSE;
#else

#ifdef MTK_ALPS_BOX_SUPPORT
#else
	printf("mtk detect key function key = %d\n", key);
#endif

	unsigned short idx, bit, din;

	if (key >= KPD_NUM_KEYS)
		return FALSE;

	if (key % 9 == 8)
		key = 8;

	if (key == 8) {
		/* Power key */
		if (1 == pmic_detect_powerkey()) {
			//dbg_print ("power key is pressed\n");
			return TRUE;
		}
		return FALSE;
	}

#ifdef MT65XX_PMIC_RST_KEY
	if (key == MT65XX_PMIC_RST_KEY) {
		printf("mtk detect key function pmic_detect_homekey MT65XX_PMIC_RST_KEY = %d\n",MT65XX_PMIC_RST_KEY);
		if (1 == pmic_detect_homekey()) {
			printf("mtk detect key function pmic_detect_homekey pressed\n");
			return TRUE;
		}
		return FALSE;
	}
#endif

	idx = key / 16;
	bit = key % 16;

	din = DRV_Reg16(KP_MEM1 + (idx << 2)) & (1U << bit);
	if (!din) {
		printf("key %d is pressed\n", key);
		return TRUE;
	}
	return FALSE;
#endif
}

BOOL mtk_detect_pmic_just_rst(void)
{
	kal_uint32 just_rst=0;

	printf("detecting pmic just reset\n");

	pmic_read_interface(0x050C, &just_rst, 0x01, 14);
	if (just_rst) {
		printf("Just recover form a reset\n");
		pmic_config_interface(0x050C, 0x01, 0x01, 4);
		return TRUE;
	}
	return FALSE;
}
