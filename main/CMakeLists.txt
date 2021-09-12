cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED ENV{PBPATH})
    message(FATAL_ERROR "PBPATH environment variable was not set")
endif()

set(DEPENDENCIES
    esp32-ds18b20
    esp32-owb
    spiffs
    nvs_flash
    libesphttpd
    espfs
)

set(SRC_DIRS
    $ENV{PBPATH}/main
    $ENV{PBPATH}/lib/EmbeddedProto/src
    $ENV{PBPATH}/lib/PBProtoBuf/IO
)

set(INCLUDE_DIRS
    $ENV{PBPATH}/lib/EmbeddedProto/src
    $ENV{PBPATH}/lib/PBProtoBuf
    $ENV{PBPATH}/lib/PBProtoBuf/Generated
)

idf_component_register(
    SRC_DIRS ${SRC_DIRS}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES ${DEPENDENCIES}
)

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++14)
spiffs_create_partition_image(config ../Config FLASH_IN_PROJECT)
