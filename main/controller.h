#ifndef MAIN_CONTROLLER_H 
#define MAIN_CONTROLLER_H

#include "PBCommon.h"
#include "CppTask.h"
#include "MessageDefs.h"
#include "pump.h"
#include "messages.h"

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
        PBRet _doControl(double temp);
        PBRet _initIO(const ControllerConfig& cfg) const;
        PBRet _handleProductPump(double temp);
        PBRet _initPumps(const PumpCfg& refluxPumpCfg, const PumpCfg& prodPumpCfg);
        PBRet _updateSettings(ctrlSettings_t ctrlSettings);

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

        ControllerConfig _cfg {};
        ctrlParams_t _ctrlParams {};
        ctrlSettings_t _ctrlSettings {};
        Pump _refluxPump {};
        Pump _prodPump {};
        bool _configured = false;
        
        double _prevError = 0.0;
        double _integral = 0.0;
        double _derivative = 0.0;
        double _prevTemp = 0.0;
};

#endif // MAIN_CONTROLLER_H