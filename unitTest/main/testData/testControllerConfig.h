#ifndef TEST_CONTROLLERCONFIG_H
#define TEST_CONTROLLERCONFIG_H

// Unit test config for PBOneWire

static const char* ctrlConfig = "\
{\
    \"ControllerConfigValid\": {\
        \"dt\": 0.2,\
        \"GPIO_fan\": 21,\
        \"GPIO_element1\": 13,\
        \"GPIO_element2\": 32,\
        \"RefluxPump\": {\
            \"GPIO\": 23,\
            \"PWMChannel\": 0,\
            \"timerChannel\": 0\
        },\
        \"ProductPump\": {\
            \"GPIO\": 22,\
            \"PWMChannel\": 1,\
            \"timerChannel\": 1\
        }\
    },\
    \"ControllerConfigInvalid\": {\
        \"dt\": 0.2,\
        \"GPIO_fan\": 21,\
        \"GPIO_element1\": 13,\
        \"RefluxPump\": {\
            \"GPIO\": 23,\
            \"PWMChannel\": 0,\
            \"timerChannel\": 0\
        },\
        \"ProductPump\": {\
            \"GPIO\": 22,\
            \"PWMChannel\": 1,\
            \"timerChannel\": 1\
        }\
    }\
}";

#endif // TEST_CONTROLLERCONFIG_H