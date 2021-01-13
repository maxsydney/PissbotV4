#ifndef UTILITIES_H
#define UTILITIES_H

#include "PBCommon.h"
#include <vector>

class Utilities
{
    static constexpr const char* Name = "Utilities";

    public:

        // Math
        static PBRet polyVal(const std::vector<double>& coeffs, double val, double& result);
        static double bound(double val, double lowerLim, double upperLim);

        // Checkers
        static bool check(double val);
        static bool check(const std::vector<double>& val);

};

#endif // UTILITIES_H