#include "unity.h"
#include "main/SensorManager.h"

class SensorManagerUT
{
    public:
        
};

TEST_CASE("checkInputs", "[SensorManager]")
{
    // Valid configuration
    {
        SensorManagerConfig cfg{};
        TEST_ASSERT_EQUAL(PBRet::FAILURE, SensorManager::checkInputs(cfg));
    }

    // Invalid dt
    {

    }

    // Invalid PBOneWire config
    {

    }
}