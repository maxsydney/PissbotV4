// #include "unity.h"
// #include "main/pump.h"

// TEST_CASE("Constructor", "[Pump]")
// {
//     // Sucessful initialization
//     PumpCfg cfg(GPIO_NUM_23, LEDC_CHANNEL_0, LEDC_TIMER_0);
//     Pump P(cfg);
//     TEST_ASSERT_TRUE(P.isConfigured());
// }

// TEST_CASE("Set Speed", "[Pump]")
// {
//     PumpCfg cfg(GPIO_NUM_23, LEDC_CHANNEL_0, LEDC_TIMER_0);
//     Pump P(cfg);

//     // Normal speed
//     P.setSpeed(256);
//     TEST_ASSERT_EQUAL_UINT16(256, P.getSpeed());

//     // Above maximum speed
//     P.setSpeed(1000);
//     TEST_ASSERT_EQUAL_UINT16(Pump::PUMP_MAX_OUTPUT, P.getSpeed());

//     // Below miniumum speed
//     P.setSpeed(0);
//     TEST_ASSERT_EQUAL_UINT16(Pump::PUMP_MIN_OUTPUT, P.getSpeed());
// }