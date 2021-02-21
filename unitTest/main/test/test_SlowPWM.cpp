#include "unity.h"
#include "main/SlowPWM.h"
#include "testSlowPWMConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

void includeSlowPWMTests(void)
{
    // Dummy function to force discovery of unit tests by main test runner
}

static SlowPWMConfig validConfig(void)
{
    SlowPWMConfig cfg {};
    cfg.PWMFreq = 1.0;

    return cfg;
}

TEST_CASE("Constructor", "[SlowPWM]")
{
    // Default constructor does't configure
    {
        SlowPWM spwm {};
        TEST_ASSERT_FALSE(spwm.isConfigured());
    }
    // Valid config
    {
        SlowPWMConfig validCfg = validConfig();
        SlowPWM spwm(validCfg);
        TEST_ASSERT_TRUE(spwm.isConfigured());
    }

    // Invalid config
    {
        SlowPWMConfig invalidCfg {};
        SlowPWM spwm(invalidCfg);
        TEST_ASSERT_FALSE(spwm.isConfigured());
    }
}

TEST_CASE("checkInputs", "[SlowPWM]")
{
    // Valid config
    {
        SlowPWMConfig validCfg = validConfig();
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, SlowPWM::checkInputs(validCfg));
    }

    // Invalid PWM frequency
    {
        SlowPWMConfig invalidCfg {};
        invalidCfg.PWMFreq = 100.0;
        TEST_ASSERT_EQUAL(PBRet::FAILURE, SlowPWM::checkInputs(invalidCfg));
    }
}

TEST_CASE("loadFromJSONValid", "[SlowPWM]")
{
    SlowPWMConfig testConfig {};
    cJSON* root = cJSON_Parse(slowPWMTestConfig);
    TEST_ASSERT_NOT_EQUAL(root, nullptr);
 
    // Get config node
    cJSON* cfg = cJSON_GetObjectItem(root, "validSlowPWM");
    TEST_ASSERT_NOT_EQUAL(cfg, nullptr);

    TEST_ASSERT_EQUAL(PBRet::SUCCESS, SlowPWM::loadFromJSON(testConfig, cfg));
    TEST_ASSERT_EQUAL(5, testConfig.PWMFreq);
}

TEST_CASE("loadFromJSONInvalid", "[SlowPWM]")
{
    SlowPWMConfig testConfig {};
    cJSON* root = cJSON_Parse(slowPWMTestConfig);
    TEST_ASSERT_NOT_EQUAL(root, nullptr);
 
    // Get config node
    cJSON* cfg = cJSON_GetObjectItem(root, "invalidSlowPWM");
    TEST_ASSERT_NOT_EQUAL(cfg, nullptr);

    TEST_ASSERT_EQUAL(PBRet::FAILURE, SlowPWM::loadFromJSON(testConfig, cfg));
}

TEST_CASE("update", "[SlowPWM]")
{
    // Duty cycle 0.25
    {
        SlowPWMConfig cfg = validConfig();
        SlowPWM spwm(cfg);
        TEST_ASSERT_TRUE(spwm.isConfigured());
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, spwm.setDutyCycle(0.25));

        // Output should be set high at t=0
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, spwm.update(0));
        TEST_ASSERT_EQUAL(1, spwm.getOutputState());

        // Output should be high at 0.25 s
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, spwm.update(250000));
        TEST_ASSERT_EQUAL(1, spwm.getOutputState());

        // Output should be low at 0.251 s
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, spwm.update(251000));
        TEST_ASSERT_EQUAL(0, spwm.getOutputState());

        // Output remains low until end of period
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, spwm.update(999999));
        TEST_ASSERT_EQUAL(0, spwm.getOutputState());
    }

    // Duty cycle 0.5
    {
        SlowPWMConfig cfg = validConfig();
        SlowPWM spwm(cfg);
        TEST_ASSERT_TRUE(spwm.isConfigured());
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, spwm.setDutyCycle(0.5));

        // Output should be set high at t=0
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, spwm.update(0));
        TEST_ASSERT_EQUAL(1, spwm.getOutputState());

        // Output should be high at 0.25 s
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, spwm.update(250000));
        TEST_ASSERT_EQUAL(1, spwm.getOutputState());

        // Output should be high at 0.5 s
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, spwm.update(500000));
        TEST_ASSERT_EQUAL(1, spwm.getOutputState());

        // Output should be low at 0.51 s
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, spwm.update(510000));
        TEST_ASSERT_EQUAL(0, spwm.getOutputState());

        // Output remains high until end of period
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, spwm.update(999999));
        TEST_ASSERT_EQUAL(0, spwm.getOutputState());
    }

    // Duty cycle 1.0
    {
        SlowPWMConfig cfg = validConfig();
        SlowPWM spwm(cfg);
        TEST_ASSERT_TRUE(spwm.isConfigured());
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, spwm.setDutyCycle(1.0));

        // Output should be set high at t=0
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, spwm.update(0));
        TEST_ASSERT_EQUAL(1, spwm.getOutputState());

        // Output should be high at 0.25 s
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, spwm.update(250000));
        TEST_ASSERT_EQUAL(1, spwm.getOutputState());

        // Output should be high at 0.5 s
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, spwm.update(500000));
        TEST_ASSERT_EQUAL(1, spwm.getOutputState());

        // Output remains high until end of period
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, spwm.update(999999));
        TEST_ASSERT_EQUAL(1, spwm.getOutputState());
    }

    // Skip timer period
    {
        SlowPWMConfig cfg = validConfig();
        SlowPWM spwm(cfg);
        TEST_ASSERT_TRUE(spwm.isConfigured());
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, spwm.setDutyCycle(0.5));

        // Output should be high at 1.5s
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, spwm.update(1500000));
        TEST_ASSERT_EQUAL(1, spwm.getOutputState());

        // Output should be low at 1.51s
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, spwm.update(1510000));
        TEST_ASSERT_EQUAL(0, spwm.getOutputState());

        // Output should be high at 2.0s
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, spwm.update(2000000));
        TEST_ASSERT_EQUAL(1, spwm.getOutputState());
    }
}

TEST_CASE("setDutyCycle", "[SlowPWM]")
{
    SlowPWMConfig cfg = validConfig();
    SlowPWM spwm(cfg);
    TEST_ASSERT_TRUE(spwm.isConfigured());

    // Valid duty cycle
    {
        const double validDutyCycle = 0.5;
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, spwm.setDutyCycle(validDutyCycle));
        TEST_ASSERT_EQUAL(validDutyCycle, spwm.getDutyCycle());

    }

    // Duty cycle greater than 1
    {
        const double invalidDutyCycle = 1.1;
        TEST_ASSERT_EQUAL(PBRet::FAILURE, spwm.setDutyCycle(invalidDutyCycle));
    }

    // Duty cycle less than 0
    {
        const double invalidDutyCycle = -0.1;
        TEST_ASSERT_EQUAL(PBRet::FAILURE, spwm.setDutyCycle(invalidDutyCycle));
    }
}

#ifdef __cplusplus
}
#endif