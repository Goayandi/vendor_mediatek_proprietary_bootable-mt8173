#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <debug.h>
#include <reg.h>
#include <target.h>
#include <board.h>

#include <platform/timer.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>

#define LP8557_BL_CMD           0x00
#define LP8557_BL_MASK          0x01
#define LP8557_BL_ON            0x01
#define LP8557_BL_OFF           0x00
#define LP8557_BRIGHTNESS_CTRL      0x04
#define LP8557_CONFIG           0x10
#define LP8557_EPROM_START      0x10
#define LP8557_EPROM_END        0x1E
/*
 * write 8bit
 */
int i2c_w8(int bus, uint8_t flags, uint16_t address, uint8_t reg, uint8_t val)
{
	int ret;
	struct mt_i2c_t i2c;
	uint8_t data[4];

	memset(&i2c, 0x0, sizeof(i2c));
	i2c.id = bus;
	//i2c.dir = flags;
	i2c.addr = address;
	i2c.mode = ST_MODE;
	i2c.speed = 100;
	//i2c.is_rs_enable = 1;

	data[0] = reg;
	data[1] = val;
	//ret = mt_i2c_write_new(&i2c, data, 2);
	ret = i2c_write(&i2c, data, 2);

	return ret;

}

/*
 * read 8bit
 */
int i2c_r8(int bus, uint8_t flags, uint16_t address, uint8_t reg, uint8_t *val)
{
	int ret;
	struct mt_i2c_t i2c;
	uint8_t data[4];

	memset(&i2c, 0x0, sizeof(i2c));
	i2c.id = bus;
	//i2c.dir = flags;
	i2c.addr = address;
	i2c.mode = ST_MODE;
	i2c.speed = 100;

	data[0] = reg;
	ret = i2c_read(&i2c, data, 1);   // set register command
	if (ret == I2C_OK) {
		*val = data[0];
	}

	return ret;
}

/*
 * Enable backlight to some default value
 */
int backlight_on(void)
{
	printf("[backlight]:backlight_on\n");
	mt_set_gpio_mode(GPIOEXT14, GPIO_MODE_GPIO);
	mt_set_gpio_dir(GPIOEXT14, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIOEXT14, GPIO_OUT_ONE);
	mdelay(100);

	i2c_w8(4, 0, 0x2c, LP8557_CONFIG, 4); /* configure i2c & disable led open detection */
	i2c_w8(4, 0, 0x2c, 0x14, 0xdf); /* 4V OV, 5 LED string enabled */
	i2c_w8(4, 0, 0x2c, 0x04, 0x80);
	i2c_w8(4, 0, 0x2c, LP8557_BL_CMD, 1);

	return 0;
}
/*
 * Disable backlight
 */
int backlight_off(void)
{
	printf("[backlight]:backlight_off\n");
	mt_set_gpio_mode(GPIOEXT14, GPIO_MODE_GPIO);
	mt_set_gpio_dir(GPIOEXT14, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIOEXT14, GPIO_OUT_ZERO);
	return 0;
}

