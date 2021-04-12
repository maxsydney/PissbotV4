#include <driver/ledc.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_log.h>
#include "pump.h"
#include "Controller.h"
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
        .duty_resolution = LEDC_TIMER_10_BIT,
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

PBRet Pump::_drivePump(uint32_t pumpSpeed) const
{
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

    return _pumpSpeed;
}

PBRet Pump::updatePumpSpeed(uint32_t pumpSpeed)
{
    // Update the current speed of the pump

    if (isConfigured() == false) {
        ESP_LOGW(Pump::Name, "Pump was not configured");
        return PBRet::FAILURE;
    }

    // Saturate output command
    _pumpSpeed = Utilities::bound(pumpSpeed, Pump::PUMP_OFF, Pump::PUMP_MAX_SPEED);
    return _drivePump(_pumpSpeed);
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
    cJSON* GPIOPumpNode = cJSON_GetObjectItem(cfgRoot, "GPIO");
    if (cJSON_IsNumber(GPIOPumpNode)) {
        cfg.pumpGPIO = static_cast<gpio_num_t>(GPIOPumpNode->valueint);
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