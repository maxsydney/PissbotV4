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
        Controller(uint8_t freq, ctrlParams_t ctrlParams, ctrlSettings_t ctrlSettings, gpio_num_t P1_pin, 
            ledc_channel_t P1_channel, ledc_timer_t timerChannel1, gpio_num_t P2_pin, ledc_channel_t P2_channel, 
            ledc_timer_t timerChannel2, gpio_num_t fanPin, gpio_num_t elem24Pin, gpio_num_t elem3Pin);
        Controller();

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
        void _initPumps(gpio_num_t P1_pin, ledc_channel_t P1_channel, ledc_timer_t timerChannel1, 
                        gpio_num_t P2_pin, ledc_channel_t P2_channel, ledc_timer_t timerChannel2);
    
        uint8_t _updateFreq;
        float _updatePeriod;
        ctrlParams_t _ctrlParams;
        ctrlSettings_t _ctrlSettings;
        Pump _refluxPump;
        Pump _prodPump;
        gpio_num_t _fanPin;
        gpio_num_t _elem24Pin;
        gpio_num_t _elem3Pin;
        double _prevError;
        double _integral;
};

#ifdef __cplusplus
}
#endif