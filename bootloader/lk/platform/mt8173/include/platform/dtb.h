#ifndef _DTB_H_
#define _DTB_H_

#define DTB_MAGIC "MTDT"

struct dtb_header {
	char magic[4];
	unsigned int version;
	unsigned int num_of_dtbs;
};

struct dtb_entry {
	unsigned int platform_id;
	unsigned int version_id;
	unsigned int soc_rev;
	unsigned int offset;
	unsigned int size;
};

#endif
