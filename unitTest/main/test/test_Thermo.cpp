#include "unity.h"
#include <stdio.h>
#include "main/thermo.h"

TEST_CASE("Antoine Equation Ethanol", "[Thermo}")
{
    // Test cases generated from 
    // http://ddbonline.ddbst.com/computeVapourPressureCalculation/computeVapourPressureCalculationCGI.exe
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 96.2356, Thermo::computeVapourPressureAntoine(ThermoConstants::ETH_A, ThermoConstants::ETH_B, ThermoConstants::ETH_C, 77.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 100.169, Thermo::computeVapourPressureAntoine(ThermoConstants::ETH_A, ThermoConstants::ETH_B, ThermoConstants::ETH_C, 78.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 104.233, Thermo::computeVapourPressureAntoine(ThermoConstants::ETH_A, ThermoConstants::ETH_B, ThermoConstants::ETH_C, 79.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 108.432, Thermo::computeVapourPressureAntoine(ThermoConstants::ETH_A, ThermoConstants::ETH_B, ThermoConstants::ETH_C, 80.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 112.767, Thermo::computeVapourPressureAntoine(ThermoConstants::ETH_A, ThermoConstants::ETH_B, ThermoConstants::ETH_C, 81.0));
}

TEST_CASE("Antoine Equation H20", "[Thermo}")
{
    // Test cases generated from 
    // http://ddbonline.ddbst.com/computeVapourPressureCalculation/computeVapourPressureCalculationCGI.exe
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 12.3056, Thermo::computeVapourPressureAntoine(ThermoConstants::H20_A, ThermoConstants::H20_B, ThermoConstants::H20_C, 50.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 19.8702, Thermo::computeVapourPressureAntoine(ThermoConstants::H20_A, ThermoConstants::H20_B, ThermoConstants::H20_C, 60.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 31.0872, Thermo::computeVapourPressureAntoine(ThermoConstants::H20_A, ThermoConstants::H20_B, ThermoConstants::H20_C, 70.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 47.2671, Thermo::computeVapourPressureAntoine(ThermoConstants::H20_A, ThermoConstants::H20_B, ThermoConstants::H20_C, 80.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 70.0298, Thermo::computeVapourPressureAntoine(ThermoConstants::H20_A, ThermoConstants::H20_B, ThermoConstants::H20_C, 90.0));
}

TEST_CASE("Vapour Pressure H20", "[Thermo}")
{
    // Test cases generated from 
    // https://en.wikipedia.org/wiki/Vapour_pressure_of_water
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 12.3440, Thermo::computeVapourPressureH20(50.0));
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 19.9320, Thermo::computeVapourPressureH20(60.0));
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 31.1760, Thermo::computeVapourPressureH20(70.0));
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 47.3730, Thermo::computeVapourPressureH20(80.0));
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 70.1170, Thermo::computeVapourPressureH20(90.0));
}

