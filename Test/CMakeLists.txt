cmake_minimum_required(VERSION 3.13)
project (distillerTest)

set(CMAKE_CXX_FLAGS "-std=c++11")

include_directories(../main)
include_directories(
    $ENV{IDF_PATH}/components/esp_rom/include
    $ENV{IDF_PATH}/components/driver/include
    $ENV{IDF_PATH}/components/esp_common/include
    $ENV{IDF_PATH}/components/soc/esp32/include
    $ENV{IDF_PATH}/components/esp32/include
    $ENV{IDF_PATH}/components/fatfs/test_fatfs_host/sdkconfig
    $ENV{IDF_PATH}/components/freertos/include
    $ENV{IDF_PATH}/components/xtensa/include
    $ENV{IDF_PATH}/components/xtensa/esp32/include
    $ENV{IDF_PATH}/components/soc/include
)

add_executable(hello willItCompile)