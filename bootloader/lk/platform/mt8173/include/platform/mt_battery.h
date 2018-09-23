#ifndef _mt_battery_header_
#define _mt_battery_header_

typedef struct {
	void (*set_charging_current)(kal_int32 cc);
	void (*set_input_current_limit)(kal_int32 limit);
	void (*set_reg_voltage)(kal_int32 cv);
	void (*charging_hw_init)(void);
	void (*enable_charging)(kal_bool enable);
	void (*charger_dump_register)(void);
	void (*kick_ext_charger_watchdog)(void);
	CHARGER_TYPE (*charger_type_detection)(void);
} charger_control_interface;

extern charger_control_interface charger;

#endif

