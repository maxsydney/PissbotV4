#ifndef MAIN_CONTROLLER_H
#define MAIN_CONTROLLER_H

#include "PBCommon.h"
#include "CppTask.h"
#include "messageServer.h"
#include "SensorManager.h"
#include "pump.h"

enum class ControllerState
{
    OFF,
    ON
};

// Controller Settings
// TODO: Replace these ASAP
typedef struct
{
    float setpoint;
    float P_gain;
    float I_gain;
    float D_gain;
    float LPFCutoff;
} ctrlParams_t;

typedef struct
{
    int fanState;
    int flush;
    int elementLow;
    int elementHigh;
    int prodCondensor;
} ctrlSettings_t;

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

public:
    ControlSettings(void) = default;
    ControlSettings(bool prodCondensorPump, bool refluxCondensorPump)
        : MessageBase(ControlSettings::messageType, ControlSettings::Name, esp_timer_get_time()), 
        _prodCondensorPump(prodCondensorPump), _refluxCondensorPump(refluxCondensorPump) {}
    ControlSettings(const ControlSettings &other)
        : MessageBase(ControlSettings::messageType, ControlSettings::Name, esp_timer_get_time()), 
        _prodCondensorPump(other._prodCondensorPump), _refluxCondensorPump(other._refluxCondensorPump) {}

    // TODO: Remove getters and make members public
    bool getProdCondensorPump(void) const { return _prodCondensorPump; }
    bool getRefluxCondensorPump(void) const { return _refluxCondensorPump; }

private:
    bool _prodCondensorPump = false;
    bool _refluxCondensorPump = false;
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
    ControlSettings ctrlSettings{};
    PumpConfig refluxPumpConfig{};
    PumpConfig prodPumpConfig{};
    gpio_num_t fanPin = (gpio_num_t)GPIO_NUM_NC;
    gpio_num_t element1Pin = (gpio_num_t)GPIO_NUM_NC;
    gpio_num_t element2Pin = (gpio_num_t)GPIO_NUM_NC;
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
    ControlCommand(ControllerState fanState, ControllerState LPElementState, ControllerState HPElementState)
        : MessageBase(ControlCommand::messageType, ControlCommand::Name, esp_timer_get_time()),
          fanState(fanState), LPElementState(LPElementState), HPElementState(HPElementState) {}
    ControlCommand(const ControlCommand& other)
        : MessageBase(ControlCommand::messageType, ControlCommand::Name, esp_timer_get_time()), fanState(other.fanState),
          LPElementState(other.LPElementState), HPElementState(other.HPElementState) {}

    PBRet serialize(std::string &JSONStr) const;
    PBRet deserialize(const cJSON *root);

    ControllerState fanState = ControllerState::OFF;
    ControllerState LPElementState = ControllerState::OFF;
    ControllerState HPElementState = ControllerState::OFF;
};

class Controller : public Task
{
    static constexpr const char *Name = "Controller";
    static constexpr const char *FSBasePath = "/spiffs";
    static constexpr const char *FSPartitionLabel = "PBData";
    static constexpr const char *ctrlTuningFile = "/spiffs/ctrlTuning.json";
    static constexpr double HYSTERESIS_BOUND_UPPER = 60;
    static constexpr double HYSTERESIS_BOUND_LOWER = 58;

public:
    // Constructors
    Controller(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const ControllerConfig &cfg);

    // Update methods
    void updateComponents();

    // Utility
    static PBRet checkInputs(const ControllerConfig &cfg);
    static PBRet loadFromJSON(ControllerConfig &cfg, const cJSON *cfgRoot);
    bool isConfigured(void) const { return _configured; }

    friend class ControllerUT;

private:
    // Initialization
    PBRet _initIO(const ControllerConfig &cfg) const;
    PBRet _initPumps(const PumpConfig &refluxPumpConfig, const PumpConfig &prodPumpConfig);

    // Updates
    PBRet _doControl(double temp);
    PBRet _updatePeripheralState(const ControlCommand &cmd);
    PBRet _handleProductPump(double temp);

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

    Pump _refluxPump{};
    Pump _prodPump{};
    bool _configured = false;

    double _prevError = 0.0;
    double _integral = 0.0;
    double _derivative = 0.0;
    double _prevTemp = 0.0;
};

#endif // MAIN_CONTROLLER_H