set(TEST_SRCS
    ./testMain.c
    ./test/test_Controller.cpp
    ./test/test_Messages.cpp
    ./test/test_pump.cpp
    ./test/test_Thermo.cpp
    ${COMPONENT_DIR}/../../main/controller.cpp
    ${COMPONENT_DIR}/../../main/messages.cpp
    ${COMPONENT_DIR}/../../main/pump.cpp
    ${COMPONENT_DIR}/../../main/thermo.cpp
)

set(TEST_INCLUDE_DIRS
    ${COMPONENT_DIR}/../../main
)

idf_component_register(
    SRCS ${TEST_SRCS}
    INCLUDE_DIRS ${TEST_INCLUDE_DIRS}
)