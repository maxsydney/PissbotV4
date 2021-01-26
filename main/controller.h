#ifndef MAIN_CONTROLLER_H 
#define MAIN_CONTROLLER_H

#include "PBCommon.h"
#include "CppTask.h"
#include "messageServer.h"
#include "SensorManager.h"
#include "pump.h"

enum class ControllerState { OFF, ON };

// Controller Settings
// TODO: Replace these ASAP
typedef struct { 
    float setpoint;
    float P_gain;
    float I_gain;
    float D_gain;
    float LPFCutoff;
} ctrlParams_t;

typedef struct {
    int fanState;
    int flush;
    int elementLow;
    int elementHigh;
    int prodCondensor;
} ctrlSettings_t;

enum class ControllerDataRequestType { None, Tuning, Settings };

class ControlSettings : public MessageBase
{
    static constexpr MessageType messageType = MessageType::ControlSettings;
    static constexpr const char* Name = "Controller settings";

    public:
        ControlSettings(void) = default;
        ControlSettings(bool fanState, bool elementLow, bool elementHigh, 
                        bool prodCondensorPump, bool refluxCondensorPump)
            : MessageBase(ControlSettings::messageType, ControlSettings::Name), _fanState(fanState), 
              _elementLow(elementLow), _elementHigh(elementHigh), _prodCondensorPump(prodCondensorPump), 
              _refluxCondensorPump(refluxCondensorPump) {}

        bool getFanState(void) const { return _fanState; }
        bool getElementLow(void) const { return _elementLow; }
        bool getElementHigh(void) const { return _elementHigh; }
        bool getProdCondensorPump(void) const { return _prodCondensorPump; }
        bool getRefluxCondensorPump(void) const { return _refluxCondensorPump; }

    private:

        bool _fanState = false;
        bool _elementLow = false;
        bool _elementHigh = false;
        bool _prodCondensorPump = false;
        bool _refluxCondensorPump = false;
};

class ControlTuning : public MessageBase
{
    static constexpr MessageType messageType = MessageType::ControlTuning;
    static constexpr const char* Name = "Controller tuning";

    public:
        ControlTuning(void) = default;
        ControlTuning(double setpoint, double PGain, double IGain, double DGain, double LPFCutoff)
            : MessageBase(ControlTuning::messageType, ControlTuning::Name), _setpoint(setpoint),
              _PGain(PGain), _IGain(IGain), _DGain(DGain), _LPFCutoff(LPFCutoff) {}

        double getSetpoint(void) const { return _setpoint; }
        double getPGain(void) const { return _PGain; }
        double getIGain(void) const { return _IGain; }
        double getDGain(void) const { return _DGain; }
        double getLPFCutoff(void) const { return _LPFCutoff; }

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
    ControlTuning ctrlTuning {};
    ControlSettings ctrlSettings {};
    PumpConfig refluxPumpConfig {};
    PumpConfig prodPumpConfig {};
    gpio_num_t fanPin = (gpio_num_t) GPIO_NUM_NC;
    gpio_num_t element1Pin = (gpio_num_t) GPIO_NUM_NC;
    gpio_num_t element2Pin = (gpio_num_t) GPIO_NUM_NC;
};

class ControllerDataRequest : public MessageBase
{
    static constexpr MessageType messageType = MessageType::ControllerDataRequest;
    static constexpr const char* Name = "Controller data request";

    public:
        ControllerDataRequest(void) = default;
        ControllerDataRequest(ControllerDataRequestType requestType)
            : MessageBase(ControllerDataRequest::messageType, ControllerDataRequest::Name),
              _requestType(requestType) {}

        ControllerDataRequestType getType(void) const { return _requestType; }

    private:

        ControllerDataRequestType _requestType = ControllerDataRequestType::None;
};

class ControlCommand : public MessageBase
{
    static constexpr MessageType messageType = MessageType::ControlCommand;
    static constexpr const char* Name = "Controller command";

    public:
        ControlCommand(void) = default;
        ControlCommand(ControllerState fanState, ControllerState LPElementState, ControllerState HPElementState)
            : MessageBase(ControlCommand::messageType, ControlCommand::Name),
              _fanState(fanState), _LPElementState(LPElementState), _HPElementState(HPElementState) {}

        ControllerState getFanState(void) const { return _fanState; }
        ControllerState getLPElementState(void) const { return _LPElementState; }
        ControllerState getHPElementState(void) const { return _HPElementState; }

    private:
        ControllerState _fanState = ControllerState::OFF;
        ControllerState _LPElementState = ControllerState::OFF;
        ControllerState _HPElementState = ControllerState::OFF;
};

class Controller: public Task
{
    static constexpr const char* Name = "Controller";

    public:
        // Constructors
        Controller(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const ControllerConfig& cfg);

        // Update methods
        void updateComponents();

        // Utility
        static PBRet checkInputs(const ControllerConfig& cfg);
        bool isConfigured(void) const { return _configured; }

    private:
        // Initialization
        PBRet _initIO(const ControllerConfig& cfg) const;
        PBRet _initPumps(const PumpConfig& refluxPumpConfig, const PumpConfig& prodPumpConfig);

        // Updates
        PBRet _doControl(double temp);
        PBRet _updateAuxOutputs(const ControlCommand& cmd);
        PBRet _handleProductPump(double temp);

        // Setup methods
        PBRet _initFromParams(const ControllerConfig& cfg);
        PBRet _setupCBTable(void) override;

        // FreeRTOS hook method
        void taskMain(void) override;

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
        
        // Controller data
        ControllerConfig _cfg {};
        ControlCommand _outputState {};
        TemperatureData _currentTemp {};

        Pump _refluxPump {};
        Pump _prodPump {};
        bool _configured = false;
        
        double _prevError = 0.0;
        double _integral = 0.0;
        double _derivative = 0.0;
        double _prevTemp = 0.0;
};

#endif // MAIN_CONTROLLER_H