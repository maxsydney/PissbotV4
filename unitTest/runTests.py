import os

print("Running unit tests")
os.system("idf.py build")
os.system("./makeUnitTestImg.sh")
os.system("/Users/maxsydney/esp/qemu/build/xtensa-softmmu/qemu-system-xtensa -nographic --no-reboot -machine esp32 -drive file=outputs/unitTest.bin,if=mtd,format=raw -serial file:outputs/testResults.txt")
