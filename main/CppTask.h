#ifndef CppTASK_H
#define CppTASK_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <map>
#include <queue>
#include "MessageServer.h"
#include "functional"

using queueCallback = std::function<PBRet(const std::shared_ptr<PBMessageWrapper>&)>;
using CBTable = std::map<PBMessageType, queueCallback>;

class Task
{
    static constexpr const char* Name = "Task";
    
    public:
        // Constructors
        Task(const char* taskName, UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID)
            : _name(taskName), _priority(priority), _stackDepth(stackDepth), _coreID(coreID) {}
        virtual ~Task(void) = default;

        void begin(void) { start(); }

    protected:
        virtual void taskMain(void) = 0;

        void start(void);
        xTaskHandle getTaskHandle(void) const { return _taskHandle; }

    protected:
    
        // Interface methods
        virtual PBRet _setupCBTable(void) = 0;

        // Queue handling
        PBRet _processQueue(void);

        // Task callback table. When messages arrive in the general purpose
        // queue, this table maps the message type to a callback function
        // to process it
        CBTable _cbTable {};

        // Configuration data
        xTaskHandle _taskHandle {};
        const char* _name = nullptr;
        UBaseType_t _priority {};
        UBaseType_t _stackDepth {};
        BaseType_t _coreID {};

        // Use c++ queues instead of FreeRTOS queues
        std::queue<std::shared_ptr<PBMessageWrapper>> _GPQueue {};

    private:
        static void runTask(void* taskPtr);
};

#endif // CppTASK_H