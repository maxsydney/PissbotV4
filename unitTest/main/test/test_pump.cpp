#include "unity.h"
#include <stdio.h>
#include "main/pump.h"

TEST_CASE("Constructor", "[Pump]")
{
    // Sucessful initialization
    Pump P(GPIO_NUM_23, LEDC_CHANNEL_0, LEDC_TIMER_0);
    TEST_ASSERT_TRUE(P.isConfigured());
}

TEST_CASE("Set Speed", "[Pump]")
{
    Pump P(GPIO_NUM_23, LEDC_CHANNEL_0, LEDC_TIMER_0);

    // Normal speed
    P.setSpeed(256);
    TEST_ASSERT_EQUAL_UINT16(256, P.getSpeed());

    // Above maximum speed
    P.setSpeed(1000);
    TEST_ASSERT_EQUAL_UINT16(PUMP_MAX_OUTPUT, P.getSpeed());

    // Below miniumum speed
    P.setSpeed(0);
    TEST_ASSERT_EQUAL_UINT16(PUMP_MIN_OUTPUT, P.getSpeed());
}