#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "CppTask.h"
#include "PBCommon.h"
#include "OneWireBus.h"
#include "Controller.h"
#include "ConnectionManager.h"
#include <libesphttpd/esp.h>
#include "libesphttpd/httpd.h"
#include "libesphttpd/httpd-freertos.h"
#include "Generated/WebserverMessaging.h"
#include "Generated/MessageBase.h"

static constexpr uint32_t socketLogLength = 128;
using PBSocketLogMessage = SocketLogMessage<socketLogLength>;

class WebserverConfig
{
    public:
        int maxConnections = 0;
        double maxBroadcastFreq = 0.0;
};

class Webserver : public Task
{
    static constexpr const char* Name = "Webserver";
    static constexpr int LISTEN_PORT = 80;

    public:
        Webserver(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const WebserverConfig& cfg);

        // Websocket methods
        static void openConnection(Websock *ws);
        static void closeConnection(Websock *ws);
        static void processWebsocketMessage(Websock *ws, char *data, int len, int flags);

        // Utility methods
        static PBRet checkInputs(const WebserverConfig& cfg);
        static PBRet loadFromJSON(WebserverConfig& cfg, const cJSON* cfgRoot);
        static PBRet socketLog(const std::string& logMsg);
        bool isConfigured(void) const { return _configured; }

    private:

        // Initialisation
        PBRet _initFromParams(const WebserverConfig& cfg);
        PBRet _startupWebserver(const WebserverConfig& cfg);
        PBRet _setupCBTable(void) override; 

        // Queue callbacks
        PBRet _broadcastDataCB(std::shared_ptr<PBMessageWrapper> msg);

        // Utility methods
        static PBRet _requestControllerTuning(void);
        static PBRet _requestControllerSettings(void);
        static PBRet _requestControllerPeripheralState(void);
        static PBRet _processAssignSensorMessage(cJSON* root);

        // Websocket methods
        PBRet _sendToAll(const std::string& msg);
        PBRet _sendToAll(const PBMessageWrapper& wrapper);

        // FreeRTOS hook method
        void taskMain(void) override;

        HttpdFreertosInstance _httpdFreertosInstance {};
        RtosConnType* connectionMemory = nullptr;
        WebserverConfig _cfg {};
        bool _configured = false;
};

#endif // WEBSERVER_H
