#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "controlLoop.h"
#include "pump.h"
#include "messages.h"

class Controller
{
    public:
        Controller() = default;
        Controller(uint8_t freq, ctrlParams_t params, ctrlSettings_t settings, const PumpCfg& refluxPumpCfg, 
                   const PumpCfg& prodPumpCfg, gpio_num_t fanPin, gpio_num_t element1Pin, gpio_num_t element2Pin);

        void updatePumpSpeed(double temp);
        void updateComponents();

        // Setters
        void setRefluxSpeed(uint16_t speed) {_refluxPump.setSpeed(speed);}
        void setProductSpeed(uint16_t speed) {_prodPump.setSpeed(speed);}
        void setSetPoint(double sp) {_ctrlParams.setpoint = sp;}
        void setPGain(float P) {_ctrlParams.P_gain = P;}
        void setIGain(float I) {_ctrlParams.I_gain = I;}
        void setDGain(float D) {_ctrlParams.D_gain = D;}
        void setControllerParams(ctrlParams_t ctrlParams) {_ctrlParams = ctrlParams;}
        void setControllerSettings(ctrlSettings_t ctrlSettings);
        void setRefluxPumpMode(PumpMode mode) {_refluxPump.setMode(mode);}
        void setProductPumpMode(PumpMode mode) {_prodPump.setMode(mode);}

        // Getters
        double getSetpoint(void) const {return _ctrlParams.setpoint;}
        double getPGain(void) const {return _ctrlParams.P_gain;}
        double getIGain(void) const {return _ctrlParams.I_gain;}
        double getDGain(void) const {return _ctrlParams.D_gain;}
        const Pump& getRefluxPump(void) const { return _refluxPump; }
        const Pump& getProductPump(void) const { return _prodPump; }
        const ctrlParams_t& getControllerParams(void) const {return _ctrlParams;}
        const ctrlSettings_t& getControllerSettings(void) const {return _ctrlSettings;}
        const PumpMode& getRefluxPumpMode(void) const {return _refluxPump.getMode();}
        const PumpMode& getProductPumpMode(void) const {return _prodPump.getMode();}
        uint16_t getRefluxPumpSpeed(void) const {return _refluxPump.getSpeed();}
        uint16_t getProductPumpSpeed(void) const {return _prodPump.getSpeed();}

    private:
        void _initComponents(void) const;
        void _handleProductPump(double temp);
        void _initPumps(const PumpCfg& refluxPumpCfg, const PumpCfg& prodPumpCfg);
    
        uint8_t _updateFreq = 0;
        float _updatePeriod = 0.0;
        ctrlParams_t _ctrlParams = {};
        ctrlSettings_t _ctrlSettings = {};
        Pump _refluxPump = {};
        Pump _prodPump = {};
        gpio_num_t _fanPin = GPIO_NUM_0;
        gpio_num_t _element1 = GPIO_NUM_0;
        gpio_num_t _element2 = GPIO_NUM_0;
        
        double _prevError = 0.0;
        double _integral = 0.0;
        double _derivative = 0.0;
        double _prevTemp = 0.0;
};

#ifdef __cplusplus
}
#endif