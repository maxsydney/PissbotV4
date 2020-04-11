#pragma once
#include "controlLoop.h"
#include "cJSON.h"

#define CMD_LEN 16
#define ARG_LEN 16

typedef struct message_S {
    char* key;
    double value;
    struct message_S* next;
} Message;

typedef struct {
    char cmd[CMD_LEN];
    char arg[ARG_LEN];
} Cmd_t;

Message* parseMessage(char* dataPacket, uint32_t len);

/*
*   --------------------------------------------------------------------  
*   processPair
*   --------------------------------------------------------------------
*   Takes a line if text in the format "key:pair\n" as input, creates a
*   new message struct and fills the appropriate fields
*/
Message* processPair(char* pair);

/*
*   --------------------------------------------------------------------  
*   printMessages
*   --------------------------------------------------------------------
*   Iterates through linked list of message key:value data pairs and
* 	prints them to the console.
*/
void printMessages(Message* head);

/*
*   --------------------------------------------------------------------  
*   findByKey
*   --------------------------------------------------------------------
*   Traverses linked list and searches for a given key. If found, the 
*   corresponding value is returned. If not, 0 is returned. This 
*   can potentially introduce bugs when actual values are 0. Could
* 	implement this function with a value pointer as input and allow 
*   function to return error code.
*/
float findByKey(Message* head, char* key);

/*
*   --------------------------------------------------------------------  
*   freeMessages
*   --------------------------------------------------------------------
*   Frees memory allocated for handling of incoming messages. Must be 
*   called on all message data structures to avoid memory leaks
*/
int freeMessages(Message* head);

/*
*   --------------------------------------------------------------------  
*   decode_data
*   --------------------------------------------------------------------
*   Decodes incoming messages. Messages can be either of type
*   INFO or CONN. CONN messages signify a new connection and prompt the
*   server to begin serving data. INFO messages contain a packet of 
*   updated controller settings. 
*/
Data* decode_data(cJSON* JSON_data);

/*
*   --------------------------------------------------------------------  
*   decodeCommand
*   --------------------------------------------------------------------
*   Decodes incoming commands. Command messages contain 1 commands in the
*   format CMD&ARG and trigger routines running on the ESP32.
*/
Cmd_t decodeCommand(char* commandPacket);

