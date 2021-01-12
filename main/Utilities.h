#ifndef UTILITIES_H
#define UTILITIES_H

#include <vector>

class Utilities
{
    static constexpr const char* Name = "Utilities";

    public:

        double polyVal(const std::vector<double>& coeffs, double val);
};

#endif // UTILITIES_H