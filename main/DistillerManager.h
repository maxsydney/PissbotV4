#ifndef MAIN_DISTILLERMANAGER_H
#define MAIN_DISTILLERMANAGER_H

#include "PBCommon.h"
#include "CppTask.h"
#include "Controller.h"
#include "WebServer.h"
#include "driver/gpio.h"
#include "SensorManager.h"
#include "Generated/MessageBase.h"

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

        gpio_num_t LEDGPIO = (gpio_num_t) GPIO_NUM_NC;
};

// TODO: With robust inter-task communication, tasks shouldn't need to get a pointer
//       the DistillerManager. Consider making a regular class instead of a singleton
class DistillerManager : public Task
{
    static constexpr const char* Name = "Distiller Manager";
    static constexpr const double HeartBeatPeriod = 5000;   // [ms]
    static constexpr MessageOrigin ID = MessageOrigin::DistillerManager;

    public:
        // Delete copy and assignment constructors
        DistillerManager(DistillerManager &other) = delete;
        void operator=(const DistillerManager &) = delete;

        // Access methods
        static DistillerManager* getInstance(UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, const DistillerConfig& cfg);
        static DistillerManager* getInstance(void);

        static PBRet checkInputs(const DistillerConfig& cfg);
        static PBRet loadFromJSON(DistillerConfig& cfg, const cJSON* cfgRoot);

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
        PBRet _socketLogCB(std::shared_ptr<PBMessageWrapper> msg);

        // Pointer to singleton object
        static DistillerManager* _managerPtr;

        PBRet _doHeartbeat(void);

        // Class data
        bool _configured = false;
        double _lastHeartbeatTime = 0.0;     // [ms]
        gpio_num_t _LEDGPIO = (gpio_num_t) GPIO_NUM_NC;
        std::unique_ptr<Webserver> _webserver;
        std::unique_ptr<Controller> _controller;
        std::unique_ptr<SensorManager> _sensorManager;
};

#endif // MAIN_DISTILLERMANAGER_H