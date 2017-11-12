none:
	@echo "invalid target; targets: header, image"

build:
	@mkdir -p out/

header: build
	gcc -Iinc/ -o out/generate-glumboot-header src/generate-glumboot-header.c

image: build
	@gcc -Iinc/ -lz $(CFLAGS) -o out/generate-image src/generate-image.c

test: header
	# dummy
	@out/generate-glumboot-header < tests/01-dummy.txt > tests/01-dummy.h
	@make --no-print-directory image CFLAGS=-DGLUMBOOT_PARTITION_FILE="../tests/01-dummy.h"
	@out/generate-image --out out/01-dummy.img
	@md5sum -c tests/01-dummy.md5

	# wrong offsets
	@make --no-print-directory image CFLAGS=-DGLUMBOOT_PARTITION_FILE="../tests/02-offsets.h"
	@out/generate-image --fix-offsets --out out/02-offsets.img
	@md5sum -c tests/02-offsets.md5

	# missing part
	@make --no-print-directory image CFLAGS=-DGLUMBOOT_PARTITION_FILE="../tests/03-missing-misc.h"
	-out/generate-image --fix-offsets --out out/03-missing-misc.img

	# print table
	@make --no-print-directory image CFLAGS=-DGLUMBOOT_PARTITION_FILE="../tests/04-print-table.h"
	out/generate-image --fix-offsets --print-table --out out/04-print-table.img > out/04-table.txt
	@md5sum -c tests/04-print-table.md5

clean:
	@rm -rf out/
	@rm -rf src/partitions.h
	@rm -rf inc/partitions.h
	@rm -rf tests/01-dummy.h
