#ifndef MAIN_DISTILLERMANAGER_H
#define MAIN_DISTILLERMANAGER_H

#include "PBCommon.h"
#include "CppTask.h"
#include "controller.h"
#include "WebServer.h"
#include "SensorManager.h"
#include "MessageDefs.h"

// Main system manager class. This class is a singleton and can be accessed
// using the DistillerManager::getInstance() method. The DistillerManager is
// responsible for initializing the system, and managing high level communication
// between tasks

class DistillerConfig
{
    public:
        ControllerConfig ctrlConfig {};
        SensorManagerConfig sensorManagerConfig {};
        WebserverConfig webserverConfig {};
};

// TODO: With robust inter-task communication, tasks shouldn't need to get a pointer
//       the DistillerManager. Consider making a regular class instead of a singleton
class DistillerManager : public Task
{
    static constexpr const char* Name = "Distiller Manager";

    public:
        // Delete copy and assignment constructors
        DistillerManager(DistillerManager &other) = delete;
        void operator=(const DistillerManager &) = delete;

        // Access methods
        static DistillerManager* getInstance(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const DistillerConfig& cfg);
        static DistillerManager* getInstance(void);

        static PBRet checkInputs(const DistillerConfig& cfg);

        // Getters
        bool isConfigured(void) const { return _configured; }

        // FreeRTOS hook method
        void taskMain(void) override;
       
    private:
        // Private constructor
        DistillerManager(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const DistillerConfig& cfg);

        PBRet _initFromParams(const DistillerConfig& cfg);

        // Setup methods
        PBRet _setupCBTable(void) override;

        // Queue callbacks
        PBRet _generalMessagCB(std::shared_ptr<MessageBase> msg);

        // Pointer to singleton object
        static DistillerManager* _managerPtr;

        // Class data
        bool _configured = false;
        DistillerConfig _cfg {};
        std::unique_ptr<Webserver> _webserver;
        std::unique_ptr<Controller> _controller;
        std::unique_ptr<SensorManager> _sensorManager;
};

#endif // MAIN_DISTILLERMANAGER_H