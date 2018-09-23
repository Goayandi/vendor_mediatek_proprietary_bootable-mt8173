
#include <string.h>
#include <printf.h>
#include <platform/mtk_ir_lk_core.h>
#include <sys/types.h>


u32  mtk_ir_nec_decode( void * preserve)
{
	u32 _au4IrRxData[2];
	u32 _u4Info = IR_READ32(IRRX_COUNT_HIGH_REG);
	u32 u4BitCnt = NEC_INFO_TO_BITCNT(_u4Info);
	char *pu1Data = (char *)_au4IrRxData;
	static u32 _u4PrevKey = BTN_REPEAT;   // pre key

	_au4IrRxData[0] = IR_READ32(IRRX_COUNT_MID_REG);//NEC 's code data is in this register
	_au4IrRxData[1] = IR_READ32(IRRX_COUNT_LOW_REG);

	if ((0 != _au4IrRxData[0]) || (0 != _au4IrRxData[1]) || _u4Info != 0) {
		IR_LOG_ALWAYS( "RxIsr Info:0x%08x data: 0x%08x%08x\n", _u4Info,
		               _au4IrRxData[1], _au4IrRxData[0]);
	} else {
		return BTN_INVALID_KEY;
	}

	/* Check repeat key. */
	if ((u4BitCnt == MTK_NEC_BITCNT_REPEAT) ) {
		if (((NEC_INFO_TO_1STPULSE(_u4Info) == MTK_NEC_1ST_PULSE_REPEAT) ||
		        (NEC_INFO_TO_1STPULSE(_u4Info) == MTK_NEC_1ST_PULSE_REPEAT - 1) ||
		        (NEC_INFO_TO_1STPULSE(_u4Info) == MTK_NEC_1ST_PULSE_REPEAT + 1)) &&
		        (NEC_INFO_TO_2NDPULSE(_u4Info) == 0) &&
		        (NEC_INFO_TO_3RDPULSE(_u4Info) == 0)) {
			goto end;          // repeat key
		} else {
			IR_LOG_ALWAYS("invalid repeat key!!!\n");
			_u4PrevKey = BTN_NONE;
			goto end;
		}
	}

	/* Check invalid pulse. */
	if (u4BitCnt != MTK_NEC_BITCNT_NORMAL) {
		IR_LOG_ALWAYS("u4BitCnt(%d), should be(%d)!!!\n",u4BitCnt, MTK_NEC_BITCNT_NORMAL );
		_u4PrevKey = BTN_NONE;
		goto end;
	}
	/* Check invalid key. */
	if ((pu1Data[2] + pu1Data[3]) != MTK_NEC_BIT8_VERIFY) {
		IR_LOG_ALWAYS("invalid nec key code!!!\n");
		_u4PrevKey = BTN_NONE;
		goto end;
	}
	_u4PrevKey = pu1Data[2];
end:

	return _u4PrevKey;

}

static u32  mtk_ir_rc6_decode( void * preserve)
{

	u32 _au4IrRxData[2];
	u32 _u4Info = IR_READ32(IRRX_COUNT_HIGH_REG);
	_au4IrRxData[0] = IR_READ32(IRRX_COUNT_MID_REG);//NEC 's code data is in this register
	_au4IrRxData[1] = IR_READ32(IRRX_COUNT_LOW_REG);

	char *pu1Data = (char *)_au4IrRxData;
	u16 u4BitCnt = RC6_INFO_TO_BITCNT(_u4Info);

	u16 u2RC6Leader = MTK_RC6_GET_LEADER(pu1Data[0]);
	u16 u2RC6Custom = MTK_RC6_GET_CUSTOM(pu1Data[0], pu1Data[1]);
	u16 u2RC6Toggle = MTK_RC6_GET_TOGGLE(pu1Data[0]);
	u32 u4RC6key = MTK_RC6_GET_KEYCODE(pu1Data[1], pu1Data[2]);

	IR_LOG_ALWAYS( "RxIsr Info:0x%08x data: 0x%08x%08x\n", _u4Info,
	               _au4IrRxData[1], _au4IrRxData[0]);

	IR_LOG_ALWAYS( "Bitcnt: 0x%02x, Leader: 0x%02x, Custom: 0x%02x, Toggle: 0x%02x, Keycode; 0x%02x\n",
	               u4BitCnt, u2RC6Leader,u2RC6Custom,u2RC6Toggle,u4RC6key);



	/* Check data. */
	if ((u4BitCnt != MTK_RC6_BITCNT)
	        || (pu1Data == NULL)
	        || (u2RC6Leader != MTK_RC6_LEADER)) {

		return BTN_INVALID_KEY;
	}


	return u4RC6key;


}



