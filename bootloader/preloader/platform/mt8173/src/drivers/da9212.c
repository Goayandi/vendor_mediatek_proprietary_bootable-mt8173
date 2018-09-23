#include "platform.h"
#include "i2c.h"
#include "gpio.h"
#include "da9212.h"
#include "cust_i2c.h"
#include "cust_gpio_usage.h"

/**********************************************************
  *   I2C Slave Setting
  *********************************************************/
#define DA9212_SLAVE_ADDR_WRITE   0xD0
#define DA9212_SLAVE_ADDR_READ    0xD1

/**********************************************************
  *   Global Variable
  *********************************************************/
#ifdef I2C_EXT_BUCK_CHANNEL
#define da9212_I2C_ID I2C_EXT_BUCK_CHANNEL
#else
#define da9212_I2C_ID I2C5
#endif

#ifndef GPIO_EXT_BUCK_IC_EN_PIN
unsigned int g_vproc_en_gpio_number = 0;
#else
unsigned int g_vproc_en_gpio_number = GPIO_EXT_BUCK_IC_EN_PIN;
#endif

#ifndef GPIO_EXT_BUCK_EN_A_PIN
unsigned int g_vproc_en_a_gpio_number = 0;
#else
unsigned int g_vproc_en_a_gpio_number = GPIO_EXT_BUCK_EN_A_PIN;
#endif

#ifndef GPIO_EXT_BUCK_EN_B_PIN
unsigned int g_vproc_en_b_gpio_number = 0;
#else
unsigned int g_vproc_en_b_gpio_number = GPIO_EXT_BUCK_EN_B_PIN;
#endif

static struct mt_i2c_t da9212_i2c;

int g_da9212_driver_ready = 0;
int g_da9212_hw_exist = 0;

#define da9212_print(fmt, args...) printf(fmt, ##args);

/**********************************************************
  *
  *   [I2C Function For Read/Write da9212]
  *
  *********************************************************/
kal_uint32 da9212_write_byte(kal_uint8 addr, kal_uint8 value)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint8 write_data[2];
	kal_uint16 len;

	write_data[0] = addr;
	write_data[1] = value;

	da9212_i2c.id = da9212_I2C_ID;
	/* Since i2c will left shift 1 bit,
		we need to set da9212 I2C address to >>1 */
	da9212_i2c.addr = (DA9212_SLAVE_ADDR_WRITE >> 1);
	da9212_i2c.mode = ST_MODE;
	da9212_i2c.speed = 100;
	len = 2;

	ret_code = i2c_write(&da9212_i2c, write_data, len);
	/* da9212_print("%s: i2c_write: ret_code: %d\n", __func__, ret_code); */

	if (ret_code == 0)
		return 1; /* ok */
	else
		return 0; /* fail */
}

kal_uint32 da9212_read_byte(kal_uint8 addr, kal_uint8 *dataBuffer)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint16 len;
	*dataBuffer = addr;

	da9212_i2c.id = da9212_I2C_ID;
	/* Since i2c will left shift 1 bit,
		we need to set da9212 I2C address to >>1 */
	da9212_i2c.addr = (DA9212_SLAVE_ADDR_WRITE >> 1);
	da9212_i2c.mode = ST_MODE;
	da9212_i2c.speed = 100;
	len = 1;

	ret_code = i2c_write_read(&da9212_i2c, dataBuffer, len, len);
	/* da9212_print("%s: i2c_read: ret_code: %d\n", __func__, ret_code); */

	if (ret_code == 0)
		return 1;  /* ok */
	else
		return 0; /* fail */
}

/**********************************************************
  *
  *   [Read / Write Function]
  *
  *********************************************************/
kal_uint32 da9212_read_interface(kal_uint8 RegNum,
kal_uint8 *val, kal_uint8 MASK, kal_uint8 SHIFT)
{
	kal_uint8 da9212_reg = 0;
	kal_uint32 ret = 0;

	ret = da9212_read_byte(RegNum, &da9212_reg);

	da9212_reg &= (MASK << SHIFT);
	*val = (da9212_reg >> SHIFT);

	return ret;
}

kal_uint32 da9212_config_interface(kal_uint8 RegNum,
kal_uint8 val, kal_uint8 MASK, kal_uint8 SHIFT)
{
	kal_uint8 da9212_reg = 0;
	kal_uint32 ret = 0;

	ret = da9212_read_byte(RegNum, &da9212_reg);

	da9212_reg &= ~(MASK << SHIFT);
	da9212_reg |= (val << SHIFT);

	ret = da9212_write_byte(RegNum, da9212_reg);

	return ret;
}

kal_uint32 da9212_get_reg_value(kal_uint32 reg)
{
	kal_uint32 ret = 0;
	kal_uint8 reg_val = 0;

	ret = da9212_read_interface((kal_uint8) reg, &reg_val, 0xFF, 0x0);

	if (ret == 0)
		da9212_print("%d", ret);
	return reg_val;
}

void da9212_dump_register(void)
{
	kal_uint8 i = 0;

	da9212_print("[da9212] page 0,1: ");
	da9212_print("[da9212] [0x%x]=0x%x ", 0x0, da9212_get_reg_value(0x0));
	for (i = DA9212_REG_STATUS_A; i <= DA9212_REG_BUCKB_CONT; i++)
		da9212_print("[da9212] [0x%x]=0x%x\n",
			i, da9212_get_reg_value(i));

	for (i = DA9212_REG_BUCK_ILIM; i <= DA9212_REG_VBUCKB_B; i++)
		da9212_print("[da9212] [0x%x]=0x%x\n",
			i, da9212_get_reg_value(i));

	da9212_print("[da9212] page 2,3: ");
	for (i = 0x05; i <= 0x06; i++) {
		da9212_config_interface(DA9212_REG_PAGE_CON, 0x2, 0xF, 0);
		da9212_print("[da9212] [0x%x]=0x%x\n",
			i, da9212_get_reg_value(i));
	}
	for (i = 0x43; i <= 0x4F; i++) {
		da9212_config_interface(DA9212_REG_PAGE_CON, 0x2, 0xF, 0);
		da9212_print("[da9212] [0x%x]=0x%x\n",
			i, da9212_get_reg_value(i));
	}
	da9212_config_interface(DA9212_REG_PAGE_CON, 0x0, 0xF, 0);
}

int get_da9212_i2c_ch_num(void)
{
	return da9212_I2C_ID;
}

void ext_buck_en(int val)
{
	if (g_vproc_en_gpio_number != 0) {
		mt_set_gpio_mode(g_vproc_en_gpio_number, 0); /* 0:GPIO mode */
		mt_set_gpio_dir(g_vproc_en_gpio_number, 1); /* dir = output */
		mt_set_gpio_out(g_vproc_en_gpio_number, val);
	}
	if (g_vproc_en_a_gpio_number != 0) {
		mt_set_gpio_mode(g_vproc_en_a_gpio_number, 0); /* 0:GPIO mode */
		mt_set_gpio_dir(g_vproc_en_a_gpio_number, 1); /* dir = output */
		mt_set_gpio_out(g_vproc_en_a_gpio_number, val);
	}
	if (g_vproc_en_b_gpio_number != 0) {
		mt_set_gpio_mode(g_vproc_en_b_gpio_number, 0); /* 0:GPIO mode */
		mt_set_gpio_dir(g_vproc_en_b_gpio_number, 1); /* dir = output */
		mt_set_gpio_out(g_vproc_en_b_gpio_number, val);
	}
}

void da9212_hw_init(void)
{
	kal_uint8 reg_val = 0;
	kal_uint32 ret = 0;
	/* page select to 0 after one access */
	ret = da9212_config_interface(DA9212_REG_PAGE_CON,
			0x0, 0xF, DA9212_PEG_PAGE_SHIFT);
	/* BUCKA_EN = 1 */
	ret = da9212_config_interface(DA9212_REG_BUCKA_CONT,
			DA9212_BUCK_ON, 0x1, DA9212_BUCK_EN_SHIFT);
	/* BUCKB_EN = 1 */
	ret = da9212_config_interface(DA9212_REG_BUCKB_CONT,
			DA9212_BUCK_ON, 0x1, DA9212_BUCK_EN_SHIFT);
	/* GPIO setting*/
	ret = da9212_config_interface(DA9212_REG_GPIO_0_1,
			0x4, 0xF, DA9212_GPIO0_PIN_SHIFT);
	ret = da9212_config_interface(DA9212_REG_GPIO_0_1,
			0x4, 0xF, DA9212_GPIO1_PIN_SHIFT);
	ret = da9212_config_interface(DA9212_REG_GPIO_2_3,
			0x7, 0xF, DA9212_GPIO2_PIN_SHIFT);
	ret = da9212_config_interface(DA9212_REG_GPIO_2_3,
			0x7, 0xF, DA9212_GPIO3_PIN_SHIFT);
	ret = da9212_config_interface(DA9212_REG_GPIO_4,
			0x04, 0xFF, DA9212_GPIO4_PIN_SHIFT);
	/* BUCKA_GPI = GPIO0 */
#ifdef GPIO_EXT_BUCK_EN_A_PIN
	ret = da9212_config_interface(DA9212_REG_BUCKA_CONT,
			0x01, 0x03, DA9212_BUCK_GPI_SHIFT);
#else
	ret = da9212_config_interface(DA9212_REG_BUCKA_CONT,
			0x00, 0x03, DA9212_BUCK_GPI_SHIFT);
#endif
	/* BUCKB_GPI = GPIO1 */
#ifdef GPIO_EXT_BUCK_EN_B_PIN
	ret = da9212_config_interface(DA9212_REG_BUCKB_CONT,
			0x02, 0x03, DA9212_BUCK_GPI_SHIFT);
#else
	ret = da9212_config_interface(DA9212_REG_BUCKB_CONT,
			0x00, 0x03, DA9212_BUCK_GPI_SHIFT);
#endif
	ret = da9212_config_interface(DA9212_REG_BUCKA_CONT,
			0x00, 0x01, DA9212_VBUCK_SEL_SHIFT); /* VBUCKA_A */
	ret = da9212_config_interface(DA9212_REG_BUCKB_CONT,
			0x00, 0x01, DA9212_VBUCK_SEL_SHIFT); /* VBUCKB_A */
	/* VBUCKA_GPI = None */
	ret = da9212_config_interface(DA9212_REG_BUCKA_CONT,
			0x00, 0x03, DA9212_VBUCK_GPI_SHIFT);
	/* VBUCKB_GPI = None */
	ret = da9212_config_interface(DA9212_REG_BUCKB_CONT,
			0x00, 0x03, DA9212_VBUCK_GPI_SHIFT);

	ret = da9212_config_interface(DA9212_REG_PAGE_CON,
			DA9212_REG_PAGE4, 0xF, DA9212_PEG_PAGE_SHIFT);
	da9212_read_interface(DA9212_VARIANT_ID, &reg_val, 0xFF, 0);
	ret = da9212_config_interface(DA9212_REG_PAGE_CON,
			DA9212_REG_PAGE0, 0xF, DA9212_PEG_PAGE_SHIFT);
	if (reg_val == DA9212_VARIANT_ID_AB) {
		/* Disable force PWM mode (this is reserve register) */
		da9212_print("[da9212] 1DA9212_VARIANT_ID = 0x%x ", reg_val);
		ret = da9212_config_interface(DA9212_REG_BUCKA_CONF,
				DA9212_BUCK_MODE_PWM, 0x3, DA9212_BUCK_MODE_SHIFT);
		/* Disable force PWM mode (this is reserve register) */
		ret = da9212_config_interface(DA9212_REG_BUCKB_CONF,
				DA9212_BUCK_MODE_PWM, 0x3, DA9212_BUCK_MODE_SHIFT);
	} else {
		da9212_print("[da9212] 2DA9212_VARIANT_ID = 0x%x ", reg_val);
		/* Disable force AUTO mode (this is reserve register) */
		ret = da9212_config_interface(DA9212_REG_BUCKA_CONF,
				DA9212_BUCK_MODE_AUTO, 0x3, DA9212_BUCK_MODE_SHIFT);
		/* Disable force AUTO mode (this is reserve register) */
		ret = da9212_config_interface(DA9212_REG_BUCKB_CONF,
				DA9212_BUCK_MODE_AUTO, 0x3, DA9212_BUCK_MODE_SHIFT);
	}

	/* PWM mode/1.0V, Setting VBUCKA_A = 1.0V */
	ret = da9212_config_interface(DA9212_REG_VBUCKA_A,
			0x46, 0xFF, DA9212_VBUCK_SHIFT);
	/* PWM mode/1.0V, Setting VBUCKB_A = 1.0V */
	ret = da9212_config_interface(DA9212_REG_VBUCKB_A,
			0x46, 0xFF, DA9212_VBUCK_SHIFT);
}

void da9212_hw_component_detect(void)
{
	kal_uint32 ret = 0;
	kal_uint8 val = 0;
	//da9212_print("[da9212] [0x%x]=0x%x ", 0x0, da9212_get_reg_value(0x0));
	/* select to page 2, clear REVERT at first time*/
	ret = da9212_config_interface(DA9212_REG_PAGE_CON,
			0x2, 0xFF, DA9212_PEG_PAGE_SHIFT);

	ret = da9212_read_interface(0x5, &val, 0xF, 4);

	/* check default SPEC. value */
	if (val == 0xD)
		g_da9212_hw_exist = 1;
	else
		g_da9212_hw_exist = 0;

	//da9212_print("%s: val = %d\n", __func__, val);
}

int is_da9212_sw_ready(void)
{
	da9212_print("g_da9212_driver_ready = %d\n", g_da9212_driver_ready);
	return g_da9212_driver_ready;
}

int is_da9212_exist(void)
{
	da9212_print("g_da9212_hw_exist = %d\n", g_da9212_hw_exist);
	return g_da9212_hw_exist;
}

void da9212_driver_probe(void)
{
	da9212_hw_component_detect();
	if (g_da9212_hw_exist == 1) {
		da9212_hw_init();
		//da9212_dump_register();
	} else {
		da9212_print("[da9212_driver_probe] PL da9212 is not exist\n");
	}
	g_da9212_driver_ready = 1;

	da9212_print("PL g_da9212_hw_exist=%d, g_da9212_driver_ready = %d\n",
		g_da9212_hw_exist, g_da9212_driver_ready);
}
