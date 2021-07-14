// #include "WebServerMessaging.h"

// PBRet SocketLogMessage::serialize(std::string &JSONStr) const
// {
//     // Serialize log message to JSON
//     cJSON* root = cJSON_CreateObject();
//     if (root == nullptr) {
//         ESP_LOGW(SocketLogMessage::Name, "Unable to create root JSON object");
//         return PBRet::FAILURE;
//     }

//     if (cJSON_AddStringToObject(root, "MessageType", SocketLogMessage::Name) == nullptr)
//     {
//         ESP_LOGW(SocketLogMessage::Name, "Unable to add MessageType to flowrate data JSON string");
//         cJSON_Delete(root);
//         return PBRet::FAILURE;
//     }

//     if (cJSON_AddStringToObject(root, SocketLogMessage::SocketLogStr, msg.c_str()) == nullptr)
//     {
//         ESP_LOGW(SocketLogMessage::Name, "Unable to add log message to SocketLogMessage JSON string");
//         cJSON_Delete(root);
//         return PBRet::FAILURE;
//     }

//     char* stringPtr = cJSON_Print(root);
//     JSONStr = std::string(stringPtr);
//     cJSON_Delete(root);
//     free(stringPtr);

//     return PBRet::SUCCESS;
// }