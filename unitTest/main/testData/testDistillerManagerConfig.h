#ifndef TEST_DISTILLERMANAGERCONFIG_H
#define TEST_DISTILLERMANAGERCONFIG_H

// Unit test config for DistillerManager

static const char* distillerManagerConfig = "\
{\
    \"DistillerConfigValid\": {\
        \"ControllerConfig\": {\
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
            },\
            \"slowPMWLPElement\": {\
                \"PWMFreq\": 5\
            },\
            \"slowPMWHPElement\": {\
                \"PWMFreq\": 5\
            }\
        },\
        \"SensorManagerConfig\": {\
            \"dt\": 0.2,\
            \"oneWireConfig\": {\
                \"GPIO_onewire\": 15,\
                \"DS18B20Resolution\": 11\
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
        \"WebserverConfig\": {\
            \"maxConnections\": 12,\
            \"maxBroadcastFreq\": 10\
        }\
    },\
    \"DistillerConfigInvalid\": {\
        \"ControllerConfig\": {\
            \"dt\": 0.2,\
            \"GPIO_fan\": 21,\
            \"GPIO_element1\": 13,\
            \"GPIO_element2\": 32\
        }\
    }\
}";

#endif // TEST_DISTILLERMANAGERCONFIG_H