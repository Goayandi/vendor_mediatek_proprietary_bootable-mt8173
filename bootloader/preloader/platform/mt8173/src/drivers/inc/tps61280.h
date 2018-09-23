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

#ifndef _tps61280_SW_H_
#define _tps61280_SW_H_

#define TPS61280TAG                "[TPS61280][PL] "
#define TPS61280LOG(fmt, arg...)   print(TPS61280TAG fmt, ##arg)
#define TPS61280MSG(fmt, arg...)   print(fmt, ##arg)
#define TPS61280ERR(fmt, arg...)   print(TPS61280TAG fmt, ##arg)

/**********************************************************
  *
  *   [Register]
  *
  *********************************************************/
#define TPS61280_REG_NUM 6
#define TPS61280_REG_VERSION         (0x00)
#define TPS61280_REG_CONFIG          (0x01)
#define TPS61280_REG_VOUTFLOORSET    (0x02)
#define TPS61280_REG_VOUTROOFSET     (0x03)
#define TPS61280_REG_ILIMSET         (0x04)
#define TPS61280_REG_STATUS          (0x05)
#define TPS61280_REG_E2PROMCTRL      (0xFF)

/**********************************************************
  *
  *   [MASK/SHIFT]
  *
  *********************************************************/
#define REG_CONFIG_RESET_MASK            (0x01)
#define REG_CONFIG_RESET_SHIFT           (7)
#define REG_CONFIG_ENABLE_MASK           (0x03)
#define REG_CONFIG_ENABLE_SHIFT          (5)
#define REG_CONFIG_RESERVED_MASK         (0x01)
#define REG_CONFIG_RESERVED_SHIFT        (4)
#define REG_CONFIG_GPIOCFG_MASK          (0x01)
#define REG_CONFIG_GPIOCFG_SHIFT         (3)
#define REG_CONFIG_SSFM_MASK             (0x01)
#define REG_CONFIG_SSFM_SHIFT            (2)
#define REG_CONFIG_MODE_CTRL_MASK        (0x03)
#define REG_CONFIG_MODE_CTRL_SHIFT       (0)

#define REG_VOUTFLOORSET_RESERVED_MASK   (0x07)
#define REG_VOUTFLOORSET_RESERVED_SHIFT  (5)
#define REG_VOUTFLOORSET_VOUT_TH_MASK    (0x1F)
#define REG_VOUTFLOORSET_VOUT_TH_SHIFT   (0)

#define REG_VOUTROOFSET_RESERVED_MASK   (0x07)
#define REG_VOUTROOFSET_RESERVED_SHIFT  (5)
#define REG_VOUTROOFSET_VOUT_TH_MASK    (0x1F)
#define REG_VOUTROOFSET_VOUT_TH_SHIFT   (0)

#define REG_ILIMSET_RESERVED_MASK       (0x03)
#define REG_ILIMSET_RESERVED_SHIFT      (6)
#define REG_ILIMSET_ILIM_OFF_MASK       (0x01)
#define REG_ILIMSET_ILIM_OFF_SHIFT      (5)
#define REG_ILIMSET_SOFTSTART_MASK      (0x01)
#define REG_ILIMSET_SOFTSTART_SHIFT     (4)
#define REG_ILIMSET_ILIM_MASK           (0x0F)
#define REG_ILIMSET_ILIM_SHIFT          (0)

#define REG_STATUS_TSD_MASK             (0x01)
#define REG_STATUS_TSD_SHIFT            (7)
#define REG_STATUS_HOTDIE_MASK          (0x01)
#define REG_STATUS_HOTDIE_SHIFT         (6)
#define REG_STATUS_DCDCMODE_MASK        (0x01)
#define REG_STATUS_DCDCMODE_SHIFT       (5)
#define REG_STATUS_OPMODE_MASK          (0x01)
#define REG_STATUS_OPMODE_SHIFT         (4)
#define REG_STATUS_ILIMPT_MASK          (0x01)
#define REG_STATUS_ILIMPT_SHIFT         (3)
#define REG_STATUS_ILIMBST_MASK         (0x01)
#define REG_STATUS_ILIMBST_SHIFT        (2)
#define REG_STATUS_FAULT_MASK           (0x01)
#define REG_STATUS_FAULT_SHIFT          (1)
#define REG_STATUS_PGOOD_MASK           (0x01)
#define REG_STATUS_PGOOD_SHIFT          (0)

#define REG_E2PROMCTRL_WEN_MASK         (0x01)
#define REG_E2PROMCTRL_WEN_SHIFT        (7)
#define REG_E2PROMCTRL_WP_MASK          (0x01)
#define REG_E2PROMCTRL_WP_SHIFT         (6)
#define REG_E2PROMCTRL_ISE2PROMWP_MASK  (0x01)
#define REG_E2PROMCTRL_ISE2PROMWP_SHIFT (5)
#define REG_E2PROMCTRL_RESERVED_MASK    (0x0F)
#define REG_E2PROMCTRL_RESERVED_SHIFT   (0)

/**********************************************************
  *
  *   [Extern Function]
  *
  *********************************************************/
extern U32 tps61280_init(void);

#endif // _tps61280_SW_H_

