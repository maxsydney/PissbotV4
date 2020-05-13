#include "unity.h"
#include <stdio.h>
#include "/Users/maxsydney/esp/PissbotV4/main/controlLoop.h"

void testFn(void)
{
    printf("I'm working");
}

TEST_CASE("VapourConcentration", "[mean]")
{
    const double conc = getVapourConcentration(78);
    TEST_ASSERT_EQUAL(0, conc);
}