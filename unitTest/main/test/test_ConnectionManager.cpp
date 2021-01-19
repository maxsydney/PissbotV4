#include <stdio.h>
#include <algorithm>
#include "unity.h"
#include "main/ConnectionManager.h"

#ifdef __cplusplus
extern "C" {
#endif

class ConnectionManagerUT
{
    public:

        // Reset static memory between tests
        static void clearConnections(void) { ConnectionManager::_activeWebsockets = {}; ConnectionManager::_nConnections = 0; }

        // Retrieve maximum allowable connections
        static int getMaxConnections(void) { return ConnectionManager::_maxConnections; }
};

void includeConnectionManagerTests(void)
{
    // Dummy function to force discovery of unit tests by main test runner
    printf(" ");
}

TEST_CASE("addConnection", "[ConnectionManager]")
{
    ConnectionManagerUT::clearConnections();        // Reset static memory

    // Create random uninitialized Websock pointers
    Websock* ws1 = (Websock*) 0x12345678;
    Websock* ws2 = (Websock*) 0x87654321;
    Websock* ws3 = (Websock*) 0xF00DBABE;

    // Add connections and check that they are stored
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::addConnection(ws1));
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::addConnection(ws2));
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::addConnection(ws3));

    TEST_ASSERT_EQUAL(ConnectionManager::getNumConnections(), 3);

    TEST_ASSERT_EQUAL(ws1, ConnectionManager::getActiveWebsockets().at(0));
    TEST_ASSERT_EQUAL(ws2, ConnectionManager::getActiveWebsockets().at(1));
    TEST_ASSERT_EQUAL(ws3, ConnectionManager::getActiveWebsockets().at(2));

    // TODO: Make this a separate test
    // Check unable to add more than max connections
    ConnectionManagerUT::clearConnections();
    for (int i=0; i < ConnectionManagerUT::getMaxConnections(); i++) {
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::addConnection(ws1));
    }

    TEST_ASSERT_EQUAL(PBRet::FAILURE, ConnectionManager::addConnection(ws1));
}

TEST_CASE("addManyConnections", "[ConnectionManager]")
{
    ConnectionManagerUT::clearConnections();        // Reset static memory

    // Create random uninitialized Websock pointers
    Websock* ws1 = (Websock*) 0x12345678;

    // Check unable to add more than max connections
    ConnectionManagerUT::clearConnections();
    for (int i=0; i < ConnectionManagerUT::getMaxConnections(); i++) {
        TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::addConnection(ws1));
    }

    TEST_ASSERT_EQUAL(PBRet::FAILURE, ConnectionManager::addConnection(ws1));
}

TEST_CASE("removeValidConnection", "[ConnectionManager]")
{
    ConnectionManagerUT::clearConnections();        // Reset static memory

    // Prepare ConnectionManager with some random connections
    Websock* ws1 = (Websock*) 0x12345678;
    Websock* ws2 = (Websock*) 0x87654321;
    Websock* ws3 = (Websock*) 0xF00DBABE;

    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::addConnection(ws1));
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::addConnection(ws2));
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::addConnection(ws3));

    // Remove connections one-by-one
    const std::vector<Websock*> conn = ConnectionManager::getActiveWebsockets();
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::removeConnection(ws1));
    TEST_ASSERT_EQUAL(PBRet::FAILURE, ConnectionManager::checkConnection(ws1));

    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::removeConnection(ws2));
    TEST_ASSERT_EQUAL(PBRet::FAILURE, ConnectionManager::checkConnection(ws2));

    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::removeConnection(ws3));
    TEST_ASSERT_EQUAL(PBRet::FAILURE, ConnectionManager::checkConnection(ws3));
}

TEST_CASE("removeInvalidConnection", "[ConnectionManager]")
{
    ConnectionManagerUT::clearConnections();        // Reset static memory

    // Prepare ConnectionManager with some random connections
    Websock* ws1 = (Websock*) 0x12345678;
    Websock* ws2 = (Websock*) 0x87654321;
    Websock* ws3 = (Websock*) 0xF00DBABE;
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::addConnection(ws1));

    // Try to remove a connection that doesn't exist
    TEST_ASSERT_EQUAL(PBRet::FAILURE, ConnectionManager::removeConnection(ws2));
    TEST_ASSERT_EQUAL(PBRet::FAILURE, ConnectionManager::removeConnection(ws3));
}

TEST_CASE("removeFromEmptyConnections", "[ConnectionManager]")
{
    ConnectionManagerUT::clearConnections();        // Reset static memory

    // Prepare some random connections
    Websock* ws1 = (Websock*) 0x12345678;

    // Try to remove a connection from empty ConnectionManager
    TEST_ASSERT_EQUAL(PBRet::FAILURE, ConnectionManager::removeConnection(ws1));
}

TEST_CASE("checkConnection", "[ConnectionManager]")
{
    ConnectionManagerUT::clearConnections();        // Reset static memory

    // Prepare ConnectionManager with some random connections
    Websock* ws1 = (Websock*) 0x12345678;
    Websock* ws2 = (Websock*) 0x87654321;
    Websock* ws3 = (Websock*) 0xF00DBABE;

    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::addConnection(ws1));
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::checkConnection(ws1));
    TEST_ASSERT_EQUAL(PBRet::FAILURE, ConnectionManager::checkConnection(ws2));
    TEST_ASSERT_EQUAL(PBRet::FAILURE, ConnectionManager::checkConnection(ws3));

    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::addConnection(ws2));
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::checkConnection(ws1));
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::checkConnection(ws2));
    TEST_ASSERT_EQUAL(PBRet::FAILURE, ConnectionManager::checkConnection(ws3));

    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::addConnection(ws3));
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::checkConnection(ws1));
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::checkConnection(ws2));
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::checkConnection(ws3));
}

TEST_CASE("getNumConnections", "[ConnectionManager]")
{
    ConnectionManagerUT::clearConnections();        // Reset static memory

    // Prepare ConnectionManager with some random connections
    Websock* ws1 = (Websock*) 0x12345678;
    Websock* ws2 = (Websock*) 0x87654321;
    Websock* ws3 = (Websock*) 0xF00DBABE;

    // No active connections
    TEST_ASSERT_EQUAL(0, ConnectionManager::getNumConnections());

    // 1 active connection
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::addConnection(ws1));
    TEST_ASSERT_EQUAL(1, ConnectionManager::getNumConnections());

    // 2 active connections
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::addConnection(ws2));
    TEST_ASSERT_EQUAL(2, ConnectionManager::getNumConnections());

    // 3 active connections
    TEST_ASSERT_EQUAL(PBRet::SUCCESS, ConnectionManager::addConnection(ws3));
    TEST_ASSERT_EQUAL(3, ConnectionManager::getNumConnections());
}

#ifdef __cplusplus
}
#endif