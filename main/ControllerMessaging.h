#ifndef MAIN_CONTROLLER_MESSAGING_H
#define MAIN_CONTROLLER_MESSAGING_H

#include "PBCommon.h"
#include "messageServer.h"
#include "pump.h"
#include "cJSON.h"

// TODO: Should this be in Controller.h?
enum class ComponentState
{
    OFF,
    ON
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
    static constexpr const char *UptimeStr = "Uptime";

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

#endif // MAIN_CONTROLLER_MESSAGING_H