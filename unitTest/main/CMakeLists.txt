set(COMPONENT_SRCDIRS
    "."
    "./test"
    "../../main"
    "../../components"
    "../../lib/EmbeddedProto/src"
    "../../lib/PBProtoBuf/IO/"
)

set(COMPONENT_ADD_INCLUDEDIRS
    "../../components/esp32-ds18b20/include"
    "../../components/esp32-owb/include"
    "../../components/libesphttpd/include"
    "../../"
    "./test"
    "./testData"
    "/Users/maxsydney/esp/PissbotV4/components/espfs/include"
    "../../lib/EmbeddedProto/src" 
    "../../lib/PBProtoBuf/"
    "../../lib/PBProtoBuf/Generated"
)

set(DEPENDENCIES
    # esp32-ds18b20
    # esp32-owb
    # spiffs
    # nvs_flash
    # libesphttpd
    # espfs
)

idf_component_register(
    SRC_DIRS ${COMPONENT_SRCDIRS}
    INCLUDE_DIRS ${COMPONENT_ADD_INCLUDEDIRS}
    REQUIRES ${DEPENDENCIES}
)

add_compile_definitions(CONFIG_ESPHTTPD_USE_ESPFS)
target_compile_options(${COMPONENT_LIB} PRIVATE -std=c++17)
