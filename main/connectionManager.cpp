#include "connectionManager.h"
#include "esp_log.h"

int ConnectionManager::_nConnections = 0;
Websock* ConnectionManager:: _activeWebsockets[ConnectionManager::MAX_CONNECTIONS] = {};

esp_err_t ConnectionManager::addConnection(Websock* ws)
{
    esp_err_t err = ESP_FAIL;

    if (_nConnections >= MAX_CONNECTIONS) {
        ESP_LOGW(tag, "Unable to store websocket (%p) - Table is full", ws);
        return ESP_FAIL;
    }

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (_activeWebsockets[i] == NULL) {
            _activeWebsockets[i] = ws;
            _nConnections += 1;
            err = ESP_OK;
            ESP_LOGI(tag, "Opening connection (%p)", ws);
            break;
        }
    }

    return err;
}

esp_err_t ConnectionManager::removeConnection(Websock* ws)
{
    esp_err_t err = ESP_FAIL;

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (_activeWebsockets[i] == ws) {
            _activeWebsockets[i] = NULL;
            err = ESP_OK;
            _nConnections -= 1;
            ESP_LOGI(tag, "Closing connection (%p)", ws);
            break;
        }
    }

    if (err != ESP_OK) {
        ESP_LOGW(tag, "Unable to delete websocket (%p) - Websocket not found in table", ws);
    }

    return err;
}

esp_err_t ConnectionManager::checkConnection(Websock* ws)
{
    esp_err_t err = ESP_FAIL;

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (_activeWebsockets[i] == ws) {
            err = ESP_OK;
            break;
        }
    }

    return err;
}

void ConnectionManager::printConnections(void)
{
    int count = 0;
    ESP_LOGI(tag, "--- %d Active websockets ---", _nConnections);
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if ( _activeWebsockets[i] != NULL) {
            ESP_LOGI(tag, "(%d) - %p", count++, _activeWebsockets[i]);
        }
    }
}