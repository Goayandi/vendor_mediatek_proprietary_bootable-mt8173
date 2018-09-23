#include <mmc.h>
#include <partition_parser.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <platform/errno.h>
#include <target.h>
#include <platform/mmc_core.h>

#define DBGMSG(...)
#define ERRMSG(...)
#define TAG "[GPT_LK]"

void mt_part_dump(void)
{
}

extern void register_partition_var(void);
#define GPT_SIZE (34 * 512)
int part_init(part_dev_t *dev)
{
	mt_part_init(BLK_NUM(1 * GB));
	return 0;
}
void mt_part_init(unsigned long totalblks)
{
	struct gpt_header gtp_hdr;
	//int part;
	uint8_t *buffer;
	int ret;

	partition_read_table(&gtp_hdr);

	/* check whether to resize userdata partition */
	//part = partition_get_phy_last_index();
	if (gtp_hdr.backup_header_lba == (mmc_get_device_capacity()/512 - 1)) {
		//dprintf(CRITICAL, "GPT table has been updated.\n");
		return;
	}

	buffer = (uint8_t *)malloc(GPT_SIZE*2 - 512);
	if (!buffer) {
		dprintf(CRITICAL, "Failed to Allocate memory to read partition table\n");
		return;
	}
	ret = mmc_read(0, (unsigned int *)buffer, GPT_SIZE);
	if (ret) {
		dprintf(CRITICAL, "GPT: Could not read primary gpt from mmc\n");
		free(buffer);
		return;
	}
	ret = write_partition(GPT_SIZE*2-512, buffer);
	if (ret) {
		dprintf(CRITICAL, "Failed to write partition table\n");
	}
	free(buffer);

	partition_read_table(&gtp_hdr);
	//fixme for fastboot register_partition_var();
}

/**/
/*fastboot*/
/**/

unsigned int partition_get_region(int index)
{
	if (index == PRELOADER_PART_IDX)
		return EMMC_PART_BOOT1;
	return EMMC_PART_USER;
}
int is_support_erase(int index)
{
	return 1;
}
int is_support_flash(int index)
{
	return 1;
}
u64 emmc_write(u32 part_id, u64 offset, void *data, u64 size)
{
	part_dev_t *dev = mt_part_get_device();
	return (u64)dev->write(dev,data,offset,(int)size, part_id);
}
u64 emmc_read(u32 part_id, u64 offset, void *data, u64 size)
{
	part_dev_t *dev = mt_part_get_device();
	return (u64)dev->read(dev,offset,data,(int)size, part_id);
}
int emmc_erase(u32 part_id, u64 offset, u64 size)
{
	part_dev_t *dev = mt_part_get_device();
	return dev->erase(0, offset, size, part_id);
}
unsigned long partition_reserve_size(void)
{
	return 0;
}

