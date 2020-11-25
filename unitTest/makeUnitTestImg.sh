#!/usr/bin/env bash

dd if=/dev/zero bs=1024 count=4096 of=unitTest.bin
dd if=build/bootloader/bootloader.bin bs=1 seek=$((0x1000)) of=unitTest.bin conv=notrunc
dd if=build/partitions_singleapp.bin bs=1 seek=$((0x8000)) of=unitTest.bin conv=notrunc
dd if=build/PissbotTests.bin bs=1 seek=$((0x10000)) of=unitTest.bin conv=notrunc