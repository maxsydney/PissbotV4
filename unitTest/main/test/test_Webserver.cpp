#include "unity.h"
#include "main/WebServer.h"
#include "testWebServerConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

void includeWebserverTests(void)
{
    // Dummy function to force discovery of unit tests by main test runner
}

static WebserverConfig validConfig(void)
{
    WebserverConfig cfg {};
    cfg.maxConnections = 1;
    cfg.maxBroadcastFreq = 1;

    return cfg;
}

TEST_CASE("checkInputs", "[Webserver]")
{
    // Default configuration invalid
    {
        WebserverConfig cfg {};
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Webserver::checkInputs(cfg));
    }

    // Valid config
    {
        WebserverConfig cfg = validConfig();
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, Webserver::checkInputs(cfg));
    }

    // Invalid maxConnections
    {
        WebserverConfig cfg = validConfig();
        cfg.maxConnections = -1;
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Webserver::checkInputs(cfg));
    }

    // Invalid maxBroadcastFreq
    {
        WebserverConfig cfg = validConfig();
        cfg.maxBroadcastFreq = -1;
        TEST_ASSERT_EQUAL(PBRet::FAILURE, Webserver::checkInputs(cfg));
    }
}

TEST_CASE("loadFromJSONValid", "[Webserver]")
{
    WebserverConfig testConfig {};
    cJSON* root = cJSON_Parse(webserverConfig);
    TEST_ASSERT_NOT_EQUAL(root, nullptr);
 
    // Get config node
    cJSON* cfg = cJSON_GetObjectItem(root, "ValidWebserverConfig");
    TEST_ASSERT_NOT_EQUAL(cfg, nullptr);

    TEST_ASSERT_EQUAL(PBRet::SUCCESS, Webserver::loadFromJSON(testConfig, cfg));
}

TEST_CASE("loadFromJSONInvalid", "[Webserver]")
{
    WebserverConfig testConfig {};
    cJSON* root = cJSON_Parse(webserverConfig);
    TEST_ASSERT_NOT_EQUAL(root, nullptr);
 
    // Get config node
    cJSON* cfg = cJSON_GetObjectItem(root, "InvalidWebserverConfig");
    TEST_ASSERT_NOT_EQUAL(cfg, nullptr);

    TEST_ASSERT_EQUAL(PBRet::FAILURE, Webserver::loadFromJSON(testConfig, cfg));
}

#ifdef __cplusplus
}
#endif
