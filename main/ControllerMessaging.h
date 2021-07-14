#ifndef MAIN_CONTROLLER_MESSAGING_H
#define MAIN_CONTROLLER_MESSAGING_H

#include "PBCommon.h"
#include "MessageServer.h"
#include "Pump.h"
#include "Filter.h"
#include "cJSON.h"
#include "/Users/maxsydney/esp/PissbotV4/lib/PBProtoBuf/Generated/ControllerMessaging.h"

class ControlSettings : public MessageBase
{
    static constexpr PBMessageType messageType = PBMessageType::ControllerSettings;
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

    PumpMode refluxPumpMode = PumpMode::PUMP_OFF;
    PumpMode productPumpMode = PumpMode::PUMP_OFF;
    PumpSpeeds manualPumpSpeeds{};
};

class ControlTuning : public MessageBase
{
    static constexpr PBMessageType messageType = PBMessageType::ControllerTuning;
    static constexpr const char *Name = "Controller tuning";

    static constexpr const char *SetpointStr = "Setpoint";
    static constexpr const char *PGainStr = "PGain";
    static constexpr const char *IGainStr = "IGain";
    static constexpr const char *DGainStr = "DGain";
    static constexpr const char *LPFCutoffStr = "LPFCutoff";
    static constexpr const char *LPFSampleFreqStr = "LPFSampleFreq";

public:
    ControlTuning(void) = default;
    ControlTuning(double setpoint, double PGain, double IGain, double DGain, const IIRLowpassFilterConfig& derivFilterCfg)
        : MessageBase(ControlTuning::messageType, ControlTuning::Name, esp_timer_get_time()), setpoint(setpoint),
          PGain(PGain), IGain(IGain), DGain(DGain), derivFilterCfg(derivFilterCfg) {}
    ControlTuning(const ControlTuning &other)
        : MessageBase(ControlTuning::messageType, ControlTuning::Name, esp_timer_get_time()), setpoint(other.setpoint),
          PGain(other.PGain), IGain(other.IGain), DGain(other.DGain), derivFilterCfg(other.derivFilterCfg) {}

    PBRet serialize(std::string &JSONstr) const;
    PBRet deserialize(const cJSON *root);

    double setpoint = 0.0;
    double PGain = 0.0;
    double IGain = 0.0;
    double DGain = 0.0;
    IIRLowpassFilterConfig derivFilterCfg {};
};

class ControlCommand : public MessageBase
{
    static constexpr PBMessageType messageType = PBMessageType::ControllerCommand;
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

    ComponentState fanState = ComponentState::OFF_STATE;
    double LPElementDutyCycle = 0.0;
    double HPElementDutyCycle = 0.0;
};

class ControllerState : public MessageBase
{
    static constexpr PBMessageType messageType = PBMessageType::ControllerState;
    static constexpr const char *Name = "Controller State";

    public:
        // Constructors
        ControllerState(void)
            : MessageBase(ControllerState::messageType, ControllerState::Name, esp_timer_get_time()) {}

        PBRet serialize(std::string &JSONStr) const override;
        PBRet deserialize(const cJSON *root) override;

        ControllerStateMessage message {};
};

class ControllerDataRequest : public MessageBase
{
    static constexpr PBMessageType messageType = PBMessageType::ControllerDataRequest;
    static constexpr const char *Name = "Controller data request";

public:
    ControllerDataRequest(void) = default;
    ControllerDataRequest(ControllerDataRequestType requestType)
        : MessageBase(ControllerDataRequest::messageType, ControllerDataRequest::Name, esp_timer_get_time()),
          _requestType(requestType) {}

    ControllerDataRequestType getType(void) const { return _requestType; }

    PBRet serialize(std::string &JSONStr) const override;
    PBRet deserialize(const cJSON *root) override;

private:
    ControllerDataRequestType _requestType = ControllerDataRequestType::NONE;
};

#endif // MAIN_CONTROLLER_MESSAGING_H