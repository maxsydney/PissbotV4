#include "Filter.h"
#include "Utilities.h"

#include <numeric>
#include <algorithm>

Filter::Filter(const FilterConfig& config)
{
    // Initialize filter
    if (_initFromConfig(config) == PBRet::SUCCESS) {
        ESP_LOGI(Filter::Name, "Filter configured!");
        _configured = true;
    } else {
        ESP_LOGW(Filter::Name, "Unable to configure filter");
    }
}

PBRet Filter::filter(double val, double& output)
{
    if (_configured == false) {
        ESP_LOGE(Filter::Name, "Filter was not configured");
        return PBRet::FAILURE;
    }

    if (Utilities::check(val) == false) {
        ESP_LOGE(Filter::Name, "Input value was invalid");
        return PBRet::FAILURE;
    }

    // Rotate inputs and insert
    std::rotate(_inputs.rbegin(), _inputs.rbegin() + 1, _inputs.rend());
    _inputs.front() = val;

    // Compute filter
    const double evalNum = std::inner_product(_inputs.begin(), _inputs.end(), _num.begin(), 0.0);
    const double evalDen = std::inner_product(_outputs.begin(), _outputs.end(), _den.begin(), 0.0);
    const double yn = evalNum - evalDen;

    // Insert filtered value into outputs
    if (_outputs.size() > 0u) {
        std::rotate(_outputs.rbegin(), _outputs.rbegin() + 1, _outputs.rend());
        _outputs.front() = yn;
    }

    output = yn;
    return PBRet::SUCCESS;
}

PBRet Filter::_initFromConfig(const FilterConfig& config)
{
    if (checkInputs(config) != PBRet::SUCCESS) {
        ESP_LOGE(Filter::Name, "Unable to configure filter");
        return PBRet::FAILURE;
    }

    // Store filter coeffients
    _num = config.num;
    _den = config.den;

    // Initialize input and output vectors
    _inputs = std::vector<double> (_num.size(), 0.0);
    _outputs = std::vector<double> (_den.size(), 0.0);

    return PBRet::SUCCESS;
}

PBRet Filter::checkInputs(const FilterConfig& config)
{
    esp_err_t err = 0;

    if (Utilities::check(config.num) == false) {
        ESP_LOGE(Filter::Name, "Invalid coefficient in numerator array");
        err |= ESP_FAIL;
    }

    if ((config.num.size() < MIN_NUM_LEN) || (config.num.size() > MAX_NUM_LEN)) {
        ESP_LOGE(Filter::Name, "Invalid number of numerator coefficients (got %zu, expected (%zu, %zu))", config.num.size(), MIN_NUM_LEN, MAX_NUM_LEN);
        err |= ESP_FAIL;
    }

    if (Utilities::check(config.den) == false) {
        ESP_LOGE(Filter::Name, "Invalid coefficient in denominator array");
        err |= ESP_FAIL;
    }

    if (config.den.size() > MAX_DEN_LEN) {
        ESP_LOGE(Filter::Name, "Invalid number of denominator coefficients (got %zu, expected (%zu, %zu))", config.den.size(), MIN_DEN_LEN, MAX_DEN_LEN);
        err |= ESP_FAIL;
    }

    return err == ESP_OK ? PBRet::SUCCESS : PBRet::FAILURE;
}