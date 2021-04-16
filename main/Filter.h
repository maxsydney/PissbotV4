#ifndef MAIN_FILTER_H
#define MAIN_FILTER_H

#include <vector>
#include "PBCommon.h"
#include "cJSON.h"

class FilterConfig
{
    public:
        FilterConfig(void) = default;
        FilterConfig(std::vector<double>& num, std::vector<double>& den)
            : num(num), den(den) {}

        std::vector<double> num {}; // Numerator coefficients b0, b1 ... bn
        std::vector<double> den {}; // Denominator coeffients a1, a2 ... am
};

// Compute a generic filter. Assumes the leading denominator
// coefficient to be 1
class Filter
{
    static constexpr const char *Name = "Filter";
    static constexpr size_t MAX_NUM_LEN = 6;
    static constexpr size_t MAX_DEN_LEN = 6;
    static constexpr size_t MIN_NUM_LEN = 1;
    static constexpr size_t MIN_DEN_LEN = 0;

    public:
        // Constructors
        Filter(void) = default;
        explicit Filter(const FilterConfig& config);

        // Update
        PBRet filter(double val, double& output);

        // Utility
        static PBRet checkInputs(const FilterConfig& params);
        bool isConfigured(void) const { return _configured; }

    private:

        PBRet _initFromConfig(const FilterConfig &cfg);

        std::vector<double> _inputs {};
        std::vector<double> _outputs {};
        std::vector<double> _num {};
        std::vector<double> _den {};

        bool _configured = false;
};

class IIRLowpassFilterConfig
{
    public:
        IIRLowpassFilterConfig(void) = default;
        IIRLowpassFilterConfig(double samplingFreq, double cutoffFreq)
            : Fs(samplingFreq), Fc(cutoffFreq) {}

        double Fs = 0.0;
        double Fc = 0.0;
};

// Implements a lowpass filter as a single stage biquad
// https://e2e.ti.com/cfs-file/__key/communityserver-discussions-components-files/6/Configure-the-Coefficients-for-Digital-Biquad-Filters-in-TLV320AIc3xxx-F_2E00__2E00__2E00_.pdf
class IIRLowpassFilter
{
    static constexpr const char *Name = "IIRLowpassFilter";
    public:
        // Constructors
        IIRLowpassFilter(void) = default;
        explicit IIRLowpassFilter(const IIRLowpassFilterConfig& config);

        // Update
        PBRet filter(double val, double& output);
        PBRet setCutoffFreq(double Fc);
        PBRet setSamplingFreq(double Fs);

        // Getters
        double getCutoffFreq(void) const { return _config.Fc; }
        double getSamplingFreq(void) const { return _config.Fs; }

        // Utility
        static PBRet checkInputs(const IIRLowpassFilterConfig& config);
        static PBRet loadFromJSON(IIRLowpassFilterConfig& cfg, const cJSON* cfgRoot);
        bool isConfigured(void) const { return _configured; }

        friend class IIRLowpassFilterUT;
    private:

        PBRet _initFromConfig(const IIRLowpassFilterConfig& config);
        PBRet _computeFilterCoefficients(double samplingFreq, double cutoffFreq, FilterConfig& filterConfig);

        Filter _filter {};
        IIRLowpassFilterConfig _config {};
        bool _configured = false;

};

#endif // MAIN_FILTER_H