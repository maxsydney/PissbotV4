#ifndef MAIN_CONTROLLER_H
#define MAIN_CONTROLLER_H

#include "PBCommon.h"
#include "CppTask.h"
#include "messageServer.h"
#include "SensorManager.h"
#include "pump.h"
#include "SlowPWM.h"

enum class ComponentState
{
    OFF,
    ON
};

enum class ControllerDataRequestType
{
    None,
    Tuning,
    Settings,
    PeripheralState
};

class ControlSettings : public MessageBase
{
    static constexpr MessageType messageType = MessageType::ControlSettings;
    static constexpr const char *Name = "Controller settings";
    static constexpr const char *refluxPumpModeStr = "refluxPumpMode";
    static constexpr const char *productPumpModeStr = "productPumpMode";
    static constexpr const char *refluxPumpSpeedManualStr = "refluxPumpSpeedManual";
    static constexpr const char *productPumpSpeedManualStr = "productPumpSpeedManual";

public:
    ControlSettings(void) = default;
    ControlSettings(PumpMode refluxPumpMode, PumpMode productPumpMode, const PumpSpeeds &manualPumpSpeeds)
        : MessageBase(ControlSettings::messageType, ControlSettings::Name, esp_timer_get_time()),
          refluxPumpMode(refluxPumpMode), productPumpMode(productPumpMode), manualPumpSpeeds(manualPumpSpeeds) {}
    ControlSettings(const ControlSettings &other)
        : MessageBase(ControlSettings::messageType, ControlSettings::Name, esp_timer_get_time()),
          refluxPumpMode(other.refluxPumpMode), productPumpMode(other.productPumpMode), manualPumpSpeeds(other.manualPumpSpeeds) {}
    
    PBRet serialize(std::string &JSONstr) const;
    PBRet deserialize(const cJSON *root);

    PumpMode refluxPumpMode = PumpMode::Off;
    PumpMode productPumpMode = PumpMode::Off;
    PumpSpeeds manualPumpSpeeds{};
};

class ControlTuning : public MessageBase
{
    static constexpr MessageType messageType = MessageType::ControlTuning;
    static constexpr const char *Name = "Controller tuning";

    static constexpr const char *SetpointStr = "Setpoint";
    static constexpr const char *PGainStr = "PGain";
    static constexpr const char *IGainStr = "IGain";
    static constexpr const char *DGainStr = "DGain";
    static constexpr const char *LPFCutoffStr = "LPFCutoff";

public:
    ControlTuning(void) = default;
    ControlTuning(double setpoint, double PGain, double IGain, double DGain, double LPFCutoff)
        : MessageBase(ControlTuning::messageType, ControlTuning::Name, esp_timer_get_time()), _setpoint(setpoint),
          _PGain(PGain), _IGain(IGain), _DGain(DGain), _LPFCutoff(LPFCutoff) {}
    ControlTuning(const ControlTuning &other)
        : MessageBase(ControlTuning::messageType, ControlTuning::Name, esp_timer_get_time()), _setpoint(other._setpoint),
          _PGain(other._PGain), _IGain(other._IGain), _DGain(other._DGain), _LPFCutoff(other._LPFCutoff) {}

    double getSetpoint(void) const { return _setpoint; }
    double getPGain(void) const { return _PGain; }
    double getIGain(void) const { return _IGain; }
    double getDGain(void) const { return _DGain; }
    double getLPFCutoff(void) const { return _LPFCutoff; }

    PBRet serialize(std::string &JSONstr) const;
    PBRet deserialize(const cJSON *root);

private:
    double _setpoint = 0.0;
    double _PGain = 0.0;
    double _IGain = 0.0;
    double _DGain = 0.0;
    double _LPFCutoff = 0.0;
};

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

class ControllerDataRequest : public MessageBase
{
    static constexpr MessageType messageType = MessageType::ControllerDataRequest;
    static constexpr const char *Name = "Controller data request";

public:
    ControllerDataRequest(void) = default;
    ControllerDataRequest(ControllerDataRequestType requestType)
        : MessageBase(ControllerDataRequest::messageType, ControllerDataRequest::Name, esp_timer_get_time()),
          _requestType(requestType) {}

    ControllerDataRequestType getType(void) const { return _requestType; }

private:
    ControllerDataRequestType _requestType = ControllerDataRequestType::None;
};

class ControlCommand : public MessageBase
{
    static constexpr MessageType messageType = MessageType::ControlCommand;
    static constexpr const char *Name = "Controller command";

    static constexpr const char *FanStateStr = "fanState";
    static constexpr const char *LPElementStr = "LPElement";
    static constexpr const char *HPElementStr = "HPElement";

public:
    ControlCommand(void) = default;
    ControlCommand(ComponentState fanState, double LPElementDutyCycle, double HPElementDutyCycle)
        : MessageBase(ControlCommand::messageType, ControlCommand::Name, esp_timer_get_time()),
          fanState(fanState), LPElementDutyCycle(LPElementDutyCycle), HPElementDutyCycle(HPElementDutyCycle) {}
    ControlCommand(const ControlCommand &other)
        : MessageBase(ControlCommand::messageType, ControlCommand::Name, esp_timer_get_time()), fanState(other.fanState),
          LPElementDutyCycle(other.LPElementDutyCycle), HPElementDutyCycle(other.HPElementDutyCycle) {}

    PBRet serialize(std::string &JSONStr) const;
    PBRet deserialize(const cJSON *root);

    ComponentState fanState = ComponentState::OFF;
    double LPElementDutyCycle = 0.0;
    double HPElementDutyCycle = 0.0;
};

class ControllerState : public MessageBase
{
    static constexpr MessageType messageType = MessageType::ControllerState;
    static constexpr const char *Name = "Controller State";

    static constexpr const char *proportionalStr = "PropOutput";
    static constexpr const char *integralStr = "IntegralOutput";
    static constexpr const char *derivativeStr = "DerivOutput";
    static constexpr const char *totalOutputStr = "TotalOutput";

    public:
        // Constructors
        ControllerState(void) = default;
        ControllerState(double propOutput, double integralOutput, double derivOutput, double totalOutput)
            : MessageBase(ControllerState::messageType, ControllerState::Name, esp_timer_get_time()), propOutput(propOutput),
              integralOutput(integralOutput), derivOutput(derivOutput), totalOutput(totalOutput) {};
        ControllerState(const ControllerState& other)
            : MessageBase(ControllerState::messageType, ControllerState::Name, esp_timer_get_time()), propOutput(other.propOutput),
              integralOutput(other.integralOutput), derivOutput(other.derivOutput), totalOutput(other.totalOutput) {};

    PBRet serialize(std::string &JSONStr) const;
    PBRet deserialize(const cJSON *root);

    double propOutput = 0.0;
    double integralOutput = 0.0;
    double derivOutput = 0.0;
    double totalOutput = 0.0;
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
    void setRefluxPumpMode(PumpMode pumpMode) { _ctrlSettings.refluxPumpMode = pumpMode; }
    void setProductPumpMode(PumpMode pumpMode) { _ctrlSettings.productPumpMode = pumpMode; }

    // Getters
    PumpMode getRefluxPumpMode(void) const { return _ctrlSettings.refluxPumpMode; }
    PumpMode getProductPumpMode(void) const { return _ctrlSettings.productPumpMode; }

    friend class ControllerUT;

private:
    // Initialization
    PBRet _initIO(const ControllerConfig &cfg) const;
    PBRet _initPumps(const PumpConfig &refluxPumpConfig, const PumpConfig &prodPumpConfig);
    PBRet _initPWM(const SlowPWMConfig &LPElementCfg, const SlowPWMConfig &HPElementCfg);

    // Updates
    PBRet _doControl(double temp);
    PBRet _updatePeripheralState(const ControlCommand &cmd);
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
    PBRet _generalMessageCB(std::shared_ptr<MessageBase> msg);
    PBRet _temperatureDataCB(std::shared_ptr<MessageBase> msg);
    PBRet _controlCommandCB(std::shared_ptr<MessageBase> msg);
    PBRet _controlSettingsCB(std::shared_ptr<MessageBase> msg);
    PBRet _controlTuningCB(std::shared_ptr<MessageBase> msg);
    PBRet _controlDataRequestCB(std::shared_ptr<MessageBase> msg);

    // Data broadcast
    PBRet _broadcastControllerTuning(void) const;
    PBRet _broadcastControllerSettings(void) const;
    PBRet _broadcastControllerPeripheralState(void) const;

    // Controller data
    ControllerConfig _cfg{};
    ControlCommand _peripheralState{};
    TemperatureData _currentTemp{};
    ControlTuning _ctrlTuning{};
    ControlSettings _ctrlSettings{};

    Pump _refluxPump{};
    Pump _productPump{};
    bool _configured = false;

    // Internal state
    double _currentOutput = 0.0;
    double _prevError = 0.0;
    double _integral = 0.0;
    double _derivative = 0.0;
    double _proportional = 0.0;
    double _prevTemp = 0.0;
    SlowPWM _LPElementPWM{};
    SlowPWM _HPElementPWM{};
};

#endif // MAIN_CONTROLLER_H