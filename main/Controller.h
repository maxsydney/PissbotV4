#ifndef MAIN_CONTROLLER_H
#define MAIN_CONTROLLER_H

#include "PBCommon.h"
#include "CppTask.h"
#include "SensorManager.h"
#include "Pump.h"
#include "SlowPWM.h"
#include "Filter.h"
#include "ControllerMessaging.h"
#include "Generated/MessageBase.h"
#include "Generated/ControllerMessaging.h"

struct ControllerConfig
{
    double dt = 0.0;
    PumpConfig refluxPumpConfig{};
    PumpConfig prodPumpConfig{};
    gpio_num_t fanPin = (gpio_num_t)GPIO_NUM_NC;
    gpio_num_t element1Pin = (gpio_num_t)GPIO_NUM_NC;
    gpio_num_t element2Pin = (gpio_num_t)GPIO_NUM_NC;
    SlowPWMConfig LPElementPWM{};
    SlowPWMConfig HPElementPWM{};
};

class Controller : public Task
{
    // Class strings
    static constexpr const char *Name = "Controller";
    static constexpr const char *FSBasePath = "/spiffs";
    static constexpr const char *FSPartitionLabel = "PBData";
    static constexpr const char *ctrlTuningFile = "/spiffs/ctrlTuning.json";

    // Bounds
    static constexpr double HYSTERESIS_BOUND_UPPER = 70; // Upper hysteresis bound for product pump [deg c]
    static constexpr double HYSTERESIS_BOUND_LOWER = 68; // Lower hysteresis bound for product pump [deg c]
    static constexpr double MAX_CONTROL_TEMP = 105;      // Maximum controllable temp [deg c]
    static constexpr double MIN_CONTROL_TEMP = -5;       // Minimum controllable temp
    static constexpr double TEMP_MESSAGE_TIMEOUT = 1e6;  // Time before temperarture message goes stale (us)

public:
    // Constructors
    Controller(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const ControllerConfig &cfg);

    // Utility
    static PBRet checkInputs(const ControllerConfig &cfg);
    static PBRet loadFromJSON(ControllerConfig &cfg, const cJSON *cfgRoot);
    bool isConfigured(void) const { return _configured; }

    // Setters
    void setRefluxPumpMode(PumpMode pumpMode) { _ctrlSettings.set_refluxPumpMode(pumpMode); }
    void setProductPumpMode(PumpMode pumpMode) { _ctrlSettings.set_productPumpMode(pumpMode); }

    // Getters
    PumpMode getRefluxPumpMode(void) const { return _ctrlSettings.refluxPumpMode(); }
    PumpMode getProductPumpMode(void) const { return _ctrlSettings.productPumpMode(); }

    friend class ControllerUT;

private:
    // Initialization
    PBRet _initIO(const ControllerConfig &cfg) const;
    PBRet _initPumps(const PumpConfig &refluxPumpConfig, const PumpConfig &prodPumpConfig);
    PBRet _initPWM(const SlowPWMConfig &LPElementCfg, const SlowPWMConfig &HPElementCfg);

    // Updates
    PBRet _doControl(double temp);
    PBRet _updatePeripheralState(const ControllerCommand &cmd);
    PBRet _updatePumps(void);
    PBRet _updateProductPump(double temp);
    PBRet _updateRefluxPump(void);
    PBRet _checkTemperatures(const TemperatureData &currTemp) const;

    // Setup methods
    PBRet _initFromParams(const ControllerConfig &cfg);
    PBRet _setupCBTable(void) override;

    // FreeRTOS hook method
    void taskMain(void) override;

    // Config
    PBRet saveTuningToFile(void);
    PBRet loadTuningFromFile(void);

    // Queue callbacks
    PBRet _generalMessageCB(std::shared_ptr<PBMessageWrapper> msg);
    PBRet _temperatureDataCB(std::shared_ptr<PBMessageWrapper> msg);
    PBRet _controlCommandCB(std::shared_ptr<PBMessageWrapper> msg);
    PBRet _controlSettingsCB(std::shared_ptr<PBMessageWrapper> msg);
    PBRet _controlTuningCB(std::shared_ptr<PBMessageWrapper> msg);
    PBRet _controlDataRequestCB(std::shared_ptr<PBMessageWrapper> msg);

    // Data broadcast
    PBRet _broadcastControllerTuning(void) const;
    PBRet _broadcastControllerSettings(void) const;
    PBRet _broadcastControllerPeripheralState(void) const;
    PBRet _broadcastControllerState(void) const;

    // Controller data
    ControllerConfig _cfg{};
    ControllerCommand _peripheralState{};
    TemperatureData _currentTemp{};
    ControllerTuning _ctrlTuning{};
    ControllerSettings _ctrlSettings{};

    Pump _refluxPump{};
    Pump _productPump{};
    bool _configured = false;
    IIRLowpassFilter _derivFilter {};

    // Internal state
    double _currentOutput = 0.0;
    double _prevError = 0.0;
    double _proportional = 0.0;
    double _integral = 0.0;
    double _derivative = 0.0;
    double _prevTemp = 0.0;
    SlowPWM _LPElementPWM{};
    SlowPWM _HPElementPWM{};
};

#endif // MAIN_CONTROLLER_H