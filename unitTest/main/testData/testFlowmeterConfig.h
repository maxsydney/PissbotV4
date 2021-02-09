#ifndef TEST_FLOWMETERCONFIG_H
#define TEST_FLOWMETERCONFIG_H

// Unit test config for Flowmeter

static const char* flowmeterTestConfig = "\
{\
    \"validFlowmeterConfig\": {\
      \"GPIO\": 35,\
      \"kFactor\": 1\
    },\
    \"invalidFlowmeterConfig\": {\
      \"GPIO\": 34\
    }\
}";

#endif // TEST_FLOWMETERCONFIG_H