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
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "glumboot.h"

struct mtd_partition {
	size_t order;
	struct glumboot_partition_type ptn;
};
struct mtd_partition partitions[GLUMBOOT_MAX_NR_PARTS];

int compare_offset(const void *s1, const void *s2)
{
	struct mtd_partition *p1 = (struct mtd_partition *)s1;
	struct mtd_partition *p2 = (struct mtd_partition *)s2;

	return p1->ptn.offset - p2->ptn.offset;
}

int main() {

	char *line = NULL;
	size_t size;
	size_t count = 0;
	int i, j;
	struct glumboot_partition_type *ptn;

	if (isatty(fileno(stdin))) {
		printf("please just pipe files. \n");
		exit(-1);
	}

	while (getline(&line, &size, stdin) != -1) {
		char name[20];
		uint32_t offset;
		uint32_t end;
		int ret;

		ret = sscanf(line, "0x%012llx-0x%012llx : \"%[^\"]\"", &offset, &end, name);
		if (ret != 3) {
			printf("error parsing line: %d\n", count+1);
			exit(-1);
		}

		partitions[count].order = count;
		ptn = &partitions[count].ptn;

		strcpy(ptn->name, name);
		ptn->offset = offset;
		ptn->size   = end - offset;
		ptn->flags  = 0;

		count++;
	}

	qsort(partitions, count, sizeof(struct mtd_partition), compare_offset);

	for (i = 0; i < count; i++) {
		ptn = &partitions[i].ptn;

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
		ptn++;
	}

	return 0;
}
