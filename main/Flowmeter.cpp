#include "Flowmeter.h"
#include "driver/gpio.h"

void IRAM_ATTR Flowmeter::flowmeterISR(void* arg)
{
    // Increment the pulse counter on the appropriate flowmeter object
    Flowmeter* flowmeter = static_cast<Flowmeter*>(arg);
    flowmeter->incrementFreqCounter();
}

Flowmeter::Flowmeter(const FlowmeterConfig& cfg)
{
    if (_initFromParams(cfg) != PBRet::FAILURE) {
        ESP_LOGI(Flowmeter::Name, "Flowmeter configured on pin %d", cfg.flowmeterPin);
    } else {
        ESP_LOGW(Flowmeter::Name, "Flowmeter was not configured on pin %d", cfg.flowmeterPin);
    }
}

PBRet Flowmeter::_initFromParams(const FlowmeterConfig& cfg)
{
    // Check inputs are valid
    if (checkInputs(cfg) == PBRet::FAILURE) {
        return PBRet::FAILURE;
    }

    // Initialize flowmeter GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << cfg.flowmeterPin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE
    };

    if (gpio_config(&io_conf) != ESP_OK) {
        ESP_LOGE(Flowmeter::Name, "Failed to configure flowmeter GPIO");
        return PBRet::FAILURE;
    }

    const esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK) {
        if (err != ESP_ERR_INVALID_STATE) {
            // TODO: Tidy this conditional up
            ESP_LOGE(Flowmeter::Name, "Failed to install ISR service");
            return PBRet::FAILURE;
        }
    }

    if (gpio_isr_handler_add(cfg.flowmeterPin, Flowmeter::flowmeterISR, static_cast<void*>(this)) != ESP_OK) {
        ESP_LOGE(Flowmeter::Name, "Failed to add flowmeter ISR");
        return PBRet::FAILURE;
    }

    _cfg = cfg;
    _configured = true;
    return PBRet::SUCCESS;
}

PBRet Flowmeter::readFlowrate(int64_t t, double& flowrate)
{
    // Compute the flowrate from accumulated pulses
    // TODO: Check for going back in time and -ve pulses
    const double dt = (t - _lastUpdateTime) * 1e-6;      // Convert to seconds
    flowrate = _freqCounter * _cfg.kFactor / dt;        // Compute flowrate
    _lastUpdateTime = t;               
    _freqCounter = 0;                                   // Reset counter

    ESP_LOGI(Flowmeter::Name, "Pulses: %d - Dt: %f - K Factor: %f", _freqCounter, dt, _cfg.kFactor);

    return PBRet::SUCCESS;
}

void Flowmeter::incrementFreqCounter(void) volatile
{
    _freqCounter++;
}

PBRet Flowmeter::checkInputs(const FlowmeterConfig& cfg)
{
    // Check flowmeter pin is valid GPIO
    if ((cfg.flowmeterPin <= GPIO_NUM_NC) || (cfg.flowmeterPin > GPIO_NUM_MAX)) {
        ESP_LOGE(Flowmeter::Name, "Flowmeter pin %d is invalid", cfg.flowmeterPin);
        return PBRet::FAILURE;
    }

    // K-factor should be positive
    if (cfg.kFactor <= 0) {
        ESP_LOGE(Flowmeter::Name, "K factor must be greater than 0");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet Flowmeter::loadFromJSON(FlowmeterConfig& cfg, const cJSON* cfgRoot)
{
    // Load FlowmeterConfig from JSON
    if (cfgRoot == nullptr) {
        ESP_LOGW(Flowmeter::Name, "cfg was null");
        return PBRet::FAILURE;
    }

    // Get flowmeter GPIO
    cJSON* flowMeterGPIONode = cJSON_GetObjectItem(cfgRoot, "GPIO");
    if (cJSON_IsNumber(flowMeterGPIONode)) {
        cfg.flowmeterPin = static_cast<gpio_num_t>(flowMeterGPIONode->valueint);
    } else {
        ESP_LOGI(Flowmeter::Name, "Unable to read flowmeter pin from JSON");
        return PBRet::FAILURE;
    }

    // Get flowmeter K-factor
    cJSON* kFactorNode = cJSON_GetObjectItem(cfgRoot, "kFactor");
    if (cJSON_IsNumber(kFactorNode)) {
        cfg.kFactor = static_cast<gpio_num_t>(kFactorNode->valuedouble);
    } else {
        ESP_LOGI(Flowmeter::Name, "Unable to read flowmeter K-factor from JSON");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}