#ifndef MAIN_WEBSERVER_MESSAGING_H
#define MAIN_WEBSERVER_MESSAGING_H

#include "PBCommon.h"
#include "MessageDefs.h"
#include "cJSON.h"

class SocketLogMessage : public MessageBase
{
    static constexpr MessageType messageType = MessageType::SocketLog;
    static constexpr const char* Name = "Socket Log";
    static constexpr const char* SocketLogStr = "Log";

public:
    SocketLogMessage(void) = default;
    SocketLogMessage(const std::string& msg)
        : MessageBase(SocketLogMessage::messageType, SocketLogMessage::Name, esp_timer_get_time()),
          msg(msg) {}

    PBRet serialize(std::string &JSONStr) const;
    
    std::string msg {};
};

#endif // MAIN_WEBSERVER_MESSAGING_H