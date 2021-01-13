#include <cmath>
#include "thermo.h"
#include "Utilities.h"

double Thermo::computeVapourPressureAntoine(const AntoineParams& model, double T)
{
    // Compute the partial vapour pressure in kPa based on the
    // Antoine equation 
    // Ref: https://en.wikipedia.org/wiki/computeVapourPressure_equation

    T = Utilities::bound(T, model.TLimLower, model.TLimUpper);

    const double exp = model.A - model.B / (model.C + T);
    return pow(10, exp) * model.convFactor;
}

double Thermo::computeVapourPressureH20(double T)
{
    // Compute the partial pressure of H20 in kPa based on the Buck equation
    // Ref: https://en.wikipedia.org/wiki/Arden_Buck_equation

    return 0.61121 * exp((18.678 - T/234.5)*(T / (257.14 + T)));        // TODO: Define these numbers
}

double Thermo::computeLiquidEthMolFraction(double T)
{
    // Compute the mol fraction of ethanol in a binary (H20) liquid mixture
    // Ref: The Compleat Distiller - Ch8  (Equilibrium Curves)

    const double P_eth = computeVapourPressureAntoine(AntoineModels::Ethanol, T);
    const double P_H20 = computeVapourPressureAntoine(AntoineModels::H20, T);
    return (ThermoConstants::P_atm - P_H20) / (P_eth - P_H20);
}

double Thermo::computeVapourEthMolFraction(double T)
{
    // Compute the mol fraction of ethanol in a binary (H20) vapour mixture
    // Ref: The Compleat Distiller - Ch8  (Equilibrium Curves)

    const double molFractionEth = computeLiquidEthMolFraction(T);
    const double P_eth = computeVapourPressureAntoine(AntoineModels::Ethanol, T);
    return molFractionEth * P_eth / ThermoConstants::P_atm;
}

double Thermo::computeMassFraction(double molFrac)
{
    // Compute ethanol mass fraction from mol fraction
    // Ref: 

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

