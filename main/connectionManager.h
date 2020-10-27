#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include "esp_err.h"
#include "libesphttpd/cgiwebsocket.h"

class ConnectionManager
{
    static constexpr const char* Name = "Connection Manager";

    public:
        static esp_err_t addConnection(Websock* ws);
        static esp_err_t removeConnection(Websock* ws);
        static esp_err_t checkConnection(Websock* ws);
        static void printConnections(void);
        static int getNumConnections(void) { return _nConnections; }
        static esp_err_t getConnectionPtr(size_t i, Websock** conn);

        static constexpr int MAX_CONNECTIONS = 12;

    private:
        
        static Websock* _activeWebsockets[MAX_CONNECTIONS];
        static int _nConnections;
};

#endif // CONNECTIONMANAGER_H