#ifndef MAIN_MESSAGE_DEFS_H
#define MAIN_MESSAGE_DEFS_H

#include <string>
#include "messageServer.h"

class GeneralMessage : public MessageBase
{
    static constexpr MessageType messageType = MessageType::General;

    public:
        GeneralMessage(void) = default;
        GeneralMessage(const std::string& message)
            : MessageBase(GeneralMessage::messageType), _message(message) 
        {
            _name = "General Message";
        }

        const std::string& getMessage(void) const { return _message; }
    private:
        std::string _message {};
};

#endif // MAIN_MESSAGE_DEFS_H