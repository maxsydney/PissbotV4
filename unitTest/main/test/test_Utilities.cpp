#include <stdio.h>
#include <limits>
#include "unity.h"
#include "main/Utilities.h"

#ifdef __cplusplus
extern "C" {
#endif

void includeUtilitiesTests(void)
{
    // Dummy function to force discovery of unit tests by main test runner
    printf("Test");
}

TEST_CASE("Polyval", "[Utilities]")
{
    // Single coefficient
    {
        const std::vector<double> coeffs = {1.0};
        double res = 0.0;
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, Utilities::polyVal(coeffs, 1.0, res));
        TEST_ASSERT_EQUAL(1.0, res);
    }

    // Valid polynomial
    {
        // 3x^2 + 2x + 1
        const std::vector<double> coeffs = {3.0, 2.0, 1.0};
        double res = 0.0;
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, Utilities::polyVal(coeffs, 2.0, res));
        TEST_ASSERT_EQUAL(17.0, res);
    }

    // All zeros
    {
        const std::vector<double> coeffs = {0.0, 0.0, 0.0};
        double res = 0.0;
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, Utilities::polyVal(coeffs, 1.0, res));
        TEST_ASSERT_EQUAL(0.0, res);
    }

    // Coeffs is empty
    {
        const std::vector<double> coeffs {};
        double res = 0.0;
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Utilities::polyVal(coeffs, 1.0, res));
    }

    // nan in coeffs
    {
        const std::vector<double> coeffs = {0.0, NAN, 0.0};
        double res = 0.0;
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Utilities::polyVal(coeffs, 1.0, res));
    }

    // inf in coeffs
    {
        const std::vector<double> coeffs = {0.0, 0.0, std::numeric_limits<double>::infinity()};
        double res = 0.0;
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Utilities::polyVal(coeffs, 1.0, res));
    }

    // Val is nan
    {
        const std::vector<double> coeffs = {1.0};
        double res = 0.0;
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Utilities::polyVal(coeffs, NAN, res));
    }

    // Val is inf
    {
        const std::vector<double> coeffs = {1.0};
        double res = 0.0;
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Utilities::polyVal(coeffs, std::numeric_limits<double>::infinity(), res));
    }
}

TEST_CASE("Check double", "[Utilities]")
{
    // Check valid double
    TEST_ASSERT_TRUE(Utilities::check(0.0));

    // Check nan
    TEST_ASSERT_FALSE(Utilities::check(NAN));

    // Check inf
    TEST_ASSERT_FALSE(Utilities::check(std::numeric_limits<double>::infinity()));
}

TEST_CASE("Check double vector", "[Utilities]")
{
    // Check valid vector
    {
        const std::vector<double> testVec = {0.0, 1.0, 2.0, 3.0};
        TEST_ASSERT_TRUE(Utilities::check(testVec));
    }
    
    // Empty vector passes
    {
        const std::vector<double> testVec {};
        TEST_ASSERT_TRUE(Utilities::check(testVec));
    }

    // Vector contains nan
    {
        const std::vector<double> testVec = {0.0, 1.0, NAN, 3.0};
        TEST_ASSERT_FALSE(Utilities::check(testVec));
    }

    // Vector contains inf
    {
        const std::vector<double> testVec = {0.0, 1.0, std::numeric_limits<double>::infinity(), 3.0};
        TEST_ASSERT_FALSE(Utilities::check(testVec));
    }
}

#ifdef __cplusplus
}
#endif