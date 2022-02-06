#include "Controller.h"
#include "Filesystem.h"
#include "Utilities.h"
#include "cJSON.h"
#include <fstream>
#include <sstream>
#include "IO/Writable.h"
#include "IO/Readable.h"

Controller::Controller(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const ControllerConfig& cfg)
    : Task(Controller::Name, priority, stackDepth, coreID)
{
    // Setup callback table
    _setupCBTable();

    // Set message ID
    Task::_ID = MessageOrigin::Controller;

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
    std::set<PBMessageType> subscriptions = {
        PBMessageType::TemperatureData,
        PBMessageType::ControllerTuning,
        PBMessageType::ControllerCommand,
        PBMessageType::ControllerSettings,
        PBMessageType::ControllerDataRequest
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
        if (_doControl(_currentTemp.get_headTemp()) != PBRet::SUCCESS) {
            ESP_LOGW(Controller::Name, "Control law update failed");
            // Send warning message to distiller controller
        }

        // Update peripheral outputs
        if (_updatePeripheralState(_peripheralState) != PBRet::SUCCESS) {
            ESP_LOGW(Controller::Name, "Peripheral update failed");
        }

        // Command pumps
        if (_updatePumps() != PBRet::SUCCESS) {
            ESP_LOGW(Controller::Name, "Pump speeds were not updated");
        }

        // Broadcast controller state
        if (_broadcastControllerState() != PBRet::SUCCESS) {
            ESP_LOGW(Controller::Name, "Could not broadcast controller state");
        }

        vTaskDelayUntil(&xLastWakeTime, timestep);
    }
}

PBRet Controller::_temperatureDataCB(std::shared_ptr<PBMessageWrapper> msg)
{
    // Store the current temperature estimate
    return MessageServer::unwrap(*msg, _currentTemp); 
}

PBRet Controller::_controlCommandCB(std::shared_ptr<PBMessageWrapper> msg)
{
    _peripheralState.clear();   // Reset defaults
    if (MessageServer::unwrap(*msg, _peripheralState) != PBRet::SUCCESS) {
        ESP_LOGW(Controller::Name, "Failed to decode control command message");
    }

    // Update PWM drivers
    if (_LPElementPWM.setDutyCycle(_peripheralState.LPElementDutyCycle()) != PBRet::SUCCESS) {
        ESP_LOGW(Controller::Name, "Failed to update LPElement duty cycle");
        return PBRet::FAILURE;
    }

    if (_HPElementPWM.setDutyCycle(_peripheralState.HPElementDutyCycle()) != PBRet::SUCCESS) {
        ESP_LOGW(Controller::Name, "Failed to update HPElement duty cycle");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet Controller::_controlSettingsCB(std::shared_ptr<PBMessageWrapper> msg)
{
    // Store new settings and update pumps
    _ctrlSettings.clear();
    if (MessageServer::unwrap(*msg, _ctrlSettings) != PBRet::SUCCESS) {
        ESP_LOGW(Controller::Name, "Failed to decode");
    }

    ESP_LOGI(Controller::Name, "Controller settings were updated");

    return _updatePumps();
}

PBRet Controller::_controlTuningCB(std::shared_ptr<PBMessageWrapper> msg)
{
    ControllerTuning tuning {};
    if (MessageServer::unwrap(*msg, tuning) != PBRet::SUCCESS) {
        ESP_LOGW(Controller::Name, "Failed to decode ");
    }

    // Reinitialize filter
    IIRLowpassFilterConfig filterConfig(tuning.LPFsampleFreq(), tuning.LPFcutoffFreq());
    if (IIRLowpassFilter::checkInputs(filterConfig) == PBRet::SUCCESS) {
        _derivFilter = IIRLowpassFilter(filterConfig);
    } else {
        ESP_LOGW(Controller::Name, "Failed to initialize LPF. Filter parameters were invalid");
        return PBRet::FAILURE;
    }

    // Can safely update tuning now
    _ctrlTuning = tuning;
    ESP_LOGI(Controller::Name, "Controller tuning was updated");

    // Write controller tuning to file
    if (saveTuningToFile() != PBRet::SUCCESS) {
        ESP_LOGW(Controller::Name, "Unable to save controller tuning to file");
    }

    if (_broadcastControllerTuning() != PBRet::SUCCESS) {
        ESP_LOGW(Controller::Name, "Failed to broadcast controller tuning");
    }

    return PBRet::SUCCESS;
}

PBRet Controller::_controlDataRequestCB(std::shared_ptr<PBMessageWrapper> msg)
{
    // Broadcast the requested data
    //

    ESP_LOGI(Controller::Name, "Got request for controller data");

    ControllerDataRequest request {};
    if (MessageServer::unwrap(*msg, request) != PBRet::SUCCESS) {
        ESP_LOGW(Controller::Name, "Failed to decode controller data request");
        return PBRet::FAILURE;
    }

    switch (request.get_requestType())
    {
        case (ControllerDataRequestType::TUNING):
        {
            ESP_LOGI(Controller::Name, "Got request for controller tuning data");
            return _broadcastControllerTuning();
        }
        case (ControllerDataRequestType::SETTINGS):
        {
            ESP_LOGI(Controller::Name, "Got request for controller settings");
            return _broadcastControllerSettings();
        }
        case (ControllerDataRequestType::PERIPHERAL_STATE):
        {
            ESP_LOGI(Controller::Name, "Got request for controller peripheral state");
            return _broadcastControllerPeripheralState();
        }
        case (ControllerDataRequestType::NONE):
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
    _cbTable = std::map<PBMessageType, queueCallback> {
        {PBMessageType::TemperatureData, std::bind(&Controller::_temperatureDataCB, this, std::placeholders::_1)},
        {PBMessageType::ControllerCommand, std::bind(&Controller::_controlCommandCB, this, std::placeholders::_1)},
        {PBMessageType::ControllerSettings, std::bind(&Controller::_controlSettingsCB, this, std::placeholders::_1)},
        {PBMessageType::ControllerTuning, std::bind(&Controller::_controlTuningCB, this, std::placeholders::_1)},
        {PBMessageType::ControllerDataRequest, std::bind(&Controller::_controlDataRequestCB, this, std::placeholders::_1)}
    };

    return PBRet::SUCCESS;
}

PBRet Controller::_broadcastControllerTuning(void) const
{
    // Send a controller message to the queue
    PBMessageWrapper wrapped = MessageServer::wrap(_ctrlTuning, PBMessageType::ControllerTuning, _ID);

    return MessageServer::broadcastMessage(wrapped);
}

PBRet Controller::_broadcastControllerSettings(void) const
{
    // Send a controller settings message to the queue
    PBMessageWrapper wrapped = MessageServer::wrap(_ctrlSettings, PBMessageType::ControllerSettings, _ID);

    return MessageServer::broadcastMessage(wrapped);
}

PBRet Controller::_broadcastControllerPeripheralState(void) const
{
    // Send a Control command message to the queue
    PBMessageWrapper wrapped = MessageServer::wrap(_peripheralState, PBMessageType::ControllerCommand, _ID);

    return MessageServer::broadcastMessage(wrapped);
}

PBRet Controller::_broadcastControllerState(void) const
{
    // Send a ControllerState message to the queue
    ControllerState state {};
    state.set_propOutput(_proportional);
    state.set_integralOutput(_integral);
    state.set_derivOutput(_derivative);
    state.set_totalOutput(_currentOutput);
    state.set_timeStamp(esp_timer_get_time());

    PBMessageWrapper wrapped = MessageServer::wrap(state, PBMessageType::ControllerState, _ID);

    return MessageServer::broadcastMessage(wrapped);
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
    _ctrlSettings.set_refluxPumpMode(PumpMode::PUMP_OFF);
    _ctrlSettings.set_productPumpMode(PumpMode::PUMP_OFF);

    return PBRet::SUCCESS;
}

PBRet Controller::_initPWM(const SlowPWMConfig& LPElementCfg, const SlowPWMConfig& HPElementCfg)
{
    // Initialize SlowPWM drivers

    _LPElementPWM = SlowPWM(LPElementCfg);
    _HPElementPWM = SlowPWM(HPElementCfg);

    if (_LPElementPWM.isConfigured() == false) {
        ESP_LOGW(Controller::Name, "Unable to configure low power element PWM driver");
        return PBRet::FAILURE;
    }

    if (_HPElementPWM.isConfigured() == false) {
        ESP_LOGW(Controller::Name, "Unable to configure high power element PWM driver");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet Controller::_doControl(double temp)
{
    // Implements a basic PID controller with anti-integral windup
    // and filtering on derivative

    const double err = temp - _ctrlTuning.setpoint();

    // Proportional term
    _proportional = _ctrlTuning.PGain() * err;

    // Integral term (discretized via bilinear transform)
    _integral += 0.5 * _ctrlTuning.IGain() * _cfg.dt * (err + _prevError);

    // Dynamic integral clamping/anti windup. Limit integral signal so that
    // PI control does not exceed pump maximum speed. 
    double intLimMin = 0.0;
    double intLimMax = 0.0;

    if (_proportional < Pump::PUMP_MAX_SPEED) {
        intLimMax = Pump::PUMP_MAX_SPEED - _proportional;
    } else {
        intLimMax = 0.0;
    }

    if (_proportional > Pump::PUMP_IDLE_SPEED) {
        intLimMin = Pump::PUMP_IDLE_SPEED - _proportional;
    } else {
        intLimMin = 0.0;
    }

    if (_integral > intLimMax) {
        _integral = intLimMax;
    } else if (_integral < intLimMin) {
        _integral = intLimMin;
    }

    // Derivative term filtered with biquad LPF. If filter is not configured, use 
    // raw measurements
    const double derivRaw = _ctrlTuning.DGain() * (temp - _prevTemp) / _cfg.dt;
    if (_derivFilter.filter(derivRaw, _derivative) != PBRet::SUCCESS) {
        // Error message printed in filter
        _derivative = derivRaw;
    }

    // Compute limited output
    const double totalOutput = _proportional + _integral + _derivative;
    _currentOutput = Utilities::bound(totalOutput, Pump::PUMP_OFF, Pump::PUMP_MAX_SPEED);
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
    if (_updateProductPump(_currentTemp.get_headTemp()) != PBRet::SUCCESS) {
        ESP_LOGW(Controller::Name, "Pump update failed");
        return PBRet::FAILURE;
    }
    
    return PBRet::SUCCESS;
}

PBRet Controller::_updateRefluxPump(void)
{
    // Update the reflux pump speed
    if (_ctrlSettings.get_refluxPumpMode() == PumpMode::ACTIVE_CONTROL) {
        if (_refluxPump.updatePumpSpeed(_currentOutput) != PBRet::SUCCESS) {
            ESP_LOGW(Controller::Name, "Failed to update reflux pump speed in active mode");
            return PBRet::FAILURE;
        }
    } else if (_ctrlSettings.get_refluxPumpMode() == PumpMode::MANUAL_CONTROL) {
        if (_refluxPump.updatePumpSpeed(_ctrlSettings.manualPumpSpeeds().refluxPumpSpeed()) != PBRet::SUCCESS) {
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
    if (_ctrlSettings.get_productPumpMode() == PumpMode::ACTIVE_CONTROL) {
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
    } else if (_ctrlSettings.get_productPumpMode() == PumpMode::MANUAL_CONTROL) {
        if (_productPump.updatePumpSpeed(_ctrlSettings.get_manualPumpSpeeds().productPumpSpeed()) != PBRet::SUCCESS) {
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
    if ((currTemp.get_headTemp() > MAX_CONTROL_TEMP) || (currTemp.get_headTemp() < MIN_CONTROL_TEMP))
    {
        ESP_LOGW(Controller::Name, "Head temp (%.2f) was outside controllable bounds [%.2f, %.2f]", currTemp.get_headTemp().get(), MIN_CONTROL_TEMP, MAX_CONTROL_TEMP);
        return PBRet::FAILURE;
    }

    // Check that current temperature hasn't expired
    if ((esp_timer_get_time() - currTemp.timeStamp()) > TEMP_MESSAGE_TIMEOUT) {
        ESP_LOGW(Controller::Name, "Temperature message was stale");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet Controller::_updatePeripheralState(const ControllerCommand& cmd)
{
    esp_err_t err = ESP_OK;

    // Update PWM drivers
    uint32_t LPElementState = 0;
    if (_LPElementPWM.update(esp_timer_get_time()) == PBRet::SUCCESS) {
        LPElementState = _LPElementPWM.getOutputState();
    } else {
        ESP_LOGW(Controller::Name, "Failed to update LPElement PWM driver");
        err |= ESP_FAIL;
    }

    uint32_t HPElementState = 0;
    if (_HPElementPWM.update(esp_timer_get_time()) == PBRet::SUCCESS) {
        HPElementState = _HPElementPWM.getOutputState();
    } else {
        ESP_LOGW(Controller::Name, "Failed to update HPElement PWM driver");
        err |= ESP_FAIL;
    }

    err |= gpio_set_level(_cfg.fanPin, static_cast<uint32_t> (cmd.fanState()));
    err |= gpio_set_level(_cfg.element1Pin, LPElementState);
    err |= gpio_set_level(_cfg.element2Pin, HPElementState);

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

    // Load LPElement PWM driver
    cJSON* LPElementPWMNode = cJSON_GetObjectItem(cfgRoot, "slowPMWLPElement");
    if (SlowPWM::loadFromJSON(cfg.LPElementPWM, LPElementPWMNode) != PBRet::SUCCESS) {
        return PBRet::FAILURE;
    }

    // Load HPElement PWM driver
    cJSON* HPElementPWMNode = cJSON_GetObjectItem(cfgRoot, "slowPMWHPElement");
    if (SlowPWM::loadFromJSON(cfg.HPElementPWM, HPElementPWMNode) != PBRet::SUCCESS) {
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet Controller::_initFromParams(const ControllerConfig& cfg)
{
    if (Controller::checkInputs(cfg) != PBRet::SUCCESS) {;
        ESP_LOGE(Controller::Name, "Unable to configure controller");
        return PBRet::FAILURE;
    }

    if (_initPumps(cfg.refluxPumpConfig, cfg.prodPumpConfig) != PBRet::SUCCESS) {
        ESP_LOGE(Controller::Name, "Unable to configure one or more pumps");
        return PBRet::FAILURE;
    }

    if (_initIO(cfg) != PBRet::SUCCESS) {
        ESP_LOGE(Controller::Name, "Unable to configure controller I/O");
        return PBRet::FAILURE;
    }

    if (_initPWM(cfg.LPElementPWM, cfg.HPElementPWM) != PBRet::SUCCESS) {
        ESP_LOGE(Controller::Name, "One or more PWM drivers were not configured");
        return PBRet::FAILURE;
    }

    // Load controller tuning from file (if it exists)
    if (loadTuningFromFile() != PBRet::SUCCESS) {
        ESP_LOGW(Controller::Name, "Unable to load controller tuning from file");
    }

    // Initialize derivative filter from loaded tuning object
    _derivFilter = IIRLowpassFilter(IIRLowpassFilterConfig(_ctrlTuning.LPFsampleFreq(), _ctrlTuning.LPFcutoffFreq()));
    if (_derivFilter.isConfigured() == false) {
        ESP_LOGW(Controller::Name, "Unable to initialize derivative filter");
    }

    // Set pump manual speeds to idle
    PumpSpeeds initPumpSpeeds {};
    initPumpSpeeds.set_refluxPumpSpeed(Pump::PUMP_IDLE_SPEED);
    initPumpSpeeds.set_productPumpSpeed(Pump::PUMP_IDLE_SPEED);
    _ctrlSettings.set_manualPumpSpeeds(initPumpSpeeds);

    // Set pumps to active control
    _ctrlSettings.set_refluxPumpMode(PumpMode::ACTIVE_CONTROL);
    _ctrlSettings.set_refluxPumpMode(PumpMode::ACTIVE_CONTROL);
    _cfg = cfg;

    return PBRet::SUCCESS;
}

PBRet Controller::saveTuningToFile(void)
{
    // Save the current controller tuning to a JSON file and store
    // in flash

    // Mount filesystem
    Filesystem F(Controller::FSBasePath, Controller::FSPartitionLabel, 5, true);
    if (F.isOpen() == false) {
        ESP_LOGW(Controller::Name, "Failed to mount filesystem. Controller config was not written to file");
        return PBRet::FAILURE;
    }

    std::ofstream outFile(Controller::ctrlTuningFile, std::ios::binary | std::ios::out);
    if (outFile.is_open() == false) {
        ESP_LOGE(Controller::Name, "Failed to open file for writing. Controller config was not written to file");
        return PBRet::FAILURE;
    }

    ESP_LOGI(Controller::Name, "Serializing buffer");
    Writable writeBuffer {};
    ::EmbeddedProto::Error err = _ctrlTuning.serialize(writeBuffer);
    if (err != ::EmbeddedProto::Error::NO_ERRORS) {
        ESP_LOGW(Controller::Name, "Failed to serialize control tuning object (err: %d)", static_cast<int>(err));
        return PBRet::FAILURE;
    }

    // Write data structure to file
    ESP_LOGI(Controller::Name, "Writing %d bytes to file", writeBuffer.get_size());
    outFile.write((char*) writeBuffer.get_buffer(), writeBuffer.get_size());
    ESP_LOGI(Controller::Name, "Controller tuning successfully written to file");

    size_t totalBytes = 0;
    size_t usedBytes = 0;
    F.getInfo(totalBytes, usedBytes);
    ESP_LOGW(Controller::Name, "PBData total: %d bytes, used: %d bytes", totalBytes, usedBytes);

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

    std::ifstream inFile(Controller::ctrlTuningFile);
    if (inFile.is_open() == false) {
        ESP_LOGE(Controller::Name, "Failed to open file for reading. Controller config was not read from file");
        return PBRet::FAILURE;
    }

    std::vector<uint8_t> bytes(
         (std::istreambuf_iterator<char>(inFile)),
         (std::istreambuf_iterator<char>()));

    Readable buffer {};
    for (uint8_t byte : bytes)
    {
        buffer.push(byte);
    }

    // Decode into ControllerTuning object
    ::EmbeddedProto::Error err = _ctrlTuning.deserialize(buffer);
    if (err != ::EmbeddedProto::Error::NO_ERRORS) {
        ESP_LOGW(Controller::Name, "Failed to deserialize control tuning object (err: %d)", static_cast<int>(err));
        return PBRet::FAILURE;
    }

    ESP_LOGI(Controller::Name, "Loaded tuning settings from file");

    return PBRet::SUCCESS;
}