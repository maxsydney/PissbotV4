#include <stdio.h>
#include "DistillerManager.h"

DistillerManager::DistillerManager(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, int test)
    : Task(DistillerManager::name, priority, stackDepth, coreID), _testInt(test) 
    {
        printf("In constructor: %d\n", _testInt);
    }

void DistillerManager::taskMain(void)
{
    printf("Starting up the main task");
    _testInt2 = 3;

    while(1) {
        printf("Test is working: %d - %d - %lf\n", _testInt, _testInt2, _testDouble);
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}