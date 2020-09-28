#ifndef MAIN_DISTILLERMANAGER_H
#define MAIN_DISTILLERMANAGER_H

#include "tasklet.hpp"
#include "controller.h"

// Main system manager class. Monitors all sub components
class DistillerManager
{
    static constexpr char* name = "Distiller Manager";

    public:
        DistillerManager(void) = default;
       
    private:

        int _testInt = 0;
        Controller _controller {};

};

#endif // MAIN_DISTILLERMANAGER_H