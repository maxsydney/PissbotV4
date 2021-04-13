#include "Utilities.h"
#include "esp_log.h"
#include <cmath>
#include <algorithm>

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

PBRet Utilities::interpLinear(const std::vector<double>& x, const std::vector<double>& y, double xVal, double& yVal)
{
    // Performs binary search on data to find appropriate interval, then performs linear search
    // on the interval indentified
    //
    // Note: Assumes x is sorted

    if (x.size() != y.size()) {
        ESP_LOGW(Utilities::Name, "x and y vectors were not of the same length");
        return PBRet::FAILURE;
    }

    if (xVal < x.front()) {
        ESP_LOGW(Utilities::Name, "xVal outside of interpolation range");
        return PBRet::FAILURE;
    }

    if (xVal > x.back()) {
        ESP_LOGW(Utilities::Name, "xVal outside of interpolation range");
        return PBRet::FAILURE;
    }

    // Avoid access of invalid memory
    if (xVal == x.front()) {
        yVal = y.front();
        return PBRet::SUCCESS;
    }

    if (xVal == x.back()) {
        yVal = y.back();
        return PBRet::SUCCESS;
    }

    // Get iterator to first value in x that is equal to or greater than xVal
    size_t idx = std::lower_bound(x.begin(), x.end(), xVal) - x.begin();

    const double p = (xVal - x[idx-1]) / (x[idx] - x[idx-1]);
    yVal = (1 - p) * y[idx - 1] + p * y[idx];

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

void CircularBuffer::insert(double val)
{
    _buff[_curr++] = val;

    if (_curr >= _size) {
        _curr -= _size;      // TODO: Verify that _curr cannot ever be greater than size
    }
}

double CircularBuffer::at(size_t index) const
{   
    return _buff[_computeOffset(index)];
}

void CircularBuffer::clear(void)
{
    std::fill(_buff.begin(), _buff.end(), 0.0);
}

double& CircularBuffer::operator[] (size_t index)
{
    return _buff[_computeOffset(index)];
}

size_t CircularBuffer::_computeOffset(size_t index) const
{
    // Avoid module by zero
    if (_size == 0) {
        return 0;
    }

    int offset = (_curr + index) % _size;

    return offset;
}
