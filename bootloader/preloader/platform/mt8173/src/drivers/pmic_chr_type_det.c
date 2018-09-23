
#include <typedefs.h>
#include <platform.h>
#include <pmic.h>

void upmu_set_rg_bc11_bias_en(U32 val)
{
	U32 ret=0;

	ret=pmic_config_interface( (U32)(CHR_CON19),
	                           (U32)(val),
	                           (U32)(PMIC_RG_BC11_BIAS_EN_MASK),
	                           (U32)(PMIC_RG_BC11_BIAS_EN_SHIFT)
	                         );
}

void upmu_set_rg_bc11_vsrc_en(U32 val)
{
	U32 ret=0;

	ret=pmic_config_interface( (U32)(CHR_CON18),
	                           (U32)(val),
	                           (U32)(PMIC_RG_BC11_VSRC_EN_MASK),
	                           (U32)(PMIC_RG_BC11_VSRC_EN_SHIFT)
	                         );
}

void upmu_set_rg_bc11_vref_vth(U32 val)
{
	U32 ret=0;

	ret=pmic_config_interface( (U32)(CHR_CON19),
	                           (U32)(val),
	                           (U32)(PMIC_RG_BC11_VREF_VTH_MASK),
	                           (U32)(PMIC_RG_BC11_VREF_VTH_SHIFT)
	                         );
}

void upmu_set_rg_bc11_ipu_en(U32 val)
{
	U32 ret=0;

	ret=pmic_config_interface( (U32)(CHR_CON19),
	                           (U32)(val),
	                           (U32)(PMIC_RG_BC11_IPU_EN_MASK),
	                           (U32)(PMIC_RG_BC11_IPU_EN_SHIFT)
	                         );
}

void upmu_set_rg_bc11_ipd_en(U32 val)
{
	U32 ret=0;

	ret=pmic_config_interface( (U32)(CHR_CON19),
	                           (U32)(val),
	                           (U32)(PMIC_RG_BC11_IPD_EN_MASK),
	                           (U32)(PMIC_RG_BC11_IPD_EN_SHIFT)
	                         );
}

void upmu_set_rg_bc11_cmp_en(U32 val)
{
	U32 ret=0;

	ret=pmic_config_interface( (U32)(CHR_CON19),
	                           (U32)(val),
	                           (U32)(PMIC_RG_BC11_CMP_EN_MASK),
	                           (U32)(PMIC_RG_BC11_CMP_EN_SHIFT)
	                         );
}

void upmu_set_rg_bc11_rst(U32 val)
{
	U32 ret=0;

	ret=pmic_config_interface( (U32)(CHR_CON18),
	                           (U32)(val),
	                           (U32)(PMIC_RG_BC11_RST_MASK),
	                           (U32)(PMIC_RG_BC11_RST_SHIFT)
	                         );
}

void upmu_set_rg_bc11_bb_ctrl(U32 val)
{
	U32 ret=0;

	ret=pmic_config_interface( (U32)(CHR_CON18),
	                           (U32)(val),
	                           (U32)(PMIC_RG_BC11_BB_CTRL_MASK),
	                           (U32)(PMIC_RG_BC11_BB_CTRL_SHIFT)
	                         );
}

U32 upmu_get_rgs_bc11_cmp_out(void)
{
	U32 ret=0;
	U32 val=0;

	ret=pmic_read_interface( (U32)(CHR_CON18),
	                         (&val),
	                         (U32)(PMIC_RGS_BC11_CMP_OUT_MASK),
	                         (U32)(PMIC_RGS_BC11_CMP_OUT_SHIFT)
	                       );
	return val;
}


// ============================================================ //
//extern function
// ============================================================ //
extern void charger_detect_init(void);
extern void charger_detect_release(void);

static void hw_bc11_init(void)
{
	charger_detect_init();

	//RG_BC11_BIAS_EN=1
	upmu_set_rg_bc11_bias_en(0x1);
	//RG_BC11_VSRC_EN[1:0]=00
	upmu_set_rg_bc11_vsrc_en(0x0);
	//RG_BC11_VREF_VTH = [1:0]=00
	upmu_set_rg_bc11_vref_vth(0x0);
	//RG_BC11_CMP_EN[1.0] = 00
	upmu_set_rg_bc11_cmp_en(0x0);
	//RG_BC11_IPU_EN[1.0] = 00
	upmu_set_rg_bc11_ipu_en(0x0);
	//RG_BC11_IPD_EN[1.0] = 00
	upmu_set_rg_bc11_ipd_en(0x0);
	//BC11_RST=1
	upmu_set_rg_bc11_rst(0x1);
	//BC11_BB_CTRL=1
	upmu_set_rg_bc11_bb_ctrl(0x1);

	//msleep(10);
	mdelay(50);
}

static U32 hw_bc11_DCD(void)
{
	U32 wChargerAvail = 0;

	//RG_BC11_IPU_EN[1.0] = 10
	upmu_set_rg_bc11_ipu_en(0x2);
	//RG_BC11_IPD_EN[1.0] = 01
	upmu_set_rg_bc11_ipd_en(0x1);
	//RG_BC11_VREF_VTH = [1:0]=01
	upmu_set_rg_bc11_vref_vth(0x1);
	//RG_BC11_CMP_EN[1.0] = 10
	upmu_set_rg_bc11_cmp_en(0x2);

	//msleep(20);
	mdelay(80);

	wChargerAvail = upmu_get_rgs_bc11_cmp_out();

	//RG_BC11_IPU_EN[1.0] = 00
	upmu_set_rg_bc11_ipu_en(0x0);
	//RG_BC11_IPD_EN[1.0] = 00
	upmu_set_rg_bc11_ipd_en(0x0);
	//RG_BC11_CMP_EN[1.0] = 00
	upmu_set_rg_bc11_cmp_en(0x0);
	//RG_BC11_VREF_VTH = [1:0]=00
	upmu_set_rg_bc11_vref_vth(0x0);

	return wChargerAvail;
}

static U32 hw_bc11_stepA2(void)
{
	U32 wChargerAvail = 0;

	//RG_BC11_VSRC_EN[1.0] = 10
	upmu_set_rg_bc11_vsrc_en(0x2);
	//RG_BC11_IPD_EN[1:0] = 01
	upmu_set_rg_bc11_ipd_en(0x1);
	//RG_BC11_VREF_VTH = [1:0]=00
	upmu_set_rg_bc11_vref_vth(0x0);
	//RG_BC11_CMP_EN[1.0] = 01
	upmu_set_rg_bc11_cmp_en(0x1);

	//msleep(80);
	mdelay(80);

	wChargerAvail = upmu_get_rgs_bc11_cmp_out();

	//RG_BC11_VSRC_EN[1:0]=00
	upmu_set_rg_bc11_vsrc_en(0x0);
	//RG_BC11_IPD_EN[1.0] = 00
	upmu_set_rg_bc11_ipd_en(0x0);
	//RG_BC11_CMP_EN[1.0] = 00
	upmu_set_rg_bc11_cmp_en(0x0);

	return  wChargerAvail;
}

static U32 hw_bc11_stepB2(void)
{
	U32 wChargerAvail = 0;

	//RG_BC11_IPU_EN[1:0]=10
	upmu_set_rg_bc11_ipu_en(0x2);
	//RG_BC11_VREF_VTH = [1:0]=10
	upmu_set_rg_bc11_vref_vth(0x1);
	//RG_BC11_CMP_EN[1.0] = 01
	upmu_set_rg_bc11_cmp_en(0x1);

	//msleep(80);
	mdelay(80);

	wChargerAvail = upmu_get_rgs_bc11_cmp_out();

	//RG_BC11_IPU_EN[1.0] = 00
	upmu_set_rg_bc11_ipu_en(0x0);
	//RG_BC11_CMP_EN[1.0] = 00
	upmu_set_rg_bc11_cmp_en(0x0);
	//RG_BC11_VREF_VTH = [1:0]=00
	upmu_set_rg_bc11_vref_vth(0x0);

	return  wChargerAvail;
}

static void hw_bc11_done(void)
{
	//RG_BC11_VSRC_EN[1:0]=00
	upmu_set_rg_bc11_vsrc_en(0x0);
	//RG_BC11_VREF_VTH = [1:0]=0
	upmu_set_rg_bc11_vref_vth(0x0);
	//RG_BC11_CMP_EN[1.0] = 00
	upmu_set_rg_bc11_cmp_en(0x0);
	//RG_BC11_IPU_EN[1.0] = 00
	upmu_set_rg_bc11_ipu_en(0x0);
	//RG_BC11_IPD_EN[1.0] = 00
	upmu_set_rg_bc11_ipd_en(0x0);
	//RG_BC11_BIAS_EN=0
	upmu_set_rg_bc11_bias_en(0x0);

	charger_detect_release();
}

CHARGER_TYPE hw_charger_type_detection(void)
{
	CHARGER_TYPE ret = CHARGER_UNKNOWN;
	hw_bc11_init();

	if (1 == hw_bc11_DCD()) {
		ret = NONSTANDARD_CHARGER;
	} else {
		if (1 == hw_bc11_stepA2()) {
			if (1 == hw_bc11_stepB2()) {
				ret = STANDARD_CHARGER;
			} else {
				ret = CHARGING_HOST;
			}
		} else {
			ret = STANDARD_HOST;
		}
	}
	hw_bc11_done();

	return ret;
}

