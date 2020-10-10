// #ifdef __cplusplus
// extern "C" {
// #endif

#include "controller.h"
#include "controlLoop.h"
#include "gpio.h"

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
        MessageType::ControlSettings
    };
    Subscriber sub(Controller::Name, _GPQueue, subscriptions);
    MessageServer::registerTask(sub);

    portTickType xLastWakeTime = xTaskGetTickCount();
    while (true) {
        _processQueue();

        // vTaskDelayUntil(&xLastWakeTime, _cfg.dt / portTICK_PERIOD_MS);
        vTaskDelay(130 / portTICK_RATE_MS);
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
    ESP_LOGI(Controller::Name, "Received temperature message:\nHead: %.3lf\nReflux condensor: %.3f\nProduct condensor: %.3lf\n"
                               "Radiator: %.3f\nBoiler: %.3lf\nUptime: %ld", TData->getHeadTemp(), TData->getRefluxCondensorTemp(),
                                TData->getProdCondensorTemp(), TData->getRadiatorTemp(), TData->getBoilerTemp(), (long int) TData->getTimeStamp());

    return PBRet::SUCCESS;
}

PBRet Controller::_controlCommandCB(std::shared_ptr<MessageBase> msg)
{
    return PBRet::SUCCESS;
}

PBRet Controller::_controlSettingsCB(std::shared_ptr<MessageBase> msg)
{
    return PBRet::SUCCESS;
}

PBRet Controller::_controlTuningCB(std::shared_ptr<MessageBase> msg)
{
    return PBRet::SUCCESS;
}

PBRet Controller::_setupCBTable(void)
{
    _cbTable = std::map<MessageType, queueCallback> {
        {MessageType::General, std::bind(&Controller::_generalMessageCB, this, std::placeholders::_1)},
        {MessageType::TemperatureData, std::bind(&Controller::_temperatureDataCB, this, std::placeholders::_1)},
        {MessageType::ControlCommand, std::bind(&Controller::_controlCommandCB, this, std::placeholders::_1)},
        {MessageType::ControlSettings, std::bind(&Controller::_controlSettingsCB, this, std::placeholders::_1)},
        {MessageType::ControlTuning, std::bind(&Controller::_controlTuningCB, this, std::placeholders::_1)}
    };

    return PBRet::SUCCESS;
}

PBRet Controller::_initIO(const ControllerConfig& cfg) const
{
    // Initialize fan pin
    gpio_pad_select_gpio(cfg.fanPin);
    gpio_set_direction(cfg.fanPin, GPIO_MODE_OUTPUT);
    gpio_set_level(cfg.fanPin, 0);    
    
    // Initialize 2.4kW element control pin
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[cfg.element1Pin], PIN_FUNC_GPIO);        // Element 1 pin is set to JTAG by default. Reassign to GPIO
    gpio_set_direction(cfg.element1Pin, GPIO_MODE_OUTPUT);
    gpio_set_level(cfg.element1Pin, 0);  

    // Initialize 3kW element control pin
    gpio_pad_select_gpio(cfg.element2Pin);
    gpio_set_direction(cfg.element2Pin, GPIO_MODE_OUTPUT);
    gpio_set_level(cfg.element2Pin, 0); 

    return PBRet::SUCCESS;
}

PBRet Controller::_initPumps(const PumpCfg& refluxPumpCfg, const PumpCfg& prodPumpCfg)
{
    _refluxPump = Pump(refluxPumpCfg);
    _prodPump = Pump(prodPumpCfg);
    _refluxPump.setMode(PumpMode::ACTIVE);
    _prodPump.setMode(PumpMode::ACTIVE);
    _refluxPump.setSpeed(Pump::PUMP_MIN_OUTPUT);
    _prodPump.setSpeed(Pump::PUMP_MIN_OUTPUT);

    return PBRet::SUCCESS;
}

PBRet Controller::_doControl(double temp)
{
    double err = temp - _ctrlParams.setpoint;

    // Proportional term
    double proportional = _ctrlParams.P_gain * err;

    // Integral term (discretized via bilinear transform)
    _integral += 0.5 * _ctrlParams.I_gain * _cfg.dt * (err + _prevError);

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
    _derivative = (1 - alpha) * _derivative + alpha * (_ctrlParams.D_gain * (temp - _prevTemp) / _cfg.dt);

    // Compute limited output
    double output = proportional + _integral + _derivative;

    if (output > Pump::PUMP_MAX_OUTPUT) {
        output = Pump::PUMP_MAX_OUTPUT;
    } else if (output < Pump::PUMP_MIN_OUTPUT) {
        output = Pump::PUMP_MIN_OUTPUT;
    }

    // Debugging only, spams the network
    // printf("Temp: %.3f - Err: %.3f - P term: %.3f - I term: %.3f - D term: %.3f - Total output: %.3f\n", temp, err, proportional, _integral, _derivative, output);

    _prevError = err;
    _prevTemp = temp;
    _handleProductPump(temp);
    _refluxPump.setSpeed(output);
    _refluxPump.commandPump();
    _prodPump.commandPump();

    return PBRet::SUCCESS;
}

PBRet Controller::_handleProductPump(double temp)
{
    if (temp > 60) {
        _prodPump.setSpeed(Pump::FLUSH_SPEED);
    } else if (temp < 50) {
        _prodPump.setSpeed(Pump::PUMP_MIN_OUTPUT);
    }

    return PBRet::SUCCESS;
}

void Controller::updateComponents()
{
    setPin(_cfg.fanPin, _ctrlSettings.fanState);
    setPin(_cfg.element1Pin, _ctrlSettings.elementLow);
    setPin(_cfg.element2Pin, _ctrlSettings.elementHigh);
}

PBRet Controller::_updateSettings(ctrlSettings_t ctrlSettings)
{
    _ctrlSettings = ctrlSettings;
    
    if (ctrlSettings.flush == true) {
        ESP_LOGI(Controller::Name, "Setting both pumps to flush");
        _refluxPump.setSpeed(Pump::FLUSH_SPEED);
        _prodPump.setSpeed(Pump::FLUSH_SPEED);
        _refluxPump.setMode(PumpMode::FIXED);
        _prodPump.setMode(PumpMode::FIXED);
    } else {
        ESP_LOGI(Controller::Name, "Setting both pumps to active");
        _refluxPump.setMode(PumpMode::ACTIVE);
        _prodPump.setMode(PumpMode::ACTIVE);
    }

    if (ctrlSettings.prodCondensor == true) {
        ESP_LOGI(Controller::Name, "Setting prod pump to flush");
        _prodPump.setSpeed(Pump::FLUSH_SPEED);
        _prodPump.setMode(PumpMode::FIXED);
    } else if (_ctrlSettings.flush == false) {
        ESP_LOGI(Controller::Name, "Setting prod pump to active");
        _prodPump.setMode(PumpMode::ACTIVE);
    }

    ESP_LOGI(Controller::Name, "Settings updated");
    ESP_LOGI(Controller::Name, "Fan: %d | 2.4kW Element: %d | 3kW Element: %d | Flush: %d | Product Condensor: %d", ctrlSettings.fanState,
             ctrlSettings.elementLow, ctrlSettings.elementHigh, ctrlSettings.flush, ctrlSettings.prodCondensor); 

    updateComponents();

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

PBRet Controller::_initFromParams(const ControllerConfig& cfg)
{
    if (Controller::checkInputs(cfg) != PBRet::SUCCESS) {;
        ESP_LOGW(Controller::Name, "Unable to configure controller");
        return PBRet::FAILURE;
    }

    if (_initPumps(cfg.refluxPumpCfg, cfg.prodPumpCfg) != PBRet::SUCCESS) {
        ESP_LOGW(Controller::Name, "Unable to configure one or more pumps");
        return PBRet::FAILURE;
    }

    if (_initIO(cfg) != PBRet::SUCCESS) {
        ESP_LOGW(Controller::Name, "Unable to configure controller I/O");
        return PBRet::FAILURE;
    }

    _cfg = cfg;
    return PBRet::SUCCESS;
}

// #ifdef __cplusplus
// }
// #endif
