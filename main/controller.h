#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "controlLoop.h"
#include "pump.h"
#include "messages.h"

constexpr uint16_t PUMP_MIN_OUTPUT = 25;
constexpr uint16_t PUMP_MAX_OUTPUT = 512;
constexpr uint16_t FLUSH_SPEED = 512;

class Controller
{
    public:
        Controller(uint8_t freq, ctrlParams_t ctrlParams, gpio_num_t P1_pin, ledc_channel_t P1_channel, 
                   ledc_timer_t timerChannel1, gpio_num_t P2_pin, ledc_channel_t P2_channel, 
                   ledc_timer_t timerChannel2, gpio_num_t fanPin, gpio_num_t elem24Pin, 
                   gpio_num_t elem3Pin);
        Controller();

        void updatePumpSpeed(double temp);
        void updateComponents();
        void setControllerSettings(ctrlSettings_t ctrlSettings);

        // Setters
        void setFanState(bool state) {_fanState = state;};
        void setRefluxSpeed(uint16_t speed) {_refluxPump.setSpeed(speed);};
        void setProductSpeed(uint16_t speed) {_prodPump.setSpeed(speed);};
        void setElem24State(bool state) {_elementState_24 = state;};
        void setElem3State(bool state) {_elementState_3 = state;};
        void setSetPoint(double sp) {_ctrlParams.setpoint = sp;};
        void setPGain(float P) {_ctrlParams.P_gain = P;};
        void setIGain(float I) {_ctrlParams.I_gain = I;};
        void setDGain(float D) {_ctrlParams.D_gain = D;};
        void setControllerParams(ctrlParams_t ctrlParams) {_ctrlParams = ctrlParams;};
        void setRefluxPumpMode(pumpMode_t mode) {_refluxPump.setMode(mode);};
        void setProductPumpMode(pumpMode_t mode) {_prodPump.setMode(mode);};

        // Getters
        bool getFanState() const {return _fanState;};
        uint16_t getRefluxSpeed() const {return _refluxPump.getSpeed();}
        uint16_t getProductSpeed() const {return _prodPump.getSpeed();}
        bool getElem24State() const {return _elementState_24;};
        bool getElem3State() const {return _elementState_3;};
        bool getFlush() const {return _flush;};
        bool getProdManual() const {return _prodManual;};
        double getSetpoint() const {return _ctrlParams.setpoint;};
        double getPGain() const {return _ctrlParams.P_gain;};
        double getIGain() const {return _ctrlParams.I_gain;};
        double getDGain() const {return _ctrlParams.D_gain;};
        ctrlParams_t getControllerParams() const {return _ctrlParams;};
        pumpMode_t getRefluxPumpMode() const {return _refluxPump.getMode();};
        pumpMode_t getProductPumpMode() const {return _prodPump.getMode();};

    private:
        void _initComponents() const;
        void _handleProductPump(double temp);
        void _initPumps(gpio_num_t P1_pin, ledc_channel_t P1_channel, ledc_timer_t timerChannel1, 
                        gpio_num_t P2_pin, ledc_channel_t P2_channel, ledc_timer_t timerChannel2);
    
        uint8_t _updateFreq;
        float _updatePeriod;
        ctrlParams_t _ctrlParams;
        Pump _refluxPump;
        Pump _prodPump;
        bool _fanState;
        bool _flush;
        bool _prodManual;
        gpio_num_t _fanPin;
        bool _elementState_24;
        gpio_num_t _elem24Pin;
        bool _elementState_3; 
        gpio_num_t _elem3Pin;
        double _prevError;
        double _integral;
};

#ifdef __cplusplus
}
#endif