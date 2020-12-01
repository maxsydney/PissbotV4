idf_component_register(
    SRCS 
        ./main.cpp
        ./connectionManager.cpp
        ./controller.cpp
        ./CppTask.cpp
        ./DistillerManager.cpp
        ./Filesystem.cpp
        ./messageServer.cpp
        ./OneWireBus.cpp
        ./PBds18b20.cpp
        ./pump.cpp
        ./SensorManager.cpp
        ./thermo.cpp
        ./webServer.cpp
        ./WifiManager.cpp

    INCLUDE_DIRS ""
    REQUIRES
)

target_compile_options(${COMPONENT_LIB} PRIVATE -std=c++14)

