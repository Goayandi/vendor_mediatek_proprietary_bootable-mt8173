#include <config.h>
#include <string.h>
#include <stdlib.h>
#include <printf.h>
#include <platform/mt_irq.h>
#include <kernel/event.h>
#include <sys/types.h>
#include <platform/mt_gpio.h>
#include <linux/input.h>
#include <err.h>
#include <video.h>
#include <platform/mtk_wdt.h>
#include <kernel/timer.h>

#include "mtk_ir_cus_define.h"
#include <platform/mtk_ir_lk_core.h>

#include "mtk_ir_cus_nec.h"
#include "mtk_ir_cus_rc6.h"
#include "mtk_ir_cus_rc5.h"



typedef enum {
	UPDATE_DISPLAY_SECONDS,
	UPDATE_DISPLY_KEY_UP,
	UPDATE_DISPLY_KEY_DOWN,
	UPDATE_DISPLY_KEY_ENTER,
	UPDATA_WAITING_TRIGGER,
	UPDATE_TIMEOUT,
	UPDATE_NONE,
} DISPLAY_MODE;

typedef struct {
	char * unselected_str;
	char * selected_str;
	char * backup_unselected_str;
	BOOTMODE mode;
} MTK_IR_LK_SELECT_S;


static void mtk_ir_clear_irq_stat(void);
extern u32  mtk_ir_nec_decode( void * preserve);
extern u32  mtk_ir_rc6_decode( void * preserve);
extern u32  mtk_ir_rc5_decode( void * preserve);

static void mtk_ir_get_key(void);
static void mtk_ir_boot_mode_menu_select(void);
static enum handler_return mtk_ir_timer_callback(void);


void lk_irrx_irq_handler(unsigned int irq);

static event_t ir_int_event;
static event_t ir_select_event;

static int key_repeat_times = 0;
static bool b_eint_happen = FALSE;

static int time_out_seconds = MTK_LK_BOOT_SELECT_MENU_TIME_OUT;
static timer_t timer_display;
char* time_outmsg ="Time Out after %d seconds, please select\n\n\n";

static DISPLAY_MODE display_mode = UPDATE_NONE;
extern BOOTMODE g_boot_mode;


MTK_IR_LK_SELECT_S display_select[]= {
#if MTK_LK_IRRX_USING_TIMER
	{"Time Out after %d seconds, please select\n\n\n", "", "",NORMAL_BOOT},
#else
	{"Please select\n\n\n", "", "",NORMAL_BOOT},
#endif

	{"Select Boot Mode:\n[ BTN_DOWN, BTN_UP to select. BTN_ENTER is OK.]\n\n","","",NORMAL_BOOT},
	{"[Normal      Boot]   \n","[Normal      Boot]          <<== \n","",NORMAL_BOOT},
	{"[Recovery    Mode]   \n","[Recovery    Mode]          <<== \n","",RECOVERY_BOOT},
	{"[Factory    Mode]    \n","[Factory    Mode]           <<== \n","",FACTORY_BOOT},
	{"","",NORMAL_BOOT},
};



#define FIRST_SELECT_ITEM_INDEX 2 //[Normal      Boot] 
#define LAST_SLECT_ITEM_INDEX  4 //[Factory    Mode] 



IR_GLOBAL_T g_ir = {

#if (MTK_IRRX_PROTOCOL == MTK_IR_ID_NEC)
	.proname = "NEC",
	.u4_config_high = MTK_NEC_CONFIG,
	.u4_config_low  = MTK_NEC_SAPERIOD,
	.u4_theshold    = MTK_NEC_THRESHOLD,
	.pmsg = mtk_nec_lk_table,
	.u4msg_size = ARRAY_SIZE(mtk_nec_lk_table),
	.ir_hw_decode   = mtk_ir_nec_decode,
#elif (MTK_IRRX_PROTOCOL == MTK_IR_ID_RC6)
	.proname = "RC6",
	.u4_config_high = MTK_RC6_CONFIG,
	.u4_config_low  = MTK_RC6_SAPERIOD,
	.u4_theshold    = MTK_RC6_THRESHOLD,
	.pmsg = mtk_rc6_lk_table,
	.u4msg_size = ARRAY_SIZE(mtk_rc6_lk_table),
	.ir_hw_decode   = mtk_ir_rc6_decode,
#elif (MTK_IRRX_PROTOCOL == MTK_IR_ID_RC5)
	.proname = "RC5",
	.u4_config_high = MTK_RC5_CONFIG,
	.u4_config_low  = MTK_RC5_SAPERIOD,
	.u4_theshold    = MTK_RC5_THRESHOLD,
	.pmsg = mtk_rc5_lk_table,
	.u4msg_size = ARRAY_SIZE(mtk_rc5_lk_table),
	.ir_hw_decode   = mtk_ir_rc5_decode,
#else
	ERROROUT"pls #define MTK_IRRX_PROTOCOL"  here must build fail!!!!
#endif.
};

static void mtk_ir_set_display_mode(DISPLAY_MODE mode)
{
	display_mode = mode;
	event_signal(&ir_select_event,true);
}
static enum handler_return mtk_ir_timer_callback(void)
{
	//IR_LOG_ALWAYS("Time Out after %d seconds, please select\n\n\n",time_out_seconds);
	time_out_seconds --;
	if (time_out_seconds <=0) {
		IR_LOG_ALWAYS("Time is out\n");
		mtk_ir_set_display_mode(UPDATE_TIMEOUT);

	} else {
		//IR_LOG_ALWAYS(" updata display\n");
		mtk_ir_set_display_mode(UPDATE_DISPLAY_SECONDS);
	}
}

void mtk_ir_clear_irq_stat(void)
{
	IR_WRITE_MASK(IRRX_IRCLR,IRRX_IRCLR_MASK,IRRX_IRCLR_OFFSET,0x1); // clear irrx state machine
	IR_WRITE_MASK(IRRX_IRINT_CLR,IRRX_INTCLR_MASK,IRRX_INTCLR_OFFSET,0x1);
}

void mtk_ir_get_key(void)
{
	static u32 last_key =  BTN_NONE;

	u32 cur_key = g_ir.ir_hw_decode(NULL);

	static int skip_repeat_times = 0; // when in selected ui,  to avoid too much repeat that ui refresh


	if (display_mode != UPDATE_NONE ) {
		u32 keycode = KEY_RESERVED;
		DISPLAY_MODE mode = UPDATE_NONE;

		skip_repeat_times ++;

		if ((skip_repeat_times % 4) != 0) { //every 5 repeat key
			return;
		}

		for (int i=0; i<g_ir.u4msg_size; i++) {
			if (cur_key == g_ir.pmsg[i].scancode) {
				keycode = g_ir.pmsg[i].keycode;
				break;
			}
		}

		if (keycode == KEY_DOWN) {
			mode = UPDATE_DISPLY_KEY_DOWN;
		} else if (keycode == KEY_UP) {
			mode = UPDATE_DISPLY_KEY_UP;
		} else if (keycode == KEY_ENTER) {
			mode = UPDATE_DISPLY_KEY_ENTER;
		} else {
			return;
		}

		mtk_ir_set_display_mode(mode);
		skip_repeat_times = 0;
	} else {

		if (last_key == BTN_NONE) {
			key_repeat_times = 0;// clear key_repeat_times = 0 ,recount it
		} else if (cur_key != last_key) {
			key_repeat_times = 0;// clear key_repeat_times = 0, remount
		} else {
			key_repeat_times ++;
		}

		last_key = cur_key;
		IR_LOG_ALWAYS("display_mode%d\n",display_mode);
		IR_LOG_ALWAYS("%d,0x%08x\n",key_repeat_times,cur_key);
		if (key_repeat_times == MTK_LK_DETECTED_REEPAT_TIMES) {
			IR_LOG_ALWAYS("event_signal, key_repeat_times = %d\n",key_repeat_times);
			event_signal(&ir_int_event,false);
		}
	}

}

/*lk irrx irq function */
void lk_irrx_irq_handler(unsigned int irq)
{
	mt_irq_ack(MT_IRRX_IRQ_ID);
	b_eint_happen = TRUE;
	mtk_ir_get_key();
	mtk_ir_clear_irq_stat();
}

int mtk_ir_wait_event()
{
	status_t stat = 0;

	if (b_eint_happen == FALSE) {
		return IRRX_NO_INTERRUPT;
	}
	IR_LOG_ALWAYS("\n");
	stat = event_wait_timeout(&ir_int_event,MTK_LK_WAIT_REPEAT_TIME_OUT);
	if (stat == NO_ERROR) { //ok
		IR_LOG_ALWAYS("key_repeat_times (%d),enter menu mode select\n",key_repeat_times);

#if MTK_LK_IRRX_USING_TIMER
		timer_initialize(&timer_display);
		timer_set_periodic(&timer_display,1000,mtk_ir_timer_callback,NULL);
#endif


		mtk_wdt_disable();
		mtk_ir_boot_mode_menu_select();
		mtk_wdt_init();
		mt_irq_mask(MT_IRRX_IRQ_ID);


	} else { // no ir repeat times detected
		mt_irq_mask(MT_IRRX_IRQ_ID);
		key_repeat_times = 0;
		IR_LOG_ALWAYS("stat = %d\n",stat);
		IR_LOG_ALWAYS(" time out when wait repeat_key %d times in %d ms",
		              MTK_LK_DETECTED_REEPAT_TIMES,MTK_LK_WAIT_REPEAT_TIME_OUT);

		return IRRX_WAIT_TIMEOUT;

	}
	return IRRX_OK;
}

int mtk_ir_init(int para)
{

	IR_LOG_ALWAYS("%s\n",g_ir.proname);

	IR_LOG_ALWAYS("MTK_LK_DETECTED_REEPAT_TIMES(%d)\n",MTK_LK_DETECTED_REEPAT_TIMES);
	IR_LOG_ALWAYS("MTK_LK_WAIT_REPEAT_TIME_OUT(%d)\n",MTK_LK_WAIT_REPEAT_TIME_OUT);
	IR_LOG_ALWAYS("MTK_LK_BOOT_SELECT_MENU_TIME_OUT(%d)\n",MTK_LK_BOOT_SELECT_MENU_TIME_OUT);
	IR_LOG_ALWAYS("MTK_LK_BOOT_SELECT_MENU_TIME_OUT(%d)\n",MTK_LK_IRRX_USING_TIMER);

	// disable interrupt
	IR_WRITE_MASK(IRRX_IRINT_EN,IRRX_INTEN_MASK,IRRX_INTCLR_OFFSET,0x0);

	IR_WRITE32(IRRX_CONFIG_HIGH_REG,  g_ir.u4_config_high);
	IR_WRITE32(IRRX_CONFIG_LOW_REG,  g_ir.u4_config_low);
	IR_WRITE32(IRRX_THRESHOLD_REG, g_ir.u4_theshold);
	mtk_ir_clear_irq_stat();
	event_init(&ir_int_event,false,EVENT_FLAG_AUTOUNSIGNAL);
	event_init(&ir_select_event,false,EVENT_FLAG_AUTOUNSIGNAL);
	mt_irq_set_sens(MT_IRRX_IRQ_ID, MT65xx_LEVEL_SENSITIVE);
	mt_irq_set_polarity(MT_IRRX_IRQ_ID, MT65xx_POLARITY_HIGH);
	mt_irq_unmask(MT_IRRX_IRQ_ID);
	// enable ir interrupt
	IR_WRITE_MASK(IRRX_IRINT_EN,IRRX_INTEN_MASK,IRRX_INTCLR_OFFSET,0x1);
	mt_set_gpio_mode(GPIO1,GPIO_MODE_01); // enable irrx pin function
	return 0;
}




void mtk_ir_display_string(int select)
{
	int i = 0;
	video_clean_screen();
	video_set_cursor(video_get_rows()/2, 0);
	do {
		if (i == select) {
			video_printf(display_select[i].selected_str);
		} else {
			video_printf(display_select[i].unselected_str);
		}
		i++;

	} while (strcmp(display_select[i].unselected_str,"") != 0 );
	video_set_cursor(video_get_rows()/2, 0);

}



void mtk_ir_boot_mode_menu_select()
{
	int select = FIRST_SELECT_ITEM_INDEX;
	int i = 0;

	for (; i<= LAST_SLECT_ITEM_INDEX; i++) {
		display_select[i].backup_unselected_str = display_select[i].unselected_str;
	}

	mtk_ir_display_string(select);
	char *str = malloc(512);

	display_mode = UPDATA_WAITING_TRIGGER ;
	IR_LOG_ALWAYS("display_mode%d\n",display_mode);
	while (1) {
		status_t stat = 0;
		stat = event_wait(&ir_select_event);
		if (stat != NO_ERROR) {
			IR_LOG_ALWAYS("stat =%d\n",stat);
			return;
		}

		switch (display_mode) {
			case UPDATE_TIMEOUT: // no select time out
				IR_LOG_ALWAYS("Time out\n");
				display_select[0].unselected_str = "Time out\n";

#if MTK_LK_IRRX_USING_TIMER
				timer_cancel(&timer_display);
#endif

				mtk_ir_display_string(0);
				video_clean_screen();
				return;

			case UPDATA_WAITING_TRIGGER:

				continue;

			case UPDATE_DISPLY_KEY_ENTER:

				g_boot_mode = display_select[select].mode;
				IR_LOG_ALWAYS("irrx: select = %d,g_boot_mode = %d\n",select,g_boot_mode);

#if MTK_LK_IRRX_USING_TIMER
				timer_cancel(&timer_display);
#endif
				video_clean_screen();
				return;

			case UPDATE_DISPLY_KEY_DOWN:

				//here is to restore last select's unselect stat, because UPDATE_DISPLAY_SECONDS has changed it,
				// so restore it's value from backup_unselected_str;

				display_select[select].unselected_str = display_select[select].backup_unselected_str;
				select ++;
				if (select == (LAST_SLECT_ITEM_INDEX + 1)) {
					select = FIRST_SELECT_ITEM_INDEX;
				}

				mtk_ir_display_string(select);

				break;

			case UPDATE_DISPLY_KEY_UP:

				//here is to restore last select's unselect stat, because UPDATE_DISPLAY_SECONDS has changed it,
				// so restore it's value from backup_unselected_str;
				display_select[select].unselected_str = display_select[select].backup_unselected_str;
				select --;
				if (select == (FIRST_SELECT_ITEM_INDEX -1)) {
					select = LAST_SLECT_ITEM_INDEX ;
				}
				mtk_ir_display_string(select);
				break;

			case UPDATE_DISPLAY_SECONDS:

				sprintf(str,time_outmsg,time_out_seconds);
				display_select[0].unselected_str = str;
				display_select[0].selected_str = str;

				//here is to maintain original selected item's selected stat on up
				display_select[select].unselected_str = display_select[select].selected_str;
				mtk_ir_display_string(0);

				break;
			default:
				break;

		}

	}
}


