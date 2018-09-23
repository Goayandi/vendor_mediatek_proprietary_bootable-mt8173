/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2015. All rights reserved.
*
* BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
* THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
* RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
* AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
* NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
* SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
* SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
* THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
* THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
* CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
* SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
* STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
* CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
* AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
* OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
* MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*/

#include <sys/types.h>
#include <stdint.h>
#include <platform/partition.h>
#include <platform/mt_typedefs.h>
#include <platform/boot_mode.h>
#include <platform/mt_reg_base.h>
#include <platform/errno.h>
#include <printf.h>
#include <string.h>
#include <malloc.h>
#include <libfdt.h>
#include <platform/mt_gpt.h>
#include <debug.h>
#include "ccci_ld_md_core.h"
#include "ccci_ld_md_errno.h"

#define MODULE_NAME "LK_LD_MD"

/***************************************************************************************************
** Sub section:
**   Telephony operation parsing and prepare part
***************************************************************************************************/
/* For the following option setting, please check config.h after build, or $project.mk */
#ifdef MTK_MD1_SUPPORT
#define MD1_SUPPORT (MTK_MD1_SUPPORT)
#else
#define MD1_SUPPORT (0)
#endif

#ifdef MTK_MD3_SUPPORT
#define MD3_SUPPORT (MTK_MD3_SUPPORT)
#else
#define MD3_SUPPORT (0)
#endif

#ifdef MTK_C2K_LTE_MODE
#define C2K_LTE_MODE    (MTK_C2K_LTE_MODE)
#else
#define C2K_LTE_MODE    (0)
#endif

#ifdef MTK_IRAT_SUPPORT
#define IRAT_SUPPORT    (1)
#else
#define IRAT_SUPPORT    (0)
#endif

#ifdef RAT_CONFIG_LTE_SUPPORT
#define LTE_SUPPORT (1)
#else
#define LTE_SUPPORT (0)
#endif

#ifdef MTK_PROTOCOL1_RAT_CONFIG
#define PS1_RAT_DEFAULT MTK_PROTOCOL1_RAT_CONFIG
#else
#define PS1_RAT_DEFAULT ""
#endif


static unsigned int get_capability(char cap_str[])
{
	if (cap_str == NULL)
		return 0;
	if ((strcmp(cap_str, "LF") == 0) || (strcmp(cap_str, "Lf") == 0) || (strcmp(cap_str, "lf") == 0))
		return MD_CAP_FDD_LTE;
	if ((strcmp(cap_str, "LT") == 0) || (strcmp(cap_str, "Lt") == 0) || (strcmp(cap_str, "lt") == 0))
		return MD_CAP_TDD_LTE;
	if ((strcmp(cap_str, "W") == 0) || (strcmp(cap_str, "w") == 0))
		return MD_CAP_WCDMA;
	if ((strcmp(cap_str, "C") == 0) || (strcmp(cap_str, "c") == 0))
		return MD_CAP_CDMA2000;
	if ((strcmp(cap_str, "T") == 0) || (strcmp(cap_str, "t") == 0))
		return MD_CAP_TDS_CDMA;
	if ((strcmp(cap_str, "G") == 0) || (strcmp(cap_str, "g") == 0))
		return MD_CAP_GSM;

	return 0;
}

#define MAX_CAP_STR_LENGTH	16
static unsigned int get_capablity_bit_map(char str[])
{
	char tmp_str[MAX_CAP_STR_LENGTH];
	int tmp_str_curr_pos = 0;
	unsigned int capability_bit_map = 0;
	int str_len;
	int i;

	if (str == NULL)
		return 0;

	str_len = strlen(str);
	for (i = 0; i < str_len; i++) {
		if (str[i] == ' ')
			continue;
		if (str[i] == '\t')
			continue;
		if ((str[i] == '/') || (str[i] == '_')) {
			if (tmp_str_curr_pos) {
				tmp_str[tmp_str_curr_pos] = 0;
				capability_bit_map |= get_capability(tmp_str);
			}
			tmp_str_curr_pos = 0;
			continue;
		}
		if (tmp_str_curr_pos < (MAX_CAP_STR_LENGTH-1)) {
			tmp_str[tmp_str_curr_pos] = str[i];
			tmp_str_curr_pos++;
		} else
			break;
	}
	if (tmp_str_curr_pos) {
		tmp_str[tmp_str_curr_pos] = 0;
		capability_bit_map |= get_capability(tmp_str);
	}

	return capability_bit_map;
}

#ifdef DUMMY_AP_MODE
static struct opt_cfg default_option_list[] = { /* string lenght should less then 32 */
	{"opt_md1_support", 12},/* if MD1_SUPPORT > 0, MD1_EN true  */
	{"opt_md3_support", 2},/* if MD3_SUPPORT > 0, MD3_EN true, ECCCI_C2K true, C2K_SUPPORT true */
};
#else
#ifdef LK_OPT_TO_KERNEL_CCCI
static struct opt_cfg default_option_list[] = { /* string lenght should less then 32 */
	{"opt_md1_support", MD1_SUPPORT},/* if MD1_SUPPORT > 0, MD1_EN true  */
	{"opt_md3_support", MD3_SUPPORT},/* if MD3_SUPPORT > 0, MD3_EN true, ECCCI_C2K true, C2K_SUPPORT true */
	{"opt_c2k_lte_mode", C2K_LTE_MODE},/* mode 0:none; 1: SVLTE; 2: SRLTE */
	{"opt_irat_support", IRAT_SUPPORT},
	{"opt_lte_support", LTE_SUPPORT},
};
#else
/* Platform relate option not included in this table */
static struct opt_cfg default_option_list[] = { /* string lenght should less then 32 */
	{"opt_md1_support", 12},/* if MD1_SUPPORT > 0, MD1_EN true  */
	{"opt_md3_support", 2},/* if MD3_SUPPORT > 0, MD3_EN true, ECCCI_C2K true, C2K_SUPPORT true */
};
#endif
#endif /* End of DUMMY_AP_MODE */

static char *s_g_local_cmd_line_buf;
static int s_g_local_cmd_line_buf_size;
static int s_g_curr_opt_buf_offset;

/* Using command line buffer to store opt value */
static int add_opt_setting_to_buf(char name[], char val[])
{
	int name_len;
	int value_len;

	if ((name == NULL) || (val == NULL)) {
		ALWAYS_LOG("dt args invalid\n");
		return -1;
	}

	name_len = strlen(name) + 1;
	value_len = strlen(val) + 1;

	if ((s_g_local_cmd_line_buf_size - s_g_curr_opt_buf_offset) < (value_len + name_len)) {
		ALWAYS_LOG("dt buf free size not enough\n");
		return -1;
	}

	/* copy name */
	memcpy(&s_g_local_cmd_line_buf[s_g_curr_opt_buf_offset], name, name_len);
	s_g_curr_opt_buf_offset += name_len;
	/* copy value */
	memcpy(&s_g_local_cmd_line_buf[s_g_curr_opt_buf_offset], val, value_len);
	s_g_curr_opt_buf_offset += value_len;

	return 0;
}


int update_md_opt_to_fdt_firmware(void *fdt)
{
#ifdef LK_OPT_TO_KERNEL_CCCI
	int nodeoffset;
	char *name, *value;
	int name_len, val_len;
	int i = 0;

	if (s_g_curr_opt_buf_offset == 0) {
		ALWAYS_LOG("no need update md_opt_cfg\n");
		return 0;
	}

	nodeoffset = fdt_path_offset(fdt, "/firmware/android");
	if (nodeoffset < 0) {
		ALWAYS_LOG("/firmware/android not found\n");
		return -1;
	}

	while (i < s_g_curr_opt_buf_offset) {
		name_len = strlen(&s_g_local_cmd_line_buf[i]) + 1;
		name = &s_g_local_cmd_line_buf[i];
		i += name_len;
		val_len = strlen(&s_g_local_cmd_line_buf[i]) + 1;
		value = &s_g_local_cmd_line_buf[i];
		i += val_len;
		fdt_setprop_string(fdt, nodeoffset, name, value);
	}
#endif

	return 0;
}

int prepare_tel_fo_setting(void)
{
	unsigned int i;
	char val_str[8];
	int value;
	char *ptr;
	struct opt_cfg *curr_opt_list;
	struct opt_cfg *new_opt_list;
	int using_default = 1;
	unsigned int default_cap_bit_map, lk_env_cap_bit_map = 0;

	default_cap_bit_map = get_capablity_bit_map(PS1_RAT_DEFAULT);
#ifdef ENABLE_PARSING_LK_ENV
	ptr = get_env("opt_ps1_rat");
	lk_env_cap_bit_map = get_capablity_bit_map(ptr);
#else
	ptr = NULL;
#endif

	/* update modem capability setting */
	if (lk_env_cap_bit_map != 0) {
		add_opt_setting_to_buf("opt_ps1_rat", ptr);
		insert_ccci_tag_inf("opt_ps1_rat", (char*)&lk_env_cap_bit_map, sizeof(int));
	} else
		insert_ccci_tag_inf("opt_ps1_rat", (char*)&default_cap_bit_map, sizeof(int));
	ALWAYS_LOG("default rat:%s[0x%x][0x%x]\n", PS1_RAT_DEFAULT, default_cap_bit_map, lk_env_cap_bit_map);

	new_opt_list = (struct opt_cfg *)malloc(sizeof(default_option_list));
	if (new_opt_list == NULL)/* Using default */
		curr_opt_list = default_option_list;
	else {/* copy default or lk_env value to new opt list */
		for (i = 0; i < (sizeof(default_option_list)/sizeof(struct opt_cfg)); i++) {

#ifdef ENABLE_PARSING_LK_ENV
			ptr = get_env(default_option_list[i].name);
#else
			ptr = NULL;
#endif

			if (ptr == NULL)
				value = default_option_list[i].val;
			else {
				snprintf(val_str, sizeof(val_str), "%s", ptr);
				value = str2uint(val_str);
				using_default = 0;
			}
			new_opt_list[i].name = default_option_list[i].name;
			new_opt_list[i].val = value;
		}
		curr_opt_list = new_opt_list;
	}

	if (using_default) {
		ALWAYS_LOG("using default option setting\n");
	} else {
		/* Copy back to default */
		for (i = 0; i < (sizeof(default_option_list)/sizeof(struct opt_cfg)); i++)
			default_option_list[i].val = curr_opt_list[i].val;
	}

	/* Update to tag buffer for kernel */
	for (i = 0; i < (sizeof(default_option_list)/sizeof(struct opt_cfg)); i++) {
		snprintf(val_str, sizeof(val_str), "%d", curr_opt_list[i].val);
		add_opt_setting_to_buf(curr_opt_list[i].name, val_str);
		insert_ccci_tag_inf(curr_opt_list[i].name,
		                    (char*)&curr_opt_list[i].val, sizeof(int));
		/* Special relate setting */
		if (strcmp(curr_opt_list[i].name, "opt_md3_support") == 0) {
			if (curr_opt_list[i].val > 0)
				value = 1;
			else
				value = 0;
			snprintf(val_str, sizeof(val_str), "%d", value);
			add_opt_setting_to_buf("opt_c2k_support", val_str);
			insert_ccci_tag_inf("opt_c2k_support", (char*)&value, sizeof(int));
		}
	}

	/* update using default or not */
	insert_ccci_tag_inf("opt_using_default", (char*)&using_default, sizeof(int));
	snprintf(val_str, sizeof(val_str), "%d", using_default);
	add_opt_setting_to_buf("opt_using_default", val_str);

	/* update platform option */
	value = 1;
	snprintf(val_str, sizeof(val_str), "%d", value);
	add_opt_setting_to_buf("opt_eccci_c2k", val_str);
	insert_ccci_tag_inf("opt_eccci_c2k", (char*)&value, sizeof(int));

#ifdef LK_OPT_TO_KERNEL_CCCI
	value = 1;
	insert_ccci_tag_inf("opt_using_lk_val", (char*)&value, sizeof(int)); /* Notify Kernel using kernel setting */
#endif

	if (new_opt_list)
		free(new_opt_list);

	return 0;
}

int ccci_get_opt_val(char opt[])
{
	int i;

	for (i = 0; i < (int)(sizeof(default_option_list)/sizeof(struct opt_cfg)); i++) {
		if (strcmp(default_option_list[i].name, opt) == 0)
			return default_option_list[i].val;
	}

	return -LD_ERR_OPT_NOT_FOUND;
}

void ccci_append_tel_fo_setting(char *cmdline)
{
	/* Phase out, change to using device tree */
}

int ccci_alloc_local_cmd_line_buf(int size)
{
	s_g_local_cmd_line_buf = malloc(size);
	if (s_g_local_cmd_line_buf == NULL)
		return -LD_ERR_OPT_CMD_BUF_ALLOC_FAIL;
	s_g_local_cmd_line_buf_size = size;
	return 0;
}

void ccci_free_local_cmd_line_buf(void)
{
	if (s_g_local_cmd_line_buf)
		free(s_g_local_cmd_line_buf);

	s_g_local_cmd_line_buf_size = 0;
}