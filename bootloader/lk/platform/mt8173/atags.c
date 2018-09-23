/* Copyright (c) 2009-2012, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <atags.h>
#include <reg.h>
#include <debug.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <platform.h>
#include <platform/mt_typedefs.h>
#include <platform/boot_mode.h>
#include <platform/mt_reg_base.h>
#include <platform/sec_devinfo.h>
#include <platform/env.h>
#include <platform/sec_export.h>
#include <libfdt.h>
/* #include <dfo_boot_default.h> */
extern int g_nr_bank;
extern BOOT_ARGUMENT *g_boot_arg;
extern BI_DRAM bi_dram[MAX_NR_BANK];
extern int get_meta_port_id(void);
extern unsigned int g_fb_base;
extern unsigned int g_fb_size;

unsigned *target_atag_nand_data(unsigned *ptr)
{
	return ptr;
}

unsigned *target_atag_partition_data(unsigned *ptr)
{
	return ptr;
}

unsigned *target_atag_boot(unsigned *ptr)
{
	*ptr++ = tag_size(tag_boot);
	*ptr++ = ATAG_BOOT;
	*ptr++ = g_boot_mode;

	return ptr;
}

#if defined(MTK_DLPT_SUPPORT)
extern kal_uint8 imix_r;
unsigned *target_atag_imix_r(unsigned *ptr)
{
	/* *ptr++ = tag_size(tag_imix_r); */
	/* *ptr++ = ATAG_IMIX; */
	*ptr++ = imix_r;

	dprintf(CRITICAL, "target_atag_imix_r:%d\n", imix_r);
	return ptr;
}
#endif

unsigned *target_atag_devinfo_data(unsigned *ptr)
{
	int i = 0;
	*ptr++ = tag_size(tag_devinfo_data);
	*ptr++ = ATAG_DEVINFO_DATA;
	for (i = 0; i < ATAG_DEVINFO_DATA_SIZE; i++) {
		*ptr++ = get_devinfo_with_index(i);
	}
	*ptr++ = ATAG_DEVINFO_DATA_SIZE;

	dprintf(INFO, "SSSS:0x%x\n", get_devinfo_with_index(1));
	dprintf(INFO, "SSSS:0x%x\n", get_devinfo_with_index(2));
	dprintf(INFO, "SSSS:0x%x\n", get_devinfo_with_index(3));
	dprintf(INFO, "SSSS:0x%x\n", get_devinfo_with_index(4));
	dprintf(INFO, "SSSS:0x%x\n", get_devinfo_with_index(20));
	dprintf(INFO, "SSSS:0x%x\n", get_devinfo_with_index(21));

	return ptr;
}

unsigned *target_atag_masp_data(unsigned *ptr)
{
	/*tag size*/
	*ptr++ = tag_size(tag_masp_data);
	/*tag name*/
	*ptr++ = ATAG_MASP_DATA;
	ptr = fill_atag_masp_data(ptr);

	return ptr;
}

unsigned *target_atag_mem(unsigned *ptr)
{
	int i;

	for (i = 0; i < g_nr_bank; i++) {
#ifndef MTK_LM_MODE
		*ptr++ = 4; /* tag size */
		*ptr++ = ATAG_MEM; /* tag name */
		*ptr++ = bi_dram[i].size;
		*ptr++ = bi_dram[i].start;
#else
		*ptr++ = 6; /* tag size */
		*ptr++ = ATAG_MEM64; /* tag name */
		unsigned long long *ptr64 = (unsigned long long *)ptr;
		*ptr64++ = bi_dram[i].size;
		*ptr64++ = bi_dram[i].start;
		ptr = (unsigned int *)ptr64;
#endif
	}
	return ptr;
}

unsigned *target_atag_meta(unsigned *ptr)
{
	*ptr++ = tag_size(tag_meta_com);
	*ptr++ = ATAG_META_COM;
	*ptr++ = g_boot_arg->meta_com_type;
	*ptr++ = g_boot_arg->meta_com_id;
	*ptr++ = get_meta_port_id();
	dprintf(CRITICAL, "meta com type = %d\n", g_boot_arg->meta_com_type);
	dprintf(CRITICAL, "meta com id = %d\n", g_boot_arg->meta_com_id);
	dprintf(CRITICAL, "meta uart port = %d\n", get_meta_port_id());
	return ptr;
}

/* todo: give lk strtoul and nuke this */
static unsigned hex2unsigned(const char *x)
{
	unsigned n = 0;

	while (*x) {
		switch (*x) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				n = (n << 4) | (*x - '0');
				break;
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
				n = (n << 4) | (*x - 'a' + 10);
				break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
				n = (n << 4) | (*x - 'A' + 10);
				break;
			default:
				return n;
		}
		x++;
	}

	return n;
}

#if 0
#define ATAG_DFO_DATA 0x41000805
unsigned *target_atag_dfo(unsigned *ptr)
{
	int i, j;
	dfo_boot_info *dfo_p;
	char tmp[11];
	unsigned char *buffer;

	*ptr++ = ((sizeof(struct tag_header) + DFO_BOOT_COUNT * sizeof(dfo_boot_info)) >> 2);
	*ptr++ = ATAG_DFO_DATA;

	memcpy((void *)ptr, (void *)dfo_boot_default, DFO_BOOT_COUNT * sizeof(dfo_boot_info));
	dfo_p = (dfo_boot_info *) ptr;

	ptr += DFO_BOOT_COUNT * sizeof(dfo_boot_info) >> 2;

	buffer = (unsigned char *)get_env("DFO");

	if (buffer != NULL) {

		for (i = 0; i < DFO_BOOT_COUNT; i++) {
			j = 0;
			do {
				dfo_p[i].name[j] = *buffer;
				j++;
			} while (*buffer++ != ',' && j < 31);

			dfo_p[i].name[j - 1] = '\0';
			j = 0;

			do {
				tmp[j] = *buffer;
				j++;
			} while (*buffer++ != ',' && j < 10);

			tmp[j] = '\0';

			if ((strncmp("0x", tmp, 2) == 0) || (strncmp("0X", tmp, 2) == 0))
				dfo_p[i].value = hex2unsigned(&tmp[2]);
			else
				dfo_p[i].value = atoi(tmp);
		}

		for (i = 0; i < DFO_BOOT_COUNT; i++)
			dprintf(INFO, "[DFO-%d] NAME:%s, Value:%lu\n", i, dfo_p[i].name,
			        dfo_p[i].value);

	} else
		dprintf(INFO, "No DFO. Use default values.\n");

	return ptr;
}
#endif

unsigned *target_atag_commmandline(unsigned *ptr, char *commandline)
{
	char *p;

	if (!commandline)
		return NULL;

	for (p = commandline; *p == ' '; p++);

	if (*p == '\0')
		return NULL;

	*ptr++ = (sizeof(struct tag_header) + strlen(p) + 1 + 4) >> 2;; /* size */
	*ptr++ = ATAG_CMDLINE;
	strcpy((char *)ptr, p); /* copy to atags memory region */
	ptr += (strlen(p) + 1 + 4) >> 2;
	return ptr;
}

unsigned *target_atag_initrd(unsigned *ptr, ulong initrd_start, ulong initrd_size)
{
	*ptr++ = tag_size(tag_initrd);
	*ptr++ = ATAG_INITRD2;
	/* TMP for bring up testing */
	/* *ptr++ = CFG_RAMDISK_LOAD_ADDR; */
	/* *ptr++ = 0x1072F9; */

	*ptr++ = initrd_start;
	*ptr++ = initrd_size;
	return ptr;
}

unsigned *target_atag_videolfb(unsigned *ptr)
{
	*ptr++ = tag_size (tag_videolfb);
	*ptr++ = ATAG_VIDEOLFB;
	/*Frambuffer Base. TBD*/
	*(ptr+2) = g_fb_base;
	/*Frambuffer Size. TBD*/
	*(ptr+3) = g_fb_size;
	ptr += (sizeof(struct tag_videolfb)>>2);
	return ptr;
}


unsigned *target_atag_mdinfo(unsigned *ptr)
{
	unsigned char *p;
	*ptr++ = tag_size(tag_mdinfo_data);
	*ptr++ = ATAG_MDINFO_DATA;
	p = (unsigned char *)ptr;
	*p++ = g_boot_arg->md_type[0];
	*p++ = g_boot_arg->md_type[1];
	*p++ = g_boot_arg->md_type[2];
	*p++ = g_boot_arg->md_type[3];
	return (unsigned *)p;
}


void *target_get_scratch_address(void)
{
	return ((void *)SCRATCH_ADDR);
}

#define NUM_CLUSTERS  2
#define NUM_CORES_CLUSTER0  4
#define NUM_CORES_CLUSTER1  2
/*
 * 0000: Qual core
 * 1100: Dual core
 */
#define DEVINFO_4CPU_QUAL_CORE   0x0
#define DEVINFO_4CPU_DUAL_CORE   0xC

struct cpu_dev_info {
	unsigned int reserve3 : 12;
	unsigned int cluster1 : 4;
	unsigned int reserve2 : 8;
	unsigned int cluster0 : 4;
	unsigned int reserve1 : 4;
};

int dev_info_nr_cpu(void)
{
	unsigned int devinfo = get_devinfo_with_index(3);
	struct cpu_dev_info *info = (struct cpu_dev_info *)&devinfo;
	int cluster[NUM_CLUSTERS];
	int cluster_idx;
	int nr_cpu = 2;

	memset(cluster, 0, sizeof(cluster));


	cluster[0] = info->cluster0;

	switch (cluster[0]) {
		case DEVINFO_4CPU_QUAL_CORE:
			nr_cpu += 4;
			break;
		case DEVINFO_4CPU_DUAL_CORE:
			nr_cpu += 2;
			break;
		default:
			break;
	}


	return nr_cpu;
}

#define MAX_MODEL_NUM  1
int target_fdt_model(void *fdt)
{
	unsigned int devinfo = ((get_devinfo_with_index(3) >> 24) & 0xF);
	int code = 0;
	int code_max = MAX_MODEL_NUM;
	int len;
	int nodeoffset;
	const struct fdt_property *prop;
	char *prop_name = "model";
	const char *model_name[] = {
		"MT8176",
		"MT8173",
	};

	if (devinfo)
		code = 1;

	if (code > code_max)
		return -1;

	/* Becuase the model is at the begin of device tree.
	 * use nodeoffset=0
	 */
	nodeoffset = 0;
	prop = fdt_get_property(fdt, nodeoffset, prop_name, &len);

	if (prop) {
		int namestroff;
		const char *str = model_name[code];
		fdt_setprop_string(fdt, nodeoffset, prop_name, str);
		prop = fdt_get_property(fdt, nodeoffset, prop_name, &len);
		namestroff = fdt32_to_cpu(prop->nameoff);
		dprintf(CRITICAL, "%s=%s\n", fdt_string(fdt, namestroff), (char *)prop->data);
	}

	return 0;
}

#ifndef FIX_CPU_CORE_NUM
int target_fdt_cpus(void *fdt)
{
	int cpus_offset, cpu_node, last_node = -1;
	int len;
	const struct fdt_property *prop;
	unsigned int *data;
	unsigned int reg, clk_freq;
	unsigned int cluster_idx;
	unsigned int core_num;
	unsigned int activated_cores[NUM_CLUSTERS] = {0};
	unsigned int available_cores[NUM_CLUSTERS] = {NUM_CORES_CLUSTER0, NUM_CORES_CLUSTER1};
	unsigned int max_clk_freq[NUM_CLUSTERS] = {0};
	unsigned int devinfo = get_devinfo_with_index(3);
	struct cpu_dev_info *info = (struct cpu_dev_info *)&devinfo;

	dprintf(INFO, "info->cluster0=0x%x\n", info->cluster0);
	dprintf(INFO, "info->cluster1=0x%x\n", info->cluster1);

	switch ((unsigned int)info->cluster0) {
		case DEVINFO_4CPU_QUAL_CORE:
			available_cores[0] = 4;
			break;
		case DEVINFO_4CPU_DUAL_CORE:
			available_cores[0] = 2;
			break;
		default:
			break;
	}

	switch ((unsigned int)info->cluster1) {
		default:
			available_cores[1] = 2;
			break;
	}

	cpus_offset = fdt_path_offset(fdt, "/cpus");
	if (cpus_offset < 0) {
		dprintf(CRITICAL, "couldn't find /cpus\n");
		return cpus_offset;
	}

	for (cluster_idx = 0; cluster_idx < NUM_CLUSTERS; cluster_idx++)
		max_clk_freq[cluster_idx] = 1000000000;

	for (cpu_node = fdt_first_subnode(fdt, cpus_offset); cpu_node >= 0;
	        cpu_node = ((last_node >= 0) ? fdt_next_subnode(fdt, last_node) : fdt_first_subnode(fdt, cpus_offset))) {
		prop = fdt_get_property(fdt, cpu_node, "device_type", &len);
		if ((!prop) || (len < 4) || (strcmp(prop->data, "cpu"))) {
			last_node = cpu_node;
			continue;
		}

		prop = fdt_get_property(fdt, cpu_node, "reg", &len);

		data = (uint32_t *)prop->data;
		reg = fdt32_to_cpu(*data);

		dprintf(INFO, "reg = 0x%x\n", reg);
		core_num = reg & 0xFF;
		cluster_idx = (reg & 0x100) >> 8;
		dprintf(INFO, "cluster_idx=%d, core_num=%d\n", cluster_idx, core_num);

		if (core_num >= available_cores[cluster_idx]) {
			dprintf(INFO, "delete: cluster = %d, core = %d\n", cluster_idx, core_num);
			fdt_del_node(fdt, cpu_node);
		}
		/* ========== */
		else {
			activated_cores[cluster_idx]++;

			prop = fdt_get_property(fdt, cpu_node, "clock-frequency", &len);
			data = (uint32_t *)prop->data;
			clk_freq = fdt32_to_cpu(*data);

			dprintf(INFO, "cluster_idx=%d, core_num=%d, clock-frequency = %u => %u\n",
			        cluster_idx, core_num, clk_freq, max_clk_freq[cluster_idx]);

			if (clk_freq > max_clk_freq[cluster_idx]) {
				dprintf(INFO, "setprop: clock-frequency = %u => %u\n", clk_freq, max_clk_freq[cluster_idx]);
				fdt_setprop_cell(fdt, cpu_node, "clock-frequency", max_clk_freq[cluster_idx]);
			}

			last_node = cpu_node;
		}

		if (cluster_idx == NUM_CLUSTERS) {
			dprintf(CRITICAL, "Warning: unknown cpu type in device tree\n");
			last_node = cpu_node;
		}
	}

	for (cluster_idx = 0; cluster_idx < NUM_CLUSTERS; cluster_idx++) {
		if (activated_cores[cluster_idx] > available_cores[cluster_idx])
			dprintf(CRITICAL, "Warning: unexpected reg value in device tree\n");
		dprintf(CRITICAL, "cluster-%d: %d core\n", cluster_idx, available_cores[cluster_idx]);
	}

	return 0;
}
#endif

/********************************************************************
 * mblock_info is not set here. For correct mem_reg_property without
 * sub fb_size, we do the same thing as mt_boot with full size.
 *******************************************************************/
int setup_mem_property_use_mblock_info(dt_dram_info *property, size_t p_size)
{
	dt_dram_info *p;
	unsigned int i;

	for (i = 0; i < g_nr_bank; ++i) {
		p = (property + i);
		dprintf(INFO, " p=0x%08X\n", p);

		p->start_hi = cpu_to_fdt32(bi_dram[i].start>>32);
		p->start_lo = cpu_to_fdt32(bi_dram[i].start);
		p->size_hi = cpu_to_fdt32(bi_dram[i].size>>32);
		p->size_lo = cpu_to_fdt32(bi_dram[i].size);

		dprintf(INFO, " mem_reg_property[%d].start_hi = 0x%08X\n", i, p->start_hi);
		dprintf(INFO, " mem_reg_property[%d].start_lo = 0x%08X\n", i, p->start_lo);
		dprintf(INFO, " mem_reg_property[%d].size_hi = 0x%08X\n", i, p->size_hi);
		dprintf(INFO, " mem_reg_property[%d].size_lo = 0x%08X\n", i, p->size_lo);
	}

	return 0;
}

#if defined(MTK_SHARED_SECURE_POOL_SUPPORT)
int secure_pool_dtb_append(void *fdt)
{
	int offset, ret = 0;
	int nodeoffset = 0;
	unsigned int securepool_startaddr = 0;
	unsigned int securepool_reserved_mem[4] = {0};
	unsigned int securepool_alignment[2] = {0, cpu_to_fdt32(0x400000)};
	unsigned int *prop;
	int prop_len;

	offset = fdt_path_offset(fdt, "/reserved-memory");
	nodeoffset = fdt_subnode_offset(fdt, offset, "secure-carveout");
	if (nodeoffset >= 0) {
		/* secure-carveout node is already existed. */
		prop = fdt_getprop(fdt, nodeoffset, "reg", &prop_len);
		if (prop != NULL && prop_len >= sizeof(unsigned int)*4) {
			if ((MTK_SHARED_SECURE_POOL_SIZE) != fdt32_to_cpu(prop[3])) {
				dprintf(CRITICAL, "Warning: secure-carveout reg not matched! got 0x%x, but 0x%x expected\n", fdt32_to_cpu(prop[3]), (MTK_SHARED_SECURE_POOL_SIZE));
				return 1;
			}
		}
		prop = fdt_getprop(fdt, nodeoffset, "size", &prop_len);
		if (prop != NULL && prop_len >= sizeof(unsigned int)*2) {
			if ((MTK_SHARED_SECURE_POOL_SIZE) != fdt32_to_cpu(prop[1])) {
				dprintf(CRITICAL, "Warning: secure-carveout size not matched! got 0x%x, but 0x%x expected\n", fdt32_to_cpu(prop[1]), (MTK_SHARED_SECURE_POOL_SIZE));
				return 1;
			}
		}
		return 0;
	}
	nodeoffset = fdt_add_subnode(fdt, offset, "secure-carveout");
	if (nodeoffset < 0) {
		dprintf(CRITICAL, "Warning: can't add shared-carveout node in device tree\n");
		return 1;
	}
	ret = fdt_setprop_string(fdt, nodeoffset, "compatible", "shared-secure-pool");
	if (ret) {
		dprintf(CRITICAL, "Warning: can't add shared-secure-pool compatible property in device tree\n");
		return 1;
	}
	ret = fdt_setprop(fdt, nodeoffset, "reusable", NULL, 0);
	if (ret) {
		dprintf(CRITICAL, "Warning: can't add secure-carveout reusable property in device tree\n");
		return 1;
	}
	securepool_reserved_mem[0] = 0;
	if ((g_fb_base + g_fb_size) == g_boot_arg->tee_reserved_mem.start) /* fb before tee */
		securepool_startaddr = (g_fb_base-(MTK_SHARED_SECURE_POOL_SIZE)) & ~(0x400000-1);
	else
		securepool_startaddr = (g_boot_arg->tee_reserved_mem.start-(MTK_SHARED_SECURE_POOL_SIZE)) & ~(0x400000-1);
	securepool_reserved_mem[1] = (u32)cpu_to_fdt32(securepool_startaddr);
	securepool_reserved_mem[2] = 0;
	securepool_reserved_mem[3] = (u32)cpu_to_fdt32(MTK_SHARED_SECURE_POOL_SIZE);
	ret = fdt_setprop(fdt, nodeoffset, "reg", securepool_reserved_mem, sizeof(unsigned int)*4);
	if (ret) {
		dprintf(CRITICAL, "Warning: can't add secure-carveout reg property in device tree\n");
		return 1;
	}
	ret = fdt_setprop(fdt, nodeoffset, "alignment", securepool_alignment, sizeof(unsigned int)*2);
	if (ret) {
		dprintf(CRITICAL, "Warning: can't add secure-carveout alignment property in device tree\n");
		return 1;
	}
	dprintf(CRITICAL,"secure-carveout is appended (0x%llx, 0x%llx)\n",
		securepool_startaddr, (MTK_SHARED_SECURE_POOL_SIZE));

	return ret;
}
#endif  /* MTK_SHARED_SECURE_POOL_SUPPORT */


int platform_atag_append(void *fdt)
{
	char *ptr;
	int offset;
	int ret = 0;
	int nodeoffset;
	dt_dram_info rsv_mem_reg_property;

	offset = fdt_path_offset(fdt, "/memory");

	ptr = (char *)&g_boot_arg->tee_reserved_mem;
	ret = fdt_setprop(fdt, offset, "tee_reserved_mem", ptr, sizeof(mem_desc_t));
	if (ret)
		goto exit;

	offset = fdt_path_offset(fdt, "/reserved-memory");
	if (offset < 0) {
		dprintf(CRITICAL, "Warning: can't find reserved-memory node!\n");
		ret = 1;
		goto exit;
	}

	nodeoffset = fdt_add_subnode(fdt, offset, "secos-reserved-memory");

	if (nodeoffset < 0) {
		dprintf(CRITICAL, "Warning: can't add secos-reserved-memory node!\n");
		ret = 1;
		goto exit;
	}

	ptr = (char *)&rsv_mem_reg_property;
	rsv_mem_reg_property.start_hi = cpu_to_fdt32(g_boot_arg->secos_reserved_mem.start>>32);
	rsv_mem_reg_property.start_lo = cpu_to_fdt32(g_boot_arg->secos_reserved_mem.start);
	rsv_mem_reg_property.size_hi = cpu_to_fdt32(g_boot_arg->secos_reserved_mem.size>>32);
	rsv_mem_reg_property.size_lo = cpu_to_fdt32(g_boot_arg->secos_reserved_mem.size);

	ret = fdt_setprop(fdt, nodeoffset, "reg", ptr, sizeof(dt_dram_info));
	if (ret) {
		dprintf(CRITICAL, "Warning: can't set secos-reserved-memory node reg property!\n");
		goto exit;
	}

	ret = fdt_setprop(fdt, nodeoffset, "no-map", NULL, 0);
	if (ret) {
		dprintf(CRITICAL, "Warning: can't set secos-reserved-memory node no-map property!\n");
		goto exit;
	}

	nodeoffset = fdt_add_subnode(fdt, offset, "fb-reserved-memory");

	if (nodeoffset < 0) {
		dprintf(CRITICAL, "Warning: can't add fb-reserved-memory node!\n");
		ret = 1;
		goto exit;
	}

	ptr = (char *)&rsv_mem_reg_property;
	rsv_mem_reg_property.start_hi = cpu_to_fdt32(0);
	rsv_mem_reg_property.start_lo = cpu_to_fdt32(g_fb_base);
	rsv_mem_reg_property.size_hi = cpu_to_fdt32(0);
	rsv_mem_reg_property.size_lo = cpu_to_fdt32(g_fb_size);

	ret = fdt_setprop(fdt, nodeoffset, "reg", ptr, sizeof(dt_dram_info));
	if (ret) {
		dprintf(CRITICAL, "Warning: can't set fb-reserved-memory node reg property!\n");
		goto exit;
	}

	ret = fdt_setprop(fdt, nodeoffset, "no-map", NULL, 0);
	if (ret) {
		dprintf(CRITICAL, "Warning: can't set fb-reserved-memory node no-map property!\n");
		goto exit;
	}

#if defined(MTK_SHARED_SECURE_POOL_SUPPORT)
	ret = secure_pool_dtb_append(fdt);
	if (ret) goto exit;
#endif

exit:

	if (ret)
		return 1;

	return 0;
}

