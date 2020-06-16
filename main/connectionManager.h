#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include "esp_err.h"
#include "libesphttpd/cgiwebsocket.h"

class ConnectionManager
{
    public:
        static esp_err_t addConnection(Websock* ws);
        static esp_err_t removeConnection(Websock* ws);
        static esp_err_t checkConnection(Websock* ws);
        static void printConnections(void);
        static int getNumConnections(void) { return _nConnections; }

        static constexpr int MAX_CONNECTIONS = 12;

    private:
        static constexpr char *tag = "Connection Manager";
        static Websock* _activeWebsockets[MAX_CONNECTIONS];
        static int _nConnections;
};

#endif // CONNECTIONMANAGER_H