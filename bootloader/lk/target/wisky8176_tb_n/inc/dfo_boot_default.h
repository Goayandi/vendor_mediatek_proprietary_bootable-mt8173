#ifndef DFO_BOOT_DEFAULT_H
#define DFO_BOOT_DEFAULT_H

#define DFO_BOOT_COUNT 14

typedef struct {
	char name[32];   // kernel dfo name
	unsigned long value;       // kernel dfo value
} dfo_boot_info;

const dfo_boot_info dfo_boot_default[DFO_BOOT_COUNT] = {
	// boot dfo 0
	{
		"MD5_SIZE",
		0x01600000
	},

	// boot dfo 1
	{
		"MD5_SMEM_SIZE",
		0x00200000
	},

	// boot dfo 2
	{
		"MTK_MD5_SUPPORT",
		5
	},

	// boot dfo 3
	{
		"MTK_ENABLE_MD5",
		0
	},

	// boot dfo 4
	{
		"MTK_ENABLE_MD1",
		1
	},

	// boot dfo 5
	{
		"MTK_ENABLE_MD2",
		0
	},

	// boot dfo 6
	{
		"MD1_SIZE",
		0x06000000
	},

	// boot dfo 7
	{
		"MD2_SIZE",
		0x01600000
	},

	// boot dfo 8
	{
		"MD1_SMEM_SIZE",
		0x00000800
	},

	// boot dfo 9
	{
		"MD2_SMEM_SIZE",
		0x00200000
	},

	// boot dfo 10
	{
		"MTK_MD1_SUPPORT",
		5
	},

	// boot dfo 11
	{
		"MTK_MD2_SUPPORT",
		4
	},

	// boot dfo 12
	{
		"LCM_FAKE_WIDTH",
		0
	},

	// boot dfo 13
	{
		"LCM_FAKE_HEIGHT",
		0
	}
};

#endif
