#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "PBCommon.h"
#include "ConnectionManager.h"
#include <libesphttpd/esp.h>
#include "libesphttpd/httpd.h"
#include "libesphttpd/httpd-freertos.h"

class WebserverConfig
{
    public:
        int maxConnections = 0;
};

class Webserver
{
    static constexpr const char* Name = "Webserver";
    static constexpr int LISTEN_PORT = 80;

    public:
        Webserver(void) = default;
        Webserver(const WebserverConfig& cfg);

        static PBRet checkInputs(const WebserverConfig& cfg);
        bool isConfigured(void) const { return _configured; }

    private:

        // Initialisation
        PBRet _initFromParams(const WebserverConfig& cfg);
        PBRet _startupWebserver(const WebserverConfig& cfg);

        ConnectionManager _connManager {};

        HttpdFreertosInstance _httpdFreertosInstance {};
        RtosConnType* connectionMemory = nullptr;

        WebserverConfig _cfg {};
        bool _configured = false;
};

#endif // WEBSERVER_H