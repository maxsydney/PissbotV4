#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include <string.h>
#include "messages.h"
#include "sensors.h"
#include "ds18b20.h"
#include "networking.h"
#include "controlLoop.h"
#include "cJSON.h"
#include "ota.h"

#define MAX_LINE_LEN 40
#define MAX_KEY_LEN 10
#define MAX_VALUE_LEN 5

// static char tag[] = "Messages";

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

void printMessages(Message* head)
{
	while (head) {
		printf("%s: %.2f\n", head->key, head->value);
		head = head->next;
	}
}

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

ctrlParams_t* readCtrlParams(cJSON* JSON_root)
{
    ctrlParams_t* ctrlParams = malloc(sizeof(ctrlParams_t));
    ctrlParams->setpoint = cJSON_GetObjectItem(JSON_root, "setpoint")->valuedouble;
    ctrlParams->P_gain = cJSON_GetObjectItem(JSON_root, "P_gain")->valuedouble;
    ctrlParams->I_gain = cJSON_GetObjectItem(JSON_root, "I_gain")->valuedouble;
    ctrlParams->D_gain = cJSON_GetObjectItem(JSON_root, "D_gain")->valuedouble;

    return ctrlParams;    
}

ctrlSettings_t* readCtrlSettings(cJSON* JSON_root)
{
    ctrlSettings_t* ctrlSettings = malloc(sizeof(ctrlSettings_t));
    ctrlSettings->fanState = cJSON_GetObjectItem(JSON_root, "fanState")->valueint;
    ctrlSettings->flush = cJSON_GetObjectItem(JSON_root, "flush")->valueint;
    ctrlSettings->elementLow = cJSON_GetObjectItem(JSON_root, "elementLow")->valueint;
    ctrlSettings->elementHigh = cJSON_GetObjectItem(JSON_root, "elementHigh")->valueint;
    ctrlSettings->prodCondensor = cJSON_GetObjectItem(JSON_root, "prodCondensor")->valueint;

    return ctrlSettings;
}

Cmd_t decodeCommand(char* commandPacket)
{
    char* command = strtok(commandPacket, ":");
    char* arg = strtok(NULL, "&\n");
    Cmd_t cmd;
    strncpy(cmd.cmd, command, CMD_LEN);
    strncpy(cmd.arg, arg, ARG_LEN);
    printf("Command: %s\nArg: %s\n", command, arg);

    return cmd;
}
