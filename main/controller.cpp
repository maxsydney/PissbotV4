#include "controller.h"
#include "Filesystem.h"
#include <fstream>
#include <sstream>
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

        // Check temperature data is valid
        if (_checkTemperatures(_currentTemp) != PBRet::SUCCESS) {
            ESP_LOGW(Controller::Name, "Temperatures were invalid");
            // TODO: Implement emergency stop for cases like this
        }

        // Update control
        if (_doControl(_currentTemp.headTemp) != PBRet::SUCCESS) {
            ESP_LOGW(Controller::Name, "Control law update failed");
            // Send warning message to distiller controller
        }

        // Update peripheral outputs
        if (_updatePeripheralState(_peripheralState) != PBRet::SUCCESS) {
            ESP_LOGW(Controller::Name, "Peripheral update failed");
        }

        // Command pumps
        if (_updatePumps() != PBRet::SUCCESS) {
            ESP_LOGW(Controller::Name, "Pump speeds were not updates");
        }

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
    std::shared_ptr<TemperatureData> TData = std::static_pointer_cast<TemperatureData>(msg);
    _currentTemp = TemperatureData(*TData);

    return PBRet::SUCCESS;
}

PBRet Controller::_controlCommandCB(std::shared_ptr<MessageBase> msg)
{
    std::shared_ptr<ControlCommand> cmd = std::static_pointer_cast<ControlCommand>(msg);
    _peripheralState = ControlCommand(*cmd);

    ESP_LOGI(Controller::Name, "Controller peripheral states were updated");

    if (_updatePeripheralState(_peripheralState) != PBRet::SUCCESS) {
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

    // TODO: Update pump modes here instead of aux outputs
    // if (_updatePeripheralState(_peripheralState) != PBRet::SUCCESS) {
    //     ESP_LOGW(Controller::Name, "A command message was received but all of the auxilliary components did not update successfully");
    //     return PBRet::FAILURE;
    // }

    return PBRet::SUCCESS;
}

PBRet Controller::_controlTuningCB(std::shared_ptr<MessageBase> msg)
{
    std::shared_ptr<ControlTuning> cmd = std::static_pointer_cast<ControlTuning>(msg);
    _ctrlTuning = ControlTuning(*cmd);

    // Write controller tuning to file
    if (saveTuningToFile() != PBRet::SUCCESS) {
        ESP_LOGW(Controller::Name, "Unable to save controller tuning to file");
    }

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
        case (ControllerDataRequestType::PeripheralState):
        {
            return _broadcastControllerPeripheralState();
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
    std::shared_ptr<ControlTuning> msg = std::make_shared<ControlTuning> (_ctrlTuning);

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

PBRet Controller::_broadcastControllerPeripheralState(void) const
{
    // Send a Control command message to the queue
    // TODO: Does this loopback?

    std::shared_ptr<ControlCommand> msg = std::make_shared<ControlCommand> (_peripheralState);

    ESP_LOGI(Controller::Name, "Broadcasting peripheral state");
    return MessageServer::broadcastMessage(msg);
}

PBRet Controller::_initIO(const ControllerConfig& cfg) const
{
    esp_err_t err = ESP_OK;

    // Initialize fan pin
    const gpio_config_t fanGPIOConf {
        .pin_bit_mask = (1ULL << cfg.fanPin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    if (gpio_config(&fanGPIOConf) != ESP_OK) {
        ESP_LOGE(Controller::Name, "Failed to configure fan GPIO");
        err += ESP_FAIL;
    }  
    
    // Initialize LP element pin
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[cfg.element1Pin], PIN_FUNC_GPIO);        // LP element pin is set to JTAG by default. Reassign to GPIO
    const gpio_config_t LPElementGPIOConf {
        .pin_bit_mask = (1ULL << cfg.element1Pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    if (gpio_config(&LPElementGPIOConf) != ESP_OK) {
        ESP_LOGE(Controller::Name, "Failed to configure low power element GPIO");
        err += ESP_FAIL;
    }

    // Initialize 3kW element control pin
    const gpio_config_t HPElementGPIOConf {
        .pin_bit_mask = (1ULL << cfg.element2Pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    if (gpio_config(&HPElementGPIOConf) != ESP_OK) {
        ESP_LOGE(Controller::Name, "Failed to configure high power element GPIO");
        err += ESP_FAIL;
    }

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
    _productPump = Pump(prodPumpConfig);

    if (_refluxPump.isConfigured() == false) {
        ESP_LOGW(Controller::Name, "Unable to configure reflux pump driver");
        return PBRet::FAILURE;
    }

    if (_productPump.isConfigured() == false) {
        ESP_LOGW(Controller::Name, "Unable to configure product pump driver");
        return PBRet::FAILURE;
    }

    // Set pumps to off until Controller is configured
    _refluxPumpMode = PumpMode::Off;
    _productPumpMode = PumpMode::Off;

    return PBRet::SUCCESS;
}

PBRet Controller::_doControl(double temp)
{
    // Implements a basic PID controller with anti-integral windup
    // and filtering on derivative

    double err = temp - _ctrlTuning.getSetpoint();

    // Proportional term
    double proportional = _ctrlTuning.getPGain() * err;

    // Integral term (discretized via bilinear transform)
    _integral += 0.5 * _ctrlTuning.getIGain() * _cfg.dt * (err + _prevError);

    // Dynamic integral clamping
    double intLimMin = 0.0;
    double intLimMax = 0.0;

    if (proportional < Pump::PUMP_MAX_SPEED) {
        intLimMax = Pump::PUMP_MAX_SPEED - proportional;
    } else {
        intLimMax = 0.0;
    }

    // Anti integral windup
    if (proportional > Pump::PUMP_IDLE_SPEED) {
        intLimMin = Pump::PUMP_IDLE_SPEED - proportional;
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
    // TODO: Create filter object
    const double alpha = 0.15;      // TODO: Improve hacky dterm filter
    _derivative = (1 - alpha) * _derivative + alpha * (_ctrlTuning.getDGain() * (temp - _prevTemp) / _cfg.dt);

    // Compute limited output
    _currentOutput = proportional + _integral + _derivative;
    _prevError = err;
    _prevTemp = temp;

    return PBRet::SUCCESS;
}

PBRet Controller::_updatePumps(void)
{
    // Update reflux pump
    if (_updateRefluxPump() != PBRet::SUCCESS) {
        ESP_LOGW(Controller::Name, "Pump update failed");
        return PBRet::FAILURE;
    }

    // Update product pump
    if (_updateProductPump(_currentTemp.headTemp) != PBRet::SUCCESS) {
        ESP_LOGW(Controller::Name, "Pump update failed");
        return PBRet::FAILURE;
    }
    
    return PBRet::SUCCESS;
}

PBRet Controller::_updateRefluxPump(void)
{
    // Update the reflux pump speed
    if (_refluxPumpMode == PumpMode::ActiveControl) {
        if (_refluxPump.updatePumpSpeed(_currentOutput) != PBRet::SUCCESS) {
            ESP_LOGW(Controller::Name, "Failed to update reflux pump speed in active mode");
            return PBRet::FAILURE;
        }
    } else if (_refluxPumpMode == PumpMode::ManualControl) {
        if (_refluxPump.updatePumpSpeed(_manualPumpSpeeds.refluxPumpSpeed) != PBRet::SUCCESS) {
            ESP_LOGW(Controller::Name, "Failed to update reflux pump speed in manual mode");
            return PBRet::FAILURE;
        }
    } else {
        // Pump mode is OFF or unknown. Stop the pumps
        if (_refluxPump.updatePumpSpeed(Pump::PUMP_OFF) != PBRet::SUCCESS) {
            ESP_LOGW(Controller::Name, "Failed to stop reflux pump");
            return PBRet::FAILURE;
        }
    }

    return PBRet::SUCCESS;
}

PBRet Controller::_updateProductPump(double temp)
{
    if (_productPumpMode == PumpMode::ActiveControl) {
        // Control pump speed based on head temperature
        if (temp >= Controller::HYSTERESIS_BOUND_UPPER) {
            if (_productPump.updatePumpSpeed(Pump::FLUSH_SPEED) != PBRet::SUCCESS) {
                ESP_LOGW(Controller::Name, "Failed to update reflux pump speed in active mode");
                return PBRet::FAILURE;
            }
        } else if (temp < Controller::HYSTERESIS_BOUND_LOWER) {
            if (_productPump.updatePumpSpeed(Pump::PUMP_IDLE_SPEED) != PBRet::SUCCESS) {
                ESP_LOGW(Controller::Name, "Failed to update reflux pump speed in active mode");
                return PBRet::FAILURE;
            }
        }
    } else if (_productPumpMode == PumpMode::ManualControl) {
        if (_productPump.updatePumpSpeed(_manualPumpSpeeds.productPumpSpeed) != PBRet::SUCCESS) {
            ESP_LOGW(Controller::Name, "Failed to update reflux pump speed in manual mode");
            return PBRet::FAILURE;
        }
    } else {
        // Pump mode is OFF or unknown. Stop the pumps
        if (_productPump.updatePumpSpeed(Pump::PUMP_OFF) != PBRet::SUCCESS) {
            ESP_LOGE(Controller::Name, "Failed to stop product pump");
            return PBRet::FAILURE;
        }
    }

    return PBRet::SUCCESS;
}

PBRet Controller::_checkTemperatures(const TemperatureData& currTemp) const
{
    // Verify that the input temperatures are valid

    // Check head temperature is within bounds
    if ((currTemp.headTemp > MAX_CONTROL_TEMP) || (currTemp.headTemp < MIN_CONTROL_TEMP))
    {
        ESP_LOGW(Controller::Name, "Head temp (%.2f) was outside controllable bounds [%.2f, %.2f]", currTemp.headTemp, MIN_CONTROL_TEMP, MAX_CONTROL_TEMP);
        return PBRet::FAILURE;
    }

    // Check that current temperature hasn't expired
    if ((esp_timer_get_time() - currTemp.getTimeStamp()) > TEMP_MESSAGE_TIMEOUT) {
        ESP_LOGW(Controller::Name, "Temperature message was stale");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet Controller::_updatePeripheralState(const ControlCommand& cmd)
{
    esp_err_t err = ESP_OK;

    err |= gpio_set_level(_cfg.fanPin, uint32_t(cmd.fanState));
    err |= gpio_set_level(_cfg.element1Pin, uint32_t(cmd.LPElementState));
    err |= gpio_set_level(_cfg.element2Pin, uint32_t(cmd.HPElementState));

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

    if (Pump::checkInputs(cfg.refluxPumpConfig) != PBRet::SUCCESS) {
        ESP_LOGE(Controller::Name, "Reflux pump config was invalic");
        return PBRet::FAILURE;
    }

    if (Pump::checkInputs(cfg.prodPumpConfig) != PBRet::SUCCESS) {
        ESP_LOGE(Controller::Name, "Product pump config was invalic");
        return PBRet::FAILURE;
    }

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

    // Load reflux pump configuration
    cJSON* refluxPumpNode = cJSON_GetObjectItem(cfgRoot, "RefluxPump");
    if (Pump::loadFromJSON(cfg.refluxPumpConfig, refluxPumpNode) != PBRet::SUCCESS) {
        return PBRet::FAILURE;
    }

    // Load product pump configuration
    cJSON* productPumpNode = cJSON_GetObjectItem(cfgRoot, "ProductPump");
    if (Pump::loadFromJSON(cfg.prodPumpConfig, productPumpNode) != PBRet::SUCCESS) {
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

    // Load controller tuning from file (if it exists)
    if (loadTuningFromFile() != PBRet::SUCCESS) {
        ESP_LOGW(Controller::Name, "Unable to load controller tuning from file");
    }

    // Set pumps to active control
    _refluxPumpMode = PumpMode::ActiveControl;
    _productPumpMode = PumpMode::ActiveControl;
    _cfg = cfg;

    return PBRet::SUCCESS;
}

PBRet ControlTuning::serialize(std::string& JSONstr) const
{
    // Write the ControlTuning object to JSON

    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        ESP_LOGW(ControlTuning::Name, "Unable to create root JSON object");
        return PBRet::FAILURE;
    }

    if (cJSON_AddStringToObject(root, "MessageType", ControlTuning::Name) == nullptr)
    {
        ESP_LOGW(ControlTuning::Name, "Unable to add MessageType to ControlTuning JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    // Add setpoint
    cJSON* setpoint = cJSON_CreateNumber(_setpoint);
    if (setpoint == nullptr) {
        ESP_LOGW(ControlTuning::Name, "Error creating setpoint JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, ControlTuning::SetpointStr, setpoint);

    // Add P gain
    cJSON* PGain = cJSON_CreateNumber(_PGain);
    if (PGain == nullptr) {
        ESP_LOGW(ControlTuning::Name, "Error creating PGain JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, ControlTuning::PGainStr, PGain);

    // Add I gain
    cJSON* IGain = cJSON_CreateNumber(_IGain);
    if (IGain == nullptr) {
        ESP_LOGW(ControlTuning::Name, "Error creating IGain JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, ControlTuning::IGainStr, IGain);

    // Add D gain
    cJSON* DGain = cJSON_CreateNumber(_DGain);
    if (DGain == nullptr) {
        ESP_LOGW(ControlTuning::Name, "Error creating IGain JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, ControlTuning::DGainStr, DGain);

    // Add LPF cutoff
    cJSON* LPFCutoff = cJSON_CreateNumber(_LPFCutoff);
    if (LPFCutoff == nullptr) {
        ESP_LOGW(ControlTuning::Name, "Error creating LPF cutoff JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, ControlTuning::LPFCutoffStr, LPFCutoff);

    // Copy JSON to string. cJSON requires printing to a char* pointer. Copy into
    // std::string and free memory to avoid memory leak
    char* stringPtr = cJSON_Print(root);
    JSONstr = std::string(stringPtr);
    cJSON_Delete(root);
    free(stringPtr);

    return PBRet::SUCCESS;
}

PBRet ControlTuning::deserialize(const cJSON* root)
{
    // Load the ControlTuning object from JSON

    if (root == nullptr) {
        ESP_LOGW(ControlTuning::Name, "root JSON object was null");
        return PBRet::FAILURE;
    }

    // Read setpoint
    cJSON* setpointNode = cJSON_GetObjectItem(root, ControlTuning::SetpointStr);
    if (cJSON_IsNumber(setpointNode)) {
        _setpoint = setpointNode->valuedouble;
    } else {
        ESP_LOGI(ControlTuning::Name, "Unable to read setpint from JSON");
        return PBRet::FAILURE;
    }

    // Read P gain
    cJSON* PGainNode = cJSON_GetObjectItem(root, ControlTuning::PGainStr);
    if (cJSON_IsNumber(PGainNode)) {
        _PGain = PGainNode->valuedouble;
    } else {
        ESP_LOGI(ControlTuning::Name, "Unable to read P gain from JSON");
        return PBRet::FAILURE;
    }

    // Read I gain
    cJSON* IGainNode = cJSON_GetObjectItem(root, ControlTuning::IGainStr);
    if (cJSON_IsNumber(IGainNode)) {
        _IGain = IGainNode->valuedouble;
    } else {
        ESP_LOGI(ControlTuning::Name, "Unable to read I gain from JSON");
        return PBRet::FAILURE;
    }

    // Read D gain
    cJSON* DGainNode = cJSON_GetObjectItem(root, ControlTuning::DGainStr);
    if (cJSON_IsNumber(DGainNode)) {
        _DGain = DGainNode->valuedouble;
    } else {
        ESP_LOGI(ControlTuning::Name, "Unable to read D gain from JSON");
        return PBRet::FAILURE;
    }

    // Read LPF cutoff
    cJSON* LPFCutoffNode = cJSON_GetObjectItem(root, ControlTuning::LPFCutoffStr);
    if (cJSON_IsNumber(LPFCutoffNode)) {
        _LPFCutoff = LPFCutoffNode->valuedouble;
    } else {
        ESP_LOGI(ControlTuning::Name, "Unable to read LPF cutoff from JSON");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet Controller::saveTuningToFile(void)
{
    // Save the current controller tuning to a JSON file and store
    // in flash

    std::string JSONStr {};
    if (_ctrlTuning.serialize(JSONStr) != PBRet::SUCCESS) {
        ESP_LOGW(Controller::Name, "Unable to save controller tuning to file");
        return PBRet::FAILURE;
    }

    // Mount filesystem
    Filesystem F(Controller::FSBasePath, Controller::FSPartitionLabel, 5, true);
    if (F.isOpen() == false) {
        ESP_LOGW(Controller::Name, "Failed to mount filesystem. Controller config was not written to file");
        return PBRet::FAILURE;
    }

    std::ofstream outFile(Controller::ctrlTuningFile);
    if (outFile.is_open() == false) {
        ESP_LOGE(Controller::Name, "Failed to open file for writing. Controller config was not written to file");
        return PBRet::FAILURE;
    }

    // Write JSON string out to file
    outFile << JSONStr;
    ESP_LOGI(Controller::Name, "Controller tuning successfully written to file");
    return PBRet::SUCCESS;
}

PBRet Controller::loadTuningFromFile(void)
{
    // Load a saved controller tuning from a JSON file and configure
    // controller. Returns Failure if no file can be found

    // Mount filesystem
    Filesystem F(Controller::FSBasePath, Controller::FSPartitionLabel, 5, true);
    if (F.isOpen() == false) {
        ESP_LOGW(Controller::Name, "Failed to mount filesystem. Controller config was not read from file");
        return PBRet::FAILURE;
    }

    // Read JSON string from file
    std::ifstream ctrlTuningIn(Controller::ctrlTuningFile);
    if (ctrlTuningIn.good() == false) {
        ESP_LOGW(Controller::Name, "Controller tuning file %s could not be opened", Controller::ctrlTuningFile);
        return PBRet::FAILURE;
    }

    std::stringstream JSONBuffer;
    JSONBuffer << ctrlTuningIn.rdbuf();

    cJSON* root = cJSON_Parse(JSONBuffer.str().c_str());
    if (root == nullptr) {
        ESP_LOGE(Controller::Name, "Failed to load controller tuning from JSON. Root JSON pointer was null");
        return PBRet::FAILURE;
    }
    
    return _ctrlTuning.deserialize(root);
}

PBRet ControlCommand::serialize(std::string &JSONStr) const
{
    // Write the ControlCommand object to JSON

    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        ESP_LOGW(ControlCommand::Name, "Unable to create root JSON object");
        return PBRet::FAILURE;
    }

    if (cJSON_AddStringToObject(root, "MessageType", ControlCommand::Name) == nullptr)
    {
        ESP_LOGW(ControlCommand::Name, "Unable to add MessageType to control command JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    // Add fanstate
    cJSON* fanStateNode = cJSON_CreateNumber(static_cast<int>(fanState));
    if (fanStateNode == nullptr) {
        ESP_LOGW(ControlCommand::Name, "Error creating fanState JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, ControlCommand::FanStateStr, fanStateNode);

    // Add low power element
    cJSON* LPElementNode = cJSON_CreateNumber(static_cast<int>(LPElementState));
    if (LPElementNode == nullptr) {
        ESP_LOGW(ControlCommand::Name, "Error creating low power element JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, ControlCommand::LPElementStr, LPElementNode);

    // Add high power element
    cJSON* HPElementNode = cJSON_CreateNumber(static_cast<int>(HPElementState));
    if (HPElementNode == nullptr) {
        ESP_LOGW(ControlCommand::Name, "Error creating high power element JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, ControlCommand::HPElementStr, HPElementNode);

    char* stringPtr = cJSON_Print(root);
    JSONStr = std::string(stringPtr);
    cJSON_Delete(root);
    free(stringPtr);

    return PBRet::SUCCESS;
}

PBRet ControlCommand::deserialize(const cJSON *root)
{
    // Load the ControlCommand object from JSON

    if (root == nullptr) {
        ESP_LOGW(ControlCommand::Name, "root JSON object was null");
        return PBRet::FAILURE;
    }

    // Read fanState
    const cJSON* fanStateNode = cJSON_GetObjectItem(root, ControlCommand::FanStateStr);
    if (cJSON_IsNumber(fanStateNode)) {
        fanState = static_cast<ControllerState>(fanStateNode->valueint);
    } else {
        ESP_LOGI(ControlCommand::Name, "Unable to read fanState from JSON");
        return PBRet::FAILURE;
    }

    // Read LPElement
    const cJSON* LPElementNode = cJSON_GetObjectItem(root, ControlCommand::LPElementStr);
    if (cJSON_IsNumber(LPElementNode)) {
        LPElementState = static_cast<ControllerState>(LPElementNode->valueint);
    } else {
        ESP_LOGI(ControlCommand::Name, "Unable to read LP Element state from JSON");
        return PBRet::FAILURE;
    }

    // Read HPElement
    const cJSON* HPElementNode = cJSON_GetObjectItem(root, ControlCommand::HPElementStr);
    if (cJSON_IsNumber(HPElementNode)) {
        HPElementState = static_cast<ControllerState>(HPElementNode->valueint);
    } else {
        ESP_LOGI(ControlCommand::Name, "Unable to read HP Element state from JSON");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}