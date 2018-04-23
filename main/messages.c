#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include "messages.h"
#include "networking.h"
#include "controller.h"

#define MAX_LINE_LEN 40
#define MAX_KEY_LEN 10
#define MAX_VALUE_LEN 5

Message* parseMessage(char* dataPacket, uint32_t len)
{
    Message* head = NULL;
    Message* curr = NULL;
    char* pair;
    
    pair = strtok(dataPacket, ",");

    while (pair != NULL) {
        if (head == NULL) {
            head = processPair(pair);
            curr = head;
        } else {
            curr->next = processPair(pair);
            curr = curr->next;
        }
        pair = strtok(NULL, ",");
    }

    return head;
}

/*
*   --------------------------------------------------------------------  
*   processPair
*   --------------------------------------------------------------------
*   Takes a line if text in the format "key:pair\n" as input, creates a
*   new message struct and fills the appropriate fields
*/

Message* processPair(char* pair)
{   
    int i = 0;
    float value = 0;
    char valueBuf[MAX_VALUE_LEN];
    char* name = malloc(MAX_KEY_LEN * sizeof(char));
    Message* message = malloc(sizeof(Message));

    while (*pair != ':') {
        name[i++] = *pair;
        pair++;
    }
    name[i] = '\0';

    i = 0;
    pair++;

    while (*pair) {
        valueBuf[i++] = *pair;
        pair++;
    }
    valueBuf[i] = '\0';

	value = atof(valueBuf);

    message->key = name;
    message->value = value;
    message->next = NULL;

    return message;
}

/*
*   --------------------------------------------------------------------  
*   printMessages
*   --------------------------------------------------------------------
*   Iterates through linked list of message key:value data pairs and
* 	prints them to the console.
*/

void printMessages(Message* head)
{
	while (head) {
		printf("%s: %.2f\n", head->key, head->value);
		head = head->next;
	}
}

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

float findByKey(Message* head, char* key)
{
	while (head) {
		if (strncmp(key, head->key, MAX_KEY_LEN) == 0) {
			return head->value;
		}
		head = head->next;
	}
	return 0;
}

int freeMessages(Message* head)
{
	Message* curr;
	while (head) {
		curr = head;
		free(head->key);
		head = head->next;
		free(curr);
	}
	return 1;
}

/*
*   --------------------------------------------------------------------  
*   decode_data
*   --------------------------------------------------------------------
*   Decodes incoming socket messages. Messages can be either of type
*   INFO or CONN. CONN messages signify a new connection and prompt the
*   server to begin serving data. INFO messages contain a packet of 
*   updated controller settings. 
*/
Data* decode_data(char* dataPacket)
{
    
    uint16_t len = strlen(dataPacket) - 1; 
    Message* head = parseMessage(dataPacket, len);
    if (head == NULL) {
        printf("Could not read message\n");
        return 0;
    }

    Data* data = malloc(sizeof(Data));
    data->setpoint = findByKey(head, "setpoint");
    data->P_gain = findByKey(head, "P");
    data->I_gain = findByKey(head, "I");
    data->D_gain = findByKey(head, "D");
    freeMessages(head);
    return data;    
}