#ifndef TEST_ONEWIREBUSCONFIG_H
#define TEST_ONEWIREBUSCONFIG_H

// Unit test config for PBOneWire

static const char* OneWireBusConfig = "\
{\
    \"ValidOneWireConfig\": {\
        \"GPIO_onewire\": 15,\
        \"DS18B20Resolution\": 11\
    },\
    \"InvalidOneWireConfig\": {\
        \"GPIO_onewire\": 15\
    }\
}";

#endif // TEST_ONEWIREBUSCONFIG_H