
#include <target/board.h>
#include <platform/env.h>
#include <platform/mt_gpt.h>

#include <string.h>
#include <stdlib.h>

#include <platform/hdmiddc.h>
#include <platform/hdmiedid.h>
#include <platform/hdmi_drv.h>
#include <platform/hdmi_reg.h>

HDMI_SINK_AV_CAP_T _HdmiSinkAvCap;
static unsigned char _fgHdmiNoEdidCheck = FALSE;
static unsigned int _u4i_3D_VIC;
static unsigned int _ui4First_16_NTSC_VIC;
static unsigned int _ui4First_16_PAL_VIC;
static unsigned int _ui4First_16_VIC[16];
static unsigned char _bEdidData[EDID_SIZE]; /* 4 block 512 Bytes */
static unsigned char aEDIDHeader[EDID_HEADER_LEN] =
{ 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00 };
static unsigned char aEDIDVSDBHeader[EDID_VSDB_LEN] = { 0x03, 0x0c, 0x00 };
static unsigned char aEDIDHFVSDBHeader[EDID_VSDB_LEN] = { 0xd8, 0x5d, 0xc4 };

const unsigned char _cFsStr[][7] = { {"32khz  "},
	{"44khz  "},
	{"48khz  "},
	{"88khz  "},
	{"96khz  "},
	{"176khz "},
	{"192khz "}
};


unsigned char cDstStr[50];
unsigned char edid_parsing_result = FALSE;
unsigned char edid_vsdb_exist = FALSE;
unsigned char edid_ext_block_num = 0;

unsigned char fgIsHdmiNoEDIDCheck(void)
{
	return _fgHdmiNoEdidCheck;
}

void vSetNoEdidChkInfo(void)
{
	unsigned char bInx;

	edid_parsing_result = 1;
	edid_vsdb_exist =1;
	_HdmiSinkAvCap.b_sink_support_hdmi_mode = TRUE;
	_HdmiSinkAvCap.ui4_sink_dtd_ntsc_resolution = 0xffffffff;
	_HdmiSinkAvCap.ui4_sink_dtd_pal_resolution = 0xffffffff;
	_HdmiSinkAvCap.ui4_sink_1st_dtd_ntsc_resolution = 0xffffffff;
	_HdmiSinkAvCap.ui4_sink_1st_dtd_pal_resolution = 0xffffffff;
	_HdmiSinkAvCap.ui2_sink_colorimetry = 0xffff;
	_HdmiSinkAvCap.ui4_sink_cea_ntsc_resolution = 0xffffffff;
	_HdmiSinkAvCap.ui4_sink_cea_pal_resolution = 0xffffffff;
	_HdmiSinkAvCap.ui2_sink_aud_dec = 0xffff;
	_HdmiSinkAvCap.ui1_sink_dsd_ch_num = 5;
	for (bInx = 0; bInx < 7; bInx++) {
		_HdmiSinkAvCap.ui1_sink_pcm_ch_sampling[bInx] = 0xff;
		_HdmiSinkAvCap.ui1_sink_dsd_ch_sampling[bInx] = 0xff;
		_HdmiSinkAvCap.ui1_sink_dst_ch_sampling[bInx] = 0xff;
	}

	for (bInx = 0; bInx < 7; bInx++)
		_HdmiSinkAvCap.ui1_sink_pcm_bit_size[bInx] = 0xff;

	_HdmiSinkAvCap.ui1_sink_spk_allocation = 0xff;


	_HdmiSinkAvCap.e_sink_rgb_color_bit =
	    (HDMI_SINK_DEEP_COLOR_10_BIT | HDMI_SINK_DEEP_COLOR_12_BIT |
	     HDMI_SINK_DEEP_COLOR_16_BIT);
	_HdmiSinkAvCap.e_sink_ycbcr_color_bit =
	    (HDMI_SINK_DEEP_COLOR_10_BIT | HDMI_SINK_DEEP_COLOR_12_BIT |
	     HDMI_SINK_DEEP_COLOR_16_BIT);
	_HdmiSinkAvCap.ui2_edid_chksum_and_audio_sup = 0;
	_HdmiSinkAvCap.ui1_sink_support_ai = 1;

	_HdmiSinkAvCap.b_sink_edid_ready = TRUE;

	_HdmiSinkAvCap.b_sink_3D_present = TRUE;
	_HdmiSinkAvCap.ui4_sink_cea_3D_resolution = 0xFFFFFFFF;
	_HdmiSinkAvCap.ui1_sink_max_tmds_clock = 0xFFFF;

	_HdmiSinkAvCap.ui4_sink_hdmi_4k2kvic = 0x3f;
	_HdmiSinkAvCap.b_sink_SCDC_present = 1;
	_HdmiSinkAvCap.b_sink_LTE_340M_sramble = 1;
}

void vClearEdidInfo(void)
{
	unsigned char bInx;

	edid_parsing_result = TRUE;
	edid_vsdb_exist =TRUE;

	_HdmiSinkAvCap.b_sink_support_hdmi_mode = FALSE;
	_HdmiSinkAvCap.ui4_sink_dtd_ntsc_resolution = 0;
	_HdmiSinkAvCap.ui4_sink_dtd_pal_resolution = 0;
	_HdmiSinkAvCap.ui4_sink_1st_dtd_ntsc_resolution = 0;
	_HdmiSinkAvCap.ui4_sink_1st_dtd_pal_resolution = 0;
	_HdmiSinkAvCap.ui2_sink_colorimetry = 0;
	_HdmiSinkAvCap.ui4_sink_cea_ntsc_resolution = 0;
	_HdmiSinkAvCap.ui4_sink_cea_pal_resolution = 0;
	_HdmiSinkAvCap.ui4_sink_native_ntsc_resolution = 0;
	_HdmiSinkAvCap.ui4_sink_native_pal_resolution = 0;
	_HdmiSinkAvCap.ui2_sink_vcdb_data = 0;
	_HdmiSinkAvCap.ui2_sink_aud_dec = 0;
	_HdmiSinkAvCap.ui1_sink_dsd_ch_num = 0;
	for (bInx = 0; bInx < 7; bInx++) {
		if (bInx == 0)
			_HdmiSinkAvCap.ui1_sink_pcm_ch_sampling[bInx] = 0x07;
		else
			_HdmiSinkAvCap.ui1_sink_pcm_ch_sampling[bInx] = 0;
		_HdmiSinkAvCap.ui1_sink_dsd_ch_sampling[bInx] = 0;
		_HdmiSinkAvCap.ui1_sink_dst_ch_sampling[bInx] = 0;
	}

	for (bInx = 0; bInx < 7; bInx++) {
		if (bInx == 0)
			_HdmiSinkAvCap.ui1_sink_pcm_bit_size[bInx] = 0x07;
		else
			_HdmiSinkAvCap.ui1_sink_pcm_bit_size[bInx] = 0;
	}

	_HdmiSinkAvCap.ui1_sink_spk_allocation = 0;
	_HdmiSinkAvCap.ui1_sink_i_latency_present = 0;
	_HdmiSinkAvCap.ui1_sink_p_latency_present = 0;  /* kenny add 2010/4/25 */
	_HdmiSinkAvCap.ui1_sink_p_audio_latency = 0;
	_HdmiSinkAvCap.ui1_sink_p_video_latency = 0;
	_HdmiSinkAvCap.ui1_sink_i_audio_latency = 0;
	_HdmiSinkAvCap.ui1_sink_i_video_latency = 0;

	_HdmiSinkAvCap.e_sink_rgb_color_bit = HDMI_SINK_NO_DEEP_COLOR;
	_HdmiSinkAvCap.e_sink_ycbcr_color_bit = HDMI_SINK_NO_DEEP_COLOR;
	_HdmiSinkAvCap.ui2_edid_chksum_and_audio_sup =
	    (SINK_BASIC_AUDIO_NO_SUP | SINK_SAD_NO_EXIST | SINK_BASE_BLK_CHKSUM_ERR |
	     SINK_EXT_BLK_CHKSUM_ERR);
	_HdmiSinkAvCap.ui2_sink_cec_address = 0xffff;

	_HdmiSinkAvCap.b_sink_edid_ready = FALSE;
	_HdmiSinkAvCap.ui1_sink_support_ai = 0;
	_HdmiSinkAvCap.ui1_Display_Horizontal_Size = 0;
	_HdmiSinkAvCap.ui1_Display_Vertical_Size = 0;
	_HdmiSinkAvCap.ui4_sink_hdmi_4k2kvic = 0;
	_HdmiSinkAvCap.b_sink_SCDC_present = 0;
	_HdmiSinkAvCap.b_sink_LTE_340M_sramble = 0;

	_HdmiSinkAvCap.ui1_CNC = 0;

	if (fgIsHdmiNoEDIDCheck()) {
		vSetNoEdidChkInfo();
	}

}

void vShowEdidRawData(void)
{
	unsigned short bTemp, i, j, k;

	for (bTemp = 0; bTemp < 2; bTemp++) {
		dprintf(0,"===================================================================\n");
		dprintf(0,"   EDID Block Number=#%d\n", bTemp);
		dprintf(0,"   | 00  01  02  03  04  05  06  07  08  09  0a  0b  0c  0d  0e  0f\n");
		dprintf(0,"===================================================================\n");
		j = bTemp * EDID_BLOCK_LEN;
		for (i = 0; i < 8; i++) {
			if (((i * 16) + j) < 0x10)
				dprintf(0,"0%x:  ", (i * 16) + j);
			else
				dprintf(0,"%x:  ", (i * 16) + j);

			for (k = 0; k < 16; k++) {
				if (k == 15) {
					if ((j + (i * 16 + k)) < EDID_SIZE) {   /* for Buffer overflow error */
						if (_bEdidData[j + (i * 16 + k)] > 0x0f)
							dprintf(0,"%2x\n",
							        _bEdidData[j + (i * 16 + k)]);
						else
							dprintf(0,"0%x\n",
							        _bEdidData[j + (i * 16 + k)]);
					}
				} else {
					if ((j + (i * 16 + k)) < EDID_SIZE) {   /* for Buffer overflow error */
						if (_bEdidData[j + (i * 16 + k)] > 0x0f)
							dprintf(0,"%2x  ",
							        _bEdidData[j + (i * 16 + k)]);
						else
							dprintf(0,"0%x  ",
							        _bEdidData[j + (i * 16 + k)]);
					}
				}

			}

		}

	}
	dprintf(0,"===================================================================\n");

}

unsigned char hdmi_fgreadedid(unsigned char i1noedid)
{
	unsigned char bIdx, bBlockIdx;
	unsigned char bExtBlockNo;
	unsigned int reg;

	_fgHdmiNoEdidCheck = FALSE;

	reg = *(volatile unsigned int*)(0x10005210);
	*(volatile unsigned int*)(0x10005210) = reg & 0xffffffdf;
	reg = *(volatile unsigned int*)(0x10005110);
	*(volatile unsigned int*)(0x10005110) = reg & 0xffffffdf;
	reg = *(volatile unsigned int*)(0x10013054);
	if (!(reg & 0x02000000)) {
		dprintf(0,"no hdmi cable attached\n");
		return (0);
	}
	/* block 0 : standard EDID block, address 0x00 ~ 0x7F w/ device ID=0xA0 */
	bExtBlockNo = 0xff;

	for (bBlockIdx = 0; bBlockIdx <= bExtBlockNo; bBlockIdx++) {
		for (bIdx = 0; bIdx < 5; bIdx++) {
			if ((bBlockIdx * EDID_BLOCK_LEN) < EDID_SIZE) { /* for Buffer overflow error */
				if (fgDDCDataRead
				        (EDID_ID + (bBlockIdx >> 1),
				         0x00 + (bBlockIdx & 0x01) * EDID_BLOCK_LEN, EDID_BLOCK_LEN,
				         &_bEdidData[bBlockIdx * EDID_BLOCK_LEN]) == TRUE) {
					break;
				} else {
					if (bIdx == 4) {
						dprintf(0,"-->>>>>>>>>-hdmi_fgreadedid->>>>>>>>>>1\n");
						return (0);
					}
				}
			}
		}

		bExtBlockNo = _bEdidData[EDID_ADDR_EXT_BLOCK_FLAG];
		edid_ext_block_num = bExtBlockNo;
	}
	return (1);
}

void vAnalyzeDTD(unsigned short ui2Active, unsigned short ui2HBlanking, unsigned char bFormat,
                 unsigned char fgFirstDTD)
{
	unsigned int ui4NTSC = _HdmiSinkAvCap.ui4_sink_dtd_ntsc_resolution;
	unsigned int ui4PAL = _HdmiSinkAvCap.ui4_sink_dtd_pal_resolution;
	unsigned int ui41stNTSC = _HdmiSinkAvCap.ui4_sink_1st_dtd_ntsc_resolution;
	unsigned int ui41stPAL = _HdmiSinkAvCap.ui4_sink_1st_dtd_pal_resolution;

	switch (ui2Active) {
		case 0x5a0:     /* 480i */
			if (ui2HBlanking == 0x114) {    /* NTSC */
				if (bFormat == 0) { /* p-scan */
					ui4NTSC |= SINK_480P_1440;
					if (fgFirstDTD) {
						ui41stNTSC |= SINK_480P_1440;
					}
				} else {
					ui4NTSC |= SINK_480I;
					if (fgFirstDTD) {
						ui41stNTSC |= SINK_480I;
					}
				}
			} else if (ui2HBlanking == 0x120) { /* PAL */
				if (bFormat == 0) { /* p-scan */
					ui4PAL |= SINK_576P_1440;
					if (fgFirstDTD) {
						ui41stPAL |= SINK_576P_1440;
					}
				} else {
					ui4PAL |= SINK_576I;
					if (fgFirstDTD) {
						ui41stPAL |= SINK_576I;
					}
				}
			}
			break;
		case 0x2d0:     /* 480p */
			if ((ui2HBlanking == 0x8a) && (bFormat == 0)) { /* NTSC, p-scan */
				ui4NTSC |= SINK_480P;
				if (fgFirstDTD) {
					ui41stNTSC |= SINK_480P;
				}
			} else if ((ui2HBlanking == 0x90) && (bFormat == 0)) {  /* PAL, p-scan */
				ui4PAL |= SINK_576P;
				if (fgFirstDTD) {
					ui41stPAL |= SINK_576P;
				}
			}
			break;
		case 0x500:     /* 720p */
			if ((ui2HBlanking == 0x172) && (bFormat == 0)) {    /* NTSC, p-scan */
				ui4NTSC |= SINK_720P60;
				if (fgFirstDTD) {
					ui41stNTSC |= SINK_720P60;
				}
			} else if ((ui2HBlanking == 0x2bc) && (bFormat == 0)) { /* PAL, p-scan */
				ui4PAL |= SINK_720P50;
				if (fgFirstDTD) {
					ui41stPAL |= SINK_720P50;
				}
			}
			break;
		case 0x780:     /* 1080i, 1080P */
			if ((ui2HBlanking == 0x118) && (bFormat == 1)) {    /* NTSC, interlace */
				ui4NTSC |= SINK_1080I60;
				if (fgFirstDTD) {
					ui41stNTSC |= SINK_1080I60;
				}
			} else if ((ui2HBlanking == 0x118) && (bFormat == 0)) { /* NTSC, Progressive */
				ui4NTSC |= SINK_1080P60;
				if (fgFirstDTD) {
					ui41stNTSC |= SINK_1080P60;
				}
			} else if ((ui2HBlanking == 0x2d0) && (bFormat == 1)) { /* PAL, interlace */
				ui4PAL |= SINK_1080I50;
				if (fgFirstDTD) {
					ui41stPAL |= SINK_1080I50;
				}
			} else if ((ui2HBlanking == 0x2d0) && (bFormat == 0)) { /* PAL, Progressive */
				ui4PAL |= SINK_1080P50;
				if (fgFirstDTD) {
					ui41stPAL |= SINK_1080P50;
				}
			}
			break;
	}
	_HdmiSinkAvCap.ui4_sink_dtd_ntsc_resolution = ui4NTSC;
	_HdmiSinkAvCap.ui4_sink_dtd_pal_resolution = ui4PAL;
	_HdmiSinkAvCap.ui4_sink_1st_dtd_ntsc_resolution = ui41stNTSC;
	_HdmiSinkAvCap.ui4_sink_1st_dtd_pal_resolution = ui41stPAL;
}

unsigned char fgParserEDID(unsigned char *prbData)
{
	unsigned char bIdx;
	unsigned char bTemp = 0;
	unsigned short ui2HActive, ui2HBlanking;

	_HdmiSinkAvCap.ui1_Edid_Version = *(prbData + EDID_ADDR_VERSION);
	_HdmiSinkAvCap.ui1_Edid_Revision = *(prbData + EDID_ADDR_REVISION);
	_HdmiSinkAvCap.ui1_Display_Horizontal_Size = *(prbData + EDID_IMAGE_HORIZONTAL_SIZE);
	_HdmiSinkAvCap.ui1_Display_Vertical_Size = *(prbData + EDID_IMAGE_VERTICAL_SIZE);

	/* Step 1: check if EDID header pass */
	/* ie. EDID[0] ~ EDID[7] = specify header pattern */
	for (bIdx = EDID_ADDR_HEADER; bIdx < (EDID_ADDR_HEADER + EDID_HEADER_LEN); bIdx++) {
		if (*(prbData + bIdx) != aEDIDHeader[bIdx]) {
			return (FALSE);
		}
	}

	/* Step 2: Check if EDID checksume pass */
	/* ie. value of EDID[0] + ... + [0x7F] = 256*n */
	for (bIdx = 0; bIdx < EDID_BLOCK_LEN; bIdx++) {
		/* add the value into checksum */
		bTemp += *(prbData + bIdx);
	}

	/* check if EDID checksume pass */
	if (bTemp) {
		return (FALSE);
	} else {
		_HdmiSinkAvCap.ui2_edid_chksum_and_audio_sup &= ~SINK_BASE_BLK_CHKSUM_ERR;
	}

	/* [3.3] read-back H active line to define EDID resolution */
	for (bIdx = 0; bIdx < 2; bIdx++) {
		ui2HActive =
		    (unsigned
		     short)(*(prbData + EDID_ADDR_TIMING_DSPR_1 + 18 * bIdx +
		              OFST_H_ACT_BLA_HI) & 0xf0) << 4;
		ui2HActive |= *(prbData + EDID_ADDR_TIMING_DSPR_1 + 18 * bIdx + OFST_H_ACTIVE_LO);
		ui2HBlanking =
		    (unsigned
		     short)(*(prbData + EDID_ADDR_TIMING_DSPR_1 + 18 * bIdx +
		              OFST_H_ACT_BLA_HI) & 0x0f) << 8;
		ui2HBlanking |=
		    *(prbData + EDID_ADDR_TIMING_DSPR_1 + 18 * bIdx + OFST_H_BLANKING_LO);
		bTemp = (*(prbData + EDID_ADDR_TIMING_DSPR_1 + 18 * bIdx + OFST_FLAGS) & 0x80) >> 7;
		if (bIdx == 0) {
			vAnalyzeDTD(ui2HActive, ui2HBlanking, bTemp, TRUE);
		} else {
			vAnalyzeDTD(ui2HActive, ui2HBlanking, bTemp, FALSE);
		}
	}

	/* if go here, ie. parsing EDID data ok !! */
	return (TRUE);
}

void vSetEdidChkError(void)
{
	unsigned char bInx;

	edid_parsing_result = TRUE;
	edid_vsdb_exist =TRUE;

	_HdmiSinkAvCap.b_sink_support_hdmi_mode = TRUE;
	_HdmiSinkAvCap.ui4_sink_dtd_ntsc_resolution = SINK_480P;    /* 0x1fffff; */
	_HdmiSinkAvCap.ui4_sink_dtd_pal_resolution = SINK_576P; /* 0x1fffff; */
	_HdmiSinkAvCap.ui2_sink_colorimetry = 0;
	_HdmiSinkAvCap.ui4_sink_cea_ntsc_resolution = 0;
	_HdmiSinkAvCap.ui4_sink_cea_pal_resolution = 0;
	_HdmiSinkAvCap.ui4_sink_native_ntsc_resolution = 0;
	_HdmiSinkAvCap.ui4_sink_native_pal_resolution = 0;
	_HdmiSinkAvCap.ui2_sink_vcdb_data = 0;
	_HdmiSinkAvCap.ui2_sink_aud_dec = 1;    /* PCM only */
	_HdmiSinkAvCap.ui1_sink_dsd_ch_num = 0;
	for (bInx = 0; bInx < 7; bInx++) {
		if (bInx == 0)
			_HdmiSinkAvCap.ui1_sink_pcm_ch_sampling[bInx] = 0x07;   /* 2ch max 48khz */
		else
			_HdmiSinkAvCap.ui1_sink_pcm_ch_sampling[bInx] = 0x0;
		_HdmiSinkAvCap.ui1_sink_dsd_ch_sampling[bInx] = 0;
		_HdmiSinkAvCap.ui1_sink_dst_ch_sampling[bInx] = 0;
	}

	for (bInx = 0; bInx < 7; bInx++) {
		if (bInx == 0)
			_HdmiSinkAvCap.ui1_sink_pcm_bit_size[bInx] = 0x07;  /* 2ch 24 bits */
		else
			_HdmiSinkAvCap.ui1_sink_pcm_bit_size[bInx] = 0;
	}

	_HdmiSinkAvCap.ui1_sink_spk_allocation = 0;
	_HdmiSinkAvCap.ui1_sink_i_latency_present = 0;
	_HdmiSinkAvCap.ui1_sink_p_latency_present = 0;  /* kenny add 2010/4/25 */
	_HdmiSinkAvCap.ui1_sink_p_audio_latency = 0;
	_HdmiSinkAvCap.ui1_sink_p_video_latency = 0;
	_HdmiSinkAvCap.ui1_sink_i_audio_latency = 0;
	_HdmiSinkAvCap.ui1_sink_i_video_latency = 0;

	_HdmiSinkAvCap.e_sink_rgb_color_bit = HDMI_SINK_NO_DEEP_COLOR;
	_HdmiSinkAvCap.e_sink_ycbcr_color_bit = HDMI_SINK_NO_DEEP_COLOR;
	_HdmiSinkAvCap.ui2_edid_chksum_and_audio_sup =
	    (SINK_BASIC_AUDIO_NO_SUP | SINK_SAD_NO_EXIST | SINK_BASE_BLK_CHKSUM_ERR |
	     SINK_EXT_BLK_CHKSUM_ERR);
	_HdmiSinkAvCap.ui2_sink_cec_address = 0xffff;

	_HdmiSinkAvCap.b_sink_edid_ready = FALSE;
	_HdmiSinkAvCap.ui1_sink_support_ai = 0;

	_HdmiSinkAvCap.ui4_sink_hdmi_4k2kvic = 0;
	_HdmiSinkAvCap.b_sink_SCDC_present = 0;
	_HdmiSinkAvCap.b_sink_LTE_340M_sramble = 0;
}

void vParserCEADataBlock(unsigned char *prData, unsigned char bLen)
{
	unsigned int ui4CEA_NTSC = 0, ui4CEA_PAL = 0, ui4OrgCEA_NTSC = 0, ui4OrgCEA_PAL =
	                               0, ui4NativeCEA_NTSC = 0, ui4NativeCEA_PAL = 0;
	unsigned char bTemp, bIdx;
	unsigned char bLengthSum;
	unsigned char bType, bNo, bAudCode, bPcmChNum;
	unsigned char bTemp13, bTemp8, bLatency_offset = 0;
	unsigned char b3D_Multi_present = 0, b3D_Structure_7_0 = 1, b3D_MASK_15_8 =
	                                      1, b3D_MASK_7_0 = 1, b2D_VIC_order_Index = 0;
	unsigned char i, bTemp14 = 1, bDataTemp = 1;
	unsigned int ui4Temp = 0;
	unsigned int u23D_MASK_ALL;

	while (bLen) {
		if (bLen > 0x80)
			break;
		/* Step 1: get 1st data block type & total number of this data type */
		bTemp = *prData;
		bType = bTemp >> 5; /* bit[7:5] */
		bNo = bTemp & 0x1F; /* bit[4:0] */

		if (bType == 0x02) {    /* Video data block */
			ui4CEA_NTSC = 0;
			ui4CEA_PAL = 0;

			for (bIdx = 0; bIdx < bNo; bIdx++) {
				if (*(prData + 1 + bIdx) & 0x80) {  /* Native bit */
					ui4OrgCEA_NTSC = ui4CEA_NTSC;
					ui4OrgCEA_PAL = ui4CEA_PAL;
				}
				switch (*(prData + 1 + bIdx) & 0x7f) {
					case 6:
						ui4CEA_NTSC |= SINK_480I;
						ui4OrgCEA_NTSC |= SINK_480I;
						if (*(prData + 1 + bIdx) & 0x80)    /* Native bit */
							ui4NativeCEA_NTSC |= SINK_480I;
						break;

					case 7:
						ui4CEA_NTSC |= SINK_480I;
						ui4OrgCEA_NTSC |= SINK_480I;    /* 16:9 */
						if (*(prData + 1 + bIdx) & 0x80)    /* Native bit */
							ui4NativeCEA_NTSC |= SINK_480I;

						break;
					case 2:
						ui4CEA_NTSC |= SINK_480P;
						ui4OrgCEA_NTSC |= SINK_480P;
						if (*(prData + 1 + bIdx) & 0x80)    /* Native bit */
							ui4NativeCEA_NTSC |= SINK_480P;

						break;
					case 3:
						ui4CEA_NTSC |= SINK_480P;
						ui4OrgCEA_NTSC |= SINK_480P;
						if (*(prData + 1 + bIdx) & 0x80)    /* Native bit */
							ui4NativeCEA_NTSC |= SINK_480P;

						break;
					case 14:
					case 15:
						ui4CEA_NTSC |= SINK_480P_1440;
						ui4OrgCEA_NTSC |= SINK_480P_1440;
						if (*(prData + 1 + bIdx) & 0x80)    /* Native bit */
							ui4NativeCEA_NTSC |= SINK_480P_1440;

						break;
					case 4:
						ui4CEA_NTSC |= SINK_720P60;
						ui4OrgCEA_NTSC |= SINK_720P60;
						if (*(prData + 1 + bIdx) & 0x80)    /* Native bit */
							ui4NativeCEA_NTSC |= SINK_720P60;

						break;
					case 5:
						ui4CEA_NTSC |= SINK_1080I60;
						ui4OrgCEA_NTSC |= SINK_1080I60;
						if (*(prData + 1 + bIdx) & 0x80)    /* Native bit */
							ui4NativeCEA_NTSC |= SINK_1080I60;
						break;
					case 21:
						ui4CEA_PAL |= SINK_576I;
						ui4OrgCEA_PAL |= SINK_576I;
						if (*(prData + 1 + bIdx) & 0x80)    /* Native bit */
							ui4NativeCEA_PAL |= SINK_576I;
						break;

					case 22:
						ui4CEA_PAL |= SINK_576I;
						ui4OrgCEA_PAL |= SINK_576I;
						if (*(prData + 1 + bIdx) & 0x80)    /* Native bit */
							ui4NativeCEA_PAL |= SINK_576I;

						break;
					case 16:
						ui4CEA_NTSC |= SINK_1080P60;
						ui4OrgCEA_NTSC |= SINK_1080P60;
						if (*(prData + 1 + bIdx) & 0x80)    /* Native bit */
							ui4NativeCEA_NTSC |= SINK_1080P60;
						break;

					case 17:
						ui4CEA_PAL |= SINK_576P;
						ui4OrgCEA_PAL |= SINK_576P;
						if (*(prData + 1 + bIdx) & 0x80)    /* Native bit */
							ui4NativeCEA_PAL |= SINK_576P;
						break;

					case 18:
						ui4CEA_PAL |= SINK_576P;
						ui4OrgCEA_PAL |= SINK_576P;
						if (*(prData + 1 + bIdx) & 0x80)    /* Native bit */
							ui4NativeCEA_PAL |= SINK_576P;

						break;

					case 29:
					case 30:
						ui4CEA_PAL |= SINK_576P_1440;
						ui4OrgCEA_PAL |= SINK_576P_1440;
						if (*(prData + 1 + bIdx) & 0x80)    /* Native bit */
							ui4NativeCEA_PAL |= SINK_576P_1440;
						break;

					case 19:
						ui4CEA_PAL |= SINK_720P50;
						ui4OrgCEA_PAL |= SINK_720P50;
						if (*(prData + 1 + bIdx) & 0x80)    /* Native bit */
							ui4NativeCEA_PAL |= SINK_720P50;

						break;
					case 20:
						ui4CEA_PAL |= SINK_1080I50;
						ui4OrgCEA_PAL |= SINK_1080I50;
						if (*(prData + 1 + bIdx) & 0x80)    /* Native bit */
							ui4NativeCEA_PAL |= SINK_1080I50;

						break;

					case 31:
						ui4CEA_PAL |= SINK_1080P50;
						ui4OrgCEA_PAL |= SINK_1080P50;
						if (*(prData + 1 + bIdx) & 0x80)    /* Native bit */
							ui4NativeCEA_PAL |= SINK_1080P50;
						break;

					case 32:
						ui4CEA_NTSC |= SINK_1080P24;
						ui4CEA_PAL |= SINK_1080P24;
						ui4CEA_NTSC |= SINK_1080P23976;
						ui4CEA_PAL |= SINK_1080P23976;
						ui4OrgCEA_PAL |= SINK_1080P24;
						ui4OrgCEA_NTSC |= SINK_1080P23976;
						if (*(prData + 1 + bIdx) & 0x80)    /* Native bit */
							ui4NativeCEA_PAL |= SINK_1080P24;

						break;

					case 33:
						/* ui4CEA_NTSC |= SINK_1080P25; */
						ui4CEA_PAL |= SINK_1080P25;
						ui4OrgCEA_PAL |= SINK_1080P25;
						if (*(prData + 1 + bIdx) & 0x80)    /* Native bit */
							ui4NativeCEA_PAL |= SINK_1080P25;

						break;

					case 34:
						ui4CEA_NTSC |= SINK_1080P30;
						ui4CEA_NTSC |= SINK_1080P2997;
						ui4CEA_PAL |= SINK_1080P30;
						ui4CEA_PAL |= SINK_1080P2997;
						ui4OrgCEA_PAL |= SINK_1080P30;
						ui4OrgCEA_NTSC |= SINK_1080P2997;
						if (*(prData + 1 + bIdx) & 0x80)    /* Native bit */
							ui4NativeCEA_PAL |= SINK_1080P30;
						break;

					default:
						break;
				}

				if (bIdx < 0x10) {

					switch (*(prData + 1 + bIdx) & 0x7f) {
						case 6:
						case 7:
							ui4Temp = SINK_480I;
							break;
						case 2:
						case 3:
							ui4Temp = SINK_480P;
							break;
						case 14:
						case 15:
							ui4Temp = SINK_480P_1440;
							break;
						case 4:
							ui4Temp = SINK_720P60;
							break;
						case 5:
							ui4Temp = SINK_1080I60;
							break;
						case 21:
						case 22:
							ui4Temp = SINK_576I;
							break;
						case 16:
							ui4Temp = SINK_1080P60;
							break;

						case 17:
						case 18:
							ui4Temp = SINK_576P;
							break;
						case 29:
						case 30:
							ui4Temp = SINK_576P_1440;
							break;
						case 19:
							ui4Temp = SINK_720P50;
							break;
						case 20:
							ui4Temp = SINK_1080I50;
							break;

						case 31:
							ui4Temp = SINK_1080P50;
							break;

						case 32:
							ui4Temp |= SINK_1080P24;
							ui4Temp |= SINK_1080P23976;
							break;

						case 33:
							/* ui4CEA_NTSC |= SINK_1080P25; */
							ui4Temp = SINK_1080P25;
							break;

						case 34:
							ui4Temp |= SINK_1080P30;
							ui4Temp |= SINK_1080P2997;

							break;

						default:
							break;


					}

					_ui4First_16_NTSC_VIC |= ui4CEA_NTSC;
					_ui4First_16_PAL_VIC |= ui4CEA_PAL;
					_ui4First_16_VIC[bIdx] = ui4Temp;
				}

				if (*(prData + 1 + bIdx) & 0x80) {
					ui4OrgCEA_NTSC = ui4CEA_NTSC & (~ui4OrgCEA_NTSC);
					ui4OrgCEA_PAL = ui4CEA_PAL & (~ui4OrgCEA_PAL);

					if (ui4OrgCEA_NTSC) {
						_HdmiSinkAvCap.ui4_sink_native_ntsc_resolution =
						    ui4OrgCEA_NTSC;
					} else if (ui4OrgCEA_PAL) {
						_HdmiSinkAvCap.ui4_sink_native_pal_resolution =
						    ui4OrgCEA_PAL;
					} else {
						_HdmiSinkAvCap.ui4_sink_native_ntsc_resolution = 0;
						_HdmiSinkAvCap.ui4_sink_native_pal_resolution = 0;
					}
				}
			}   /* for(bIdx = 0; bIdx < bNo; bIdx++) */

			_HdmiSinkAvCap.ui4_sink_cea_ntsc_resolution |= ui4CEA_NTSC;
			_HdmiSinkAvCap.ui4_sink_cea_pal_resolution |= ui4CEA_PAL;
			_HdmiSinkAvCap.ui4_sink_native_ntsc_resolution |= ui4NativeCEA_NTSC;
			_HdmiSinkAvCap.ui4_sink_native_pal_resolution |= ui4NativeCEA_PAL;

		} else if (bType == 0x01) { /* Audio data block */
			_HdmiSinkAvCap.ui2_edid_chksum_and_audio_sup &= ~(SINK_SAD_NO_EXIST);
			for (bIdx = 0; bIdx < (bNo / 3); bIdx++) {
				bLengthSum = bIdx * 3;

				bAudCode = (*(prData + bLengthSum + 1) & 0x78) >> 3;    /* get audio code */

				if ((bAudCode >= AVD_LPCM) && bAudCode <= AVD_WMA) {
					_HdmiSinkAvCap.ui2_sink_aud_dec |= (1 << (bAudCode - 1));   /* PCM:1 HDMI_SINK_AUDIO_DEC_LPCM AC3:2 HDMI_SINK_AUDIO_DEC_AC3 */
				}


				if (bAudCode == AVD_LPCM) { /* LPCM */
					bPcmChNum = (*(prData + bLengthSum + 1) & 0x07) + 1;
					if (bPcmChNum >= 2) {
						_HdmiSinkAvCap.ui1_sink_pcm_ch_sampling[bPcmChNum -
						                                        2] =
						                                                (*(prData + bLengthSum + 2) & 0x7f);
						_HdmiSinkAvCap.ui1_sink_pcm_bit_size[bPcmChNum -
						                                     2] =
						                                             (*(prData + bLengthSum + 3) & 0x07);
					}

				}

				if (bAudCode == AVD_DST) {  /* DST */
					bPcmChNum = (*(prData + bLengthSum + 1) & 0x07) + 1;
					if (bPcmChNum >= 2) {
						_HdmiSinkAvCap.ui1_sink_dst_ch_sampling[bPcmChNum -
						                                        2] =
						                                                (*(prData + bLengthSum + 2) & 0x7f);

					}

				}

				if (bAudCode == AVD_DSD) {  /* DSD */
					bPcmChNum = (*(prData + bLengthSum + 1) & 0x07) + 1;
					if (bPcmChNum >= 2) {
						_HdmiSinkAvCap.ui1_sink_dsd_ch_sampling[bPcmChNum -
						                                        2] =
						                                                (*(prData + bLengthSum + 2) & 0x7f);

					}

				}
			}   /* for(bIdx = 0; bIdx < bNo/3; bIdx++) */
		} else if (bType == 0x04) { /* speaker allocation tag code, 0x04 */
			_HdmiSinkAvCap.ui1_sink_spk_allocation = *(prData + 1) & 0x7f;
		} else if (bType == 0x03) { /* VDSB exit */
			for (bTemp = 0; bTemp < EDID_VSDB_LEN; bTemp++) {
				if (*(prData + bTemp + 1) != aEDIDVSDBHeader[bTemp]) {
					if (*(prData + bTemp + 1) == aEDIDHFVSDBHeader[bTemp]) {
						if (bTemp == EDID_VSDB_LEN) {
							edid_vsdb_exist = TRUE;
							_HdmiSinkAvCap.b_sink_support_hdmi_mode =
							    TRUE;
							bTemp13 = *(prData + 6);
							if (bTemp13 & 0x80) {
								_HdmiSinkAvCap.b_sink_SCDC_present =
								    TRUE;
								if (bTemp13 & 0x08)
									_HdmiSinkAvCap.
									b_sink_LTE_340M_sramble
									    = TRUE;
							}
						}

					}

					break;
				}
			}
			/* for loop to end, is. VSDB header match */
			if (bTemp == EDID_VSDB_LEN) {
				edid_vsdb_exist = TRUE;
				_HdmiSinkAvCap.b_sink_support_hdmi_mode = TRUE;
				/* Read CEC physis address */
				if (bNo >= 5) {
					_HdmiSinkAvCap.ui2_sink_cec_address =
					    (*(prData + 4) << 8) | (*(prData + 5));

				} else {
					_HdmiSinkAvCap.ui2_sink_cec_address = 0xFFFF;
				}

				/* Read Support AI */
				if (bNo >= 6) {
					bTemp = *(prData + 6);
					if (bTemp & 0x80) {
						_HdmiSinkAvCap.ui1_sink_support_ai = 1;

					} else {
						_HdmiSinkAvCap.ui1_sink_support_ai = 0;
					}


					_HdmiSinkAvCap.e_sink_rgb_color_bit = ((bTemp >> 4) & 0x07);

					_HdmiSinkAvCap.u1_sink_max_tmds = *(prData + 7);

					if (bTemp & 0x08) { /* support YCbCr Deep Color */
						_HdmiSinkAvCap.e_sink_ycbcr_color_bit =
						    ((bTemp >> 4) & 0x07);
					}

				} else {
					_HdmiSinkAvCap.ui1_sink_support_ai = 0;
				}

				/* max tmds clock */
				if (bNo >= 7) {
					bTemp = *(prData + 7);
					_HdmiSinkAvCap.ui1_sink_max_tmds_clock =
					    ((unsigned short)bTemp) * 5;
					/* _HdmiSinkAvCap.ui1_sink_max_tmds_clock = 190; */
				} else {
					_HdmiSinkAvCap.ui1_sink_max_tmds_clock = 0;
				}

				/* Read Latency data */
				if (bNo >= 8) {
					bTemp = *(prData + 8);
					if (bTemp & 0x20)
						_HdmiSinkAvCap.b_sink_hdmi_video_present = 1;
					else
						_HdmiSinkAvCap.b_sink_hdmi_video_present = 0;
					_HdmiSinkAvCap.ui1_sink_content_cnc = bTemp & 0x0f;

					if (bTemp & 0x80) { /* Latency Present */
						_HdmiSinkAvCap.ui1_sink_p_latency_present = TRUE;   /* kenny add 2010/4/25 */
						_HdmiSinkAvCap.ui1_sink_p_video_latency =
						    *(prData + 9);
						_HdmiSinkAvCap.ui1_sink_p_audio_latency =
						    *(prData + 10);
						if (bTemp & 0x40) { /* Interlace Latency present */
							_HdmiSinkAvCap.ui1_sink_i_latency_present =
							    TRUE;
							_HdmiSinkAvCap.ui1_sink_i_video_latency =
							    *(prData + 11);
							_HdmiSinkAvCap.ui1_sink_i_audio_latency =
							    *(prData + 12);
						}

					}

					_HdmiSinkAvCap.ui1_CNC = bTemp & 0x0F;

				}



				if (bNo >= 8) {
					bTemp = *(prData + 8);

					if (!(bTemp & 0x80)) {  /* Latency Present */
						bLatency_offset = bLatency_offset + 2;
					}
					if (!(bTemp & 0x40)) {  /* Interlace Latency present */
						bLatency_offset = bLatency_offset + 2;
					}

				}
				if (bNo >= 13) {    /* kenny add */
					bTemp = *(prData + 13);
					if (bTemp & 0x80)
						_HdmiSinkAvCap.b_sink_3D_present = 1;
					else
						_HdmiSinkAvCap.b_sink_3D_present = 0;

				}
				if (bNo >= 8) {
					bTemp8 = *(prData + 8);

					if (bTemp8 & 0x20) {
					}
				}

				if (bNo >= (13 - bLatency_offset)) {
					bTemp13 = *(prData + 13 - bLatency_offset);

					if (bTemp13 & 0x80) {
						_u4i_3D_VIC |= SINK_720P50;
						_u4i_3D_VIC |= SINK_720P60;
						_u4i_3D_VIC |= SINK_1080P23976;
						_u4i_3D_VIC |= SINK_1080P24;
						_HdmiSinkAvCap.b_sink_3D_present = TRUE;
					} else
						_HdmiSinkAvCap.b_sink_3D_present = FALSE;
				} else
					_HdmiSinkAvCap.b_sink_3D_present = FALSE;

				if (bNo >= (13 - bLatency_offset)) {
					bTemp13 = *(prData + 13 - bLatency_offset);

					if ((bTemp13 & 0x60) == 0x20) {
						b3D_Multi_present = 0x20;
					} else if ((bTemp13 & 0x60) == 0x40) {
						b3D_Multi_present = 0x40;
					} else {
						b3D_Multi_present = 0x00;
					}
				}

				if (bNo >= (14 - bLatency_offset)) {
					bTemp14 = *(prData + 14 - bLatency_offset);

				}

				if (_HdmiSinkAvCap.b_sink_hdmi_video_present == TRUE) {
					/* hdmi_vic */
					if ((bNo > (14 - bLatency_offset))
					        && (((bTemp14 & 0xE0) >> 5) > 0)) {
						for (bIdx = 0; bIdx < ((bTemp14 & 0xE0) >> 5);
						        bIdx++) {
							if ((*
							        (prData + 15 - bLatency_offset +
							         bIdx)) == 0x01)
								_HdmiSinkAvCap.ui4_sink_hdmi_4k2kvic
								|=
								    SINK_2160P_29_97HZ +
								    SINK_2160P_30HZ;
							if ((*
							        (prData + 15 - bLatency_offset +
							         bIdx)) == 0x02)
								_HdmiSinkAvCap.ui4_sink_hdmi_4k2kvic
								|= SINK_2160P_25HZ;
							if ((*
							        (prData + 15 - bLatency_offset +
							         bIdx)) == 0x03)
								_HdmiSinkAvCap.ui4_sink_hdmi_4k2kvic
								|=
								    SINK_2160P_23_976HZ +
								    SINK_2160P_24HZ;
							if ((*
							        (prData + 15 - bLatency_offset +
							         bIdx)) == 0x04)
								_HdmiSinkAvCap.ui4_sink_hdmi_4k2kvic
								|= SINK_2161P_24HZ;
						}
					}
				}

				if (bNo > (14 - bLatency_offset + ((bTemp14 & 0xE0) >> 5))) {
					if (b3D_Multi_present == 0x20) {
						if (((15 - bLatency_offset +
						        ((bTemp14 & 0xE0) >> 5)) +
						        (bTemp14 & 0x1F)) >=
						        (15 - bLatency_offset +
						         ((bTemp14 & 0xE0) >> 5) + 2)) {
							/* b3D_Structure_15_8=*(prData+15+((bTemp&0xE0)>>5)); */
							b3D_Structure_7_0 =
							    *(prData + 15 - bLatency_offset +
							      ((bTemp14 & 0xE0) >> 5) + 1);

						}
						/* support frame packet */
						if ((b3D_Structure_7_0 & 0x01) == 0x01) {
							for (i = 0; i < 0x10; i++) {
								_u4i_3D_VIC |= _ui4First_16_VIC[i];
							}
						}

						while (((15 - bLatency_offset +
						         ((bTemp14 & 0xE0) >> 5)) +
						        (bTemp14 & 0x1F)) >
						        ((15 - bLatency_offset +
						          ((bTemp14 & 0xE0) >> 5)) + 2 +
						         b2D_VIC_order_Index)) {
							/* 2 is 3D_structure */
							bDataTemp =
							    *(prData + 15 - bLatency_offset +
							      ((bTemp14 & 0xE0) >> 5) + 2 +
							      b2D_VIC_order_Index);
							if ((bDataTemp & 0x0F) < 0x08) {
								b2D_VIC_order_Index =
								    b2D_VIC_order_Index + 1;
								/* 3D_Structure=0,      support frame packet */
								if ((bDataTemp & 0x0F) == 0x00) {
									_u4i_3D_VIC |=
									    _ui4First_16_VIC[((bDataTemp & 0xF0) >> 4)];
								}

							} else {
								b2D_VIC_order_Index =
								    b2D_VIC_order_Index + 2;
							}
						}
					} else if (b3D_Multi_present == 0x40) {
						if (((15 - bLatency_offset +
						        ((bTemp14 & 0xE0) >> 5)) +
						        (bTemp14 & 0x1F)) >=
						        ((15 - bLatency_offset +
						          ((bTemp14 & 0xE0) >> 5)) + 4)) {
							/* 4 is 3D_structure+3D_MASK */
							/* b3D_Structure_15_8=*(prData+15+((bTemp&0xE0)>>5)); */
							b3D_Structure_7_0 =
							    *(prData + 15 - bLatency_offset +
							      ((bTemp14 & 0xE0) >> 5) + 1);
							b3D_MASK_15_8 =
							    *(prData + 15 - bLatency_offset +
							      ((bTemp14 & 0xE0) >> 5) + 2);
							b3D_MASK_7_0 =
							    *(prData + 15 - bLatency_offset +
							      ((bTemp14 & 0xE0) >> 5) + 3);
							/* support frame packet */
							if ((b3D_Structure_7_0 & 0x01) == 0x01) {
								u23D_MASK_ALL =
								    (((unsigned short)(b3D_MASK_15_8)) << 8)
								    |
								    ((unsigned
								      short)(b3D_MASK_7_0));
								for (i = 0; i < 0x10; i++) {
									if (u23D_MASK_ALL & 0x0001) {
										_u4i_3D_VIC |=
										    _ui4First_16_VIC
										    [i];
									}
									u23D_MASK_ALL =
									    u23D_MASK_ALL >> 1;
								}
							}

						}
						while (((15 - bLatency_offset +
						         ((bTemp14 & 0xE0) >> 5)) +
						        (bTemp14 & 0x1F)) >
						        (15 - bLatency_offset +
						         ((bTemp14 & 0xE0) >> 5) + 4 +
						         b2D_VIC_order_Index)) {
							bDataTemp =
							    *(prData + 15 - bLatency_offset +
							      ((bTemp14 & 0xE0) >> 5) + 4 +
							      b2D_VIC_order_Index);
							if ((bDataTemp & 0x0F) < 0x08) {
								b2D_VIC_order_Index =
								    b2D_VIC_order_Index + 1;
								/* 3D_Structure=0 */
								if ((bDataTemp & 0x0F) == 0x00) {
									_u4i_3D_VIC |=
									    _ui4First_16_VIC[((bDataTemp & 0xF0) >> 4)];
								}

							} else {
								b2D_VIC_order_Index =
								    b2D_VIC_order_Index + 2;
							}
						}

					} else {
						b3D_Structure_7_0 = 0;

						while (((15 - bLatency_offset +
						         ((bTemp14 & 0xE0) >> 5)) +
						        (bTemp14 & 0x1F)) >
						        ((15 - bLatency_offset +
						          ((bTemp14 & 0xE0) >> 5)) +
						         b2D_VIC_order_Index)) {
							bDataTemp =
							    *(prData + 15 - bLatency_offset +
							      ((bTemp14 & 0xE0) >> 5) +
							      b2D_VIC_order_Index);
							if ((bDataTemp & 0x0F) < 0x08) {
								b2D_VIC_order_Index =
								    b2D_VIC_order_Index + 1;
								/* 3D_Structure=0 */
								if ((bDataTemp & 0x0F) == 0x00) {
									_u4i_3D_VIC |=
									    _ui4First_16_VIC[((bDataTemp & 0xF0) >> 4)];
								}

							} else {
								b2D_VIC_order_Index =
								    b2D_VIC_order_Index + 2;
							}
						}
					}

				}
				_HdmiSinkAvCap.ui4_sink_cea_3D_resolution = _u4i_3D_VIC;


			} /* if(bTemp==EDID_VSDB_LEN) */
			else {
				/* vSetSharedInfo(SI_EDID_VSDB_EXIST, FALSE); */
				/* _HdmiSinkAvCap.b_sink_support_hdmi_mode = FALSE; */

			}

		} /* if(bType == 0x03) // VDSB exit */
		else if (bType == 0x07) {   /* Use Extended Tag */
			if (*(prData + 1) == 0x05) {    /* Extend Tag code ==0x05 */
				if (*(prData + 2) & 0x1) {
					/* Suppot xvYcc601 */
					_HdmiSinkAvCap.ui2_sink_colorimetry |= SINK_XV_YCC601;
				}

				if (*(prData + 2) & 0x2) {
					/* Suppot xvYcc709 */
					_HdmiSinkAvCap.ui2_sink_colorimetry |= SINK_XV_YCC709;
				}

				if (*(prData + 3) & 0x1) {
					/* support Gamut data P0 */
					_HdmiSinkAvCap.ui2_sink_colorimetry |= SINK_METADATA0;
				}

				if (*(prData + 3) & 0x2) {
					/* support Gamut data P1 */
					_HdmiSinkAvCap.ui2_sink_colorimetry |= SINK_METADATA1;
				}

				if (*(prData + 3) & 0x4) {
					/* support Gamut data P1 */
					_HdmiSinkAvCap.ui2_sink_colorimetry |= SINK_METADATA2;
				}
			} else if (*(prData + 1) == 0x0) {  /* Extend Tag code ==0x0 */
				if (*(prData + 2) & 0x40) {
					/* support selectable, QS=1 */
					_HdmiSinkAvCap.ui2_sink_vcdb_data |= SINK_RGB_SELECTABLE;
				}
			}
		}
		/* re-assign the next data block address */
		prData += (bNo + 1);    /* '1' means the tag byte */

		bLen -= (bNo + 1);

	}           /* while(bLen) */


	_HdmiSinkAvCap.ui1_sink_pcm_ch_sampling[5] |= _HdmiSinkAvCap.ui1_sink_pcm_ch_sampling[6];
	_HdmiSinkAvCap.ui1_sink_pcm_ch_sampling[4] |= _HdmiSinkAvCap.ui1_sink_pcm_ch_sampling[5];
	_HdmiSinkAvCap.ui1_sink_pcm_ch_sampling[3] |= _HdmiSinkAvCap.ui1_sink_pcm_ch_sampling[4];
	_HdmiSinkAvCap.ui1_sink_pcm_ch_sampling[2] |= _HdmiSinkAvCap.ui1_sink_pcm_ch_sampling[3];
	_HdmiSinkAvCap.ui1_sink_pcm_ch_sampling[1] |= _HdmiSinkAvCap.ui1_sink_pcm_ch_sampling[2];
	_HdmiSinkAvCap.ui1_sink_pcm_ch_sampling[0] |= _HdmiSinkAvCap.ui1_sink_pcm_ch_sampling[1];

	_HdmiSinkAvCap.ui1_sink_dsd_ch_sampling[5] |= _HdmiSinkAvCap.ui1_sink_dsd_ch_sampling[6];
	_HdmiSinkAvCap.ui1_sink_dsd_ch_sampling[4] |= _HdmiSinkAvCap.ui1_sink_dsd_ch_sampling[5];
	_HdmiSinkAvCap.ui1_sink_dsd_ch_sampling[3] |= _HdmiSinkAvCap.ui1_sink_dsd_ch_sampling[4];
	_HdmiSinkAvCap.ui1_sink_dsd_ch_sampling[2] |= _HdmiSinkAvCap.ui1_sink_dsd_ch_sampling[3];
	_HdmiSinkAvCap.ui1_sink_dsd_ch_sampling[1] |= _HdmiSinkAvCap.ui1_sink_dsd_ch_sampling[2];
	_HdmiSinkAvCap.ui1_sink_dsd_ch_sampling[0] |= _HdmiSinkAvCap.ui1_sink_dsd_ch_sampling[1];

	/* 2007/2/12 for av output chksum error */
	if (_HdmiSinkAvCap.ui2_edid_chksum_and_audio_sup & SINK_EXT_BLK_CHKSUM_ERR) {
		edid_vsdb_exist = FALSE;
		_HdmiSinkAvCap.b_sink_support_hdmi_mode = FALSE;
	}

}

unsigned char fgParserExtEDID(unsigned char *prData)
{
	unsigned char bIdx;
	unsigned char bTemp = 0;
	unsigned short ui2HActive, ui2HBlanking, ui2VBlanking;
	unsigned char bOfst, *prCEAaddr;

	_HdmiSinkAvCap.ui1_ExtEdid_Revision = *(prData + EXTEDID_ADDR_REVISION);

	for (bIdx = 0; bIdx < EDID_BLOCK_LEN; bIdx++) {
		/* add the value into checksum */
		bTemp += *(prData + bIdx);  /* i4SharedInfo(wPos+bIdx); */
	}


	bTemp = 0;
	/* check if EDID checksume pass */
	if (bTemp) {
		return (FALSE);
	} else {
		_HdmiSinkAvCap.ui2_edid_chksum_and_audio_sup &= ~SINK_EXT_BLK_CHKSUM_ERR;
	}

	/* Step 1: get the offset value of 1st detail timing description within extension block */
	bOfst = *(prData + EXTEDID_ADDR_OFST_TIME_DSPR);

	if (*(prData + EDID_ADDR_EXTEND_BYTE3) & 0x40)  /* Support basic audio */
		_HdmiSinkAvCap.ui2_edid_chksum_and_audio_sup &= ~SINK_BASIC_AUDIO_NO_SUP;

	/* Max'0528'04, move to here, after read 0x80 ~ 0xFF because it is 0x83... */
	if (*(prData + EDID_ADDR_EXTEND_BYTE3) & 0x20) {    /* receiver support YCbCr 4:4:4 */
		_HdmiSinkAvCap.ui2_sink_colorimetry |= SINK_YCBCR_444;
	}

	if (*(prData + EDID_ADDR_EXTEND_BYTE3) & 0x10)  /* receiver support YCbCr 4:2:2 */
		_HdmiSinkAvCap.ui2_sink_colorimetry |= SINK_YCBCR_422;

	_HdmiSinkAvCap.ui2_sink_colorimetry |= SINK_RGB;
	/* Step 3: read-back the pixel clock of each timing descriptor */

	/* Step 4: read-back V active line to define EDID resolution */
	for (bIdx = 0; bIdx < 6; bIdx++) {
		if (((bOfst + 18 * bIdx) > 109) || (*(prData + bOfst + 18 * bIdx) == 0)) {
			break;
		}
		ui2HActive =
		    (unsigned short)(*(prData + bOfst + 18 * bIdx + OFST_H_ACT_BLA_HI) & 0xf0) << 4;
		ui2HActive |= *(prData + bOfst + 18 * bIdx + OFST_H_ACTIVE_LO);
		ui2HBlanking =
		    (unsigned short)(*(prData + bOfst + 18 * bIdx + OFST_H_ACT_BLA_HI) & 0x0f) << 8;
		ui2HBlanking |= *(prData + bOfst + 18 * bIdx + OFST_H_BLANKING_LO);
		ui2VBlanking =
		    (unsigned short)(*(prData + bOfst + 18 * bIdx + OFST_V_ACTIVE_HI) & 0x0f) << 8;
		ui2VBlanking |= *(prData + bOfst + 18 * bIdx + OFST_V_BLANKING_LO);
		bTemp = (*(prData + bOfst + 18 * bIdx + OFST_FLAGS) & 0x80) >> 7;
		vAnalyzeDTD(ui2HActive, ui2HBlanking, bTemp, FALSE);
	}

	if (*(prData + EXTEDID_ADDR_REVISION) >= 0x03) {    /* for simplay #7-37, #7-36 */

		prCEAaddr = prData + 4;
		vParserCEADataBlock(prCEAaddr, bOfst - 4);
	}
	/* if go here, ie. parsing EDID data ok !! */
	return (TRUE);
}

void vParserExtEDIDState(unsigned char *prEdid)
{
	unsigned char bTemp;
	unsigned char *prData;

	if (edid_parsing_result == TRUE) {
		/* parsing EDID extension block if it exist */
		for (bTemp = 0; bTemp < edid_ext_block_num; bTemp++) {
			if ((EDID_BLOCK_LEN + bTemp * EDID_BLOCK_LEN) < EDID_SIZE) {    /* for Buffer Overflow error */
				if (*(prEdid + EDID_BLOCK_LEN + bTemp * EDID_BLOCK_LEN) == 0x02) {
					prData = (prEdid + EDID_BLOCK_LEN + bTemp * EDID_BLOCK_LEN);
					fgParserExtEDID(prData);
				} else if (*(prEdid + EDID_BLOCK_LEN + bTemp * EDID_BLOCK_LEN) ==
				           0xF0) {

				}
			} else {

			}
		}
	}
}

void hdmi_checkedid(unsigned char i1noedid)
{
	unsigned char bTemp;
	unsigned char bRetryCount = 2;
	unsigned char i;

	dprintf(0,"-->>>>>>>>>-hdmi_checkedid->>>>>>>>>>\n");

	vClearEdidInfo();

	for (i = 0; i < 0x10; i++)
		_ui4First_16_VIC[i] = 0;

	_ui4First_16_NTSC_VIC = 0;
	_ui4First_16_PAL_VIC = 0;
	_u4i_3D_VIC = 0;
	_HdmiSinkAvCap.b_sink_hdmi_video_present = FALSE;
	_HdmiSinkAvCap.b_sink_3D_present = FALSE;
	_HdmiSinkAvCap.ui4_sink_cea_3D_resolution = 0;


	for (bTemp = 0; bTemp < bRetryCount; bTemp++) {
		if (hdmi_fgreadedid(i1noedid) == TRUE) {
			if (fgParserEDID(&_bEdidData[0]) == TRUE) {
				edid_parsing_result = TRUE;
				_HdmiSinkAvCap.b_sink_edid_ready = TRUE;

				break;
			}

			if (bTemp == bRetryCount - 1) {
				edid_parsing_result = TRUE;
			}
			if (bTemp == bRetryCount - 1) {

				break;
			}

		} else {

			if (bTemp == bRetryCount - 1) {
				if (fgIsHdmiNoEDIDCheck()) {
					vSetNoEdidChkInfo();
				}
				return;
			}
		}
		mdelay(5);
	}

	if ((edid_ext_block_num * EDID_BLOCK_LEN) < EDID_SIZE)  /* for Buffer Overflow error */
		vParserExtEDIDState(&_bEdidData[0]);

	if (fgIsHdmiNoEDIDCheck()) {
		vSetNoEdidChkInfo();
	}

	dprintf(0,"-->>>>>>>>>-get edid end->>>>>>>>>>\n");
	/*vShowEdidRawData();*/

}

unsigned char vCheckPcmBitSize(unsigned char ui1ChNumInx)
{
	unsigned char ui1Data, u1MaxBit;
	int i;

	u1MaxBit = PCM_16BIT;
	for (i = 6; i >= ui1ChNumInx; i--) {
		ui1Data = _HdmiSinkAvCap.ui1_sink_pcm_bit_size[i];

		if (ui1Data & (1 << PCM_24BIT)) {
			if (u1MaxBit < PCM_24BIT)
				u1MaxBit = PCM_24BIT;
		} else if (ui1Data & (1 << PCM_20BIT)) {
			if (u1MaxBit < PCM_20BIT)
				u1MaxBit = PCM_20BIT;
		}
	}

	return u1MaxBit;
}

unsigned int vOutputResolution(void)
{
	unsigned int ntsc_resolution,pal_resolution,prefered_resolution;
	ntsc_resolution = (_HdmiSinkAvCap.ui4_sink_cea_ntsc_resolution | _HdmiSinkAvCap.ui4_sink_dtd_ntsc_resolution);
	pal_resolution =(_HdmiSinkAvCap.ui4_sink_cea_pal_resolution | _HdmiSinkAvCap.ui4_sink_dtd_pal_resolution);
	pal_resolution = pal_resolution + ntsc_resolution;
	prefered_resolution = _HdmiSinkAvCap.ui4_sink_1st_dtd_ntsc_resolution | _HdmiSinkAvCap.ui4_sink_1st_dtd_pal_resolution;
	dprintf(0,"vOutputResolution:0x%x\n", pal_resolution);
	if (pal_resolution & SINK_1080P60)
		return HDMI_VIDEO_1920x1080p_60Hz;
	else if (pal_resolution & SINK_1080P50)
		return HDMI_VIDEO_1920x1080p_50Hz;
	else if (pal_resolution & SINK_720P60)
		return HDMI_VIDEO_1280x720p_60Hz;
	else if (pal_resolution & SINK_720P50)
		return HDMI_VIDEO_1280x720p_50Hz;
	else if (pal_resolution & SINK_576P)
		return HDMI_VIDEO_720x576p_50Hz;
	else if (pal_resolution & SINK_480P)
		return HDMI_VIDEO_720x480p_60Hz;
}
