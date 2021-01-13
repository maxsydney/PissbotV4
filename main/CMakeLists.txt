set(DEPENDENCIES
    esp32-ds18b20
    esp32-owb
    spiffs
    nvs_flash
    libesphttpd
    espfs
)

idf_component_register(
    SRC_DIRS "."
    INCLUDE_DIRS ""
    REQUIRES ${DEPENDENCIES}
)

target_compile_options(${COMPONENT_LIB} PRIVATE -std=c++17)

