#ifndef _MT_PMIC_LK_SW_H_
#define _MT_PMIC_LK_SW_H_

#include <platform/mt_typedefs.h>

//==============================================================================
// PMIC6397 Define
//==============================================================================
#define PMIC6391_E1_CID_CODE    0x1091
#define PMIC6391_E2_CID_CODE    0x2091
#define PMIC6391_E3_CID_CODE    0x3091
#define PMIC6397_E1_CID_CODE    0x1097
#define PMIC6397_E2_CID_CODE    0x2097
#define PMIC6397_E3_CID_CODE    0x3097
#define PMIC6397_E4_CID_CODE    0x4097

#define AUXADC_BATTERY_VOLTAGE_CHANNEL  0
#define AUXADC_REF_CURRENT_CHANNEL      1
#define AUXADC_CHARGER_VOLTAGE_CHANNEL  2
#define AUXADC_TEMPERATURE_CHANNEL      3

#define VOLTAGE_FULL_RANGE  1200
#define ADC_PRECISE         1024 // 10 bits

typedef enum {
	CHARGER_UNKNOWN = 0,
	STANDARD_HOST,          // USB : 450mA
	CHARGING_HOST,
	NONSTANDARD_CHARGER,    // AC : 450mA~1A
	STANDARD_CHARGER,       // AC : ~1A
	APPLE_2_1A_CHARGER,
	APPLE_1_0A_CHARGER,
	APPLE_0_5A_CHARGER,
} CHARGER_TYPE;

#define NOUSE(a) (a = a)

//==============================================================================
// PMIC6397 Exported Function
//==============================================================================
extern U32 pmic_read_interface (U32 RegNum, U32 *val, U32 MASK, U32 SHIFT);
extern U32 pmic_config_interface (U32 RegNum, U32 val, U32 MASK, U32 SHIFT);
extern U32 pmic_IsUsbCableIn (void);
extern kal_bool upmu_is_chr_det(void);
extern int pmic_detect_powerkey(void);
extern int pmic_detect_powerkey(void);
extern kal_uint32 upmu_get_reg_value(kal_uint32 reg);
extern void PMIC_DUMP_ALL_Register(void);
extern U32 pmic6320_init (void);
extern int PMIC_IMM_GetOneChannelValue(int dwChannel, int deCount, int trimd);
extern int get_bat_sense_volt(int times);
extern int get_i_sense_volt(int times);
extern int get_charger_volt(int times);
extern int get_tbat_volt(int times);
extern CHARGER_TYPE mt_charger_type_detection(void);
extern CHARGER_TYPE hw_charger_type_detection(void);

//==============================================================================
// PMIC6397 Status Code
//==============================================================================
#define PMIC_TEST_PASS               0x0000
#define PMIC_TEST_FAIL               0xB001
#define PMIC_EXCEED_I2C_FIFO_LENGTH  0xB002
#define PMIC_CHRDET_EXIST            0xB003
#define PMIC_CHRDET_NOT_EXIST        0xB004

//==============================================================================
// PMIC6397 Register Index
//==============================================================================
//register number
#include "upmu_hw.h"

//==============================================================================
// MT6397 APIs
//==============================================================================
//#include "upmu_common.h"

#endif // _MT_PMIC_LK_SW_H_


