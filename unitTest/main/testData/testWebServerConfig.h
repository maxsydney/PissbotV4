#ifndef TEST_WEBSERVERCONFIG_H
#define TEST_WEBSERVERCONFIG_H

// Unit test config for Webserver

static const char* webserverConfig = "\
{\
  \"ValidWebserverConfig\": {\
        \"maxConnections\": 12,\
        \"maxBroadcastFreq\": 10\
    },\
    \"InvalidWebserverConfig\": {\
        \"maxConnections\": 12\
    }\
}";

#endif // TEST_WEBSERVERCONFIG_H