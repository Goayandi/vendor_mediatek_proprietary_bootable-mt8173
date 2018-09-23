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
#ifndef __SEC_CUST_CRYPTO_H__
#define __SEC_CUST_CRYPTO_H__

/*
    hw sha256 operation

    parameter:
        pu1_data_in_dram  [in]  data in dram
        u4_data_len       [in]  data size (bytes)
        u4_data_buf_len   [in]  buffer size (bytes) for data
                                u4_data_buf_len >= (u4_data_len + 9 + 63)/64*64
        pui1_sha256_res   [out] sha256 result, address can be in dram/sram
                                size for sha256 result should be 32 bytes
    return:
        0 if successful

    note:
        The function directly adds sha256 padding data to the end of the original data.
        The buffer contaning the data should be large enough to keep the padding data.
        u4_data_buf_len >= (u4_data_len + 9 + 63)/64*64
*/
extern unsigned int sec_cust_hw_sha256
(
unsigned char* pu1_data_in_dram, /* data in dram */
unsigned int   u4_data_len,      /* data size (bytes) */
unsigned int   u4_data_buf_len,  /* buffer size (bytes) for data,
                                    u4_data_buf_len >= (u4_data_len + 9 + 63)/64*64 */
unsigned char* pu1_sha256_res    /* sha256 result
                                    address can be in dram/sram
                                    size for sha256 result should be 32 bytes */
);

/*
    rsa2048 decryption for e=65537

    parameter:
        pu1_src   [in]  source data (signature), 256 bytes
        pu1_n     [in]  public key (modulus), 256 bytes
        pu1_des   [in]  destination data, 256 bytes

    return:
        0 if successful
*/

extern int sec_cust_rsa2048_dec_e65537
(
unsigned char* pu1_src,
unsigned char* pu1_n,
unsigned char* pu1_des
);

/*
   pkcs#1 v2.00 PSS decode by using hw sha256

   parameter:
       msghash         [in]  The hash to verify
       msghashlen      [in]  The length of the hash (octets)
       sig             [in]  The signature data (encoded data)
       siglen          [in]  The length of the signature data (octets)
       saltlen         [in]  The length of the salt used (octets)
       modulus_bitlen  [in]  The bit length of the RSA modulus
       res             [out] The result of the comparison, 1==valid, 0==invalid

   return:
       0 if successful (even if the comparison failed)

   note:
       for RSA2048 case the parameter
       msghashlen     = 32
       siglen         = 256
       saltlen        = 32
       modulus_bitlen = 2048  
*/
extern int sec_cust_pkcs_1_pss_decode_hw_sha256
(
const unsigned char* msghash,
unsigned long        msghashlen,
const unsigned char* sig,
unsigned long        siglen,
unsigned long        saltlen,
unsigned long        modulus_bitlen,
int*                 res
);

/*
    encrypt or decrypt 16 bytes data by using per soc key
    input
       pu8_in_out                input 16 bytes data
       u8_enc                    flag for encryption(!0)/decryption(0)

    output
       pu8_in_out                output 16 bytes data

    return
       0  - success
       <0 - fail
*/
extern int crypto_enc_dec_16bytes_data_by_soc_key(u8* pu8_in_out, u8 u8_enc);

/*
    decrypt 16 bytes data by using soc model key that is the same for all soc for the same module
    ex: all 8173 soc use the same key

    input
       pu8_in                 input 16 bytes data

    output
       pu8_out                output 16 bytes data

    return
       0  - success
       <0 - fail
*/
extern int crypto_dec_16bytes_data_by_soc_model_key(u8* pu8_in, u8* pu8_out);

/*
    hmac_sha256 by using 16 bytes per-device key on data of which size is aligned to 64 bytes

    input
       pu8_in                 input data, should be in dram (address aligned to 64 bytes), not in sram
       u32_dat_sz             data size, align to 64bytes, <= 2^29 bytes

    output
       pu8_mac                output 32 bytes data

    return
       0  - success
       <0 - fail

    note:
       1. hw crypto engine is initialized and un-initialized in the function
       2. It cannot be mixed and used with other hw crypto API
       3. It cannot be mixed and used with bldr_load_mtee_part_ex()/bldr_load_mtee_part_ex1()
*/
extern int crypto_hmac_sha256_by_per_dev_key(u8* pu8_in, u32 u32_dat_sz, u8* pu8_mac);


#endif /* __SEC_CUST_CRYPTO_H__ */
