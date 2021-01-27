#include <driver/ledc.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_log.h>
#include "pinDefs.h"
#include "pump.h"
#include "controller.h"
#include "Utilities.h"
#include <stdint.h>

Pump::Pump(const PumpConfig& cfg)
{
    if (_initFromParams(cfg) != PBRet::FAILURE) {
        ESP_LOGI(Pump::Name, "Pump configured on channel %d", _cfg.PWMChannel);
    } else {
        ESP_LOGW(Pump::Name, "Pump was not configured on channel %d", _cfg.PWMChannel);
    }
}

PBRet Pump::_initFromParams(const PumpConfig& cfg)
{
    // Check inputs are valid
    if (checkInputs(cfg) == PBRet::FAILURE) {
        return PBRet::FAILURE;
    }

    // Configure timer for PWM drivers
    // TODO: Move this to JSON config
    const ledc_timer_config_t PWM_timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_9_BIT,
        .timer_num = cfg.timerChannel,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };

    // Configure pump channels
    const ledc_channel_config_t channelConfig = {
        .gpio_num   = cfg.pumpGPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel    = cfg.PWMChannel,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = cfg.timerChannel,
        .duty       = 0,
        .hpoint     = 0xff
    };

    esp_err_t err = ledc_timer_config(&PWM_timer);
    if (err == ESP_ERR_INVALID_ARG) {
        ESP_LOGE(Pump::Name, "Invalid parameter passed to timer configuration. Unable to configure pump on channel %d", cfg.PWMChannel);
        return PBRet::FAILURE;
    } else if (err == ESP_FAIL) {
        ESP_LOGE(Pump::Name, "Can not find a proper pre-divider number base on the given frequency and the current duty_resolution. Pump configured on channel %d", cfg.PWMChannel);
        return PBRet::FAILURE;
    }

    err = ledc_channel_config(&channelConfig);
    if (err != ESP_OK) {
        ESP_LOGW(Pump::Name, "Invalid argument supplied to channel config. Unable to configure pump on channel %d", cfg.PWMChannel);
        return PBRet::FAILURE;
    }

    // Success by here
    _cfg = cfg;
    _configured = true;
    return PBRet::SUCCESS;
}

PBRet Pump::_updatePump(double pumpSpeed, PumpMode pumpMode)
{
    if (isConfigured() == false) {
        ESP_LOGW(Pump::Name, "Pump was not configured");
        return PBRet::FAILURE;
    }

    if (_setSpeed(pumpSpeed, pumpMode) == PBRet::FAILURE) {
        ESP_LOGW(Pump::Name, "Failed to update pump speed");
        return PBRet::FAILURE;
    }

    return _drivePump();
}

PBRet Pump::_drivePump(void) const
{
    const uint32_t pumpSpeed = getPumpSpeed();

    if (ledc_set_duty(LEDC_HIGH_SPEED_MODE, _cfg.PWMChannel, pumpSpeed) != ESP_OK) {
        ESP_LOGW(Pump::Name, "ledc_set_duty failed with invalid parameter error. Likely pumpSpeed (%d)", pumpSpeed);
        return PBRet::FAILURE;
    }
    
    if (ledc_update_duty(LEDC_HIGH_SPEED_MODE, _cfg.PWMChannel) != ESP_OK) {
        ESP_LOGW(Pump::Name, "ledc_update_duty failed with invalid parameter error");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

uint32_t Pump::getPumpSpeed(void) const
{
    // Return the current speed of the pump

    if (isConfigured() == false) {
        ESP_LOGW(Pump::Name, "Pump was not configured. Can not get pump speed");
        return 0;
    }

    switch (_pumpMode)
    {
        case (PumpMode::ActiveControl):
        {
            return _pumpSpeedActive;
        }
        case (PumpMode::ManualControl):
        {
            return _pumpSpeedManual;
        }
        case (PumpMode::Off):
        {
            return 0;
        }
        default:
        {
            ESP_LOGW(Pump::Name, "Unknown PumpMode");
            return 0;
        }
    }
}

PBRet Pump::_setSpeed(int16_t pumpSpeed, PumpMode pumpMode)
{
    // Sets the current pump drive speed for the associated mode.
    // The pumps physical speed will only change if the current
    // pump mode matches the mode passed here. If not, the speed
    // will be updated to the input speed when the pump is switched
    // back into the input mode

    // Saturate output command
    pumpSpeed = Utilities::bound(pumpSpeed, Pump::PUMP_MIN_OUTPUT, Pump::PUMP_MAX_OUTPUT);
    
    // Set the appropriate pump speed
    switch (_pumpMode)
    {
        case (PumpMode::ActiveControl):
        {
            _pumpSpeedActive = pumpSpeed;
            break;
        }
        case (PumpMode::ManualControl):
        {
            _pumpSpeedManual = pumpSpeed;
            break;
        }
        case (PumpMode::Off):
        {
            ESP_LOGW(Pump::Name, "Cannot update pump speed for PumpMode Off");
            return PBRet::FAILURE;
        }
        default:
        {
            ESP_LOGW(Pump::Name, "Unknown PumpMode");
            return PBRet::FAILURE;
        }
    }

    return PBRet::SUCCESS;
}

PBRet Pump::checkInputs(const PumpConfig& cfg)
{
    // Check output pin is valid GPIO
    if ((cfg.pumpGPIO <= GPIO_NUM_NC) || (cfg.pumpGPIO > GPIO_NUM_MAX)) {
        ESP_LOGE(Pump::Name, "PUMP GPIO %d is invalid", cfg.pumpGPIO);
        return PBRet::FAILURE;
    }

    // Check PWM channel is valid
    if ((cfg.PWMChannel < LEDC_CHANNEL_0) || (cfg.PWMChannel > LEDC_CHANNEL_MAX)) {
        ESP_LOGE(Pump::Name, "PUMP PWM channel %d is invalid", cfg.PWMChannel);
        return PBRet::FAILURE;
    }

    // Check timer channel is valid
    if ((cfg.timerChannel < LEDC_TIMER_0) || (cfg.timerChannel > LEDC_TIMER_MAX)) {
        ESP_LOGE(Pump::Name, "PUMP timer channel %d is invalid", cfg.timerChannel);
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet Pump::loadFromJSON(PumpConfig& cfg, const cJSON* cfgRoot)
{
    // Load PumpConfig struct from JSON

    if (cfgRoot == nullptr) {
        ESP_LOGW(Pump::Name, "cfg was null");
        return PBRet::FAILURE;
    }

    // Get pump GPIO
    cJSON* GPIOPumpeNode = cJSON_GetObjectItem(cfgRoot, "GPIO");
    if (cJSON_IsNumber(GPIOPumpeNode)) {
        cfg.pumpGPIO = static_cast<gpio_num_t>(GPIOPumpeNode->valueint);
    } else {
        ESP_LOGI(Pump::Name, "Unable to read pump GPIO from JSON");
        return PBRet::FAILURE;
    }

    // Get PWM channel
    cJSON* PWMNode = cJSON_GetObjectItem(cfgRoot, "PWMChannel");
    if (cJSON_IsNumber(PWMNode)) {
        cfg.PWMChannel = static_cast<ledc_channel_t>(PWMNode->valueint);
    } else {
        ESP_LOGI(Pump::Name, "Unable to read pump PWM channel from JSON");
        return PBRet::FAILURE;
    }

    // Get timer channel
    cJSON* timerNode = cJSON_GetObjectItem(cfgRoot, "timerChannel");
    if (cJSON_IsNumber(timerNode)) {
        cfg.timerChannel = static_cast<ledc_timer_t>(timerNode->valueint);
    } else {
        ESP_LOGI(Pump::Name, "Unable to read pump timer channel from JSON");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}