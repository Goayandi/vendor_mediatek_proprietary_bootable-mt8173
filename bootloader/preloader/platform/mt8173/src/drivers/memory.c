/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2010. All rights reserved.
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
*
* The following software/firmware and/or related documentation ("MediaTek Software")
* have been modified by MediaTek Inc. All revisions are subject to any receiver's
* applicable license agreements with MediaTek Inc.
*/


#include "typedefs.h"
#include "platform.h"
#include "dramc_register.h"
#include "dramc_pi_api.h"
#include "memory.h"
#include "emi.h"
#include "uart.h"

#define MOD "MEM"

#include "wdt.h"
#include "emi_hw.h"

extern u32 g_ddr_reserve_enable;
extern  u32 g_ddr_reserve_success;


#if MEM_TEST
int complex_mem_test (unsigned int start, unsigned int len);
#endif

#ifdef DRAMC_DEBUG_FULL_MEMORY_SCAN
int complex_mem_test1(unsigned int start, unsigned int len);
#endif

// --------------------------------------------------------
// init EMI
// --------------------------------------------------------
void
mt_mem_init (void)
{
  unsigned int emi_cona;

  /* DDR reserve mode no need to enable memory & test */
  //if((mtk_wdt_boot_check() == WDT_BY_PASS_PWK_REBOOT) && (g_ddr_reserve_enable==1) && (g_ddr_reserve_success==1))
  /*Note: factory reset failed workaround*/
  if((g_ddr_reserve_enable==1) && (g_ddr_reserve_success==1))
  {
    /*EMI register dummy read. Give clock to EMI APB register to avoid DRAM access hang...*/      
    emi_cona = *(volatile unsigned *)(EMI_CONA);      
    print("[DDR Reserve mode] EMI dummy read CONA = 0x%x\n", emi_cona);
    /* Reset EMI MPU protect setting - otherwise we can not use protected region during boot-up time */
    DRV_WriteReg32(EMI_MPUA, 0x0);
    DRV_WriteReg32(EMI_MPUB, 0x0);
    DRV_WriteReg32(EMI_MPUC, 0x0);
    DRV_WriteReg32(EMI_MPUD, 0x0);
    DRV_WriteReg32(EMI_MPUE, 0x0);
    DRV_WriteReg32(EMI_MPUF, 0x0);
    DRV_WriteReg32(EMI_MPUG, 0x0);
    DRV_WriteReg32(EMI_MPUH, 0x0);
    DRV_WriteReg32(EMI_MPUI, 0x0);
    DRV_WriteReg32(EMI_MPUJ, 0x0);
    DRV_WriteReg32(EMI_MPUK, 0x0);
    DRV_WriteReg32(EMI_MPUL, 0x0);
  }
  else /* normal boot */
  {
#ifdef DRAMC_DEBUG_RETRY
	U32 i, j, n;

	i = 0x00; n = 0x00;
	for(j = 0; j < 3; j++)
	{
		mt_set_emi ();
		#if MEM_TEST
		if ((i = complex_mem_test (0x40000000, MEM_TEST_SIZE)) == 0)
		{
			n = 0x00;
			_emi_retry = 0x00;
			print ("[%s] complex R/W mem test pass\n", MOD);
			break;
		}
		else
		{
			n = 0x01;
			_emi_retry = 0x01;
			print ("[%s] complex R/W mem test fail :%x\n", MOD, i);
			print ("[%s]mem test fail, retry %d ...\n", MOD, j);
			emi_retry_adjust(j);
			continue;
		}
		#endif
	}
	
	if((n) && (j >= 3))
	{//retry 3 times stiil fail, reboot
		 print ("[%s]mem test retry 3 tiems still fail : reboot\n", MOD);
		#ifdef DRAMC_DEBUG_CALIB_LOG
		dramc_calib_step_save(&gs_dramc_calib,DRAMC_CALIB_THIRD_BUFFER);
		#endif		 
		 mtk_arch_reset(1);
         while(1);
	}
	else
	{//memory test pass
		#ifdef DRAMC_DEBUG_CALIB_LOG
		dramc_calib_step_save(&gs_dramc_calib, DRAMC_CALIB_SECOND_BUFFER);
		#endif
	}

#else
#if !(CFG_FPGA_PLATFORM)
    mt_set_emi ();
#endif

#if MEM_TEST
    {
        int i = 0;
        if((i = complex_mem_test (0x40000000, MEM_TEST_SIZE)) == 0)
        {
			#ifdef DRAMC_DEBUG_CALIB_LOG
			dramc_calib_step_save(&gs_dramc_calib, DRAMC_CALIB_SECOND_BUFFER);
			#endif	        
            print ("[%s] complex R/W mem test pass\n", MOD);
        }
        else
        {
			#ifdef DRAMC_DEBUG_CALIB_LOG
			dramc_calib_step_save(&gs_dramc_calib,DRAMC_CALIB_THIRD_BUFFER);
			#endif
            print ("[%s] complex R/W mem test fail :%x\n", MOD, i);
            ASSERT(0);
        }
    }
	#endif
#endif
  }

#ifdef DDR_RESERVE_MODE  
  /* Always enable DDR-reserve mode */
  rgu_dram_reserved(1);
#endif

//=============================================
#ifdef DRAMC_DEBUG_FULL_MEMORY_SCAN
{
    int ii = 0;	
	unsigned int dram_rank_size[4] = {0,0,0,0};
	unsigned int dram_size = 0;
	
	/*get memory size*/
	get_dram_rank_size(dram_rank_size);
	dram_size= dram_rank_size[0] + dram_rank_size[1] + dram_rank_size[2] + dram_rank_size[3];
	if((ii = complex_mem_test1(0x40000000, dram_size)) == 0)		
    {
		#ifdef DRAMC_DEBUG_CALIB_LOG
		gs_dramc_calib.u4FullMemoryTest[0] = 0x01;//full memory test pass
		#endif
		print ("[%s] Full memory 0x%x complex R/W mem test pass\n", MOD, dram_size);
		print ("=======================================\n");			
    }
    else
    {
		#ifdef DRAMC_DEBUG_CALIB_LOG
		gs_dramc_calib.u4FullMemoryTest[0] = 0x02;//full memory test fail
		gs_dramc_calib.u4FullMemoryTest[1] = ii;  //error code
		#endif   
		print ("[%s] Full memory 0x%x complex R/W mem test fail :%x\n", MOD, dram_size, ii);
		print ("=======================================\n");
    }
	#ifdef DRAMC_DEBUG_CALIB_LOG
	dramc_calib_step_save(&gs_dramc_calib,DRAMC_CALIB_FIRST_BUFFER);
	#endif	
}
#endif
//=============================================

}

#if MEM_TEST
// --------------------------------------------------------
// do memory test
// --------------------------------------------------------
#define PATTERN1 0x5A5A5A5A
#define PATTERN2 0xA5A5A5A5

int
complex_mem_test (unsigned int start, unsigned int len)
{
    unsigned char *MEM8_BASE = (unsigned char *) start;
    unsigned short *MEM16_BASE = (unsigned short *) start;
    unsigned int *MEM32_BASE = (unsigned int *) start;
    unsigned int *MEM_BASE = (unsigned int *) start;
    unsigned char pattern8;
    unsigned short pattern16;
    unsigned int i, j, size, pattern32;
    unsigned int value;
	
	if(dramc_actiming_freq_check())
	{
		return -120;
	}
	
    size = len >> 2;

    /* === Verify the tied bits (tied high) === */
    for (i = 0; i < size; i++)
    {
        MEM32_BASE[i] = 0;
    }

    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0)
        {
            return -1;
        }
        else
        {
            MEM32_BASE[i] = 0xffffffff;
        }
    }

    /* === Verify the tied bits (tied low) === */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0xffffffff)
        {
            return -2;
        }
        else
            MEM32_BASE[i] = 0x00;
    }

    /* === Verify pattern 1 (0x00~0xff) === */
    pattern8 = 0x00;
    for (i = 0; i < len; i++)
        MEM8_BASE[i] = pattern8++;
    pattern8 = 0x00;
    for (i = 0; i < len; i++)
    {
        if (MEM8_BASE[i] != pattern8++)
        {
            return -3;
        }
    }

    /* === Verify pattern 2 (0x00~0xff) === */
    pattern8 = 0x00;
    for (i = j = 0; i < len; i += 2, j++)
    {
        if (MEM8_BASE[i] == pattern8)
            MEM16_BASE[j] = pattern8;
        if (MEM16_BASE[j] != pattern8)
        {
            return -4;
        }
        pattern8 += 2;
    }

    /* === Verify pattern 3 (0x00~0xffff) === */
    pattern16 = 0x00;
    for (i = 0; i < (len >> 1); i++)
        MEM16_BASE[i] = pattern16++;
    pattern16 = 0x00;
    for (i = 0; i < (len >> 1); i++)
    {
        if (MEM16_BASE[i] != pattern16++)
        {
            return -5;
        }
    }

    /* === Verify pattern 4 (0x00~0xffffffff) === */
    pattern32 = 0x00;
    for (i = 0; i < (len >> 2); i++)
        MEM32_BASE[i] = pattern32++;
    pattern32 = 0x00;
    for (i = 0; i < (len >> 2); i++)
    {
        if (MEM32_BASE[i] != pattern32++)
        {
            return -6;
        }
    }

    /* === Pattern 5: Filling memory range with 0x44332211 === */
    for (i = 0; i < size; i++)
        MEM32_BASE[i] = 0x44332211;

    /* === Read Check then Fill Memory with a5a5a5a5 Pattern === */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0x44332211)
        {
            return -7;
        }
        else
        {
            MEM32_BASE[i] = 0xa5a5a5a5;
        }
    }

    /* === Read Check then Fill Memory with 00 Byte Pattern at offset 0h === */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0xa5a5a5a5)
        {
            return -8;
        }
        else
        {
            MEM8_BASE[i * 4] = 0x00;
        }
    }

    /* === Read Check then Fill Memory with 00 Byte Pattern at offset 2h === */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0xa5a5a500)
        {
            return -9;
        }
        else
        {
            MEM8_BASE[i * 4 + 2] = 0x00;
        }
    }

    /* === Read Check then Fill Memory with 00 Byte Pattern at offset 1h === */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0xa500a500)
        {
            return -10;
        }
        else
        {
            MEM8_BASE[i * 4 + 1] = 0x00;
        }
    }

    /* === Read Check then Fill Memory with 00 Byte Pattern at offset 3h === */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0xa5000000)
        {
            return -11;
        }
        else
        {
            MEM8_BASE[i * 4 + 3] = 0x00;
        }
    }

    /* === Read Check then Fill Memory with ffff Word Pattern at offset 1h == */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0x00000000)
        {
            return -12;
        }
        else
        {
            MEM16_BASE[i * 2 + 1] = 0xffff;
        }
    }


    /* === Read Check then Fill Memory with ffff Word Pattern at offset 0h == */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0xffff0000)
        {
            return -13;
        }
        else
        {
            MEM16_BASE[i * 2] = 0xffff;
        }
    }


    /*===  Read Check === */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0xffffffff)
        {
            return -14;
        }
    }


    /************************************************
    * Additional verification 
    ************************************************/
    /* === stage 1 => write 0 === */

    for (i = 0; i < size; i++)
    {
        MEM_BASE[i] = PATTERN1;
    }


    /* === stage 2 => read 0, write 0xF === */
    for (i = 0; i < size; i++)
    {
        value = MEM_BASE[i];

        if (value != PATTERN1)
        {
            return -15;
        }
        MEM_BASE[i] = PATTERN2;
    }


    /* === stage 3 => read 0xF, write 0 === */
    for (i = 0; i < size; i++)
    {
        value = MEM_BASE[i];
        if (value != PATTERN2)
        {
            return -16;
        }
        MEM_BASE[i] = PATTERN1;
    }


    /* === stage 4 => read 0, write 0xF === */
    for (i = 0; i < size; i++)
    {
        value = MEM_BASE[i];
        if (value != PATTERN1)
        {
            return -17;
        }
        MEM_BASE[i] = PATTERN2;
    }


    /* === stage 5 => read 0xF, write 0 === */
    for (i = 0; i < size; i++)
    {
        value = MEM_BASE[i];
        if (value != PATTERN2)
        {
            return -18;
        }
        MEM_BASE[i] = PATTERN1;
    }


    /* === stage 6 => read 0 === */
    for (i = 0; i < size; i++)
    {
        value = MEM_BASE[i];
        if (value != PATTERN1)
        {
            return -19;
        }
    }


    /* === 1/2/4-byte combination test === */
    i = (unsigned int) MEM_BASE;

    while (i < (unsigned int) MEM_BASE + (size << 2))
    {
        *((unsigned char *) i) = 0x78;
        i += 1;
        *((unsigned char *) i) = 0x56;
        i += 1;
        *((unsigned short *) i) = 0x1234;
        i += 2;
        *((unsigned int *) i) = 0x12345678;
        i += 4;
        *((unsigned short *) i) = 0x5678;
        i += 2;
        *((unsigned char *) i) = 0x34;
        i += 1;
        *((unsigned char *) i) = 0x12;
        i += 1;
        *((unsigned int *) i) = 0x12345678;
        i += 4;
        *((unsigned char *) i) = 0x78;
        i += 1;
        *((unsigned char *) i) = 0x56;
        i += 1;
        *((unsigned short *) i) = 0x1234;
        i += 2;
        *((unsigned int *) i) = 0x12345678;
        i += 4;
        *((unsigned short *) i) = 0x5678;
        i += 2;
        *((unsigned char *) i) = 0x34;
        i += 1;
        *((unsigned char *) i) = 0x12;
        i += 1;
        *((unsigned int *) i) = 0x12345678;
        i += 4;
    }
    for (i = 0; i < size; i++)
    {
        value = MEM_BASE[i];
        if (value != 0x12345678)
        {
            return -20;
        }
    }


    /* === Verify pattern 1 (0x00~0xff) === */
    pattern8 = 0x00;
    MEM8_BASE[0] = pattern8;
    for (i = 0; i < size * 4; i++)
    {
        unsigned char waddr8, raddr8;
        waddr8 = i + 1;
        raddr8 = i;
        if (i < size * 4 - 1)
            MEM8_BASE[waddr8] = pattern8 + 1;
        if (MEM8_BASE[raddr8] != pattern8)
        {
            return -21;
        }
        pattern8++;
    }


    /* === Verify pattern 2 (0x00~0xffff) === */
    pattern16 = 0x00;
    MEM16_BASE[0] = pattern16;
    for (i = 0; i < size * 2; i++)
    {
        if (i < size * 2 - 1)
            MEM16_BASE[i + 1] = pattern16 + 1;
        if (MEM16_BASE[i] != pattern16)
        {
            return -22;
        }
        pattern16++;
    }


    /* === Verify pattern 3 (0x00~0xffffffff) === */
    pattern32 = 0x00;
    MEM32_BASE[0] = pattern32;
    for (i = 0; i < size; i++)
    {
        if (i < size - 1)
            MEM32_BASE[i + 1] = pattern32 + 1;
        if (MEM32_BASE[i] != pattern32)
        {
            return -23;
        }
        pattern32++;
    }

    return 0;
}

#endif

//====================================================
#ifdef DRAMC_DEBUG_FULL_MEMORY_SCAN
#define PATTERN1 0x5A5A5A5A
#define PATTERN2 0xA5A5A5A5
#define _errorExit(errcode) return(errcode)
#define dbg_print print
static unsigned int u4Process = 0x00;
void mem_test_show_process(unsigned int index, unsigned int len)
{
#if 1
	 unsigned int u4NewProcess, u4OldProcess;

     u4NewProcess = (unsigned long long)index*100/(unsigned long long)len;
	// dbg_print("0x%x : 0x%x : 0x%x : %d\n", index,len, len-1, u4NewProcess);	
	
	 if(index == 0x00) dbg_print("0%%");
     if(u4NewProcess != u4Process)
     {
     
	// dbg_print("1: 0x%x : 0x%x : 0x%x : %d : %d\n", index,len, len-1, u4NewProcess, u4Process);	
		u4Process = u4NewProcess;
		if((u4Process%10) == 0x00)
		{
			dbg_print("\n%d%%", u4Process); 
		}
		else
		{
			dbg_print(". "); 
		}		
     }

	 if((index != 0x00) && (index == (unsigned int)(len - 1)))
	 {
		u4Process = 0x00;
		 dbg_print("\n100%% \n");		
	 }
#endif
}

int complex_mem_test1(unsigned int start, unsigned int len)
{
    volatile unsigned char *MEM8_BASE = (volatile unsigned char *) start;
    volatile unsigned short *MEM16_BASE = (volatile unsigned short *) start;
    volatile unsigned int *MEM32_BASE = (volatile unsigned int *) start;
    volatile unsigned int *MEM_BASE = (volatile unsigned int *) start;
	volatile unsigned long long *MEM64_BASE = (volatile unsigned long long *)start; 
    unsigned char pattern8;
    unsigned short pattern16;
    unsigned long long pattern64;
    unsigned int i, j, size, pattern32;
    unsigned int value, temp;

    size = len >> 2;
	
	*(volatile unsigned int *)0x10007000 = 0x22000000;
    dbg_print("memory test start address = 0x%x, test length = 0x%x\n", start, len);
	
    /* === Verify the tied bits (tied low) === */
    for (i = 0; i < size; i++)
    {
        MEM32_BASE[i] = 0;
    }

    for (i = 0; i < size; i++)
    {
		mem_test_show_process(i, size);    
        if (MEM32_BASE[i] != 0)
        {
            dbg_print("Tied Low Test: Address %x not all zero, %x!\n\r", &MEM32_BASE[i], MEM32_BASE[i]);
            dbg_print("....32bits all zero test: Fail!\n\r");              
            _errorExit(1);
        }
        else
        {
            MEM32_BASE[i] = 0xffffffff;
        }
    }
    if(i == size)	dbg_print("..32bits all zero test: Pass!\n\r"); 


    /* === Verify the tied bits (tied high) === */
    for (i = 0; i < size; i++)
    {
    	mem_test_show_process(i, size); 
        temp = MEM32_BASE[i];
        if(temp != 0xffffffff)
        {
            dbg_print("Tied High Test: Address %x not equal 0xFFFFFFFF, %x!\n\r", &MEM32_BASE[i], temp);
            dbg_print("....32bits all one test: Fail!\n\r");  
            _errorExit(2);
        }
        else
        { 
            MEM32_BASE[i] = 0x00;
    }
    }
    if(i == size)	dbg_print("..32bits all one test: Pass!\n\r");

    /* === Verify pattern 1 (0x00~0xff) === */
    pattern8 = 0x00;
    for (i = 0; i < len; i++)
        MEM8_BASE[i] = pattern8++;
     
    pattern8 = 0x00;
    for (i = 0; i < len; i++)
    {
    	mem_test_show_process(i, len);
        if (MEM8_BASE[i] != pattern8++)
        {
            dbg_print("Address %x = %x, %x is expected!\n\r", &MEM8_BASE[i], MEM8_BASE[i], --pattern8);
            dbg_print("....8bits 0x00~0xff pattern test: Fail!\n\r");            
            _errorExit(3);
        }
    }
    if(i == len)
        dbg_print("..8bits 0x00~0xff pattern test: Pass!\n\r");


    /* === Verify pattern 3 (0x0000, 0x0001, 0x0002, ... 0xFFFF) === */
    pattern16 = 0x00;
    for (i = 0; i < (len >> 1); i++)
        MEM16_BASE[i] = pattern16++;
                
    pattern16 = 0x00;
    for (i = 0; i < (len >> 1); i++)
    {
    	mem_test_show_process(i, (len >> 1));
        temp = MEM16_BASE[i];
        if(temp != pattern16++)
        {

            dbg_print("Address %x = %x, %x is expected!\n\r", &MEM16_BASE[i], temp, --pattern16);
            dbg_print("....16bits 0x00~0xffff pattern test: Fail!\n\r");            
            _errorExit(4);
        }
    }
    if(i == (len >> 1))
        dbg_print("..16bits 0x00~0xffff pattern test: Pass!\n\r");

    /* === Verify pattern 4 (0x00000000, 0x00000001, 0x00000002, ... 0xFFFFFFFF) === */
    pattern32 = 0x00;
    for (i = 0; i < (len >> 2); i++)
        MEM32_BASE[i] = pattern32++;
    pattern32 = 0x00;
    for (i = 0; i < (len >> 2); i++)
    {
    	mem_test_show_process(i, size);
        if (MEM32_BASE[i] != pattern32++)
        {
            dbg_print("Address %x = %x, %x is expected!\n\r", &MEM32_BASE[i], MEM32_BASE[i], --pattern32);
            dbg_print("....32bits 0x00~0xffffffff pattern test: Fail!\n\r");            
            _errorExit(5);
        }
    }
    if(i == (len >> 2))
        dbg_print("..32bits 0x00~0xffffffff pattern test: Pass!\n\r");    


    /* === Pattern 5: Filling memory range with 0xa5a5a5a5 === */
    for (i = 0; i < size; i++)
        MEM32_BASE[i] = 0xa5a5a5a5;

    /* === Read Check then Fill Memory with 00 Byte Pattern at offset 0h === */
    for (i = 0; i < size; i++)
    {
    	mem_test_show_process(i, size);
        if (MEM32_BASE[i] != 0xa5a5a5a5)
        {
            dbg_print("Address %x = %x, 0xa5a5a5a5 is expected!\n\r", &MEM32_BASE[i], MEM32_BASE[i]);
            dbg_print("....0xa5a5a5a5 pattern test: Fail!\n\r");            
            _errorExit(6);
        }
        else
        {
            MEM8_BASE[i * 4] = 0x00;
        }
    }
    if(i == size)
        dbg_print("..0xa5a5a5a5 pattern test: Pass!\n\r");     

    /* === Read Check then Fill Memory with 00 Byte Pattern at offset 2h === */
    for (i = 0; i < size; i++)
    {
    	mem_test_show_process(i, size);
        if (MEM32_BASE[i] != 0xa5a5a500)
        {
            dbg_print("Address %x = %x, 0xa5a5a500 is expected!\n\r", &MEM32_BASE[i], MEM32_BASE[i]);
            dbg_print("....0xa5a5a500 pattern test: Fail!\n\r");            
            _errorExit(7);
        }
        else
        {
            MEM8_BASE[i * 4 + 2] = 0x00;
        }
    }
    if(i == size)
        dbg_print("..0xa5a5a500 pattern test: Pass!\n\r");     
    

    /* === Read Check then Fill Memory with 00 Byte Pattern at offset 1h === */
    for (i = 0; i < size; i++)
    {
    	mem_test_show_process(i, size);
        if (MEM32_BASE[i] != 0xa500a500)
        {
            dbg_print("Address %x = %x, 0xa500a500 is expected!\n\r", &MEM32_BASE[i], MEM32_BASE[i]);
            dbg_print("....0xa500a500 pattern test: Fail!\n\r");            
            _errorExit(8);
        }
        else
        {
            MEM8_BASE[i * 4 + 1] = 0x00;
        }
    }
    if(i == size)
        dbg_print("..0xa500a500 pattern test: Pass!\n\r");    
    

    /* === Read Check then Fill Memory with 00 Byte Pattern at offset 3h === */
    for (i = 0; i < size; i++)
    {
    	mem_test_show_process(i, size);
        if (MEM32_BASE[i] != 0xa5000000)
        {
            dbg_print("Address %x = %x, 0xa5000000 is expected!\n\r", &MEM32_BASE[i], MEM32_BASE[i]);
            dbg_print("....0xa5000000 pattern test: Fail!\n\r");            
            _errorExit(9);
        }
        else
        {
            MEM8_BASE[i * 4 + 3] = 0x00;
        }
    }
    if(i == size)
        dbg_print("..0xa5000000 pattern test: Pass!\n\r");    
    

    /* === Read Check then Fill Memory with ffff Word Pattern at offset 1h === */   
    for (i = 0; i < size; i++)
    {
    	mem_test_show_process(i, size);
        if (MEM32_BASE[i] != 0x00000000)
        {
            dbg_print("Address %x = %x, 0x00000000 is expected!\n\r", &MEM32_BASE[i], MEM32_BASE[i]);
            dbg_print("....0x00000000 pattern test: Fail!\n\r");            
            _errorExit(10);
        }
    }
    if(i == size)
        dbg_print("..0x00000000 pattern test: Pass!\n\r");    

    /************************************************
    * Additional verification 
    ************************************************/
    /* === stage 1 => write 0 === */
    for (i = 0; i < size; i++)
    {
        MEM_BASE[i] = PATTERN1;
    }

    /* === stage 2 => read 0, write 0xF === */
    for (i = 0; i < size; i++)
    {
    	mem_test_show_process(i, size);
        value = MEM_BASE[i];
        if (value != PATTERN1)
        {
            dbg_print("\nStage 2 error. Addr = %x, value = %x\n", &(MEM_BASE[i]), value);            
            _errorExit(11);
        }
        MEM_BASE[i] = PATTERN2;
    }
	
    /* === stage 3 => read F === */
    for (i = 0; i < size; i++)
    {
    	mem_test_show_process(i, size);
        value = MEM_BASE[i];
        if (value != PATTERN2)
        {
            dbg_print("\nStage 3 error. Addr = %x, value = %x\n", &(MEM_BASE[i]), value);
            _errorExit(12);
        }
    }
	
    dbg_print("..%x and %x Interleaving test: Pass!\n\r", PATTERN1, PATTERN2);    
             
    return 0;
}
#endif
//====================================================

