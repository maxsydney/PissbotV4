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
        MessageType type = msg->getType();
        if (type == MessageType::General) {
            std::shared_ptr<GeneralMessage> genMsg =  std::static_pointer_cast<GeneralMessage>(msg);
            ESP_LOGI(Task::Name, "Received message: %s", genMsg->getMessage().c_str());
            _GPQueue.pop();
        }
            // std::map<MessageType, std::function<PBRet(MessageBase*)>>::iterator it;
            // if ((it = _cbTable.find(type)) != _cbTable.end()) {
            //     it->second(msg);
            // } else {
            //     ESP_LOGW(Task::Name, "No callback defined for message type: %s", msg->getName().c_str());
            // }
        // }
    }
    return PBRet::SUCCESS;
}