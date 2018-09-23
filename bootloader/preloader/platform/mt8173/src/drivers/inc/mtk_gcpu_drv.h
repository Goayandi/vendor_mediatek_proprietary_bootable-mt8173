/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 * 
 * MediaTek Inc. (C) 2010. All rights reserved.
 * 
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#ifndef __MTK_GCPU_DRV_H__
#define __MTK_GCPU_DRV_H__

extern void mtk_gcpu_drv_init(void);
extern void mtk_gcpu_drv_init_and_load_ram_code(void);
extern void mtk_gcpu_drv_uninit(void);

/*
    hmac_sha256 by using 16 bytes per-device key on data of which size is aligned to 64 bytes    
*/
extern s32 mtk_gcpu_hmac_sha256_by_using_per_dev_key
(
u8* pu1_seed_for_key, /* seed to generate the per-device key, 16 bytes */
u8* pu1_data_in_dram, /* data in dram, address should be aligned to 64 bytes */
u32 u4_data_len,      /* data size (bytes), 64xN bytes, <= 2^29 bytes */
u8* pui1_res          /* hmac_sha256 result
                         address can be in dram/sram
                         size for hmac_sha256 result should be 32 bytes */
);

/*
    aes128 enc with ecb mode by using 16 bytes per-device key on data of which size is 16xN bytes    
*/
extern s32 mtk_gcpu_aes128_enc_ecb_by_using_per_dev_key
(
u8* pu1_seed,      /* seed for scrambling the per-device key, 16 bytes */
u32 u4_mode,       /* mode for per-dev key scrambling
                         0: directly use pure per-dev key, and pu1_seed is ignore and should be set to NULL */
u8* pu1_dat,       /* data */
u32 u4_dat_len,    /* data size (bytes), 16xN bytes */
u8* pu1_res        /* result, its size is the same as one of data */
);

/*
    The function directly adds sha256 padding data to the end of the original data.
    The buffer contaning the data should be larger to keep the padding data.
    u4_data_buf_len >= (u4_data_len + 9 + 63)/64*64
*/
extern s32 mtk_gcpu_sha256
(
u8* pu1_data_in_dram, /* data in dram */
u32 u4_data_len,      /* data size (bytes) */
u32 u4_data_buf_len,  /* buffer size (bytes) for data,
                         u4_data_buf_len >= (u4_data_len + 9 + 63)/64*64 */
u8* pui1_sha256_res   /* sha256 result
                         address can be in dram/sram
                         size for sha256 result should be 32 bytes */
);

/*
    decrypt mtee image 
*/
extern s32 mtk_gcpu_decrypt_mtee_img
(
u8* pui1_src_dram,    /* src dram address */
u8* pui1_des_dram,    /* des dram address */
u32 u4_enc_img_sz,    /* encrypted image size, aligned to 16 bytes */
u8* pui1_dec_param    /* 32 bytes decrypt parameter */
);
#endif  /* __MTK_GCPU_DRV_H__ */
