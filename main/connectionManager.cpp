#include "connectionManager.h"
#include "esp_log.h"
#include <algorithm>

PBRet ConnectionManager::addConnection(Websock* ws)
{
    if (_nConnections >= _maxConnections) {
        ESP_LOGW(ConnectionManager::Name, "Unable to store websocket (%p) - Table is full", ws);
        return PBRet::FAILURE;
    }

    _activeWebsockets.push_back(ws);
    ESP_LOGI(ConnectionManager::Name, "Opening connection (%p)", ws);
    return PBRet::SUCCESS;
}

PBRet ConnectionManager::removeConnection(Websock* ws)
{
    ESP_LOGI(ConnectionManager::Name, "Closing connection (%p)", ws);
    _activeWebsockets.erase(std::remove(_activeWebsockets.begin(), _activeWebsockets.end(), ws), _activeWebsockets.end());

    return PBRet::SUCCESS;
}

PBRet ConnectionManager::checkConnection(Websock* ws) const
{
    if (std::find(_activeWebsockets.begin(), _activeWebsockets.end(), ws) != _activeWebsockets.end()) {
        return PBRet::SUCCESS;
    }

    return PBRet::FAILURE;
}

void ConnectionManager::printConnections(void) const
{
    int count = 0;
    ESP_LOGI(ConnectionManager::Name, "--- %d Active websockets ---", _nConnections);
    for (int i = 0; i < _maxConnections; i++) {
        if ( _activeWebsockets[i] != NULL) {
            ESP_LOGI(ConnectionManager::Name, "(%d) - %p", count++, _activeWebsockets[i]);
        }
    }
}