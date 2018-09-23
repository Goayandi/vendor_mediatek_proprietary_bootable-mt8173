// 6595_DDR.cpp : Defines the entry point for the console application.
//

#include <emi.h>
#include <typedefs.h>
#include "dramc_common.h"
#include "dramc_register.h"
#include "dramc_pi_api.h"
#include "partition.h"
#include "emi.h"
#include "platform.h"
#include "custom_emi2.h"
extern u32 seclib_get_devinfo_with_index(u32 index);
#ifdef DRAMC_DEBUG_TIME
Dramc_debug_time_struct gs_Dramc_Debug_Time;
#endif
int io_width;
DRAMC_CTX_T *psCurrDramCtx;

DRAMC_CTX_T DramCtx_LPDDR3 =
{
	CHANNEL_A,		// DRAM_CHANNEL
	TYPE_LPDDR3,	// DRAM_DRAM_TYPE_T
	PACKAGE_SBS,	// DRAM_PACKAGE_T
	DATA_WIDTH_32BIT,		// DRAM_DATA_WIDTH_T
	DEFAULT_TEST2_1_CAL, 	// test2_1;
	DEFAULT_TEST2_2_CAL,	// test2_2;
	TEST_XTALK_PATTERN,	// test_pattern;
#ifdef DUAL_FREQ_K
	DUAL_FREQ_LOW,
#else
    #ifdef DDR_667
        333,
    #elif defined (DDR_800)
        400,
    #elif defined (DDR_1066)
    	533,
    #elif defined (DDR_1333)
    	666,
    #elif defined (DDR_1600)
    	800,
    #elif defined (DDR_1780)
    	890,
    #elif defined (DDR_1866)
    	933,
    #elif defined (DDR_2000)
    	1000,
    #elif defined (DDR_2133)
    	1066,
    #elif defined (DDR_1420)
    	710,
    #elif defined (DDR_2400)
    	1200,
    #elif defined (DDR_1792)
	896,
    #else
    	890,
    #endif
#endif

	533,			// frequency_low;
	DISABLE,		// fglow_freq_write_en;
	DISABLE,	// ssc_en;
	DISABLE		// en_4bitMux;
};

DRAMC_CTX_T DramCtx_PCDDR3 =
{
	CHANNEL_A,		// DRAM_CHANNEL
	TYPE_PCDDR3,		// DRAM_DRAM_TYPE_T
	PACKAGE_SBS,	// DRAM_PACKAGE_T
	DATA_WIDTH_32BIT,		// DRAM_DATA_WIDTH_T
	DEFAULT_TEST2_1_CAL, 	// test2_1;
	DEFAULT_TEST2_2_CAL,	// test2_2;
	TEST_XTALK_PATTERN,	// test_pattern; Audio or Xtalk.
	900, // frequency;
	533,			// frequency_low;
	ENABLE,		// fglow_freq_write_en;
	DISABLE,	// ssc_en;
	DISABLE		// en_4bitMux;
};

#ifndef COMBO_MCP

#ifdef DDRTYPE_LPDDR2
#define    EMI_CONA_VAL     LPDDR2_EMI_CONA
#endif

#ifdef DDRTYPE_LPDDR3
//#define    EMI_CONA_VAL         0x2A3AE
#define    EMI_CONA_VAL     LPDDR3_EMI_CONA
#endif

#ifdef LPDDR3_EMI_CONH
    #define    EMI_CONH_VAL     LPDDR3_EMI_CONH
#else
    #define    EMI_CONH_VAL     0
#endif

#endif //ifndef COMBO_MCP

static int enable_combo_dis = 0;
extern int num_of_emi_records;
extern EMI_SETTINGS emi_settings[];
#define TIMEOUT 3
extern unsigned int g_ddr_reserve_enable;
extern unsigned int g_ddr_reserve_success;

//==============================================================
#define PMIC_6397_VCORE_VMEM

#if defined(DDR_1792) ||defined(DDR_1866)
#define VcHV_VmHV  0	//HV
#define VcNV_VmNV  1	//NV
#define VcLV_VmLV  0	//LV
#else
#define VcHV_VmHV  0	//HV
#define VcLV_VmNV  1	//NV
#define VcLLV_VmLV 0	//LV
#endif

#define Vcore_HV_LPPDR3  (0x48)   //1.150V
#define Vcore_NV_LPPDR3  (0x44)   //1.125V
#define Vcore_LV_LPPDR3  (0x34)   //1.025V
#define Vcore_LLV_LPPDR3 (0x25)   //0.931V

#define Vmem_HV_LPDDR3 	 (0x50)   //1.300V
#define Vmem_NV_LPDDR3   (0x44)   //1.225V
#define Vmem_LV_LPDDR3   (0x36)   //1.138V

#define VCORE_CON9      0x0278
#define VCORE_CON10     0x027A
#define VDRM_CON9       0x038A
#define VDRM_CON10      0x038C

void pmic_Vcore_adjust(int nAdjust)
{
#ifdef PMIC_6397_VCORE_VMEM

    unsigned int OldVcore1 = 0;
    unsigned int OldVcore2 = 0;

	#ifdef DRAMC_DEBUG_RETRY
	if(_emi_retry)	nAdjust = 0x01;//retry need set to HV
	#endif

    switch(nAdjust)
	{
	case 1 :        // HV. Vcore 1.15V
            pmic_config_interface(VCORE_CON9, Vcore_HV_LPPDR3, 0x7F,0);
            pmic_config_interface(VCORE_CON10,Vcore_HV_LPPDR3, 0x7F,0);
            break;
        case 2 :        // NV. Vcore 1.125V
            pmic_config_interface(VCORE_CON9, Vcore_NV_LPPDR3, 0x7F,0);
            pmic_config_interface(VCORE_CON10,Vcore_NV_LPPDR3, 0x7F,0);
            break;
        case 3 :        // LV. Vcore 1.025V
            pmic_config_interface(VCORE_CON9, Vcore_LV_LPPDR3, 0x7F,0);
            pmic_config_interface(VCORE_CON10,Vcore_LV_LPPDR3, 0x7F,0);
            break;
	case 4 :		// LLV. Vcore 0.931V
		pmic_config_interface(VCORE_CON9, Vcore_LLV_LPPDR3, 0x7F,0);
		pmic_config_interface(VCORE_CON10,Vcore_LLV_LPPDR3, 0x7F,0);
		break;
	case 5 :        // Vcore ++
        pmic_read_interface(VCORE_CON9,&OldVcore1, 0x7F,0);
        pmic_read_interface(VCORE_CON10,&OldVcore1, 0x7F,0);
        pmic_config_interface(VCORE_CON9, OldVcore1+1, 0x7F,0);
        pmic_config_interface(VCORE_CON10, OldVcore1+1, 0x7F,0);
        break;
	case 6 :        // Vcore --
        pmic_read_interface(VCORE_CON9,&OldVcore1, 0x7F,0);
        pmic_read_interface(VCORE_CON10,&OldVcore1, 0x7F,0);
        pmic_config_interface(VCORE_CON9, OldVcore1-1, 0x7F,0);
        pmic_config_interface(VCORE_CON10, OldVcore1-1, 0x7F,0);
        break;
        default :
            pmic_config_interface(VCORE_CON9, Vcore_NV_LPPDR3, 0x7F,0);
            pmic_config_interface(VCORE_CON10,Vcore_NV_LPPDR3, 0x7F,0);
            break;
    }
#endif
}

void pmic_Vmem_adjust(int nAdjust)
{
#ifdef PMIC_6397_VCORE_VMEM

    unsigned int OldVmem1 = 0;
    unsigned int OldVmem2 = 0;

	#ifdef DRAMC_DEBUG_RETRY
	if(_emi_retry)	nAdjust = 0x01;//retry need set to HV
	#endif

    switch(nAdjust)
	{
        case 1 :        // HV. Vcore 1.3V
            pmic_config_interface(VDRM_CON9, Vmem_HV_LPDDR3, 0x7F,0);
            pmic_config_interface(VDRM_CON10,Vmem_HV_LPDDR3, 0x7F,0);
            break;
        case 2 :        // NV. Vcore 1.225V
            pmic_config_interface(VDRM_CON9, Vmem_NV_LPDDR3, 0x7F,0);
            pmic_config_interface(VDRM_CON10,Vmem_NV_LPDDR3, 0x7F,0);
            break;
        case 3 :        // LV. Vcore 1.138V
            pmic_config_interface(VDRM_CON9, Vmem_LV_LPDDR3, 0x7F,0);
            pmic_config_interface(VDRM_CON10,Vmem_LV_LPDDR3, 0x7F,0);
            break;
	case 5 :        //Vmem ++
	    pmic_read_interface(VDRM_CON9,&OldVmem1, 0x7F,0);
	    pmic_read_interface(VDRM_CON10,&OldVmem1, 0x7F,0);
	    pmic_config_interface(VDRM_CON9, OldVmem1+1, 0x7F,0);
	    pmic_config_interface(VDRM_CON10, OldVmem1+1, 0x7F,0);
	    break;
	case 6 :        //Vmem --
	    pmic_read_interface(VDRM_CON9,&OldVmem1, 0x7F,0);
	    pmic_read_interface(VDRM_CON10,&OldVmem1, 0x7F,0);
	    pmic_config_interface(VDRM_CON9, OldVmem1-1, 0x7F,0);
	    pmic_config_interface(VDRM_CON10, OldVmem1-1, 0x7F,0);
	    break;
        default :
            pmic_config_interface(VDRM_CON9, Vmem_NV_LPDDR3, 0x7F,0);
            pmic_config_interface(VDRM_CON10,Vmem_NV_LPDDR3, 0x7F,0);
            break;
    }
#endif
}

void pmic_voltage_read(UINT8 temp)
{
#ifdef PMIC_6397_VCORE_VMEM
    int ret_val = 0;
    unsigned int OldVcore1 = 0;
    unsigned int OldVcore2 = 0;
    unsigned int OldVmem1 = 0;
    unsigned int OldVmem2 = 0;

    print("[PMIC]pmic_voltage_read : \r\n");

    ret_val=pmic_read_interface(VCORE_CON9,&OldVcore1,0x7F,0);
    ret_val=pmic_read_interface(VDRM_CON9,&OldVmem1, 0x7F,0);

    print("[Vcore]VCORE_CON9=0x%x,\r\n[Vmem] VDRM_CON9=0x%x \r\n", OldVcore1,OldVmem1);
#endif
}

#ifdef DRAMC_DEBUG_RETRY
unsigned int _emi_retry = 0x00;
void emi_retry_adjust(int nAdjust)
{
	print("[emi]retry adjust : %d\r\n", nAdjust);
 	switch(nAdjust)
	{
        case 0 :
			pmic_Vcore_adjust(1);
			pmic_Vmem_adjust(1);
			print("[retry] LPDDR3 HV : Vcore = 1.15V Vmem = 1.30V \r\n");
            break;
        case 1 :
            break;
        case 2 :
            break;
        default :
            break;
    }
}
#endif

//==============================================================
#ifdef COMBO_MCP
void EMI_Init(DRAMC_CTX_T *p, EMI_SETTINGS* emi_set)
#else
void EMI_Init(DRAMC_CTX_T *p)
#endif
{
	//----------------EMI Setting--------------------
	*(volatile unsigned *)(EMI_APB_BASE+0x00000028)=0x08420000;  //Yi-chih's command
	*(volatile unsigned *)(EMI_APB_BASE+0x00000060)=0x40000500;  //disable EMI top DCM
	*(volatile unsigned *)(EMI_APB_BASE+0x00000140)=0x20406188;//0x12202488;   // 83 for low latency
	*(volatile unsigned *)(EMI_APB_BASE+0x00000144)=0x20406188;//0x12202488; //new add
	/*
	*(volatile unsigned *)(EMI_APB_BASE+0x00000100)=0x40107a06;//0x40105808; //m0 cpu ori:0x8020783f
	// *(volatile unsigned *)(EMI_APB_BASE+0x00000108)=0x808070ea;//0xa0a05028; //m1 ori:0x80200000 // ultra can over limit
	*(volatile unsigned *)(EMI_APB_BASE+0x00000110)=0x808070d5; m2
	*(volatile unsigned *)(EMI_APB_BASE+0x00000118)=0x0810784a;//0x030fd80d; // ???????????????????????????????????????????????????????????//m3 mdmcu ori:0x80807809 ori:0x07007010 bit[12]:enable bw limiter
	*(volatile unsigned *)(EMI_APB_BASE+0x00000120)=0x30407042; //m4 fcore ori:0x8080781a
	*(volatile unsigned *)(EMI_APB_BASE+0x00000128)=0x808070d5; //m5 MM
	*(volatile unsigned *)(EMI_APB_BASE+0x00000130)=0x80807045; //m6 vcodec ori:0x8080381a

	*/
	*((UINT32P)(EMI_APB_BASE+0x00000100))= 0x7f077a49;
	*((UINT32P)(EMI_APB_BASE+0x00000110))= 0xa0a070dd;
	*((UINT32P)(EMI_APB_BASE+0x00000118))= 0x07007046;
	*((UINT32P)(EMI_APB_BASE+0x00000120))= 0x40407046;
	*((UINT32P)(EMI_APB_BASE+0x00000128))= 0xa0a070c6;
	//*((UINT32P)(EMI_APB_BASE+0x00000130))= 0xffff7047;
	*((UINT32P)(EMI_APB_BASE+0x00000130))= 0x404070ff;
	*((UINT32P)(EMI_APB_BASE+0x00000138))= 0x404070ff;

	*(volatile unsigned *)(EMI_APB_BASE+0x00000148)=0x9719595e;//0323 chg, ori :0x00462f2f
	*(volatile unsigned *)(EMI_APB_BASE+0x0000014c)=0x9719595e; // new add
//	*(volatile unsigned *)(EMI_APB_BASE+0x00000000)=0x20202027; // DUAL CHANEEL

#ifdef COMBO_MCP
	*(volatile unsigned *)(EMI_APB_BASE+0x00000000)=emi_set->EMI_CONA_VAL;
#else
	if (p->dram_type==TYPE_LPDDR3)
	{
		*(volatile unsigned *)(EMI_APB_BASE+0x00000000)=LPDDR3_EMI_CONA;
	}
	else
	{
		*(volatile unsigned *)(EMI_APB_BASE+0x00000000)=PCDDR3_EMI_CONA;
	}
#endif
	*(volatile unsigned *)(EMI_APB_BASE+0x000000f8)=0x00000000;
	*(volatile unsigned *)(EMI_APB_BASE+0x00000400)=0x00ff0001;
	*(volatile unsigned *)(EMI_APB_BASE+0x00000008)=0x17283544;
	*(volatile unsigned *)(EMI_APB_BASE+0x00000010)=0x0a1a0b1a;
	*(volatile unsigned *)(EMI_APB_BASE+0x00000018)=0x00000000; //SMI threthold
	*(volatile unsigned *)(EMI_APB_BASE+0x00000020)=0xFFFF0848;
	*(volatile unsigned *)(EMI_APB_BASE+0x00000030)=0x2b2b2a38;
#ifdef COMBO_MCP
    *(volatile unsigned *)(EMI_APB_BASE+0x00000038)=emi_set->EMI_CONH_VAL;
#else
	#ifdef LPDDR3_EMI_CONH
	*(volatile unsigned *)(EMI_APB_BASE+0x00000038)=LPDDR3_EMI_CONH;
	#else
	*(volatile unsigned *)(EMI_APB_BASE+0x00000038)=0x00000000;
	#endif
#endif
	*(volatile unsigned *)(EMI_APB_BASE+0x00000158)=0x00010800;// ???????????????????????0x08090800;
	*(volatile unsigned *)(EMI_APB_BASE+0x00000078)=0x80030303;// ???????????0x00030F4d;
	*(volatile unsigned *)(EMI_APB_BASE+0x0000015c)=0x80030303;// ??????????????????????????? 0x00030F4d;

	*(volatile unsigned *)(EMI_APB_BASE+0x00000150)=0x64f3fc79;
	*(volatile unsigned *)(EMI_APB_BASE+0x00000154)=0x64f3fc79;




	//==============Scramble address==========================
	//============== Defer WR threthold
	*(volatile unsigned *)(EMI_APB_BASE+0x000000f0)=0x38470000;
	//============== Reserve bufer
	//
	//MDMCU don't always ULTRA, but small age
	#ifdef SBR
	*(volatile unsigned *)(EMI_APB_BASE+0x00000078)=0x3422cc3f;// defer ultra excpt MDMCU
	*(volatile unsigned *)(EMI_APB_BASE+0x000000f8)=0x00006000;// LPDDR3
	#else
	*(volatile unsigned *)(EMI_APB_BASE+0x00000078)=0x34220c3f;// defer ultra excpt MDMCU
	#endif
	#ifdef SBR
		#ifdef PBC_MASK
	        *(volatile unsigned *)(EMI_APB_BASE+0x000000e8)=0x00060124;// LPDDR3
	        #else
	        *(volatile unsigned *)(EMI_APB_BASE+0x000000e8)=0x00060324;// LPDDR3
		#endif
	#else
	*(volatile unsigned *)(EMI_APB_BASE+0x000000e8)=0x00060124;// LPDDR3
	#endif
	// Turn on M1 Ultra and all port DRAMC hi enable
	*(volatile unsigned *)(EMI_APB_BASE+0x00000158)=0xff03ff00;// ???????????????????????0x08090800;
	// RFF)_PBC_MASK; [9] decrease noSBR push to DRAMC

	// Page hit is high
	*(volatile unsigned *)(EMI_APB_BASE+0x00000060)=0x400005ff;
	*(volatile unsigned *)(EMI_APB_BASE+0x000000d0)=0xCCCCCCCC;//R/8 W/8 outstanding
	*(volatile unsigned *)(EMI_APB_BASE+0x000000d8)=0xcccccccc;//R/8 W/8 outstanding

	// check RESP error
	//*((UINT32P)(EMI_APB_BASE+0x000001c0))=0x10000000;
	//*((UINT32P)(EMI_APB_BASE+0x000001c8))=0x10000000;
	//*((UINT32P)(EMI_APB_BASE+0x000001d0))=0x10000000;
	//*((UINT32P)(EMI_APB_BASE+0x00000200))=0x10000000;

        //*(volatile unsigned *)(EMI_APB_BASE+0x100)=0x7F007A49;
	//===========END===========================================
#ifdef SBR_TEST
    *(volatile unsigned *)(EMI_APB_BASE+0x78)=0x3422fc3f;
    *(volatile unsigned *)(EMI_APB_BASE+0xf8)=0x6000;
    *(volatile unsigned *)(EMI_APB_BASE+0xe8)=0x603a7;
    *(volatile unsigned *)(EMI_APB_BASE+0x158)=0xff03ff00;
    *(volatile unsigned *)(EMI_APB_BASE+0x100)=0x00007845;
    *(volatile unsigned *)(EMI_APB_BASE+0x110)=0xa0a070d3;
    *(volatile unsigned *)(EMI_APB_BASE+0x128)=0xa0a070d3;
    *(volatile unsigned *)(EMI_APB_BASE+0x130)=0xffff704b;
#endif
}

void CHA_HWGW_Print(DRAMC_CTX_T *p)
{
	static U8 LowFreq_Min_R0_DQS[4] = {0xff, 0xff, 0xff, 0xff};
	static U8 LowFreq_Max_R0_DQS[4] = {0x00, 0x00, 0x00, 0x00};
	static U8 HighFreq_Min_R0_DQS[4] = {0xff, 0xff, 0xff, 0xff};
	static U8 HighFreq_Max_R0_DQS[4] = {0x00, 0x00, 0x00, 0x00};
	U8 ucstatus = 0, R0_DQS[4], Count;
	U32 u4value, u4value1;

	p->channel = CHANNEL_A;
	ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x374), &u4value);
	R0_DQS[0] = (u4value >> 0) & 0x7f;
	R0_DQS[1] = (u4value >> 8) & 0x7f;
	R0_DQS[2] = (u4value >> 16) & 0x7f;
	R0_DQS[3] = (u4value >> 24) & 0x7f;
	ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x94), &u4value1);

	mcSHOW_DBG_MSG(("[Channel %d]Clock=%d,Reg.94h=%xh,Reg.374h=%xh\n", p->channel, p->frequency, u4value1, u4value));

#ifdef DUAL_FREQ_TEST
	if (p->frequency == DUAL_FREQ_LOW)
	{
		for (Count=0; Count<4; Count++)
		{
			if (R0_DQS[Count] < LowFreq_Min_R0_DQS[Count])
			{
				LowFreq_Min_R0_DQS[Count] = R0_DQS[Count];
			}
			if (R0_DQS[Count]  > LowFreq_Max_R0_DQS[Count])
			{
				LowFreq_Max_R0_DQS[Count] = R0_DQS[Count];
			}
		}

		mcSHOW_DBG_MSG(("[Channel %d]Clock=%d,DQS0=(%d, %d),DQS1=(%d, %d),DQS2=(%d, %d),DQS3=(%d, %d)\n",
			p->channel, p->frequency,
			LowFreq_Min_R0_DQS[0], LowFreq_Max_R0_DQS[0], LowFreq_Min_R0_DQS[1], LowFreq_Max_R0_DQS[1],
			LowFreq_Min_R0_DQS[2], LowFreq_Max_R0_DQS[2], LowFreq_Min_R0_DQS[3], LowFreq_Max_R0_DQS[3]));
	}
	else
	{
		for (Count=0; Count<4; Count++)
		{
			if (R0_DQS[Count] < HighFreq_Min_R0_DQS[Count])
			{
				HighFreq_Min_R0_DQS[Count] = R0_DQS[Count];
			}
			if (R0_DQS[Count]  > HighFreq_Max_R0_DQS[Count])
			{
				HighFreq_Max_R0_DQS[Count] = R0_DQS[Count];
			}
		}
		mcSHOW_DBG_MSG(("[Channel %d]Clock=%d,DQS0=(%d, %d),DQS1=(%d, %d),DQS2=(%d, %d),DQS3=(%d, %d)\n",
			p->channel, p->frequency,
			HighFreq_Min_R0_DQS[0], HighFreq_Max_R0_DQS[0], HighFreq_Min_R0_DQS[1], HighFreq_Max_R0_DQS[1],
			HighFreq_Min_R0_DQS[2], HighFreq_Max_R0_DQS[2], HighFreq_Min_R0_DQS[3], HighFreq_Max_R0_DQS[3]));

	}
#else
	for (Count=0; Count<4; Count++)
	{
		if (R0_DQS[Count] < LowFreq_Min_R0_DQS[Count])
		{
			LowFreq_Min_R0_DQS[Count] = R0_DQS[Count];
		}
		if (R0_DQS[Count]  > LowFreq_Max_R0_DQS[Count])
		{
			LowFreq_Max_R0_DQS[Count] = R0_DQS[Count];
		}
	}
	mcSHOW_DBG_MSG(("[Channel %d]Clock=%d,DQS0=(%d, %d),DQS1=(%d, %d),DQS2=(%d, %d),DQS3=(%d, %d)\n",
		p->channel, p->frequency,
		LowFreq_Min_R0_DQS[0], LowFreq_Max_R0_DQS[0], LowFreq_Min_R0_DQS[1], LowFreq_Max_R0_DQS[1],
		LowFreq_Min_R0_DQS[2], LowFreq_Max_R0_DQS[2], LowFreq_Min_R0_DQS[3], LowFreq_Max_R0_DQS[3]));

#endif

#ifdef TEMP_SENSOR_ENABLE
	p->channel = CHANNEL_A;
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x03B8), &u4value);
        mcSHOW_ERR_MSG(("[CHA] MRR(MR4) Reg.3B8h[10:8]=%x\n", (u4value & 0x700)>>8));
	p->channel = CHANNEL_B;
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x03B8), &u4value);
        mcSHOW_ERR_MSG(("[CHB] MRR(MR4) Reg.3B8h[10:8]=%x\n", (u4value & 0x700)>>8));
#endif

}

static void Dump_Registers(DRAMC_CTX_T *p)
{
	U8 ucstatus = 0;
	U32 uiAddr;
	U32 u4value;

	if (p->channel == CHANNEL_A)
	{
		mcSHOW_DBG_MSG2(("Channel A registers dump...\n"));
	}
	else
	{
		mcSHOW_DBG_MSG2(("Channel B registers dump...\n"));
	}

	mcSHOW_DBG_MSG2(("EMI_CONA=%x\n",*(volatile unsigned *)(EMI_APB_BASE+0x00000000)));

	for (uiAddr=0x0; uiAddr<=0x690; uiAddr+=4)
	{
		ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(uiAddr), &u4value);
		mcSHOW_DBG_MSG2(("addr:%x, value:%x\n", uiAddr, u4value));

	}
}

static void Dump_EMI_Registers(void)
{
	U8 ucstatus = 0;
	U32 uiAddr;
	U32 u4value;

	for (uiAddr=0; uiAddr<=0x160; uiAddr+=8)
	{
	    u4value = 	(*(volatile unsigned int *)(EMI_APB_BASE + (uiAddr)));
		mcSHOW_DBG_MSG2(("addr:%x, value:%x\n", uiAddr, u4value));
	}
}

#ifdef COMBO_MCP
void do_calib(DRAMC_CTX_T *p, EMI_SETTINGS* emi_set, int skip_dual_freq_k)
#else
void do_calib(DRAMC_CTX_T *p, int skip_dual_freq_k)
#endif
{
#ifdef DRAMC_DEBUG_TIME
	U32 u4time = get_timer(0);
#endif
	U8 ucstatus = 0;
    U32 u4value;

#if defined(DDR_INIT_TIME_PROFILING)
    /* enable ARM CPU PMU */
    asm volatile(
        "MRC p15, 0, %0, c9, c12, 0\n"
        "BIC %0, %0, #1 << 0\n"   /* disable */
        "ORR %0, %0, #1 << 2\n"   /* reset cycle count */
        "BIC %0, %0, #1 << 3\n"   /* count every clock cycle */
        "MCR p15, 0, %0, c9, c12, 0\n"
        : "+r"(temp)
        :
        : "cc"
    );
    asm volatile(
        "MRC p15, 0, %0, c9, c12, 0\n"
        "ORR %0, %0, #1 << 0\n"   /* enable */
        "MCR p15, 0, %0, c9, c12, 0\n"
        "MRC p15, 0, %0, c9, c12, 1\n"
        "ORR %0, %0, #1 << 31\n"
        "MCR p15, 0, %0, c9, c12, 1\n"
        : "+r"(temp)
        :
        : "cc"
    );

    mcDELAY_US(100);

	/* get CPU cycle count from the ARM CPU PMU */
	asm volatile(
	    "MRC p15, 0, %0, c9, c12, 0\n"
	    "BIC %0, %0, #1 << 0\n"   /* disable */
	    "MCR p15, 0, %0, c9, c12, 0\n"
	    "MRC p15, 0, %0, c9, c13, 0\n"
	    : "+r"(temp)
	    :
	    : "cc"
	);
	opt_print(" mcDELAY_US(100) takes %d CPU cycles\n\r", temp);
#endif

    // not necessary, marked
    //DramcDiv2PhaseSync((DRAMC_CTX_T *) p);

#if defined(DDR_INIT_TIME_PROFILING)
    /* enable ARM CPU PMU */
    asm volatile(
        "MRC p15, 0, %0, c9, c12, 0\n"
        "BIC %0, %0, #1 << 0\n"   /* disable */
        "ORR %0, %0, #1 << 2\n"   /* reset cycle count */
        "BIC %0, %0, #1 << 3\n"   /* count every clock cycle */
        "MCR p15, 0, %0, c9, c12, 0\n"
        : "+r"(temp)
        :
        : "cc"
    );
    asm volatile(
        "MRC p15, 0, %0, c9, c12, 0\n"
        "ORR %0, %0, #1 << 0\n"   /* enable */
        "MCR p15, 0, %0, c9, c12, 0\n"
        "MRC p15, 0, %0, c9, c12, 1\n"
        "ORR %0, %0, #1 << 31\n"
        "MCR p15, 0, %0, c9, c12, 1\n"
        : "+r"(temp)
        :
        : "cc"
    );
#endif

// DDR_CHANNEL_INIT:

    p->channel = CHANNEL_A;
    DramcSwImpedanceCal((DRAMC_CTX_T *) p, 1);
    //DramcHwImpedanceCal((DRAMC_CTX_T *) p);

#ifdef DUAL_FREQ_K
#ifndef DRAMC_DEBUG_TEST_MENU
    // No need for preloader because should already 1.125V here.
	#ifdef DUAL_FREQ_DIFF_VOLTAGE
	// LV 1.025V in low freq
	pmic_Vcore_adjust(3);//Vcore 1.025V
	#else
	#if 1//defined(DDR_1792) ||defined(DDR_1866) //force 1.1V for display UHD limitation
	pmic_vcore_init();//pmic_Vcore_adjust(2);//Vcore 1.125V
	    #else
	pmic_Vcore_adjust(3);//Vcore 1.025V
        #endif
#endif
#endif
#endif
	// Run again here for different voltage. For preloader, if the following code is executed after voltage change, no need.
	#ifdef SPM_CONTROL_AFTERK
	TransferToRegControl();
	#endif

	MemPllPreInit(p);
	MemPllInit(p);
	mcDELAY_US(1);
	mt_mempll_cali(p);
	DramcDiv2PhaseSync((DRAMC_CTX_T *) p);

#ifdef DUAL_FREQ_K
DDR_CALI_START:
    p->channel = CHANNEL_A;
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_ACTIM1), &u4value);
    mcSET_FIELD(u4value, 0x0, MASK_ACTIM1_REFRCNT, POS_ACTIM1_REFRCNT);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_ACTIM1), u4value);
    p->channel = CHANNEL_B;
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_ACTIM1), &u4value);
    mcSET_FIELD(u4value, 0x0, MASK_ACTIM1_REFRCNT, POS_ACTIM1_REFRCNT);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_ACTIM1), u4value);
#endif

	{
#ifdef MATYPE_ADAPTATION
		// Backup here because Reg.04h may be modified based on different column address of different die or channel.
		// Default value should be the smallest number.
		ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x04), &u4Backup_Reg_04);
#endif

		// Calibration
#ifdef CA_WR_ENABLE
	    p->channel = CHANNEL_A;
		DramcCATraining((DRAMC_CTX_T *) p);
	    p->channel = CHANNEL_B;
	    DramcCATraining((DRAMC_CTX_T *) p);

#ifdef COMBO_MCP
	    p->channel = CHANNEL_A;
	    DramcWriteLeveling((DRAMC_CTX_T *) p, emi_set);
	    p->channel = CHANNEL_B;
		DramcWriteLeveling((DRAMC_CTX_T *) p, emi_set);
#else
	    p->channel = CHANNEL_A;
	    DramcWriteLeveling((DRAMC_CTX_T *) p);
	    p->channel = CHANNEL_B;
		DramcWriteLeveling((DRAMC_CTX_T *) p);
#endif
#endif
	#ifdef DUAL_RANKS
		if (uiDualRank)
		{
		    p->channel = CHANNEL_A;
		    DualRankDramcRxdqsGatingCal((DRAMC_CTX_T *) p);
		    p->channel = CHANNEL_B;
			DualRankDramcRxdqsGatingCal((DRAMC_CTX_T *) p);
		}
		else
		{
		    p->channel = CHANNEL_A;
		    DramcRxdqsGatingCal((DRAMC_CTX_T *) p);
		    p->channel = CHANNEL_B;
			DramcRxdqsGatingCal((DRAMC_CTX_T *) p);
		}
	#else
            p->channel = CHANNEL_A;
	        DramcRxdqsGatingCal((DRAMC_CTX_T *) p);
	        p->channel = CHANNEL_B;
		DramcRxdqsGatingCal((DRAMC_CTX_T *) p);
	#endif

		if (((DRAMC_CTX_T *) p)->fglow_freq_write_en==ENABLE)
		{
		    mcSHOW_DBG_MSG2(("**********************NOTICE*************************\n"));
			mcSHOW_DBG_MSG2(("Low speed write and high speed read calibration...\n"));
		    mcSHOW_DBG_MSG2(("*****************************************************\n"));
			// change low frequency and use test engine2 to write data, after write, recover back to the original frequency

		    // do channel A & B low frequency write simultaneously
		    CurrentRank = 0;
			DramcLowFreqWrite((DRAMC_CTX_T *) p);
	#ifdef DUAL_RANKS
		    if (uiDualRank)
		    {
			    CurrentRank = 1;
			    // Swap CS0 and CS1.
			    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x110), &u4value);
			    u4value = u4value |0x08;
			    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x110), u4value);

			    // do channel A & B low frequency write simultaneously
			    DramcLowFreqWrite((DRAMC_CTX_T *) p);

			    // Swap CS back.
			    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x110), &u4value);
			    u4value = u4value & (~0x08);
			    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x110), u4value);
			    CurrentRank = 0;
		    }
    #endif
        }
	#ifdef DUAL_RANKS
	        if (uiDualRank)
	        {
		        p->channel = CHANNEL_A;
		        DramcDualRankRxdatlatCal((DRAMC_CTX_T *) p);
		        p->channel = CHANNEL_B;
		        DramcDualRankRxdatlatCal((DRAMC_CTX_T *) p);
	        }
	        else
	        {
		        p->channel = CHANNEL_A;
		        DramcRxdatlatCal((DRAMC_CTX_T *) p);
		        p->channel = CHANNEL_B;
		        DramcRxdatlatCal((DRAMC_CTX_T *) p);
	        }
	#else
		    p->channel = CHANNEL_A;
		    DramcRxdatlatCal((DRAMC_CTX_T *) p);
		    p->channel = CHANNEL_B;
		    DramcRxdatlatCal((DRAMC_CTX_T *) p);
	#endif

	#ifdef RX_DUTY_CALIBRATION
	p->channel = CHANNEL_A;
	DramcClkDutyCal(p);
	p->channel = CHANNEL_B;
	DramcClkDutyCal(p);
	#endif

	p->channel = CHANNEL_A;
    DramcRxWindowPerbitCal((DRAMC_CTX_T *) p);
	p->channel = CHANNEL_B;
	DramcRxWindowPerbitCal((DRAMC_CTX_T *) p);

	p->channel = CHANNEL_A;
	DramcTxWindowPerbitCal((DRAMC_CTX_T *) p);
	p->channel = CHANNEL_B;
    DramcTxWindowPerbitCal((DRAMC_CTX_T *) p);
/*
		if (((DRAMC_CTX_T *) p)->fglow_freq_write_en==ENABLE)
		{
			// after TX calibration, use high speed write to to RX DQS per bit calibration again
			((DRAMC_CTX_T *) p)->fglow_freq_write_en = DISABLE;
			if (((DRAMC_CTX_T *) p)->test_pattern==TEST_AUDIO_PATTERN)
			{
				DramcRxWindowPerbitCal((DRAMC_CTX_T *) p);
			}

               // Enable "Low frequency write" for channel B calibration
		if (((DRAMC_CTX_T *) p)->channel == CHANNEL_A)
		{
		    ((DRAMC_CTX_T *) p)->fglow_freq_write_en = ENABLE;
		}
	} */

	// Set here in order to save for frequency jump.
	p->channel = CHANNEL_A;
	DramcRANKINCTLConfig(p);
	p->channel = CHANNEL_B;
	DramcRANKINCTLConfig(p);

#ifdef DUAL_FREQ_K
	//p->channel = CHANNEL_A;
        //print_DBG_info();
	//p->channel = CHANNEL_B;
        //print_DBG_info();

  if(skip_dual_freq_k != 1)
  {
	if (p->frequency == DUAL_FREQ_LOW)
	{
		DramcSaveFreqSetting(p);

#ifdef DUAL_FREQ_DIFF_VOLTAGE
		#ifndef DRAMC_DEBUG_TEST_MENU
		// switch to HV 1.125V in high freq
		#if 1//defined(DDR_1792) ||defined(DDR_1866) // force 1.1V for display UHD limitation
		pmic_vcore_init();//pmic_Vcore_adjust(2);//Vcore 1.125V
              #else
		pmic_Vcore_adjust(3);//Vcore 1.025V
		#endif
              #endif
#endif

        p->frequency = mt_get_dram_freq_setting();
        if(p->frequency == DUAL_FREQ_LOW)
            goto DDR_CALI_END;
		DramcSwitchFreq(p, 1);

	#if 1//defined(DUAL_FREQ_DIFF_ACTIMING) || defined(DUAL_FREQ_DIFF_RLWL)
	    #ifdef COMBO_MCP
	        p->channel = CHANNEL_B;
		    DramcPreInit((DRAMC_CTX_T *) p, emi_set);
		    p->channel = CHANNEL_A;
		    DramcPreInit((DRAMC_CTX_T *) p, emi_set);

		    DramcDiv2PhaseSync((DRAMC_CTX_T *) p);

		    p->channel = CHANNEL_B;
		    DramcInit((DRAMC_CTX_T *) p, emi_set);
		    p->channel = CHANNEL_A;
		    DramcInit((DRAMC_CTX_T *) p, emi_set);
	    #else
		    p->channel = CHANNEL_B;
		    DramcPreInit((DRAMC_CTX_T *) p);
		    p->channel = CHANNEL_A;
		    DramcPreInit((DRAMC_CTX_T *) p);

		    DramcDiv2PhaseSync((DRAMC_CTX_T *) p);

		    p->channel = CHANNEL_B;
		    DramcInit((DRAMC_CTX_T *) p);
		    p->channel = CHANNEL_A;
		    DramcInit((DRAMC_CTX_T *) p);
		#endif

	#endif
		goto DDR_CALI_START;
	}
	else
	{
		DramcSaveFreqSetting(p);

 	}
	//DramcDumpFreqSetting(p);
  } //!skip_dual_freq_k

#endif
#ifdef MATYPE_ADAPTATION
		ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x04), u4Backup_Reg_04);
#endif
	}
DDR_CALI_END:
#if defined(DDR_INIT_TIME_PROFILING)
	/* get CPU cycle count from the ARM CPU PMU */
	asm volatile(
	    "MRC p15, 0, %0, c9, c12, 0\n"
	    "BIC %0, %0, #1 << 0\n"   /* disable */
	    "MCR p15, 0, %0, c9, c12, 0\n"
	    "MRC p15, 0, %0, c9, c13, 0\n"
	    : "+r"(temp)
	    :
	    : "cc"
	);
	opt_print("DRAMC calibration takes %d CPU cycles\n\r", temp);
#endif


	p->channel = CHANNEL_A;
	DramcRunTimeConfig(p);
	p->channel = CHANNEL_B;
	DramcRunTimeConfig(p);

#ifdef SPM_CONTROL_AFTERK
	TransferToSPMControl();
#endif

#ifdef DRAMC_DEBUG_CALIB_LOG
	p->channel = CHANNEL_A;
	dramc_calib_step_test(p, STEP_DRAM_CALIB_DONE);
	p->channel = CHANNEL_B;
	dramc_calib_step_test(p, STEP_DRAM_CALIB_DONE);
#endif

#ifdef DRAMC_DEBUG_TIME
gs_Dramc_Debug_Time.u4Total_dram_calib = get_timer(u4time);
#endif

}

#ifdef COMBO_MCP
void Init_DRAM(DRAMC_CTX_T *p, EMI_SETTINGS* emi_set)
#else
void Init_DRAM(DRAMC_CTX_T *p)
#endif
{
#ifdef DRAMC_DEBUG_TIME
	U32 u4time = get_timer(0);
#endif
	U8 ucstatus = 0;
    U32 u4value;
#ifdef MATYPE_ADAPTATION
	U32 u4Backup_Reg_04;
#endif
#if 0
    pmic_config_interface(0x8004, 0x0, 0x1, 7); // 0x8004 bit7 = 1'b0
#ifdef VBIASN_02V
    pmic_config_interface(0x544, 0x1 , 0x1F, 11); //0.2V
#else
    pmic_config_interface(0x544, 0x0, 0x1F, 11); //Set VbiasN to 0V
#endif
    //Set for Vref at 0.6V when power on
    pmic_config_interface(0x8006, 0x007D, 0xFFFF, 0);
    pmic_config_interface(0x8008, 0x007D, 0xFFFF, 0);
    pmic_config_interface(0x800A, 0x007D, 0xFFFF, 0);
    mcDELAY_MS(25);	// According to ACD spec, need to delay 25ms in normal operation (DS1,DS0)=(1,0).
#endif
#ifdef COMBO_MCP
    EMI_Init(p, emi_set);

    p->channel = CHANNEL_A;
    DramcPreInit((DRAMC_CTX_T *) p, emi_set);
    p->channel = CHANNEL_B;
    DramcPreInit((DRAMC_CTX_T *) p, emi_set);

    DramcDiv2PhaseSync((DRAMC_CTX_T *) p);

    p->channel = CHANNEL_A;
    DramcInit((DRAMC_CTX_T *) p, emi_set);
    p->channel = CHANNEL_B;
    DramcInit((DRAMC_CTX_T *) p, emi_set);
#else

    EMI_Init(p);

    p->channel = CHANNEL_A;
    DramcPreInit((DRAMC_CTX_T *) p);

    p->channel = CHANNEL_B;
    DramcPreInit((DRAMC_CTX_T *) p);

    DramcDiv2PhaseSync((DRAMC_CTX_T *) p);

    p->channel = CHANNEL_A;
    DramcInit((DRAMC_CTX_T *) p);
    p->channel = CHANNEL_B;
    DramcInit((DRAMC_CTX_T *) p);
#endif

#ifdef FTTEST_ZQONLY
    while (1)
    {
        p->channel = CHANNEL_A;
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x088), LPDDR3_MODE_REG_10);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1e4), 0x00000001);
        mcDELAY_US(1);		// tZQINIT>=1us
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1e4), 0x00000000);

        p->channel = CHANNEL_B;
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x088), LPDDR3_MODE_REG_10);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1e4), 0x00000001);
        mcDELAY_US(1);		// tZQINIT>=1us
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1e4), 0x00000000);
    }
#endif

#ifdef DRAMC_DEBUG_CALIB_LOG
dramc_calib_step_init(p);
#endif

#ifdef DRAMC_DEBUG_TIME
gs_Dramc_Debug_Time.u4Total_dram_init = get_timer(u4time);
#endif

}

void release_dram(void)
{
#ifdef DDR_RESERVE_MODE
    int counter = TIMEOUT;
    rgu_release_rg_dramc_conf_iso();
    rgu_release_rg_dramc_iso();
    rgu_release_rg_dramc_sref();
    while(counter)
    {
      if(rgu_is_dram_slf() == 0) /* expect to exit dram-self-refresh */
        break;
      counter--;
    }
    if(counter == 0)
    {
      if(g_ddr_reserve_enable==1 && g_ddr_reserve_success==1)
      {
        print("[DDR Reserve] release dram from self-refresh FAIL!\n");
        g_ddr_reserve_success = 0;
      }
    }
#endif
}

void check_ddr_reserve_status(void)
{
#ifdef DDR_RESERVE_MODE
    int counter = TIMEOUT;
    if(rgu_is_reserve_ddr_enabled())
    {
      g_ddr_reserve_enable = 1;
      if(rgu_is_reserve_ddr_mode_success())
      {
        while(counter)
        {
          if(rgu_is_dram_slf())
          {
            g_ddr_reserve_success = 1;
            break;
          }
          counter--;
        }
        if(counter == 0)
        {
          print("[DDR Reserve] ddr reserve mode success but DRAM not in self-refresh!\n");
          g_ddr_reserve_success = 0;
        }
      }
    else
      {
        print("[DDR Reserve] ddr reserve mode FAIL!\n");
        g_ddr_reserve_success = 0;
      }
    }
    else
    {
      print("[DDR Reserve] ddr reserve mode not be enabled yet\n");
      g_ddr_reserve_enable = 0;
    }

    /* release dram, no matter success or failed */
    release_dram();
#endif
}

extern const U32 uiLPDDR_PHY_Mapping_POP_CHA[32];
extern const U32 uiLPDDR_PHY_Mapping_POP_CHB[32];
unsigned int DRAM_MRR(int MRR_num)
{
    unsigned int MRR_value = 0x0;
    unsigned int dram_type, ucstatus, u4value;
    DRAMC_CTX_T *p = psCurrDramCtx;

    if ((p->dram_type == TYPE_LPDDR3) || (p->dram_type == TYPE_LPDDR2))
    {
        // set DQ bit 0, 1, 2, 3, 4, 5, 6, 7 pinmux
        if (p->channel == CHANNEL_A)
        {
            if (p->dram_type == TYPE_LPDDR3)
            {
                // refer to CA training pinmux array
                ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_RRRATE_CTL), &u4value);
                mcSET_FIELD(u4value, uiLPDDR_PHY_Mapping_POP_CHA[0], MASK_RRRATE_CTL_BIT0_SEL, POS_RRRATE_CTL_BIT0_SEL);
                mcSET_FIELD(u4value, uiLPDDR_PHY_Mapping_POP_CHA[1], MASK_RRRATE_CTL_BIT1_SEL, POS_RRRATE_CTL_BIT1_SEL);
                mcSET_FIELD(u4value, uiLPDDR_PHY_Mapping_POP_CHA[2], MASK_RRRATE_CTL_BIT2_SEL, POS_RRRATE_CTL_BIT2_SEL);
                mcSET_FIELD(u4value, uiLPDDR_PHY_Mapping_POP_CHA[3], MASK_RRRATE_CTL_BIT3_SEL, POS_RRRATE_CTL_BIT3_SEL);
                ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_RRRATE_CTL), u4value);

                ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_MRR_CTL), &u4value);
                mcSET_FIELD(u4value, uiLPDDR_PHY_Mapping_POP_CHA[4], MASK_MRR_CTL_BIT4_SEL, POS_MRR_CTL_BIT4_SEL);
                mcSET_FIELD(u4value, uiLPDDR_PHY_Mapping_POP_CHA[5], MASK_MRR_CTL_BIT5_SEL, POS_MRR_CTL_BIT5_SEL);
                mcSET_FIELD(u4value, uiLPDDR_PHY_Mapping_POP_CHA[6], MASK_MRR_CTL_BIT6_SEL, POS_MRR_CTL_BIT6_SEL);
                mcSET_FIELD(u4value, uiLPDDR_PHY_Mapping_POP_CHA[7], MASK_MRR_CTL_BIT7_SEL, POS_MRR_CTL_BIT7_SEL);
                ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_MRR_CTL), u4value);
            }
            else // LPDDR2
            {
                //TBD
            }
        }
        else
        {
            if (p->dram_type == TYPE_LPDDR3)
            {
                // refer to CA training pinmux array
                ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_RRRATE_CTL), &u4value);
                mcSET_FIELD(u4value, uiLPDDR_PHY_Mapping_POP_CHB[0], MASK_RRRATE_CTL_BIT0_SEL, POS_RRRATE_CTL_BIT0_SEL);
                mcSET_FIELD(u4value, uiLPDDR_PHY_Mapping_POP_CHB[1], MASK_RRRATE_CTL_BIT1_SEL, POS_RRRATE_CTL_BIT1_SEL);
                mcSET_FIELD(u4value, uiLPDDR_PHY_Mapping_POP_CHB[2], MASK_RRRATE_CTL_BIT2_SEL, POS_RRRATE_CTL_BIT2_SEL);
                mcSET_FIELD(u4value, uiLPDDR_PHY_Mapping_POP_CHB[3], MASK_RRRATE_CTL_BIT3_SEL, POS_RRRATE_CTL_BIT3_SEL);
                ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_RRRATE_CTL), u4value);

                ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_MRR_CTL), &u4value);
                mcSET_FIELD(u4value, uiLPDDR_PHY_Mapping_POP_CHB[4], MASK_MRR_CTL_BIT4_SEL, POS_MRR_CTL_BIT4_SEL);
                mcSET_FIELD(u4value, uiLPDDR_PHY_Mapping_POP_CHB[5], MASK_MRR_CTL_BIT5_SEL, POS_MRR_CTL_BIT5_SEL);
                mcSET_FIELD(u4value, uiLPDDR_PHY_Mapping_POP_CHB[6], MASK_MRR_CTL_BIT6_SEL, POS_MRR_CTL_BIT6_SEL);
                mcSET_FIELD(u4value, uiLPDDR_PHY_Mapping_POP_CHB[7], MASK_MRR_CTL_BIT7_SEL, POS_MRR_CTL_BIT7_SEL);
                ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_MRR_CTL), u4value);
            }
            else // LPDDR2
            {
                //TBD
            }
        }

        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x088), MRR_num);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1e4), 0x00000002);
        mcDELAY_US(1);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1e4), 0x00000000);
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x03B8), &u4value);
        MRR_value = (u4value >> 20) & 0xFF;
    }

    return MRR_value;
}

#ifdef COMBO_MCP
EMI_SETTINGS emi_setting_default_lpddr3 =
{        //default 2 rank
		0x0,		/* sub_version */
		0x0003,		/* TYPE */
		0,		/* EMMC ID/FW ID checking length */
		0,		/* FW length */
		{0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},		/* NAND_EMMC_ID */
		{0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},		/* FW_ID */
		0x50535057,		/* EMI_CONA_VAL */
		0x00000000,		/* EMI_CONH_VAL */
		0xAAFD478C,		/* DRAMC_ACTIM_VAL */
		0x11000000,		/* DRAMC_GDDR3CTL1_VAL */
		0x00048403,		/* DRAMC_CONF1_VAL */
		0x000063B1,		/* DRAMC_DDR2CTL_VAL */
		0xBFC70401,		/* DRAMC_TEST2_3_VAL */
		0x030000A9,		/* DRAMC_CONF2_VAL */
		0xD1976442,		/* DRAMC_PD_CTRL_VAL */
		0x91001F59,		/* DRAMC_ACTIM1_VAL*/
		0x21000000,		/* DRAMC_MISCTL0_VAL*/
		0x000025E1,		/* DRAMC_ACTIM05T_VAL*/
		0x002156C1,		/* DRAM_CRKCFG_VAL*/
		0x2801110D,		/* DRAMC_TEST2_4_VAL*/
		{0x40000000,0x40000000,0,0},		/* DRAM RANK SIZE */
		{0,0,0,0,0,0,0,0,0,0},		/* reserved 10 */
		0x00830001,		/* LPDDR3_MODE_REG1 */
		0x001C0002,		/* LPDDR3_MODE_REG2 */
		0x00020003,		/* LPDDR3_MODE_REG3 */
		0x00000006,		/* LPDDR3_MODE_REG5 */
		0x00FF000A,		/* LPDDR3_MODE_REG10 */
		0x0000003F,		/* LPDDR3_MODE_REG63 */
};

static int mt_get_dram_type_for_dis(void)
{
    int i;
    int type = 2;
    type = (emi_settings[0].type & 0xF);
    for (i = 0 ; i < num_of_emi_records; i++)
    {
      //print("[EMI][%d] type%d\n",i,type);
      if (type != (emi_settings[i].type & 0xF))
      {
          print("It's not allow to combine two type dram when combo discrete dram enable\n");
          ASSERT(0);
          break;
      }
    }
    return type;
}

static int mt_get_dram_density(void)
{
    int value, density;
    long long size;

    value = DRAM_MRR(8);
    io_width = ((value & 0xC0) >> 6)? 2 : 4; //0:32bit(4byte), 1:16bit(2byte)
    //print("[EMI]DRAM IO width = %d bit\n", io_width*8);

    density = (value & 0x3C) >> 2;
    switch(density)
    {
        case 0x6:
            size = 0x20000000;  //4Gb
            //print("[EMI]DRAM density = 4Gb\n");
            break;
        case 0xE:
            size = 0x30000000;  //6Gb
            //print("[EMI]DRAM density = 6Gb\n");
            break;
        case 0x7:
            size = 0x40000000;  //8Gb
            //print("[EMI]DRAM density = 8Gb\n");
            break;
        case 0xD:
            size = 0x60000000;  //12Gb
            //print("[EMI]DRAM density = 12Gb\n");
            break;
        case 0x8:
            size = 0x80000000;  //16Gb
            //print("[EMI]DRAM density = 16Gb\n");
            break;
        //case 0x9:
            //size = 0x100000000L; //32Gb
            //print("[EMI]DRAM density = 32Gb\n");
            //break;
        default:
            size = 0; //reserved
     }
     if(io_width ==2)
    size = size + size;
    
     return size;
}

#if 0
static char id[22];
static int emmc_nand_id_len=16;
static int fw_id_len;
#endif

U8 R0_gw_coarse_valid[2], R1_gw_coarse_valid[2];
int mdl_number = -1;
U8 MDL_first_search_done = 0;


int mdl_get_rank_number(void)
{
    if((R0_gw_coarse_valid[0] != 0) &&
       (R0_gw_coarse_valid[1] != 0) &&
       (R1_gw_coarse_valid[0] != 0) &&
       (R1_gw_coarse_valid[1] != 0))
    {
        //rank 0 and rank 1 calibration pass
        return 2;
    }
    else if(
       (R0_gw_coarse_valid[0] != 0) &&
       (R0_gw_coarse_valid[1] != 0) &&
       (R1_gw_coarse_valid[0] == 0) &&
       (R1_gw_coarse_valid[1] == 0))
    {
        //rank 0 calibration pass, rank 1 calibration fail
        return 1;
    }
    else
        return -1;  //error
}
extern int num_of_emi_records;

static int mt_get_mdl_number (void)
{
    static int found = 0;
    int i;
    int j;
    int has_emmc_nand = 0;
    int discrete_dram_num = 0;
    int mcp_dram_num = 0;

    unsigned int mode_reg_5, drama_density,dramb_density, dram_channel_nr, dram_rank_nr;
    unsigned int dram_type;

   #ifdef DISCRETE_DDR_MDL_SEARCH
             DRAMC_CTX_T *p;
             EMI_SETTINGS *emi_set;

           if(MDL_first_search_done==0)
           {//first time search
                 dram_type = mt_get_dram_type_for_dis();

            if (TYPE_LPDDR3 == dram_type)
            {
                print("[EMI] LPDDR3 MDL search init\r\n");

            }
            else if (TYPE_PCDDR3 == dram_type)
            {
          print("this chip no support PCDDR3 !!! \n");
          ASSERT(0);
            }

                emi_set = &emi_setting_default_lpddr3;
                p = psCurrDramCtx =&DramCtx_LPDDR3;

                Init_DRAM(psCurrDramCtx, emi_set);

                if(num_of_emi_records<2)
                  *(volatile unsigned *)(EMI_APB_BASE+0x00000000)=emi_settings[0].EMI_CONA_VAL;

                  print("[EMI] MDL search dram init\r\n");

      		  p->channel = CHANNEL_A;
		  DualRankDramcRxdqsGatingCal((DRAMC_CTX_T *) p);
		  drama_density = mt_get_dram_density();
		  print(" CHA dram_die_density:0x%x\n", drama_density);                  
		  p->channel = CHANNEL_B;
                  DualRankDramcRxdqsGatingCal((DRAMC_CTX_T *) p);
                  dramb_density = mt_get_dram_density();
                  print(" CHB dram_die_density:0x%x\n", dramb_density);
                  dram_rank_nr = mdl_get_rank_number();
                  
                  if((R0_gw_coarse_valid[0]!=0)&&(R1_gw_coarse_valid[0]!=0))
                  	drama_density=drama_density+drama_density;
                  	
                  if((R0_gw_coarse_valid[1]!=0)&&(R1_gw_coarse_valid[1]!=0))
                  	dramb_density=dramb_density+dramb_density;

                  print(" CHA dram_density:0x%x\n", drama_density);
                  print(" CHB dram_density:0x%x\n", dramb_density);  

                  printf("CH_A Rank0 value %d, CH_B Rank0 value %d\n", R0_gw_coarse_valid[0] , R0_gw_coarse_valid[1] );
                  printf("CH_A Rank1 value %d, CH_B Rank1 value %d\n", R1_gw_coarse_valid[0] , R1_gw_coarse_valid[1] );
                  // read back value is not "0" means rank is there
                  printf("[EMI] get_rank_number:%d\n",dram_rank_nr);
                  MDL_first_search_done=1;
                  if((drama_density ==0x80000000)&&(dramb_density ==0x80000000)&&(num_of_emi_records>=4)&&( io_width ==2))
                  	mdl_number = 3; //use total 4GB setting  
                  else if((drama_density ==0x80000000)&&(dramb_density ==0x80000000)&&(num_of_emi_records>=3))
                   mdl_number = 2; //use total 4GB setting
                   else if((dram_rank_nr==2)&&(num_of_emi_records>=2))
                   mdl_number = 1;  //use 2 rank mode setting
                   else
                   mdl_number = 0;//use 1 rank mode setting
                    	}
                   else
                   	{
                   	  if(mdl_number==-1)
                   	  	mdl_number=0;
                   	}
                   return mdl_number;

   #else
    if (!(found))
    {
        int result=0;
        //platform_get_mcp_id (id, emmc_nand_id_len,&fw_id_len);
        for (i = 0 ; i < num_of_emi_records; i++)
        {
            if ((emi_settings[i].type & 0x0F00) == 0x0000)
            {
                discrete_dram_num ++;
            }
            else
            {
                mcp_dram_num ++;
            }
        }

        /*If the number >=2  &&
         * one of them is discrete DRAM
         * enable combo discrete dram parse flow
         * */
        if ((discrete_dram_num > 0) && (num_of_emi_records >= 2))
        {
            /* if we enable combo discrete dram
             * check all dram are all same type and not DDR3
             * */
            enable_combo_dis = 1;
            dram_type = emi_settings[0].type & 0x000F;
            for (i = 0 ; i < num_of_emi_records; i++)
            {
                if (dram_type != (emi_settings[i].type & 0x000F))
                {
                    printf("[EMI] Combo discrete dram only support when combo lists are all same dram type.");
                    ASSERT(0);
                }
                if ((emi_settings[i].type & 0x000F) == TYPE_PCDDR3)
                {
                    // has PCDDR3, disable combo discrete drame, no need to check others setting
                    enable_combo_dis = 0;
                    break;
                }
                dram_type = emi_settings[i].type & 0x000F;
            }

        }
        printf("[EMI] mcp_dram_num:%d,discrete_dram_num:%d,enable_combo_dis:%d\r\n",mcp_dram_num,discrete_dram_num,enable_combo_dis);
        /*
         *
         * 0. if there is only one discrete dram, use index=0 emi setting and boot it.
         * */
        if ((0 == mcp_dram_num) && (1 == discrete_dram_num))
        {
            mdl_number = 0;
            found = 1;
            return mdl_number;
        }

#if 0
        /* 1.
         * if there is MCP dram in the list, we try to find emi setting by emmc ID
         * */
        if (mcp_dram_num > 0)
        {
            result = platform_get_mcp_id (id, emmc_nand_id_len,&fw_id_len);

            for (i = 0; i < num_of_emi_records; i++)
            {
                if (emi_settings[i].type != 0)
                {
                    if ((emi_settings[i].type & 0xF00) != 0x000)
                    {
                        if (result == 0)
                        {   /* valid ID */

                            if ((emi_settings[i].type & 0xF00) == 0x100)
                            {
                                /* NAND */
                                if (memcmp(id, emi_settings[i].ID, emi_settings[i].id_length) == 0){
                                    memset(id + emi_settings[i].id_length, 0, sizeof(id) - emi_settings[i].id_length);
                                    mdl_number = i;
                                    found = 1;
                                    break; /* found */
                                }
                            }
                            else
                            {

                                /* eMMC */
                                if (memcmp(id, emi_settings[i].ID, emi_settings[i].id_length) == 0)
                                {
#if 1
                                    printf("fw id len:%d\n",emi_settings[i].fw_id_length);
                                    if (emi_settings[i].fw_id_length > 0)
                                    {
                                        char fw_id[6];
                                        memset(fw_id, 0, sizeof(fw_id));
                                        memcpy(fw_id,id+emmc_nand_id_len,fw_id_len);
                                        for (j = 0; j < fw_id_len;j ++){
                                            printf("0x%x, 0x%x ",fw_id[j],emi_settings[i].fw_id[j]);
                                        }
                                        if(memcmp(fw_id,emi_settings[i].fw_id,fw_id_len) == 0)
                                        {
                                            mdl_number = i;
                                            found = 1;
                                            break; /* found */
                                        }
                                        else
                                        {
                                            printf("[EMI] fw id match failed\n");
                                        }
                                    }
                                    else
                                    {
                                        mdl_number = i;
                                        found = 1;
                                        break; /* found */
                                    }
#else
                                        mdl_number = i;
                                        found = 1;
                                        break; /* found */
#endif
                                }
                                else{
                                      printf("[EMI] index(%d) emmc id match failed\n",i);
                                }

                            }
                        }
                    }
                }
            }
        }
#endif
#if 1
        /* 2. find emi setting by MODE register 5
         * */
        // if we have found the index from by eMMC ID checking, we can boot android by the setting
        // if not, we try by vendor ID
        if ((0 == found) && (1 == enable_combo_dis))
        {
            EMI_SETTINGS *emi_set;
            //print_DBG_info();
            //print("-->%x,%x,%x\n",emi_set->DRAMC_ACTIM_VAL,emi_set->sub_version,emi_set->fw_id_length);
            //print("-->%x,%x,%x\n",emi_setting_default.DRAMC_ACTIM_VAL,emi_setting_default.sub_version,emi_setting_default.fw_id_length);
            dram_type = mt_get_dram_type_for_dis();
            if (TYPE_LPDDR3 == dram_type)
            {
                print("[EMI] LPDDR3 discrete dram init\r\n");
                emi_set = &emi_setting_default_lpddr3;
                psCurrDramCtx = &DramCtx_LPDDR3;
#if defined(FREQ_BY_CHIP) && !defined(DUAL_FREQ_K)
                psCurrDramCtx->frequency = mt_get_dram_freq_setting();
#endif
                Init_DRAM(psCurrDramCtx, emi_set);
            }
            else if (TYPE_PCDDR3 == dram_type)
            {
                //TBD
            }
            //print_DBG_info();
            do_calib(psCurrDramCtx, emi_set, 1); //skip dual frequency calibration

            unsigned int manu_id = DRAM_MRR(0x5);
            print("[EMI]rank0: MR5:%x\n",manu_id);

            //try to find discrete dram by DDR2_MODE_REG5(vendor ID)
            for (i = 0; i < num_of_emi_records; i++)
            {
                if (TYPE_LPDDR3 == dram_type)
                    mode_reg_5 = emi_settings[i].iLPDDR3_MODE_REG_5;
                print("emi_settings[%d].MODE_REG_5:%x,emi_settings[%d].type:%x\n",i,mode_reg_5,i,emi_settings[i].type);
                //only check discrete dram type
                if ((emi_settings[i].type & 0x0F00) == 0x0000)
                {
                    //support for compol discrete dram
                    if ((mode_reg_5 == manu_id) )
                    {
                        dram_density = mt_get_dram_density();
                        dram_channel_nr = ((emi_settings[i].EMI_CONA_VAL & 0x1) == 0x1)? 2 : 1;
                        print("emi_settings[%d].DRAM_RANK_SIZE[0]:0x%x, dram_density:0x%x, dram_channel_nr:%d\n",i,emi_settings[i].DRAM_RANK_SIZE[0], dram_density, dram_channel_nr);
                        if(emi_settings[i].DRAM_RANK_SIZE[0] == dram_density * dram_channel_nr)
                        {
                            dram_rank_nr = mt_get_rank_number();
                            if(dram_rank_nr == -1)
                            {
                                print("[EMI]Get dram rank number fail\n");
                                return mdl_number;
                            }
                            else if(dram_rank_nr == 2)
                            {
                                if((emi_settings[i].DRAM_RANK_SIZE[0] == 0) ||
                                   (emi_settings[i].DRAM_RANK_SIZE[1] == 0))
                                    continue;
                            }
                            else if(dram_rank_nr == 1)
                            {
                                if((emi_settings[i].DRAM_RANK_SIZE[0] != 0) &&
                                   (emi_settings[i].DRAM_RANK_SIZE[1] != 0))
                                    continue;
                            }
                            mdl_number = i;
                            found = 1;
                            break;
                        }
                    }
                }
            }
        }
#endif
        printf("found:%d,i:%d\n",found,i);
    }
    #endif
    return mdl_number;
}

#endif //#if 0

int get_dram_rank_nr (void)
{

    int index;
    int emi_cona;
#ifdef COMBO_MCP
    index = mt_get_mdl_number ();
    if (index < 0 || index >=  num_of_emi_records)
    {
        return -1;
    }

    emi_cona = emi_settings[index].EMI_CONA_VAL;
#else
    emi_cona = EMI_CONA_VAL;
#if CFG_FPGA_PLATFORM
    return 1;
#endif
#endif

    if ((emi_cona & (1 << 17)) != 0 || //for channel 0
        (emi_cona & (1 << 16)) != 0 )  //for channel 1
        return 2;
    else
        return 1;


}

int mt_get_dram_type (void)
{
    int n;
#ifdef COMBO_MCP
   /* if combo discrete is enabled, the dram_type is LPDDR2 or LPDDR4, depend on the emi_setting list*/
    if ( 1 == enable_combo_dis)
    return mt_get_dram_type_for_dis();

    n = mt_get_mdl_number();

    if (n < 0  || n >= num_of_emi_records)
    {
        return 0; /* invalid */
    }

    return (emi_settings[n].type & 0xF);
#else
    //KT: set TYPE_LPDDR3 temporally, should be corrected by enabling combo MCP
    return TYPE_LPDDR3;
#endif

}

#ifdef DUAL_FREQ_K
extern U32 PLL_LowFreq_RegVal[PLLGRPREG_SIZE], CHA_LowFreq_RegVal[FREQREG_SIZE], CHB_LowFreq_RegVal[FREQREG_SIZE];
extern U32 PLL_HighFreq_RegVal[PLLGRPREG_SIZE], CHA_HighFreq_RegVal[FREQREG_SIZE], CHB_HighFreq_RegVal[FREQREG_SIZE];

mt_set_vcore_dvfs_info(vcore_dvfs_info_t* vcore_dvfs_info)
{
    vcore_dvfs_info->pll_setting_num = PLLGRPREG_SIZE;
    vcore_dvfs_info->freq_setting_num = FREQREG_SIZE;
    vcore_dvfs_info->low_freq_pll_setting_addr = PLL_LowFreq_RegVal;
    vcore_dvfs_info->low_freq_cha_setting_addr = CHA_LowFreq_RegVal;
    vcore_dvfs_info->low_freq_chb_setting_addr = CHB_LowFreq_RegVal;
    vcore_dvfs_info->high_freq_pll_setting_addr = PLL_HighFreq_RegVal;
    vcore_dvfs_info->high_freq_cha_setting_addr = CHA_HighFreq_RegVal;
    vcore_dvfs_info->high_freq_chb_setting_addr = CHB_HighFreq_RegVal;

    printf("[vcore dvfs][preloader]low_freq_pll_setting_addr = 0x%x\n", vcore_dvfs_info->low_freq_pll_setting_addr);
    printf("[vcore dvfs][preloader]low_freq_cha_setting_addr = 0x%x\n", vcore_dvfs_info->low_freq_cha_setting_addr);
    printf("[vcore dvfs][preloader]low_freq_chb_setting_addr = 0x%x\n", vcore_dvfs_info->low_freq_chb_setting_addr);
    printf("[vcore dvfs][preloader]high_freq_pll_setting_addr = 0x%x\n", vcore_dvfs_info->high_freq_pll_setting_addr);
    printf("[vcore dvfs][preloader]high_freq_cha_setting_addr = 0x%x\n", vcore_dvfs_info->high_freq_cha_setting_addr);
    printf("[vcore dvfs][preloader]high_freq_chb_setting_addr = 0x%x\n", vcore_dvfs_info->high_freq_chb_setting_addr);
    printf("[vcore dvfs][preloader]pll_setting_num = %d\n", vcore_dvfs_info->pll_setting_num);
    printf("[vcore dvfs][preloader]freq_setting_num = %d\n", vcore_dvfs_info->freq_setting_num);
}
#else
mt_set_vcore_dvfs_info(vcore_dvfs_info_t* vcore_dvfs_info)
{
    vcore_dvfs_info->pll_setting_num = 0;
    vcore_dvfs_info->freq_setting_num = 0;
    vcore_dvfs_info->low_freq_pll_setting_addr = 0;
    vcore_dvfs_info->low_freq_cha_setting_addr = 0;
    vcore_dvfs_info->low_freq_chb_setting_addr = 0;
    vcore_dvfs_info->high_freq_pll_setting_addr = 0;
    vcore_dvfs_info->high_freq_cha_setting_addr = 0;
    vcore_dvfs_info->high_freq_chb_setting_addr = 0;
}
#endif

void get_dram_rank_size (int dram_rank_size[])
{
#ifdef COMBO_MCP
    int index, rank_nr, i;

    index = mt_get_mdl_number();

    if (index < 0 || index >= num_of_emi_records)
    {
        return;
    }

    rank_nr = get_dram_rank_nr();

    for(i = 0; i < rank_nr; i++){
        dram_rank_size[i] = emi_settings[index].DRAM_RANK_SIZE[i];

        printf("%d:dram_rank_size:%x\n",i,dram_rank_size[i]);
    }

    return;
#else

    unsigned col_bit, row_bit, ch0_rank0_size, ch0_rank1_size, ch1_rank0_size, ch1_rank1_size;
    unsigned emi_cona = EMI_CONA_VAL, emi_conh = EMI_CONH_VAL;

    dram_rank_size[0] = 0;
    dram_rank_size[1] = 0;

    ch0_rank0_size = (emi_conh >> 16) & 0xf;
    ch0_rank1_size = (emi_conh >> 20) & 0xf;
    ch1_rank0_size = (emi_conh >> 24) & 0xf;
    ch1_rank1_size = (emi_conh >> 28) & 0xf;

    //Channel 0
    {
        if(ch0_rank0_size == 0)
        {
            //rank 0 setting
            col_bit = ((emi_cona >> 4) & 0x03) + 9;
            row_bit = ((emi_cona >> 12) & 0x03) + 13;
            dram_rank_size[0] = (1 << (row_bit + col_bit)) * 4 * 8; // 4 byte * 8 banks
        }
        else
        {
            dram_rank_size[0] = (ch0_rank0_size * 256 << 20);
        }

        if (0 != (emi_cona &  (1 << 17)))   //rank 1 exist
        {
            if(ch0_rank1_size == 0)
            {
                col_bit = ((emi_cona >> 6) & 0x03) + 9;
                row_bit = ((emi_cona >> 14) & 0x03) + 13;
                dram_rank_size[1] = ((1 << (row_bit + col_bit)) * 4 * 8); // 4 byte * 8 banks
            }
            else
            {
                dram_rank_size[1] = (ch0_rank1_size * 256 << 20);
            }
        }
    }

    if(0 != (emi_cona & 0x01))     //channel 1 exist
    {
        if(ch1_rank0_size == 0)
        {
            //rank0 setting
            col_bit = ((emi_cona >> 20) & 0x03) + 9;
            row_bit = ((emi_cona >> 28) & 0x03) + 13;
            dram_rank_size[0] += ((1 << (row_bit + col_bit)) * 4 * 8); // 4 byte * 8 banks
        }
        else
        {
            dram_rank_size[0] += (ch1_rank0_size * 256 << 20);
        }

        if (0 != (emi_cona &  (1 << 16)))   //rank 1 exist
        {
            if(ch1_rank1_size == 0)
            {
                col_bit = ((emi_cona >> 22) & 0x03) + 9;
                row_bit = ((emi_cona >> 30) & 0x03) + 13;
                dram_rank_size[1] += ((1 << (row_bit + col_bit)) * 4 * 8); // 4 byte * 8 banks
            }
            else
            {
                dram_rank_size[1] += (ch1_rank1_size * 256 << 20);
            }
        }
    }

    printf("DRAM rank0 size:0x%x,\nDRAM rank1 size=0x%x\n", dram_rank_size[0], dram_rank_size[1]);

    return;
#endif
}

CHIP_TYPE mt_get_chip_type_by_efuse()
{
    int value;

    value = seclib_get_devinfo_with_index(24);
    //print("chip info = 0x%x\n",value);

    value &= 0xF;  //only need bit[3:0]
    if(((value >= 0) && (value <= 5)) || ((value >= 11) && (value <= 15)))
    {
        return CHIP_6595M;
    }
    else if((value >= 6) && (value <=10))
    {
        return CHIP_6595;
    }

    return -1;
}
#ifdef DRAMC_DEBUG_TEST_MENU_FREQ
unsigned int _u4ddr_freq = 0x00;
unsigned int _u4ddr_freq_index = 0x00;
unsigned int _ddr_freq_table[] = {400,533,666,800,896,933};
unsigned int tm_ddr_get_freq_n()
{
	unsigned int i = 0x00;
	i = sizeof(_ddr_freq_table) / sizeof(_ddr_freq_table[0]);
	return i;
}

void tm_ddr_get_freq_index(unsigned int t4Freq)
{
	unsigned int i = 0x00;
	for(i = 0; i < tm_ddr_get_freq_n(); i++)
	{
		if(_ddr_freq_table[i] == t4Freq)	break;
	}
	_u4ddr_freq_index = i;
}

void tm_ddr_set_freq_index(unsigned int t4Index)
{
	if((t4Index >= 0x00) && (t4Index < tm_ddr_get_freq_n()))
	{
		_u4ddr_freq = _ddr_freq_table[t4Index];
	}
	mt_get_dram_freq_setting();
}

#endif

extern u32 seclib_get_devinfo_with_index(u32 index);

int mt_get_dram_freq_setting()
{
    unsigned int value, freq;
    CHIP_TYPE chip_type;
    u32 bound_setting;

#if defined(FREQ_BY_CHIP)
    if(mt_get_chip_sw_ver() == CHIP_SW_VER_01)  //MT6595 E1
    {
        freq = 896;
    }

            chip_type = mt_get_chip_type_by_efuse();

            if(chip_type == CHIP_6595)    //MT6595 E2
                freq = 896;
            else if(chip_type == CHIP_6595M)  //MT6595M
                freq = 666;
#else
    #ifdef DDR_667
        freq = 333;
    #elif defined (DDR_800)
        freq = 400;
    #elif defined (DDR_1066)
    	freq = 533;
    #elif defined (DDR_1333)
    	freq = 666;
    #elif defined (DDR_1600)
    	freq = 800;
    #elif defined (DDR_1780)
    	freq = 890;
    #elif defined (DDR_1866)
    	 if(mt_get_chip_sw_ver() == CHIP_SW_VER_01)
        	freq =896;
        else
    	freq = 933;
    #elif defined (DDR_2000)
    	freq = 1000;
    #elif defined (DDR_2133)
    	freq = 1066;
    #elif defined (DDR_1420)
    	freq = 710;
    #elif defined (DDR_2400)
    	freq = 1200;
    #elif defined (DDR_1792)
	     if(mt_get_chip_sw_ver() == CHIP_SW_VER_01)
	    freq = 896;
        else
    	    freq = 933;
    #else
    	freq = 890;
    #endif
#endif

	bound_setting=seclib_get_devinfo_with_index(4);
	if ((bound_setting&0x00003000)==0x00001000)
		freq = 800;

	#ifdef FORCE_1792
	   if(mt_get_chip_sw_ver() == CHIP_SW_VER_01)
	  freq = 896;
     else
    	    freq = 933;
	#endif

  #ifdef FORCE_1600
   freq = 666;
  #endif

#ifdef DRAMC_DEBUG_TEST_MENU_FREQ
	if(_u4ddr_freq)
	{
		freq = _u4ddr_freq;
	}
	else
	{
		_u4ddr_freq = freq;
		return freq;
	}
#endif
    print("mt_get_dram_freq = %dMHz\n", freq);
    return freq;
}

#ifdef DRAMC_DEBUG_CALIB_LOG
#define NUM_ELE(a)      			(sizeof(a) / sizeof(a[0]))
#define HAL_READ32(_reg_)           (*((volatile UINT32*)(_reg_)))
#define HAL_WRITE32(_reg_, _val_)   (*((volatile UINT32*)(_reg_)) = (_val_))

static UINT32 DramCPUTest(UINT32 u4BaseAddr, UINT32 u4Len)
{
	UINT32 u4DramTestPattern[]=
	{
		0x00000000, 0xFFFFFFFF, 0x55555555, 0xAAAAAAAA,
		0x55aa55aa, 0xaa55aa55, 0x55aa55aa, 0xaa55aa55,
		0x5555aaaa, 0xaaaa5555, 0x5555aaaa, 0xaaaa5555,
		0x00000000, 0xFFFFFFFF, 0x55555555, 0xAAAAAAAA
	};
	UINT32 u4Val, i, j, u4Pattern;
	UINT32 m,n, u4Mask;

	u4Val = 0x00;
	for(j = 0; j < 32; j++)
	{
		m =0;
		u4Mask = ((UINT32)0x1) << j;
		for(i = 0; i < NUM_ELE(u4DramTestPattern); i++)
		{
			u4Pattern = u4DramTestPattern[i];
			HAL_WRITE32(u4BaseAddr, u4Pattern);
			if((HAL_READ32(u4BaseAddr)&u4Mask) != (u4Pattern&u4Mask))
			{//fail
				m++;
			}
		}

		if(m)
		{//fail
			u4Val |= u4Mask;
		}
	}

	return u4Val;

}

static UINT32 DramAgentTest(DRAMC_CTX_T *p, UINT32 u4BaseAddr, UINT32 u4Len)
{
	UINT32 u4err_value = 0x00;

	u4err_value = DramcEngine2(p, TE_OP_WRITE_READ_CHECK, 0x55000000, 0xaa000010, 2, 0, 0, 0);
   //u4err_value |= DramcEngine2(p, TE_OP_WRITE_READ_CHECK, p->test2_1, p->test2_2, 2, 0, 0, 0);
   return u4err_value;
}

DRAMC_debug_calib gs_dramc_calib;
const UINT32 Dramc_Calib_FreqAddr[DRAMC_CALIB_FREQ_NUM] =
{
	0x618,		// CHA MEMPLL2 phase
	0x624,		// CHA MEMPLL3 phase.
	0x630,		// CHA MEMPLL4 phase
	0x618,		// CHB MEMPLL2 phase
	0x624,		// CHB MEMPLL3 phase.
	0x630,		// CHB MEMPLL4 phase
	0x668,		// 05PHY MEMPLL2 phase
	0x674,		// 05PHY MEMPLL3 phase
	0x680,		// 05PHY MEMPLL4 phas
	0x61c,		// CHB PLL group delay
	0x628,
	0x634,
	0x61c,		// CHA PLL group delay
	0x628,
	0x634,
	0x66c,		// 05PHY PLL group delay
	0x678,
	0x684,
	0x614,		// CHA MEMPLL2 settings.
	0x620,		// CHA MEMPLL3 settings.
	0x62c,		// CHA MEMPLL4 settings.
	0x664,		// 05PHY MEMPLL2 settings.
	0x670,		// 05PHY MEMPLL3 settings.
	0x67c,		// 05PHY MEMPLL4 settings.
	0x614,		// CHB MEMPLL2 settings.
	0x620,		// CHB MEMPLL3 settings.
	0x62c,		// CHB MEMPLL4 settings.
	0x280,		// MPLL POSTDIV
	0x284,		// MPLL PCM - with update bit.
	0x284,		// MPLL PCM
};

const UINT32 Dramc_Calib_RegAddr[DRAMC_CALIB_REG_NUM] =
{
	0x000,		// AC timing
	0x004,		// AC timing
	0x008,		// AC timing
	0x044,		// AC timing
	0x048,	 	// AC timing
	0x0fc, 		// AC timing
	0x1dc,		// AC timing
	0x1e8,		// AC timing
	0x1f8,		// AC timing
	0x110,		// AC timing

	0x00c,		// CA Training : Clock output delay
	0x1a8,	 	// CA Training : CA0~CA3 output delay
	0x1ac,		// CA Training : CA4~CA7 output delay
	0x1b0,		// CA Training : CA8~CA11 output delay

	0x010,		// WL, TX per bit : DQM output delay
	0x014,		// WL, TX per bit : DQS output delay.
	0x200,		// WL, TX per bit : DQ output delay
	0x204,		// WL, TX per bit : DQ output delay
	0x208,		// WL, TX per bit : DQ output delay
	0x20c,		// WL, TX per bit : DQ output delay

	0x0e0,		// GW : R0 Coarse tune. DQSINCTL
	0x404,		// GW : R0 Coarse tune. TXDLY_DQSGATE and  TXDLY_DQSGATE_P1
	0x410,		// GW : R0 Coarse tune. dly_DQSGATE and  dly_DQSGATE_P1
	0x094,		// GW : R0 Fine tune.
	0x118,		// GW : R1 Coarse tune. R1DQSINCTL
	0x418,		// GW : R1 Coarse tune. TXDLY_R1DQSGATE,  TXDLY_R1DQSGATE_P1, dly_R1DQSGATE and  dly_R1DQSGATE_P1
	0x098,		// GW : R1 Fine tune.

	0x07c,		// DLE, AC timing : [6:4] = DATLAT[2:0]
	0x0e4,		// DLE : [3] = DATLAT[3]
	0x0f0,		// DLE : [25] = DATLAT[4]
	0x080,		// DLE : [7:5] RX pipe, [12:8] DSEL

	0x148,		// RX per bit : CLK duty
	0x018,		// RX per bit : DQS input delay
	0x01c,		// RX per bit : DQS input delay
	0x210,		// RX per bit : DQ0~3 input delay
	0x214,		// RX per bit : DQ4~7 input delay
	0x218,		// RX per bit : DQ8~11 input delay
	0x21c,		// RX per bit : DQ12~15 input delay
	0x220,		// RX per bit : DQ16~19 input delay
	0x224,		// RX per bit : DQ20~23 input delay
	0x228,		// RX per bit : DQ24~27 input delay
	0x22c,		// RX per bit : DQ28~31 input delay

	0x08c,		// CLK1 output delay. TX pipes.

	0x41c,		// Selph for WL settings
	0x420,
	0x424,
	0x428,
	0x42c,

	0x138,		// RANKINCTL_ROOT1
	0x1c4,		// RANKINCTL
};

UINT32 dramc_calib_step_init(DRAMC_CTX_T *p)
{
	UINT32 i;

	if(MDL_first_search_done==0)		return 0xFF;
	if(gs_dramc_calib.u4Header == DRAMC_CALIB_HEADER) return 0xFF;//only need init once

	print("dram calib step init :\n");
	print("dram calib step sizeof = %d:\n", sizeof(DRAMC_debug_calib));
	memset(&gs_dramc_calib, 0x00, sizeof(DRAMC_debug_calib));

	gs_dramc_calib.u4Header = DRAMC_CALIB_HEADER;
	memcpy((UINT8 *)gs_dramc_calib.u4PreloaderVersion,BUILD_TIME,15);
	gs_dramc_calib.u4ChipID = mt_get_chip_sw_ver();
	gs_dramc_calib.u4RankNum = get_dram_rank_nr();
	get_dram_rank_size(gs_dramc_calib.u4DramRankSize);

	print("%x, 0x%x, 0x%x \n", gs_dramc_calib.u4RankNum, gs_dramc_calib.u4DramRankSize[0], gs_dramc_calib.u4DramRankSize[1]);
	//----------------------------------------------------------
	for(i = 0; i < DRAMC_CALIB_FREQ_NUM; i++)
	{
		gs_dramc_calib.sDramCalibReg[i].addr = Dramc_Calib_FreqAddr[i];
	}

	for(i = 0; i < DRAMC_CALIB_REG_NUM; i++)
	{
		gs_dramc_calib.sDramCalibReg[DRAMC_CALIB_FREQ_NUM + i].addr = Dramc_Calib_RegAddr[i];
	}
	gs_dramc_calib.sDramCalibReg[DRAMC_CALIB_FREQ_NUM + DRAMC_CALIB_REG_NUM].addr = 0x32;//MR2
	//----------------------------------------------------------

	p->channel = CHANNEL_A;
	dramc_calib_step_test(p,STEP_DRAM_INIT);
	p->channel = CHANNEL_B;
	dramc_calib_step_test(p,STEP_DRAM_INIT);

	return 0x00;
}

UINT32 dramc_calib_step_test(DRAMC_CTX_T *p, UINT32 index)
{
	UINT32 i;
	UINT32 OldVcore1 = 0;
	UINT32 OldVmem1 = 0;
	UINT32 u4BaseAddr, u4Len;
	UINT32 u4Addr, u4value;
	DRAMC_debug_dramcalib_step *sDramCalibStep;

	if(MDL_first_search_done==0)			return 0xFF;
	if(p->frequency == DUAL_FREQ_LOW)
	{
		if(index > STEP_DRAM_CALIB_SW)		return 0xFF;//only test the High Freq result
	}
	if(RXPERBIT_LOG_PRINT == 0x00)
	{
		if(index == STEP_DRAM_CALIB_RX)		return 0xFF;
	}

	print("dram calib step test :%d\n", index);
	if(p->channel == CHANNEL_A)
	{
		u4BaseAddr = 0x40000000;
		u4Len = 0x100;
		sDramCalibStep = (DRAMC_debug_dramcalib_step *)(&gs_dramc_calib.sDramCalibStepA[index]);
	}
	else if(p->channel == CHANNEL_B)
	{
		u4BaseAddr = 0x40000100;
		u4Len = 0x100;
		sDramCalibStep = (DRAMC_debug_dramcalib_step *)(&gs_dramc_calib.sDramCalibStepB[index]);
	}
	else
	{
		return 0xFF;
	}

	if(p->frequency != DUAL_FREQ_LOW)
	{
		sDramCalibStep->result0 = DramCPUTest(u4BaseAddr, u4Len);
		sDramCalibStep->result1 = DramAgentTest(p, u4BaseAddr, u4Len);
		print("result0 = 0x%x, result1 = 0x%x, \n", sDramCalibStep->result0, sDramCalibStep->result1);
	}

	switch (index) {
		case STEP_DRAM_INIT:
		    break;
		case STEP_DRAM_CALIB_SW:
			if(p->channel == CHANNEL_A)
			{
				ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_DRVCTL1), &u4value);
				gs_dramc_calib.u4DramDrv = u4value & 0xFF00FF00;
			}
		    break;
		case STEP_DRAM_CALIB_CA:
			if(p->channel == CHANNEL_A)
			{
				gs_dramc_calib.u4DramFreq = DramOperateDataRate(p);
				#ifdef PMIC_6397_VCORE_VMEM
				pmic_read_interface(VCORE_CON9,&OldVcore1,0x7F,0);
				pmic_read_interface(VDRM_CON9,&OldVmem1, 0x7F,0);
				gs_dramc_calib.u4Vcore = OldVcore1;
				gs_dramc_calib.u4Vmem = OldVmem1;
				#endif
			}
		    break;
		case STEP_DRAM_CALIB_WL:
			break;
		case STEP_DRAM_CALIB_GW:
		    break;
		case STEP_DRAM_CALIB_DATLAT:
		    break;
		case STEP_DRAM_CALIB_DUTY:
			break;
		case STEP_DRAM_CALIB_RX:
		    break;
		case STEP_DRAM_CALIB_TX:
			break;
		case STEP_DRAM_CALIB_DONE:
			gs_dramc_calib.u4DramVendorID = DRAM_MRR(0x5);
			for(i = 0; i < (DRAMC_CALIB_FREQ_NUM + DRAMC_CALIB_REG_NUM); i++)
			{
				u4Addr = gs_dramc_calib.sDramCalibReg[i].addr & 0xFFFFFFFC;
				ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(u4Addr), &u4value);
				if(p->channel == CHANNEL_A)
				{
					gs_dramc_calib.sDramCalibReg[i].value0 = u4value;
				}
				else if(p->channel == CHANNEL_B)
				{
					gs_dramc_calib.sDramCalibReg[i].value1 = u4value;
				}
			}
			u4value = DramcGetMR2ByFreq(mt_get_dram_freq_setting());
			gs_dramc_calib.sDramCalibReg[DRAMC_CALIB_FREQ_NUM + DRAMC_CALIB_REG_NUM].value0 = u4value;
			gs_dramc_calib.sDramCalibReg[DRAMC_CALIB_FREQ_NUM + DRAMC_CALIB_REG_NUM].value1 = u4value;
			break;
        default :
            break;
    }

	return 0x00;
}

UINT32 dramc_calib_step_save(DRAMC_debug_calib *p, UINT32 index)
{
	#ifdef DRAMC_DEBUG_TIME
	U32 u4time = get_timer(0);
	#endif

	if(index <= DRAMC_CALIB_THIRD_BUFFER)
	{
		p->u4LogVersion = index;			//log version
	}
	else
	{
		p->u4LogVersion = 0xFFFFFFFF;		//invalued
		index = DRAMC_CALIB_THIRD_BUFFER; 	//fail
	}

	//if(part_write_dramc(index*0x1000, 0x1000, p) == 0x00)//sizeof byte
	if(part_write_dramc(index*8, 8, p) == 0x00)//sizeof block 512byte
	{//write to eMMC success
		print("dram calib infor write to EMMC success: 0x%x, 0x%x\n", (index*8) << 9, 0x08 << 9);
	}
	else
	{//dump by itself
		dramc_calib_step_dump(p);
	}

	#ifdef DRAMC_DEBUG_TIME
	gs_Dramc_Debug_Time.u4Total_dram_logsave = get_timer(u4time);
	print("[dramc calib step save ]index = %d : time (%dms)\n", index, gs_Dramc_Debug_Time.u4Total_dram_logsave);
	#endif

	return 0x00;
}

UINT32 dramc_calib_step_dump(DRAMC_debug_calib *p)
{
#ifndef MT8173_BOX_SUPPORT
#ifndef USER_BUILD
//only dump the debug log in eng load
	UINT32 i = 0x00;
	UINT32 j = 0x00;
	UINT32 u4value = 0x00;

	print("==============================================\n");
	print("mt8173 dram calib infor dump: \n");
	print("Header = 0x%x \n", p->u4Header);
	print("Log Version = %d \n", p->u4LogVersion);
	print("Preloader Version = %s \n", p->u4PreloaderVersion);
	print("Chip ID = %d \n", p->u4ChipID);
	print("Board ID = %d \n", p->u4DeviceID[0]);
	print("DRAM Vendor ID = %d \n", p->u4DramVendorID);
	print("Vcore = 0x%x \n", p->u4Vcore);
	print("Vmem = 0x%x \n", p->u4Vmem);
	print("DRAM Freq = %d \n", p->u4DramFreq);
	print("DRAM Drv = 0x%x \n", p->u4DramDrv);
	print("DRAM MR2 = 0x%x \n", p->sDramCalibReg[DRAMC_CALIB_FREQ_NUM + DRAMC_CALIB_REG_NUM].value0);

	if(p->u4RankNum == 2)
	{
		print("2 Channel 2 Rank:\n");
		print("CHA Rank0 = 0x%x, CHB Rank0 = 0x%x, CHA Rank1 = 0x%x, CHB Rank1 = 0x%x\n", (p->u4DramRankSize[0] >> 1), (p->u4DramRankSize[0] >> 1), (p->u4DramRankSize[1] >> 1), (p->u4DramRankSize[1] >> 1));
	}
	else
	{
		print("2 Channel 1 Rank:\n");
		print("CHA Rank0 = 0x%x, CHB Rank0 = 0x%x\n", (p->u4DramRankSize[0] >> 1), (p->u4DramRankSize[0] >> 1));
	}

	if(p->u4FullMemoryTest[0] == 0x01)
	{
		print("Full Memory Test : PASS\n");
	}
	else if(p->u4FullMemoryTest[0] == 0x02)
	{
		print("Full Memory Test : FAIL, error code = 0x%x\n", p->u4FullMemoryTest[1]);
	}
	else
	{
		print("Full Memory Test : NA\n");
	}
	//----------------------------------------------------------
	print("\ndram calib test result CHA:\n");
	print("        CPU Test     AgentTest:\n");
	for(i = 0; i < STEP_MAX; i++)
	{
		print("step%d : 0x%x                 0x%x\n", i, p->sDramCalibStepA[i].result0, p->sDramCalibStepA[i].result1);
	}

	print("\ndram calib test result CHB:\n");
	print("        CPU Test     AgentTest:\n");
	for(i = 0; i < STEP_MAX; i++)
	{
		print("step%d : 0x%x                 0x%x\n", i, p->sDramCalibStepB[i].result0, p->sDramCalibStepB[i].result1);
	}
	//----------------------------------------------------------
	print("\nCA training window CHA:\n");
	for(i = 0; i < 10; i++)
	{
		print("CA%d : Shift %d   CA win = %d, CLK win = %d, center = %d\n", i, p->sDramCalibWinA_CA[i].shift, p->sDramCalibWinA_CA[i].CA_Win, p->sDramCalibWinA_CA[i].CLK_Win, p->sDramCalibWinA_CA[i].center);
	}

	print("\nCA training window CHB:\n");
	for(i = 0; i < 10; i++)
	{
		print("CA%d : Shift %d   CA win = %d, CLK win = %d, center = %d\n", i, p->sDramCalibWinB_CA[i].shift, p->sDramCalibWinB_CA[i].CA_Win, p->sDramCalibWinB_CA[i].CLK_Win, p->sDramCalibWinB_CA[i].center);
	}
	//----------------------------------------------------------
	for(j = 0; j < 4; j++)
	{
		if(p->u4RankNum == 1)
		{
			if((j == 1) || (j == 3)) continue;
		}
		print("\n========================================\n");
		if(j == 0)
		{
			print("CHA Rank0 GW calibartion:\n");
		}
		else if(j == 1)
		{
			print("CHA Rank1 GW calibartion:\n");
		}
		else if(j == 2)
		{
			print("CHB Rank0 GW calibartion:\n");
		}
		else if(j == 3)
		{
			print("CHB Rank1 GW calibartion:\n");
		}

		print("coase tune = %d, fine tune = %d\n", p->sDramCalibGW[j].coase, p->sDramCalibGW[j].fine);
		print("========================================\n");
		for(i = 0; i < DQS_GW_COARSE_MAX; i++)
		{
			print("%d |  0x%x\n", p->sDramCalibGW[j].step[i], p->sDramCalibGW[j].value[i]);
		}
	}

	for(j = 0; j < 4; j++)
	{
		if(p->u4RankNum == 1)
		{
			if((j == 1) || (j == 3)) continue;
		}
		print("\n========================================\n");
		if(j == 0)
		{
			print("CHA Rank0 DATLAT calibartion:\n");
		}
		else if(j == 1)
		{
			print("CHA Rank1 DATLAT calibartion:\n");
		}
		else if(j == 2)
		{
			print("CHB Rank0 DATLAT calibartion:\n");
		}
		else if(j == 3)
		{
			print("CHB Rank1 DATLAT calibartion:\n");
		}

		print("first step = %d, best step = %d, total pass = %d\n", p->sDramCalibDATLAT[j].first, p->sDramCalibDATLAT[j].best, p->sDramCalibDATLAT[j].total);
		print("========================================\n");
		for(i = p->sDramCalibDATLAT[j].start; i < DATLAT_TAP_NUMBER; i++)
		{
			print("Step %d | err = 0x%x\n", p->sDramCalibDATLAT[j].step[i], p->sDramCalibDATLAT[j].value[i]);
		}
	}
	//----------------------------------------------------------
	print("\nRX DQ & DQS window CHA: \n");
	for(i = 0; i < 32; i++)
	{
		print("RX dq%d = %d, dqs = %d, win = %d, offset = %d\n", i, p->sDramCalibWinA_RX[i].setuptime, p->sDramCalibWinA_RX[i].holdtime, p->sDramCalibWinA_RX[i].window, p->sDramCalibWinA_RX[i].offset);
	}
	print("\nRX DQ & DQS window CHB: \n");
	for(i = 0; i < 32; i++)
	{
		print("RX dq%d = %d, dqs = %d, win = %d, offset = %d\n", i, p->sDramCalibWinB_RX[i].setuptime, p->sDramCalibWinB_RX[i].holdtime, p->sDramCalibWinB_RX[i].window, p->sDramCalibWinB_RX[i].offset);
	}

	print("\nTX DQ & DQS window CHA: \n");
	for(i = 0; i < 32; i++)
	{
		print("TX dq%d = %d, dqs = %d, win = %d, offset = %d\n", i, p->sDramCalibWinA_TX[i].setuptime, p->sDramCalibWinA_TX[i].holdtime, p->sDramCalibWinA_TX[i].window, p->sDramCalibWinA_TX[i].offset);
	}
	print("\nTX DQ & DQS window CHB: \n");
	for(i = 0; i < 32; i++)
	{
		print("TX dq%d = %d, dqs = %d, win = %d, offset = %d\n", i, p->sDramCalibWinB_TX[i].setuptime, p->sDramCalibWinB_TX[i].holdtime, p->sDramCalibWinB_TX[i].window, p->sDramCalibWinB_TX[i].offset);
	}

	print("\nDRAM register setting dump: \n");
	for(i = DRAMC_CALIB_FREQ_NUM; i < (DRAMC_CALIB_FREQ_NUM + DRAMC_CALIB_REG_NUM); i++)
	{
		print("reg addr = 0x%x, reg value = 0x%x : 0x%x\n", p->sDramCalibReg[i].addr, p->sDramCalibReg[i].value0, p->sDramCalibReg[i].value1);
	}
	for(i = 0; i < DRAMC_CALIB_FREQ_NUM; i++)
	{
		print("reg addr = 0x%x, reg value = 0x%x : 0x%x\n", p->sDramCalibReg[i].addr, p->sDramCalibReg[i].value0, p->sDramCalibReg[i].value1);
	}

	print("==============================================\n");
#endif
#endif
	return 0x00;
}
#endif

unsigned int dramc_actiming_freq_check()
{
	unsigned int u4ACTiming = 0x00;
	unsigned int u4Freq = mt_get_dram_freq_setting();
	unsigned int u4value = (*(volatile unsigned int *)(CHA_DRAMCAO_BASE + (0x00)));

	u4value = u4value >> 24;
	//print("AC Timing = 0x%x, Freq = %dMHz\n", u4value, u4Freq);
	if(u4value == 0x99)
	{
		u4ACTiming = 800;
	}
	else
	{
		u4ACTiming = 933;
	}

	if(u4Freq > u4ACTiming)
	{
		print("[EMI]Error : DRAM Current AC Timing = %dMHz, Freq = %dMHz, Not Match!!!\n", u4ACTiming, u4Freq);
		return 0x01;
	}

	return 0x00;

}

#ifdef DRAMC_DEBUG_TEST_MENU
#define must_print      print
extern unsigned int g_uart;
#define DEBUG_SERIAL_READ_NODATA   (-1)
#define DEBUG_SERIAL_COM_ERROR     (-2)

#define UART_BASE(uart)             (uart)
#define UART_RBR(uart)              (UART_BASE(uart)+0x0)       /* Read only */
#define UART_THR(uart)              (UART_BASE(uart)+0x0)       /* Write only */
#define UART_LSR(uart)              (UART_BASE(uart)+0x14)
#define UART_MSR(uart)              (UART_BASE(uart)+0x18)

void WriteDebugByte(UINT8 ch)
{
    UINT32 LSR;

    while(1) {
        LSR = INREG32(UART_LSR(g_uart));
        if (LSR & UART_LSR_THRE) {
            OUTREG32(UART_THR(g_uart), (UINT32)ch);
            break;
        }
    }
}

static char  UserCommand[128];
static unsigned char command_index = 0;
int nVoltage = 1;
void CTP_GetUARTByte_nonLoop(UINT8 *buffer, UINT16 *datain)
{
    volatile UINT16  LSR;

    LSR = INREG16(UART_LSR(g_uart));
    if (LSR & UART_LSR_DR) {
        *buffer = (UINT8)INREG16(UART_RBR(g_uart));
        *datain = 1;
    } else {
        *buffer = (UINT8)DEBUG_SERIAL_READ_NODATA;
        *datain = 0;
    }
}

uint32 UART_Get_Command(void)
{
    unsigned char buff;
    unsigned short count, result_count;

    count = 1;

    CTP_GetUARTByte_nonLoop(&buff, &result_count);

    while (result_count) {
        if ((buff == 0x0d) || (buff == 0x0a)) {
            UserCommand[command_index] = '\0';
            command_index = 0;
            buff = '\n';
            //PutUARTByte(buff);
            WriteDebugByte(buff);

            buff = '\r';
            //PutUARTByte(buff);
            WriteDebugByte(buff);
            return 1;
        }

        /* check if the input char is backspace */
        if (buff == '\b') {
            /* check if any data in the command buffer */
            if (command_index) {
                /* put "backspace" */
                //PutUARTByte('\b');
                //PutUARTByte(' ');
                //PutUARTByte('\b');
                /* clear the char in the command buffer */
	      WriteDebugByte('\b');
               WriteDebugByte(' ');
   	      WriteDebugByte('\b');

                //command[--command_index] = 0;
                UserCommand[--command_index] = 0;
            }
        } else {
            if (result_count) {
                //PutUARTByte(buff);
                WriteDebugByte(buff);
            }

            /* store the char in the command buffer */
            //command[command_index++] = buff;
            UserCommand[command_index++] = buff;
        }

        CTP_GetUARTByte_nonLoop(&buff, &result_count);
    }

    return 0;
}

void toUpperString(char *str)
{
    char *temp = str;
    while (*temp) {
        if (*temp>='a' && *temp<='z') {
            *temp -= 'a'-'A';
        }
        temp++;
    }
}

void Pring_Voltage_String(void)
 {
	switch(nVoltage) {
	case 1:  // (Vcore HV, Vmem HV)
		must_print("(Vcore HV, Vmem HV) had been set...\n");
		break;
	case 2:  // (Vcore NV, Vmem NV)
		must_print("(Vcore NV, Vmem NV) had been set...\n");
		break;
	case 3:  //  (Vcore LV, Vmem LV)
		must_print("(Vcore LV, Vmem LV) had been set...\n");
		break;
	case 4:  // (Vcore)
		must_print("(Vcore )  ++ had been set...\n");
		break;
	case 5:  // (Vcore)
		must_print("(Vcore )  -- had been set...\n");
		break;
	case 6:  // (Vmem)
		must_print("(Vmem  )  ++ had been set...\n");
		break;
	case 7:  // (Vmem)
		must_print("(Vmem  )  -- had been set...\n");
		break;
	default:
		break;
	}
}


void DRAM_Test_Menu(void)
{
	int nCount;
	int nTemp, nOffset;
	char cDigit;
		int needDelay=0;
	int p_emphasis = 1;
		int i;

	*(volatile unsigned int *)0x10007000 = 0x22000000;
	must_print("DRAM Test Menu:\n");
	must_print("G : Start the test...\n");
	must_print("P : Print voltage settings\n");
	must_print("V : Voltage adjustment. \n");
	#ifdef DRAMC_DEBUG_TEST_MENU_FREQ
	must_print("[ : Freq- adjustment. \n");
	must_print("] : Freq+ adjustment. \n");
	tm_ddr_get_freq_index(mt_get_dram_freq_setting());//freq and index init
	#endif
	must_print("Please enter selection:");
	while (1) {
		if ( UART_Get_Command() )  {
			toUpperString(UserCommand);
			if ( !(strcmp((const char *)UserCommand, "D")) )
			{

			}
			else if ( !(strcmp((const char *)UserCommand, "P")) )
			{
				pmic_voltage_read(0);
			}
			else if ( !(strcmp((const char *)UserCommand, "G")) )
			{
				must_print("Start the test...\n");
				break;
			}
			else if ( !(strcmp((const char *)UserCommand, "V")) )
			{
				must_print(". \n");

				#if defined(DDR_1792) ||defined(DDR_1866)
				must_print("Vcore(HV, NV, LV)=(1.15, 1.125, 1.025)\n");
				must_print("Vmem (HV, NV, LV)=(1.30, 1.22, 1.14)\n");
				must_print("1 : (Vcore HV:1.150,Vmem HV:1.30) \n");
				must_print("2 : (Vcore NV:1.125,Vmem NV:1.22)\n");
				must_print("3 : (Vcore LV:1.025,Vmem LV:1.14)\n");
				#else
				must_print("Vcore(HV, NV, LV)=(1.15, 1.025, 0.931)\n");
				must_print("Vmem (HV, NV, LV)=(1.30, 1.22, 1.14)\n");
				must_print("1 : (Vcore HV:1.150,Vmem HV:1.30) \n");
				must_print("2 : (Vcore NV:1.025,Vmem NV:1.22)\n");
				must_print("3 : (Vcore LV:0.931,Vmem LV:1.14)\n");
				#endif

				must_print("4 : (Vcore )  ++ ...\n");
				must_print("5 : (Vcore )  -- ...\n");
				must_print("6 : (Vmem  )  ++ ...\n");
				must_print("7 : (Vmem  )  -- ...\n");
				must_print("Please enter pattern selection:(1-7)");
				while (1) {
					if ( UART_Get_Command() )  {
						int  nSel = atoi(UserCommand);
						if ( (nSel>=1) && (nSel<=7)) {
							nVoltage = nSel;
							Pring_Voltage_String();
						}
						switch(nVoltage) {
						case 1:  // (Vcore HV, Vmem HV)
							pmic_Vcore_adjust(1);
							pmic_Vmem_adjust(1);
							pmic_voltage_read(0);
							break;
						case 2:  // (Vcore NV, Vmem NV)
							#if defined(DDR_1792) ||defined(DDR_1866)
							pmic_Vcore_adjust(2);
							#else
							pmic_Vcore_adjust(3);
							#endif
							pmic_Vmem_adjust(2);
							pmic_voltage_read(0);
							break;
						case 3:  //  (Vcore LV, Vmem LV)
							#if defined(DDR_1792) ||defined(DDR_1866)
							pmic_Vcore_adjust(3);
							#else
							pmic_Vcore_adjust(4);
							#endif
							pmic_Vmem_adjust(3);
							pmic_voltage_read(0);
							break;
						case 4:  // (Vcore ++)
							pmic_Vcore_adjust(5);
							pmic_voltage_read(0);
							break;
						case 5:  // (Vcore --)
							pmic_Vcore_adjust(6);
							pmic_voltage_read(0);
							break;
						case 6:  // (Vmem ++)
							pmic_Vmem_adjust(5);
							pmic_voltage_read(0);
							break;
						case 7:  // (Vmem --)
							pmic_Vmem_adjust(6);
							pmic_voltage_read(0);
							break;
						default:
							must_print("Wrong  selection %d.\n", nSel);
							break;
						}
						nVoltage = 0x00;
						break;

					 }
				}
			}
			#ifdef DRAMC_DEBUG_TEST_MENU_FREQ
			else if ( !(strcmp((const char *)UserCommand, "[")))
			{//Freq-
				if(_u4ddr_freq_index > 0x00)
				{
					_u4ddr_freq_index--;
				}
				else
				{
					_u4ddr_freq_index = tm_ddr_get_freq_n() - 1;
				}
				tm_ddr_set_freq_index(_u4ddr_freq_index);
			}
			else if ( !(strcmp((const char *)UserCommand, "]")))
			{//Freq+
				if(_u4ddr_freq_index == (tm_ddr_get_freq_n() - 1))
				{
					_u4ddr_freq_index = 0x00;
				}
				else
				{
					_u4ddr_freq_index++;
				}
				tm_ddr_set_freq_index(_u4ddr_freq_index);
			}
			#endif
			must_print("\n");
			must_print("G : Start the test...\n");
			must_print("P : Print voltage settings\n");
			must_print("V : Voltage adjustment. \n");
			#ifdef DRAMC_DEBUG_TEST_MENU_FREQ
			must_print("[ : Freq- adjustment. \n");
			must_print("] : Freq+ adjustment. \n");
			#endif
			must_print("Please enter selection:");
		}
	}
}
#endif

#ifdef DRAMC_DEBUG_SELFREFRESH
void DramcEnterSelfRefreshTest(DRAMC_CTX_T *p, U8 op)
{
	U8 ucstatus = 0;
	U32 uiTemp;

	if(p->channel == CHANNEL_B)		print("B Channel self refresh : %d\n", op);
	else 							print("A Channel self refresh : %d\n", op);

    if (op == 1) // enter self refresh
    {
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_CONF1), &uiTemp);
        mcSET_BIT(uiTemp, POS_CONF1_SELFREF);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_CONF1), uiTemp);
        mcDELAY_US(2);
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_SPCMDRESP), &uiTemp);
        while ( (mcTEST_BIT(uiTemp, POS_SPCMDRESP_SREF_STATE))==0)
        {
    	    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_SPCMDRESP), &uiTemp);
        }

		print("enter self refresh...\n");
    }
    else // exit self refresh
    {
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_CONF1), &uiTemp);
        mcCLR_BIT(uiTemp, POS_CONF1_SELFREF);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_CONF1), uiTemp);
        mcDELAY_US(2);
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_SPCMDRESP), &uiTemp);
        while ( (mcTEST_BIT(uiTemp, POS_SPCMDRESP_SREF_STATE))!=0)
        {
    	    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_SPCMDRESP), &uiTemp);
        }
		print("leave self refresh...\n");
    }
}

void mt_dram_selfrefresh_test(DRAMC_CTX_T *p)
{//self-refresh test
	unsigned long long i;
	unsigned long long size;
	unsigned int *MEM32_BASE;
	U32 u32State;

	u32State = 0x00;
	*(volatile unsigned int *)0x10007000 = 0x22000000;

	size=0x1000/4;
	MEM32_BASE=0x40000000;
	for (i = 0; i < size; i++)
	{
		MEM32_BASE[i] = 0xFF0FF000 + i;
	}

	size=0x1000/4;
	MEM32_BASE=0x40001000;
	for (i = 0; i < size; i++)
	{
		MEM32_BASE[i] = 0x551aa000 + i;
	}

	size=0x1000/4;
	MEM32_BASE=0x40002000;
	for (i = 0; i < size; i++)
	{
		MEM32_BASE[i] = 0x5a25a000 + i;
	}

	size=0x1000/4;
	MEM32_BASE=0x80000000;
	for (i = 0; i < size; i++)
	{
		MEM32_BASE[i] = 0xFF8FF000 + i;
	}

	size=0x1000/4;
	MEM32_BASE=0x80001000;
	for (i = 0; i < size; i++)
	{
		MEM32_BASE[i] = 0x559aa000 + i;
	}

	size=0x1000/4;
	MEM32_BASE=0x80002000;
	for (i = 0; i < size; i++)
	{
		MEM32_BASE[i] = 0x5aa5a000 + i;
	}

	p->channel = CHANNEL_A;
	DramcEnterSelfRefreshTest(p, 1);
	p->channel = CHANNEL_B;
	DramcEnterSelfRefreshTest(p, 1);

	gpt_busy_wait_us(1000);

	p->channel = CHANNEL_A;
	DramcEnterSelfRefreshTest(p, 0);
	p->channel = CHANNEL_B;
	DramcEnterSelfRefreshTest(p, 0);

	print("test 1 : %d\n", u32State);
	size=0x1000/4;
	MEM32_BASE=0x40000000;
	for (i = 0; i < size; i++)
	{
		if(MEM32_BASE[i] != (0xFF0FF000 + i))
		{
			u32State = 0x01;
			print("fail0: addr=0x%x , data=0x%x ...\n", (U32)&(MEM32_BASE[i]), MEM32_BASE[i]);
		}
	}

	print("test 2 : %d\n", u32State);
	size=0x1000/4;
	MEM32_BASE=0x40001000;
	for (i = 0; i < size; i++)
	{
		if(MEM32_BASE[i] != (0x551aa000 + i))
		{
			u32State = 0x01;
			print("fail1: addr=0x%x , data=0x%x ...\n", (U32)&(MEM32_BASE[i]), MEM32_BASE[i]);
		}
	}

	print("test 3 : %d\n", u32State);
	size=0x1000/4;
	MEM32_BASE=0x40002000;
	for (i = 0; i < size; i++)
	{
		if(MEM32_BASE[i] != (0x5a25a000 + i))
		{
			u32State = 0x01;
			print("fail2: addr=0x%x , data=0x%x ...\n", (U32)&(MEM32_BASE[i]), MEM32_BASE[i]);
		}
	}

	print("test 4 : %d\n", u32State);
	size=0x1000/4;
	MEM32_BASE=0x80000000;
	for (i = 0; i < size; i++)
	{
		if(MEM32_BASE[i] != (0xFF8FF000 + i))
		{
			u32State = 0x01;
			print("fail8: addr=0x%x , data=0x%x ...\n", (U32)&(MEM32_BASE[i]), MEM32_BASE[i]);
		}
	}

	print("test 5 : %d\n", u32State);
	size=0x1000/4;
	MEM32_BASE=0x80001000;
	for (i = 0; i < size; i++)
	{
		if(MEM32_BASE[i] != (0x559aa000 + i))
		{
			u32State = 0x01;
			print("fail9: addr=0x%x , data=0x%x ...\n", (U32)&(MEM32_BASE[i]), MEM32_BASE[i]);
		}
	}

	print("test 6 : %d\n", u32State);
	size=0x1000/4;
	MEM32_BASE=0x80002000;
	for (i = 0; i < size; i++)
	{
		if(MEM32_BASE[i] != (0x5aa5a000 + i))
		{
			u32State = 0x01;
			print("faila: addr=0x%x , data=0x%x ...\n", (U32)&(MEM32_BASE[i]), MEM32_BASE[i]);
		}
	}

	print("test 7 : %d\n", u32State);

	if(!u32State) print("self refresh test pass!\n");
}
#endif

#ifdef SUPPORT_DA
void mt_set_emi(EMI_SETTINGS *emi_set)
#else
void mt_set_emi(void)
#endif
{
#ifdef DRAMC_DEBUG_TIME
	U32 u4time = get_timer(0);
	U32 u4time1 = get_timer(0);
#endif
	U32 ii, u4err_value;
	DRAMC_CTX_T *p;

#ifdef DRAMC_DEBUG_TIME
	memset(&gs_Dramc_Debug_Time, 0, sizeof(Dramc_debug_time_struct));
#endif
//===========================================================
#ifndef DUAL_FREQ_K
	#if VcHV_VmHV
	pmic_Vcore_adjust(1);
	pmic_Vmem_adjust(1);
	print("[EMI] LPDDR3 HV : Vcore = 1.15V Vmem = 1.30V \r\n");
	#elif VcLV_VmLV
	pmic_Vcore_adjust(3);
	pmic_Vmem_adjust(3);
	print("[EMI] LPDDR3 LV : Vcore = 1.025V Vmem = 1.138V \r\n");
	#elif VcLV_VmNV
    pmic_Vcore_adjust(3);
	pmic_Vmem_adjust(2);
	print("[EMI] LPDDR3 LV : Vcore = 1.025V Vmem = 1.225V \r\n");
	#elif VcLLV_VmLV
	pmic_Vcore_adjust(4);
	pmic_Vmem_adjust(3);
	print("[EMI] LPDDR3 LLV : Vcore = 0.931V Vmem = 1.138V \r\n");
	#else
	pmic_Vcore_adjust(2);
	pmic_Vmem_adjust(2);
	print("[EMI] LPDDR3 NV : Vcore = 1.125V Vmem = 1.225V \r\n");
	#endif
#else
	#if 1//defined(DDR_1792) ||defined(DDR_1866)   //force 1.1V for display UHD limitation
	pmic_vcore_init();//pmic_Vcore_adjust(2);//Vcore 1.125V
	pmic_Vmem_adjust(2); //Vmem 1.225V
            print("[EMI] LPDDR3 NV : Vcore = 1.125V Vmem = 1.225V \r\n");
	#else
	pmic_Vcore_adjust(3);//Vcore 1.025V
	pmic_Vmem_adjust(2); //Vmem 1.225V
	print("[EMI] LPDDR3 LV : Vcore = 1.025V Vmem = 1.225V \r\n");
            #endif
#endif

#ifdef DRAMC_DEBUG_TEST_MENU
   DRAM_Test_Menu();
#endif

//===========================================================

#ifdef COMBO_MCP
  #ifndef SUPPORT_DA
    int index = 0;
    EMI_SETTINGS *emi_set;

    index = mt_get_mdl_number ();
    print("[Check]mt_get_mdl_number 0x%x\n",index);
    //print("[EMI] eMMC/NAND ID = %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n", id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7], id[8],id[9],id[10],id[11],id[12],id[13],id[14],id[15]);
    if (index < 0 || index >=  num_of_emi_records)
    {
        print("[EMI] setting failed 0x%x\r\n", index);
        ASSERT(0);
    }

    print("[EMI] MDL number = %d\r\n", index);
    emi_set = &emi_settings[index];

	#ifdef DRAMC_DEBUG_TIME
	gs_Dramc_Debug_Time.u4Total_dram_autodetect = get_timer(u4time1);
	#endif

  #endif //SUPPORT_DA
    //print("[EMI] emi_set eMMC/NAND ID = %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n", emi_set->ID[0], emi_set->ID[1], emi_set->ID[2], emi_set->ID[3], emi_set->ID[4], emi_set->ID[5], emi_set->ID[6], emi_set->ID[7], emi_set->ID[8],emi_set->ID[9],emi_set->ID[10],emi_set->ID[11],emi_set->ID[12],emi_set->ID[13],emi_set->ID[14],emi_set->ID[15]);
    if ((emi_set->type & 0xF) == TYPE_LPDDR3)
    {
        p = psCurrDramCtx = &DramCtx_LPDDR3;
#if defined(FREQ_BY_CHIP) && !defined(DUAL_FREQ_K)
        p->frequency = mt_get_dram_freq_setting();
#endif
        Init_DRAM(p, emi_set);
    }
    else if ((emi_set->type & 0xF) == TYPE_PCDDR3)
    {
        p = psCurrDramCtx = &DramCtx_PCDDR3;
#if defined(FREQ_BY_CHIP) && !defined(DUAL_FREQ_K)
        p->frequency = mt_get_dram_freq_setting();
#endif
        Init_DRAM(p, emi_set);
    }
    else
    {
        print("The DRAM type is not supported");
        ASSERT(0);
    }
  #if 0
     p->channel = CHANNEL_A;
	   print("\n\nChannel A setting before calibration ...\n\n");
     Dump_Registers(p);
	   p->channel = CHANNEL_B;
	   print("\n\nChannel B setting before calibration ...\n\n");
     Dump_Registers(p);
     print("\n\nEMI setting ...\n\n");
     Dump_EMI_Registers();
   #endif

    do_calib(p, emi_set, 0);

#else

#ifdef DDRTYPE_LPDDR3
	p = psCurrDramCtx = &DramCtx_LPDDR3;
#endif

#ifdef DDRTYPE_DDR3
	p = psCurrDramCtx = &DramCtx_PCDDR3;
#endif

#ifdef defined(FREQ_BY_CHIP) && !defined(DUAL_FREQ_K)
        p->frequency = mt_get_dram_freq_setting();
#endif

       	Init_DRAM(p);


   #if 0
  p->channel = CHANNEL_A;
	print("\n\nChannel A setting before calibration ...\n\n");
  Dump_Registers(p);
	p->channel = CHANNEL_B;
	print("\n\nChannel B setting before calibration ...\n\n");
  Dump_Registers(p);
  print("\n\nEMI setting ...\n\n");
  Dump_EMI_Registers();
   #endif

    do_calib(p, 0);

#endif

    {
	    int i;
	    u32 dram_rank_size[4] = {0,0,0,0};
	    u64 total_dram_size = 0;
	    get_dram_rank_size(dram_rank_size);
	    for(i=0; i<4; i++){
	    	total_dram_size += dram_rank_size[i];
	    }
	    if((dram_rank_size[0]==0x80000000ULL)&&(dram_rank_size[1]==0x80000000ULL)) {
	    	print("[Enable 4GB Support]\n");
	    	*(volatile unsigned int *)(0x10003208) |= 1 << 15;
	    	*(volatile unsigned int *)(0x10001f00) |= 1 << 13;//for IOMMU check dram address
	    }
    }

     #if 0
	p->channel = CHANNEL_A;
	print("\n\nChannel A setting after calibration ...\n\n");
    Dump_Registers(p);
	p->channel = CHANNEL_B;
	print("\n\nChannel B setting after calibration ...\n\n");
    Dump_Registers(p);
    print("\n\nEMI setting ...\n\n");
    Dump_EMI_Registers();
    #endif

	#if 1
	// Single rank test.
	for (ii=0; ii<2; ii++) {
		u4err_value = DramcEngine2((DRAMC_CTX_T *) p, TE_OP_WRITE_READ_CHECK, 0x55000000, 0xaa010000, 2, 0, 0, (U8)ii);
		mcSHOW_DBG_MSG(("[A60808_MISC_CMD_TA2_XTALK-%d] err_value=0x%x\n", ii, u4err_value));
	}
	#endif

	print("[EMI]DRAM Vendor_ID : 0x%x\n",DRAM_MRR(0x5));

	#ifdef DRAMC_DEBUG_TIME
	gs_Dramc_Debug_Time.u4Total_set_emi = get_timer(u4time);
	#endif
	//====================================================
	#ifdef DRAMC_DEBUG_TIME
	print("[EMI]EMI Set = %dms : DRAM Init(%dms) + DRAM Calib(%dms) + DRAM Auto(%dms)\n", gs_Dramc_Debug_Time.u4Total_set_emi, gs_Dramc_Debug_Time.u4Total_dram_init, gs_Dramc_Debug_Time.u4Total_dram_calib, gs_Dramc_Debug_Time.u4Total_dram_autodetect);

	#endif

	#ifdef DRAMC_DEBUG_SELFREFRESH
	mt_dram_selfrefresh_test(p);
	#endif
	//====================================================

}
