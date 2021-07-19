#include "Filter.h"
#include "Utilities.h"

#include <numeric>
#include <algorithm>
#include <math.h>

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

IIRLowpassFilter::IIRLowpassFilter(const IIRLowpassFilterConfig& config)
{
    // Initialize filter
    if (_initFromConfig(config) == PBRet::SUCCESS) {
        ESP_LOGI(IIRLowpassFilter::Name, "Filter configured!");
        _configured = true;
    } else {
        ESP_LOGW(IIRLowpassFilter::Name, "Unable to configure filter");
    }
}

PBRet IIRLowpassFilter::filter(double val, double& output)
{
    if (_configured == false) {
        // ESP_LOGE(IIRLowpassFilter::Name, "IIRLowpassFilter object was not configured");
        return PBRet::FAILURE;
    }

    if (_filter.isConfigured() == false) {
        ESP_LOGE(IIRLowpassFilter::Name, "Base filter object was not configured");
        return PBRet::FAILURE;
    }

    return _filter.filter(val, output);
}

PBRet IIRLowpassFilter::setCutoffFreq(double Fc)
{
    // Create dummy config and check that it is valid
    IIRLowpassFilterConfig filterConfig {};
    filterConfig.set_sampleFreq(_config.sampleFreq());
    filterConfig.set_cutoffFreq(Fc);

    printf("Updating cutoff frequency\n");
    printf("Fs: %f - Fc: %f\n", filterConfig.sampleFreq(), filterConfig.cutoffFreq());

    return _initFromConfig(filterConfig);
}

PBRet IIRLowpassFilter::setSamplingFreq(double Fs)
{
    // Create dummy config and check that it is valid
    IIRLowpassFilterConfig filterConfig {};
    filterConfig.set_sampleFreq(Fs);
    filterConfig.set_cutoffFreq(_config.cutoffFreq());

    printf("Updating sampling frequency\n");
    printf("Fs: %f - Fc: %f\n", filterConfig.sampleFreq(), filterConfig.cutoffFreq());

    return _initFromConfig(filterConfig);
}

PBRet IIRLowpassFilter::checkInputs(const IIRLowpassFilterConfig& config)
{
    esp_err_t err = 0;

    if (Utilities::check(config.sampleFreq()) == false) {
        ESP_LOGE(IIRLowpassFilter::Name, "Sampling frequency was inf or NaN");
        err |= ESP_FAIL;
    }

    if (config.sampleFreq() <= 0) {
        ESP_LOGE(IIRLowpassFilter::Name, "Sampling frequency was <= 0");
        err |= ESP_FAIL;
    }

    if (Utilities::check(config.cutoffFreq()) == false) {
        ESP_LOGE(IIRLowpassFilter::Name, "Cutoff frequency was inf or NaN");
        err |= ESP_FAIL;
    }

    if (config.cutoffFreq() <= 0) {
        ESP_LOGE(IIRLowpassFilter::Name, "Cutoff frequency was <= 0");
        err |= ESP_FAIL;
    }

    if (config.cutoffFreq() >= (config.sampleFreq() / 2)) {
        ESP_LOGE(IIRLowpassFilter::Name, "Sampling frequency (%.3f) was above the nyquist frequency", config.cutoffFreq());
        err |= ESP_FAIL;
    }

    return err == ESP_OK ? PBRet::SUCCESS : PBRet::FAILURE;
}

PBRet IIRLowpassFilter::loadFromJSON(IIRLowpassFilterConfig& cfg, const cJSON* cfgRoot)
{
    // if (cfgRoot == nullptr) {
    //     ESP_LOGW(IIRLowpassFilter::Name, "cfg was null");
    //     return PBRet::FAILURE;
    // }

    // // Get sampling frequency
    // cJSON* FsNode = cJSON_GetObjectItem(cfgRoot, "Fs");
    // if (cJSON_IsNumber(FsNode)) {
    //     cfg.Fs = FsNode->valuedouble;
    // } else {
    //     ESP_LOGW(IIRLowpassFilter::Name, "Unable to read sampling frequency from JSON");
    //     return PBRet::FAILURE;
    // }

    // // Get cutoff frequency
    // cJSON* FcNode = cJSON_GetObjectItem(cfgRoot, "Fc");
    // if (cJSON_IsNumber(FcNode)) {
    //     cfg.sampleFreq() = FcNode->valuedouble;
    // } else {
    //     ESP_LOGW(IIRLowpassFilter::Name, "Unable to read cutoff frequency from JSON");
    //     return PBRet::FAILURE;
    // }

    return PBRet::SUCCESS;
}

PBRet IIRLowpassFilter::_initFromConfig(const IIRLowpassFilterConfig& config)
{
    if (checkInputs(config) == PBRet::SUCCESS) {
        _config = config;

        FilterConfig filterConfig {};
        if (_computeFilterCoefficients(_config.sampleFreq(), _config.cutoffFreq(), filterConfig) != PBRet::SUCCESS) {
            ESP_LOGW(IIRLowpassFilter::Name, "Failed to compute filter coefficients. Filter was not configured");
            return PBRet::FAILURE;
        }

        _filter = Filter(filterConfig);
        if (_filter.isConfigured() == false) {
            ESP_LOGW(IIRLowpassFilter::Name, "Failed to configure base filter object");
            return PBRet::FAILURE;
        }

        printf("IIR Filter configured\n");
        printf("Fs: %f - Fc: %f\n", _config.sampleFreq(), _config.cutoffFreq());

        ESP_LOGW(IIRLowpassFilter::Name, "Invalid inputs. Filter was not configured");
        return PBRet::SUCCESS;
    }


    return PBRet::FAILURE;
}

PBRet IIRLowpassFilter::_computeFilterCoefficients(double samplingFreq, double cutoffFreq, FilterConfig& filterConfig)
{
    // Compute biquad filter coefficients
    // https://e2e.ti.com/cfs-file/__key/communityserver-discussions-components-files/6/Configure-the-Coefficients-for-Digital-Biquad-Filters-in-TLV320AIc3xxx-F_2E00__2E00__2E00_.pdf

    const double omega0 = 2.0 * M_PI * cutoffFreq / samplingFreq;
    const double Q = 0.707;
    const double alpha = sin(omega0) / (2.0 * Q);

    // Compute normalized denominator coefficients
    const double a0 = 1.0 + alpha;
    const double a1 = -2.0 * cos(omega0) / a0;
    const double a2 = (1.0 - alpha) / a0;

    // Compute normalized numerator coefficients
    const double b0 = (1.0 - cos(omega0)) / (2.0 * a0);
    const double b1 = (1.0 - cos(omega0)) / a0;
    const double b2 = b0;

    filterConfig.num = {b0, b1, b2};
    filterConfig.den = {a1, a2};

    return PBRet::SUCCESS;
}