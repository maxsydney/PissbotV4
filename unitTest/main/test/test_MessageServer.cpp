#include "unity.h"
#include "main/MessageServer.h"
#include "Generated/ControllerMessaging.h"

#ifdef __cplusplus
extern "C" {
#endif

void includeMessageServerTests(void)
{
    // Dummy function to force discovery of unit tests by main test runner
}

TEST_CASE("wrapUnwrap", "[MessageServer]")
{
    // Wrap and unwrap a basic ControllerTuning message
    ControllerTuning controllerTuning {};
    controllerTuning.set_setpoint(10.0);
    controllerTuning.set_PGain(20.0);
    controllerTuning.set_IGain(30.0);
    controllerTuning.set_DGain(40.0);
    controllerTuning.set_LPFsampleFreq(50.0);
    controllerTuning.set_LPFcutoffFreq(60.0);
    
    PBMessageWrapper wrapped = MessageServer::wrap(controllerTuning, PBMessageType::ControllerTuning, MessageOrigin::DistillerManager);
    TEST_ASSERT_EQUAL(PBMessageType::ControllerTuning, wrapped.get_type());
    TEST_ASSERT_EQUAL(MessageOrigin::DistillerManager, wrapped.get_origin());

    // Payload buffer corrupted
    {
        PBMessageWrapper wrappedCorrupted {};
        wrappedCorrupted = wrapped;

        // Get payload buffer reference and corrupt it
        wrappedCorrupted.mutable_payload()[0] = 0xFF;

        // Attempt to unwrap
        ControllerTuning recovered {};
        TEST_ASSERT_EQUAL(PBRet::FAILURE, MessageServer::unwrap(wrappedCorrupted, recovered));
    }

    // Unwrap into wrong type
    {
        ControllerCommand recovered {};
        TEST_ASSERT_EQUAL(PBRet::FAILURE, MessageServer::unwrap(wrapped, recovered));
    }

    // Successful unwrap
    {
        ControllerTuning recovered {};
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, MessageServer::unwrap(wrapped, recovered));
        TEST_ASSERT_EQUAL_DOUBLE(controllerTuning.setpoint(), recovered.setpoint());
        TEST_ASSERT_EQUAL_DOUBLE(controllerTuning.PGain(), recovered.PGain());
        TEST_ASSERT_EQUAL_DOUBLE(controllerTuning.IGain(), recovered.IGain());
        TEST_ASSERT_EQUAL_DOUBLE(controllerTuning.DGain(), recovered.DGain());
        TEST_ASSERT_EQUAL_DOUBLE(controllerTuning.LPFsampleFreq(), recovered.LPFsampleFreq());
        TEST_ASSERT_EQUAL_DOUBLE(controllerTuning.LPFcutoffFreq(), recovered.LPFcutoffFreq());
    }
}

#ifdef __cplusplus
}
#endif
