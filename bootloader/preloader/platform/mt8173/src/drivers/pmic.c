
#include <typedefs.h>
#include <platform.h>
#include <pmic_wrap_init.h>
#include <pmic.h>
#include <i2c.h>
#include "timer.h"

#ifdef MTK_BQ24297_SUPPORT
#include <bq24297.h>
#endif

//#define PMIC_DEBUG
#ifdef PMIC_DEBUG
#define PMIC_PRINT   print
#else
#define PMIC_PRINT
#endif

//flag to indicate ca15 related power is ready
volatile int g_ca15_ready __attribute__ ((section (".data"))) = 0;

//////////////////////////////////////////////////////////////////////////////////////////
// PMIC access API
//////////////////////////////////////////////////////////////////////////////////////////
U32 pmic_read_interface (U32 RegNum, U32 *val, U32 MASK, U32 SHIFT)
{
    U32 return_value = 0;    
    U32 pmic_reg = 0;
    U32 rdata;    

    return_value= pwrap_wacs2(0, (RegNum), 0, &rdata);
    pmic_reg=rdata;
    if(return_value!=0)
    {   
        PMIC_PRINT("[pmic_read_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
        return return_value;
    }
    PMIC_PRINT("[pmic_read_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);
    
    pmic_reg &= (MASK << SHIFT);
    *val = (pmic_reg >> SHIFT);    
    PMIC_PRINT("[pmic_read_interface] val=0x%x\n", *val);

    return return_value;
}

U32 pmic_config_interface (U32 RegNum, U32 val, U32 MASK, U32 SHIFT)
{
    U32 return_value = 0;    
    U32 pmic_reg = 0;
    U32 rdata;

    //1. mt_read_byte(RegNum, &pmic_reg);
    return_value= pwrap_wacs2(0, (RegNum), 0, &rdata);
    pmic_reg=rdata;    
    if(return_value!=0)
    {   
        PMIC_PRINT("[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
        return return_value;
    }
    PMIC_PRINT("[pmic_config_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);
    
    pmic_reg &= ~(MASK << SHIFT);
    pmic_reg |= (val << SHIFT);

    //2. mt_write_byte(RegNum, pmic_reg);
    return_value= pwrap_wacs2(1, (RegNum), pmic_reg, &rdata);
    if(return_value!=0)
    {   
        PMIC_PRINT("[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
        return return_value;
    }
    PMIC_PRINT("[pmic_config_interface] write Reg[%x]=0x%x\n", RegNum, pmic_reg);    

#if 0
    //3. Double Check    
    //mt_read_byte(RegNum, &pmic_reg);
    return_value= pwrap_wacs2(0, (RegNum), 0, &rdata);
    pmic_reg=rdata;    
    if(return_value!=0)
    {   
        print("[pmic_config_interface] Reg[%x]= pmic_wrap write data fail\n", RegNum);
        return return_value;
    }
    print("[pmic_config_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);
#endif    

    return return_value;
}

#if 0
U32 upmu_get_reg_value(U32 reg)
{
    U32 ret=0;
    U32 reg_val=0;

    //printf("[upmu_get_reg_value] \n");
    ret=pmic_read_interface(reg, &reg_val, 0xFFFF, 0x0);
    
    return reg_val;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// PMIC-Charger Type Detection
//////////////////////////////////////////////////////////////////////////////////////////
CHARGER_TYPE g_ret = CHARGER_UNKNOWN;
int g_charger_in_flag = 0;
int g_first_check=0;

CHARGER_TYPE mt_charger_type_detection(void)
{
    if( g_first_check == 0 )
    {
        g_first_check = 1;
#ifdef MTK_BQ24297_SUPPORT
        if (check_bq24297_exist())
            g_ret = bq24297_charger_type_detection();
        else
            g_ret = hw_charger_type_detection();
#elif defined(MTK_BQ25890_SUPPORT)
        g_ret = hw_charger_type_detection();//bq25890_charger_type_detection();//wisky-lxh@20170324,fix bq25890 lk i2c error
#else
        g_ret = hw_charger_type_detection();
#endif

    }
    else
    {
        print("[mt_charger_type_detection] Got data !!, %d, %d\r\n", g_charger_in_flag, g_first_check);
    }

    print("[mt_charger_type_detection] chr_type: %d.\r\n", g_ret);
    return g_ret;
}

//==============================================================================
// PMIC Usage APIs
//==============================================================================
U32 get_pmic6397_chip_version (void)
{
    U32 ret=0;
    U32 eco_version = 0;
    
    ret=pmic_read_interface( (U32)(CID),
                             (&eco_version),
                             (U32)(PMIC_CID_MASK),
                             (U32)(PMIC_CID_SHIFT)
                             );

    return eco_version;
}

U32 pmic_IsUsbCableIn (void) 
{    
    U32 ret=0;
    U32 val=0;
    
    ret=pmic_read_interface( (U32)(CHR_CON0),
                             (&val),
                             (U32)(PMIC_RGS_CHRDET_MASK),
                             (U32)(PMIC_RGS_CHRDET_SHIFT)
                             );


    if(val)
        return PMIC_CHRDET_EXIST;
    else
        return PMIC_CHRDET_NOT_EXIST;
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
    if (val==1){     
        printf("[pmic_detect_powerkey_PL] Release\n");
        return 0;
    }else{
        printf("[pmic_detect_powerkey_PL] Press\n");
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
    
    return val;
}

void hw_set_cc(int cc_val)
{
#if 0
    U32 ret_val=0;
    U32 reg_val=0;    
    U32 i=0;
    U32 hw_charger_ov_flag=0;

    printf("hw_set_cc: %d\r\n", cc_val);
    
    //VCDT_HV_VTH, 7V
    ret_val=pmic_config_interface(CHR_CON1, 0x0B, PMIC_RG_VCDT_HV_VTH_MASK, PMIC_RG_VCDT_HV_VTH_SHIFT); 
    //VCDT_HV_EN=1
    ret_val=pmic_config_interface(CHR_CON0, 0x01, PMIC_RG_VCDT_HV_EN_MASK, PMIC_RG_VCDT_HV_EN_SHIFT); 
    //CS_EN=1
    ret_val=pmic_config_interface(CHR_CON2, 0x01, PMIC_RG_CS_EN_MASK, PMIC_RG_CS_EN_SHIFT);
    //CSDAC_MODE=1
    ret_val=pmic_config_interface(CHR_CON23, 0x01, PMIC_RG_CSDAC_MODE_MASK, PMIC_RG_CSDAC_MODE_SHIFT);

    ret_val=pmic_read_interface(CHR_CON0, &hw_charger_ov_flag, PMIC_RGS_VCDT_HV_DET_MASK, PMIC_RGS_VCDT_HV_DET_SHIFT);
    if(hw_charger_ov_flag == 1)
    {
        ret_val=pmic_config_interface(CHR_CON0, 0x00, PMIC_RG_CHR_EN_MASK, PMIC_RG_CHR_EN_SHIFT);
        printf("[PreLoader_charger_ov] turn off charging \n"); 
        return;
    }

    // CS_VTH
    switch(cc_val){
        case 1600: ret_val=pmic_config_interface(CHR_CON4, 0x00, PMIC_RG_CS_VTH_MASK, PMIC_RG_CS_VTH_SHIFT); break;
        case 1500: ret_val=pmic_config_interface(CHR_CON4, 0x01, PMIC_RG_CS_VTH_MASK, PMIC_RG_CS_VTH_SHIFT); break;       
        case 1400: ret_val=pmic_config_interface(CHR_CON4, 0x02, PMIC_RG_CS_VTH_MASK, PMIC_RG_CS_VTH_SHIFT); break;
        case 1300: ret_val=pmic_config_interface(CHR_CON4, 0x03, PMIC_RG_CS_VTH_MASK, PMIC_RG_CS_VTH_SHIFT); break;
        case 1200: ret_val=pmic_config_interface(CHR_CON4, 0x04, PMIC_RG_CS_VTH_MASK, PMIC_RG_CS_VTH_SHIFT); break;
        case 1100: ret_val=pmic_config_interface(CHR_CON4, 0x05, PMIC_RG_CS_VTH_MASK, PMIC_RG_CS_VTH_SHIFT); break;
        case 1000: ret_val=pmic_config_interface(CHR_CON4, 0x06, PMIC_RG_CS_VTH_MASK, PMIC_RG_CS_VTH_SHIFT); break;
        case 900:  ret_val=pmic_config_interface(CHR_CON4, 0x07, PMIC_RG_CS_VTH_MASK, PMIC_RG_CS_VTH_SHIFT); break;            
        case 800:  ret_val=pmic_config_interface(CHR_CON4, 0x08, PMIC_RG_CS_VTH_MASK, PMIC_RG_CS_VTH_SHIFT); break;
        case 700:  ret_val=pmic_config_interface(CHR_CON4, 0x09, PMIC_RG_CS_VTH_MASK, PMIC_RG_CS_VTH_SHIFT); break;       
        case 650:  ret_val=pmic_config_interface(CHR_CON4, 0x0A, PMIC_RG_CS_VTH_MASK, PMIC_RG_CS_VTH_SHIFT); break;
        case 550:  ret_val=pmic_config_interface(CHR_CON4, 0x0B, PMIC_RG_CS_VTH_MASK, PMIC_RG_CS_VTH_SHIFT); break;
        case 450:  ret_val=pmic_config_interface(CHR_CON4, 0x0C, PMIC_RG_CS_VTH_MASK, PMIC_RG_CS_VTH_SHIFT); break;
        case 400:  ret_val=pmic_config_interface(CHR_CON4, 0x0D, PMIC_RG_CS_VTH_MASK, PMIC_RG_CS_VTH_SHIFT); break;
        case 200:  ret_val=pmic_config_interface(CHR_CON4, 0x0E, PMIC_RG_CS_VTH_MASK, PMIC_RG_CS_VTH_SHIFT); break;
        case 70:   ret_val=pmic_config_interface(CHR_CON4, 0x0F, PMIC_RG_CS_VTH_MASK, PMIC_RG_CS_VTH_SHIFT); break;            
        default:
            dbg_print("hw_set_cc: argument invalid!!\r\n");
            break;
    }

    ret_val=pmic_config_interface(CHR_CON21, 0x04, PMIC_RG_CSDAC_DLY_MASK, PMIC_RG_CSDAC_DLY_SHIFT);
    ret_val=pmic_config_interface(CHR_CON21, 0x01, PMIC_RG_CSDAC_STP_MASK, PMIC_RG_CSDAC_STP_SHIFT);
    ret_val=pmic_config_interface(CHR_CON20, 0x01, PMIC_RG_CSDAC_STP_INC_MASK, PMIC_RG_CSDAC_STP_INC_SHIFT);
    ret_val=pmic_config_interface(CHR_CON20, 0x02, PMIC_RG_CSDAC_STP_DEC_MASK, PMIC_RG_CSDAC_STP_DEC_SHIFT);
    ret_val=pmic_config_interface(CHR_CON13, 0x03, PMIC_RG_CHRWDT_TD_MASK, PMIC_RG_CHRWDT_TD_SHIFT);
    ret_val=pmic_config_interface(CHR_CON15, 0x00, PMIC_RG_CHRWDT_INT_EN_MASK, PMIC_RG_CHRWDT_INT_EN_SHIFT);
    ret_val=pmic_config_interface(CHR_CON13, 0x00, PMIC_RG_CHRWDT_EN_MASK, PMIC_RG_CHRWDT_EN_SHIFT);
    ret_val=pmic_config_interface(CHR_CON15, 0x01, PMIC_RG_CHRWDT_FLAG_WR_MASK, PMIC_RG_CHRWDT_FLAG_WR_SHIFT);
    ret_val=pmic_config_interface(CHR_CON0, 0x01, PMIC_RG_CSDAC_EN_MASK, PMIC_RG_CSDAC_EN_SHIFT);
    ret_val=pmic_config_interface(CHR_CON23, 0x01, PMIC_RG_HWCV_EN_MASK, PMIC_RG_HWCV_EN_SHIFT);
    ret_val=pmic_config_interface(CHR_CON0, 0x01, PMIC_RG_CHR_EN_MASK, PMIC_RG_CHR_EN_SHIFT);
#ifndef USER_BUILD
    for(i=CHR_CON0 ; i<=CHR_CON29 ; i++)    
    {        
        ret_val=pmic_read_interface(i,&reg_val,0xFFFF,0x0);        
        print("[PreLoader] Bank0[0x%x]=0x%x\n", i, reg_val);    
    }
#endif
    printf("hw_set_cc: done\r\n");
#endif
}

void pl_hw_ulc_det(void)
{
    U32 ret_val=0;
    
    ret_val=pmic_config_interface(CHR_CON23, 0x01, PMIC_RG_ULC_DET_EN_MASK, PMIC_RG_ULC_DET_EN_SHIFT);
    ret_val=pmic_config_interface(CHR_CON22, 0x01, PMIC_RG_LOW_ICH_DB_MASK, PMIC_RG_LOW_ICH_DB_SHIFT);
}

int hw_check_battery(void)
{
    U32 ret_val=0;
    U32 reg_val=0;

    ret_val=pmic_config_interface(CHR_CON7,    0x01, PMIC_RG_BATON_EN_MASK, PMIC_RG_BATON_EN_SHIFT);      //BATON_EN=1
    ret_val=pmic_config_interface(CHR_CON7,    0x00, PMIC_BATON_TDET_EN_MASK, PMIC_BATON_TDET_EN_SHIFT);  //BATON_TDET_EN=0
    ret_val=pmic_config_interface(AUXADC_CON0, 0x00, PMIC_RG_BUF_PWD_B_MASK, PMIC_RG_BUF_PWD_B_SHIFT);    //RG_BUF_PWD_B=0
    //dump to check
    ret_val=pmic_read_interface(CHR_CON7,&reg_val,0xFFFF,0x0);    print("[hw_check_battery+] [0x%x]=0x%x\n",CHR_CON7,reg_val);
    ret_val=pmic_read_interface(AUXADC_CON0,&reg_val,0xFFFF,0x0); print("[hw_check_battery+] [0x%x]=0x%x\n",AUXADC_CON0,reg_val);

    ret_val=pmic_read_interface(CHR_CON7, &reg_val, PMIC_RGS_BATON_UNDET_MASK, PMIC_RGS_BATON_UNDET_SHIFT);

    if (reg_val == 1)
    {                     
        printf("[pl_hw_check_battery] No Battery!!\n");
        //dump to check
        ret_val=pmic_read_interface(CHR_CON7,&reg_val,0xFFFF,0x0);    print("[hw_check_battery-] [0x%x]=0x%x\n",CHR_CON7,reg_val);
        ret_val=pmic_read_interface(AUXADC_CON0,&reg_val,0xFFFF,0x0); print("[hw_check_battery-] [0x%x]=0x%x\n",AUXADC_CON0,reg_val);        
        return 0;
    }
    else
    {
        printf("[pl_hw_check_battery] Battery exist!!\n");
        //dump to check
        ret_val=pmic_read_interface(CHR_CON7,&reg_val,0xFF,0x0);    print("[hw_check_battery-] [0x%x]=0x%x\n",CHR_CON7,reg_val);
        ret_val=pmic_read_interface(AUXADC_CON0,&reg_val,0xFF,0x0); print("[hw_check_battery-] [0x%x]=0x%x\n",AUXADC_CON0,reg_val);
        pl_hw_ulc_det();        
        return 1;
    }
}

void pl_charging(int en_chr)
{
    U32 ret_val=0;
    U32 reg_val=0;
    U32 i=0;
    
    if(en_chr == 1)
    {
        printf("[pl_charging] enable\n");    
        hw_set_cc(450);
        //USBDL set 1
        ret_val=pmic_config_interface(CHR_CON16, 0x01, PMIC_RG_USBDL_SET_MASK, PMIC_RG_USBDL_SET_SHIFT);        
    }
    else
    {
        printf("[pl_charging] disable\n");    
        //USBDL set 0
        ret_val=pmic_config_interface(CHR_CON16, 0x00, PMIC_RG_USBDL_SET_MASK, PMIC_RG_USBDL_SET_SHIFT);
        ret_val=pmic_config_interface(CHR_CON23, 0x00, PMIC_RG_HWCV_EN_MASK, PMIC_RG_HWCV_EN_SHIFT);
        ret_val=pmic_config_interface(CHR_CON0, 0x00, PMIC_RG_CHR_EN_MASK, PMIC_RG_CHR_EN_SHIFT);        
    }
#ifndef USER_BUILD
    for(i=CHR_CON0 ; i<=CHR_CON29 ; i++)    
    {        
        ret_val=pmic_read_interface(i,&reg_val,0xFFFF,0x0);        
        print("[pl_charging] Bank0[0x%x]=0x%x\n", i, reg_val);    
    }
#endif
}

void pl_kick_chr_wdt(void)
{
    int ret_val=0;

    ret_val=pmic_config_interface(CHR_CON13, 0x03, PMIC_RG_CHRWDT_TD_MASK, PMIC_RG_CHRWDT_TD_SHIFT);
    ret_val=pmic_config_interface(CHR_CON15, 0x00, PMIC_RG_CHRWDT_INT_EN_MASK, PMIC_RG_CHRWDT_INT_EN_SHIFT);
    ret_val=pmic_config_interface(CHR_CON13, 0x00, PMIC_RG_CHRWDT_EN_MASK, PMIC_RG_CHRWDT_EN_SHIFT);
    ret_val=pmic_config_interface(CHR_CON15, 0x01, PMIC_RG_CHRWDT_FLAG_WR_MASK, PMIC_RG_CHRWDT_FLAG_WR_SHIFT);

    //printf("[pl_kick_chr_wdt] done\n");
}

void pl_close_pre_chr_led(void)
{
    U32 ret_val=0;    

    ret_val=pmic_config_interface(CHR_CON22, 0x00, PMIC_RG_CHRIND_ON_MASK, PMIC_RG_CHRIND_ON_SHIFT);
    
    printf("[pmic_init] Close pre-chr LED\n");
}

//==============================================================================
// PMIC Init Code
//==============================================================================
void PMIC_INIT_SETTING_V1(void)
{
    U32 ret = 0;
	U32 chip_version = 0;
    /* put init setting from DE/SA */

	ret = pmic_config_interface(0x0, 0x0, 0x1, 0);	/* [0:0]: RG_VCDT_HV_EN; Disable HV. Only compare LV threshold. */

	chip_version = get_pmic6397_chip_version();
	switch (chip_version) {
	/* [7:4]: RG_VCDT_HV_VTH; 7V OVP */
	case PMIC6391_E1_CID_CODE:
	case PMIC6391_E2_CID_CODE:
	case PMIC6391_E3_CID_CODE:
		ret = pmic_config_interface(0x2, 0xC, 0xF, 4);
		break;
	case PMIC6397_E2_CID_CODE:
	case PMIC6397_E3_CID_CODE:
	case PMIC6397_E4_CID_CODE:
		ret = pmic_config_interface(0x2, 0xB, 0xF, 4);
		break;
	default:
		print("[Power/PMIC] Error chip ID %d\r\n", chip_version);
		ret = pmic_config_interface(0x2, 0xB, 0xF, 4);
		break;
	}
    ret = pmic_config_interface(0xC,0x1,0x7,1); // [3:1]: RG_VBAT_OV_VTH; VBAT_OV=4.3V
    ret = pmic_config_interface(0x1A,0x3,0xF,0); // [3:0]: RG_CHRWDT_TD; align to 6250's
    ret = pmic_config_interface(0x24,0x1,0x1,1); // [1:1]: RG_BC11_RST; 
    ret = pmic_config_interface(0x2A,0x0,0x7,4); // [6:4]: RG_CSDAC_STP; align to 6250's setting
    ret = pmic_config_interface(0x2E,0x1,0x1,7); // [7:7]: RG_ULC_DET_EN; 
    ret = pmic_config_interface(0x2E,0x1,0x1,6); // [6:6]: RG_HWCV_EN; 
    ret = pmic_config_interface(0x2E,0x1,0x1,2); // [2:2]: RG_CSDAC_MODE; 
    ret = pmic_config_interface(0x102,0x0,0x1,3); // [3:3]: RG_PWMOC_CK_PDN; For OC protection
    ret = pmic_config_interface(0x128,0x1,0x1,9); // [9:9]: RG_SRCVOLT_HW_AUTO_EN; 
    ret = pmic_config_interface(0x128,0x1,0x1,8); // [8:8]: RG_OSC_SEL_AUTO; 
    ret = pmic_config_interface(0x128,0x1,0x1,6); // [6:6]: RG_SMPS_DIV2_SRC_AUTOFF_DIS; 
    ret = pmic_config_interface(0x128,0x1,0x1,5); // [5:5]: RG_SMPS_AUTOFF_DIS; 
    ret = pmic_config_interface(0x130,0x1,0x1,7); // [7:7]: VDRM_DEG_EN; 
    ret = pmic_config_interface(0x130,0x1,0x1,6); // [6:6]: VSRMCA7_DEG_EN; 
    ret = pmic_config_interface(0x130,0x1,0x1,5); // [5:5]: VPCA7_DEG_EN; 
    ret = pmic_config_interface(0x130,0x1,0x1,4); // [4:4]: VIO18_DEG_EN; 
    ret = pmic_config_interface(0x130,0x1,0x1,3); // [3:3]: VGPU_DEG_EN; For OC protection
    ret = pmic_config_interface(0x130,0x1,0x1,2); // [2:2]: VCORE_DEG_EN; 
    ret = pmic_config_interface(0x130,0x1,0x1,1); // [1:1]: VSRMCA15_DEG_EN; 
    ret = pmic_config_interface(0x130,0x1,0x1,0); // [0:0]: VCA15_DEG_EN; 
    ret = pmic_config_interface(0x178,0x1,0x1,11); // [11:11]: RG_INT_EN_THR_H; 
    ret = pmic_config_interface(0x178,0x1,0x1,10); // [10:10]: RG_INT_EN_THR_L; 
    ret = pmic_config_interface(0x178,0x1,0x1,4); // [4:4]: RG_INT_EN_BAT_L; 
    ret = pmic_config_interface(0x17E,0x1,0x1,11); // [11:11]: RG_INT_EN_VGPU; OC protection
    ret = pmic_config_interface(0x17E,0x1,0x1,8); // [8:8]: RG_INT_EN_VCA15; OC protection
    ret = pmic_config_interface(0x206,0x600,0x0FFF,0); // [12:0]: BUCK_RSV; for OC protection
    ret = pmic_config_interface(0x210,0x1,0x3,10); // [11:10]: QI_VCORE_VSLEEP; sleep mode only (0.7V)
    ret = pmic_config_interface(0x210,0x0,0x3,6); // [7:6]: QI_VSRMCA7_VSLEEP; sleep mode only (0.85V)
    ret = pmic_config_interface(0x210,0x1,0x3,4); // [5:4]: QI_VSRMCA15_VSLEEP; sleep mode only (0.7V)
    ret = pmic_config_interface(0x210,0x0,0x3,2); // [3:2]: QI_VPCA7_VSLEEP; sleep mode only (0.85V)
    ret = pmic_config_interface(0x210,0x1,0x3,0); // [1:0]: QI_VCA15_VSLEEP; sleep mode only (0.7V)
    ret = pmic_config_interface(0x216,0x0,0x3,12); // [13:12]: RG_VCA15_CSL2; for OC protection
    ret = pmic_config_interface(0x216,0x0,0x3,10); // [11:10]: RG_VCA15_CSL1; for OC protection
    ret = pmic_config_interface(0x224,0x1,0x1,15); // [15:15]: VCA15_SFCHG_REN; soft change rising enable
    ret = pmic_config_interface(0x224,0x5,0x7F,8); // [14:8]: VCA15_SFCHG_RRATE; soft change rising step=0.5us
    ret = pmic_config_interface(0x224,0x1,0x1,7); // [7:7]: VCA15_SFCHG_FEN; soft change falling enable
    ret = pmic_config_interface(0x224,0x17,0x7F,0); // [6:0]: VCA15_SFCHG_FRATE; soft change falling step=2us
    ret = pmic_config_interface(0x22A,0x0,0x7F,0); // [6:0]: VCA15_VOSEL_SLEEP; sleep mode only (0.7V)
    ret = pmic_config_interface(0x238,0x1,0x1,8); // [8:8]: VCA15_VSLEEP_EN; set sleep mode reference voltage from R2R to V2V
    ret = pmic_config_interface(0x238,0x3,0x3,4); // [5:4]: VCA15_VOSEL_TRANS_EN; rising & falling enable
    ret = pmic_config_interface(0x244,0x1,0x1,5); // [5:5]: VSRMCA15_TRACK_SLEEP_CTRL;
    ret = pmic_config_interface(0x246,0x0,0x3,4); // [5:4]: VSRMCA15_VOSEL_SEL;
    ret = pmic_config_interface(0x24A,0x1,0x1,15); // [15:15]: VSRMCA15_SFCHG_REN; 
    ret = pmic_config_interface(0x24A,0x5,0x7F,8); // [14:8]: VSRMCA15_SFCHG_RRATE; 
    ret = pmic_config_interface(0x24A,0x1,0x1,7); // [7:7]: VSRMCA15_SFCHG_FEN; 
    ret = pmic_config_interface(0x24A,0x17,0x7F,0); // [6:0]: VSRMCA15_SFCHG_FRATE; 
    ret = pmic_config_interface(0x250,0x00,0x7F,0); // [6:0]: VSRMCA15_VOSEL_SLEEP; Sleep mode setting only (0.7V)
    ret = pmic_config_interface(0x25E,0x1,0x1,8); // [8:8]: VSRMCA15_VSLEEP_EN; set sleep mode reference voltage from R2R to V2V
    ret = pmic_config_interface(0x25E,0x3,0x3,4); // [5:4]: VSRMCA15_VOSEL_TRANS_EN; rising & falling enable
    ret = pmic_config_interface(0x270,0x1,0x1,1); // [1:1]: VCORE_VOSEL_CTRL; sleep mode voltage control follow SRCLKEN
    ret = pmic_config_interface(0x272,0x0,0x3,4); // [5:4]: VCORE_VOSEL_SEL;
    ret = pmic_config_interface(0x276,0x1,0x1,15); // [15:15]: VCORE_SFCHG_REN; 
    ret = pmic_config_interface(0x276,0x5,0x7F,8); // [14:8]: VCORE_SFCHG_RRATE; 
    ret = pmic_config_interface(0x276,0x17,0x7F,0); // [6:0]: VCORE_SFCHG_FRATE; 
    ret = pmic_config_interface(0x27C,0x0,0x7F,0); // [6:0]: VCORE_VOSEL_SLEEP; Sleep mode setting only (0.7V)
    ret = pmic_config_interface(0x28A,0x1,0x1,8); // [8:8]: VCORE_VSLEEP_EN; Sleep mode HW control  R2R to VtoV
    ret = pmic_config_interface(0x28A,0x0,0x3,4); // [5:4]: VCORE_VOSEL_TRANS_EN; Follows MT6320 VCORE setting.
    ret = pmic_config_interface(0x28A,0x3,0x3,0); // [1:0]: VCORE_TRANSTD; 
    ret = pmic_config_interface(0x28E,0x1,0x3,8); // [9:8]: RG_VGPU_CSL; for OC protection
    ret = pmic_config_interface(0x29C,0x1,0x1,15); // [15:15]: VGPU_SFCHG_REN; 
    ret = pmic_config_interface(0x29C,0x5,0x7F,8); // [14:8]: VGPU_SFCHG_RRATE; 
    ret = pmic_config_interface(0x29C,0x17,0x7F,0); // [6:0]: VGPU_SFCHG_FRATE; 
    ret = pmic_config_interface(0x2B0,0x0,0x3,4); // [5:4]: VGPU_VOSEL_TRANS_EN; 
    ret = pmic_config_interface(0x2B0,0x3,0x3,0); // [1:0]: VGPU_TRANSTD; 
    ret = pmic_config_interface(0x332,0x0,0x3,4); // [5:4]: VPCA7_VOSEL_SEL; 
    ret = pmic_config_interface(0x336,0x1,0x1,15); // [15:15]: VPCA7_SFCHG_REN; 
    ret = pmic_config_interface(0x336,0x5,0x7F,8); // [14:8]: VPCA7_SFCHG_RRATE; 
    ret = pmic_config_interface(0x336,0x1,0x1,7); // [7:7]: VPCA7_SFCHG_FEN; 
    ret = pmic_config_interface(0x336,0x17,0x7F,0); // [6:0]: VPCA7_SFCHG_FRATE; 
    ret = pmic_config_interface(0x33C,0x18,0x7F,0); // [6:0]: VPCA7_VOSEL_SLEEP; 
    ret = pmic_config_interface(0x34A,0x1,0x1,8); // [8:8]: VPCA7_VSLEEP_EN; 
    ret = pmic_config_interface(0x34A,0x3,0x3,4); // [5:4]: VPCA7_VOSEL_TRANS_EN; 
    ret = pmic_config_interface(0x356,0x0,0x1,5); // [5:5]: VSRMCA7_TRACK_SLEEP_CTRL;
    ret = pmic_config_interface(0x358,0x0,0x3,4); // [5:4]: VSRMCA7_VOSEL_SEL; 
    ret = pmic_config_interface(0x35C,0x1,0x1,15); // [15:15]: VSRMCA7_SFCHG_REN; 
    ret = pmic_config_interface(0x35C,0x5,0x7F,8); // [14:8]: VSRMCA7_SFCHG_RRATE; 
    ret = pmic_config_interface(0x35C,0x1,0x1,7); // [7:7]: VSRMCA7_SFCHG_FEN; 
    ret = pmic_config_interface(0x35C,0x17,0x7F,0); // [6:0]: VSRMCA7_SFCHG_FRATE; 
    ret = pmic_config_interface(0x362,0x18,0x7F,0); // [6:0]: VSRMCA7_VOSEL_SLEEP; 
    ret = pmic_config_interface(0x370,0x1,0x1,8); // [8:8]: VSRMCA7_VSLEEP_EN; 
    ret = pmic_config_interface(0x370,0x3,0x3,4); // [5:4]: VSRMCA7_VOSEL_TRANS_EN; 
    ret = pmic_config_interface(0x39C,0x1,0x1,8); // [8:8]: VDRM_VSLEEP_EN; 
    ret = pmic_config_interface(0x440,0x1,0x1,2); // [2:2]: VIBR_THER_SHEN_EN; 
    ret = pmic_config_interface(0x500,0x1,0x1,5); // [5:5]: THR_HWPDN_EN; 
    ret = pmic_config_interface(0x502,0x1,0x1,3); // [3:3]: RG_RST_DRVSEL; 
    ret = pmic_config_interface(0x502,0x1,0x1,2); // [2:2]: RG_EN_DRVSEL; 
    ret = pmic_config_interface(0x508,0x1,0x1,1); // [1:1]: PWRBB_DEB_EN; 
    ret = pmic_config_interface(0x50C,0x1,0x1,12); // [12:12]: VSRMCA15_PG_H2L_EN; 
    ret = pmic_config_interface(0x50C,0x1,0x1,11); // [11:11]: VPCA15_PG_H2L_EN; 
    ret = pmic_config_interface(0x50C,0x1,0x1,10); // [10:10]: VCORE_PG_H2L_EN; 
    ret = pmic_config_interface(0x50C,0x1,0x1,9); // [9:9]: VSRMCA7_PG_H2L_EN; 
    ret = pmic_config_interface(0x50C,0x1,0x1,8); // [8:8]: VPCA7_PG_H2L_EN; 
    ret = pmic_config_interface(0x512,0x1,0x1,1); // [1:1]: STRUP_PWROFF_PREOFF_EN; 
    ret = pmic_config_interface(0x512,0x1,0x1,0); // [0:0]: STRUP_PWROFF_SEQ_EN; 
    ret = pmic_config_interface(0x55E,0xFC,0xFF,8); // [15:8]: RG_ADC_TRIM_CH_SEL; 
    ret = pmic_config_interface(0x560,0x1,0x1,1); // [1:1]: FLASH_THER_SHDN_EN; 
    ret = pmic_config_interface(0x566,0x1,0x1,1); // [1:1]: KPLED_THER_SHDN_EN; 
    ret = pmic_config_interface(0x600,0x1,0x1,9); // [9:9]: SPK_THER_SHDN_L_EN; 
    ret = pmic_config_interface(0x604,0x0,0x1,0); // [0:0]: RG_SPK_INTG_RST_L;
    ret = pmic_config_interface(0x606,0x1,0x1,9); // [9:9]: SPK_THER_SHDN_R_EN; 
    ret = pmic_config_interface(0x60A,0x1,0xF,11); // [14:11]: RG_SPKPGA_GAINR; 
    ret = pmic_config_interface(0x612,0x1,0xF,8); // [11:8]: RG_SPKPGA_GAINL; 
    ret = pmic_config_interface(0x632,0x1,0x1,8); // [8:8]: FG_SLP_EN; 
    ret = pmic_config_interface(0x638,0xFFC2,0xFFFF,0); // [15:0]: FG_SLP_CUR_TH; 
    ret = pmic_config_interface(0x63A,0x14,0xFF,0); // [7:0]: FG_SLP_TIME; 
    ret = pmic_config_interface(0x63C,0xFF,0xFF,8); // [15:8]: FG_DET_TIME; 
    ret = pmic_config_interface(0x714,0x1,0x1,7); // [7:7]: RG_LCLDO_ENC_REMOTE_SENSE_VA28; 
    ret = pmic_config_interface(0x714,0x1,0x1,4); // [4:4]: RG_LCLDO_REMOTE_SENSE_VA33; 
    ret = pmic_config_interface(0x714,0x1,0x1,1); // [1:1]: RG_HCLDO_REMOTE_SENSE_VA33; 
    ret = pmic_config_interface(0x71A,0x1,0x1,15); // [15:15]: RG_NCP_REMOTE_SENSE_VA18; 
    ret = pmic_config_interface(0x260,0x10,0x7F,8); // [14:8]: VSRMCA15_VOSEL_OFFSET; set offset=100mV
    ret = pmic_config_interface(0x260,0x0,0x7F,0); // [6:0]: VSRMCA15_VOSEL_DELTA; set delta=0mV
    ret = pmic_config_interface(0x262,0x48,0x7F,8); // [14:8]: VSRMCA15_VOSEL_ON_HB; set HB=1.15V
    ret = pmic_config_interface(0x262,0x25,0x7F,0); // [6:0]: VSRMCA15_VOSEL_ON_LB; set LB=0.93125V
    ret = pmic_config_interface(0x264,0x0,0x7F,0); // [6:0]: VSRMCA15_VOSEL_SLEEP_LB; set sleep LB=0.7V
    ret = pmic_config_interface(0x372,0x4,0x7F,8); // [14:8]: VSRMCA7_VOSEL_OFFSET; set offset=25mV
    ret = pmic_config_interface(0x372,0x0,0x7F,0); // [6:0]: VSRMCA7_VOSEL_DELTA; set delta=0mV
    ret = pmic_config_interface(0x374,0x48,0x7F,8); // [14:8]: VSRMCA7_VOSEL_ON_HB; set HB=1.15V
    ret = pmic_config_interface(0x374,0x25,0x7F,0); // [6:0]: VSRMCA7_VOSEL_ON_LB; set LB=0.93125V
    ret = pmic_config_interface(0x376,0x18,0x7F,0); // [6:0]: VSRMCA7_VOSEL_SLEEP_LB; set sleep LB=0.85000V
    ret = pmic_config_interface(0x21E,0x3,0x3,0); // [1:1]: VCA15_VOSEL_CTRL, VCA15_EN_CTRL; DVS HW control by SRCLKEN
    ret = pmic_config_interface(0x244,0x3,0x3,0); // [1:1]: VSRMCA15_VOSEL_CTRL, VSRAM15_EN_CTRL;
    ret = pmic_config_interface(0x330,0x0,0x1,1); // [1:1]: VPCA7_VOSEL_CTRL;
    ret = pmic_config_interface(0x356,0x0,0x1,1); // [1:1]: VSRMCA7_VOSEL_CTRL;
    ret = pmic_config_interface(0x21E,0x1,0x1,4); // [4:4]: VCA15_TRACK_ON_CTRL; DVFS tracking enable
    ret = pmic_config_interface(0x244,0x1,0x1,4); // [4:4]: VSRMCA15_TRACK_ON_CTRL;
    ret = pmic_config_interface(0x330,0x0,0x1,4); // [4:4]: VPCA7_TRACK_ON_CTRL;
    ret = pmic_config_interface(0x356,0x0,0x1,4); // [4:4]: VSRMCA7_TRACK_ON_CTRL;
    ret = pmic_config_interface(0x134,0x3,0x3,14); // [15:14]: VGPU OC; 
    ret = pmic_config_interface(0x134,0x3,0x3,2); // [3:2]: VCA15 OC;
}

#define Vcore_HV (0x38 + 0x11)  //1.15
#define Vcore_NV (0x38 + 0x00)  //1.05
#define Vcore_LV (0x38 - 0x11)  //0.95  , +8 =>1.00V
#define Vmem_HV (0x56 + 0x09)
#define Vmem_NV (0x56 + 0x00)
#define Vmem_LV (0x56 - 0x09)

static void pmic_default_buck_voltage(void)
{
	int reg_val=0;
	int buck_val=0;
	pmic_read_interface(EFUSE_DOUT_288_303, &reg_val, 0xFFFF, 0);
	if ((reg_val &0x01) == 0x01) {
		print("[EFUSE_DOUT_288_303] FUSE 288=0x%x\n", reg_val);

		/* VCORE */
		pmic_read_interface(EFUSE_DOUT_256_271, &reg_val, 0xF, 12);
		pmic_read_interface(VCORE_CON9, &buck_val, PMIC_VCORE_VOSEL_MASK, PMIC_VCORE_VOSEL_SHIFT);
		buck_val = (buck_val&0x07)|(reg_val<<3);
		pmic_config_interface(VCORE_CON9, buck_val, PMIC_VCORE_VOSEL_MASK, PMIC_VCORE_VOSEL_SHIFT);
		pmic_config_interface(VCORE_CON10, buck_val, PMIC_VCORE_VOSEL_ON_MASK, PMIC_VCORE_VOSEL_ON_SHIFT);

		pmic_read_interface(EFUSE_DOUT_272_287, &reg_val, 0xFFFF, 0);
		/* VCA15 */
		buck_val = 0;
		pmic_read_interface(VCA15_CON9, &buck_val, PMIC_VCA15_VOSEL_MASK, PMIC_VCA15_VOSEL_SHIFT);
		buck_val = (buck_val&0x07)|((reg_val&0x0F)<<3);
		pmic_config_interface(VCA15_CON9, buck_val, PMIC_VCA15_VOSEL_MASK, PMIC_VCA15_VOSEL_SHIFT);
		pmic_config_interface(VCA15_CON10, buck_val, PMIC_VCA15_VOSEL_ON_MASK, PMIC_VCA15_VOSEL_ON_SHIFT);

		/* VSAMRCA15 */
		buck_val = 0;
		pmic_read_interface(VSRMCA15_CON9, &buck_val, PMIC_VSRMCA15_VOSEL_MASK, PMIC_VSRMCA15_VOSEL_SHIFT);
		buck_val = (buck_val&0x07)|((reg_val&0xF0)>>1);
		pmic_config_interface(VSRMCA15_CON9, buck_val, PMIC_VSRMCA15_VOSEL_MASK, PMIC_VSRMCA15_VOSEL_SHIFT);
		pmic_config_interface(VSRMCA15_CON10, buck_val, PMIC_VSRMCA15_VOSEL_ON_MASK, PMIC_VSRMCA15_VOSEL_ON_SHIFT);

		/* VCA7 */
		buck_val = 0;
		pmic_read_interface(VPCA7_CON9, &buck_val, PMIC_VPCA7_VOSEL_MASK, PMIC_VPCA7_VOSEL_SHIFT);
		buck_val = (buck_val&0x07)|((reg_val&0xF00)>>5);
		pmic_config_interface(VPCA7_CON9, buck_val, PMIC_VPCA7_VOSEL_MASK, PMIC_VPCA7_VOSEL_SHIFT);
		pmic_config_interface(VPCA7_CON10, buck_val, PMIC_VPCA7_VOSEL_ON_MASK, PMIC_VPCA7_VOSEL_ON_SHIFT);

		/* VSAMRCA7 */
		buck_val = 0;
		pmic_read_interface(VSRMCA7_CON9, &buck_val, PMIC_VPCA7_VOSEL_MASK, PMIC_VPCA7_VOSEL_SHIFT);
		buck_val = (buck_val&0x07)|((reg_val&0xF000)>>9);
		pmic_config_interface(VSRMCA7_CON9, buck_val, PMIC_VSRMCA7_VOSEL_MASK, PMIC_VSRMCA7_VOSEL_SHIFT);
		pmic_config_interface(VSRMCA7_CON10, buck_val, PMIC_VSRMCA7_VOSEL_ON_MASK, PMIC_VSRMCA7_VOSEL_ON_SHIFT);

		//set the power control by register(use original) 
		pmic_config_interface(BUCK_CON3,0x1,0x1,12);
	}
}

U32 pmic_init (void)
{
    U32 ret_code = PMIC_TEST_PASS;
    int ret_val=0;
    int reg_val=0;

    print("[pmic_init] Start..................\n");

    /* Adjust default BUCK voltage */
    pmic_default_buck_voltage();

    //Enable PMIC RST function (depends on main chip RST function)
    /*
        state1: RG_SYSRSTB_EN = 1, RG_STRUP_MAN_RST_EN=1, RG_RST_PART_SEL=1
        state2: RG_SYSRSTB_EN = 1, RG_STRUP_MAN_RST_EN=0, RG_RST_PART_SEL=1
        state3: RG_SYSRSTB_EN = 1, RG_STRUP_MAN_RST_EN=x, RG_RST_PART_SEL=0
    */
    ret_val=pmic_config_interface(TOP_RST_MISC,  0x1, PMIC_RG_SYSRSTB_EN_MASK, PMIC_RG_SYSRSTB_EN_SHIFT);
    ret_val=pmic_config_interface(TOP_RST_MISC,  0x0, PMIC_RG_STRUP_MAN_RST_EN_MASK, PMIC_RG_STRUP_MAN_RST_EN_SHIFT);
	ret_val=pmic_config_interface(TOP_RST_MISC,  0x1, PMIC_RG_RST_PART_SEL_MASK, PMIC_RG_RST_PART_SEL_SHIFT);
    ret_val=pmic_read_interface(TOP_RST_MISC, &reg_val, 0xFFFF, 0);
    print("[pmic_init] Enable PMIC RST function (depends on main chip RST function) Reg[0x%x]=0x%x\n", TOP_RST_MISC, reg_val);
    
    //Enable CA15 by default for different PMIC behavior
    pmic_config_interface(VCA15_CON7, 0x1, PMIC_VCA15_EN_MASK, PMIC_VCA15_EN_SHIFT);
    pmic_config_interface(VSRMCA15_CON7, 0x1, PMIC_VSRMCA15_EN_MASK, PMIC_VSRMCA15_EN_SHIFT);
    pmic_config_interface(VPCA7_CON7, 0x0, PMIC_VPCA7_EN_MASK, PMIC_VPCA7_EN_SHIFT);
    gpt_busy_wait_us(200);
    g_ca15_ready = 1;
        
    ret_val=pmic_read_interface(VCA15_CON7, &reg_val, 0xFFFF, 0);
    print("Reg[0x%x]=0x%x\n", VCA15_CON7, reg_val);
    ret_val=pmic_read_interface(VSRMCA15_CON7, &reg_val, 0xFFFF, 0);
    print("Reg[0x%x]=0x%x\n", VSRMCA15_CON7, reg_val);

    //pmic initial setting
    PMIC_INIT_SETTING_V1();
    print("[PMIC_INIT_SETTING_V1] Done\n");

    //26M clock amplitute adjust
    pmic_config_interface(RG_DCXO_ANALOG_CON1, 0x0, PMIC_RG_DCXO_LDO_BB_V_MASK, PMIC_RG_DCXO_LDO_BB_V_SHIFT);
    pmic_config_interface(RG_DCXO_ANALOG_CON1, 0x1, PMIC_RG_DCXO_ATTEN_BB_MASK, PMIC_RG_DCXO_ATTEN_BB_SHIFT);

    //cal 12Mhz clock if no efuse
    //vWrite2BCbus(0x039E, 0x0041);
    //vWrite2BCbus(0x039E, 0x0040);	
	//vWrite2BCbus(0x039E, 0x0050);
	pmic_read_interface(EFUSE_DOUT_304_319, &reg_val, 0xFFFF, 0);
	if ((reg_val & 0x8000) == 0)
	{
	    pmic_config_interface(BUCK_K_CON0, 0x0041, 0xFFFF, 0);
	    pmic_config_interface(BUCK_K_CON0, 0x0040, 0xFFFF, 0);
	    pmic_config_interface(BUCK_K_CON0, 0x0050, 0xFFFF, 0);
    }

#ifdef MTK_BQ24297_SUPPORT
    //BC11_RST=1
    pmic_config_interface(CHR_CON18,0x1,PMIC_RG_BC11_RST_MASK,PMIC_RG_BC11_RST_SHIFT); 
    //RG_BC11_BB_CTRL=1
    pmic_config_interface(CHR_CON18,0x1,PMIC_RG_BC11_BB_CTRL_MASK,PMIC_RG_BC11_BB_CTRL_SHIFT);    
#endif
	pmic_config_interface(CHR_CON13, 0x00, PMIC_RG_CHRWDT_EN_MASK, PMIC_RG_CHRWDT_EN_SHIFT);

#if CFG_BATTERY_DETECT
    hw_check_battery();
#endif

    //Set VDRM to 1.21875V only for 8173
    pmic_config_interface(VDRM_CON9, 0x43, 0x7F,0);
    pmic_config_interface(VDRM_CON10, 0x43, 0x7F,0);
	
    #ifdef MEMPLL_CLK_793
    pmic_config_interface(VCORE_CON9, Vcore_HV, 0x7F,0);
    pmic_config_interface(VCORE_CON10, Vcore_HV, 0x7F,0);
    pmic_config_interface(VDRM_CON9, Vmem_HV, 0x7F,0);
    pmic_config_interface(VDRM_CON10, Vmem_HV, 0x7F,0);
    #endif
	pmic_config_interface(VSRMCA7_CON9, 0x40, PMIC_VSRMCA7_VOSEL_MASK, PMIC_VSRMCA7_VOSEL_SHIFT);
	pmic_config_interface(VSRMCA7_CON10, 0x40, PMIC_VSRMCA7_VOSEL_ON_MASK, PMIC_VSRMCA7_VOSEL_ON_SHIFT);
	da9212_driver_probe();
	
	//按照MTK邮件修改，解决lcd供电不稳导致的开机黑块/黑条纹
	DRV_WriteReg32(0x1021581c,0x000200);
	DRV_WriteReg32(0x1021681c,0x000200);
	
    print("[pmic_init] Done...................\n");

    return ret_code;
}

#define Vcore_HV_LPPDR3  (0x48)   //1.150V
#define Vcore_NV_LPPDR3  (0x44)   //1.125V
#define Vcore_LV_LPPDR3  (0x34)   //1.025V
#define Vcore_LLV_LPPDR3 (0x25)   //0.931V

void  pmic_vcore_init (void)
{
  
   #if (CFG_DDR_HIGH_VCORE||CFG_DISP_PTPOD_SUPPORT || CFG_VDEC_PTPOD_SUPPORT || CFG_VENC_PTPOD_SUPPORT)
            pmic_config_interface(VCORE_CON9, Vcore_NV_LPPDR3, 0x7F,0);//Vcore 1.125V
            pmic_config_interface(VCORE_CON10,Vcore_NV_LPPDR3, 0x7F,0);
   #else
            pmic_config_interface(VCORE_CON9, Vcore_LV_LPPDR3, 0x7F,0);//Vcore 1.025V
            pmic_config_interface(VCORE_CON10,Vcore_LV_LPPDR3, 0x7F,0);
   #endif
 
}
