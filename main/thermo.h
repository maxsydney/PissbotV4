#ifndef THERMO_H
#define THERMO_H

#ifdef __cplusplus
extern "C" {
#endif

class ThermoConstants
{
    public:
        static constexpr double H20_A = 8.07131;
        static constexpr double H20_B = 1730.63;
        static constexpr double H20_C = 233.426;

        static constexpr double ETH_A = 7.68117;
        static constexpr double ETH_B = 1332.04;
        static constexpr double ETH_C = 199.200;

        static constexpr double P_atm = 101.325;
};

class ThermoConversions
{
    public:
        static constexpr double mmHg_to_kPa = 0.133322387415;
};

class Thermo
{
    public:
        static double computeVapourPressureAntoine(double a, double b, double c, double temp);
        static double computeVapourPressureH20(double temp);
        static double computeLiquidEthConcentration(double temp);
        static double computeVapourEthConcentration(double temp);
};

#ifdef __cplusplus
}
#endif

#endif // THERMO_H

