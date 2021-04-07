#include "OneWireBusMessaging.h"
#include <string>

PBRet TemperatureData::serialize(std::string &JSONStr) const
{
    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        ESP_LOGW(TemperatureData::Name, "Unable to create root JSON object");
        return PBRet::FAILURE;
    }

    if (cJSON_AddStringToObject(root, "MessageType", TemperatureData::Name) == nullptr)
    {
        ESP_LOGW(TemperatureData::Name, "Unable to add MessageType to temperature JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    if (cJSON_AddNumberToObject(root, TemperatureData::HeadTempStr, headTemp) == nullptr) {
        ESP_LOGW(TemperatureData::Name, "Unable to add head temp to temperature JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    if (cJSON_AddNumberToObject(root, TemperatureData::RefluxCondensorTempStr, refluxCondensorTemp) == nullptr) {
        ESP_LOGW(TemperatureData::Name, "Unable to add reflux condensor temp to temperature JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    if (cJSON_AddNumberToObject(root, TemperatureData::ProdCondensorTempStr, prodCondensorTemp) == nullptr) {
        ESP_LOGW(TemperatureData::Name, "Unable to add product condensor temp to temperature JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    if (cJSON_AddNumberToObject(root, TemperatureData::RadiatorTempStr, radiatorTemp) == nullptr) {
        ESP_LOGW(TemperatureData::Name, "Unable to add radiator temp to temperature JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    if (cJSON_AddNumberToObject(root, TemperatureData::BoilerTempStr, boilerTemp) == nullptr) {
        ESP_LOGW(TemperatureData::Name, "Unable to add boiler temp to temperature JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    if (cJSON_AddNumberToObject(root, TemperatureData::UptimeStr, _timeStamp) == nullptr) {
        ESP_LOGW(TemperatureData::Name, "Unable to add timestamp to temperature JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    char* stringPtr = cJSON_Print(root);
    JSONStr = std::string(stringPtr);
    cJSON_Delete(root);
    free(stringPtr);

    return PBRet::SUCCESS;
}