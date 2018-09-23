#ifndef _CUST_BAT_H_
#define _CUST_BAT_H_

#include <platform/mt_typedefs.h>

typedef enum {
	Cust_CC_1600MA = 0x0,
	Cust_CC_1500MA = 0x1,
	Cust_CC_1400MA = 0x2,
	Cust_CC_1300MA = 0x3,
	Cust_CC_1200MA = 0x4,
	Cust_CC_1100MA = 0x5,
	Cust_CC_1000MA = 0x6,
	Cust_CC_900MA  = 0x7,
	Cust_CC_800MA  = 0x8,
	Cust_CC_700MA  = 0x9,
	Cust_CC_650MA  = 0xA,
	Cust_CC_550MA  = 0xB,
	Cust_CC_450MA  = 0xC,
	Cust_CC_400MA  = 0xD,
	Cust_CC_200MA  = 0xE,
	Cust_CC_70MA   = 0xF,
	Cust_CC_0MA    = 0xDD
} cust_charging_current_enum;

typedef struct {
	unsigned int BattVolt;
	unsigned int BattPercent;
} VBAT_TO_PERCENT;

/* Battery Temperature Protection */
#define MAX_CHARGE_TEMPERATURE  50
#define MIN_CHARGE_TEMPERATURE  0
#define ERR_CHARGE_TEMPERATURE  0xFF

/* Recharging Battery Voltage */
#define RECHARGING_VOLTAGE      4110

/* Charging Current Setting */
#define USB_CHARGER_CURRENT                 500
#define AC_CHARGER_CURRENT                  1500

/* Battery Meter Solution */
#define CONFIG_ADC_SOLUTION     1

/* Battery Voltage and Percentage Mapping Table */
VBAT_TO_PERCENT Batt_VoltToPercent_Table[] = {
	/*BattVolt,BattPercent*/
	{3400,0},
	{3686,10},
	{3740,20},
	{3771,30},
	{3789,40},
	{3820,50},
	{3873,60},
	{3943,70},
	{4013,80},
	{4100,90},
	{4189,100},
};

/* Precise Tunning */
//#define BATTERY_AVERAGE_SIZE  600
#define BATTERY_AVERAGE_SIZE    60


#define CHARGING_IDLE_MODE   1

#define CHARGING_PICTURE     1

/* Common setting */
#define R_CURRENT_SENSE 2               // 0.2 Ohm
#define R_BAT_SENSE 4                   // times of voltage
#define R_I_SENSE 4                     // times of voltage
#define R_CHARGER_1 330
#define R_CHARGER_2 39
#define R_CHARGER_SENSE ((R_CHARGER_1+R_CHARGER_2)/R_CHARGER_2) // times of voltage
//#define V_CHARGER_MAX 6000                // 6 V
#define V_CHARGER_MAX 7000              // 7 V
#define V_CHARGER_MIN 4400              // 4.4 V
#define V_CHARGER_ENABLE 0              // 1:ON , 0:OFF
#define BACKLIGHT_KEY 10                    // camera key

/* Teperature related setting */
#define RBAT_PULL_UP_R             24000
//#define RBAT_PULL_UP_VOLT          2500
#define RBAT_PULL_UP_VOLT          1200

#define BAT_TEMP_PROTECT_ENABLE    1

typedef struct {
	INT32 BatteryTemp;
	INT32 TemperatureR;
} BATT_TEMPERATURE;

#define BAT_NTC_10 1
#define BAT_NTC_47 0
#define BAT_NTC_100 0

#if BAT_NTC_10
#define TBAT_OVER_CRITICAL_LOW     68237 //use lowest value in NTC table
#endif

#if BAT_NTC_47
#define TBAT_OVER_CRITICAL_LOW     499900
#endif

#if BAT_NTC_100
#define TBAT_OVER_CRITICAL_LOW      1151037
#endif

#if (BAT_NTC_10 == 1)
BATT_TEMPERATURE Batt_Temperature_Table[] = {
	{-20,68237},
	{-15,53650},
	{-10,42506},
	{ -5,33892},
	{  0,27219},
	{  5,22021},
	{ 10,17926},
	{ 15,14674},
	{ 20,12081},
	{ 25,10000},
	{ 30,8315},
	{ 35,6948},
	{ 40,5834},
	{ 45,4917},
	{ 50,4161},
	{ 55,3535},
	{ 60,3014}
};
#endif

#if (BAT_NTC_47 == 1)
BATT_TEMPERATURE Batt_Temperature_Table[] = {
	{-20, 499900},
	{-15, 371600},
	{-10, 278700},
	{-5, 210800},
	{0, 160800},
	{5, 123800},
	{10, 96030},
	{15, 75100},
	{20, 59190},
	{25, 47000},
	{30, 37590},
	{35, 30270},
	{40, 24540},
	{45, 20020},
	{50, 16430},
	{55, 13570},
	{60, 11270},
	{65, 9409},
	{70, 7897}
};
#endif

#if (BAT_NTC_100 == 1)
BATT_TEMPERATURE Batt_Temperature_Table[] = {
	{-20,1151037},
	{-15,846579},
	{-10,628988},
	{ -5,471632},
	{  0,357012},
	{  5,272500},
	{ 10,209710},
	{ 15,162651},
	{ 20,127080},
	{ 25,100000},
	{ 30,79222},
	{ 35,63167},
	{ 40,50677},
	{ 45,40904},
	{ 50,33195},
	{ 55,27091},
	{ 60,22224}
};
#endif

/*****************************************************************************
*   JEITA battery temperature standard
    charging info ,like temperatue, charging current, re-charging voltage, CV threshold would be reconfigurated.
    Temperature hysteresis default 6C.
    Reference table:
    degree    AC Current   USB current    CV threshold    Recharge Vol    hysteresis condition
    > 60      no charging current,              X                X              <54(Down)
    45~60     FULL         500mA           4.096V           3.996V              <39(Down) >60(Up)
    10~45     FULL         500mA           4.208V           4.108V              <10(Down) >45(Up)
    0~10      FULL         500mA           4.096V           3.996V              <0(Down)  >16(Up)
    -10~0     500mA        500mA               4V             3.9V              <-10(Down) >6(Up)
    <-10      no charging current,              X                X              >-10(Up)
****************************************************************************/
typedef enum {
	TEMP_BELOW_NEG_10 = 0,
	TEMP_NEG_10_TO_POS_0,
	TEMP_POS_0_TO_POS_10,
	TEMP_POS_10_TO_POS_45,
	TEMP_POS_45_TO_POS_60,
	TEMP_ABOVE_POS_60
} temp_state_enum;

#define TEMP_POS_60_THRESHOLD  60
#define TEMP_POS_60_THRES_MINUS_X_DEGREE 60

#define TEMP_POS_45_THRESHOLD  45
#define TEMP_POS_45_THRES_MINUS_X_DEGREE 45

#define TEMP_POS_10_THRESHOLD  14
#define TEMP_POS_10_THRES_PLUS_X_DEGREE 14

#define TEMP_POS_0_THRESHOLD  0
#define TEMP_POS_0_THRES_PLUS_X_DEGREE 0

#define TEMP_NEG_10_THRESHOLD  -10
#define TEMP_NEG_10_THRES_PLUS_X_DEGREE  -10  //-10 not threshold

/* The option of new charging animation */
#define ANIMATION_NEW

#endif /* _CUST_BAT_H_ */
