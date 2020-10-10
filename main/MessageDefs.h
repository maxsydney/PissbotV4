#ifndef MAIN_MESSAGE_DEFS_H
#define MAIN_MESSAGE_DEFS_H

#include <string>
#include "messageServer.h"

class GeneralMessage : public MessageBase
{
    static constexpr MessageType messageType = MessageType::General;
    static constexpr const char* Name = "General Message";

    public:
        GeneralMessage(void) = default;
        GeneralMessage(const std::string& message)
            : MessageBase(GeneralMessage::messageType, GeneralMessage::Name), _message(message) {}

        const std::string& getMessage(void) const { return _message; }
        
    private:
        std::string _message {};

};

#endif // MAIN_MESSAGE_DEFS_H