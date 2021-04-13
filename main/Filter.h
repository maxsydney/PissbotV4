#ifndef MAIN_FILTER_H
#define MAIN_FILTER_H

#include <vector>

class IIRFilterParams
{
    public:
        IIRFilterParams(void) = default;
        IIRFilterParams()

        std::vector<double> num {};
        std::vector<double> den {};
};

class IIRFilter
{
    constexpr size_t MAX_NUM_LEN = 6;
    constexpr size_t MAX_DEN_LEN = 6;

    public:
        
};

#endif // MAIN_FILTER_H