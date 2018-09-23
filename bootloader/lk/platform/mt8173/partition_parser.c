/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>
#include "mmc.h"
#include "partition_parser.h"
#include <mt_partition.h>
#include <platform/partition_fastboot.h>

static uint32_t mmc_boot_read_gpt(struct gpt_header *p_gtp_hdr);
static uint32_t mmc_boot_read_mbr(void);

struct partition_entry partition_entries[NUM_PARTITIONS];
unsigned gpt_partitions_exist = 0;
unsigned partition_count = 0;

unsigned int partition_read_table(struct gpt_header *p_gtp_hdr)
{
	unsigned int ret;

	/* Read MBR of the card */
	ret = mmc_boot_read_mbr();
	if (ret != MMC_BOOT_E_SUCCESS) {
		dprintf(CRITICAL, "MMC Boot: MBR read failed!\n");
		return MMC_BOOT_E_FAILURE;
	}

	/* Read GPT of the card if exist */
	if (gpt_partitions_exist) {
		ret = mmc_boot_read_gpt(p_gtp_hdr);
		if (ret != MMC_BOOT_E_SUCCESS) {
			dprintf(CRITICAL, "MMC Boot: GPT read failed!\n");
			return MMC_BOOT_E_FAILURE;
		}
	}
	return MMC_BOOT_E_SUCCESS;
}

/*
 * Read MBR from MMC card and fill partition table.
 */
static unsigned int mmc_boot_read_mbr(void)
{
	unsigned char buffer[BLOCK_SIZE];
	unsigned int dtype;
	int ret = MMC_BOOT_E_SUCCESS;
	int idx, i;

	/* Print out the MBR first */
	ret = mmc_read(0, (unsigned int *)buffer, BLOCK_SIZE);
	if (ret) {
		dprintf(CRITICAL, "Could not read partition from mmc\n");
		return ret;
	}

	/* Check to see if signature exists */
	ret = partition_verify_mbr_signature(BLOCK_SIZE, buffer);
	if (ret) {
		return ret;
	}

	/*
	 * Process each of the four partitions in the MBR by reading the table
	 * information into our mbr table.
	 */
	partition_count = 0;
	idx = TABLE_ENTRY_0;
	for (i = 0; i < 4; i++) {
		/* Type 0xEE indicates end of MBR and GPT partitions exist */
		dtype = buffer[idx + i * TABLE_ENTRY_SIZE + OFFSET_TYPE];
		if (dtype == MBR_PROTECTED_TYPE) {
			gpt_partitions_exist = 1;
			return ret;
		}
	}
	return MMC_BOOT_E_FAILURE;
}

/*
 * Read GPT from MMC and fill partition table
 */
static unsigned int mmc_boot_read_gpt(struct gpt_header *p_gtp_hdr)
{

	int ret = MMC_BOOT_E_SUCCESS;
	unsigned char data[BLOCK_SIZE];
	unsigned int i = 0; /* Counter for each 512 block */
	unsigned int j = 0; /* Counter for each 128 entry in the 512 block */
	unsigned int n = 0; /* Counter for UTF-16 -> 8 conversion */
	unsigned char UTF16_name[MAX_GPT_NAME_SIZE];
	/* LBA of first partition -- 1 Block after Protected MBR + 1 for PT */
	unsigned long long partition_0;
	partition_count = 0;

	/* Print out the GPT first */
	ret = mmc_read(BLOCK_SIZE, (unsigned int *)data, BLOCK_SIZE);
	if (ret) {
		dprintf(CRITICAL, "GPT: Could not read primary gpt from mmc\n");
	}

	ret = partition_parse_gpt_header(data, p_gtp_hdr);
	if (ret) {
		dprintf(INFO, "GPT: (WARNING) Primary signature invalid\n");
		return ret;
	}
	partition_0 = GET_LLWORD_FROM_BYTE(&data[PARTITION_ENTRIES_OFFSET]);
	/* Read GPT Entries */
	for (i = 0; i < (ROUNDUP(p_gtp_hdr->max_partition_count, 4)) / 4; i++) {
		ASSERT(partition_count < NUM_PARTITIONS);
		ret = mmc_read((partition_0 * BLOCK_SIZE) + (i * BLOCK_SIZE),
		               (uint32_t *) data, BLOCK_SIZE);

		if (ret) {
			dprintf(CRITICAL,
			        "GPT: mmc read card failed reading partition entries.\n");
			return ret;
		}

		for (j = 0; j < 4; j++) {
			memcpy(&(partition_entries[partition_count].type_guid),
			       &data[(j * p_gtp_hdr->partition_entry_size)],
			       PARTITION_TYPE_GUID_SIZE);
			if (partition_entries[partition_count].type_guid[0] ==
			        0x00
			        && partition_entries[partition_count].
			        type_guid[1] == 0x00) {
				i = ROUNDUP(p_gtp_hdr->max_partition_count, 4);
				break;
			}
			memcpy(&
			       (partition_entries[partition_count].
			        unique_partition_guid),
			       &data[(j * p_gtp_hdr->partition_entry_size) +
			             UNIQUE_GUID_OFFSET],
			       UNIQUE_PARTITION_GUID_SIZE);
			partition_entries[partition_count].first_lba =
			    GET_LLWORD_FROM_BYTE(&data
			                         [(j * p_gtp_hdr->partition_entry_size) +
			                          FIRST_LBA_OFFSET]);
			partition_entries[partition_count].last_lba =
			    GET_LLWORD_FROM_BYTE(&data
			                         [(j * p_gtp_hdr->partition_entry_size) +
			                          LAST_LBA_OFFSET]);
			partition_entries[partition_count].size =
			    partition_entries[partition_count].last_lba -
			    partition_entries[partition_count].first_lba + 1;
			partition_entries[partition_count].attribute_flag =
			    GET_LLWORD_FROM_BYTE(&data
			                         [(j * p_gtp_hdr->partition_entry_size) +
			                          ATTRIBUTE_FLAG_OFFSET]);

			memset(&UTF16_name, 0x00, MAX_GPT_NAME_SIZE);
			memcpy(UTF16_name, &data[(j * p_gtp_hdr->partition_entry_size) +
			                         PARTITION_NAME_OFFSET],
			       MAX_GPT_NAME_SIZE);
			/*
			 * Currently partition names in *.xml are UTF-8 and lowercase
			 * Only supporting english for now so removing 2nd byte of UTF-16
			 */
			for (n = 0; n < MAX_GPT_NAME_SIZE / 2; n++) {
				partition_entries[partition_count].name[n] =
				    UTF16_name[n * 2];
			}
			partition_count++;
		}
	}
	return ret;
}

/*
 * A8h reflected is 15h, i.e. 10101000 <--> 00010101
*/
int reflect(int data, int len)
{
	int ref = 0;

	for (int i = 0; i < len; i++) {
		if (data & 0x1) {
			ref |= (1 << ((len - 1) - i));
		}
		data = (data >> 1);
	}

	return ref;
}

/*
* Function to calculate the CRC32
*/
unsigned int calculate_crc32(unsigned char *buffer, int len)
{
	int byte_length = 8;    /*length of unit (i.e. byte) */
	int msb = 0;
	int polynomial = 0x04C11DB7;    /* IEEE 32bit polynomial */
	unsigned int regs = 0xFFFFFFFF; /* init to all ones */
	int regs_mask = 0xFFFFFFFF; /* ensure only 32 bit answer */
	int regs_msb = 0;
	unsigned int reflected_regs;

	for (int i = 0; i < len; i++) {
		int data_byte = buffer[i];
		data_byte = reflect(data_byte, 8);
		for (int j = 0; j < byte_length; j++) {
			msb = data_byte >> (byte_length - 1);   /* get MSB */
			msb &= 1;   /* ensure just 1 bit */
			regs_msb = (regs >> 31) & 1;    /* MSB of regs */
			regs = regs << 1;   /* shift regs for CRC-CCITT */
			if (regs_msb ^ msb) {   /* MSB is a 1 */
				regs = regs ^ polynomial;   /* XOR with generator poly */
			}
			regs = regs & regs_mask;    /* Mask off excess upper bits */
			data_byte <<= 1;    /* get to next bit */
		}
	}
	regs = regs & regs_mask;
	reflected_regs = reflect(regs, 32) ^ 0xFFFFFFFF;

	return reflected_regs;
}

static void
patch_gpt(uint8_t *gptImage, uint64_t density, uint32_t array_size,
          uint32_t max_part_count, uint32_t part_entry_size, uint32_t block_size)
{
	uint8_t *partition_entry_array_start;
	unsigned char *primary_gpt_header;
	unsigned char *secondary_gpt_header;
	unsigned int offset;
	unsigned long long card_size_sec;
	int total_part = 0, phy_last_part = 0;
	unsigned int last_part_offset;
	unsigned int crc_value;

	/* Get size of MMC */
	card_size_sec = (density) / block_size;
	/* Working around cap at 4GB */
	if (card_size_sec == 0) {
		card_size_sec = 4 * 1024 * 1024 * 2 - 1;
	}

	/* Generate second gpt header and entry table */
	memcpy(gptImage + (block_size * 2) + array_size,
	       gptImage + (block_size * 2),
	       array_size);
	memcpy(gptImage + (block_size * 2) + (array_size * 2),
	       gptImage + block_size,
	       block_size);

	/* Patching primary header */
	primary_gpt_header = (gptImage + block_size);
	PUT_LONG_LONG(primary_gpt_header + BACKUP_HEADER_OFFSET,
	              ((long long)(card_size_sec - 1)));
	PUT_LONG_LONG(primary_gpt_header + LAST_USABLE_LBA_OFFSET,
	              ((long long)(card_size_sec - 34)));

	/* Patching backup GPT */
	offset = (2 * array_size);
	secondary_gpt_header = offset + block_size + primary_gpt_header;
	PUT_LONG_LONG(secondary_gpt_header + PRIMARY_HEADER_OFFSET,
	              ((long long)(card_size_sec - 1)));
	PUT_LONG_LONG(secondary_gpt_header + LAST_USABLE_LBA_OFFSET,
	              ((long long)(card_size_sec - 34)));
	PUT_LONG_LONG(secondary_gpt_header + PARTITION_ENTRIES_OFFSET,
	              ((long long)(card_size_sec - 33)));
	PUT_LONG_LONG(secondary_gpt_header + BACKUP_HEADER_OFFSET,
	              ((long long)(1)));

	/* Find last partition */
	while (*(primary_gpt_header + block_size + total_part * ENTRY_SIZE) !=
	        0) {
		if (GET_LLWORD_FROM_BYTE(primary_gpt_header + block_size + total_part * ENTRY_SIZE + FIRST_LBA_OFFSET) >=
		        GET_LLWORD_FROM_BYTE(primary_gpt_header + block_size + phy_last_part * ENTRY_SIZE + FIRST_LBA_OFFSET)) {
			phy_last_part = total_part;
		}
		total_part++;
	}

	/* Patching last partition */
	last_part_offset =
	    phy_last_part * ENTRY_SIZE + PARTITION_ENTRY_LAST_LBA;
	PUT_LONG_LONG(primary_gpt_header + block_size + last_part_offset,
	              (long long)(card_size_sec - 34));
	PUT_LONG_LONG(primary_gpt_header + block_size + last_part_offset +
	              array_size, (long long)(card_size_sec - 34));

	/* Updating CRC of the Partition entry array in both headers */
	partition_entry_array_start = primary_gpt_header + block_size;
	crc_value = calculate_crc32(partition_entry_array_start,
	                            max_part_count * part_entry_size);
	PUT_LONG(primary_gpt_header + PARTITION_CRC_OFFSET, crc_value);

	crc_value = calculate_crc32(partition_entry_array_start + array_size,
	                            max_part_count * part_entry_size);
	PUT_LONG(secondary_gpt_header + PARTITION_CRC_OFFSET, crc_value);

	/* Clearing CRC fields to calculate */
	PUT_LONG(primary_gpt_header + HEADER_CRC_OFFSET, 0);
	crc_value = calculate_crc32(primary_gpt_header, 92);
	PUT_LONG(primary_gpt_header + HEADER_CRC_OFFSET, crc_value);

	PUT_LONG(secondary_gpt_header + HEADER_CRC_OFFSET, 0);
	crc_value = (calculate_crc32(secondary_gpt_header, 92));
	PUT_LONG(secondary_gpt_header + HEADER_CRC_OFFSET, crc_value);
}

/*
 * Write the GPT to the MMC.
 */
static unsigned int write_gpt(uint32_t size, uint8_t *gptImage, uint32_t block_size)
{
	unsigned int ret;
	uint64_t device_density;

	/* check size */
	if (size < ((MIN_PARTITION_ARRAY_SIZE*2)+(block_size*3))) {
		dprintf(CRITICAL,
		        "write_gpt check size fail:size(%d) < MIN_PARTITION_ARRAY_SIZE(%d)*2 + block_size(%d)*3",
		        size, MIN_PARTITION_ARRAY_SIZE, block_size);
		ret = 0xFFFFFFFF;
		goto end;
	}

	/* Get the density of the mmc device */

	device_density = mmc_get_device_capacity();

	/* Patching the primary and the backup header of the GPT table */
	patch_gpt(gptImage, device_density, MIN_PARTITION_ARRAY_SIZE, 128, 128, block_size);

	/* write primary */
	ret = mmc_write(0, (MIN_PARTITION_ARRAY_SIZE+(block_size*2)), (unsigned int *)gptImage);
	if (ret) {
		dprintf(CRITICAL, "Failed to write primary\n");
		goto end;
	}

	/* write secondary */
	ret = mmc_write((device_density - (block_size + MIN_PARTITION_ARRAY_SIZE)),
	                (block_size + MIN_PARTITION_ARRAY_SIZE),
	                (unsigned int *)(gptImage + (MIN_PARTITION_ARRAY_SIZE+(block_size*2))));
	if (ret) {
		dprintf(CRITICAL, "Failed ot write secondary\n");
		goto end;
	}

end:
	return ret;
}

unsigned int write_partition(unsigned size, unsigned char *partition)
{
	return write_gpt(size, partition, 512);
}

/*
 * Find index of parition in array of partition entries
 */
int partition_get_index(const char *name)
{
	unsigned int input_string_length = strlen(name);
	unsigned n;

	if ( partition_count >= NUM_PARTITIONS) {
		return INVALID_PTN;
	}
	if (!strcmp(name,"preloader")) {
		return PRELOADER_PART_IDX;
	}
	for (n = 0; n < partition_count; n++) {
		if (!memcmp
		        (name, &partition_entries[n].name, input_string_length)
		        && input_string_length ==
		        strlen((const char *)&partition_entries[n].name)) {
			return n;
		}
	}
	return INVALID_PTN;
}

int partition_get_phy_last_index(void)
{
	unsigned n, l;

	if ( partition_count >= NUM_PARTITIONS) {
		return INVALID_PTN;
	}

	l = 0;
	for (n = 0; n < partition_count; n++) {
		if (partition_entries[n].first_lba >= partition_entries[l].first_lba) {
			l = n;
		}
	}
	return l;
}

int partition_get_name(int index, char **partition_name)
{
	*partition_name = (char *)&partition_entries[index].name;
	return 0;
}

/* Get size of the partition */
unsigned long long partition_get_size(int index)
{
	if (index == PRELOADER_PART_IDX)
		return 0x40000;
	if (index == INVALID_PTN || (unsigned)index >= partition_count)
		return -1;
	else {
		return partition_entries[index].size * BLOCK_SIZE;
	}
}

/* Get offset of the partition */
unsigned long long partition_get_offset(int index)
{
	if (index == PRELOADER_PART_IDX)
		return 0;
	if (index == INVALID_PTN)
		return 0;
	else {
		return partition_entries[index].first_lba * BLOCK_SIZE + MBR_START_ADDRESS_BYTE;
	}
}

/* Debug: Print all parsed partitions */
void partition_dump()
{
	unsigned i = 0;
	for (i = 0; i < partition_count; i++) {
		dprintf(SPEW,
		        "ptn[%d]:Name[%s] Size[%llu] Type[%u] First[%llu] Last[%llu]\n",
		        i, partition_entries[i].name, partition_entries[i].size,
		        partition_entries[i].dtype,
		        partition_entries[i].first_lba,
		        partition_entries[i].last_lba);
	}
}

unsigned int
partition_verify_mbr_signature(unsigned size, unsigned char *buffer)
{
	/* Avoid checking past end of buffer */
	if ((TABLE_SIGNATURE + 1) > size) {
		return MMC_BOOT_E_FAILURE;
	}
	/* Check to see if signature exists */
	if ((buffer[TABLE_SIGNATURE] != MMC_MBR_SIGNATURE_BYTE_0) ||
	        (buffer[TABLE_SIGNATURE + 1] != MMC_MBR_SIGNATURE_BYTE_1)) {
		dprintf(CRITICAL, "MBR signature does not match.\n");
		return MMC_BOOT_E_FAILURE;
	}
	return MMC_BOOT_E_SUCCESS;
}

unsigned int
mbr_partition_get_type(unsigned size, unsigned char *partition,
                       unsigned int *partition_type)
{
	unsigned int type_offset = TABLE_ENTRY_0 + OFFSET_TYPE;

	if (size < type_offset) {
		goto end;
	}

	*partition_type = partition[type_offset];
end:
	return MMC_BOOT_E_SUCCESS;
}

int partition_get_type(int index, char **partition_type)
{
	int i, loops;

	if (index < 0 || index >= 129) {
		return -1;
	}

	loops = sizeof(g_part_name_map) / sizeof(struct part_name_map);

	for (i = 0; i < loops; i++) {
		if (!g_part_name_map[i].fb_name[0])
			break;
		if (!strcmp(partition_entries[index].name, g_part_name_map[i].r_name)) {
			*partition_type = g_part_name_map[i].partition_type;
			return 0;
		}
	}
	return -1;
}

/*
 * Parse the gpt header and get the required header fields
 * Return 0 on valid signature
 */
unsigned int
partition_parse_gpt_header(unsigned char *buffer, struct gpt_header *p_gtp_hdr)
{
	/* Check GPT Signature */
	if (((uint32_t *) buffer)[0] != GPT_SIGNATURE_2 ||
	        ((uint32_t *) buffer)[1] != GPT_SIGNATURE_1)
		return 1;

	p_gtp_hdr->header_size = GET_LWORD_FROM_BYTE(&buffer[HEADER_SIZE_OFFSET]);
	p_gtp_hdr->backup_header_lba = GET_LLWORD_FROM_BYTE(&buffer[BACKUP_HEADER_OFFSET]);
	p_gtp_hdr->first_usable_lba = GET_LLWORD_FROM_BYTE(&buffer[FIRST_USABLE_LBA_OFFSET]);
	p_gtp_hdr->max_partition_count = GET_LWORD_FROM_BYTE(&buffer[PARTITION_COUNT_OFFSET]);
	p_gtp_hdr->partition_entry_size = GET_LWORD_FROM_BYTE(&buffer[PENTRY_SIZE_OFFSET]);

	return 0;
}
