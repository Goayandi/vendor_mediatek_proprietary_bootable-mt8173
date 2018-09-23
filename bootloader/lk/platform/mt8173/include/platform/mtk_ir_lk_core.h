/********************************************************************************************
 *     LEGAL DISCLAIMER
 *
 *     (Header of MediaTek Software/Firmware Release or Documentation)
 *
 *     BY OPENING OR USING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *     THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE") RECEIVED
 *     FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON AN "AS-IS" BASIS
 *     ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED,
 *     INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
 *     A PARTICULAR PURPOSE OR NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY
 *     WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 *     INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK
 *     ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
 *     NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S SPECIFICATION
 *     OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *     BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE LIABILITY WITH
 *     RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION,
 *     TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE
 *     FEES OR SERVICE CHARGE PAID BY BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *     THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE WITH THE LAWS
 *     OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS PRINCIPLES.
 ************************************************************************************************/

#ifndef __MTK_IR_H__
#define __MTK_IR_H__

#include "mtk_ir_cus_define.h"

#include <platform/mt_typedefs.h>
#include <platform/boot_mode.h>

#define IR_LOG_TAG "LK"

#define IRRX_BASE_PHY        (u32)0x1100c000

#define IRRX_CLK_FREQUENCE    32*1000            // 32KHZ

/**************************************************
    IRRX register define
    ************************************************/
#define IRRX_COUNT_HIGH_REG        0x0000
#define IRRX_CH_BITCNT_MASK         0x0000003f
#define IRRX_CH_BITCNT_BITSFT       0
#define IRRX_CH_1ST_PULSE_MASK      0x0000ff00
#define IRRX_CH_1ST_PULSE_BITSFT    8
#define IRRX_CH_2ND_PULSE_MASK      0x00ff0000
#define IRRX_CH_2ND_PULSE_BITSFT    16
#define IRRX_CH_3RD_PULSE_MASK      0xff000000
#define IRRX_CH_3RD_PULSE_BITSFT    24


#define IRRX_COUNT_MID_REG         0x0004
#define IRRX_COUNT_LOW_REG         0x0008


#define IRRX_CONFIG_HIGH_REG     0x000c

#define IRRX_CH_DISPD        ((u32)(1 << 15))
#define IRRX_CH_IGB0         ((u32)(1 << 14))
#define IRRX_CH_CHKEN        ((u32)(1 << 13))   //enable puse width
#define IRRX_CH_DISCH        ((u32)(1 << 7))
#define IRRX_CH_DISCL        ((u32)(1 << 6))
#define IRRX_CH_IGSYN        ((u32)(1 << 5))
#define IRRX_CH_ORDINV       ((u32)(1 << 4))
#define IRRX_CH_RC5_1ST      ((u32)(1 << 3))
#define IRRX_CH_RC5          ((u32)(1 << 2))
#define IRRX_CH_IRI          ((u32)(1 << 1))
#define IRRX_CH_HWIR         ((u32)(1 << 0))


#define IRRX_CH_END_7        ((u32)(0x07 << 16))
#define IRRX_CH_END_15       ((u32)(0x0f << 16))  //[22:16]
#define IRRX_CH_END_23       ((u32)(0x17 << 16))
#define IRRX_CH_END_31       ((u32)(0x1f << 16))
#define IRRX_CH_END_39       ((u32)(0x27 << 16))
#define IRRX_CH_END_47       ((u32)(0x2f << 16))
#define IRRX_CH_END_55       ((u32)(0x07 << 16))
#define IRRX_CH_END_63       ((u32)(0x0f << 16))

//////////////////////////////
#define IRRX_CONFIG_LOW_REG       0x0010           //IRCFGL
#define IRRX_SAPERIOD_MASK        ((u32)0xff<<0)  //[7:0]   sampling period
#define IRRX_SAPERIOD_OFFSET      ((u32)0)

#define IRRX_CHK_PERIOD_MASK      ((u32)0x1fff<<8)  //[20:8]   ir pulse width sample period
#define IRRX_CHK_PERIOD_OFFSET    ((u32)8)




#define IRRX_THRESHOLD_REG        0x0014
#define IRRX_THRESHOLD_MASK      ((u32)0x7f<<0)
#define IRRX_THRESHOLD_OFFSET     ((u32)0)

#define IRRX_ICLR_MASK          ((u32)1<<7) // interrupt clear reset ir
#define IRRX_ICLR_OFFSET         ((u32)7)

#define IRRX_DGDEL_MASK          ((u32)3<<8) // de-glitch select
#define IRRX_DGDEL_OFFSET        ((u32)8)

#define IRRX_RCMM_THD_REG        0x0018

#define IRRX_RCMM_ENABLE_MASK      ((u32)0x1<<31)
#define IRRX_RCMM_ENABLE_OFFSET    ((u32)31)    // 1 enable rcmm , 0 disable rcmm

#define IRRX_RCMM_THD_00_MASK      ((u32)0x7f<<0)
#define IRRX_RCMM_THD_00_OFFSET    ((u32)0)
#define IRRX_RCMM_THD_01_MASK      ((u32)0x7f<<7)
#define IRRX_RCMM_THD_01_OFFSET    ((u32)7)

#define IRRX_RCMM_THD_10_MASK      ((u32)0x7f<<14)
#define IRRX_RCMM_THD_10_OFFSET    ((u32)14)
#define IRRX_RCMM_THD_11_MASK      ((u32)0x7f<<21)
#define IRRX_RCMM_THD_11_OFFSET    ((u32)21)


#define IRRX_RCMM_THD_REG0        0x001c
#define IRRX_RCMM_THD_20_MASK      ((u32)0x7f<<0)
#define IRRX_RCMM_THD_20_OFFSET    ((u32)0)
#define IRRX_RCMM_THD_21_MASK      ((u32)0x7f<<7)
#define IRRX_RCMM_THD_21_OFFSET    ((u32)7)



#define IRRX_IRCLR                0x0020
#define IRRX_IRCLR_MASK           ((u32)0x1<<0)
#define IRRX_IRCLR_OFFSET         ((u32)0)

#define IRRX_EXP_IRM1                0x0060   //expect byte0 ,byte1,byte2,byte3 [31:0]
#define IRRX_EXP_IRL1                0x0038   //expect byte4 ,byte5,byte6          [23:0]

//

#define IRRX_CHKDATA0               0x0088
#define IRRX_CHKDATA1               0x008C
#define IRRX_CHKDATA2               0x0090
#define IRRX_CHKDATA3               0x0094
#define IRRX_CHKDATA4               0x0098
#define IRRX_CHKDATA5               0x009C
#define IRRX_CHKDATA6               0x00a0
#define IRRX_CHKDATA7               0x00a4
#define IRRX_CHKDATA8               0x00a8
#define IRRX_CHKDATA9               0x00ac
#define IRRX_CHKDATA10              0x00b0
#define IRRX_CHKDATA11              0x00b4
#define IRRX_CHKDATA12              0x00b8
#define IRRX_CHKDATA13              0x00bc
#define IRRX_CHKDATA14              0x00c0
#define IRRX_CHKDATA15              0x00c4
#define IRRX_CHKDATA16              0x00c8

#define IRRX_IRINT_EN              0x00cc
#define IRRX_INTEN_MASK           ((u32)0x1<<0)
#define IRRX_INTEN_OFFSET         ((u32)0)


#define IRRX_IRINT_CLR              0x00d0
#define IRRX_INTCLR_MASK           ((u32)0x1<<0)
#define IRRX_INTCLR_OFFSET         ((u32)0)

#define IRRX_WDTSET             0x00d4    //WDTSET
#define IRRX_WDT                0x00d8    //WDT

#define IRRX_INTSTAT_REG        0x00d4   //here must be care, 8127 
#define IRRX_INTSTAT_OFFSET     ((u32)18)


#define IRRX_WDTLMT             0x00dC    //WDTLMT

#define IRRX_BASE_PHY_END        (IRRX_BASE_PHY + IRRX_WDTLMT)

#define REGISTER_WRITE32(u4Addr, u4Val)     (*((volatile unsigned long*)(u4Addr)) = (u4Val))
#define REGISTER_READ32(u4Addr)             (*((volatile unsigned long*)(u4Addr)))

#define IO_WRITE32(base, offset, u4Val)     (*((volatile unsigned long*)(base + offset)) = (u4Val))
#define IO_READ32(base, offset)              (*((volatile unsigned long*)(base + offset)))

#define IR_READ32(u4Addr)          IO_READ32(IRRX_BASE_PHY, (u4Addr))
#define IR_WRITE32(u4Addr, u4Val)  IO_WRITE32(IRRX_BASE_PHY, u4Addr, u4Val)

#define IR_WRITE_MASK(u4Addr, u4Mask, u4Offet, u4Val)  IR_WRITE32(u4Addr, ((IR_READ32(u4Addr) & (~(u4Mask))) | (((u4Val) << (u4Offet)) & (u4Mask))))
#define IR_READ_MASK(u4Addr, u4Mask, u4Offet)  ((IR_READ32(u4Addr) & (u4Mask)) >> (u4Offet))

extern int mtk_ir_init(int para);
extern int mtk_ir_wait_event(void);

struct mtk_ir_lk_msg {
	u32 scancode; // rc scan code
	u32 keycode;  // linux input code
};

typedef struct IR_GLOBAL {
	char * proname;   //protocol name
	u32 u4_config_high;
	u32 u4_config_low;
	u32 u4_theshold;
	struct mtk_ir_lk_msg *pmsg;
	int u4msg_size;
	u32 (*ir_hw_decode)(void * preserve); // decode function. preserve for future use
} IR_GLOBAL_T;


#define IR_LOG_ALWAYS(fmt, arg...)  \
    do { \
         printf("%s\n"fmt, __FUNCTION__, ##arg);     \
    }while (0)


#define MTK_NEC_CONFIG      (IRRX_CH_END_15 + IRRX_CH_IGSYN + IRRX_CH_HWIR)
#define MTK_NEC_SAPERIOD    (0x00F) //
#define MTK_NEC_THRESHOLD   (0x1)

/*
for BD     (8/3MHZ) * MTK_NEC_SAPERIOD
for 8127 (1/32KHZ)*MTK_NEC_SAPERIOD
*/

#define MTK_NEC_1ST_PULSE_REPEAT  (3)
#define MTK_NEC_BITCNT_NORMAL    (33)
#define MTK_NEC_BITCNT_REPEAT    (1)
#define MTK_NEC_BIT8_VERIFY      (0xff)

//set deglitch with the min number. when glitch < (33*6 = 198us,ignore)

#define NEC_INFO_TO_BITCNT(u4Info)      ((u4Info & IRRX_CH_BITCNT_MASK)    >> IRRX_CH_BITCNT_BITSFT)
#define NEC_INFO_TO_1STPULSE(u4Info)    ((u4Info & IRRX_CH_1ST_PULSE_MASK) >> IRRX_CH_1ST_PULSE_BITSFT)
#define NEC_INFO_TO_2NDPULSE(u4Info)    ((u4Info & IRRX_CH_2ND_PULSE_MASK) >> IRRX_CH_2ND_PULSE_BITSFT)
#define NEC_INFO_TO_3RDPULSE(u4Info)    ((u4Info & IRRX_CH_3RD_PULSE_MASK) >> IRRX_CH_3RD_PULSE_BITSFT)


#define MTK_RC6_CONFIG   (IRRX_CH_END_15 | IRRX_CH_IGSYN | IRRX_CH_HWIR  | IRRX_CH_ORDINV | IRRX_CH_RC5)
#define MTK_RC6_SAPERIOD    (0xe) //
#define MTK_RC6_THRESHOLD   (0x1)


#define MTK_RC6_BITCNT   0x1e
#define MTK_RC6_LEADER   0x8
#define MTK_RC6_TOGGLE0  0x1
#define MTK_RC6_TOGGLE1  0x2
#define MTK_RC6_CUSTOM   0x32


#define RC6_INFO_TO_BITCNT(u4Info)      ((u4Info & IRRX_CH_BITCNT_MASK)    >> IRRX_CH_BITCNT_BITSFT)

#define MTK_RC6_GET_LEADER(bdata0) ((bdata0>>4))
#define MTK_RC6_GET_TOGGLE(bdata0) ((bdata0 & 0xc)>>2)
#define MTK_RC6_GET_CUSTOM(bdata0,bdata1) (((bdata0 & 0x3) << 6) |bdata1 >> 2)
#define MTK_RC6_GET_KEYCODE(bdata1,bdata2)  \
                (((bdata2>>2) | ((bdata1 & 0x3)<<6)) & 0xff)


#define MTK_RC5_CONFIG   (IRRX_CH_IGSYN | IRRX_CH_HWIR | IRRX_CH_ORDINV | IRRX_CH_RC5)
#define MTK_RC5_SAPERIOD    (0x1e) //
#define MTK_RC5_THRESHOLD   (0x1)


#define MTK_RC5_BITCNT   0x10
#define MTK_RC5_CUSTOM   0x00

#define MTK_RC5_GET_TOGGLE(bdata0) (((~(bdata0)) & 0x80) >> 7)
#define MTK_RC5_GET_CUSTOM(bdata0) (((~(bdata0)) & 0x7C) >> 2)
#define MTK_RC5_GET_KEYCODE(bdata0,bdata1)  \
                ((((~(bdata0)) & 0x03) << 4) | (((~(bdata1)) & 0xF0) >> 4))

#define RC5_INFO_TO_BITCNT(u4Info)      ((u4Info & IRRX_CH_BITCNT_MASK)    >> IRRX_CH_BITCNT_BITSFT)

#define IRRX_OK 0
#define IRRX_NO_INTERRUPT 1
#define IRRX_WAIT_TIMEOUT 2

#define BTN_NONE                    0XFFFFFFFF
#define BTN_REPEAT                    0XFFFFFFFd
#define BTN_INVALID_KEY             -1


#endif



