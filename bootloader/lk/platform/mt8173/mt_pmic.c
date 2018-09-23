#include <debug.h>
#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_pmic.h>
#include <platform/mt_gpt.h>
#include <platform/mt_pmic_wrap_init.h>
#include <printf.h>
#include <platform/upmu_hw.h>
#include <platform/upmu_common.h>

//==============================================================================
// Global variable
//==============================================================================
int Enable_PMIC_LOG = 1;

CHARGER_TYPE g_ret = CHARGER_UNKNOWN;
int g_charger_in_flag = 0;
int g_first_check=0;
int g_pmic_cid=0;

extern int g_R_BAT_SENSE;
extern int g_R_I_SENSE;
extern int g_R_CHARGER_1;
extern int g_R_CHARGER_2;

//==============================================================================
// PMIC access API
//==============================================================================
U32 pmic_read_interface (U32 RegNum, U32 *val, U32 MASK, U32 SHIFT)
{
	U32 return_value = 0;
	U32 pmic_reg = 0;
	U32 rdata;

	//mt6323_read_byte(RegNum, &pmic_reg);
	return_value= pwrap_wacs2(0, (RegNum), 0, &rdata);
	pmic_reg=rdata;
	if (return_value!=0) {
		dprintf(CRITICAL, "[pmic_read_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		return return_value;
	}
	dprintf(INFO, "[pmic_read_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);

	pmic_reg &= (MASK << SHIFT);
	*val = (pmic_reg >> SHIFT);
	dprintf(INFO, "[pmic_read_interface] val=0x%x\n", *val);

	return return_value;
}

U32 pmic_config_interface (U32 RegNum, U32 val, U32 MASK, U32 SHIFT)
{
	U32 return_value = 0;
	U32 pmic_reg = 0;
	U32 rdata;

	//1. mt6323_read_byte(RegNum, &pmic_reg);
	return_value= pwrap_wacs2(0, (RegNum), 0, &rdata);
	pmic_reg=rdata;
	if (return_value!=0) {
		dprintf(CRITICAL, "[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		return return_value;
	}
	dprintf(INFO, "[pmic_config_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);

	pmic_reg &= ~(MASK << SHIFT);
	pmic_reg |= (val << SHIFT);

	//2. mt6323_write_byte(RegNum, pmic_reg);
	return_value= pwrap_wacs2(1, (RegNum), pmic_reg, &rdata);
	if (return_value!=0) {
		dprintf(CRITICAL, "[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		return return_value;
	}
	dprintf(INFO, "[pmic_config_interface] write Reg[%x]=0x%x\n", RegNum, pmic_reg);

#if 0
	//3. Double Check
	//mt6323_read_byte(RegNum, &pmic_reg);
	return_value= pwrap_wacs2(0, (RegNum), 0, &rdata);
	pmic_reg=rdata;
	if (return_value!=0) {
		dprintf(CRITICAL, "[pmic_config_interface] Reg[%x]= pmic_wrap write data fail\n", RegNum);
		return return_value;
	}
	dprintf(INFO, "[pmic_config_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);
#endif

	return return_value;
}

//==============================================================================
// PMIC APIs
//==============================================================================


//==============================================================================
// PMIC6397 Usage APIs
//==============================================================================
U32 pmic_IsUsbCableIn (void)
{
	U32 ret=0;
	U32 val=0;

	ret=pmic_read_interface( (U32)(CHR_CON0),
	                         (&val),
	                         (U32)(PMIC_RGS_CHRDET_MASK),
	                         (U32)(PMIC_RGS_CHRDET_SHIFT)
	                       );
	if (Enable_PMIC_LOG>1)
		dprintf(CRITICAL, "%d", ret);

	return val;
}

kal_bool upmu_is_chr_det(void)
{
	U32 tmp32;
	tmp32=upmu_get_rgs_chrdet();
	if (tmp32 == 0) {
		//printk("[upmu_is_chr_det] No charger\n");
		return KAL_FALSE;
	} else {
		//printk("[upmu_is_chr_det] Charger exist\n");
		return KAL_TRUE;
	}
}

kal_bool pmic_chrdet_status(void)
{
	if ( upmu_is_chr_det() == KAL_TRUE ) {
#ifndef USER_BUILD
		dprintf(INFO, "[pmic_chrdet_status] Charger exist\r\n");
#endif

		return KAL_TRUE;
	} else {
#ifndef USER_BUILD
		dprintf(INFO, "[pmic_chrdet_status] No charger\r\n");
#endif

		return KAL_FALSE;
	}
}

int pmic_detect_powerkey(void)
{
	U32 ret=0;
	U32 val=0;

	ret=pmic_read_interface( (U32)(CHRSTATUS),
	                         (&val),
	                         (U32)(PMIC_PWRKEY_DEB_MASK),
	                         (U32)(PMIC_PWRKEY_DEB_SHIFT)
	                       );

	if (Enable_PMIC_LOG>1)
		dprintf(CRITICAL, "%d", ret);

	if (val==1) {
		dprintf(CRITICAL, "[pmic_detect_powerkey] Release\n");
		return 0;
	} else {
		dprintf(CRITICAL, "[pmic_detect_powerkey] Press\n");
		return 1;
	}
}

int pmic_detect_homekey(void)
{
	U32 ret=0;
	U32 val=0;

	ret=pmic_read_interface( (U32)(OCSTATUS2),
	                         (&val),
	                         (U32)(PMIC_HOMEKEY_DEB_MASK),
	                         (U32)(PMIC_HOMEKEY_DEB_SHIFT)
	                       );

	if (Enable_PMIC_LOG>1)
		dprintf(CRITICAL, "%d", ret);

	return val;
}

kal_uint32 upmu_get_reg_value(kal_uint32 reg)
{
	U32 ret=0;
	U32 temp_val=0;

	ret=pmic_read_interface(reg, &temp_val, 0xFFFF, 0x0);

	if (Enable_PMIC_LOG>1)
		dprintf(CRITICAL, "%d", ret);

	return temp_val;
}

void PMIC_DUMP_ALL_Register(void)
{
	U32 i=0;
	U32 ret=0;
	U32 reg_val=0;

	for (i=0; i<0xFFFF; i++) {
		ret=pmic_read_interface(i,&reg_val,0xFFFF,0);
		dprintf(CRITICAL, "Reg[0x%x]=0x%x, %d\n", i, reg_val, ret);
	}
}

//==============================================================================
// PMIC6397 Init Code
//==============================================================================
void PMIC_INIT_SETTING_V1(void)
{
	U32 chip_version = 0;

	/* put init setting from DE/SA */
	chip_version = upmu_get_cid();
	switch (chip_version) {
		/* [7:4]: RG_VCDT_HV_VTH; 7V OVP */
		case PMIC6391_E1_CID_CODE:
		case PMIC6391_E2_CID_CODE:
		case PMIC6391_E3_CID_CODE:
			pmic_config_interface(0x2, 0xC, 0xF, 4);
			break;
		case PMIC6397_E2_CID_CODE:
		case PMIC6397_E3_CID_CODE:
		case PMIC6397_E4_CID_CODE:
			pmic_config_interface(0x2, 0xB, 0xF, 4);
			break;
		default:
			dprintf(CRITICAL, "[Power/PMIC] Error chip ID %d\r\n", chip_version);
			pmic_config_interface(0x2, 0xB, 0xF, 4);
			break;
	}

	pmic_config_interface(0x0, 0x0, 0x1, 0);	/* [0:0]: RG_VCDT_HV_EN; Disable HV. Only compare LV threshold. */
	pmic_config_interface(0xC,0x1,0x7,1); // [3:1]: RG_VBAT_OV_VTH; VBAT_OV=4.3V
	pmic_config_interface(0x1A,0x3,0xF,0); // [3:0]: RG_CHRWDT_TD; align to 6250's
	pmic_config_interface(0x24,0x1,0x1,1); // [1:1]: RG_BC11_RST;
	pmic_config_interface(0x2A,0x0,0x7,4); // [6:4]: RG_CSDAC_STP; align to 6250's setting
	pmic_config_interface(0x2E,0x1,0x1,7); // [7:7]: RG_ULC_DET_EN;
	pmic_config_interface(0x2E,0x1,0x1,6); // [6:6]: RG_HWCV_EN;
	pmic_config_interface(0x2E,0x1,0x1,2); // [2:2]: RG_CSDAC_MODE;
	pmic_config_interface(0x102,0x0,0x1,3); // [3:3]: RG_PWMOC_CK_PDN; For OC protection
	pmic_config_interface(0x128,0x1,0x1,9); // [9:9]: RG_SRCVOLT_HW_AUTO_EN;
	pmic_config_interface(0x128,0x1,0x1,8); // [8:8]: RG_OSC_SEL_AUTO;
	pmic_config_interface(0x128,0x1,0x1,6); // [6:6]: RG_SMPS_DIV2_SRC_AUTOFF_DIS;
	pmic_config_interface(0x128,0x1,0x1,5); // [5:5]: RG_SMPS_AUTOFF_DIS;
	pmic_config_interface(0x130,0x1,0x1,7); // [7:7]: VDRM_DEG_EN;
	pmic_config_interface(0x130,0x1,0x1,6); // [6:6]: VSRMCA7_DEG_EN;
	pmic_config_interface(0x130,0x1,0x1,5); // [5:5]: VPCA7_DEG_EN;
	pmic_config_interface(0x130,0x1,0x1,4); // [4:4]: VIO18_DEG_EN;
	pmic_config_interface(0x130,0x1,0x1,3); // [3:3]: VGPU_DEG_EN; For OC protection
	pmic_config_interface(0x130,0x1,0x1,2); // [2:2]: VCORE_DEG_EN;
	pmic_config_interface(0x130,0x1,0x1,1); // [1:1]: VSRMCA15_DEG_EN;
	pmic_config_interface(0x130,0x1,0x1,0); // [0:0]: VCA15_DEG_EN;
	pmic_config_interface(0x178,0x1,0x1,11); // [11:11]: RG_INT_EN_THR_H;
	pmic_config_interface(0x178,0x1,0x1,10); // [10:10]: RG_INT_EN_THR_L;
	pmic_config_interface(0x178,0x1,0x1,4); // [4:4]: RG_INT_EN_BAT_L;
	pmic_config_interface(0x17E,0x1,0x1,11); // [11:11]: RG_INT_EN_VGPU; OC protection
	pmic_config_interface(0x17E,0x1,0x1,8); // [8:8]: RG_INT_EN_VCA15; OC protection
	pmic_config_interface(0x206,0x600,0x0FFF,0); // [12:0]: BUCK_RSV; for OC protection
	pmic_config_interface(0x210,0x1,0x3,10); // [11:10]: QI_VCORE_VSLEEP; sleep mode only (0.7V)
	pmic_config_interface(0x210,0x0,0x3,6); // [7:6]: QI_VSRMCA7_VSLEEP; sleep mode only (0.85V)
	pmic_config_interface(0x210,0x1,0x3,4); // [5:4]: QI_VSRMCA15_VSLEEP; sleep mode only (0.7V)
	pmic_config_interface(0x210,0x0,0x3,2); // [3:2]: QI_VPCA7_VSLEEP; sleep mode only (0.85V)
	pmic_config_interface(0x210,0x1,0x3,0); // [1:0]: QI_VCA15_VSLEEP; sleep mode only (0.7V)
	pmic_config_interface(0x216,0x0,0x3,12); // [13:12]: RG_VCA15_CSL2; for OC protection
	pmic_config_interface(0x216,0x0,0x3,10); // [11:10]: RG_VCA15_CSL1; for OC protection
	pmic_config_interface(0x224,0x1,0x1,15); // [15:15]: VCA15_SFCHG_REN; soft change rising enable
	pmic_config_interface(0x224,0x5,0x7F,8); // [14:8]: VCA15_SFCHG_RRATE; soft change rising step=0.5us
	pmic_config_interface(0x224,0x1,0x1,7); // [7:7]: VCA15_SFCHG_FEN; soft change falling enable
	pmic_config_interface(0x224,0x17,0x7F,0); // [6:0]: VCA15_SFCHG_FRATE; soft change falling step=2us
	pmic_config_interface(0x22A,0x0,0x7F,0); // [6:0]: VCA15_VOSEL_SLEEP; sleep mode only (0.7V)
	pmic_config_interface(0x238,0x1,0x1,8); // [8:8]: VCA15_VSLEEP_EN; set sleep mode reference voltage from R2R to V2V
	pmic_config_interface(0x238,0x3,0x3,4); // [5:4]: VCA15_VOSEL_TRANS_EN; rising & falling enable
	pmic_config_interface(0x244,0x1,0x1,5); // [5:5]: VSRMCA15_TRACK_SLEEP_CTRL;
	pmic_config_interface(0x246,0x0,0x3,4); // [5:4]: VSRMCA15_VOSEL_SEL;
	pmic_config_interface(0x24A,0x1,0x1,15); // [15:15]: VSRMCA15_SFCHG_REN;
	pmic_config_interface(0x24A,0x5,0x7F,8); // [14:8]: VSRMCA15_SFCHG_RRATE;
	pmic_config_interface(0x24A,0x1,0x1,7); // [7:7]: VSRMCA15_SFCHG_FEN;
	pmic_config_interface(0x24A,0x17,0x7F,0); // [6:0]: VSRMCA15_SFCHG_FRATE;
	pmic_config_interface(0x250,0x00,0x7F,0); // [6:0]: VSRMCA15_VOSEL_SLEEP; Sleep mode setting only (0.7V)
	pmic_config_interface(0x25E,0x1,0x1,8); // [8:8]: VSRMCA15_VSLEEP_EN; set sleep mode reference voltage from R2R to V2V
	pmic_config_interface(0x25E,0x3,0x3,4); // [5:4]: VSRMCA15_VOSEL_TRANS_EN; rising & falling enable
	pmic_config_interface(0x270,0x1,0x1,1); // [1:1]: VCORE_VOSEL_CTRL; sleep mode voltage control follow SRCLKEN
	pmic_config_interface(0x272,0x0,0x3,4); // [5:4]: VCORE_VOSEL_SEL;
	pmic_config_interface(0x276,0x1,0x1,15); // [15:15]: VCORE_SFCHG_REN;
	pmic_config_interface(0x276,0x5,0x7F,8); // [14:8]: VCORE_SFCHG_RRATE;
	pmic_config_interface(0x276,0x17,0x7F,0); // [6:0]: VCORE_SFCHG_FRATE;
	pmic_config_interface(0x27C,0x0,0x7F,0); // [6:0]: VCORE_VOSEL_SLEEP; Sleep mode setting only (0.7V)
	pmic_config_interface(0x28A,0x1,0x1,8); // [8:8]: VCORE_VSLEEP_EN; Sleep mode HW control  R2R to VtoV
	pmic_config_interface(0x28A,0x0,0x3,4); // [5:4]: VCORE_VOSEL_TRANS_EN; Follows MT6320 VCORE setting.
	pmic_config_interface(0x28A,0x3,0x3,0); // [1:0]: VCORE_TRANSTD;
	pmic_config_interface(0x28E,0x1,0x3,8); // [9:8]: RG_VGPU_CSL; for OC protection
	pmic_config_interface(0x29C,0x1,0x1,15); // [15:15]: VGPU_SFCHG_REN;
	pmic_config_interface(0x29C,0x5,0x7F,8); // [14:8]: VGPU_SFCHG_RRATE;
	pmic_config_interface(0x29C,0x17,0x7F,0); // [6:0]: VGPU_SFCHG_FRATE;
	pmic_config_interface(0x2B0,0x0,0x3,4); // [5:4]: VGPU_VOSEL_TRANS_EN;
	pmic_config_interface(0x2B0,0x3,0x3,0); // [1:0]: VGPU_TRANSTD;
	pmic_config_interface(0x332,0x0,0x3,4); // [5:4]: VPCA7_VOSEL_SEL;
	pmic_config_interface(0x336,0x1,0x1,15); // [15:15]: VPCA7_SFCHG_REN;
	pmic_config_interface(0x336,0x5,0x7F,8); // [14:8]: VPCA7_SFCHG_RRATE;
	pmic_config_interface(0x336,0x1,0x1,7); // [7:7]: VPCA7_SFCHG_FEN;
	pmic_config_interface(0x336,0x17,0x7F,0); // [6:0]: VPCA7_SFCHG_FRATE;
	pmic_config_interface(0x33C,0x18,0x7F,0); // [6:0]: VPCA7_VOSEL_SLEEP;
	pmic_config_interface(0x34A,0x1,0x1,8); // [8:8]: VPCA7_VSLEEP_EN;
	pmic_config_interface(0x34A,0x3,0x3,4); // [5:4]: VPCA7_VOSEL_TRANS_EN;
	pmic_config_interface(0x356,0x0,0x1,5); // [5:5]: VSRMCA7_TRACK_SLEEP_CTRL;
	pmic_config_interface(0x358,0x0,0x3,4); // [5:4]: VSRMCA7_VOSEL_SEL;
	pmic_config_interface(0x35C,0x1,0x1,15); // [15:15]: VSRMCA7_SFCHG_REN;
	pmic_config_interface(0x35C,0x5,0x7F,8); // [14:8]: VSRMCA7_SFCHG_RRATE;
	pmic_config_interface(0x35C,0x1,0x1,7); // [7:7]: VSRMCA7_SFCHG_FEN;
	pmic_config_interface(0x35C,0x17,0x7F,0); // [6:0]: VSRMCA7_SFCHG_FRATE;
	pmic_config_interface(0x362,0x18,0x7F,0); // [6:0]: VSRMCA7_VOSEL_SLEEP;
	pmic_config_interface(0x370,0x1,0x1,8); // [8:8]: VSRMCA7_VSLEEP_EN;
	pmic_config_interface(0x370,0x3,0x3,4); // [5:4]: VSRMCA7_VOSEL_TRANS_EN;
	pmic_config_interface(0x39C,0x1,0x1,8); // [8:8]: VDRM_VSLEEP_EN;
	pmic_config_interface(0x440,0x1,0x1,2); // [2:2]: VIBR_THER_SHEN_EN;
	pmic_config_interface(0x500,0x1,0x1,5); // [5:5]: THR_HWPDN_EN;
	pmic_config_interface(0x502,0x1,0x1,3); // [3:3]: RG_RST_DRVSEL;
	pmic_config_interface(0x502,0x1,0x1,2); // [2:2]: RG_EN_DRVSEL;
	pmic_config_interface(0x508,0x1,0x1,1); // [1:1]: PWRBB_DEB_EN;
	pmic_config_interface(0x50C,0x1,0x1,12); // [12:12]: VSRMCA15_PG_H2L_EN;
	pmic_config_interface(0x50C,0x1,0x1,11); // [11:11]: VPCA15_PG_H2L_EN;
	pmic_config_interface(0x50C,0x1,0x1,10); // [10:10]: VCORE_PG_H2L_EN;
	pmic_config_interface(0x50C,0x1,0x1,9); // [9:9]: VSRMCA7_PG_H2L_EN;
	pmic_config_interface(0x50C,0x1,0x1,8); // [8:8]: VPCA7_PG_H2L_EN;
	pmic_config_interface(0x512,0x1,0x1,1); // [1:1]: STRUP_PWROFF_PREOFF_EN;
	pmic_config_interface(0x512,0x1,0x1,0); // [0:0]: STRUP_PWROFF_SEQ_EN;
	pmic_config_interface(0x55E,0xFC,0xFF,8); // [15:8]: RG_ADC_TRIM_CH_SEL;
	pmic_config_interface(0x560,0x1,0x1,1); // [1:1]: FLASH_THER_SHDN_EN;
	pmic_config_interface(0x566,0x1,0x1,1); // [1:1]: KPLED_THER_SHDN_EN;
	pmic_config_interface(0x600,0x1,0x1,9); // [9:9]: SPK_THER_SHDN_L_EN;
	pmic_config_interface(0x604,0x0,0x1,0); // [0:0]: RG_SPK_INTG_RST_L;
	pmic_config_interface(0x606,0x1,0x1,9); // [9:9]: SPK_THER_SHDN_R_EN;
	pmic_config_interface(0x60A,0x1,0xF,11); // [14:11]: RG_SPKPGA_GAINR;
	pmic_config_interface(0x612,0x1,0xF,8); // [11:8]: RG_SPKPGA_GAINL;
	pmic_config_interface(0x632,0x1,0x1,8); // [8:8]: FG_SLP_EN;
	pmic_config_interface(0x638,0xFFC2,0xFFFF,0); // [15:0]: FG_SLP_CUR_TH;
	pmic_config_interface(0x63A,0x14,0xFF,0); // [7:0]: FG_SLP_TIME;
	pmic_config_interface(0x63C,0xFF,0xFF,8); // [15:8]: FG_DET_TIME;
	pmic_config_interface(0x714,0x1,0x1,7); // [7:7]: RG_LCLDO_ENC_REMOTE_SENSE_VA28;
	pmic_config_interface(0x714,0x1,0x1,4); // [4:4]: RG_LCLDO_REMOTE_SENSE_VA33;
	pmic_config_interface(0x714,0x1,0x1,1); // [1:1]: RG_HCLDO_REMOTE_SENSE_VA33;
	pmic_config_interface(0x71A,0x1,0x1,15); // [15:15]: RG_NCP_REMOTE_SENSE_VA18;
	pmic_config_interface(0x260,0x10,0x7F,8); // [14:8]: VSRMCA15_VOSEL_OFFSET; set offset=100mV
	pmic_config_interface(0x260,0x0,0x7F,0); // [6:0]: VSRMCA15_VOSEL_DELTA; set delta=0mV
	pmic_config_interface(0x262,0x48,0x7F,8); // [14:8]: VSRMCA15_VOSEL_ON_HB; set HB=1.15V
	pmic_config_interface(0x262,0x25,0x7F,0); // [6:0]: VSRMCA15_VOSEL_ON_LB; set LB=0.93125V
	pmic_config_interface(0x264,0x0,0x7F,0); // [6:0]: VSRMCA15_VOSEL_SLEEP_LB; set sleep LB=0.7V
	pmic_config_interface(0x372,0x4,0x7F,8); // [14:8]: VSRMCA7_VOSEL_OFFSET; set offset=25mV
	pmic_config_interface(0x372,0x0,0x7F,0); // [6:0]: VSRMCA7_VOSEL_DELTA; set delta=0mV
	pmic_config_interface(0x374,0x48,0x7F,8); // [14:8]: VSRMCA7_VOSEL_ON_HB; set HB=1.15V
	pmic_config_interface(0x374,0x25,0x7F,0); // [6:0]: VSRMCA7_VOSEL_ON_LB; set LB=0.93125V
	pmic_config_interface(0x376,0x18,0x7F,0); // [6:0]: VSRMCA7_VOSEL_SLEEP_LB; set sleep LB=0.85000V
	pmic_config_interface(0x21E,0x3,0x3,0); // [1:1]: VCA15_VOSEL_CTRL, VCA15_EN_CTRL; DVS HW control by SRCLKEN
	pmic_config_interface(0x244,0x3,0x3,0); // [1:1]: VSRMCA15_VOSEL_CTRL, VSRAM15_EN_CTRL;
	pmic_config_interface(0x330,0x0,0x1,1); // [1:1]: VPCA7_VOSEL_CTRL;
	pmic_config_interface(0x356,0x0,0x1,1); // [1:1]: VSRMCA7_VOSEL_CTRL;
	pmic_config_interface(0x21E,0x1,0x1,4); // [4:4]: VCA15_TRACK_ON_CTRL; DVFS tracking enable
	pmic_config_interface(0x244,0x1,0x1,4); // [4:4]: VSRMCA15_TRACK_ON_CTRL;
	pmic_config_interface(0x330,0x0,0x1,4); // [4:4]: VPCA7_TRACK_ON_CTRL;
	pmic_config_interface(0x356,0x0,0x1,4); // [4:4]: VSRMCA7_TRACK_ON_CTRL;
	pmic_config_interface(0x134,0x3,0x3,14); // [15:14]: VGPU OC;
	pmic_config_interface(0x134,0x3,0x3,2); // [3:2]: VCA15 OC;
}

void PMIC_CUSTOM_SETTING_V1(void)
{
	dprintf(INFO, "[LK_PMIC_INIT_SETTING_V1] \n");
}

U32 pmic_init (void)
{
	U32 ret_code = PMIC_TEST_PASS;

	dprintf(CRITICAL, "[LK_pmic6397_init] Start...................\n");

	g_pmic_cid=upmu_get_cid();
	dprintf(CRITICAL, "[LK_PMIC_INIT] PMIC CID=0x%x\n", g_pmic_cid);

	//upmu_set_rg_chrind_on(0);
	//dprintf(CRITICAL, "[LK_PMIC_INIT] Turn Off chrind\n");

	PMIC_INIT_SETTING_V1();
	dprintf(CRITICAL, "[LK_PMIC_INIT_SETTING_V1] Done\n");

	PMIC_CUSTOM_SETTING_V1();
	dprintf(CRITICAL, "[LK_PMIC_CUSTOM_SETTING_V1] Done\n");

	//PMIC_DUMP_ALL_Register();

	return ret_code;
}

//==============================================================================
// PMIC6397 API for LK : AUXADC
//==============================================================================
int PMIC_IMM_GetOneChannelValue(int dwChannel, int deCount, int trimd)
{
	kal_int32 u4Sample_times = 0;
	kal_int32 u4channel[8] = {0,0,0,0,0,0,0,0};
	kal_int32 adc_result=0;
	kal_int32 adc_result_temp=0;
	kal_int32 r_val_temp=0;
	kal_int32 count=0;
	kal_int32 count_time_out=1000;
	kal_int32 ret_data=0;

	if (dwChannel==1) {
		pmic_config_interface(0x0020, 0x0801, 0xFFFF, 0);
		upmu_set_rg_source_ch0_norm_sel(1);
		upmu_set_rg_source_ch0_lbat_sel(1);
		dwChannel=0;
	}

	/*
	    0 : V_BAT
	    1 : V_I_Sense
	    2 : V_Charger
	    3 : V_TBAT
	    4~7 : reserved
	*/
	upmu_set_rg_auxadc_chsel(dwChannel);

	//upmu_set_rg_avg_num(0x3);

	if (dwChannel==3) {
		upmu_set_rg_buf_pwd_on(1);
		upmu_set_rg_buf_pwd_b(1);
		upmu_set_baton_tdet_en(1);
		mdelay(20);
	}

	if (dwChannel==4) {
		upmu_set_rg_vbuf_en(1);
		upmu_set_rg_vbuf_byp(0);
		upmu_set_rg_vbuf_calen(1);
	}

	u4Sample_times=0;

	do {
		upmu_set_rg_auxadc_start(0);
		upmu_set_rg_auxadc_start(1);

		//Duo to HW limitation
		mdelay(10);

		count=0;
		ret_data=0;

		switch (dwChannel) {
			case 0:
				while ( upmu_get_rg_adc_rdy_c0() != 1 ) {
					if ( (count++) > count_time_out) {
						dprintf(CRITICAL, "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
						break;
					}
				}
				if (trimd == 1) {
					ret_data = upmu_get_rg_adc_out_c0_trim();
				} else {
					ret_data = upmu_get_rg_adc_out_c0();
				}
				break;
			case 1:
				while ( upmu_get_rg_adc_rdy_c1() != 1 ) {
					if ( (count++) > count_time_out) {
						dprintf(CRITICAL, "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
						break;
					}
				}
				if (trimd == 1) {
					ret_data = upmu_get_rg_adc_out_c1_trim();
				} else {
					ret_data = upmu_get_rg_adc_out_c1();
				}
				break;
			case 2:
				while ( upmu_get_rg_adc_rdy_c2() != 1 ) {
					if ( (count++) > count_time_out) {
						dprintf(CRITICAL, "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
						break;
					}
				}
				if (trimd == 1) {
					ret_data = upmu_get_rg_adc_out_c2_trim();
				} else {
					ret_data = upmu_get_rg_adc_out_c2();
				}
				break;
			case 3:
				while ( upmu_get_rg_adc_rdy_c3() != 1 ) {
					if ( (count++) > count_time_out) {
						dprintf(CRITICAL, "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
						break;
					}
				}
				if (trimd == 1) {
					ret_data = upmu_get_rg_adc_out_c3_trim();
				} else {
					ret_data = upmu_get_rg_adc_out_c3();
				}
				break;
			case 4:
				while ( upmu_get_rg_adc_rdy_c4() != 1 ) {
					if ( (count++) > count_time_out) {
						dprintf(CRITICAL, "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
						break;
					}
				}
				if (trimd == 1) {
					ret_data = upmu_get_rg_adc_out_c4_trim();
				} else {
					ret_data = upmu_get_rg_adc_out_c4();
				}
				break;
			case 5:
				while ( upmu_get_rg_adc_rdy_c5() != 1 ) {
					if ( (count++) > count_time_out) {
						dprintf(CRITICAL, "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
						break;
					}
				}
				if (trimd == 1) {
					ret_data = upmu_get_rg_adc_out_c5_trim();
				} else {
					ret_data = upmu_get_rg_adc_out_c5();
				}
				break;
			case 6:
				while ( upmu_get_rg_adc_rdy_c6() != 1 ) {
					if ( (count++) > count_time_out) {
						dprintf(CRITICAL, "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
						break;
					}
				}
				if (trimd == 1) {
					ret_data = upmu_get_rg_adc_out_c6_trim();
				} else {
					ret_data = upmu_get_rg_adc_out_c6();
				}
				break;
			case 7:
				while ( upmu_get_rg_adc_rdy_c7() != 1 ) {
					if ( (count++) > count_time_out) {
						dprintf(CRITICAL, "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
						break;
					}
				}
				if (trimd == 1) {
					ret_data = upmu_get_rg_adc_out_c7_trim();
				} else {
					ret_data = upmu_get_rg_adc_out_c7();
				}
				break;
			default:
				dprintf(CRITICAL, "[AUXADC] Invalid channel value(%d,%d)\n", dwChannel, trimd);
				return -1;
				break;
		}

		u4channel[dwChannel] += ret_data;

		u4Sample_times++;

		//debug
		dprintf(INFO, "[AUXADC] u4Sample_times=%d, ret_data=%d, u4channel[%d]=%d\n",
		        u4Sample_times, ret_data, dwChannel, u4channel[dwChannel]);

	} while (u4Sample_times < deCount);

	/* Value averaging  */
	u4channel[dwChannel] = u4channel[dwChannel] / deCount;
	adc_result_temp = u4channel[dwChannel];

	switch (dwChannel) {
		case 0:
			r_val_temp = g_R_BAT_SENSE;
			adc_result = (adc_result_temp*r_val_temp*VOLTAGE_FULL_RANGE)/ADC_PRECISE;
			break;
		case 1:
			r_val_temp = g_R_I_SENSE;
			adc_result = (adc_result_temp*r_val_temp*VOLTAGE_FULL_RANGE)/ADC_PRECISE;
			break;
		case 2:
			r_val_temp = (((g_R_CHARGER_1+g_R_CHARGER_2)*100)/g_R_CHARGER_2);
			adc_result = (adc_result_temp*r_val_temp*VOLTAGE_FULL_RANGE)/ADC_PRECISE;
			break;
		case 3:
			r_val_temp = 1;
			adc_result = (adc_result_temp*r_val_temp*VOLTAGE_FULL_RANGE)/ADC_PRECISE;
			break;
		case 4:
			r_val_temp = 1;
			adc_result = (adc_result_temp*r_val_temp*VOLTAGE_FULL_RANGE)/ADC_PRECISE;
			break;
		case 5:
			r_val_temp = 1;
			adc_result = (adc_result_temp*r_val_temp*VOLTAGE_FULL_RANGE)/ADC_PRECISE;
			break;
		case 6:
			r_val_temp = 1;
			adc_result = (adc_result_temp*r_val_temp*VOLTAGE_FULL_RANGE)/ADC_PRECISE;
			break;
		case 7:
			r_val_temp = 1;
			adc_result = (adc_result_temp*r_val_temp*VOLTAGE_FULL_RANGE)/ADC_PRECISE;
			break;
		default:
			dprintf(CRITICAL, "[AUXADC] Invalid channel value(%d,%d)\n", dwChannel, trimd);
			return -1;
			break;
	}

	//debug
	dprintf(CRITICAL, "[AUXADC] adc_result_temp=%d, adc_result=%d, r_val_temp=%d\n",
	        adc_result_temp, adc_result, r_val_temp);

	count=0;

	if (dwChannel==0) {
		upmu_set_rg_source_ch0_norm_sel(0);
		upmu_set_rg_source_ch0_lbat_sel(0);
	}

	if (dwChannel==3) {
		upmu_set_baton_tdet_en(0);
		upmu_set_rg_buf_pwd_b(0);
		upmu_set_rg_buf_pwd_on(0);
	}

	if (dwChannel==4) {
		//upmu_set_rg_vbuf_en(0);
		//upmu_set_rg_vbuf_byp(0);
		upmu_set_rg_vbuf_calen(0);
	}

	return adc_result;

}

int get_bat_sense_volt(int times)
{
	return PMIC_IMM_GetOneChannelValue(0,times,1);
}

int get_i_sense_volt(int times)
{
	return PMIC_IMM_GetOneChannelValue(1,times,1);
}

int get_charger_volt(int times)
{
	return PMIC_IMM_GetOneChannelValue(2,times,1);
}

int get_tbat_volt(int times)
{
	return PMIC_IMM_GetOneChannelValue(3,times,1);
}

CHARGER_TYPE mt_charger_type_detection(void)
{
	if ( g_first_check == 0 ) {
		g_first_check = 1;
		g_ret = hw_charger_type_detection();
	} else {
		dprintf(CRITICAL, "[mt_charger_type_detection] Got data !!, %d, %d\r\n", g_charger_in_flag, g_first_check);
	}

	return g_ret;
}

