idf_component_register(SRCS ./LCD.c ./gpio.c ./input.c ./messages.c ./networking.c ./ota.c ./sensors.c ./webServer.c ./controlLoop.cpp ./controller.cpp ./main.cpp ./pump.cpp)
                       INCLUDE_DIRS include                       # Edit following two lines to set component requirements (see docs)
                       REQUIRES )
                       PRIV_REQUIRES )

