#include "unity.h"
#include <stdio.h>
#include "main/messages.h"
#include "main/controlLoop.h"

TEST_CASE("ReadCtrlParams", "[Messages]")
{
    // Test valid message
    ctrlParams_t ctrlParams = {};
    const char* validMsg = "{\"type\":\"INFO\",\"subtype\":\"ctrlParams\",\"data\":{\"setpoint\":77.25,\"P_gain\":80,\"I_gain\":2,\"D_gain\":8}}";
    cJSON* root = NULL;
    root = cJSON_Parse(validMsg);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL_INT(ESP_OK, readCtrlParams(root, &ctrlParams));
    TEST_ASSERT_EQUAL_FLOAT(77.25, ctrlParams.setpoint);
    TEST_ASSERT_EQUAL_FLOAT(80, ctrlParams.P_gain);
    TEST_ASSERT_EQUAL_FLOAT(2, ctrlParams.I_gain);
    TEST_ASSERT_EQUAL_FLOAT(8, ctrlParams.D_gain);

    // Missing opening brace
    const char* missingOpeningBrace = "\"type\":\"INFO\",\"subtype\":\"ctrlParams\",\"data\":{\"setpoint\":77.25,\"P_gain\":80,\"I_gain\":2,\"D_gain\":8}}";
    root = NULL;
    root = cJSON_Parse(missingOpeningBrace);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL_INT(ESP_FAIL, readCtrlParams(root, &ctrlParams));

    // Missing closing brace
    const char* missingClosingBrace = "{\"type\":\"INFO\",\"subtype\":\"ctrlParams\",\"data\":{\"setpoint\":77.25,\"P_gain\":80,\"I_gain\":2,\"D_gain\":8}";
    root = NULL;
    root = cJSON_Parse(missingClosingBrace);
    TEST_ASSERT_EQUAL_INT(ESP_FAIL, readCtrlParams(root, &ctrlParams));

    // Missing field
    const char* missingField = "{\"type\":\"INFO\",\"subtype\":\"ctrlParams\",\"data\":{\"setpoint\":77.25,\"I_gain\":2,\"D_gain\":8}}";
    root = NULL;
    root = cJSON_Parse(missingField);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL_INT(ESP_FAIL, readCtrlParams(root, &ctrlParams));

    // Null pointer
    root = NULL;
    TEST_ASSERT_EQUAL_INT(ESP_FAIL, readCtrlParams(root, &ctrlParams));
}

TEST_CASE("readCtrlSettings", "[Messages]")
{
    // Test valid message
    ctrlSettings_t ctrlSettings = {};
    const char* validMsg = "{\"type\":\"INFO\",\"subtype\":\"ctrlSettings\",\"data\":{\"fanState\":0,\"flush\":1,\"elementLow\":0,\"elementHigh\":0,\"prodCondensor\":0}}";
    cJSON* root = NULL;
    root = cJSON_Parse(validMsg);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL_INT(ESP_OK, readCtrlSettings(root, &ctrlSettings));
    TEST_ASSERT_EQUAL_INT(0, ctrlSettings.fanState);
    TEST_ASSERT_EQUAL_INT(1, ctrlSettings.flush);
    TEST_ASSERT_EQUAL_INT(0, ctrlSettings.elementLow);
    TEST_ASSERT_EQUAL_INT(0, ctrlSettings.elementHigh);
    TEST_ASSERT_EQUAL_INT(0, ctrlSettings.prodCondensor);

    // Missing opening brace
    const char* missingOpeningBrace = "\"type\":\"INFO\",\"subtype\":\"ctrlSettings\",\"data\":{\"fanState\":0,\"flush\":1,\"elementLow\":0,\"elementHigh\":0,\"prodCondensor\":0}}";
    root = NULL;
    root = cJSON_Parse(missingOpeningBrace);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL_INT(ESP_FAIL, readCtrlSettings(root, &ctrlSettings));

    // Missing closing brace
    const char* missingClosingBrace = "{\"type\":\"INFO\",\"subtype\":\"ctrlSettings\",\"data\":{\"fanState\":0,\"flush\":1,\"elementLow\":0,\"elementHigh\":0,\"prodCondensor\":0}";
    root = NULL;
    root = cJSON_Parse(missingClosingBrace);
    TEST_ASSERT_EQUAL_INT(ESP_FAIL, readCtrlSettings(root, &ctrlSettings));

    // Missing field
    const char* missingField = "{\"type\":\"INFO\",\"subtype\":\"ctrlSettings\",\"data\":{\"fanState\":0,\"elementLow\":0,\"elementHigh\":0,\"prodCondensor\":0}}";
    root = NULL;
    root = cJSON_Parse(missingField);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL_INT(ESP_FAIL, readCtrlSettings(root, &ctrlSettings));

    // Null pointer
    root = NULL;
    TEST_ASSERT_EQUAL_INT(ESP_FAIL, readCtrlSettings(root, &ctrlSettings));
}

TEST_CASE("readTempSensorParams", "[Messages]")
{
    // Test valid message
    DS18B20_t sensor = {};
    const char* validMsg = "{\"type\":\"INFO\",\"subtype\":\"ASSIGN\",\"data\":{\"addr\":[1,2,3,4,5,6,7,8],\"task\":2}}";
    cJSON* root = NULL;
    root = cJSON_Parse(validMsg);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL_INT(ESP_OK, readTempSensorParams(root, &sensor));
    TEST_ASSERT_EQUAL_UINT8(1, sensor.addr.bytes[0]);
    TEST_ASSERT_EQUAL_UINT8(2, sensor.addr.bytes[1]);
    TEST_ASSERT_EQUAL_UINT8(3, sensor.addr.bytes[2]);
    TEST_ASSERT_EQUAL_UINT8(4, sensor.addr.bytes[3]);
    TEST_ASSERT_EQUAL_UINT8(5, sensor.addr.bytes[4]);
    TEST_ASSERT_EQUAL_UINT8(6, sensor.addr.bytes[5]);
    TEST_ASSERT_EQUAL_UINT8(7, sensor.addr.bytes[6]);
    TEST_ASSERT_EQUAL_UINT8(8, sensor.addr.bytes[7]);
    TEST_ASSERT_TRUE(sensor.task == T_prod);
    
    // Test corrupt sensor address
    const char* corruptAddr = "{\"type\":\"INFO\",\"subtype\":\"ASSIGN\",\"data\":{\"addr\":[1,2,3,\"a\",5,6,7,8],\"task\":2}}";
    root = NULL;
    root = cJSON_Parse(corruptAddr);
    TEST_ASSERT_EQUAL_INT(ESP_FAIL, readTempSensorParams(root, &sensor));

    // Test no array
    const char* noArray = "{\"type\":\"INFO\",\"subtype\":\"ASSIGN\",\"data\":{\"addr\":4,\"task\":2}}";
    root = NULL;
    root = cJSON_Parse(noArray);
    TEST_ASSERT_EQUAL_INT(ESP_FAIL, readTempSensorParams(root, &sensor));

    // Test invalid message
    const char* invalidMsg = "\"type\":\"INFO\",\"subtype\":\"ASSIGN\",\"data\":{\"addr\":[1,2,3,4,5,6,7,8],\"task\":2}}";        // Missing opening brace
    root = NULL;
    root = cJSON_Parse(invalidMsg);
    TEST_ASSERT_EQUAL_INT(ESP_FAIL, readTempSensorParams(root, &sensor));

    // Task not a number
    const char* invalidTask = "{\"type\":\"INFO\",\"subtype\":\"ASSIGN\",\"data\":{\"addr\":[1,2,3,4,5,6,7,8],\"task\":\"a\"}}";
    root = NULL;
    root = cJSON_Parse(invalidTask);
    TEST_ASSERT_EQUAL_INT(ESP_FAIL, readTempSensorParams(root, &sensor));

    // Null pointer
    root = NULL;
    TEST_ASSERT_EQUAL_INT(ESP_FAIL, readTempSensorParams(root, &sensor));
}