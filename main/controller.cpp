// #ifdef __cplusplus
// extern "C" {
// #endif

#include "controller.h"
#include "MessageDefs.h"        // Phase this out. Bad design

Controller::Controller(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const ControllerConfig& cfg)
    : Task(Controller::Name, priority, stackDepth, coreID)
{
    // Setup callback table
    _setupCBTable();

    // Initialize controller
    if (_initFromParams(cfg) == PBRet::SUCCESS) {
        ESP_LOGI(Controller::Name, "Controller configured!");
        _configured = true;
    } else {
        ESP_LOGW(Controller::Name, "Unable to configure controller");
    }
}

void Controller::taskMain(void)
{
    // Subscribe to messages
    std::set<MessageType> subscriptions = { 
        MessageType::General,
        MessageType::TemperatureData,
        MessageType::ControlTuning,
        MessageType::ControlCommand,
        MessageType::ControlSettings,
        MessageType::ControllerDataRequest
    };
    Subscriber sub(Controller::Name, _GPQueue, subscriptions);
    MessageServer::registerTask(sub);

    // Set update frequency
    TickType_t timestep =  _cfg.dt * 1000 / portTICK_PERIOD_MS;      // TODO: Define method for converting time
    portTickType xLastWakeTime = xTaskGetTickCount();

    while (true) {
        // Retrieve data from the queue
        _processQueue();

        // Check temperature data

        // Check status of connected components

        // Update control
        if (_doControl(0) != PBRet::SUCCESS) {
            ESP_LOGW(Controller::Name, "Controller update failed");
            // Send warning message to distiller controller
        }

        // Command pumps

        vTaskDelayUntil(&xLastWakeTime, timestep);
    }
}

PBRet Controller::_generalMessageCB(std::shared_ptr<MessageBase> msg)
{
    std::shared_ptr<GeneralMessage> genMsg = std::static_pointer_cast<GeneralMessage>(msg);
    ESP_LOGI(Controller::Name, "Received general message: %s", genMsg->getMessage().c_str());  

    return PBRet::SUCCESS;
}
PBRet Controller::_temperatureDataCB(std::shared_ptr<MessageBase> msg)
{
    // std::shared_ptr<TemperatureData> TData = std::static_pointer_cast<TemperatureData>(msg);

    // ESP_LOGI(Controller::Name, "Received temperature message:\nHead: %.3lf\nReflux condensor: %.3f\nProduct condensor: %.3lf\n"
    //                            "Radiator: %.3f\nBoiler: %.3lf\nUptime: %ld", TData->getHeadTemp(), TData->getRefluxCondensorTemp(),
    //                             TData->getProdCondensorTemp(), TData->getRadiatorTemp(), TData->getBoilerTemp(), (long int) TData->getTimeStamp());

    return PBRet::SUCCESS;
}

// TODO: This can be removed
PBRet Controller::_controlCommandCB(std::shared_ptr<MessageBase> msg)
{
    std::shared_ptr<ControlCommand> cmd = std::static_pointer_cast<ControlCommand>(msg);
    _outputState = ControlCommand(*cmd);

    if (_updateAuxOutputs(_outputState) != PBRet::SUCCESS) {
        ESP_LOGW(Controller::Name, "A command message was received but all of the auxilliary componenst did not update successfully");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet Controller::_controlSettingsCB(std::shared_ptr<MessageBase> msg)
{
    std::shared_ptr<ControlSettings> cmd = std::static_pointer_cast<ControlSettings>(msg);
    _cfg.ctrlSettings = ControlSettings(*cmd);

    ESP_LOGI(Controller::Name, "Controller settings were updated");

    if (_updateAuxOutputs(_outputState) != PBRet::SUCCESS) {
        ESP_LOGW(Controller::Name, "A command message was received but all of the auxilliary componenst did not update successfully");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet Controller::_controlTuningCB(std::shared_ptr<MessageBase> msg)
{
    std::shared_ptr<ControlTuning> cmd = std::static_pointer_cast<ControlTuning>(msg);
    _cfg.ctrlTuning = ControlTuning(*cmd);

    ESP_LOGI(Controller::Name, "Controller tuning was updated");

    return PBRet::SUCCESS;
}

PBRet Controller::_controlDataRequestCB(std::shared_ptr<MessageBase> msg)
{
    // Broadcast the requested data
    //

    ESP_LOGI(Controller::Name, "Got request for data");

    ControllerDataRequest request = *std::static_pointer_cast<ControllerDataRequest>(msg);

    switch (request.getType())
    {
        case (ControllerDataRequestType::Tuning):
        {
            return _broadcastControllerTuning();
        }
        case (ControllerDataRequestType::Settings):
        {
            return _broadcastControllerSettings();
        }
        case (ControllerDataRequestType::None):
        {
            ESP_LOGW(Controller::Name, "Cannot respond to request for data None");
            break;
        }
        default:
        {
            ESP_LOGW(Controller::Name, "Controller data request not supported");
            break;
        }
    }

    // If we have made it here, we haven't been able to respond to data request
    return PBRet::FAILURE;
}

PBRet Controller::_setupCBTable(void)
{
    _cbTable = std::map<MessageType, queueCallback> {
        {MessageType::General, std::bind(&Controller::_generalMessageCB, this, std::placeholders::_1)},
        {MessageType::TemperatureData, std::bind(&Controller::_temperatureDataCB, this, std::placeholders::_1)},
        {MessageType::ControlCommand, std::bind(&Controller::_controlCommandCB, this, std::placeholders::_1)},
        {MessageType::ControlSettings, std::bind(&Controller::_controlSettingsCB, this, std::placeholders::_1)},
        {MessageType::ControlTuning, std::bind(&Controller::_controlTuningCB, this, std::placeholders::_1)},
        {MessageType::ControllerDataRequest, std::bind(&Controller::_controlDataRequestCB, this, std::placeholders::_1)}
    };

    return PBRet::SUCCESS;
}

PBRet Controller::_broadcastControllerTuning(void) const
{
    // Send a temperature data message to the queue
    std::shared_ptr<ControlTuning> msg = std::make_shared<ControlTuning> (_cfg.ctrlTuning);

    ESP_LOGI(Controller::Name, "Broadcasting controller tuning");
    return MessageServer::broadcastMessage(msg);
}

PBRet Controller::_broadcastControllerSettings(void) const
{
    // Send a temperature data message to the queue
    std::shared_ptr<ControlSettings> msg = std::make_shared<ControlSettings> (_cfg.ctrlSettings);

    ESP_LOGI(Controller::Name, "Broadcasting controller settings");
    return MessageServer::broadcastMessage(msg);
}

PBRet Controller::_initIO(const ControllerConfig& cfg) const
{
    esp_err_t err = ESP_OK;

    // Initialize fan pin
    gpio_pad_select_gpio(cfg.fanPin);
    err |= gpio_set_direction(cfg.fanPin, GPIO_MODE_OUTPUT);
    err |= gpio_set_level(cfg.fanPin, 0);    
    
    // Initialize 2.4kW element control pin
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[cfg.element1Pin], PIN_FUNC_GPIO);        // Element 1 pin is set to JTAG by default. Reassign to GPIO
    err |= gpio_set_direction(cfg.element1Pin, GPIO_MODE_OUTPUT);
    err |= gpio_set_level(cfg.element1Pin, 0);  

    // Initialize 3kW element control pin
    gpio_pad_select_gpio(cfg.element2Pin);
    err |= gpio_set_direction(cfg.element2Pin, GPIO_MODE_OUTPUT);
    err |= gpio_set_level(cfg.element2Pin, 0); 

    if (err != ESP_OK) {
        ESP_LOGW(Controller::Name, "Failed to configure one or more IO");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet Controller::_initPumps(const PumpConfig& refluxPumpConfig, const PumpConfig& prodPumpConfig)
{
    // Initialize both pumps to active control idle

    _refluxPump = Pump(refluxPumpConfig);
    _prodPump = Pump(prodPumpConfig);

    if (_refluxPump.isConfigured() == false) {
        ESP_LOGW(Controller::Name, "Unable to configure reflux pump driver");
        return PBRet::FAILURE;
    }

    if (_prodPump.isConfigured() == false) {
        ESP_LOGW(Controller::Name, "Unable to configure product pump driver");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet Controller::_doControl(double headTemp)
{
    // Implements a basic PID controller with anti-integral windup
    // and filtering on derivative

    double err = headTemp - _cfg.ctrlTuning.getSetpoint();

    // Proportional term
    double proportional = _cfg.ctrlTuning.getPGain() * err;

    // Integral term (discretized via bilinear transform)
    _integral += 0.5 * _cfg.ctrlTuning.getIGain() * _cfg.dt * (err + _prevError);

    // Dynamic integral clamping
    double intLimMin = 0.0;
    double intLimMax = 0.0;

    if (proportional < Pump::PUMP_MAX_OUTPUT) {
        intLimMax = Pump::PUMP_MAX_OUTPUT - proportional;
    } else {
        intLimMax = 0.0;
    }

    // Anti integral windup
    if (proportional > Pump::PUMP_MIN_OUTPUT) {
        intLimMin = Pump::PUMP_MIN_OUTPUT - proportional;
    } else {
        intLimMin = 0.0;
    }

    if (_integral > intLimMax) {
        _integral = intLimMax;
    } else if (_integral < intLimMin) {
        _integral = intLimMin;
    }

    // Derivative term (discretized via backwards temperature differentiation)
    // TODO: Filtering on D term? Quite tricky due to low temp sensor sample rate
    const double alpha = 0.15;      // TODO: Improve hacky dterm filter
    _derivative = (1 - alpha) * _derivative + alpha * (_cfg.ctrlTuning.getDGain() * (headTemp - _prevTemp) / _cfg.dt);

    // Compute limited output
    double output = proportional + _integral + _derivative;

    _prevError = err;
    _prevTemp = headTemp;

    _handleProductPump(headTemp);

    if (_refluxPump.updatePumpActiveControl(output) != PBRet::SUCCESS) {
        ESP_LOGW(Controller::Name, "Controller updated successfully but failed to set pump speed");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

// TODO: Improve this implementation
PBRet Controller::_handleProductPump(double temp)
{
    if (temp > 60) {
        _prodPump.updatePumpActiveControl(Pump::FLUSH_SPEED);
    } else if (temp < 58) {
        _prodPump.updatePumpActiveControl(Pump::PUMP_MIN_OUTPUT);
    }

    return PBRet::SUCCESS;
}

PBRet Controller::_updateAuxOutputs(const ControlCommand& cmd)
{
    esp_err_t err = ESP_OK;

    err |= gpio_set_level(_cfg.fanPin, uint32_t(cmd.getFanState()));
    err |= gpio_set_level(_cfg.element1Pin, uint32_t(cmd.getLPElementState()));
    err |= gpio_set_level(_cfg.element2Pin, uint32_t(cmd.getHPElementState()));

    if (err != ESP_OK) {
        ESP_LOGW(Controller::Name, "One or more of the auxilliary states were not updated");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet Controller::checkInputs(const ControllerConfig& cfg)
{
    if (cfg.dt <= 0) {
        ESP_LOGE(Controller::Name, "Update time step %lf is invalid. Controller was not configured", cfg.dt);
        return PBRet::FAILURE;
    }

    // TODO: Check if these pins are valid output pins
    if ((cfg.fanPin <= GPIO_NUM_NC) || (cfg.fanPin > GPIO_NUM_MAX)) {
        ESP_LOGE(Controller::Name, "Fan GPIO %d is invalid. Controller was not configured", cfg.fanPin);
        return PBRet::FAILURE;
    }

    if ((cfg.element1Pin <= GPIO_NUM_NC) || (cfg.element1Pin > GPIO_NUM_MAX)) {
        ESP_LOGE(Controller::Name, "Element 1 GPIO %d is invalid. Controller was not configured", cfg.element1Pin);
        return PBRet::FAILURE;
    }
    
    if ((cfg.element2Pin <= GPIO_NUM_NC) || (cfg.element2Pin > GPIO_NUM_MAX)) {
        ESP_LOGE(Controller::Name, "Element 1 GPIO %d is invalid. Controller was not configured", cfg.element2Pin);
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet Controller::loadFromJSON(ControllerConfig& cfg, const cJSON* cfgRoot)
{
    // Load ControllerConfig struct from JSON

    if (cfgRoot == nullptr) {
        ESP_LOGW(Controller::Name, "cfg was null");
        return PBRet::FAILURE;
    }

    // Get controller dt
    cJSON* dtNode = cJSON_GetObjectItem(cfgRoot, "dt");
    if (cJSON_IsNumber(dtNode)) {
        cfg.dt = dtNode->valuedouble;
    } else {
        ESP_LOGI(Controller::Name, "Unable to read controller dt from JSON");
        return PBRet::FAILURE;
    }

    // Get fan GPIO
    cJSON* GPIOFanNode = cJSON_GetObjectItem(cfgRoot, "GPIO_fan");
    if (cJSON_IsNumber(GPIOFanNode)) {
        cfg.fanPin = static_cast<gpio_num_t>(GPIOFanNode->valueint);
    } else {
        ESP_LOGI(Controller::Name, "Unable to read fan GPIO from JSON");
        return PBRet::FAILURE;
    }

    // Get element 1 GPIO
    cJSON* GPIOElement1Node = cJSON_GetObjectItem(cfgRoot, "GPIO_element1");
    if (cJSON_IsNumber(GPIOElement1Node)) {
        cfg.element1Pin = static_cast<gpio_num_t>(GPIOElement1Node->valueint);
    } else {
        ESP_LOGI(Controller::Name, "Unable to read element 1 GPIO from JSON");
        return PBRet::FAILURE;
    }

    // Get element 2 GPIO
    cJSON* GPIOElement2Node = cJSON_GetObjectItem(cfgRoot, "GPIO_element2");
    if (cJSON_IsNumber(GPIOElement2Node)) {
        cfg.element2Pin = static_cast<gpio_num_t>(GPIOElement2Node->valueint);
    } else {
        ESP_LOGI(Controller::Name, "Unable to read element 2 GPIO from JSON");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet Controller::_initFromParams(const ControllerConfig& cfg)
{
    if (Controller::checkInputs(cfg) != PBRet::SUCCESS) {;
        ESP_LOGW(Controller::Name, "Unable to configure controller");
        return PBRet::FAILURE;
    }

    if (_initPumps(cfg.refluxPumpConfig, cfg.prodPumpConfig) != PBRet::SUCCESS) {
        ESP_LOGW(Controller::Name, "Unable to configure one or more pumps");
        return PBRet::FAILURE;
    }

    if (_initIO(cfg) != PBRet::SUCCESS) {
        ESP_LOGW(Controller::Name, "Unable to configure controller I/O");
        return PBRet::FAILURE;
    }

    // Set pumps to active control
    _refluxPump.setPumpMode(PumpMode::ActiveControl);
    _prodPump.setPumpMode(PumpMode::ActiveControl);
    _cfg = cfg;

    return PBRet::SUCCESS;
}

// #ifdef __cplusplus
// }
// #endif
