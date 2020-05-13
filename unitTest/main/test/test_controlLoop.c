#include "unity.h"
#include <stdio.h>
#include "/Users/maxsydney/esp/PissbotV4/main/controlLoop.h"

TEST_CASE("Antoine Equation Ethanol", "[ControlLoop}")
{
    // Test cases generated from 
    // http://ddbonline.ddbst.com/computeVapourPressureCalculation/computeVapourPressureCalculationCGI.exe
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 96.2356, computeVapourPressureAntoine(ETH_A, ETH_B, ETH_C, 77.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 100.169, computeVapourPressureAntoine(ETH_A, ETH_B, ETH_C, 78.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 104.233, computeVapourPressureAntoine(ETH_A, ETH_B, ETH_C, 79.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 108.432, computeVapourPressureAntoine(ETH_A, ETH_B, ETH_C, 80.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 112.767, computeVapourPressureAntoine(ETH_A, ETH_B, ETH_C, 81.0));
}

TEST_CASE("Antoine Equation H20", "[ControlLoop}")
{
    // Test cases generated from 
    // http://ddbonline.ddbst.com/computeVapourPressureCalculation/computeVapourPressureCalculationCGI.exe
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 12.3056, computeVapourPressureAntoine(H20_A, H20_B, H20_C, 50.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 19.8702, computeVapourPressureAntoine(H20_A, H20_B, H20_C, 60.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 31.0872, computeVapourPressureAntoine(H20_A, H20_B, H20_C, 70.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 47.2671, computeVapourPressureAntoine(H20_A, H20_B, H20_C, 80.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 70.0298, computeVapourPressureAntoine(H20_A, H20_B, H20_C, 90.0));
}

TEST_CASE("Vapour Pressure H20", "[ControlLoop}")
{
    // Test cases generated from 
    // https://en.wikipedia.org/wiki/Vapour_pressure_of_water
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 12.3440, computeVapourPressureH20(50.0));
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 19.9320, computeVapourPressureH20(60.0));
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 31.1760, computeVapourPressureH20(70.0));
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 47.3730, computeVapourPressureH20(80.0));
    TEST_ASSERT_FLOAT_WITHIN(5e-2, 70.1170, computeVapourPressureH20(90.0));
}

