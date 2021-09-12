cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED ENV{PBPATH})
    message(FATAL_ERROR "PBPATH environment variable was not set")
endif()

set(COMPONENT_SRCDIRS
    $ENV{PBPATH}/unitTest/main
    $ENV{PBPATH}/unitTest/main/test
    $ENV{PBPATH}/main
    $ENV{PBPATH}/components
    $ENV{PBPATH}/lib/PBProtoBuf/IO
    $ENV{PBPATH}/lib/EmbeddedProto/src
)

set(COMPONENT_ADD_INCLUDEDIRS
    $ENV{PBPATH}
    $ENV{PBPATH}/components/esp32-ds18b20/include
    $ENV{PBPATH}/components/esp32-owb/include
    $ENV{PBPATH}/components/libesphttpd/include
    $ENV{PBPATH}/unitTest/main/test
    $ENV{PBPATH}/unitTest/main/testData
    $ENV{PBPATH}/components/espfs/include
    $ENV{PBPATH}/lib/PBProtoBuf
    $ENV{PBPATH}/lib/EmbeddedProto/src
    $ENV{PBPATH}/lib/PBProtoBuf/Generated
)

register_component()
add_compile_definitions(CONFIG_ESPHTTPD_USE_ESPFS)
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++17)
