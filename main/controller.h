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


struct ControllerConfig
{
    double dt = 0.0;
    ctrlParams_t ctrlTuningParams {};
    PumpCfg refluxPumpCfg = {};
    PumpCfg prodPumpCfg = {};
    gpio_num_t fanPin = GPIO_NUM_NC;
    gpio_num_t element1Pin = GPIO_NUM_NC;
    gpio_num_t element2Pin = GPIO_NUM_NC;
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

        void updateComponents();

        static PBRet checkInputs(const ControllerConfig& cfg);

        bool isConfigured(void) const { return _configured; }

    private:
        // Initialization
        PBRet _initIO(const ControllerConfig& cfg) const;
        PBRet _initPumps(const PumpCfg& refluxPumpCfg, const PumpCfg& prodPumpCfg);

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

        // Controller data
        ControllerConfig _cfg {};
        ctrlParams_t _ctrlParams {};
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