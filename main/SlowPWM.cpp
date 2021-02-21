#include "SlowPWM.h"
#include <cmath>

SlowPWM::SlowPWM(const SlowPWMConfig& cfg)
{
    // Initialize controller
    if (_initFromParams(cfg) == PBRet::SUCCESS) {
        ESP_LOGI(SlowPWM::Name, "SlowPWM configured!");
        _configured = true;
    } else {
        ESP_LOGW(SlowPWM::Name, "Unable to configure SlowPWM");
    }
}

PBRet SlowPWM::update(int64_t currTime)
{
    // Simple timer compare to determine output state
    double cycleTime = (currTime - _lastCycle) * 1e-6;
    if (cycleTime >= _pwmDt) {
        cycleTime = fmod(cycleTime, _pwmDt);
        _lastCycle = (std::floor(currTime*1e-6 / _pwmDt) * _pwmDt) * 1e6;        // Set last cycle time to latest increment of _pwmDt in us
    }

    const double thresh = _dutyCycle * _pwmDt;
    if (cycleTime <= thresh) {
        _outputState = 1;
    } else {
        _outputState = 0;
    }

    return PBRet::SUCCESS;
}

PBRet SlowPWM::setDutyCycle(double dutyCycle)
{
    if ((dutyCycle < 0.0) || (dutyCycle > 1.0)) {
        ESP_LOGE(SlowPWM::Name, "Duty cycle (%.1f) was outside the valid range (0.0, 1.0)", dutyCycle);
        return PBRet::FAILURE;
    }

    _dutyCycle = dutyCycle;
    return PBRet::SUCCESS;
}

PBRet SlowPWM::checkInputs(const SlowPWMConfig& cfg)
{
    // Check PWM frequency is within bounds
    if ((cfg.PWMFreq < SlowPWMConfig::MIN_PWM_FREQ) || (cfg.PWMFreq > SlowPWMConfig::MAX_PWM_FREQ)) {
        ESP_LOGE(SlowPWM::Name, "PWM frequency (%.2f) was outside of the allowable bounds (%.2f, %.2f)", cfg.PWMFreq, SlowPWMConfig::MIN_PWM_FREQ, SlowPWMConfig::MAX_PWM_FREQ);
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet SlowPWM::loadFromJSON(SlowPWMConfig &cfg, const cJSON *cfgRoot)
{
    // Load SlowPWMConfig from JSON

    if (cfgRoot == nullptr) {
        ESP_LOGW(SlowPWM::Name, "cfgRoot was null");
        return PBRet::FAILURE;
    }

    // Get PWM freq
    cJSON* PWMFreqNode = cJSON_GetObjectItem(cfgRoot, "PWMFreq");
    if (cJSON_IsNumber(PWMFreqNode)) {
        cfg.PWMFreq = PWMFreqNode->valuedouble;
    } else {
        ESP_LOGI(SlowPWM::Name, "Unable to read PWM frequency from JSON");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet SlowPWM::_initFromParams(const SlowPWMConfig &cfg)
{
    if (SlowPWM::checkInputs(cfg) != PBRet::SUCCESS) {;
        ESP_LOGW(SlowPWM::Name, "Unable to configure SlowPWM");
        return PBRet::FAILURE;
    }

    _cfg = cfg;
    _pwmDt = 1.0 / _cfg.PWMFreq;

    return PBRet::SUCCESS;
}