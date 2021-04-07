#include "SensorManagerMessaging.h"

PBRet FlowrateData::serialize(std::string& JSONStr) const
{
    // Serialize the FlowrateDate object into a JSON string
    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        ESP_LOGW(FlowrateData::Name, "Unable to create root JSON object");
        return PBRet::FAILURE;
    }

    if (cJSON_AddStringToObject(root, "MessageType", FlowrateData::Name) == nullptr)
    {
        ESP_LOGW(FlowrateData::Name, "Unable to add MessageType to flowrate data JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    // Add reflux flowrate
    cJSON* refluxNode = cJSON_CreateNumber(refluxFlowrate);
    if (refluxNode == nullptr) {
        ESP_LOGW(FlowrateData::Name, "Error creating reflux flowrate JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, FlowrateData::RefluxFlowrateStr, refluxNode);

    // Add product flowrate
    cJSON* productNode = cJSON_CreateNumber(productFlowrate);
    if (productNode == nullptr) {
        ESP_LOGW(FlowrateData::Name, "Error creating product flowrate JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, FlowrateData::ProductFlowrateStr, productNode);

    // Add timestamp
    cJSON* timestampNode = cJSON_CreateNumber(_timeStamp);
    if (timestampNode == nullptr) {
        ESP_LOGW(FlowrateData::Name, "Error creating timestamp JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, MessageBase::TimeStampStr, timestampNode);

    // Copy JSON to string. cJSON requires printing to a char* pointer. Copy into
    // std::string and free memory to avoid memory leak
    char* stringPtr = cJSON_Print(root);
    JSONStr = std::string(stringPtr);
    cJSON_Delete(root);
    free(stringPtr);

    return PBRet::SUCCESS;
}

PBRet ConcentrationData::serialize(std::string& JSONStr) const
{
    // Serialize the ConcentrationData object into a JSON string
    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        ESP_LOGW(ConcentrationData::Name, "Unable to create root JSON object");
        return PBRet::FAILURE;
    }

    if (cJSON_AddStringToObject(root, "MessageType", ConcentrationData::Name) == nullptr)
    {
        ESP_LOGW(ConcentrationData::Name, "Unable to add MessageType to concenctration data JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    // Add vapour concentration
    cJSON* vapourConcNode = cJSON_CreateNumber(vapourConcentration);
    if (vapourConcNode == nullptr) {
        ESP_LOGW(ConcentrationData::Name, "Error creating vapour concentration JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, ConcentrationData::VapourConcStr, vapourConcNode);

    // Add boiler concentration
    cJSON* boilerConcNode = cJSON_CreateNumber(boilerConcentration);
    if (boilerConcNode == nullptr) {
        ESP_LOGW(ConcentrationData::Name, "Error creating boiler concentration JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, ConcentrationData::BoilerConcStr, boilerConcNode);

    char* stringPtr = cJSON_Print(root);
    JSONStr = std::string(stringPtr);
    cJSON_Delete(root);
    free(stringPtr);

    return PBRet::SUCCESS;
}