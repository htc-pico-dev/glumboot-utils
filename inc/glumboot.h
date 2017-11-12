/*
 * author: thewisenerd <thewisenerd@protonmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef __MTD_GLUMBOOT_H__
#define __MTD_GLUMBOOT_H__

#include <stdint.h>
typedef uint8_t u8;
typedef uint32_t u32;

#define GLUMBOOT_MAX_NR_PARTS 10

struct glumboot_partition_type {
	char name[20];
	u32 offset;
	u32 size;
	u32 flags;
};

struct glumboot_partition_table {
	char magic[8];				/* "glumboot" */
	u8 reserved__1[4];			/*  reserved */
	u32 partition_count;
	u32 crc32;
	u8 reserved__2[12];			/*  reserved */
	struct glumboot_partition_type partitions[0];
};

#endif /* __MTD_GLUMBOOT_H__ */
