#ifndef CppTASK_H
#define CppTASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

class Task
{
    public:
        // Constructors
        Task(const char* taskName, UBaseType_t priority, UBaseType_t stackDepth, BaseType_t coreID)
            : _name(taskName), _priority(priority), _stackDepth(stackDepth), _coreID(coreID) {}
        virtual ~Task(void) = default;

        void begin(void) { start(); }

    protected:
        virtual void taskMain(void) = 0;

        void start(void) {
            if (_name == nullptr) {
                printf("Task name was null\n");
                return;
            }

            printf("Creating task: %s\n", _name);
            xTaskCreatePinnedToCore(&runTask, _name, _stackDepth, this, _priority, &_taskHandle, _coreID);
        }

        xTaskHandle getTaskHandle(void) const { return _taskHandle; }

    private:
        static void runTask(void* taskPtr)
        {
            if (taskPtr != nullptr) {
                Task* task = (Task*) taskPtr;
                task->taskMain();
            } else {
                printf("Task object was null\n");
                vTaskDelete(NULL);
            }
            

            // Once task has returned, kill the task properly
        }

        xTaskHandle _taskHandle {};
        const char* _name = nullptr;
        UBaseType_t _priority {};
        UBaseType_t _stackDepth {};
        BaseType_t _coreID {};
};

#ifdef __cplusplus
}
#endif

#endif // CppTASK_H