#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "PBCommon.h"
#include <libesphttpd/esp.h>
#include "libesphttpd/httpd.h"
#include "libesphttpd/httpd-freertos.h"

class WebserverConfig
{
    int maxConnections = 0;
};

class Webserver
{
    static constexpr const char* Name = "Webserver";

    public:
        Webserver(void) = default;
        Webserver(const WebserverConfig& cfg);

        static PBRet checkInputs(const WebserverConfig& cfg);
        bool isConfigured(void) const { return _configured; }

    private:

        // Initialisation
        PBRet _initFromParams(const WebserverConfig& cfg);
        PBRet _startupWebserver(void);

        HttpdFreertosInstance _httpdFreertosInstance {};

        WebserverConfig _cfg {};
        bool _configured = false;
};

#endif // WEBSERVER_H