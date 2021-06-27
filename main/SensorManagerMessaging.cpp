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

CalibrateSensorCommand::CalibrateSensorCommand(cJSON* root)
    : MessageBase(CalibrateSensorCommand::messageType, CalibrateSensorCommand::Name, esp_timer_get_time())
{
    if (deserialize(root) == PBRet::SUCCESS) {
        _valid = true;
    } else {
        ESP_LOGW(CalibrateSensorCommand::Name, "Failed to deserialize CalibrateSensorCommand");
    }

    cJSON_free(root);
}

PBRet CalibrateSensorCommand::deserialize(const cJSON *root)
{
    if (root == nullptr) {
        ESP_LOGW(CalibrateSensorCommand::Name, "Unable to parse assign sensor message. cJSON object was null");
        return PBRet::FAILURE;
    }

    ESP_LOGI(CalibrateSensorCommand::Name, "Decoding calibrate sensor command");

    cJSON* sensorData = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (sensorData == nullptr) {
        ESP_LOGW(CalibrateSensorCommand::Name, "Unable to parse sensor data from calibrate sensor command");
        return PBRet::FAILURE;
    }

    cJSON* sensorAddr = cJSON_GetObjectItemCaseSensitive(sensorData, "addr");
    if (sensorAddr == nullptr) {
        ESP_LOGW(CalibrateSensorCommand::Name, "Unable to parse sensor address from calibrate sensor command");
        return PBRet::FAILURE;
    }

    // Read address from JSON
    cJSON* byte = nullptr;
    int i = 0;
    cJSON_ArrayForEach(byte, sensorAddr)
    {
        if (cJSON_IsNumber(byte) == false) {
            ESP_LOGW(CalibrateSensorCommand::Name, "Unable to decode sensor address. Address byte was not a number");
            return PBRet::FAILURE;
        }
        address.bytes[i++] = byte->valueint;
    }

    // Read read assigned sensor calibration
    cJSON* calCoeffA = cJSON_GetObjectItemCaseSensitive(sensorData, "calCoeffA");
    if (calCoeffA == nullptr) {
        ESP_LOGW(CalibrateSensorCommand::Name, "Unable to parse linear scaling coefficient from calibrate sensor command");
        return PBRet::FAILURE;
    }
    cal.A = calCoeffA->valuedouble;

    cJSON* calCoeffB = cJSON_GetObjectItemCaseSensitive(sensorData, "calCoeffB");
    if (calCoeffB == nullptr) {
        ESP_LOGW(CalibrateSensorCommand::Name, "Unable to parse constant offset coefficient from calibrate sensor command");
        return PBRet::FAILURE;
    }
    cal.B = calCoeffB->valuedouble;

    return PBRet::SUCCESS;
}