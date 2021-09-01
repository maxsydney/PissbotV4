#!/usr/bin/env bash

dd if=/dev/zero bs=1024 count=4096 of=outputs/unitTest.bin
dd if=build/bootloader/bootloader.bin bs=1 seek=$((0x1000)) of=outputs/unitTest.bin conv=notrunc
dd if=build/partition_table/partition-table.bin bs=1 seek=$((0x8000)) of=outputs/unitTest.bin conv=notrunc
dd if=build/unitTests.bin bs=1 seek=$((0x10000)) of=outputs/unitTest.bin conv=notrunc