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

        static PBRet addConnection(Websock* ws);
        static PBRet removeConnection(Websock* ws);
        static PBRet checkConnection(Websock* ws);
        static void printConnections(void);
        static int getNumConnections(void) { return _nConnections; }
        static PBRet getConnectionPtr(size_t i, Websock** conn);

        static const std::vector<Websock*> getActiveWebsockets(void) { return _activeWebsockets; }

    private:
        
        static std::vector<Websock*> _activeWebsockets;
        static constexpr int _maxConnections = 12;
        static int _nConnections;
};

#endif // CONNECTIONMANAGER_H