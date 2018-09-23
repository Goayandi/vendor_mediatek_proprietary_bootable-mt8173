#include <platform/mt_boot_sound.h>
#include <platform/project.h>
#include <platform/sync_write.h>
#include <kernel/thread.h>
#include <kernel/event.h>
#include <reg.h>
#include <printf.h>
#include <string.h>
#include <malloc.h>
#include <target/mt_boot_sound_custom.h>

#define RENDER_BUF_PADDING_SIZE 7680
#define LONG_SLEEP_TIME 10
#define SHORT_SLEEP_TIME 1

// SPM releated
#define SPM_PROJECT_CODE     0xb16
#define AUD_PWR_STA_MASK    (0x1 << 24)
#define AUD_SRAM_ACK        (0xf << 12)

static volatile bool boot_sound_init = false;
static volatile bool boot_sound_output = false;
static event_t render_done_event;

static unsigned int mt_aud_get_reg(volatile unsigned int *address)
{
	return readl(address);
}

static void mt_aud_set_reg(volatile unsigned int *address, unsigned int value, unsigned int mask)
{
	unsigned int val = mt_aud_get_reg(address);
	val &= (~mask);
	val |= (value & mask);
	mt_reg_sync_writel(val, address);
}

static void mt_aud_print_reg_info()
{
	dprintf(ALWAYS, "AUDIO_TOP_CON0 = 0x%x\n", mt_aud_get_reg(AUDIO_TOP_CON0));
	dprintf(ALWAYS, "AUDIO_TOP_CON1 = 0x%x\n", mt_aud_get_reg(AUDIO_TOP_CON1));
	dprintf(ALWAYS, "AUDIO_TOP_CON2 = 0x%x\n", mt_aud_get_reg(AUDIO_TOP_CON2));
	dprintf(ALWAYS, "AUDIO_TOP_CON3 = 0x%x\n", mt_aud_get_reg(AUDIO_TOP_CON3));

	dprintf(ALWAYS, "AFE_TOP_CON0 = 0x%x\n", mt_aud_get_reg(AFE_TOP_CON0));
	dprintf(ALWAYS, "AFE_DAC_CON0 = 0x%x\n", mt_aud_get_reg(AFE_DAC_CON0));
	dprintf(ALWAYS, "AFE_DAC_CON1 = 0x%x\n", mt_aud_get_reg(AFE_DAC_CON1));
	dprintf(ALWAYS, "AFE_I2S_CON = 0x%x\n", mt_aud_get_reg(AFE_I2S_CON));
	dprintf(ALWAYS, "AFE_I2S_CON1 = 0x%x\n", mt_aud_get_reg(AFE_I2S_CON1));
	dprintf(ALWAYS, "AFE_I2S_CON2 = 0x%x\n", mt_aud_get_reg(AFE_I2S_CON2));
	dprintf(ALWAYS, "AFE_I2S_CON3 = 0x%x\n", mt_aud_get_reg(AFE_I2S_CON3));

	dprintf(ALWAYS, "AFE_CONN0 = 0x%x\n", mt_aud_get_reg(AFE_CONN0));
	dprintf(ALWAYS, "AFE_CONN1 = 0x%x\n", mt_aud_get_reg(AFE_CONN1));
	dprintf(ALWAYS, "AFE_CONN2 = 0x%x\n", mt_aud_get_reg(AFE_CONN2));
	dprintf(ALWAYS, "AFE_CONN3 = 0x%x\n", mt_aud_get_reg(AFE_CONN3));
	dprintf(ALWAYS, "AFE_CONN4 = 0x%x\n", mt_aud_get_reg(AFE_CONN4));
	dprintf(ALWAYS, "AFE_CONN5 = 0x%x\n", mt_aud_get_reg(AFE_CONN5));
	dprintf(ALWAYS, "AFE_CONN6 = 0x%x\n", mt_aud_get_reg(AFE_CONN6));
	dprintf(ALWAYS, "AFE_CONN7 = 0x%x\n", mt_aud_get_reg(AFE_CONN7));
	dprintf(ALWAYS, "AFE_CONN8 = 0x%x\n", mt_aud_get_reg(AFE_CONN8));
	dprintf(ALWAYS, "AFE_CONN9 = 0x%x\n", mt_aud_get_reg(AFE_CONN9));

	dprintf(ALWAYS, "AFE_CONN_24BIT = 0x%x\n", mt_aud_get_reg(AFE_CONN_24BIT));
	dprintf(ALWAYS, "AFE_DAIBT_CON0 = 0x%x\n", mt_aud_get_reg(AFE_DAIBT_CON0));
	dprintf(ALWAYS, "AFE_MRGIF_CON = 0x%x\n", mt_aud_get_reg(AFE_MRGIF_CON));
	dprintf(ALWAYS, "AFE_MRGIF_MON0 = 0x%x\n", mt_aud_get_reg(AFE_MRGIF_MON0));
	dprintf(ALWAYS, "AFE_MRGIF_MON1 = 0x%x\n", mt_aud_get_reg(AFE_MRGIF_MON1));
	dprintf(ALWAYS, "AFE_MRGIF_MON2 = 0x%x\n", mt_aud_get_reg(AFE_MRGIF_MON2));

	dprintf(ALWAYS, "AFE_DL1_BASE = 0x%x\n", mt_aud_get_reg(AFE_DL1_BASE));
	dprintf(ALWAYS, "AFE_DL1_CUR = 0x%x\n", mt_aud_get_reg(AFE_DL1_CUR));
	dprintf(ALWAYS, "AFE_DL1_END = 0x%x\n", mt_aud_get_reg(AFE_DL1_END));
	dprintf(ALWAYS, "AFE_DL2_BASE = 0x%x\n", mt_aud_get_reg(AFE_DL2_BASE));
	dprintf(ALWAYS, "AFE_DL2_CUR = 0x%x\n", mt_aud_get_reg(AFE_DL2_CUR));
	dprintf(ALWAYS, "AFE_DL2_END = 0x%x\n", mt_aud_get_reg(AFE_DL2_END));
	dprintf(ALWAYS, "AFE_AWB_BASE = 0x%x\n", mt_aud_get_reg(AFE_AWB_BASE));
	dprintf(ALWAYS, "AFE_AWB_END = 0x%x\n", mt_aud_get_reg(AFE_AWB_END));
	dprintf(ALWAYS, "AFE_AWB_CUR = 0x%x\n", mt_aud_get_reg(AFE_AWB_CUR));
	dprintf(ALWAYS, "AFE_VUL_BASE = 0x%x\n", mt_aud_get_reg(AFE_VUL_BASE));
	dprintf(ALWAYS, "AFE_VUL_END = 0x%x\n", mt_aud_get_reg(AFE_VUL_END));
	dprintf(ALWAYS, "AFE_VUL_CUR = 0x%x\n", mt_aud_get_reg(AFE_VUL_CUR));
	dprintf(ALWAYS, "AFE_DAI_BASE = 0x%x\n", mt_aud_get_reg(AFE_DAI_BASE));
	dprintf(ALWAYS, "AFE_DAI_END = 0x%x\n", mt_aud_get_reg(AFE_DAI_END));
	dprintf(ALWAYS, "AFE_DAI_CUR = 0x%x\n", mt_aud_get_reg(AFE_DAI_CUR));

	dprintf(ALWAYS, "AFE_DL1_D2_BASE = 0x%x\n", mt_aud_get_reg(AFE_DL1_D2_BASE));
	dprintf(ALWAYS, "AFE_DL1_D2_END = 0x%x\n", mt_aud_get_reg(AFE_DL1_D2_END));
	dprintf(ALWAYS, "AFE_DL1_D2_CUR = 0x%x\n", mt_aud_get_reg(AFE_DL1_D2_CUR));
	dprintf(ALWAYS, "AFE_VUL_D2_BASE = 0x%x\n", mt_aud_get_reg(AFE_VUL_D2_BASE));
	dprintf(ALWAYS, "AFE_VUL_D2_END = 0x%x\n", mt_aud_get_reg(AFE_VUL_D2_END));
	dprintf(ALWAYS, "AFE_VUL_D2_CUR = 0x%x\n", mt_aud_get_reg(AFE_VUL_D2_CUR));

	dprintf(ALWAYS, "AFE_MEMIF_MSB = 0x%x\n", mt_aud_get_reg(AFE_MEMIF_MSB));
	dprintf(ALWAYS, "AFE_MEMIF_MON0 = 0x%x\n", mt_aud_get_reg(AFE_MEMIF_MON0));
	dprintf(ALWAYS, "AFE_MEMIF_MON1 = 0x%x\n", mt_aud_get_reg(AFE_MEMIF_MON1));
	dprintf(ALWAYS, "AFE_MEMIF_MON2 = 0x%x\n", mt_aud_get_reg(AFE_MEMIF_MON2));
	dprintf(ALWAYS, "AFE_MEMIF_MON4 = 0x%x\n", mt_aud_get_reg(AFE_MEMIF_MON4));

	dprintf(ALWAYS, "AFE_ADDA_DL_SRC2_CON0 = 0x%x\n", mt_aud_get_reg(AFE_ADDA_DL_SRC2_CON0));
	dprintf(ALWAYS, "AFE_ADDA_DL_SRC2_CON1 = 0x%x\n", mt_aud_get_reg(AFE_ADDA_DL_SRC2_CON1));
	dprintf(ALWAYS, "AFE_ADDA_UL_SRC_CON0 = 0x%x\n", mt_aud_get_reg(AFE_ADDA_UL_SRC_CON0));
	dprintf(ALWAYS, "AFE_ADDA_UL_SRC_CON1 = 0x%x\n", mt_aud_get_reg(AFE_ADDA_UL_SRC_CON1));

	dprintf(ALWAYS, "AFE_ADDA_TOP_CON0 = 0x%x\n", mt_aud_get_reg(AFE_ADDA_TOP_CON0));
	dprintf(ALWAYS, "AFE_ADDA_UL_DL_CON0 = 0x%x\n", mt_aud_get_reg(AFE_ADDA_UL_DL_CON0));
	dprintf(ALWAYS, "AFE_ADDA_SRC_DEBUG = 0x%x\n", mt_aud_get_reg(AFE_ADDA_SRC_DEBUG));
	dprintf(ALWAYS, "AFE_ADDA_SRC_DEBUG_MON0 = 0x%x\n", mt_aud_get_reg(AFE_ADDA_SRC_DEBUG_MON0));
	dprintf(ALWAYS, "AFE_ADDA_SRC_DEBUG_MON1 = 0x%x\n", mt_aud_get_reg(AFE_ADDA_SRC_DEBUG_MON1));
	dprintf(ALWAYS, "AFE_ADDA_NEWIF_CFG0 = 0x%x\n", mt_aud_get_reg(AFE_ADDA_NEWIF_CFG0));
	dprintf(ALWAYS, "AFE_ADDA_NEWIF_CFG1 = 0x%x\n", mt_aud_get_reg(AFE_ADDA_NEWIF_CFG1));
	dprintf(ALWAYS, "AFE_ADDA2_TOP_CON0 = 0x%x\n", mt_aud_get_reg(AFE_ADDA2_TOP_CON0));
	dprintf(ALWAYS, "AFE_ADDA_PREDIS_CON0 = 0x%x\n", mt_aud_get_reg(AFE_ADDA_PREDIS_CON0));
	dprintf(ALWAYS, "AFE_ADDA_PREDIS_CON1 = 0x%x\n", mt_aud_get_reg(AFE_ADDA_PREDIS_CON1));

	dprintf(ALWAYS, "AFE_IRQ_MCU_CON = 0x%x\n", mt_aud_get_reg(AFE_IRQ_MCU_CON));
	dprintf(ALWAYS, "AFE_IRQ_MCU_STATUS = 0x%x\n", mt_aud_get_reg(AFE_IRQ_MCU_STATUS));
	dprintf(ALWAYS, "AFE_IRQ_MCU_CLR = 0x%x\n", mt_aud_get_reg(AFE_IRQ_MCU_CLR));
	dprintf(ALWAYS, "AFE_IRQ_MCU_CNT1 = 0x%x\n", mt_aud_get_reg(AFE_IRQ_MCU_CNT1));
	dprintf(ALWAYS, "AFE_IRQ_MCU_CNT2 = 0x%x\n", mt_aud_get_reg(AFE_IRQ_MCU_CNT2));
	dprintf(ALWAYS, "AFE_IRQ_MCU_EN = 0x%x\n", mt_aud_get_reg(AFE_IRQ_MCU_MON));
	dprintf(ALWAYS, "AFE_IRQ_MCU_MON2 = 0x%x\n", mt_aud_get_reg(AFE_IRQ_MCU_MON2));
	dprintf(ALWAYS, "AFE_IRQ_MCU_CNT5 = 0x%x\n", mt_aud_get_reg(AFE_IRQ_MCU_CNT5));
	dprintf(ALWAYS, "AFE_IRQ1_MCU_CNT_MON = 0x%x\n", mt_aud_get_reg(AFE_IRQ1_MCU_CNT_MON));
	dprintf(ALWAYS, "AFE_IRQ2_MCU_CNT_MON = 0x%x\n", mt_aud_get_reg(AFE_IRQ2_MCU_CNT_MON));
	dprintf(ALWAYS, "AFE_IRQ1_MCU_EN_CNT_MON = 0x%x\n", mt_aud_get_reg(AFE_IRQ_MCU_CNT_MON));
	dprintf(ALWAYS, "AFE_IRQ5_MCU_CNT_MON = 0x%x\n", mt_aud_get_reg(AFE_IRQ5_MCU_CNT_MON));
	dprintf(ALWAYS, "AFE_MEMIF_MAXLEN = 0x%x\n", mt_aud_get_reg(AFE_MEMIF_MAXLEN));
	dprintf(ALWAYS, "AFE_MEMIF_PBUF_SIZE = 0x%x\n", mt_aud_get_reg(AFE_MEMIF_PBUF_SIZE));

	dprintf(ALWAYS, "CLK_CFG_4 = 0x%x\n", mt_aud_get_reg(CLK_CFG_4));
	dprintf(ALWAYS, "INFRA_PDN_STA = 0x%x\n", mt_aud_get_reg(INFRA_PDN_STA));
	dprintf(ALWAYS, "DCM_CFG = 0x%x\n", mt_aud_get_reg(DCM_CFG));
}

static void mt_aud_turn_on_mtcmos()
{
	mt_aud_set_reg(POWRON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (1U << 0), 0xffffffff);

	mt_aud_set_reg(SLEEP_AUDIO_PWR_CON, 1 << 2, 1 << 2);
	mt_aud_set_reg(SLEEP_AUDIO_PWR_CON, 1 << 3, 1 << 3);

	while (!(mt_aud_get_reg(SLEEP_PWR_STA) & AUD_PWR_STA_MASK)
	        || !(mt_aud_get_reg(SLEEP_PWR_STAS) & AUD_PWR_STA_MASK)) {
	}

	mt_aud_set_reg(SLEEP_AUDIO_PWR_CON, 0x0, 1 << 4);
	mt_aud_set_reg(SLEEP_AUDIO_PWR_CON, 0x0, 1 << 1);
	mt_aud_set_reg(SLEEP_AUDIO_PWR_CON, 0x1, 0x1);
	mt_aud_set_reg(SLEEP_AUDIO_PWR_CON, 0x0, 0xf << 8);

	while ((mt_aud_get_reg(SLEEP_AUDIO_PWR_CON) & AUD_SRAM_ACK)) {
	}
}

static void mt_aud_turn_off_mtcmos()
{
	mt_aud_set_reg(POWRON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (1U << 0), 0xffffffff);

	mt_aud_set_reg(SLEEP_AUDIO_PWR_CON, 0xf << 8, 0xf << 8);

	while ((mt_aud_get_reg(SLEEP_AUDIO_PWR_CON) & AUD_SRAM_ACK) != AUD_SRAM_ACK) {
	}

	mt_aud_set_reg(SLEEP_AUDIO_PWR_CON, 1 << 1, 1 << 1);

	mt_aud_set_reg(SLEEP_AUDIO_PWR_CON, 0x10, 0x11);

	mt_aud_set_reg(SLEEP_AUDIO_PWR_CON, 0x0, 0xc);

	while ((mt_aud_get_reg(SLEEP_PWR_STA) & AUD_PWR_STA_MASK)
	        || (mt_aud_get_reg(SLEEP_PWR_STAS) & AUD_PWR_STA_MASK)) {
	}
}

static unsigned int mt_aud_rate_to_index(unsigned int rate)
{
	switch (rate) {
		case 8000:
			return 0;
		case 11025:
			return 1;
		case 12000:
			return 2;
		case 16000:
			return 4;
		case 22050:
			return 5;
		case 24000:
			return 6;
		case 32000:
			return 8;
		case 44100:
			return 9;
		case 48000:
			return 10;
		case 88000:
			return 11;
		case 96000:
			return 12;
		case 174000:
			return 13;
		case 192000:
			return 14;
		default:
			return 9;
			break;
	}
}

static void mt_aud_set_dl_src2(unsigned int sample_rate)
{
	unsigned int afe_adda_dl_src2_con0, afe_adda_dl_src2_con1;

	if (sample_rate == 44100)
		afe_adda_dl_src2_con0 = 7;
	else if (sample_rate == 8000)
		afe_adda_dl_src2_con0 = 0;
	else if (sample_rate == 11025)
		afe_adda_dl_src2_con0 = 1;
	else if (sample_rate == 12000)
		afe_adda_dl_src2_con0 = 2;
	else if (sample_rate == 16000)
		afe_adda_dl_src2_con0 = 3;
	else if (sample_rate == 22050)
		afe_adda_dl_src2_con0 = 4;
	else if (sample_rate == 24000)
		afe_adda_dl_src2_con0 = 5;
	else if (sample_rate == 32000)
		afe_adda_dl_src2_con0 = 6;
	else if (sample_rate == 48000)
		afe_adda_dl_src2_con0 = 8;
	else
		afe_adda_dl_src2_con0 = 7;


	if (afe_adda_dl_src2_con0 == 0 || afe_adda_dl_src2_con0 == 3) { /* 8k or 16k voice mode */
		afe_adda_dl_src2_con0 =
		    (afe_adda_dl_src2_con0 << 28) | (0x03 << 24) | (0x03 << 11) | (0x01 << 5);
	} else {
		afe_adda_dl_src2_con0 = (afe_adda_dl_src2_con0 << 28) | (0x03 << 24) | (0x03 << 11);
	}

	afe_adda_dl_src2_con0 = afe_adda_dl_src2_con0 | (0x01 << 1);
	afe_adda_dl_src2_con1 = 0xf74f0000;

	mt_aud_set_reg(AFE_ADDA_DL_SRC2_CON0, afe_adda_dl_src2_con0, 0xffffffff);
	mt_aud_set_reg(AFE_ADDA_DL_SRC2_CON1, afe_adda_dl_src2_con1, 0xffffffff);
}


static void mt_aud_turn_on_afe_hw(unsigned int start_addr, unsigned int end_addr, unsigned int rate)
{
	mt_aud_turn_on_mtcmos();

	mt_aud_set_reg(AUDIO_TOP_CON0, 1 << 14, 1 << 14); /* APB bus init */

	mt_aud_set_reg(AUDIO_TOP_CON0, 0x0, 1 << 2); /* pdn_afe on */
	mt_aud_set_reg(AUDIO_TOP_CON0, 0x0, 1 << 25); /* pdn_dac on */
	mt_aud_set_reg(AUDIO_TOP_CON0, 0x0, 1 << 26); /* pdn_dac_predis on */

	mt_aud_set_reg(AFE_CONN_24BIT, 0x0, 0x18); /* O_3/O_4 output 16bit */

	mt_aud_set_reg(AFE_CONN1, 1 << 21, 1 << 21); /* I_05 -> O_03 connect*/
	mt_aud_set_reg(AFE_CONN2, 1 << 6, 1 << 6); /* I_06 -> O_04 connect */

	mt_aud_set_reg(AFE_DL1_BASE, start_addr, 0xffffffff);
	mt_aud_set_reg(AFE_DL1_END, end_addr, 0xffffffff);

	mt_aud_set_reg(AFE_ADDA_PREDIS_CON0, 0, 0xffffffff);
	mt_aud_set_reg(AFE_ADDA_PREDIS_CON1, 0, 0xffffffff);

	mt_aud_set_dl_src2(rate);

	/* dac i2s setting */
	mt_aud_set_reg(AFE_I2S_CON1, (mt_aud_rate_to_index(rate) << 8) | 0x8, 0xfffffffe);
	mt_aud_set_reg(AFE_ADDA_DL_SRC2_CON0, 0x1, 0x1);
	mt_aud_set_reg(AFE_I2S_CON1, 0x1, 0x1);
	mt_aud_set_reg(AFE_ADDA_UL_DL_CON0, 0x1, 0x1);

#ifdef MT_BOOT_SOUND_CUSTOM_I2SO_OUTPUT
	mt_aud_set_reg(AFE_CONN_24BIT, 0x0, 0x3); /* O_0/O_1 output 16bit */
	mt_aud_set_reg(AFE_CONN0, 1 << 5, 1 << 5); /* I_05 -> O_00 connect */
	mt_aud_set_reg(AFE_CONN0, 1 << 22, 1 << 22); /* I_06 -> O_01 connect */
	/* i2s0 soft reset begin */
	mt_aud_set_reg(AUDIO_TOP_CON1, 0x2, 0x2);
	mt_aud_set_reg(AFE_DAC_CON1, mt_aud_rate_to_index(rate) << 8, 0x00000f00);
	mt_aud_set_reg(AFE_I2S_CON, 0x90000008, 0xfffffffe);
	mt_aud_set_reg(AFE_I2S_CON, 0x1, 0x1);
	mt_aud_set_reg(AFE_I2S_CON3, (mt_aud_rate_to_index(rate) << 8) | 0x8, 0xfffffffe);
	mt_aud_set_reg(AFE_I2S_CON3, 0x1, 0x1);
	/* i2s0 soft reset end */
	udelay(1);
	mt_aud_set_reg(AUDIO_TOP_CON1, 0x0, 0x2);
#endif

	/* dl1 memif setting */
	mt_aud_set_reg(AFE_MEMIF_PBUF_SIZE, 0x0, 0x00030000); /* 16bit format */
	mt_aud_set_reg(AFE_DAC_CON1, mt_aud_rate_to_index(rate), 0x0000000f);
	mt_aud_set_reg(AFE_DAC_CON1, 0, 1 << 21);

	mt_aud_set_reg(AFE_DAC_CON0, 0x1, 0x1); /* afe on */
}

static void mt_aud_turn_off_afe_hw()
{
	mt_aud_set_reg(AFE_DAC_CON0, 0x0, 0x2); /* dl1 off */
	mt_aud_set_reg(AFE_CONN1, 0x0, 1 << 21); /* I_05 -> O_03 disconnect*/
	mt_aud_set_reg(AFE_CONN2, 0x0, 1 << 6); /* I_06 -> O_04 disconnect */
	mt_aud_set_reg(AFE_ADDA_DL_SRC2_CON0, 0x0, 0x1);
	mt_aud_set_reg(AFE_I2S_CON1, 0x0, 0x1); /* dac i2s off */
	mt_aud_set_reg(AFE_ADDA_UL_DL_CON0, 0x0, 0x1);
#ifdef MT_BOOT_SOUND_CUSTOM_I2SO_OUTPUT
	mt_aud_set_reg(AFE_I2S_CON3, 0x0, 0x1);
	mt_aud_set_reg(AFE_I2S_CON, 0x0, 0x1);
	mt_aud_set_reg(AFE_CONN0, 0 << 5, 1 << 5); /* I_05 -> O_00 disconnect */
	mt_aud_set_reg(AFE_CONN0, 0 << 22, 1 << 22); /* I_06 -> O_01 disconnect */
#endif
	mt_aud_set_reg(AFE_DAC_CON0, 0x0, 0x1); /* afe off */
	mt_aud_set_reg(AUDIO_TOP_CON0, 1 << 26, 1 << 26); /* pdn_dac_predis off */
	mt_aud_set_reg(AUDIO_TOP_CON0, 1 << 25, 1 << 25); /* pdn_dac off */
	mt_aud_set_reg(AUDIO_TOP_CON0, 1 << 2, 1 << 2); /* pdn_afe off */

	mt_aud_turn_off_mtcmos();
}

static int boot_sound_render(void *arg)
{
	unsigned char *buffer;
	unsigned int sample_rate = get_boot_sound_sample_rate();
	unsigned int data_size = get_boot_sound_data_size();
	unsigned int buffer_size = data_size + RENDER_BUF_PADDING_SIZE;
	unsigned int long_sleep_data_bytes = LONG_SLEEP_TIME * sample_rate / 250;
	unsigned int current_sleep_time = 0;
	unsigned int max_sleep_time = (buffer_size * 250 / sample_rate) * 2;
	unsigned int start_addr;
	unsigned int hw_pos;
	unsigned int data_end;

	buffer = malloc(buffer_size);
	if (!buffer) {
		dprintf(CRITICAL, "[boot_sound_render] allocate buffer 0x%x fail\n", buffer_size);
		return -1;
	}

	boot_sound_output = true;

	start_addr = (unsigned int)buffer;
	start_addr = ((start_addr + 15) >> 4) << 4;
	data_end = start_addr + data_size;

	memset(buffer, 0, buffer_size);
	memcpy(start_addr, mt_boot_sound_data, data_size);
	arch_clean_invalidate_cache_range((addr_t)buffer, buffer_size);

	mt_aud_turn_on_afe_hw(start_addr, ((unsigned int)buffer + buffer_size - 1), sample_rate);
	mt_boot_sound_turn_on_sink_output(sample_rate);

	mt_aud_set_reg(AFE_DAC_CON0, 0x2, 0x2); /* dl1 on */

	//mt_aud_print_reg_info();

	for (;;) {
		hw_pos = mt_aud_get_reg(AFE_DL1_CUR);

		//dprintf(ALWAYS, "[boot_sound_render] hw_pos = 0x%x data_end = 0x%x diff = %d MEMIF_MON0:0x%x\n",
		//  hw_pos, data_end, data_end - hw_pos, mt_aud_get_reg(AFE_MEMIF_MON0));

		if (hw_pos > data_end) {
			dprintf(ALWAYS, "[boot_sound_render] render done hw_pos:0x%x data_end:0x%x\n",
			        hw_pos, data_end);
			break;
		} else if (current_sleep_time > max_sleep_time) {
			dprintf(CRITICAL, "[boot_sound_render] sleep timeout %u > %u\n",
			        current_sleep_time, max_sleep_time);
			dprintf(CRITICAL, "[boot_sound_render] sleep timeout 0x%x/0x%x\n",
			        hw_pos, data_end);
			break;
		}

		if ((data_end - hw_pos) > long_sleep_data_bytes) {
			thread_sleep(LONG_SLEEP_TIME);
			current_sleep_time += LONG_SLEEP_TIME;
		} else {
			thread_sleep(SHORT_SLEEP_TIME);
			current_sleep_time += SHORT_SLEEP_TIME;
		}
	}

	mt_boot_sound_turn_off_sink_output();
	mt_aud_turn_off_afe_hw();

	if (buffer) free(buffer);

	event_signal(&render_done_event, false);
	return 0;
}

int mt_boot_sound_init(void)
{
	thread_t *thr;

	dprintf(ALWAYS, "[mt_boot_sound_init] start\n");

	if (boot_sound_init) {
		dprintf(CRITICAL, "[mt_boot_sound_init] output sound already\n");
		return -1;
	}

	boot_sound_init = true;
	boot_sound_output = false;

	event_init(&render_done_event, false, EVENT_FLAG_AUTOUNSIGNAL);

	thr = thread_create("boot_sound_render", &boot_sound_render, NULL, HIGH_PRIORITY, 32768);
	if (!thr) {
		dprintf(CRITICAL, "[mt_boot_sound_init] fail to create boot_sound_render thread\n");
		return -1;
	}

	thread_resume(thr);

	return 0;
}

int mt_boot_sound_deinit(void)
{
	if (boot_sound_init) {
		dprintf(ALWAYS, "[mt_boot_sound_deinit] start\n");
		if (boot_sound_output)
			event_wait(&render_done_event);

		event_destroy(&render_done_event);
		boot_sound_init = false;
		dprintf(ALWAYS, "[mt_boot_sound_deinit] end\n");
	}
	return 0;
}
