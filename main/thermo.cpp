

#include <cmath>
#include "thermo.h"
#include "Utilities.h"

// Compute the partial vapour pressure in kPa based on the
// Antoine equation https://en.wikipedia.org/wiki/computeVapourPressure_equation
double Thermo::computeVapourPressureAntoine(double A, double B, double C, double T)
{
    double exp = A - B / (C + T);
    return pow(10, exp) * ThermoConversions::mmHg_to_kPa;
}

// Compute the partial pressure of H20 in kPa based on the Buck equation
// https://en.wikipedia.org/wiki/Arden_Buck_equation
double Thermo::computeVapourPressureH20(double T)
{
    return 0.61121 * exp((18.678 - T/234.5)*(T / (257.14 + T)));
}

// Compute bubble line based on The Compleat Distiller
// Ch8 - Equilibrium curves
double Thermo::computeLiquidEthMolFraction(double T)
{
    double P_eth = computeVapourPressureAntoine(ThermoConstants::ETH_A, ThermoConstants::ETH_B, ThermoConstants::ETH_C, T);
    double P_H20 = Thermo::computeVapourPressureH20(T);
    return (ThermoConstants::P_atm - P_H20) / (P_eth - P_H20);
}

double Thermo::computeVapourEthMolFraction(double T)
{
    double molFractionEth = computeLiquidEthMolFraction(T);
    double P_eth = computeVapourPressureAntoine(ThermoConstants::ETH_A, ThermoConstants::ETH_B, ThermoConstants::ETH_C, T);
    return molFractionEth * P_eth / ThermoConstants::P_atm;
}

double Thermo::computeMassFraction(double molFrac)
{
    // Compute ethanol mass fraction from mol fraction

    return 0.0;
}

double Thermo::computeVapourABV(double T)
{
    // Compute ethanol ABV from vapour temperature using a polynomial fit
    // Ref: On the Conversion of Alcohol - Edwin Croissant

    return 0.0;
}

double Thermo::computeLiquidABV(double T)
{
    // Compute ethanol ABV from liquid temperature using a polynomial fit
    // Ref: On the Conversion of Alcohol - Edwin Croissant

    return 0.0;
}

