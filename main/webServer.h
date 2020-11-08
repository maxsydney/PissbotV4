#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "CppTask.h"
#include "PBCommon.h"
#include "OneWireBus.h"
#include "controller.h"
#include "ConnectionManager.h"
#include <libesphttpd/esp.h>
#include "libesphttpd/httpd.h"
#include "libesphttpd/httpd-freertos.h"

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
    static constexpr const char* CtrlTuningStr = "ControlTuning";
    static constexpr const char* CtrlSettingsStr = "ControlSettings";

    public:
        Webserver(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const WebserverConfig& cfg);

        // Websocket methods
        static void openConnection(Websock *ws);
        static void closeConnection(Websock *ws);
        static void processWebsocketMessage(Websock *ws, char *data, int len, int flags);

        // Utility methods
        static PBRet checkInputs(const WebserverConfig& cfg);
        bool isConfigured(void) const { return _configured; }

    private:

        // Initialisation
        PBRet _initFromParams(const WebserverConfig& cfg);
        PBRet _startupWebserver(const WebserverConfig& cfg);
        PBRet _setupCBTable(void) override; 

        // Queue callbacks
        PBRet _temperatureDataCB(std::shared_ptr<MessageBase> msg);
        PBRet _controlSettingsCB(std::shared_ptr<MessageBase> msg);
        PBRet _controlTuningCB(std::shared_ptr<MessageBase> msg);

        // Message serialization
        static PBRet serializeTemperatureDataMsg(const TemperatureData& TData, std::string& outStr);
        static PBRet serializeControlTuningMsg(const ControlTuning& ctrlTuning, std::string& outStr);
        static PBRet serializeControlSettingsMessage(const ControlSettings& ctrlSettings, std::string& outStr);

        // Message parsing
        static PBRet _parseControlTuningMessage(cJSON* msgRoot);
        static PBRet _parseControlSettingsMessage(cJSON* msgRoot);

        // Utility methods
        static PBRet _requestControllerTuning(void);
        static PBRet _requestControllerSettings(void);

        // Websocket methods
        PBRet _sendToAll(const std::string& msg);

        // Queued messages to broadcast
        // TODO: Replace these with actual data objects
        std::string _temperatureMessage {};
        std::string _ctrlTuningMessage {};
        std::string _ctrlSettingsMessage {};

        // FreeRTOS hook method
        void taskMain(void) override;

        HttpdFreertosInstance _httpdFreertosInstance {};
        RtosConnType* connectionMemory = nullptr;
        WebserverConfig _cfg {};
        bool _configured = false;
};

#endif // WEBSERVER_H