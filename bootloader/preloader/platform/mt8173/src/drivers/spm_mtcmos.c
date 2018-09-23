/* XXX: only in kernel
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/delay.h>    //udelay

#include <mach/mt_typedefs.h>
#include <mach/mt_spm.h>
#include <mach/mt_spm_mtcmos.h>
#include <mach/hotplug.h>
#include <mach/mt_clkmgr.h>
*/
#include <platform.h>
#include <spm.h>
#include <spm_mtcmos.h>
#include <spm_mtcmos_internal.h>
#include <timer.h>  //udelay
#include <pll.h>
#include <mt_ptp2.h>

/**************************************
 * for CPU MTCMOS
 **************************************/
/* XXX: only in kernel
static DEFINE_SPINLOCK(spm_cpu_lock);


void spm_mtcmos_cpu_lock(unsigned long *flags)
{
    spin_lock_irqsave(&spm_cpu_lock, *flags);
}

void spm_mtcmos_cpu_unlock(unsigned long *flags)
{
    spin_unlock_irqrestore(&spm_cpu_lock, *flags);
}
*/
#define spm_mtcmos_cpu_lock(x)
#define spm_mtcmos_cpu_unlock(x)
#define spm_mtcmos_noncpu_lock(x)
#define spm_mtcmos_noncpu_unlock(x)


typedef int (*spm_cpu_mtcmos_ctrl_func) (int state, int chkWfiBeforePdn);
static spm_cpu_mtcmos_ctrl_func spm_cpu_mtcmos_ctrl_funcs[] = {
	spm_mtcmos_ctrl_cpu0,
	spm_mtcmos_ctrl_cpu1,
	spm_mtcmos_ctrl_cpu2,
	spm_mtcmos_ctrl_cpu3,
	spm_mtcmos_ctrl_cpu4,
	spm_mtcmos_ctrl_cpu5,
	spm_mtcmos_ctrl_cpu6,
	spm_mtcmos_ctrl_cpu7
};

int spm_mtcmos_ctrl_cpu(unsigned int cpu, int state, int chkWfiBeforePdn)
{
	return (*spm_cpu_mtcmos_ctrl_funcs[cpu])(state, chkWfiBeforePdn);
}

int spm_mtcmos_ctrl_cpu0(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(SPM_SLEEP_TIMER_STA) & CA7_CPU0_STANDBYWFI) == 0);

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA7_CPU0_PWR_CON, spm_read(SPM_CA7_CPU0_PWR_CON) | PWR_ISO);

		spm_write(SPM_CA7_CPU0_PWR_CON, spm_read(SPM_CA7_CPU0_PWR_CON) | SRAM_CKISO);
		spm_write(SPM_CA7_CPU0_PWR_CON, spm_read(SPM_CA7_CPU0_PWR_CON) & ~SRAM_ISOINT_B);
		spm_write(SPM_CA7_CPU0_L1_PDN, spm_read(SPM_CA7_CPU0_L1_PDN) | L1_PDN);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_CA7_CPU0_L1_PDN) & L1_PDN_ACK) != L1_PDN_ACK);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA7_CPU0_PWR_CON, spm_read(SPM_CA7_CPU0_PWR_CON) & ~PWR_RST_B);
		spm_write(SPM_CA7_CPU0_PWR_CON, spm_read(SPM_CA7_CPU0_PWR_CON) | PWR_CLK_DIS);

		spm_write(SPM_CA7_CPU0_PWR_CON, spm_read(SPM_CA7_CPU0_PWR_CON) & ~PWR_ON);
		spm_write(SPM_CA7_CPU0_PWR_CON, spm_read(SPM_CA7_CPU0_PWR_CON) & ~PWR_ON_2ND);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while (((spm_read(SPM_PWR_STATUS) & CA7_CPU0) != 0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA7_CPU0) != 0));
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_mtcmos_cpu_unlock(&flags);
	} else {		/* STA_POWER_ON */

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA7_CPU0_PWR_CON, spm_read(SPM_CA7_CPU0_PWR_CON) | PWR_ON);
		udelay(1);
		spm_write(SPM_CA7_CPU0_PWR_CON, spm_read(SPM_CA7_CPU0_PWR_CON) | PWR_ON_2ND);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while (((spm_read(SPM_PWR_STATUS) & CA7_CPU0) != CA7_CPU0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA7_CPU0) != CA7_CPU0));
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA7_CPU0_PWR_CON, spm_read(SPM_CA7_CPU0_PWR_CON) & ~PWR_ISO);

		spm_write(SPM_CA7_CPU0_L1_PDN, spm_read(SPM_CA7_CPU0_L1_PDN) & ~L1_PDN);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_CA7_CPU0_L1_PDN) & L1_PDN_ACK) != 0);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */
		udelay(1);
		spm_write(SPM_CA7_CPU0_PWR_CON, spm_read(SPM_CA7_CPU0_PWR_CON) | SRAM_ISOINT_B);
		spm_write(SPM_CA7_CPU0_PWR_CON, spm_read(SPM_CA7_CPU0_PWR_CON) & ~SRAM_CKISO);

		spm_write(SPM_CA7_CPU0_PWR_CON, spm_read(SPM_CA7_CPU0_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_CA7_CPU0_PWR_CON, spm_read(SPM_CA7_CPU0_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);
	}

	return 0;
}

int spm_mtcmos_ctrl_cpu1(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(SPM_SLEEP_TIMER_STA) & CA7_CPU1_STANDBYWFI) == 0);

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA7_CPU1_PWR_CON, spm_read(SPM_CA7_CPU1_PWR_CON) | PWR_ISO);

		spm_write(SPM_CA7_CPU1_PWR_CON, spm_read(SPM_CA7_CPU1_PWR_CON) | SRAM_CKISO);
		spm_write(SPM_CA7_CPU1_PWR_CON, spm_read(SPM_CA7_CPU1_PWR_CON) & ~SRAM_ISOINT_B);
		spm_write(SPM_CA7_CPU1_L1_PDN, spm_read(SPM_CA7_CPU1_L1_PDN) | L1_PDN);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_CA7_CPU1_L1_PDN) & L1_PDN_ACK) != L1_PDN_ACK);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA7_CPU1_PWR_CON, spm_read(SPM_CA7_CPU1_PWR_CON) & ~PWR_RST_B);
		spm_write(SPM_CA7_CPU1_PWR_CON, spm_read(SPM_CA7_CPU1_PWR_CON) | PWR_CLK_DIS);

		spm_write(SPM_CA7_CPU1_PWR_CON, spm_read(SPM_CA7_CPU1_PWR_CON) & ~PWR_ON);
		spm_write(SPM_CA7_CPU1_PWR_CON, spm_read(SPM_CA7_CPU1_PWR_CON) & ~PWR_ON_2ND);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while (((spm_read(SPM_PWR_STATUS) & CA7_CPU1) != 0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA7_CPU1) != 0));
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_mtcmos_cpu_unlock(&flags);
	} else {		/* STA_POWER_ON */

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA7_CPU1_PWR_CON, spm_read(SPM_CA7_CPU1_PWR_CON) | PWR_ON);
		udelay(1);
		spm_write(SPM_CA7_CPU1_PWR_CON, spm_read(SPM_CA7_CPU1_PWR_CON) | PWR_ON_2ND);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while (((spm_read(SPM_PWR_STATUS) & CA7_CPU1) != CA7_CPU1)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA7_CPU1) != CA7_CPU1));
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA7_CPU1_PWR_CON, spm_read(SPM_CA7_CPU1_PWR_CON) & ~PWR_ISO);

		spm_write(SPM_CA7_CPU1_L1_PDN, spm_read(SPM_CA7_CPU1_L1_PDN) & ~L1_PDN);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_CA7_CPU1_L1_PDN) & L1_PDN_ACK) != 0);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */
		udelay(1);
		spm_write(SPM_CA7_CPU1_PWR_CON, spm_read(SPM_CA7_CPU1_PWR_CON) | SRAM_ISOINT_B);
		spm_write(SPM_CA7_CPU1_PWR_CON, spm_read(SPM_CA7_CPU1_PWR_CON) & ~SRAM_CKISO);

		spm_write(SPM_CA7_CPU1_PWR_CON, spm_read(SPM_CA7_CPU1_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_CA7_CPU1_PWR_CON, spm_read(SPM_CA7_CPU1_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);
	}

	return 0;
}

int spm_mtcmos_ctrl_cpu2(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(SPM_SLEEP_TIMER_STA) & CA7_CPU2_STANDBYWFI) == 0);

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA7_CPU2_PWR_CON, spm_read(SPM_CA7_CPU2_PWR_CON) | PWR_ISO);

		spm_write(SPM_CA7_CPU2_PWR_CON, spm_read(SPM_CA7_CPU2_PWR_CON) | SRAM_CKISO);
		spm_write(SPM_CA7_CPU2_PWR_CON, spm_read(SPM_CA7_CPU2_PWR_CON) & ~SRAM_ISOINT_B);
		spm_write(SPM_CA7_CPU2_L1_PDN, spm_read(SPM_CA7_CPU2_L1_PDN) | L1_PDN);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_CA7_CPU2_L1_PDN) & L1_PDN_ACK) != L1_PDN_ACK);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA7_CPU2_PWR_CON, spm_read(SPM_CA7_CPU2_PWR_CON) & ~PWR_RST_B);
		spm_write(SPM_CA7_CPU2_PWR_CON, spm_read(SPM_CA7_CPU2_PWR_CON) | PWR_CLK_DIS);

		spm_write(SPM_CA7_CPU2_PWR_CON, spm_read(SPM_CA7_CPU2_PWR_CON) & ~PWR_ON);
		spm_write(SPM_CA7_CPU2_PWR_CON, spm_read(SPM_CA7_CPU2_PWR_CON) & ~PWR_ON_2ND);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while (((spm_read(SPM_PWR_STATUS) & CA7_CPU2) != 0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA7_CPU2) != 0));
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_mtcmos_cpu_unlock(&flags);
	} else {		/* STA_POWER_ON */

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA7_CPU2_PWR_CON, spm_read(SPM_CA7_CPU2_PWR_CON) | PWR_ON);
		udelay(1);
		spm_write(SPM_CA7_CPU2_PWR_CON, spm_read(SPM_CA7_CPU2_PWR_CON) | PWR_ON_2ND);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while (((spm_read(SPM_PWR_STATUS) & CA7_CPU2) != CA7_CPU2)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA7_CPU2) != CA7_CPU2));
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA7_CPU2_PWR_CON, spm_read(SPM_CA7_CPU2_PWR_CON) & ~PWR_ISO);

		spm_write(SPM_CA7_CPU2_L1_PDN, spm_read(SPM_CA7_CPU2_L1_PDN) & ~L1_PDN);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_CA7_CPU2_L1_PDN) & L1_PDN_ACK) != 0);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */
		udelay(1);
		spm_write(SPM_CA7_CPU2_PWR_CON, spm_read(SPM_CA7_CPU2_PWR_CON) | SRAM_ISOINT_B);
		spm_write(SPM_CA7_CPU2_PWR_CON, spm_read(SPM_CA7_CPU2_PWR_CON) & ~SRAM_CKISO);

		spm_write(SPM_CA7_CPU2_PWR_CON, spm_read(SPM_CA7_CPU2_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_CA7_CPU2_PWR_CON, spm_read(SPM_CA7_CPU2_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);
	}

	return 0;
}

int spm_mtcmos_ctrl_cpu3(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(SPM_SLEEP_TIMER_STA) & CA7_CPU3_STANDBYWFI) == 0);

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA7_CPU3_PWR_CON, spm_read(SPM_CA7_CPU3_PWR_CON) | PWR_ISO);

		spm_write(SPM_CA7_CPU3_PWR_CON, spm_read(SPM_CA7_CPU3_PWR_CON) | SRAM_CKISO);
		spm_write(SPM_CA7_CPU3_PWR_CON, spm_read(SPM_CA7_CPU3_PWR_CON) & ~SRAM_ISOINT_B);
		spm_write(SPM_CA7_CPU3_L1_PDN, spm_read(SPM_CA7_CPU3_L1_PDN) | L1_PDN);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_CA7_CPU3_L1_PDN) & L1_PDN_ACK) != L1_PDN_ACK);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA7_CPU3_PWR_CON, spm_read(SPM_CA7_CPU3_PWR_CON) & ~PWR_RST_B);
		spm_write(SPM_CA7_CPU3_PWR_CON, spm_read(SPM_CA7_CPU3_PWR_CON) | PWR_CLK_DIS);

		spm_write(SPM_CA7_CPU3_PWR_CON, spm_read(SPM_CA7_CPU3_PWR_CON) & ~PWR_ON);
		spm_write(SPM_CA7_CPU3_PWR_CON, spm_read(SPM_CA7_CPU3_PWR_CON) & ~PWR_ON_2ND);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while (((spm_read(SPM_PWR_STATUS) & CA7_CPU3) != 0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA7_CPU3) != 0));
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_mtcmos_cpu_unlock(&flags);
	} else {		/* STA_POWER_ON */

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA7_CPU3_PWR_CON, spm_read(SPM_CA7_CPU3_PWR_CON) | PWR_ON);
		udelay(1);
		spm_write(SPM_CA7_CPU3_PWR_CON, spm_read(SPM_CA7_CPU3_PWR_CON) | PWR_ON_2ND);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while (((spm_read(SPM_PWR_STATUS) & CA7_CPU3) != CA7_CPU3)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA7_CPU3) != CA7_CPU3));
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA7_CPU3_PWR_CON, spm_read(SPM_CA7_CPU3_PWR_CON) & ~PWR_ISO);

		spm_write(SPM_CA7_CPU3_L1_PDN, spm_read(SPM_CA7_CPU3_L1_PDN) & ~L1_PDN);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_CA7_CPU3_L1_PDN) & L1_PDN_ACK) != 0);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */
		udelay(1);
		spm_write(SPM_CA7_CPU3_PWR_CON, spm_read(SPM_CA7_CPU3_PWR_CON) | SRAM_ISOINT_B);
		spm_write(SPM_CA7_CPU3_PWR_CON, spm_read(SPM_CA7_CPU3_PWR_CON) & ~SRAM_CKISO);

		spm_write(SPM_CA7_CPU3_PWR_CON, spm_read(SPM_CA7_CPU3_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_CA7_CPU3_PWR_CON, spm_read(SPM_CA7_CPU3_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);
	}

	return 0;
}

int spm_mtcmos_ctrl_cpu4(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(SPM_SLEEP_TIMER_STA) & CA15_CPU0_STANDBYWFI) == 0);

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA15_CPU0_PWR_CON, spm_read(SPM_CA15_CPU0_PWR_CON) | SRAM_CKISO);
		spm_write(SPM_CA15_L1_PWR_CON, spm_read(SPM_CA15_L1_PWR_CON) | CPU0_CA15_L1_PDN);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_CA15_L1_PWR_CON) & CPU0_CA15_L1_PDN_ACK) !=
		       CPU0_CA15_L1_PDN_ACK);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA15_CPU0_PWR_CON, spm_read(SPM_CA15_CPU0_PWR_CON) | PWR_ISO);

		spm_write(SPM_CA15_CPU0_PWR_CON, spm_read(SPM_CA15_CPU0_PWR_CON) & ~PWR_ON);
		spm_write(SPM_CA15_CPU0_PWR_CON, spm_read(SPM_CA15_CPU0_PWR_CON) & ~PWR_ON_2ND);

		spm_write(SPM_CA15_CPU0_PWR_CON, spm_read(SPM_CA15_CPU0_PWR_CON) & ~PWR_RST_B);

#ifndef CONFIG_FPGA_EARLY_PORTING
		while (((spm_read(SPM_PWR_STATUS) & CA15_CPU0) != 0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA15_CPU0) != 0));
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_mtcmos_cpu_unlock(&flags);

		if (!(spm_read(SPM_PWR_STATUS) & (CA15_CPU1 | CA15_CPU2 | CA15_CPU3)) &&
		    !(spm_read(SPM_PWR_STATUS_2ND) & (CA15_CPU1 | CA15_CPU2 | CA15_CPU3)))
			spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);
	} else {		/* STA_POWER_ON */

		if (!(spm_read(SPM_PWR_STATUS) & CA15_CPUTOP) &&
		    !(spm_read(SPM_PWR_STATUS_2ND) & CA15_CPUTOP))
			spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA15_CPU0_PWR_CON, spm_read(SPM_CA15_CPU0_PWR_CON) | PWR_ON);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_PWR_STATUS) & CA15_CPU0) != CA15_CPU0);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA15_CPU0_PWR_CON, spm_read(SPM_CA15_CPU0_PWR_CON) | PWR_ON_2ND);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_PWR_STATUS_2ND) & CA15_CPU0) != CA15_CPU0);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA15_L1_PWR_CON, spm_read(SPM_CA15_L1_PWR_CON) & ~CPU0_CA15_L1_PDN);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_CA15_L1_PWR_CON) & CPU0_CA15_L1_PDN_ACK) != 0);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA15_CPU0_PWR_CON, spm_read(SPM_CA15_CPU0_PWR_CON) & ~SRAM_CKISO);
		spm_write(SPM_CA15_CPU0_PWR_CON, spm_read(SPM_CA15_CPU0_PWR_CON) & ~PWR_ISO);

		spm_write(SPM_CA15_CPU0_PWR_CON, spm_read(SPM_CA15_CPU0_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);
	}

	return 0;
}

int spm_mtcmos_ctrl_cpu5(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(SPM_SLEEP_TIMER_STA) & CA15_CPU1_STANDBYWFI) == 0);

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA15_CPU1_PWR_CON, spm_read(SPM_CA15_CPU1_PWR_CON) | SRAM_CKISO);
		spm_write(SPM_CA15_L1_PWR_CON, spm_read(SPM_CA15_L1_PWR_CON) | CPU1_CA15_L1_PDN);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_CA15_L1_PWR_CON) & CPU1_CA15_L1_PDN_ACK) !=
		       CPU1_CA15_L1_PDN_ACK);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA15_CPU1_PWR_CON, spm_read(SPM_CA15_CPU1_PWR_CON) | PWR_ISO);

		spm_write(SPM_CA15_CPU1_PWR_CON, spm_read(SPM_CA15_CPU1_PWR_CON) & ~PWR_ON);
		spm_write(SPM_CA15_CPU1_PWR_CON, spm_read(SPM_CA15_CPU1_PWR_CON) & ~PWR_ON_2ND);

		spm_write(SPM_CA15_CPU1_PWR_CON, spm_read(SPM_CA15_CPU1_PWR_CON) & ~PWR_RST_B);

#ifndef CONFIG_FPGA_EARLY_PORTING
		while (((spm_read(SPM_PWR_STATUS) & CA15_CPU1) != 0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA15_CPU1) != 0));
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_mtcmos_cpu_unlock(&flags);

		if (!(spm_read(SPM_PWR_STATUS) & (CA15_CPU0 | CA15_CPU2 | CA15_CPU3)) &&
		    !(spm_read(SPM_PWR_STATUS_2ND) & (CA15_CPU0 | CA15_CPU2 | CA15_CPU3)))
			spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);
	} else {		/* STA_POWER_ON */

		if (!(spm_read(SPM_PWR_STATUS) & CA15_CPUTOP) &&
		    !(spm_read(SPM_PWR_STATUS_2ND) & CA15_CPUTOP))
			spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA15_CPU1_PWR_CON, spm_read(SPM_CA15_CPU1_PWR_CON) | PWR_ON);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_PWR_STATUS) & CA15_CPU1) != CA15_CPU1);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA15_CPU1_PWR_CON, spm_read(SPM_CA15_CPU1_PWR_CON) | PWR_ON_2ND);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_PWR_STATUS_2ND) & CA15_CPU1) != CA15_CPU1);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA15_L1_PWR_CON, spm_read(SPM_CA15_L1_PWR_CON) & ~CPU1_CA15_L1_PDN);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_CA15_L1_PWR_CON) & CPU1_CA15_L1_PDN_ACK) != 0);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA15_CPU1_PWR_CON, spm_read(SPM_CA15_CPU1_PWR_CON) & ~SRAM_CKISO);
		spm_write(SPM_CA15_CPU1_PWR_CON, spm_read(SPM_CA15_CPU1_PWR_CON) & ~PWR_ISO);

		spm_write(SPM_CA15_CPU1_PWR_CON, spm_read(SPM_CA15_CPU1_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);
	}

	return 0;
}

int spm_mtcmos_ctrl_cpu6(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(SPM_SLEEP_TIMER_STA) & CA15_CPU2_STANDBYWFI) == 0);

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA15_CPU2_PWR_CON, spm_read(SPM_CA15_CPU2_PWR_CON) | SRAM_CKISO);
		spm_write(SPM_CA15_L1_PWR_CON, spm_read(SPM_CA15_L1_PWR_CON) | CPU2_CA15_L1_PDN);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_CA15_L1_PWR_CON) & CPU2_CA15_L1_PDN_ACK) !=
		       CPU2_CA15_L1_PDN_ACK);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA15_CPU2_PWR_CON, spm_read(SPM_CA15_CPU2_PWR_CON) | PWR_ISO);

		spm_write(SPM_CA15_CPU2_PWR_CON, spm_read(SPM_CA15_CPU2_PWR_CON) & ~PWR_ON);
		spm_write(SPM_CA15_CPU2_PWR_CON, spm_read(SPM_CA15_CPU2_PWR_CON) & ~PWR_ON_2ND);

		spm_write(SPM_CA15_CPU2_PWR_CON, spm_read(SPM_CA15_CPU2_PWR_CON) & ~PWR_RST_B);

#ifndef CONFIG_FPGA_EARLY_PORTING
		while (((spm_read(SPM_PWR_STATUS) & CA15_CPU2) != 0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA15_CPU2) != 0));
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_mtcmos_cpu_unlock(&flags);

		if (!(spm_read(SPM_PWR_STATUS) & (CA15_CPU2 | CA15_CPU1 | CA15_CPU3)) &&
		    !(spm_read(SPM_PWR_STATUS_2ND) & (CA15_CPU2 | CA15_CPU1 | CA15_CPU3)))
			spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);
	} else {		/* STA_POWER_ON */

		if (!(spm_read(SPM_PWR_STATUS) & CA15_CPUTOP) &&
		    !(spm_read(SPM_PWR_STATUS_2ND) & CA15_CPUTOP))
			spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA15_CPU2_PWR_CON, spm_read(SPM_CA15_CPU2_PWR_CON) | PWR_ON);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_PWR_STATUS) & CA15_CPU2) != CA15_CPU2);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA15_CPU2_PWR_CON, spm_read(SPM_CA15_CPU2_PWR_CON) | PWR_ON_2ND);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_PWR_STATUS_2ND) & CA15_CPU2) != CA15_CPU2);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA15_L1_PWR_CON, spm_read(SPM_CA15_L1_PWR_CON) & ~CPU2_CA15_L1_PDN);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_CA15_L1_PWR_CON) & CPU2_CA15_L1_PDN_ACK) != 0);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA15_CPU2_PWR_CON, spm_read(SPM_CA15_CPU2_PWR_CON) & ~SRAM_CKISO);
		spm_write(SPM_CA15_CPU2_PWR_CON, spm_read(SPM_CA15_CPU2_PWR_CON) & ~PWR_ISO);

		spm_write(SPM_CA15_CPU2_PWR_CON, spm_read(SPM_CA15_CPU2_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);
	}

	return 0;
}

int spm_mtcmos_ctrl_cpu7(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(SPM_SLEEP_TIMER_STA) & CA15_CPU3_STANDBYWFI) == 0);

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA15_CPU3_PWR_CON, spm_read(SPM_CA15_CPU3_PWR_CON) | SRAM_CKISO);
		spm_write(SPM_CA15_L1_PWR_CON, spm_read(SPM_CA15_L1_PWR_CON) | CPU3_CA15_L1_PDN);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_CA15_L1_PWR_CON) & CPU3_CA15_L1_PDN_ACK) !=
		       CPU3_CA15_L1_PDN_ACK);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA15_CPU3_PWR_CON, spm_read(SPM_CA15_CPU3_PWR_CON) | PWR_ISO);

		spm_write(SPM_CA15_CPU3_PWR_CON, spm_read(SPM_CA15_CPU3_PWR_CON) & ~PWR_ON);
		spm_write(SPM_CA15_CPU3_PWR_CON, spm_read(SPM_CA15_CPU3_PWR_CON) & ~PWR_ON_2ND);

		spm_write(SPM_CA15_CPU3_PWR_CON, spm_read(SPM_CA15_CPU3_PWR_CON) & ~PWR_RST_B);

#ifndef CONFIG_FPGA_EARLY_PORTING
		while (((spm_read(SPM_PWR_STATUS) & CA15_CPU3) != 0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA15_CPU3) != 0));
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_mtcmos_cpu_unlock(&flags);

		if (!(spm_read(SPM_PWR_STATUS) & (CA15_CPU3 | CA15_CPU1 | CA15_CPU2)) &&
		    !(spm_read(SPM_PWR_STATUS_2ND) & (CA15_CPU3 | CA15_CPU1 | CA15_CPU2)))
			spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);
	} else {		/* STA_POWER_ON */

		if (!(spm_read(SPM_PWR_STATUS) & CA15_CPUTOP) &&
		    !(spm_read(SPM_PWR_STATUS_2ND) & CA15_CPUTOP))
			spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA15_CPU3_PWR_CON, spm_read(SPM_CA15_CPU3_PWR_CON) | PWR_ON);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_PWR_STATUS) & CA15_CPU3) != CA15_CPU3);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA15_CPU3_PWR_CON, spm_read(SPM_CA15_CPU3_PWR_CON) | PWR_ON_2ND);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_PWR_STATUS_2ND) & CA15_CPU3) != CA15_CPU3);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA15_L1_PWR_CON, spm_read(SPM_CA15_L1_PWR_CON) & ~CPU3_CA15_L1_PDN);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_CA15_L1_PWR_CON) & CPU3_CA15_L1_PDN_ACK) != 0);
#endif				/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA15_CPU3_PWR_CON, spm_read(SPM_CA15_CPU3_PWR_CON) & ~SRAM_CKISO);
		spm_write(SPM_CA15_CPU3_PWR_CON, spm_read(SPM_CA15_CPU3_PWR_CON) & ~PWR_ISO);

		spm_write(SPM_CA15_CPU3_PWR_CON, spm_read(SPM_CA15_CPU3_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);
	}

	return 0;
}

int spm_mtcmos_ctrl_cpusys0(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		/* TODO: add per cpu power status check? */

		if (chkWfiBeforePdn)
			while ((spm_read(SPM_SLEEP_TIMER_STA) & CA7_CPUTOP_STANDBYWFI) == 0);

		/* XXX: no dbg0 mtcmos on k2 */
		/* spm_mtcmos_ctrl_dbg0(state); */

		/* XXX: async adb on mt8173 */
		spm_topaxi_prot_l2(L2_PDN_REQ, 1);
		spm_topaxi_prot(CA7_PDN_REQ, 1);

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA7_CPUTOP_PWR_CON, spm_read(SPM_CA7_CPUTOP_PWR_CON) | PWR_ISO);

		spm_write(SPM_CA7_CPUTOP_PWR_CON, spm_read(SPM_CA7_CPUTOP_PWR_CON) | SRAM_CKISO);
#if 1
		spm_write(SPM_CA7_CPUTOP_PWR_CON,
			  spm_read(SPM_CA7_CPUTOP_PWR_CON) & ~SRAM_ISOINT_B);
		spm_write(SPM_CA7_CPUTOP_L2_PDN, spm_read(SPM_CA7_CPUTOP_L2_PDN) | L2_SRAM_PDN);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_CA7_CPUTOP_L2_PDN) & L2_SRAM_PDN_ACK) != L2_SRAM_PDN_ACK);
#endif	/* #ifndef CONFIG_FPGA_EARLY_PORTING */
		ndelay(1500);
#else
		ndelay(100);
		spm_write(SPM_CA7_CPUTOP_PWR_CON,
			  spm_read(SPM_CA7_CPUTOP_PWR_CON) & ~SRAM_ISOINT_B);
		spm_write(SPM_CA7_CPUTOP_L2_SLEEP,
			  spm_read(SPM_CA7_CPUTOP_L2_SLEEP) & ~L2_SRAM_SLEEP_B);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_CA7_CPUTOP_L2_SLEEP) & L2_SRAM_SLEEP_B_ACK) != 0);
#endif	/* #ifndef CONFIG_FPGA_EARLY_PORTING */
#endif

		spm_write(SPM_CA7_CPUTOP_PWR_CON, spm_read(SPM_CA7_CPUTOP_PWR_CON) & ~PWR_RST_B);
		spm_write(SPM_CA7_CPUTOP_PWR_CON, spm_read(SPM_CA7_CPUTOP_PWR_CON) | PWR_CLK_DIS);

		spm_write(SPM_CA7_CPUTOP_PWR_CON, spm_read(SPM_CA7_CPUTOP_PWR_CON) & ~PWR_ON);
		spm_write(SPM_CA7_CPUTOP_PWR_CON, spm_read(SPM_CA7_CPUTOP_PWR_CON) & ~PWR_ON_2ND);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while (((spm_read(SPM_PWR_STATUS) & CA7_CPUTOP) != 0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA7_CPUTOP) != 0));
#endif	/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_mtcmos_cpu_unlock(&flags);
	} else {		/* STA_POWER_ON */

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA7_CPUTOP_PWR_CON, spm_read(SPM_CA7_CPUTOP_PWR_CON) | PWR_ON);
		udelay(1);
		spm_write(SPM_CA7_CPUTOP_PWR_CON, spm_read(SPM_CA7_CPUTOP_PWR_CON) | PWR_ON_2ND);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while (((spm_read(SPM_PWR_STATUS) & CA7_CPUTOP) != CA7_CPUTOP)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA7_CPUTOP) != CA7_CPUTOP));
#endif	/* #ifndef CONFIG_FPGA_EARLY_PORTING */

		spm_write(SPM_CA7_CPUTOP_PWR_CON, spm_read(SPM_CA7_CPUTOP_PWR_CON) & ~PWR_ISO);

#if 1
		spm_write(SPM_CA7_CPUTOP_L2_PDN, spm_read(SPM_CA7_CPUTOP_L2_PDN) & ~L2_SRAM_PDN);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_CA7_CPUTOP_L2_PDN) & L2_SRAM_PDN_ACK) != 0);
#endif	/* #ifndef CONFIG_FPGA_EARLY_PORTING */
#else
		spm_write(SPM_CA7_CPUTOP_L2_SLEEP,
			  spm_read(SPM_CA7_CPUTOP_L2_SLEEP) | L2_SRAM_SLEEP_B);
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(SPM_CA7_CPUTOP_L2_SLEEP) & L2_SRAM_SLEEP_B_ACK) !=
		       L2_SRAM_SLEEP_B_ACK);
#endif	/* #ifndef CONFIG_FPGA_EARLY_PORTING */
#endif
		ndelay(900);
		spm_write(SPM_CA7_CPUTOP_PWR_CON, spm_read(SPM_CA7_CPUTOP_PWR_CON) | SRAM_ISOINT_B);
		ndelay(100);
		spm_write(SPM_CA7_CPUTOP_PWR_CON, spm_read(SPM_CA7_CPUTOP_PWR_CON) & ~SRAM_CKISO);

		spm_write(SPM_CA7_CPUTOP_PWR_CON, spm_read(SPM_CA7_CPUTOP_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_CA7_CPUTOP_PWR_CON, spm_read(SPM_CA7_CPUTOP_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);

		/* XXX: async adb on mt8173 */
		spm_topaxi_prot_l2(L2_PDN_REQ, 0);
		spm_topaxi_prot(CA7_PDN_REQ, 0);

		/* XXX: no dbg0 mtcmos on k2 */
		/* spm_mtcmos_ctrl_dbg0(state); */
	}

	return 0;
}

int spm_mtcmos_ctrl_cpusys1(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
#if 0
		/* assert ACINCATM before wait for WFIL2 */
		spm_write(CA15L_MISCDBG, spm_read(CA15L_MISCDBG) | 0x1);

		turn_off_SPARK();
#endif
		if (chkWfiBeforePdn)
			while ((spm_read(SPM_SLEEP_TIMER_STA) & CA15_CPUTOP_STANDBYWFI) == 0);

		spm_topaxi_prot(CA15_PDN_REQ, 1);

		spm_mtcmos_cpu_lock(&flags);
#if 0
		turn_off_FBB();
#endif
		spm_write(SPM_CA15_CPUTOP_PWR_CON, spm_read(SPM_CA15_CPUTOP_PWR_CON) | SRAM_CKISO);
		spm_write(SPM_CA15_CPUTOP_PWR_CON, spm_read(SPM_CA15_CPUTOP_PWR_CON) & ~SRAM_ISOINT_B);

#if 1
		spm_write(SPM_CA15_L2_PWR_CON, spm_read(SPM_CA15_L2_PWR_CON) | CA15_L2_PDN);
		while ((spm_read(SPM_CA15_L2_PWR_CON) & CA15_L2_PDN_ACK) != CA15_L2_PDN_ACK);
#else
		spm_write(SPM_CA15_L2_PWR_CON, spm_read(SPM_CA15_L2_PWR_CON) & ~L2_SRAM_SLEEP_B);
		while ((spm_read(SPM_CA15_L2_PWR_CON) & L2_SRAM_SLEEP_B_ACK) != 0);
#endif
		spm_write(SPM_CA15_CPUTOP_PWR_CON, spm_read(SPM_CA15_CPUTOP_PWR_CON) & ~PWR_RST_B);

		spm_write(SPM_CA15_CPUTOP_PWR_CON, spm_read(SPM_CA15_CPUTOP_PWR_CON) | PWR_CLK_DIS);
		spm_write(SPM_CA15_CPUTOP_PWR_CON, spm_read(SPM_CA15_CPUTOP_PWR_CON) | PWR_ISO);

		spm_write(SPM_CA15_CPUTOP_PWR_CON, spm_read(SPM_CA15_CPUTOP_PWR_CON) & ~PWR_ON);
		spm_write(SPM_CA15_CPUTOP_PWR_CON, spm_read(SPM_CA15_CPUTOP_PWR_CON) & ~PWR_ON_2ND);
		while (((spm_read(SPM_PWR_STATUS) & CA15_CPUTOP) != 0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA15_CPUTOP) != 0));

		spm_mtcmos_cpu_unlock(&flags);

		if ((spm_read(SPM_SLEEP_DUAL_VCORE_PWR_CON) & VCA15_PWR_ISO) == 0) {
#if 0
			/* enable dcm for low power */
			if (CHIP_SW_VER_01 == mt_get_chip_sw_ver())
				spm_write(TOPAXI_DCMCTL, spm_read(TOPAXI_DCMCTL) | 0x00000771);
#endif
			spm_write(SPM_SLEEP_DUAL_VCORE_PWR_CON,
				  spm_read(SPM_SLEEP_DUAL_VCORE_PWR_CON) | VCA15_PWR_ISO);
		}
	} else {		/* STA_POWER_ON */

		if ((spm_read(SPM_SLEEP_DUAL_VCORE_PWR_CON) & VCA15_PWR_ISO) == VCA15_PWR_ISO) {
			spm_write(SPM_SLEEP_DUAL_VCORE_PWR_CON,
				  spm_read(SPM_SLEEP_DUAL_VCORE_PWR_CON) & ~VCA15_PWR_ISO);
#if 0
			/* disable dcm for performance */
			if (CHIP_SW_VER_01 == mt_get_chip_sw_ver()) {
				spm_write(TOPAXI_DCMCTL, spm_read(TOPAXI_DCMCTL) | 0x00000001);
				spm_write(TOPAXI_DCMCTL, spm_read(TOPAXI_DCMCTL) & ~0x00000770);
			}
#endif
		}

		/* enable L2 cache hardware reset: Default CA15L_L2RSTDISABLE init is 0 */
		/* spm_write(CA15L_RST_CTL, spm_read(CA15L_RST_CTL) & ~CA15L_L2RSTDISABLE); */

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA15_CPUTOP_PWR_CON, spm_read(SPM_CA15_CPUTOP_PWR_CON) | PWR_ON);
		while ((spm_read(SPM_PWR_STATUS) & CA15_CPUTOP) != CA15_CPUTOP);

		spm_write(SPM_CA15_CPUTOP_PWR_CON, spm_read(SPM_CA15_CPUTOP_PWR_CON) | PWR_ON_2ND);
		while ((spm_read(SPM_PWR_STATUS_2ND) & CA15_CPUTOP) != CA15_CPUTOP);

		spm_write(SPM_CA15_CPUTOP_PWR_CON, spm_read(SPM_CA15_CPUTOP_PWR_CON) & ~PWR_CLK_DIS);

#if 1
		spm_write(SPM_CA15_L2_PWR_CON, spm_read(SPM_CA15_L2_PWR_CON) & ~CA15_L2_PDN);
		while ((spm_read(SPM_CA15_L2_PWR_CON) & CA15_L2_PDN_ACK) != 0);
#else
		spm_write(SPM_CA15_L2_PWR_CON, spm_read(SPM_CA15_L2_PWR_CON) | L2_SRAM_SLEEP_B);
		while ((spm_read(SPM_CA15_L2_PWR_CON) & L2_SRAM_SLEEP_B_ACK) !=
		       L2_SRAM_SLEEP_B_ACK);
#endif

		spm_write(SPM_CA15_CPUTOP_PWR_CON, spm_read(SPM_CA15_CPUTOP_PWR_CON) | SRAM_ISOINT_B);

		spm_write(SPM_CA15_CPUTOP_PWR_CON, spm_read(SPM_CA15_CPUTOP_PWR_CON) & ~SRAM_CKISO);

		spm_write(SPM_CA15_CPUTOP_PWR_CON, spm_read(SPM_CA15_CPUTOP_PWR_CON) & ~PWR_ISO);
#if 0
		ptp2_pre_init();
#endif
		spm_write(SPM_CA15_CPUTOP_PWR_CON, spm_read(SPM_CA15_CPUTOP_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);

		spm_topaxi_prot(CA15_PDN_REQ, 0);
	}

	return 0;

}

#define SLAVE1_MAGIC_REG (SRAMROM_BASE + 0x38)
#define SLAVE2_MAGIC_REG (SRAMROM_BASE + 0x38)
#define SLAVE3_MAGIC_REG (SRAMROM_BASE + 0x38)

#define SLAVE1_MAGIC_NUM 0x534C4131
#define SLAVE2_MAGIC_NUM 0x4C415332
#define SLAVE3_MAGIC_NUM 0x41534C33

#define SLAVE_JUMP_REG  (SRAMROM_BASE + 0x34)

extern void cpu_wake_up_forever_wfi(void);

void spm_mtcmos_ctrl_cpusys_init(CPUSYS_INIT_STAGE stage)
{
    //FIXME: check with minhsien about function address physical or virtual
    spm_write(SLAVE_JUMP_REG, cpu_wake_up_forever_wfi);
    
    switch (stage)
    {
        case E2_STAGE_1:
            /* enable register control */
            spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));
            spm_write(SPM_CA15_CPU3_PWR_CON, CA15_CPU_PWR_CON_DEF_OFF);
            spm_write(SPM_CA15_CPU2_PWR_CON, CA15_CPU_PWR_CON_DEF_OFF);
            spm_write(SPM_CA15_CPU1_PWR_CON, CA15_CPU_PWR_CON_DEF_OFF);
            spm_write(SPM_CA15_CPU0_PWR_CON, CA15_CPU_PWR_CON_DEF_OFF);
            spm_write(SPM_CA15_L1_PWR_CON, CA15_L1_PWR_CON_DEF_OFF);
            //XXX: check with kev -> can we spm_topaxi_prot here? no!
            //spm_topaxi_prot(CA15_PDN_REQ, 1);
            spm_write(SPM_CA15_CPUTOP_PWR_CON, CA15_CPU_PWR_CON_DEF_OFF);
            spm_write(SPM_CA15_L2_PWR_CON, CA15_L2_PWR_CON_DEF_OFF);
            break;
        case E2_STAGE_2:
            spm_write(SLAVE3_MAGIC_REG, SLAVE3_MAGIC_NUM);
            spm_mtcmos_ctrl_cpu3(STA_POWER_DOWN, 1);
            spm_write(SLAVE2_MAGIC_REG, SLAVE2_MAGIC_NUM);
            spm_mtcmos_ctrl_cpu2(STA_POWER_DOWN, 1);
            spm_write(SLAVE1_MAGIC_REG, SLAVE1_MAGIC_NUM);
            spm_mtcmos_ctrl_cpu1(STA_POWER_DOWN, 1);
            //XXX: check with kev, cory, james -> VCA15_PWR_ISO here? Yes!
            spm_write(SPM_SLEEP_DUAL_VCORE_PWR_CON, spm_read(SPM_SLEEP_DUAL_VCORE_PWR_CON) & ~VCA15_PWR_ISO);
            break;
        case E1_LITTLE_STAGE_1:
            /* enable register control */
            spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));
            spm_write(SPM_CA15_CPU0_PWR_CON, spm_read(SPM_CA15_CPU0_PWR_CON) & ~PWR_RST_B);
            spm_write(SPM_CA15_CPU1_PWR_CON, spm_read(SPM_CA15_CPU1_PWR_CON) & ~PWR_RST_B);
            spm_write(SPM_CA15_CPU2_PWR_CON, spm_read(SPM_CA15_CPU2_PWR_CON) & ~PWR_RST_B);
            spm_write(SPM_CA15_CPU3_PWR_CON, spm_read(SPM_CA15_CPU3_PWR_CON) & ~PWR_RST_B);
            ptp2_init();
            turn_on_LO();
            turn_on_FBB();
            spm_mtcmos_ctrl_cpusys1(STA_POWER_ON, 1);
            dump_ptp2_ctrl_regs();
            spm_mtcmos_ctrl_cpu7(STA_POWER_DOWN, 0);
            spm_mtcmos_ctrl_cpu6(STA_POWER_DOWN, 0);
            spm_mtcmos_ctrl_cpu5(STA_POWER_DOWN, 0);
            spm_mtcmos_ctrl_cpu4(STA_POWER_DOWN, 0);
            spm_write(SLAVE3_MAGIC_REG, SLAVE3_MAGIC_NUM);
            spm_mtcmos_ctrl_cpu3(STA_POWER_DOWN, 1);
            spm_write(SLAVE2_MAGIC_REG, SLAVE2_MAGIC_NUM);
            spm_mtcmos_ctrl_cpu2(STA_POWER_DOWN, 1);
            spm_write(SLAVE1_MAGIC_REG, SLAVE1_MAGIC_NUM);
            spm_mtcmos_ctrl_cpu1(STA_POWER_DOWN, 1);
            break;

        case E1_BIG_STAGE_1:
            /* enable register control */
            spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));
            spm_write(SPM_CA15_CPU0_PWR_CON, spm_read(SPM_CA15_CPU0_PWR_CON) & ~PWR_RST_B);
            spm_write(SPM_CA15_CPU1_PWR_CON, spm_read(SPM_CA15_CPU1_PWR_CON) & ~PWR_RST_B);
            spm_write(SPM_CA15_CPU2_PWR_CON, spm_read(SPM_CA15_CPU2_PWR_CON) & ~PWR_RST_B);
            spm_write(SPM_CA15_CPU3_PWR_CON, spm_read(SPM_CA15_CPU3_PWR_CON) & ~PWR_RST_B);
            ptp2_init();
            turn_on_LO();
            turn_on_FBB();
            spm_mtcmos_ctrl_cpusys1(STA_POWER_ON, 1);
            dump_ptp2_ctrl_regs();
            spm_mtcmos_ctrl_cpu7(STA_POWER_DOWN, 0);
            spm_mtcmos_ctrl_cpu6(STA_POWER_DOWN, 0);
            spm_mtcmos_ctrl_cpu5(STA_POWER_DOWN, 0);
            spm_write(SLAVE3_MAGIC_REG, SLAVE3_MAGIC_NUM);
            spm_mtcmos_ctrl_cpu3(STA_POWER_DOWN, 1);
            spm_write(SLAVE2_MAGIC_REG, SLAVE2_MAGIC_NUM);
            spm_mtcmos_ctrl_cpu2(STA_POWER_DOWN, 1);
            spm_write(SLAVE1_MAGIC_REG, SLAVE1_MAGIC_NUM);
            spm_mtcmos_ctrl_cpu1(STA_POWER_DOWN, 1);
            break;

        case E1_BIG_STAGE_2:
            spm_mtcmos_ctrl_cpu4(STA_POWER_ON, 1);
            break;

        case E1_BIG_STAGE_3:
            spm_mtcmos_ctrl_cpu0(STA_POWER_DOWN, 1);
            //becuase preloader run on cpusys0 L2, so we can't turn off cpusys0 in prealoder and postpone to kernel
            //spm_mtcmos_ctrl_cpusys0(STA_POWER_DOWN, 1);
            break;

        default:
            break;
    } 
}

bool spm_cpusys0_can_power_down(void)
{
	return !(spm_read(SPM_PWR_STATUS) &
		 (CA15_CPU0 | CA15_CPU1 | CA15_CPU2 | CA15_CPU3 | CA15_CPUTOP | CA7_CPU1 | CA7_CPU2
		  | CA7_CPU3))
	    && !(spm_read(SPM_PWR_STATUS_2ND) &
		 (CA15_CPU0 | CA15_CPU1 | CA15_CPU2 | CA15_CPU3 | CA15_CPUTOP | CA7_CPU1 | CA7_CPU2
		  | CA7_CPU3));
}

bool spm_cpusys1_can_power_down(void)
{
	return !(spm_read(SPM_PWR_STATUS) &
		 (CA7_CPU0 | CA7_CPU1 | CA7_CPU2 | CA7_CPU3 | CA7_CPUTOP | CA15_CPU1 | CA15_CPU2 |
		  CA15_CPU3))
	    && !(spm_read(SPM_PWR_STATUS_2ND) &
		 (CA7_CPU0 | CA7_CPU1 | CA7_CPU2 | CA7_CPU3 | CA7_CPUTOP | CA15_CPU1 | CA15_CPU2 |
		  CA15_CPU3));
}



/**************************************
 * for non-CPU MTCMOS
 **************************************/
int spm_topaxi_prot(int bit, int en)
{
	unsigned long flags;
	spm_mtcmos_noncpu_lock(flags);

	if (en == 1) {
		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) | (1 << bit));
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(TOPAXI_PROT_STA1) & (1 << bit)) != (1 << bit)) {
		}
#endif
	} else {
		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) & ~(1 << bit));
#ifndef CONFIG_FPGA_EARLY_PORTING
		while (spm_read(TOPAXI_PROT_STA1) & (1 << bit)) {
		}
#endif
	}

	spm_mtcmos_noncpu_unlock(flags);

	return 0;
}

int spm_topaxi_prot_l2(int bit, int en)
{
	unsigned long flags;
	spm_mtcmos_noncpu_lock(flags);

	if (en == 1) {
		spm_write(TOPAXI_PROT_EN1, spm_read(TOPAXI_PROT_EN1) | (1 << bit));
#ifndef CONFIG_FPGA_EARLY_PORTING
		while ((spm_read(TOPAXI_PROT_STA3) & (1 << bit)) != (1 << bit)) {
		}
#endif
	} else {
		spm_write(TOPAXI_PROT_EN1, spm_read(TOPAXI_PROT_EN1) & ~(1 << bit));
#ifndef CONFIG_FPGA_EARLY_PORTING
		while (spm_read(TOPAXI_PROT_STA3) & (1 << bit)) {
		}
#endif
	}

	spm_mtcmos_noncpu_unlock(flags);

	return 0;
}

