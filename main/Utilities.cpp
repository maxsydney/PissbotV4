#include "Utilities.h"
#include "esp_log.h"

double Utilities::polyVal(const std::vector<double>& coeffs, double val)
{
    // Implements Horner's method for evaluating polynomials
    // Ref: https://en.wikipedia.org/wiki/Horner%27s_method

    if (coeffs.size() == 0) {
        ESP_LOGW(Utilties::Name, "Coeffs vector was empty");
        return 0.0;
    }

    double result = coeffs[0];
    for (int i = 1; i < coeffs.size(); i++) {
        result *= val;
        result += coeffs[i];
    }
    
    return result;
}