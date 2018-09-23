#ifndef _H_EXTD_DDP_
#define _H_EXTD_DDP_
//#include <linux/types.h>
#include "ddp_hal.h"
#include "mt_typedefs.h"
#include "ddp_data_type.h"
#include "lcm_drv.h"
#include "ddp_reg.h"
#include "disp_event.h"

enum EXT_DISP_STATUS {
	EXT_DISP_STATUS_OK = 0,

	EXT_DISP_STATUS_NOT_IMPLEMENTED,
	EXT_DISP_STATUS_ALREADY_SET,
	EXT_DISP_STATUS_ERROR,
} ;

struct ext_disp_input_config {
	unsigned int layer;
	unsigned int layer_en;
	unsigned int fmt;
	unsigned long addr;
	unsigned long addr_sub_u;
	unsigned long addr_sub_v;
	unsigned long vaddr;
	unsigned int src_x;
	unsigned int src_y;
	unsigned int src_w;
	unsigned int src_h;
	unsigned int src_pitch;
	unsigned int dst_x;
	unsigned int dst_y;
	unsigned int dst_w;
	unsigned int dst_h; /* clip region */
	unsigned int keyEn;
	unsigned int key;
	unsigned int aen;
	unsigned char alpha;
	unsigned int sur_aen;
	unsigned int src_alpha;
	unsigned int dst_alpha;

	unsigned int isTdshp;
	unsigned int isDirty;

	unsigned int buff_idx;
	unsigned int identity;
	unsigned int connected_type;
	unsigned int security;
	unsigned int dirty;
} ;

int ext_disp_init(unsigned int hdmi_res);
int ext_disp_config_input(struct ext_disp_input_config *input);
int extd_disp_trigger(int blocking);
unsigned int ext_disp_get_vsync_interval(void);


#endif
