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

TEST_CASE("interpLinear", "[Utilities]")
{
    // Sample polynomial y = 3x^2 + 4x + 5
    const std::vector<double> xVector = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0};
    const std::vector<double> yVector = {5.0, 12.0, 25.0, 44.0, 69.0, 100.0};

    // x and y vectors different length
    {
        const std::vector<double> yVectorShort = {5.0, 12.0};
        double output = 0.0;
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Utilities::interpLinear(xVector, yVectorShort, 0.0, output));
    }

    // Search value smaller than first x value
    {
        double output = 0.0;
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Utilities::interpLinear(xVector, yVector, -1.0, output));
    }

    // Search value larger than last x value
    {
        double output = 0.0;
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Utilities::interpLinear(xVector, yVector, 6.0, output));
    }

    // Valid search at known x
    {
        double output = 0.0;
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, Utilities::interpLinear(xVector, yVector, 2.0, output));
        TEST_ASSERT_EQUAL(25.0, output);
    }

    // Search value equal to first x value
    {
        double output = 0.0;
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, Utilities::interpLinear(xVector, yVector, xVector.front(), output));
        TEST_ASSERT_EQUAL(yVector.front(), output);
    }

    // Search value equal to last x value
    {
        double output = 0.0;
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, Utilities::interpLinear(xVector, yVector, xVector.back(), output));
        TEST_ASSERT_EQUAL(yVector.back(), output);
    }

    // Midway point
    {
        double output = 0.0;
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, Utilities::interpLinear(xVector, yVector, 1.5, output));
        TEST_ASSERT_EQUAL(18.5, output);
    }

    // 3/4 point
    {
        double output = 0.0;
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, Utilities::interpLinear(xVector, yVector, 1.75, output));
        TEST_ASSERT_EQUAL(21.75, output);
    }
}

TEST_CASE("CheckDouble", "[Utilities]")
{
    // Check valid double
    TEST_ASSERT_TRUE(Utilities::check(0.0));

    // Check nan
    TEST_ASSERT_FALSE(Utilities::check(NAN));

    // Check inf
    TEST_ASSERT_FALSE(Utilities::check(std::numeric_limits<double>::infinity()));
}

TEST_CASE("CheckDoubleVector", "[Utilities]")
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

TEST_CASE("CircularBuffer", "[Utilities]")
{
    CircularBuffer buff(5);

    // Check all entries initialized to 0
    for (size_t i = 0; i < buff.size(); i++) {
        TEST_ASSERT_EQUAL(0.0, buff.at(i));
    }

    // Add some values and check order is preserved
    buff.insert(0.0);
    buff.insert(1.0);
    buff.insert(2.0);

    TEST_ASSERT_EQUAL(0.0, buff.at(0));
    TEST_ASSERT_EQUAL(0.0, buff.at(1));
    TEST_ASSERT_EQUAL(0.0, buff.at(2));
    TEST_ASSERT_EQUAL(1.0, buff.at(3));
    TEST_ASSERT_EQUAL(2.0, buff.at(4));

    // Fill buffer and check all values can be accessed
    buff.insert(3.0);
    buff.insert(4.0);
    for (size_t i = 0; i < buff.size(); i++) {
        TEST_ASSERT_EQUAL(i, buff.at(i));
        TEST_ASSERT_EQUAL(i, buff[i]);
    }

    // Test values can be modified
    buff[2] = 0.0;
    TEST_ASSERT_EQUAL(0.0, buff.at(2));

    // Fill with many values
    for (size_t i = 0; i < 20; i++) {
        buff.insert(i);
    }

    TEST_ASSERT_EQUAL(15.0, buff.at(0));
    TEST_ASSERT_EQUAL(16.0, buff.at(1));
    TEST_ASSERT_EQUAL(17.0, buff.at(2));
    TEST_ASSERT_EQUAL(18.0, buff.at(3));
    TEST_ASSERT_EQUAL(19.0, buff.at(4));

    // Attempting to access past boundary returns wrapped index
    TEST_ASSERT_EQUAL(buff.at(5), buff.at(0));
    TEST_ASSERT_EQUAL(buff.at(6), buff.at(1));
    TEST_ASSERT_EQUAL(buff.at(7), buff.at(2));
    TEST_ASSERT_EQUAL(buff.at(8), buff.at(3));
    TEST_ASSERT_EQUAL(buff.at(9), buff.at(4));
}

TEST_CASE("CircularBufferEmpty", "[Utilities]")
{
    // TODO: Add tests
}

#ifdef __cplusplus
}
#endif