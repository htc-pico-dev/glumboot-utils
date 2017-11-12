/*
 * author: thewisenerd <thewisenerd@protonmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <zlib.h>

static int verbose = 0;
static int fixup_offsets = 0;
static int skip_validation = 0;
static int print_table = 0;
static char *outfile = "file.img";

static struct option long_opts[] = {
	{"verbose",         no_argument,       NULL, 'v'},
	{"fix-offsets",     no_argument,       NULL, 'x'},
	{"skip-validation", no_argument,       NULL, 'N'},
	{"print-table",     no_argument,       NULL, 'p'},
	{"out",             required_argument, NULL, 'o'},
	{0, 0, 0, 0},
};

#include "glumboot.h"
struct mtd_partition {
	size_t order;
	struct glumboot_partition_type ptn;
};


static struct mtd_partition partitions[] = {
#ifdef GLUMBOOT_PARTITION_FILE

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)

#include STR(GLUMBOOT_PARTITION_FILE)
#else
#include "partitions.h"
#endif
};
static const size_t count = sizeof(partitions) / sizeof(struct mtd_partition);

static const char nullb = 0xFF;
static size_t padto = 256 * 1024; /* 256 kbyte */

static int compare_order(const void *s1, const void *s2)
{
	struct mtd_partition *p1 = (struct mtd_partition *)s1;
	struct mtd_partition *p2 = (struct mtd_partition *)s2;

	return p1->order - p2->order;
}

int compare_offset(const void *s1, const void *s2)
{
	struct mtd_partition *p1 = (struct mtd_partition *)s1;
	struct mtd_partition *p2 = (struct mtd_partition *)s2;

	return p1->ptn.offset - p2->ptn.offset;
}

static int validate_offsets(void)
{
	int i;
	struct mtd_partition *mtd_ptn;
	struct glumboot_partition_type *glum_ptn, *glum_ptn_next;
	mtd_ptn = partitions;

	for (i = 0; i < count-1; i++) {
		glum_ptn = &mtd_ptn->ptn;
		glum_ptn_next = &partitions[i+1].ptn;

		if (glum_ptn->offset + glum_ptn->size > glum_ptn_next->offset) {
			fprintf(stderr, "validate_offsets failed!\n");
			fprintf(stderr, "partition \"%s\" overlaps on partition \"%s\"\n", glum_ptn->name, glum_ptn_next->name);
			return -1;
		}

		mtd_ptn++;
	}

	return 0;
}

/* this is device specific code */
static int validate_device(void)
{
	int i;
	struct mtd_partition *mtd_ptn;
	struct glumboot_partition_type *glum_ptn;
	mtd_ptn = partitions;
	u32 end = partitions[count-1].ptn.offset + partitions[count-1].ptn.size;

	struct mtd_partition *recovery = NULL;
	struct mtd_partition *boot = NULL;
	struct mtd_partition *misc = NULL;

#define DEVICE_MEMORY (512 * 1024 * 1024)

	if (end > DEVICE_MEMORY) {
		fprintf(stderr, "partition table overflows device available memory\n");
		fprintf(stderr, "\t0x%012x > 0x%012x\n", end, DEVICE_MEMORY);
		return -1;
	}

	for (i = 0; i  < count; i++) {
		glum_ptn = &mtd_ptn->ptn;

		if (!strcmp(glum_ptn->name, "recovery"))
			recovery = mtd_ptn;
		if (!strcmp(glum_ptn->name, "boot"))
			boot = mtd_ptn;
		if (!strcmp(glum_ptn->name, "misc"))
			misc = mtd_ptn;

		mtd_ptn++;
	}

	if (!recovery || !boot || !misc) {
		fprintf(stderr, "missing required partition definitions:\n");
		if (!recovery) fprintf(stderr, "\trecovery\n");
		if (!boot) fprintf(stderr, "\tboot\n");
		if (!misc) fprintf(stderr, "\tmisc\n");

		return -1;
	}

	return 0;
}

static void dump_partitions(void)
{
	int i;
	struct mtd_partition *mtd_ptn;
	struct glumboot_partition_type *glum_ptn;
	mtd_ptn = partitions;

	printf("------------------------------------------------------\n");

	for (i = 0; i  < count; i++) {
		glum_ptn = &mtd_ptn->ptn;
		printf("0x%012llx-0x%012llx : \"%s\"\n",
			glum_ptn->offset, glum_ptn->offset + glum_ptn->size, glum_ptn->name);
		mtd_ptn++;
	}

	printf("------------------------------------------------------\n");
}

static void dump_table(void)
{
	int i, j;
	struct mtd_partition *mtd_ptn;
	struct glumboot_partition_type *ptn;

	/* fixup order */
	qsort(partitions, count, sizeof(struct mtd_partition), compare_order);
	for (i = 0; i  < count; i++)
		partitions[i].order = i;

	/* order by offsets */
	qsort(partitions, count, sizeof(struct mtd_partition), compare_offset);

	mtd_ptn = partitions;

	for (i = 0; i  < count; i++) {
		ptn = &mtd_ptn->ptn;

		printf("{ %d,\t", partitions[i].order);

		printf("{ ");

		printf("\"%s\",", ptn->name);
		j = 20 - strlen(ptn->name);
		while(j--) printf(" ");

		printf("0x%08x,\t", ptn->offset);
		printf("0x%08x,\t", ptn->size);
		printf("0x%08x,\t", ptn->flags);

		printf("},");

		printf(" },\n");


		mtd_ptn++;
	}

}

int real_main (int argc, char **argv)
{
	FILE * fp;
	int ret = 0, i, j;
	struct glumboot_partition_table table;
	struct glumboot_partition_type *glum_part, *glum_parts;

	struct mtd_partition *mtd_ptn;

	if (verbose) {
		puts("original: ");
		dump_partitions();
	}

	/* order by offsets */
	qsort(partitions, count, sizeof(struct mtd_partition), compare_offset);

	if (verbose) {
		puts("sorted: ");
		dump_partitions();
	}

	if (fixup_offsets) {
		mtd_ptn = partitions;
		for (i = 0; i < count-1; i++) {
			u32 next_offset = mtd_ptn->ptn.offset + mtd_ptn->ptn.size;
			partitions[i+1].ptn.offset = next_offset;
			mtd_ptn++;
		}

		if (verbose) {
			puts("fixup_offsets: ");
			dump_partitions();
		}
	}

	if (!skip_validation) {
		if (ret = validate_offsets()) {
			return ret;
		}

		if (ret = validate_device()) {
			return ret;
		}
	}

	/* write image */

	/* order by ptn order */
	qsort(partitions, count, sizeof(struct mtd_partition), compare_order);

	/* setup table */
	memset((void*)&table, 0, sizeof(table));
	memcpy(table.magic, "glumboot", 8);
	table.partition_count = count;

	/* setup partitions */
	glum_parts = malloc(sizeof(struct glumboot_partition_type) * count);
	if (!glum_parts) {
		fprintf(stderr, "malloc failed!");
		ret = -ENOMEM;
		goto out;
	}
	memset(glum_parts, 0, sizeof(struct glumboot_partition_type) * count);
	glum_part = glum_parts;

	if (verbose) {
		puts("glum_parts:");
		printf("------------------------------------------------------\n");
	}
	for (i = 0; i < count; i++) {
		strcpy(glum_part->name, partitions[i].ptn.name);
		glum_part->offset = partitions[i].ptn.offset;
		glum_part->size = partitions[i].ptn.size;
		glum_part->flags = partitions[i].ptn.flags;

		if (verbose) {
			printf("0x%012llx-0x%012llx : \"%s\"\n",
				glum_part->offset, glum_part->offset + glum_part->size, glum_part->name);
		}

		glum_part++;
	}
	if (verbose) {
		printf("------------------------------------------------------\n");
	}

	table.crc32 = crc32(0, (char *)glum_parts, sizeof(struct glumboot_partition_type) * count);
	if (verbose) {
		printf("crc32: 0x%08x\n", table.crc32);
	}

	/* write to file */
	fp = fopen (outfile, "w");
	if (fp == NULL) {
		ret = errno;
		fprintf(stderr, "error opening file \"%s\": %s\n", outfile, strerror( ret ));
		goto out;
	}

	fwrite(&table, sizeof(table), 1, fp);
	fwrite(glum_parts, sizeof(struct glumboot_partition_type) * count, 1, fp);

	padto -= (sizeof(struct glumboot_partition_type) * count) + (sizeof(table));
	while(padto--) {
		fwrite(&nullb, 1, 1, fp);
	}

out:
	if (fp)
		fclose(fp);
	if (glum_parts)
		free(glum_parts);
	if (ret == 0 && print_table)
		dump_table();

	return ret;
}

int main (int argc, char **argv)
{
	int ret;
	int optidx;

	while ((ret = getopt_long(argc, argv, "vxNpo:", long_opts, &optidx)) != -1) {
		switch(ret) {
		case 'v':
			verbose = 1;
			break;

		case 'o':
			outfile = optarg;
			break;

		case 'x':
			fixup_offsets = 1;
			break;

		case 'N':
			skip_validation = 1;
			break;

		case 'p':
			print_table = 1;
			break;

		case '?':
			/* getopt_long already printed an error message. */
			return -1;

		default:
			abort ();
		}
	}

	if (verbose) {
		printf("verbose         : %s\n", verbose ? "true": "false");
		printf("fixup_offsets   : %s\n", fixup_offsets ? "true": "false");
		printf("skip_validation : %s\n", skip_validation ? "true": "false");
		printf("outfile         : %s\n", outfile);
	}

	return real_main(argc, argv);
}
