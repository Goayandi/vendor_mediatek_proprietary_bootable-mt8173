

#include <platform/disp_drv_platform.h>
#include <platform/extd_ddp.h>
#include <platform/ddp_manager.h>
#include <platform/disp_drv.h>

#include <platform/hdmi_drv.h>
//#include <platform/primary_display.h>


#define EXTD_DDP_LK_BOOT

#ifdef EXTD_DDP_LK_BOOT
typedef void * cmdqRecHandle;
#endif


#if 1
enum EXTD_POWER_STATE {
	EXTD_DEINIT = 0,
	EXTD_INIT,
	EXTD_RESUME,
	EXTD_SUSPEND
};

enum EXT_DISP_PATH_MODE {
	EXTD_DIRECT_LINK_MODE,
	EXTD_DECOUPLE_MODE,
	EXTD_SINGLE_LAYER_MODE,
	EXTD_DEBUG_RDMA_DPI_MODE
} ;

typedef struct {
	LCM_PARAMS *params;
	LCM_DRIVER *drv;
	LCM_INTERFACE_ID lcm_if_id;
	int module;
	int is_inited;
} extd_drv_handle, *pextd_drv_handle;

typedef struct {
	enum EXTD_POWER_STATE state;
	int init;
	unsigned int session;
	int need_trigger_overlay;
	enum EXT_DISP_PATH_MODE mode;
	unsigned int last_vsync_tick;

#ifndef EXTD_DDP_LK_BOOT
	struct mutex lock;
#endif

	extd_drv_handle *plcm;
	cmdqRecHandle cmdq_handle_config;
	cmdqRecHandle cmdq_handle_trigger;
	disp_path_handle dpmgr_handle;
	disp_path_handle ovl2mem_path_handle;
} ext_disp_path_context;

static ext_disp_path_context g_extd_context;

static ext_disp_path_context *pgc = &g_extd_context;

disp_ddp_path_config extd_dpi_params;


static int extd_disp_build_path_direct_link(void)
{
	int ret = 0;

	DISP_MODULE_ENUM dst_module = 0;
	DISPFUNC();
	pgc->mode = EXTD_DIRECT_LINK_MODE;

	/*create dpmgr_handle*/
	pgc->dpmgr_handle = dpmgr_create_path(DDP_SCENARIO_SUB_DISP, pgc->cmdq_handle_config);
	if (pgc->dpmgr_handle) {
		DISPCHECK("dpmgr create path SUCCESS(0x%p)\n", pgc->dpmgr_handle);
	} else {
		DISPCHECK("dpmgr create path FAIL\n");
		return -1;
	}

	dst_module = DISP_MODULE_DPI0;
	dpmgr_path_set_dst_module(pgc->dpmgr_handle, dst_module);
	DISPCHECK("dpmgr set dst module FINISHED(%s)\n", ddp_get_module_name(dst_module));


	return ret;
}


static int extd_disp_path_context_init(void)
{
	DISPFUNC();

	//if(!is_context_inited)
	{
		memset((void*)&g_extd_context, 0, sizeof(ext_disp_path_context));
		pgc = &g_extd_context;
		//is_context_inited = 1;
	}

	return 0;
}

static int _should_config_ovl_input(void)
{
	DISPFUNC();

	/* should extend this when display path dynamic switch is ready */
	if (pgc->mode == EXTD_SINGLE_LAYER_MODE || pgc->mode == EXTD_DEBUG_RDMA_DPI_MODE)
		return 0;
	else
		return 1;

}

static int _convert_disp_input_to_rdma(RDMA_CONFIG_STRUCT *dst, struct ext_disp_input_config *src)
{
	DISPFUNC();

	if (src && dst) {
		dst->inputFormat = src->fmt;
		dst->address = src->addr;
		dst->width = src->src_w;
		dst->height = src->src_h;
		dst->pitch = src->src_pitch;

		return 0;
	} else {
		DISPERR("src(0x%p) or dst(0x%p) is null\n", src, dst);
		return -1;
	}
}

static int _convert_disp_input_to_ovl(OVL_CONFIG_STRUCT *dst,struct ext_disp_input_config *src)
{
	DISPFUNC();

	if (src && dst) {
		dst->layer = src->layer;
		dst->layer_en = src->layer_en;
		dst->fmt = src->fmt;
		dst->addr = src->addr;
		dst->vaddr = src->vaddr;
		dst->src_x = src->src_x;
		dst->src_y = src->src_y;
		dst->src_w = src->src_w;
		dst->src_h = src->src_h;
		dst->src_pitch = src->src_pitch;
		dst->dst_x = src->dst_x;
		dst->dst_y = src->dst_y;
		dst->dst_w = src->dst_w;
		dst->dst_h = src->dst_h;
		dst->keyEn = src->keyEn;
		dst->key = src->key;
		dst->aen = src->aen;
		dst->alpha = src->alpha;

		//dst->sur_aen = src->sur_aen;
		//dst->src_alpha = src->src_alpha;
		//dst->dst_alpha = src->dst_alpha;

		dst->isDirty = src->isDirty;

		dst->buff_idx = src->buff_idx;
		dst->identity = src->identity;
		dst->connected_type = src->connected_type;
		dst->security = src->security;

		return 0;
	} else {
		DISPERR("src(0x%p) or dst(0x%p) is null\n", src, dst);
		return -1;
	}
}

int ext_disp_config_input(struct ext_disp_input_config *input)
{
	int ret = 0;

	DISPFUNC();

	disp_ddp_path_config *data_config;

	/* all dirty should be cleared in dpmgr_path_get_last_config() */


	data_config = dpmgr_path_get_last_config(pgc->dpmgr_handle);
	data_config->dst_dirty = 0;
	data_config->ovl_dirty = 0;
	data_config->rdma_dirty = 0;
	data_config->wdma_dirty = 0;




	/* hope we can use only 1 input struct for input config, just set layer number */
	if (_should_config_ovl_input()) {
		ret = _convert_disp_input_to_ovl(&(data_config->ovl_config[input->layer]), input);
		data_config->ovl_dirty = 1;
	} else {
		ret = _convert_disp_input_to_rdma(&(data_config->rdma_config), input);
		data_config->rdma_dirty = 1;
	}



	//memcpy(&(data_config->dispif_config), &(extd_dpi_params.dispif_config), sizeof(LCM_PARAMS));
	ret =
	    dpmgr_path_config(pgc->dpmgr_handle, data_config,CMDQ_DISABLE);

	/* this is used for decouple mode, to indicate whether we need to trigger ovl */
	pgc->need_trigger_overlay = 1;
	/* /DISPMSG("ext_disp_config_input done\n"); */



	return ret;
}


int extd_disp_trigger(int blocking)
{
	int ret = 0;
	DISPFUNC();

	if (pgc->mode == EXTD_DIRECT_LINK_MODE || pgc->mode == EXTD_SINGLE_LAYER_MODE) {

		DISPCHECK("trigger mode: %s\n", (pgc->mode == EXTD_DIRECT_LINK_MODE)?"DIRECT_LINK":"SINGLE_LAYER");
		dpmgr_path_start(pgc->dpmgr_handle, CMDQ_DISABLE);
		//if(primary_display_use_cmdq == CMDQ_DISABLE)
		{
			dpmgr_path_trigger(pgc->dpmgr_handle, NULL,CMDQ_DISABLE);
		}

		//_set_cmdq_config_handle_dirty();
		//_reset_cmdq_config_handle();

		//if(blocking)
		{
			//dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ*2);
		}
	} else if (pgc->mode == EXTD_DECOUPLE_MODE) {

	} else if (pgc->mode == EXTD_DEBUG_RDMA_DPI_MODE) {

	} else {
		DISPCHECK("primary display mode is WRONG(%d)\n", (unsigned int)pgc->mode);
	}


	return ret;
}

int ext_disp_is_video_mode(void)
{
	// TODO: we should store the video/cmd mode in runtime, because ROME will support cmd/vdo dynamic switch
	return 1;
}

unsigned int ext_disp_get_vsync_interval(void)
{
	int ret = 0;
	unsigned int time0 = 0;
	unsigned int time1 = 0;
	unsigned int lcd_time = 0;
	unsigned int fps = 0;
	DISPFUNC();

	ret = dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC,2000);
	if (ret <= 0) {
		DISPERR("wait vsycn time out! #%d\n", __LINE__);
		goto fail;
	}

	ret = dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC,2000);
	if (ret <= 0) {
		DISPERR("wait vsycn time out! #%d\n", __LINE__);
		goto fail;
	}

	// because we are polling irq status, so the first event should be ignored
	time0 = gpt4_tick2time_us(gpt4_get_current_tick());
	//DISPMSG("vsync signaled:%d\n", gpt4_tick2time_us(gpt4_get_current_tick()));

	ret = dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC,2000);
	if (ret <= 0) {
		DISPERR("wait vsycn time out! #%d\n", __LINE__);
		goto fail;
	}
	//DISPMSG("vsync signaled:%d\n", gpt4_tick2time_us(gpt4_get_current_tick()));

	ret = dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC,2000);
	if (ret > 0) {
		time1 = gpt4_tick2time_us(gpt4_get_current_tick());
	} else {
		DISPERR("wait vsycn time out! #%d\n", __LINE__);
		goto fail;
	}
	lcd_time = (time1 - time0)/2;

	if (0 != lcd_time)
		fps = (100000000/lcd_time);
	else
		fps = (6000);

	DISPMSG("lcd_time %u fps %u\n", lcd_time, fps);

	return fps ;
fail:
	DISPERR("wait event fail\n");
	return 0;
}

#endif


int ext_disp_init(unsigned int hdmi_res)
{
	disp_ddp_path_config data_config;
	DISP_STATUS ret = DISP_STATUS_OK;
	DISPFUNC();


	dprintf(0,"ext_disp_init,hdmi_res=0x%08x\n",hdmi_res);

#if  HDMI_SUB_PATH_LK
	unsigned int resolutionmode = 0xb;
	unsigned int deepcolor = 1;
	unsigned int colorspace = 0;
	unsigned int audiofs = 1;
	unsigned int chnumber = 2;
	unsigned int audiotype = 0;

	dprintf(0,"hdmi_res=0x%08x\n",hdmi_res);

	set_hdmi_tmds_driver(hdmi_res, deepcolor, colorspace, audiofs,  chnumber,  audiotype);

	//dpi_enable_colorbar(0,1); /*enable dpi0 color bar for lk test*/
#endif

#if HDMI_SUB_PATH_LK
	/*context init*/
	extd_disp_path_context_init();

	/**************************************************************************/
	/*create path*/
	extd_disp_build_path_direct_link();

	/**************************************************************************/

	/*path init*/
	dpmgr_path_init(pgc->dpmgr_handle, CMDQ_DISABLE);

	/*path config*/
	memset((void *)&data_config, 0, sizeof(disp_ddp_path_config));


	data_config.dst_w = hdmi_get_width();
	data_config.dst_h = hdmi_get_height();

	dprintf(0,"dst_w=0x%x dst_h=0x%x\n",data_config.dst_w,data_config.dst_h);

	data_config.dst_dirty = 1;
	ret = dpmgr_path_config(pgc->dpmgr_handle, &data_config, CMDQ_DISABLE);



	if (ext_disp_is_video_mode()) {
		dpmgr_map_event_to_irq(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC, DDP_IRQ_RDMA1_DONE);
	}

	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC);
	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE);

#endif
	return 0;
}
