#include "CppTask.h"
#include "MessageDefs.h"

void Task::start(void) 
{
    if (_name == nullptr) {
        ESP_LOGW(Task::Name, "Task name was null");
        return;
    }

    xTaskCreatePinnedToCore(&runTask, _name, _stackDepth, this, _priority, &_taskHandle, _coreID);
}

void Task::runTask(void* taskPtr)
{
    if (taskPtr != nullptr) {
        Task* task = (Task*) taskPtr;
        task->taskMain();
    } else {
        ESP_LOGW(Task::Name, "Task object was null\n");
        vTaskDelete(NULL);
    }
    

    // Once task has returned, kill the task properly
}

PBRet Task::_processQueue(void)
{
    for (size_t i = 0; i < _GPQueue.size(); i++) {
        const std::shared_ptr<MessageBase> msg = _GPQueue.front();
        _GPQueue.pop();
        
        MessageType type = msg->getType();
        CBTable::iterator it = _cbTable.find(type);
        if (it != _cbTable.end()) {
            if (it->second(msg) == PBRet::FAILURE) {
                ESP_LOGW(_name, "Callback failed for %s", msg->getName().c_str());
            }
        } else {
            ESP_LOGW(_name, "No callback defined for message type: %s", msg->getName().c_str());
        }
    }
    
    return PBRet::SUCCESS;
}