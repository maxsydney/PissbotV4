set(COMPONENT_SRCDIRS
    "."
    "./test"
    "../../main"
    "../../components"
)

set(COMPONENT_ADD_INCLUDEDIRS
    "../../components/esp32-ds18b20/include"
    "../../components/esp32-owb/include"
    "../../components/libesphttpd/include"
    "../../"
    "./test"
    "./testData"
    "/Users/maxsydney/esp/PissbotV4/components/espfs/include"
)

register_component()

add_compile_definitions(CONFIG_ESPHTTPD_USE_ESPFS)

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++17)
