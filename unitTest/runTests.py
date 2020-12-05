import os

print("Running unit tests")
os.system("make -j12")
os.system(".makeUnitTestImg.sh")
os.system("/Users/maxsydney/esp/qemu/build/xtensa-softmmu/qemu-system-xtensa -nographic -machine esp32 -drive file=unitTest.bin,if=mtd,format=raw")
