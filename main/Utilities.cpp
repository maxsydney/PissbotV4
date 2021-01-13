#include "Utilities.h"
#include "esp_log.h"
#include <cmath>

PBRet Utilities::polyVal(const std::vector<double>& coeffs, double val, double& result)
{
    // Implements Horner's method for evaluating polynomials
    // Ref: https://en.wikipedia.org/wiki/Horner%27s_method

    if (coeffs.size() == 0) {
        ESP_LOGW(Utilities::Name, "Coeffs vector was empty");
        return PBRet::FAILURE;
    }

    if (Utilities::check(coeffs) == false) {
        ESP_LOGW(Utilities::Name, "Coeffs vector contained inf/nan");
        return PBRet::FAILURE;
    }

    if (Utilities::check(val) == false) {
        ESP_LOGW(Utilities::Name, "val was inf/nan");
        return PBRet::FAILURE;
    }

    result = coeffs[0];
    for (int i = 1; i < coeffs.size(); i++) {
        result *= val;
        result += coeffs[i];
    }
    
    return PBRet::SUCCESS;
}

double Utilities::bound(double val, double lowerLim, double upperLim)
{
    if (val < lowerLim) {
        return lowerLim;
    } else if (val > upperLim) {
        return upperLim;
    } else {
        return val;
    }
}

bool Utilities::check(double val)
{
    if (std::isnan(val) || !std::isfinite(val)) {
        return false;
    }

    return true;
}

bool Utilities::check(const std::vector<double>& val)
{
    for (const double a : val) {
        if (Utilities::check(a) == false) {
            return false;
        }
    }

    return true;
}