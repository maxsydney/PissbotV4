#ifndef MAIN_DISTILLERMANAGER_H
#define MAIN_DISTILLERMANAGER_H

#include "CppTask.h"
#include "controller.h"

// Main system manager class. Monitors all sub components
class DistillerManager : public Task
{
    static constexpr char* name = "Distiller Manager";

    public:
        DistillerManager(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, int test);
        void taskMain(void) override;
       
    private:
        int _testInt = 0;
        int _testInt2 = 5;
        double _testDouble = .1234;
        Controller _controller {};

};

#endif // MAIN_DISTILLERMANAGER_H