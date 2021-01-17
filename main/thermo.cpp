#include <cmath>
#include "thermo.h"
#include "Utilities.h"
#include "ABVTables.h"

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
    // Ref: On the Conversion of Alcohol - Edwin Croissant

    return molFrac / (molFrac + MolarMass::H20 / MolarMass::Ethanol * (1 - molFrac));
}

double Thermo::computeABV(double massFrac)
{
    // Compute ethanol ABV from mass fraction (ABW) using a polynomial fit
    // Ref: On the Conversion of Alcohol - Edwin Croissant

    static const std::vector<double> coeffs =  {
        -2.0695760421183493E+00,
        8.8142267038252680E+00,
        -1.5765836469736477E+01,
        1.5009692673927390E+01,
        -7.8964816507513707E+00,
        2.0463351302912738E+00,
        -4.0926819348115739E-01,
        1.2709666849144778E+00,
        -3.9705486746795932E-05
    };

    double ABV = 0.0;
    if (Utilities::polyVal(coeffs, massFrac, ABV) != PBRet::SUCCESS) {
        ESP_LOGW(Thermo::Name, "Unable to compute ABV");
    }

    return ABV;
}

double Thermo::computeVapourABV(double T)
{
    // Compute ethanol ABV from vapour temperature using a polynomial fit
    // Ref: On the Conversion of Alcohol - Edwin Croissant

    const double molFrac = Thermo::computeVapourEthMolFraction(T);
    const double massFrac = Thermo::computeMassFraction(molFrac);
    const double ethABV = Thermo::computeABV(massFrac);

    return ethABV;
}

double Thermo::computeLiquidABV(double T)
{
    // Compute ethanol ABV from liquid temperature using a polynomial fit
    // Ref: On the Conversion of Alcohol - Edwin Croissant

    const double molFrac = Thermo::computeLiquidEthMolFraction(T);
    const double massFrac = Thermo::computeMassFraction(molFrac);
    const double ethABV = Thermo::computeABV(massFrac);

    return ethABV;
}

double Thermo::computeLiquidABVLookup(double T)
{
    // Compute the ethanol ABV from liquid temperature based on a lookup table
    // Ref: Higher Alcohols in the Alcoholic Distillation From Fermented 
    //      Cane Molasses - EQUILIBRIUM COMPOSITIONS FOR THE SYSTEM ETHANOL-WATER 
    //      AT ONE ATMOSPHERE
    double ABV = 0.0;

    if (Utilities::interpLinear(ABVTables::T, ABVTables::liquidABV, T, ABV) != PBRet::SUCCESS) {
        ESP_LOGW(Thermo::Name, "Unable to compute liquid ABV from lookup table");
    }

    return ABV;
}

double Thermo::computeVapourABVLookup(double T)
{
    // Compute the ethanol ABV from vapour temperature based on a lookup table
    // Ref: Higher Alcohols in the Alcoholic Distillation From Fermented 
    //      Cane Molasses - EQUILIBRIUM COMPOSITIONS FOR THE SYSTEM ETHANOL-WATER 
    //      AT ONE ATMOSPHERE
    double ABV = 0.0;

    if (Utilities::interpLinear(ABVTables::T, ABVTables::vapourABV, T, ABV) != PBRet::SUCCESS) {
        ESP_LOGW(Thermo::Name, "Unable to compute vapour ABV from lookup table");
    }

    return ABV;
}