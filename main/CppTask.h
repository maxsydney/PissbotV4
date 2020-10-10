#ifndef CppTASK_H
#define CppTASK_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <map>
#include <queue>
#include "messageServer.h"
#include "functional"

class Task
{
    static constexpr const char* Name = "Task";
    
    public:
        // Constructors
        Task(const char* taskName, UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID)
            : _name(taskName), _priority(priority), _stackDepth(stackDepth), _coreID(coreID) {}
        Task(const char* taskName, UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID, std::queue<std::shared_ptr<MessageBase>>& GPQueue)
            : _name(taskName), _priority(priority), _stackDepth(stackDepth), _coreID(coreID), _GPQueue(GPQueue) {}
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
        std::map<MessageType, std::function<PBRet(MessageBase*)>> _cbTable {};

        // Configuration data
        xTaskHandle _taskHandle {};
        const char* _name = nullptr;
        UBaseType_t _priority {};
        UBaseType_t _stackDepth {};
        BaseType_t _coreID {};

        // Use c++ queues instead of FreeRTOS queues
        std::queue<std::shared_ptr<MessageBase>> _GPQueue {};

    private:
        static void runTask(void* taskPtr);
};

#endif // CppTASK_H