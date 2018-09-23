/* Copyright (c) 2010-2013, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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

#include "mmc.h"

/*
 * MMC write function
 */
unsigned int
mmc_write(unsigned long long data_addr, unsigned int data_len, unsigned int *in)
{
	u64 ret = emmc_write(EMMC_PART_USER, data_addr + MBR_START_ADDRESS_BYTE, in, data_len);
	if (ret != data_len) return MMC_BOOT_E_FAILURE;
	return MMC_BOOT_E_SUCCESS;
}

/*
 * MMC read function
 */
unsigned int
mmc_read(unsigned long long data_addr, unsigned int *out, unsigned int data_len)
{
	u64 ret = emmc_read(EMMC_PART_USER, data_addr + MBR_START_ADDRESS_BYTE, out, data_len);
	if (ret != data_len) return MMC_BOOT_E_FAILURE;
	return MMC_BOOT_E_SUCCESS;
}

/* Return the density of the mmc device */
extern uint64_t g_emmc_user_size;
uint64_t mmc_get_device_capacity()
{
	return g_emmc_user_size;
}
