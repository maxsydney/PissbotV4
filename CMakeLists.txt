cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(PissbotFirmware)

include(components/espfs/cmake/include.cmake)
target_add_espfs(PissbotFirmware.elf espfs html)
