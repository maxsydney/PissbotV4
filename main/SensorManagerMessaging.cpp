#include "SensorManagerMessaging.h"

// TODO: Unit test me
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
    if (cJSON_AddNumberToObject(root, FlowrateData::RefluxFlowrateStr, refluxFlowrate) == nullptr) {
        ESP_LOGW(FlowrateData::Name, "Unable to add reflux flowrate to FlowrateData JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    // Add product flowrate
    if (cJSON_AddNumberToObject(root, FlowrateData::ProductFlowrateStr, productFlowrate) == nullptr) {
        ESP_LOGW(FlowrateData::Name, "Unable to add product flowrate to FlowrateData JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    // Add timestamp
    if (cJSON_AddNumberToObject(root, MessageBase::TimeStampStr, _timeStamp) == nullptr) {
        ESP_LOGW(FlowrateData::Name, "Unable to add timestamp to FlowrateData JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    char* stringPtr = cJSON_Print(root);
    JSONStr = std::string(stringPtr);
    cJSON_Delete(root);
    free(stringPtr);

    return PBRet::SUCCESS;
}

PBRet FlowrateData::deserialize(const cJSON *root)
{
    // TODO: Implement me

    ESP_LOGW(FlowrateData::Name, "FlowrateData deserialization not implemented");

    return PBRet::FAILURE;
}

// TODO: Unit test me
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
    if (cJSON_AddNumberToObject(root, ConcentrationData::VapourConcStr, vapourConcentration) == nullptr) {
        ESP_LOGW(ConcentrationData::Name, "Unable to add vapour concentration to ConcentrationData JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    // Add boiler concentration
    if (cJSON_AddNumberToObject(root, ConcentrationData::BoilerConcStr, boilerConcentration) == nullptr) {
        ESP_LOGW(ConcentrationData::Name, "Unable to add boiler concentration to ConcentrationData JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    char* stringPtr = cJSON_Print(root);
    JSONStr = std::string(stringPtr);
    cJSON_Delete(root);
    free(stringPtr);

    return PBRet::SUCCESS;
}

PBRet ConcentrationData::deserialize(const cJSON *root)
{
    // TODO: Implement me

    ESP_LOGW(ConcentrationData::Name, "ConcentrationData deserialization not implemented");

    return PBRet::FAILURE;
}

PBRet AssignSensorCommand::serialize(std::string &JSONStr) const
{
    // TODO: Implement me

    ESP_LOGW(AssignSensorCommand::Name, "AssignSensorCommand serialization not implemented");

    return PBRet::FAILURE;
}

PBRet AssignSensorCommand::deserialize(const cJSON *root)
{
    // TODO: Implement me

    ESP_LOGW(AssignSensorCommand::Name, "AssignSensorCommand deserialization not implemented");

    return PBRet::FAILURE;
}

PBRet SensorManagerCommand::serialize(std::string &JSONStr) const
{
    // TODO: Implement me

    ESP_LOGW(SensorManagerCommand::Name, "SensorManagerCommand serialization not implemented");

    return PBRet::FAILURE;
}

PBRet SensorManagerCommand::deserialize(const cJSON *root)
{
    // TODO: Implement me

    ESP_LOGW(SensorManagerCommand::Name, "SensorManagerCommand serialization not implemented");

    return PBRet::FAILURE;
}