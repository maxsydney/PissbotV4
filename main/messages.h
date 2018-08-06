#pragma once
#include "controller.h"

typedef struct message_S {
    char* key;
    double value;
    struct message_S* next;
} Message;

Message* parseMessage(char* dataPacket, uint32_t len);
Message* processPair(char* pair);
void printMessages(Message* head);
float findByKey(Message* head, char* key);
int freeMessages(Message* head);
Data* decode_data(char* dataPacket);
esp_err_t decodeCommand(char* commandPacket);