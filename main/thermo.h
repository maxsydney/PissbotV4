#ifndef THERMO_H
#define THERMO_H

#ifdef __cplusplus
extern "C" {
#endif

class AntoineParams
{
    public:
        constexpr AntoineParams(void) = default;
        constexpr AntoineParams(double A, double B, double C, double TLimLower, double TLimUpper, double convFactor)
            : A(A), B(B), C(C), TLimLower(TLimLower), TLimUpper(TLimUpper), convFactor(convFactor) {}

        double A = 0.0;             // Antoine equation A coefficient
        double B = 0.0;             // Antoine equation B coefficient
        double C = 0.0;             // Antoine equation C coefficient
        double TLimLower = 0.0;     // Antoine coefficients lower temperature limit [Deg C]
        double TLimUpper = 0.0;     // Antoine coefficients upper temperature limit [Deg C]
        double convFactor = 0.0;    // Conversion factor to kPa
};

class ThermoConversions
{
    public:
        static constexpr double mmHg_to_kPa = 0.133322387415;
        static constexpr double mbar_to_kPa = 0.101325;
};

class AntoineModels
{ 
    public:
        static constexpr AntoineParams H20 = {8.07131, 1730.63, 233.426, 1.0, 100.0, ThermoConversions::mmHg_to_kPa};       // Ref: http://ddbonline.ddbst.com/AntoineCalculation/AntoineCalculationCGI.exe
        static constexpr AntoineParams Ethanol = {7.68117, 1332.04, 199.2, 77.0, 243.0, ThermoConversions::mmHg_to_kPa};  // Ref: http://ddbonline.ddbst.com/AntoineCalculation/AntoineCalculationCGI.exe
};

class MolarMass
{
    public:
        static constexpr double Ethanol = 46.0684;      // [g / mol] - Ref: On the Conversion of Alcohol - Edwin Croissant
        static constexpr double H20 = 18.0153;          // [g / mol] - Ref: On the Conversion of Alcohol - Edwin Croissant
};

class ThermoConstants
{
    public:
        static constexpr double P_atm = 101.325;
};

class Thermo
{
    static constexpr const char* Name = "Thermo";

    public:
        static double computeVapourPressureAntoine(const AntoineParams& model, double T);
        static double computeVapourPressureH20(double T);
        static double computeLiquidEthMolFraction(double T);
        static double computeVapourEthMolFraction(double T);
        static double computeMassFraction(double molFrac);
        static double computeVapourABV(double T);
        static double computeLiquidABV(double T);
        static double computeABV(double massFrac);
        static double computeLiquidABVLookup(double T);
        static double computeVapourABVLookup(double T);
};

#ifdef __cplusplus
}
#endif

#endif // THERMO_H

