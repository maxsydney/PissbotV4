#ifdef __cplusplus
extern "C" {
#endif

#include <cmath>
#include "thermo.h"

// Compute the partial vapour pressure in kPa based on the
// Antoine equation https://en.wikipedia.org/wiki/computeVapourPressure_equation
double Thermo::computeVapourPressureAntoine(double A, double B, double C, double temp)
{
    double exp = A - B / (C + temp);
    return pow(10, exp) * ThermoConversions::mmHg_to_kPa;
}

// Compute the partial pressure of H20 in kPa based on the Buck equation
// https://en.wikipedia.org/wiki/Arden_Buck_equation
double Thermo::computeVapourPressureH20(double temp)
{
    return 0.61121 * exp((18.678 - temp/234.5)*(temp / (257.14 + temp)));
}

// Compute bubble line based on The Compleat Distiller
// Ch8 - Equilibrium curves
double Thermo::computeLiquidEthConcentration(double temp)
{
    double P_eth = computeVapourPressureAntoine(ThermoConstants::ETH_A, ThermoConstants::ETH_B, ThermoConstants::ETH_C, temp);
    double P_H20 = Thermo::computeVapourPressureH20(temp);
    return (ThermoConstants::P_atm - P_H20) / (P_eth - P_H20);
}

double Thermo::computeVapourEthConcentration(double temp)
{
    double molFractionEth = computeLiquidEthConcentration(temp);
    double P_eth = computeVapourPressureAntoine(ThermoConstants::ETH_A, ThermoConstants::ETH_B, ThermoConstants::ETH_C, temp);
    return molFractionEth * P_eth / ThermoConstants::P_atm;
}

#ifdef __cplusplus
}
#endif
