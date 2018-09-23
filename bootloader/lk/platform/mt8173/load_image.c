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

#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>

#include <libfdt.h>
#include <mmc.h>
#include <partition_parser.h>
#include <platform/mt_typedefs.h>
#include <platform/boot_mode.h>
#include <platform/mt_reg_base.h>
#include <platform/bootimg.h>
#include <platform/errno.h>
#include <printf.h>
#include <string.h>
#include <malloc.h>
#include <platform/mt_gpt.h>
#include <platform/sec_status.h>

#include <platform/mtk_key.h>
#include <target/cust_key.h>
#include <part_interface.h>
#define MODULE_NAME "LK_BOOT"
/* ************************************************************************ */

static unsigned int multi_dtb_image_offset;

#define PUBK_LEN 256
#define BOOT_STATE_GREEN   0x0
#define BOOT_STATE_ORANGE  0x1
#define BOOT_STATE_YELLOW  0x2
#define BOOT_STATE_RED     0x3

#define DEVICE_STATE_UNLOCKED 0x0
#define DEVICE_STATE_LOCKED   0x1

/* ********* */
/* * Notice : it's kernel start addr (and not include any debug header) */
unsigned int g_kmem_off = 0;

/* ********* */
/* * Notice : it's rootfs start addr (and not include any debug header) */
unsigned int g_rmem_off = 0;

unsigned int g_smem_off = 0;
unsigned int g_rcimg_sz = 0;
extern boot_img_hdr *g_boot_hdr;
unsigned int g_bimg_sz = 0;
int g_pg_sz = 0;
int g_kimg_sz = 0;
int g_rimg_sz = 0;
unsigned int g_boot_state = BOOT_STATE_GREEN;

extern unsigned int device_tree_size;
extern void mtk_wdt_restart(void);

int print_boot_state(void)
{
	int ret = 0;
	switch (g_boot_state) {
		case BOOT_STATE_ORANGE:
			dprintf(CRITICAL, "boot state: orange\n");
			break;
		case BOOT_STATE_YELLOW:
			dprintf(CRITICAL, "boot state: yellow\n");
			break;
		case BOOT_STATE_RED:
			dprintf(CRITICAL, "boot state: red\n");
			break;
		case BOOT_STATE_GREEN:
			dprintf(CRITICAL, "boot state: green\n");
			break;
		default:
			dprintf(CRITICAL, "boot state: unknown\n");
			break;
	}

	return ret;
}

int yellow_state_warning(void)
{
	const char* title_msg = "yellow state\n\n";
	unsigned char pubk[PUBK_LEN] = {0};
	int ret = 0;

	video_clean_screen();
	video_set_cursor(video_get_rows() / 2, 0);
	video_printf(title_msg);
	video_printf("Your device has loaded a different operating system\n");
	video_printf("ID:\n");

	ret = sec_get_custom_pubk(pubk, PUBK_LEN);
	if (ret) {
		video_printf("Cannot get custom public key, abort in 5 seconds\n");
		mtk_wdt_restart();
		mdelay(5000);
		mtk_wdt_restart();
		return -1;
	}
	video_printf("%x %x %x %x %x %x %x %x\n", pubk[0], pubk[1], pubk[2], pubk[3], pubk[4], pubk[5], pubk[6], pubk[7]);
	video_printf("Yes (Volume UP)   : Confirm and Boot.\n\n");
	video_printf("No  (Volume Down) : Abort.\n\n");

	while (1) {
		mtk_wdt_restart();
		if (mtk_detect_key(MT65XX_MENU_SELECT_KEY)) //VOL_UP
			return 0;
		else if (mtk_detect_key(MT65XX_MENU_OK_KEY)) //VOL_DOWN
			return -1;
		else {
			/* ignore */
		}
	}
}

int orange_state_warning(void)
{
	const char* title_msg = "Orange State\n\n";
	int ret = 0;

	video_clean_screen();
	video_set_cursor(video_get_rows() / 2, 0);
	video_printf(title_msg);
	video_printf("Your device has been unlocked and can't be trusted\n");
	video_printf("Your device will boot in 5 seconds\n");
	mtk_wdt_restart();
	mdelay(5000);
	mtk_wdt_restart();

	return 0;
}

int red_state_warning(void)
{
	const char* title_msg = "Red State\n\n";
	int ret = 0;

	video_clean_screen();
	video_set_cursor(video_get_rows() / 2, 0);
	video_printf(title_msg);
	video_printf("Your device has failed verification and may not\n");
	video_printf("work properly\n");
	video_printf("Your device will boot in 5 seconds\n");
	mtk_wdt_restart();
	mdelay(5000);
	mtk_wdt_restart();

	return -1;
}

int show_warning(void)
{
	int ret = 0;
	switch (g_boot_state) {
		case BOOT_STATE_ORANGE:
			ret = orange_state_warning();
			break;
		case BOOT_STATE_YELLOW:
#ifdef MTK_SECURITY_YELLOW_STATE_SUPPORT
			ret = yellow_state_warning();
			if (0 == ret) /* user confirms to boot into yellow state */
				break;
			/* fall into red state if user refuses to enter yellow state */
#else
			ret = -1;
			/* fall into red state since yellow state is not supported */
#endif
		case BOOT_STATE_RED:
			ret = red_state_warning();
			ret = -1; /* return error */
			break;
		case BOOT_STATE_GREEN:
		default:
			break;
	}

	return ret;
}

int set_boot_state_to_cmdline()
{
	int ret = 0;

	switch (g_boot_state) {
		case BOOT_STATE_ORANGE:
			cmdline_append("androidboot.verifiedbootstate=orange");
			break;
		case BOOT_STATE_YELLOW:
			cmdline_append("androidboot.verifiedbootstate=yellow");
			break;
		case BOOT_STATE_RED:
			cmdline_append("androidboot.verifiedbootstate=red");
			break;
		case BOOT_STATE_GREEN:
			cmdline_append("androidboot.verifiedbootstate=green");
			break;
		default:
			break;
	}

	return ret;
}

int verified_boot_flow(char *part_name, unsigned char *target)
{
	int ret = 0;
	unsigned int img_vfy_time = 0;
	int lock_state = 0;

	/* if MTK_SECURITY_SW_SUPPORT is not defined, boot state is always green */
#ifdef MTK_SECURITY_SW_SUPPORT
	/* please refer to the following website for verified boot flow */
	/* http://source.android.com/devices/tech/security/verifiedboot/verified-boot.html */
	ret = sec_query_device_lock(&lock_state);
	if (ret) {
		g_boot_state = BOOT_STATE_RED;
		goto _end;
	}

	if (DEVICE_STATE_LOCKED == lock_state) {
		if (g_boot_state == BOOT_STATE_GREEN)
			goto _end;

		sec_clear_pubk();
		g_boot_state = BOOT_STATE_GREEN;
		img_vfy_time = get_timer(0);

		ret = android_verified_boot(part_name, target);
		if (0 == ret)
			g_boot_state = BOOT_STATE_GREEN;
		else if (ERR_NOT_OEM_KEY == ret)
			g_boot_state = BOOT_STATE_YELLOW;
		else {
			g_boot_state = BOOT_STATE_RED;
			sec_clear_pubk();
		}
		dprintf(INFO, "[SBC] img vfy(%d ms)\n", (unsigned int)get_timer(img_vfy_time));
	}
	else if (DEVICE_STATE_UNLOCKED == lock_state) {
		g_boot_state = BOOT_STATE_ORANGE;
	}
	else {/* unknown lock state*/
		g_boot_state = BOOT_STATE_RED;
	}
#endif //MTK_SECURITY_SW_SUPPORT

_end:
	ret = print_boot_state();
	if (ret)
		return ret;

	ret = show_warning();
	if (ret)
		return ret;

	ret = set_boot_state_to_cmdline();
	if (ret)
		return ret;

	return ret;
}

#if 1

static int mboot_common_load_part_info(part_dev_t *dev, char *part_name, part_hdr_t *part_hdr)
{
	long len;
	unsigned long long addr;
	int part;

	dev = mt_part_get_device();

	if (!dev)
		return -ENODEV;
	part = partition_get_index(part_name);
	addr = partition_get_offset(part);

	/* *************** */
	/* * read partition header */
	/* * */
	len = partition_read(part_name, 0, (uchar*)part_hdr, sizeof(part_hdr_t));

	if (len < 0) {
		dprintf(CRITICAL, "[%s] %s partition read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -1;
	}

	dprintf(CRITICAL, "\n=========================================\n");
	dprintf(CRITICAL, "[%s] %s magic number : 0x%x\n", MODULE_NAME, part_name, part_hdr->info.magic);
	part_hdr->info.name[31] = '\0'; /* append end char */
	dprintf(CRITICAL, "[%s] %s name         : %s\n", MODULE_NAME, part_name, part_hdr->info.name);
	dprintf(CRITICAL, "[%s] %s size         : %d\n", MODULE_NAME, part_name, part_hdr->info.dsize);
	dprintf(CRITICAL, "=========================================\n");

	/* *************** */
	/* * check partition magic */
	/* * */
	if (part_hdr->info.magic != PART_MAGIC) {
		dprintf(CRITICAL, "[%s] %s partition magic error\n", MODULE_NAME, part_name);
		return -1;
	}
	/* *************** */
	/* * check partition name */
	/* * */
	if (strncmp(part_hdr->info.name, part_name, sizeof(part_hdr->info.name))) {
		dprintf(CRITICAL, "[%s] %s partition name error\n", MODULE_NAME, part_name);
		return -1;
	}
	/* *************** */
	/* * check partition data size */
	/* * */
	if (part_hdr->info.dsize > partition_get_size(part)) {
		dprintf(CRITICAL, "[%s] %s partition size error\n", MODULE_NAME, part_name);
		return -1;
	}

	return 0;
}

/**********************************************************
 * Routine: mboot_common_load_part
 *
 * Description: common function for loading image from nand flash
 *              this function is called by
 *                  (1) 'mboot_common_load_logo' to display logo
 *
 **********************************************************/
int mboot_common_load_part(char *part_name, unsigned long addr)
{
	long len;
	unsigned long long start_addr = 0;
	int part;
	part_dev_t *dev;
	part_hdr_t *part_hdr;

	dev = mt_part_get_device();
	if (!dev) {
		return -ENODEV;
	}

	part = partition_get_index(part_name);
	if (part == INVALID_PTN) {
		return -ENOENT;
	}

	start_addr = partition_get_offset(part);

	part_hdr = (part_hdr_t *) malloc(sizeof(part_hdr_t));

	if (!part_hdr) {
		return -ENOMEM;
	}

	len = mboot_common_load_part_info(dev, part_name, part_hdr);
	if (len < 0) {
		len = -EINVAL;
		goto exit;
	}
	/* **************** */
	/* * read image data */
	/* * */
	dprintf(CRITICAL, "read the data of %s\n", part_name);

	len = partition_read(part_name, sizeof(part_hdr_t), (uchar*)addr, (size_t)part_hdr->info.dsize);
	if (len < 0) {
		len = -EIO;
		goto exit;
	}

exit:
	if (part_hdr)
		free(part_hdr);

	return len;
}

/**********************************************************
 * Routine: mboot_common_load_logo
 *
 * Description: function to load logo to display
 *
 **********************************************************/
int mboot_common_load_logo(unsigned long logo_addr, char *filename)
{
	int ret;
#if (CONFIG_COMMANDS & CFG_CMD_FAT)
	long len;
#endif

#if (CONFIG_COMMANDS & CFG_CMD_FAT)
	len = file_fat_read(filename, (unsigned char *)logo_addr, 0);

	if (len > 0)
		return (int)len;
#endif

	ret = mboot_common_load_part(PART_LOGO, logo_addr);

	return ret;
}

/**********************************************************
 * Routine: mboot_android_check_img_info
 *
 * Description: this function is called to
 *              (1) check the header of kernel / rootfs
 *
 * Notice : this function will be called by 'mboot_android_check_bootimg_hdr'
 *
 **********************************************************/
int mboot_android_check_img_info(char *part_name, part_hdr_t *part_hdr)
{
	dprintf(CRITICAL, "\n=========================================\n");
	dprintf(CRITICAL, "[%s] %s magic number : 0x%x\n", MODULE_NAME, part_name, part_hdr->info.magic);
	dprintf(CRITICAL, "[%s] %s size         : 0x%x\n", MODULE_NAME, part_name, part_hdr->info.dsize);
	dprintf(CRITICAL, "=========================================\n");

	/* *************** */
	/* * check partition magic */
	/* * */
	if (part_hdr->info.magic != PART_MAGIC) {
		dprintf(CRITICAL, "[%s] %s partition magic error\n", MODULE_NAME, part_name);
		return -1;
	}
	/* *************** */
	/* * check partition name */
	/* * */
	if (strncmp(part_hdr->info.name, part_name, sizeof(part_hdr->info.name))) {
		dprintf(CRITICAL, "[%s] %s partition name error\n", MODULE_NAME, part_name);
		return -1;
	}
	/* *************** */
	/* * return the image size */
	/* * */
	return part_hdr->info.dsize;
}

/**********************************************************
 * Routine: mboot_android_check_bootimg_hdr
 *
 * Description: this function is called to
 *              (1) 'read' the header of boot image from nand flash
 *              (2) 'parse' the header of boot image to obtain
 *                  - (a) kernel image size
 *                  - (b) rootfs image size
 *                  - (c) rootfs offset
 *
 * Notice : this function must be read first when doing nand / msdc boot
 *
 **********************************************************/
static int mboot_android_check_bootimg_hdr(part_dev_t *dev, char *part_name, boot_img_hdr *boot_hdr)
{
	long len;
	int ret = 0;
	unsigned long long addr;
	int part;

	dev = mt_part_get_device();

	if (!dev)
		return -ENODEV;
	part = partition_get_index(part_name);
	addr = partition_get_offset(part);

	/* *************** */
	/* * read partition header */
	/* * */

	dprintf(CRITICAL, "part page addr is 0x%llx\n", addr);

	len = partition_read(part_name, 0, (uchar*)boot_hdr, sizeof(boot_img_hdr));
	if (len < 0) {
		dprintf(CRITICAL, "[%s] %s boot image header read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -1;
	}
	/* *************** */
	/* * check partition magic */
	/* * */
	if (strncmp((char const *)boot_hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE) != 0) {
		dprintf(CRITICAL, "[%s] boot image header magic error\n", MODULE_NAME);
		if (fdt_check_header(boot_hdr->magic) == 0) {
			int next;
			int offset = fdt_path_offset(boot_hdr, "/images/kernel@1");
			g_bimg_sz = fdt32_to_cpu(((struct fdt_header *)boot_hdr->magic)->totalsize);
			fdt_next_tag(boot_hdr, offset, &next);
			fdt_next_tag(boot_hdr, next, &offset);
			multi_dtb_image_offset = fdt_off_dt_struct(boot_hdr) + offset + sizeof(struct fdt_property);
			return 0;
		}
		return -1;
	}

	dprintf(CRITICAL, "\n============================================================\n");
	boot_hdr->magic[7] = '\0';
	dprintf(CRITICAL, "[%s] Android Partition Name		        : %s\n", MODULE_NAME, part_name);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Magic	        : %s\n", MODULE_NAME, boot_hdr->magic);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Kernel Size	: 0x%x\n", MODULE_NAME, boot_hdr->kernel_size);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Rootfs Size	: 0x%x\n", MODULE_NAME, boot_hdr->ramdisk_size);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Page Size	: 0x%x\n", MODULE_NAME, boot_hdr->page_size);
	dprintf(CRITICAL, "============================================================\n");

	/* *************** */
	/* * follow bootimg.h to calculate the location of rootfs */
	/* * */
	if (len != -1) {
		unsigned int k_pg_cnt = 0;
		unsigned int r_pg_cnt = 0, s_pg_cnt = 0;

		if (g_is_64bit_kernel) {
			g_kmem_off = target_get_scratch_address();
		} else
			g_kmem_off = CFG_BOOTIMG_LOAD_ADDR;

		k_pg_cnt = (boot_hdr->kernel_size + boot_hdr->page_size - 1) / boot_hdr->page_size;

		r_pg_cnt = (boot_hdr->ramdisk_size + boot_hdr->page_size - 1) / boot_hdr->page_size;

		dprintf(CRITICAL, " > page count of kernel image = %d\n", k_pg_cnt);
		g_rmem_off = g_kmem_off + k_pg_cnt * boot_hdr->page_size;
		if (boot_hdr->unused)
			g_smem_off = g_rmem_off + r_pg_cnt * boot_hdr->page_size;

		if (boot_hdr->second_size) {
			device_tree_size = boot_hdr->second_size;
			g_smem_off = g_rmem_off + r_pg_cnt * boot_hdr->page_size;
			s_pg_cnt = (boot_hdr->second_size + boot_hdr->page_size - 1) / boot_hdr->page_size;
		}

		dprintf(CRITICAL, " > kernel mem offset = 0x%x\n", g_kmem_off);
		dprintf(CRITICAL, " > rootfs mem offset = 0x%x\n", g_rmem_off);
		dprintf(CRITICAL, " > dtb mem offset = 0x%x\n", g_smem_off);

		/* *************** */
		/* * specify boot image size */
		/* * */
		/* g_bimg_sz = PART_BLKS_BOOTIMIG * BLK_SIZE; */
		/* Amazon change: change 1 -> 2 to accomodate our signature */
		g_bimg_sz = (k_pg_cnt + r_pg_cnt + s_pg_cnt + 2) * boot_hdr->page_size;
		g_pg_sz = boot_hdr->page_size;

		dprintf(CRITICAL, " > boot image size = 0x%x\n", g_bimg_sz);

		ret = verified_boot_flow("boot", "/boot");
		if (ret)
			len = -1;
	}

	return 0;
}

/**********************************************************
 * Routine: mboot_android_check_recoveryimg_hdr
 *
 * Description: this function is called to
 *              (1) 'read' the header of boot image from nand flash
 *              (2) 'parse' the header of boot image to obtain
 *                  - (a) kernel image size
 *                  - (b) rootfs image size
 *                  - (c) rootfs offset
 *
 * Notice : this function must be read first when doing nand / msdc boot
 *
 **********************************************************/
static int mboot_android_check_recoveryimg_hdr(part_dev_t *dev, char *part_name, boot_img_hdr *boot_hdr)
{
	long len;
	int ret = 0;
	unsigned long long addr;
	int part;

	part = partition_get_index(part_name);
	addr = partition_get_offset(part);

	/* *************** */
	/* * read partition header */
	/* * */

	dprintf(CRITICAL, "part page addr is 0x%llx\n", addr);

	len = partition_read(part_name, 0, (uchar*)boot_hdr, sizeof(boot_img_hdr));
	if (len < 0) {
		dprintf(CRITICAL, "[%s] %s Recovery image header read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -1;
	}
	/* *************** */
	/* * check partition magic */
	/* * */
	if (strncmp((char const *)boot_hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE) != 0) {
		dprintf(CRITICAL, "[%s] boot image header magic error\n", MODULE_NAME);
		if (fdt_check_header(boot_hdr->magic) == 0) {
			int next;
			int offset = fdt_path_offset(boot_hdr, "/images/kernel@1");
			g_bimg_sz = fdt32_to_cpu(((struct fdt_header *)boot_hdr->magic)->totalsize);
			fdt_next_tag(boot_hdr, offset, &next);
			fdt_next_tag(boot_hdr, next, &offset);
			multi_dtb_image_offset = fdt_off_dt_struct(boot_hdr) + offset + sizeof(struct fdt_property);
			return 0;
		}
		return -1;
	}

	dprintf(CRITICAL, "\n============================================================\n");
	boot_hdr->magic[7] = '\0';
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Magic          : %s\n", MODULE_NAME, boot_hdr->magic);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Kernel Size    : 0x%08X\n", MODULE_NAME, boot_hdr->kernel_size);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Kernel Address : 0x%08X\n", MODULE_NAME, boot_hdr->kernel_addr);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Rootfs Size    : 0x%08X\n", MODULE_NAME, boot_hdr->ramdisk_size);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Rootfs Address : 0x%08X\n", MODULE_NAME, boot_hdr->ramdisk_addr);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Tags Address   : 0x%08X\n", MODULE_NAME, boot_hdr->tags_addr);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Page Size      : 0x%08X\n", MODULE_NAME, boot_hdr->page_size);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Command Line   : %s\n", MODULE_NAME, boot_hdr->cmdline);
	dprintf(CRITICAL, "============================================================\n");

	/* *************** */
	/* * follow bootimg.h to calculate the location of rootfs */
	/* * */
	if (len != -1) {
		unsigned int k_pg_cnt = 0;
		unsigned int r_pg_cnt = 0, s_pg_cnt = 0;

		if (g_is_64bit_kernel) {
			g_kmem_off = target_get_scratch_address();
		} else {
			g_kmem_off = CFG_BOOTIMG_LOAD_ADDR;
		}

		k_pg_cnt = (boot_hdr->kernel_size + boot_hdr->page_size - 1) / boot_hdr->page_size;

		r_pg_cnt = (boot_hdr->ramdisk_size + boot_hdr->page_size - 1) / boot_hdr->page_size;

		dprintf(CRITICAL, " > page count of kernel image = %d\n", k_pg_cnt);
		g_rmem_off = g_kmem_off + k_pg_cnt * boot_hdr->page_size;
		if (boot_hdr->unused)
			g_smem_off = g_rmem_off + r_pg_cnt * boot_hdr->page_size;

		if (boot_hdr->second_size) {
			device_tree_size = boot_hdr->second_size;
			g_smem_off = g_rmem_off + r_pg_cnt * boot_hdr->page_size;
			s_pg_cnt = (boot_hdr->second_size + boot_hdr->page_size - 1) / boot_hdr->page_size;
		}

		dprintf(CRITICAL, " > kernel mem offset = 0x%x\n", g_kmem_off);
		dprintf(CRITICAL, " > rootfs mem offset = 0x%x\n", g_rmem_off);
		dprintf(CRITICAL, " > dtb mem offset = 0x%x\n", g_smem_off);

		/* *************** */
		/* * specify boot image size */
		/* * */
		/* g_rcimg_sz = partition_get_offset(part); */
		/* change 1 -> 2 to accomodate our signature */
		g_rcimg_sz = (k_pg_cnt + r_pg_cnt + s_pg_cnt + 2) * boot_hdr->page_size;
		g_pg_sz = boot_hdr->page_size;

		dprintf(CRITICAL, " > Recovery image size = 0x%x\n", g_rcimg_sz);
		ret = verified_boot_flow("recovery", "/recovery");
		if (ret)
			len = -1;
	}

	return 0;
}

/**********************************************************
 * Routine: mboot_android_load_bootimg_hdr
 *
 * Description: this is the entry function to handle boot image header
 *
 **********************************************************/
int mboot_android_load_bootimg_hdr(char *part_name, unsigned long addr)
{
	long len;
	part_dev_t *dev;
	boot_img_hdr *boot_hdr;

	dev = mt_part_get_device();
	if (!dev) {
		dprintf(CRITICAL, "mboot_android_load_bootimg_hdr, dev = NULL\n");
		return -ENODEV;
	}

	boot_hdr = (boot_img_hdr *) malloc(sizeof(boot_img_hdr));
	if (!boot_hdr) {
		dprintf(CRITICAL, "mboot_android_load_bootimg_hdr, boot_hdr = NULL\n");
		return -ENOMEM;
	}

	g_boot_hdr = boot_hdr;

	len = mboot_android_check_bootimg_hdr(dev, part_name, boot_hdr);

#ifdef SKIP_MTK_PARTITION_HEADER_CHECK
	g_rimg_sz = boot_hdr->ramdisk_size;
#endif

	return len;
}

/**********************************************************
 * Routine: mboot_android_load_recoveryimg_hdr
 *
 * Description: this is the entry function to handle Recovery image header
 *
 **********************************************************/
int mboot_android_load_recoveryimg_hdr(char *part_name, unsigned long addr)
{
	long len;
	part_dev_t *dev;
	boot_img_hdr *boot_hdr;

	dev = mt_part_get_device();
	if (!dev) {
		dprintf(CRITICAL, "mboot_android_load_recoveryimg_hdr, dev = NULL\n");
		return -ENODEV;
	}

	boot_hdr = (boot_img_hdr *) malloc(sizeof(boot_img_hdr));
	if (!boot_hdr) {
		dprintf(CRITICAL, "mboot_android_load_bootimg_hdr, boot_hdr = NULL\n");
		return -ENOMEM;
	}

	g_boot_hdr = boot_hdr;

	len = mboot_android_check_recoveryimg_hdr(dev, part_name, boot_hdr);

#ifdef SKIP_MTK_PARTITION_HEADER_CHECK
	g_rimg_sz = boot_hdr->ramdisk_size;
#endif

	return len;
}

/**********************************************************
 * Routine: mboot_android_load_recoveryimg
 *
 * Description: main function to load Android Recovery Image
 *
 **********************************************************/
int mboot_android_load_recoveryimg(char *part_name, unsigned long addr)
{
	long len = 0;
	unsigned long long start_addr;
	int part;
	part_dev_t *dev;

	dev = mt_part_get_device();
	if (!dev) {
		dprintf(CRITICAL, "mboot_android_load_bootimg , dev = NULL\n");
		return -ENODEV;
	}

	part = partition_get_index(part_name);
	if (part == INVALID_PTN) {
		dprintf(CRITICAL, "mboot_android_load_bootimg , part = NULL\n");
		return -ENOENT;
	}
	/* *************** */
	/* * not to include unused header */
	/* * */
	if (multi_dtb_image_offset != 0) {
		addr -= multi_dtb_image_offset;
		start_addr = partition_get_offset(part);
	} else
		start_addr = partition_get_offset(part) + BIMG_HEADER_SZ;
#ifndef SKIP_MTK_PARTITION_HEADER_CHECK
	addr = addr - MKIMG_HEADER_SZ;
#endif

	/* *************** */
	/* * read image data */
	/* * */
	dprintf(CRITICAL, "\nread the data of %s (size = 0x%x)\n", part_name, g_rcimg_sz);

#ifdef MTK_EMMC_SUPPORT
	dprintf(CRITICAL, " > from - 0x%016llx (skip recovery img hdr)\n", start_addr);
#else
	dprintf(CRITICAL, " > from - 0x%x (skip recovery img hdr)\n", start_addr);
#endif
	dprintf(CRITICAL, " > to   - 0x%x (starts with kernel img hdr)\n", addr);

	len = partition_read(part_name, g_boot_hdr->page_size, (uchar*)addr, (size_t)g_rcimg_sz);

	if (multi_dtb_image_offset != 0) {
		char buf[24], buf2[24];
		int next, i;
		int offset = fdt_path_offset((void *)addr, "/images/ramdisk@1");
		fdt_next_tag((void *)addr, offset, &next);
		fdt_next_tag((void *)addr, next, &offset);
		offset += addr + fdt_off_dt_struct(addr);
		g_rmem_off = (unsigned int)(((struct fdt_property *)offset)->data);
		g_rimg_sz = fdt32_to_cpu(((struct fdt_property *)offset)->len);
		sprintf(buf2, "%s.dtb", PROJECT);
		for (i = 1; i < 20; i++) {
			sprintf(buf, "%s%d", "/images/fdt@", i);
			dprintf(CRITICAL, "Try %d: %s\n", i, buf);
			offset = fdt_path_offset((void *)addr, buf);
			fdt_next_tag((void *)addr, offset, &next);
			offset = addr + next + fdt_off_dt_struct(addr);
			if (!strnicmp(buf2, ((struct fdt_property *)offset)->data, sizeof(buf2))) {
				fdt_next_tag((void *)addr, next, &offset);
				offset += addr + fdt_off_dt_struct(addr);
				g_smem_off = (unsigned int)(((struct fdt_property *)offset)->data);
				device_tree_size = fdt32_to_cpu(((struct fdt_property *)offset)->len);
				dprintf(CRITICAL, "Found: %u %u\n", g_smem_off, device_tree_size);
				break;
			}
		}
	}
#ifndef SKIP_MTK_PARTITION_HEADER_CHECK
	/* *************** */
	/* * check kernel header */
	/* * */
	g_kimg_sz = mboot_android_check_img_info(PART_KERNEL, (part_hdr_t *) (g_kmem_off - MKIMG_HEADER_SZ));
	if (g_kimg_sz == -1) {
		len = -EIO;
		goto exit;
	}
	/* *************** */
	/* * check rootfs header */
	/* * */
	g_rimg_sz = mboot_android_check_img_info(PART_ROOTFS, (part_hdr_t *) (g_rmem_off - MKIMG_HEADER_SZ));
	if (g_rimg_sz == -1) {
		len = -EIO;
		goto exit;
	}

	if (len < 0) {
		len = -EIO;
		goto exit;
	}

exit:
#endif
	return len;
}

/**********************************************************
 * Routine: mboot_android_load_bootimg
 *
 * Description: main function to load Android Boot Image
 *
 **********************************************************/
int mboot_android_load_bootimg(char *part_name, unsigned long addr)
{
	long len;
	unsigned long long start_addr;
	int part;
	part_dev_t *dev;

	dev = mt_part_get_device();
	if (!dev) {
		dprintf(CRITICAL, "mboot_android_load_bootimg , dev = NULL\n");
		return -ENODEV;
	}

	part = partition_get_index(part_name);
	if (part == INVALID_PTN) {
		dprintf(CRITICAL, "mboot_android_load_bootimg , part = NULL\n");
		return -ENOENT;
	}
	/* *************** */
	/* * not to include unused header */
	/* * */
	if (multi_dtb_image_offset != 0) {
		addr -= multi_dtb_image_offset;
		start_addr = partition_get_offset(part);
	} else
		start_addr = partition_get_offset(part) + BIMG_HEADER_SZ;
#ifndef SKIP_MTK_PARTITION_HEADER_CHECK
	addr = addr - MKIMG_HEADER_SZ;
#endif

	/* *************** */
	/* * read image data */
	/* * */
	dprintf(CRITICAL, "\nread the data of %s (size = 0x%x)\n", part_name, g_bimg_sz);

#ifdef MTK_EMMC_SUPPORT
	dprintf(CRITICAL, " > from - 0x%016llx (skip boot img hdr)\n", start_addr);
#else
	dprintf(CRITICAL, " > from - 0x%x (skip boot img hdr)\n", start_addr);
#endif
	dprintf(CRITICAL, " > to   - 0x%x (starts with kernel img hdr)\n", addr);

	len = partition_read(part_name, g_boot_hdr->page_size, (uchar*)addr, (size_t)g_bimg_sz);

	if (multi_dtb_image_offset != 0) {
		char buf[24], buf2[24];
		int next, i;
		int offset = fdt_path_offset((void *)addr, "/images/ramdisk@1");
		fdt_next_tag((void *)addr, offset, &next);
		fdt_next_tag((void *)addr, next, &offset);
		offset += addr + fdt_off_dt_struct(addr);
		g_rmem_off = (unsigned int)(((struct fdt_property *)offset)->data);
		g_rimg_sz = fdt32_to_cpu(((struct fdt_property *)offset)->len);
		sprintf(buf2, "%s.dtb", PROJECT);
		for (i = 1; i < 20; i++) {
			sprintf(buf, "%s%d", "/images/fdt@", i);
			dprintf(CRITICAL, "Try %d: %s\n", i, buf);
			offset = fdt_path_offset((void *)addr, buf);
			fdt_next_tag((void *)addr, offset, &next);
			offset = addr + next + fdt_off_dt_struct(addr);
			if (!strnicmp(buf2, ((struct fdt_property *)offset)->data, sizeof(buf2))) {
				fdt_next_tag((void *)addr, next, &offset);
				offset += addr + fdt_off_dt_struct(addr);
				g_smem_off = (unsigned int)(((struct fdt_property *)offset)->data);
				device_tree_size = fdt32_to_cpu(((struct fdt_property *)offset)->len);
				dprintf(CRITICAL, "Found: %u %u\n", g_smem_off, device_tree_size);
				break;
			}
		}
	}
#ifndef SKIP_MTK_PARTITION_HEADER_CHECK
	/* *************** */
	/* * check kernel header */
	/* * */
	g_kimg_sz = mboot_android_check_img_info(PART_KERNEL, (part_hdr_t *) (g_kmem_off - MKIMG_HEADER_SZ));
	if (g_kimg_sz == -1) {
		len = -EIO;
		goto exit;
	}
	/* *************** */
	/* * check rootfs header */
	/* * */
	g_rimg_sz = mboot_android_check_img_info(PART_ROOTFS, (part_hdr_t *) (g_rmem_off - MKIMG_HEADER_SZ));
	if (g_rimg_sz == -1) {
		len = -EIO;
		goto exit;
	}

	if (len < 0) {
		len = -EIO;
		goto exit;
	}

exit:
#endif
	return len;
}

/**********************************************************
 * Routine: mboot_recovery_load_raw_part
 *
 * Description: load raw data for recovery mode support
 *
 **********************************************************/
int mboot_recovery_load_raw_part(char *part_name, unsigned long *addr, unsigned int size)
{
	long len;
	unsigned long begin;
	unsigned long long start_addr;
	int part;
	part_dev_t *dev;

	dev = mt_part_get_device();
	if (!dev) {
		return -ENODEV;
	}

	part = partition_get_index(part_name);
	if (part == INVALID_PTN) {
		return -ENOENT;
	}
	start_addr = partition_get_offset(part);
	begin = get_timer(0);

	len = partition_read(part_name, 0, (uchar*)addr, (size_t)size);

	if (len < 0) {
		len = -EIO;
		goto exit;
	}

	dprintf(CRITICAL, "[%s] Load '%s' partition to 0x%08X (%d bytes in %ld ms)\n", MODULE_NAME, part_name, (unsigned int)addr, size, get_timer(begin));

exit:
	return len;
}

/**********************************************************
 * Routine: mboot_recovery_load_raw_part_offset
 *
 * Description: load partition raw data with offset
 *
 * offset and size must page alignemnt
 **********************************************************/
int mboot_recovery_load_raw_part_offset(char *part_name, unsigned long *addr, unsigned long offset, unsigned int size)
{
	long len;
	unsigned long begin;
	unsigned long long start_addr;
	int part;
	part_dev_t *dev;

	dev = mt_part_get_device();
	if (!dev) {
		return -ENODEV;
	}

	part = partition_get_index(part_name);
	if (!part) {
		return -ENOENT;
	}

	start_addr = partition_get_offset(part) + ROUNDUP(offset, dev->blkdev->blksz);
	begin = get_timer(0);

	len = partition_read(part_name, ROUNDUP(offset, dev->blkdev->blksz),
			(uchar*)addr, (size_t)ROUNDUP(size, dev->blkdev->blksz));
	if (len < 0) {
		len = -EIO;
		goto exit;
	}

	dprintf(INFO, "[%s] Load '%s' partition to 0x%08X (%d bytes in %ld ms)\n",
	        MODULE_NAME, part_name, (unsigned int)addr, size, get_timer(begin));

exit:
	return len;
}

/**********************************************************
 * Routine: mboot_recovery_load_misc
 *
 * Description: load recovery command
 *
 **********************************************************/
int mboot_recovery_load_misc(unsigned long *misc_addr, unsigned int size)
{
	int ret;

	dprintf(CRITICAL, "[mboot_recovery_load_misc]: size is %u\n", size);
	dprintf(CRITICAL, "[mboot_recovery_load_misc]: misc_addr is 0x%x\n", misc_addr);

	ret = mboot_recovery_load_raw_part(PART_MISC, misc_addr, size);

	if (ret < 0)
		return ret;

	return ret;
}

/**********************************************************
 * Routine: mboot_get_inhouse_img_size
 *
 * Description: Get img size from mkimage header (LK,Logo)
                The size include both image and header and the size is align to 4k.
 *
 **********************************************************/
unsigned int mboot_get_inhouse_img_size(char *part_name, unsigned int *size)
{
	int ret = 0;
	long len = 0;
	u64 addr;
	int part_index;
	part_dev_t *dev;
	part_hdr_t mkimage_hdr;
	part_hdr_t *part_hdr;
	unsigned page_size = 0x1000;

	*size = 0;

	dev = mt_part_get_device();
	if (!dev) {
		dprintf(CRITICAL, "mboot_get_img_size, dev = NULL\n");
		return -ENODEV;
	}

	part_index = partition_get_index(part_name);
	if (part_index == INVALID_PTN) {
		dprintf(CRITICAL, "mboot_get_img_size (%s), part = NULL\n", part_name);
		return -ENOENT;
	}

	addr = partition_get_offset(part_index);

	/*Read mkimage header*/
	len = partition_read(part_name, 0, (uchar*)&mkimage_hdr, (size_t)sizeof(part_hdr_t));

	dprintf(CRITICAL, "\n============================================================\n");
	dprintf(CRITICAL, "[%s] INHOUSE Partition addr             : %x\n", MODULE_NAME, addr);
	dprintf(CRITICAL, "[%s] INHOUSE Partition Name             : %s\n", MODULE_NAME, part_name);
	dprintf(CRITICAL, "[%s] INHOUSE IMG HDR - Magic            : %x\n", MODULE_NAME, mkimage_hdr.info.magic);
	dprintf(CRITICAL, "[%s] INHOUSE IMG size                    : %x\n", MODULE_NAME, mkimage_hdr.info.dsize);
	dprintf(CRITICAL, "[%s] INHOUSE IMG HDR size                : %x\n", MODULE_NAME, sizeof(part_hdr_t));
	dprintf(CRITICAL, "============================================================\n");

	*size =  (((mkimage_hdr.info.dsize + sizeof(part_hdr_t)  + page_size - 1) / page_size) * page_size);
	dprintf(CRITICAL, "[%s] INHOUSE IMG size           : %x\n", MODULE_NAME, *size);

	//mboot_common_load_part_info(dev, part_name, part_hdr);

	return ret;
}

unsigned int mboot_get_img_size(char *part_name, unsigned int *size)
{
	int ret = 0;
	long len = 0;
	u64 addr;
	int part_index;
	part_dev_t *dev;
	boot_img_hdr boot_hdr;
#define BOOT_SIG_HDR_SZ 16
	unsigned char boot_sig_hdr[BOOT_SIG_HDR_SZ] = {0};
	unsigned boot_sig_size = 0;

	unsigned page_size = 0x800; /* used to cache page size in boot image hdr, default 2KB */

	*size = 0;

	dev = mt_part_get_device();
	if (!dev) {
		dprintf(CRITICAL, "mboot_get_img_size, dev = NULL\n");
		return -ENODEV;
	}

	part_index = partition_get_index(part_name);
	if (!part_index) {
		dprintf(CRITICAL, "mboot_get_img_size (%s), part = NULL\n", part_name);
		return -ENOENT;
	}

	addr = partition_get_offset(part_index);

	len = partition_read(part_name, 0, (uchar*)&boot_hdr, sizeof(boot_img_hdr));

	if (len < 0) {
		dprintf(CRITICAL, "[%s] %s boot image header read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -1;
	}

	dprintf(CRITICAL, "\n============================================================\n");
	boot_hdr.magic[7] = '\0';
	dprintf(CRITICAL, "[%s] Android Partition Name             : %s\n", MODULE_NAME, part_name);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Magic            : %s\n", MODULE_NAME, boot_hdr.magic);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Kernel Size      : 0x%08X\n", MODULE_NAME, boot_hdr.kernel_size);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Kernel Address   : 0x%08X\n", MODULE_NAME, boot_hdr.kernel_addr);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Rootfs Size      : 0x%08X\n", MODULE_NAME, boot_hdr.ramdisk_size);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Rootfs Address   : 0x%08X\n", MODULE_NAME, boot_hdr.ramdisk_addr);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Tags Address     : 0x%08X\n", MODULE_NAME, boot_hdr.tags_addr);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Page Size        : 0x%08X\n", MODULE_NAME, boot_hdr.page_size);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Command Line     : %s\n", MODULE_NAME, boot_hdr.cmdline);
	dprintf(CRITICAL, "============================================================\n");

	page_size = boot_hdr.page_size;
	*size += page_size; /* boot header size is 1 page */
	*size += (((boot_hdr.kernel_size + page_size - 1) / page_size) * page_size);
	*size += (((boot_hdr.ramdisk_size + page_size - 1) / page_size) * page_size);
	*size += (((boot_hdr.second_size + page_size - 1) / page_size) * page_size);

	/* try to get boot siganture size if it exists */
#if defined(MTK_EMMC_SUPPORT) && defined(MTK_NEW_COMBO_EMMC_SUPPORT)
	len = dev->read(dev, addr + (u64)(*size), (uchar*)&boot_sig_hdr, BOOT_SIG_HDR_SZ, EMMC_PART_USER);
#else
	len = dev->read(dev, addr + (ulong)(*size), (uchar*)&boot_sig_hdr, BOOT_SIG_HDR_SZ);
#endif

	if (len < 0) {
		dprintf(CRITICAL, "[%s] %s boot sig header read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -1;
	}
#define ASN_ID_SEQUENCE  0x30
	if (boot_sig_hdr[0] == ASN_ID_SEQUENCE) {
		/* boot signature exists */
		unsigned len = 0;
		unsigned len_size = 0;
		if (boot_sig_hdr[1] & 0x80) {
			/* multi-byte length field */
			unsigned int i = 0;
			len_size = 1 + (boot_sig_hdr[1] & 0x7f);
			for (i = 0; i < len_size - 1; i++) {
				len = (len << 8) | boot_sig_hdr[2 + i];
			}
		} else {
			/* single-byte length field */
			len_size = 1;
			len = boot_sig_hdr[1];
		}

		boot_sig_size = 1 + len_size + len;
	}
	*size += (((boot_sig_size + page_size - 1) / page_size) * page_size);

	return ret;
}

#endif
