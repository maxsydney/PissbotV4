#ifndef TEST_SENSORMANAGERCONFIG_H
#define TEST_SENSORMANAGERCONFIG_H

// Unit test config for SensorManager

static const char* sensorManagerConfig = "\
{\
  \"ValidSensorManagerConfig\": {\
    \"dt\": 0.2,\
    \"oneWireConfig\": {\
      \"GPIO_onewire\": 15,\
      \"DS18B20Resolution\": 11,\
      \"GPIO_refluxFlow\": 35,\
      \"GPIO_prodFlow\": 34\
    },\
    \"refluxFlowmeterConfig\": {\
      \"GPIO\": 35,\
      \"kFactor\": 1\
    },\
    \"productFlowmeterConfig\": {\
      \"GPIO\": 34,\
      \"kFactor\": 1\
    }\
  },\
  \"InvalidSensorManagerConfig\": {\
    \"dt\": 0.2,\
    \"oneWireConfig\": {\
      \"GPIO_onewire\": 15,\
      \"DS18B20Resolution\": 11,\
      \"GPIO_prodFlow\": 34\
    }\
  }\
}";

#endif // TEST_SENSORMANAGERCONFIG_H