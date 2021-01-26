#ifndef TEST_PUMPCONFIG_H
#define TEST_PUMPCONFIG_H

// Unit test config for Pump

static const char* pumpConfig = "\
{\
    \"ValidPump\": {\
        \"GPIO\": 22,\
        \"PWMChannel\": 1,\
        \"timerChannel\": 1\
    },\
        \"InvalidPump\": {\
        \"GPIO\": 22,\
        \"timerChannel\": 1\
    }\
}";

#endif // TEST_PUMPCONFIG_H