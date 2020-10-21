#include "SensorManager.h"
#include "MessageDefs.h"

SensorManager::SensorManager(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const SensorManagerConfig& cfg)
    : Task(SensorManager::Name, priority, stackDepth, coreID)
{
    // Setup callback table
    _setupCBTable();

    // Initialize SensorManager
    if (_initFromParams(cfg) == PBRet::SUCCESS) {
        ESP_LOGI(SensorManager::Name, "SensorManager configured!");
        _configured = true;
    } else {
        ESP_LOGW(SensorManager::Name, "Unable to configure SensorManager");
    }
}

void SensorManager::taskMain(void)
{
    // Subscribe to messages
    std::set<MessageType> subscriptions = { 
        MessageType::General
    };
    Subscriber sub(SensorManager::Name, _GPQueue, subscriptions);
    MessageServer::registerTask(sub);

    // Set update frequency
    const TickType_t timestep =  _cfg.dt * 1000 / portTICK_PERIOD_MS;      // TODO: Define method for converting time
    portTickType xLastWakeTime = xTaskGetTickCount();

    while (true) {
        // Retrieve data from the queue
        _processQueue();

        // Read temperature sensors
        TemperatureData Tdata {};
        if (_OWBus.readTempSensors(Tdata) != PBRet::SUCCESS) {
            ESP_LOGW(SensorManager::Name, "Unable to read temperature sensors");
            
            // Record fault
        }

        // Read flowmeters

        // Broadcast data
        _broadcastTemps(Tdata);

        vTaskDelayUntil(&xLastWakeTime, timestep);
    }
}

PBRet SensorManager::_generalMessageCB(std::shared_ptr<MessageBase> msg)
{
    std::shared_ptr<GeneralMessage> genMsg = std::static_pointer_cast<GeneralMessage>(msg);
    ESP_LOGI(SensorManager::Name, "Received general message: %s", genMsg->getMessage().c_str());  

    return PBRet::SUCCESS;
}

PBRet SensorManager::_setupCBTable(void)
{
    _cbTable = std::map<MessageType, queueCallback> {
        {MessageType::General, std::bind(&SensorManager::_generalMessageCB, this, std::placeholders::_1)},
    };

    return PBRet::SUCCESS;
}

PBRet SensorManager::_initFromParams(const SensorManagerConfig& cfg)
{
    esp_err_t err = ESP_OK;

    _cfg = cfg;

    // Initialize PBOneWire bus
    _OWBus = PBOneWire(cfg.oneWirePin);
    if (_OWBus.isConfigured() == false) {
        ESP_LOGE(SensorManager::Name, "OneWire bus is not configured");
        err = ESP_FAIL;
    }

    // Initialize flowrate sensors

    return err == ESP_OK ? PBRet::SUCCESS : PBRet::FAILURE;
}

PBRet SensorManager::_broadcastTemps(const TemperatureData& Tdata) const
{
    // Send a temperature data message to the queue
    std::shared_ptr<TemperatureData> msg = std::make_shared<TemperatureData> (Tdata);

    return MessageServer::broadcastMessage(msg);;
}

PBRet SensorManager::checkInputs(const SensorManagerConfig& cfg)
{
    if (cfg.dt <= 0) {
        ESP_LOGE(SensorManager::Name, "Update time step %lf is invalid. SensorManager was not configured", cfg.dt);
        return PBRet::FAILURE;
    }

    if ((cfg.oneWirePin <= GPIO_NUM_NC) || (cfg.oneWirePin > GPIO_NUM_MAX)) {
        ESP_LOGE(SensorManager::Name, "OneWire GPIO %d is invalid. SensorManager was not configured", cfg.oneWirePin);
        return PBRet::FAILURE;
    }

    if ((cfg.refluxFlowPin <= GPIO_NUM_NC) || (cfg.refluxFlowPin > GPIO_NUM_MAX)) {
        ESP_LOGE(SensorManager::Name, "Reflux flowmeter GPIO %d is invalid. SensorManager was not configured", cfg.refluxFlowPin);
        return PBRet::FAILURE;
    }

    if ((cfg.productFlowPin <= GPIO_NUM_NC) || (cfg.productFlowPin > GPIO_NUM_MAX)) {
        ESP_LOGE(SensorManager::Name, "Product flowmeter GPIO %d is invalid. SensorManager was not configured", cfg.productFlowPin);
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}