#include <stdio.h>
#include "unity.h"
#include "main/thermo.h"

#ifdef __cplusplus
extern "C"
{
#endif

void includeThermoTests(void)
{
    // Dummy function to force discovery of unit tests by main test runner
}

TEST_CASE("Antoine Equation Ethanol", "[Thermo]")
{
    // Test cases generated from
    // http://ddbonline.ddbst.com/computeVapourPressureCalculation/computeVapourPressureCalculationCGI.exe

    // Below lower T limit
    TEST_ASSERT_EQUAL_FLOAT(
        Thermo::computeVapourPressureAntoine(AntoineModels::Ethanol, AntoineModels::Ethanol.TLimLower), 
        Thermo::computeVapourPressureAntoine(AntoineModels::Ethanol, AntoineModels::Ethanol.TLimLower - 1));

    // Above upper T limit
    TEST_ASSERT_EQUAL_FLOAT(
        Thermo::computeVapourPressureAntoine(AntoineModels::Ethanol, AntoineModels::Ethanol.TLimUpper), 
        Thermo::computeVapourPressureAntoine(AntoineModels::Ethanol, AntoineModels::Ethanol.TLimUpper + 1));

    // Valid T
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 96.2356, Thermo::computeVapourPressureAntoine(AntoineModels::Ethanol, 77.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 100.169, Thermo::computeVapourPressureAntoine(AntoineModels::Ethanol, 78.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 104.233, Thermo::computeVapourPressureAntoine(AntoineModels::Ethanol, 79.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 108.432, Thermo::computeVapourPressureAntoine(AntoineModels::Ethanol, 80.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 112.767, Thermo::computeVapourPressureAntoine(AntoineModels::Ethanol, 81.0));
}

TEST_CASE("Antoine Equation H20", "[Thermo]")
{
    // Test cases generated from
    // http://ddbonline.ddbst.com/computeVapourPressureCalculation/computeVapourPressureCalculationCGI.exe

    // Below lower T limit
    TEST_ASSERT_EQUAL_FLOAT(
        Thermo::computeVapourPressureAntoine(AntoineModels::H20, AntoineModels::H20.TLimLower), 
        Thermo::computeVapourPressureAntoine(AntoineModels::H20, AntoineModels::H20.TLimLower - 1));

    // Above upper T limit
    TEST_ASSERT_EQUAL_FLOAT(
        Thermo::computeVapourPressureAntoine(AntoineModels::H20, AntoineModels::H20.TLimUpper), 
        Thermo::computeVapourPressureAntoine(AntoineModels::H20, AntoineModels::H20.TLimUpper + 1));

    TEST_ASSERT_FLOAT_WITHIN(1e-3, 12.3056, Thermo::computeVapourPressureAntoine(AntoineModels::H20, 50.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 19.8702, Thermo::computeVapourPressureAntoine(AntoineModels::H20, 60.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 31.0872, Thermo::computeVapourPressureAntoine(AntoineModels::H20, 70.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 47.2671, Thermo::computeVapourPressureAntoine(AntoineModels::H20, 80.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 70.0298, Thermo::computeVapourPressureAntoine(AntoineModels::H20, 90.0));
}

TEST_CASE("Vapour Pressure H20", "[Thermo]")
{
    // Test cases generated from
    // https://en.wikipedia.org/wiki/Vapour_pressure_of_water
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 12.3440, Thermo::computeVapourPressureH20(50.0));
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 19.9320, Thermo::computeVapourPressureH20(60.0));
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 31.1760, Thermo::computeVapourPressureH20(70.0));
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 47.3730, Thermo::computeVapourPressureH20(80.0));
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 70.1170, Thermo::computeVapourPressureH20(90.0));
}

TEST_CASE("Mol fraction liquid", "[Thermo]")
{
    // Test cases generated from
    // The Compleat Distiller - Fig. 8-6
    // Note: Tolerance is slightly larger here due to using different antoine coefficients
    //       to those used in the reference
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 0.86996, Thermo::computeLiquidEthMolFraction(80.0));
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 0.58316, Thermo::computeLiquidEthMolFraction(85.0));
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 0.35372, Thermo::computeLiquidEthMolFraction(90.0));
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 0.16252, Thermo::computeLiquidEthMolFraction(95.0));
}

TEST_CASE("Mol fraction vapour", "[Thermo]")
{
    // Test cases generated from
    // The Compleat Distiller - Fig. 8-6
    // Note: Tolerance is slightly larger here due to using different antoine coefficients
    //       to those used in the reference
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 0.94, Thermo::computeVapourEthMolFraction(80.09125));
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 0.76, Thermo::computeVapourEthMolFraction(85.09472));
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 0.55, Thermo::computeVapourEthMolFraction(90.00073));
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 0.29, Thermo::computeVapourEthMolFraction(95.1303));
}

TEST_CASE("Compute mass fraction", "[Thermo]")
{
    // Test cases based on 
    // On the Conversion of Alcohol - Edwin Croissant - Alcohol strength by mass to mole fraction and vice versa
    TEST_ASSERT_FLOAT_WITHIN(1e-8, 0.0, Thermo::computeMassFraction(0.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-8, 0.22126345001419268, Thermo::computeMassFraction(0.1));
    TEST_ASSERT_FLOAT_WITHIN(1e-8, 0.5228859950898993, Thermo::computeMassFraction(0.3));
    TEST_ASSERT_FLOAT_WITHIN(1e-8, 0.7188785915919336, Thermo::computeMassFraction(0.5));
    TEST_ASSERT_FLOAT_WITHIN(1e-8, 0.8564612095833287, Thermo::computeMassFraction(0.7));
    TEST_ASSERT_FLOAT_WITHIN(1e-8, 0.958358730271, Thermo::computeMassFraction(0.9));
    TEST_ASSERT_FLOAT_WITHIN(1e-8, 1.0, Thermo::computeMassFraction(1.0));
}

TEST_CASE("Compute ABV", "[Thermo]")
{
    // Test cases based on 
    // On the Conversion of Alcohol - Edwin Croissant - Alcohol strength by volume to mass and vice versa
    TEST_ASSERT_FLOAT_WITHIN(1e-4, 0.0, Thermo::computeABV(0.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-8, 0.3045662789241823, Thermo::computeABV(0.25));
    TEST_ASSERT_FLOAT_WITHIN(1e-8, 0.5788769426443598, Thermo::computeABV(0.5));
    TEST_ASSERT_FLOAT_WITHIN(1e-8, 0.6773919643274459, Thermo::computeABV(0.6));
    TEST_ASSERT_FLOAT_WITHIN(1e-8, 0.7694699472341111, Thermo::computeABV(0.7));
    TEST_ASSERT_FLOAT_WITHIN(1e-8, 0.8130296405120505, Thermo::computeABV(0.75));
    TEST_ASSERT_FLOAT_WITHIN(1e-8, 0.854843434215804, Thermo::computeABV(0.8));
    TEST_ASSERT_FLOAT_WITHIN(1e-8, 0.8947805543559094, Thermo::computeABV(0.85));
    TEST_ASSERT_FLOAT_WITHIN(1e-8, 0.932613984458387, Thermo::computeABV(0.9));
    TEST_ASSERT_FLOAT_WITHIN(1e-8, 0.9679367694056882, Thermo::computeABV(0.95));
    TEST_ASSERT_FLOAT_WITHIN(1e-4, 1.0, Thermo::computeABV(1.0));
}

TEST_CASE("Compute liquid ABV Lookup", "[Thermo]")
{
    // Test cases based on
    // Higher Alcohols in the Alcoholic Distillation From Fermented Cane Molasses - EQUILIBRIUM COMPOSITIONS 
    // FOR THE SYSTEM ETHANOL-WATER AT ONE ATMOSPHERE

    // Test some known cases
    TEST_ASSERT_FLOAT_WITHIN(1e-2, 2.52, Thermo::computeLiquidABVLookup(98.05));
    TEST_ASSERT_FLOAT_WITHIN(1e-2, 5.02, Thermo::computeLiquidABVLookup(96.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-2, 9.98, Thermo::computeLiquidABVLookup(93.1));
    TEST_ASSERT_FLOAT_WITHIN(1e-2, 16.1, Thermo::computeLiquidABVLookup(90.02));
    TEST_ASSERT_FLOAT_WITHIN(1e-2, 25.14, Thermo::computeLiquidABVLookup(87.02));
    TEST_ASSERT_FLOAT_WITHIN(1e-4, 33.95, Thermo::computeLiquidABVLookup(85.04));

    // Test interpolation working
    TEST_ASSERT_FLOAT_WITHIN(1e-2, 24.845, Thermo::computeLiquidABVLookup(87.17));
    TEST_ASSERT_FLOAT_WITHIN(1e-2, 4.71, Thermo::computeLiquidABVLookup(96.28));

    // Edge cases
    TEST_ASSERT_FLOAT_WITHIN(1e-4, 0.0, Thermo::computeLiquidABVLookup(0.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-4, 0.0, Thermo::computeLiquidABVLookup(100.0));
}


#ifdef __cplusplus
}
#endif
