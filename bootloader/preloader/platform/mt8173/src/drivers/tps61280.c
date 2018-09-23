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

#include "platform.h"
#include "i2c.h"
#include "tps61280.h"
#include "cust_i2c.h"

/**********************************************************
  *
  *   [I2C Slave Setting]
  *
  *********************************************************/
#ifndef I2C_EXT_VBAT_BOOST_CHANNEL
#define TPS61280_I2C_BUS 2
#else
#define TPS61280_I2C_BUS I2C_EXT_VBAT_BOOST_CHANNEL
#endif
#define TPS61280_SLAVE_ADDR 0x75

/**********************************************************
  *
  *   [Global Variable]
  *
  *********************************************************/
U8 tps61280_reg[TPS61280_REG_NUM] = {0};
int g_tps61280_hw_exist=0;

/**********************************************************
  *
  *   [I2C Function For Read/Write TPS61280]
  *
  *********************************************************/
U32 tps61280_write_byte(U8 cmd, U8 writeData)
{
    U8 cmd_buf[2]={0x00, 0x00};
    U32 ret = I2C_OK;
    int readData = 0;

    struct mt_i2c_t i2c;

    cmd_buf[0] = cmd;
    cmd_buf[1] = writeData;

    i2c.id = TPS61280_I2C_BUS;
	//i2c.dir = 1; //I2C4~6 need set to 1
	i2c.addr = TPS61280_SLAVE_ADDR;
	i2c.mode = ST_MODE;
	i2c.speed = 100;
	//i2c.is_rs_enable = 1;

    ret = i2c_write(&i2c, &cmd_buf[0], 2);
	if (I2C_OK != ret)
    {
        TPS61280ERR("Write 1 byte fails(%x).\n", ret);
        return ret;
    }

    return ret;
}

U32 tps61280_read_byte(U8 cmd, U8 *returnData)
{
    U8 cmd_buf[2]={0x00, 0x00};
    U32 ret = I2C_OK;
    int readData = 0;

    struct mt_i2c_t i2c;

    cmd_buf[0] = cmd;
    cmd_buf[1] = 0x00;

    i2c.id = TPS61280_I2C_BUS;
	//i2c.dir = 1; //I2C4~6 need set to 1
	i2c.addr = TPS61280_SLAVE_ADDR;
	i2c.mode = ST_MODE;
	i2c.speed = 100;
    //i2c.is_rs_enable = 1;

	ret = i2c_write_read(&i2c, &cmd_buf[0], 1, 1);
	if(I2C_OK != ret)
    {
        TPS61280ERR("Read 1 byte fails(%x).\n", ret);
		ret = -1;
		return ret;
	}

    readData = cmd_buf[0];
    *returnData = readData;

    return ret;
}

/**********************************************************
  *
  *   [Read / Write Function]
  *
  *********************************************************/
U32 tps61280_read_interface (U8 RegNum, U8 *val, U8 MASK, U8 SHIFT)
{
    U8 tps61280_reg = 0;
    int ret = 0;

    ret = tps61280_read_byte(RegNum, &tps61280_reg);
    tps61280_reg &= (MASK << SHIFT);
    *val = (tps61280_reg >> SHIFT);

    return ret;
}

U32 tps61280_config_interface (U8 RegNum, U8 val, U8 MASK, U8 SHIFT)
{
    U8 tps61280_reg = 0;
    int ret = 0;

    ret = tps61280_read_byte(RegNum, &tps61280_reg);

    tps61280_reg &= ~(MASK << SHIFT);
    tps61280_reg |= (val << SHIFT);

    ret = tps61280_write_byte(RegNum, tps61280_reg);

    return ret;
}


/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
void tps61280_dump_register(void)
{
    int i=0;
    TPS61280LOG("Dump Register:\n");
    for (i=0; i<TPS61280_REG_NUM; i++)
    {
        tps61280_read_byte(i, &tps61280_reg[i]);
        TPS61280LOG("[0x%x]=0x%x ", i, tps61280_reg[i]);
    }
    TPS61280LOG("\n");
}

void tps61280_hw_component_detect(void)
{
    int ret=0;
    U8 reg_val=0;

    ret=tps61280_read_interface(TPS61280_REG_VOUTFLOORSET,
                                &reg_val,REG_VOUTFLOORSET_VOUT_TH_MASK, REG_VOUTFLOORSET_VOUT_TH_SHIFT);

    if((ret !=0) || (reg_val == 0))
        g_tps61280_hw_exist=0;
    else
        g_tps61280_hw_exist=1;

    TPS61280LOG("[tps61280_hw_component_detect] exist=%d, Reg[0x3]=0x%x\n", g_tps61280_hw_exist, reg_val);
}

/**********************************************************
  *
  *   [platform_driver API]
  *
  *********************************************************/
U32 tps61280_init (void)
{
    U8 reg_val = 0;
    int ret = 0;

    TPS61280LOG("[%s]\n", __FUNCTION__);

    tps61280_hw_component_detect();
    if (!g_tps61280_hw_exist)
        return ret;

    TPS61280LOG("Before Config TPS61280\n");

    tps61280_dump_register();

    /* set to PFMAuto mode */
    ret = tps61280_config_interface(TPS61280_REG_CONFIG,
                                    0x1,
                                    REG_CONFIG_MODE_CTRL_MASK,
                                    REG_CONFIG_MODE_CTRL_SHIFT);

    /* per TI's suggestion, add software flow to avoid leakage issue. Set 4.4V and back to 3.xV */
    TPS61280LOG("set to 4.4V\n");
    ret = tps61280_config_interface(TPS61280_REG_VOUTROOFSET,
                                    0x1F,
                                    REG_VOUTROOFSET_VOUT_TH_MASK,
                                    REG_VOUTROOFSET_VOUT_TH_SHIFT);


	/* delay for 5 ms at least */
	mdelay(10);

    /* CONFIG VOUTROOF_TH to 10000: 3.65V */
    TPS61280LOG("set to 3.65V\n");
    ret = tps61280_config_interface(TPS61280_REG_VOUTROOFSET,
                                    0x10,
                                    REG_VOUTROOFSET_VOUT_TH_MASK,
                                    REG_VOUTROOFSET_VOUT_TH_SHIFT);

    /* CONFIG ILIM to max 1111: 5000mAV */
    ret = tps61280_config_interface(TPS61280_REG_ILIMSET,
                                    0x0F,
                                    REG_ILIMSET_ILIM_MASK,
                                    REG_ILIMSET_ILIM_SHIFT);

    TPS61280LOG("After Config TPS61280\n");
    tps61280_dump_register();
}

