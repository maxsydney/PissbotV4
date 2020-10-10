#ifndef MAIN_DISTILLERMANAGER_H
#define MAIN_DISTILLERMANAGER_H

#include "PBCommon.h"
#include "CppTask.h"
#include "controller.h"
#include "MessageDefs.h"

class DistillerConfig
{
    public:
        ControllerConfig ctrlConfig {};
};

// Main system manager class. This class is a singleton and can be accessed
// using the DistillerManager::getInstance() method. The DistillerManager is
// responsible for initializing the system, and managing high level communication
// between tasks
class DistillerManager : public Task
{
    static constexpr char* Name = "Distiller Manager";

    public:
        // Delete copy and assignment constructors
        DistillerManager(DistillerManager &other) = delete;
        void operator=(const DistillerManager &) = delete;

        // Access methods
        static DistillerManager* getInstance(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const DistillerConfig& cfg);
        static DistillerManager* getInstance(void);

        // Getters
        bool isConfigured(void) const { return _configured; }

        // FreeRTOS hook method
        void taskMain(void) override;
       
    private:
        // Private constructor
        DistillerManager(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const DistillerConfig& cfg);

        // Setup methods
        PBRet _setupCBTable(void) override;

        // Queue callbacks
        PBRet _generalMessagCB(std::shared_ptr<MessageBase> msg);

        // Pointer to singleton object
        static DistillerManager* _managerPtr;

        // Class data
        bool _configured = false;
        DistillerConfig _cfg {};
        Controller* _controller {};
};

#endif // MAIN_DISTILLERMANAGER_H