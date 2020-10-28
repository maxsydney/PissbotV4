#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include "PBCommon.h"
#include "esp_err.h"
#include "libesphttpd/cgiwebsocket.h"
#include <vector>

class ConnectionManager
{
    static constexpr const char* Name = "Connection Manager";

    public:
        ConnectionManager(void) = default;
        ConnectionManager(int maxConnections)
            : _maxConnections(maxConnections) {}

        PBRet addConnection(Websock* ws);
        PBRet removeConnection(Websock* ws);
        PBRet checkConnection(Websock* ws) const;
        void printConnections(void) const;
        int getNumConnections(void) const { return _nConnections; }
        PBRet getConnectionPtr(size_t i, Websock** conn);

        bool isConfigured(void) const { return _configured; }
        const std::vector<Websock*> getActiveWebsockets(void) const { return _activeWebsockets; }

    private:
        
        bool _configured = false;
        std::vector<Websock*> _activeWebsockets {};
        int _maxConnections = 0;
        int _nConnections = 0;;
};

#endif // CONNECTIONMANAGER_H