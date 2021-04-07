#include "ControllerMessaging.h"

PBRet ControlTuning::serialize(std::string& JSONstr) const
{
    // Write the ControlTuning object to JSON

    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        ESP_LOGW(ControlTuning::Name, "Unable to create root JSON object");
        return PBRet::FAILURE;
    }

    if (cJSON_AddStringToObject(root, "MessageType", ControlTuning::Name) == nullptr)
    {
        ESP_LOGW(ControlTuning::Name, "Unable to add MessageType to ControlTuning JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    // Add setpoint
    cJSON* setpoint = cJSON_CreateNumber(_setpoint);
    if (setpoint == nullptr) {
        ESP_LOGW(ControlTuning::Name, "Error creating setpoint JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, ControlTuning::SetpointStr, setpoint);

    // Add P gain
    cJSON* PGain = cJSON_CreateNumber(_PGain);
    if (PGain == nullptr) {
        ESP_LOGW(ControlTuning::Name, "Error creating PGain JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, ControlTuning::PGainStr, PGain);

    // Add I gain
    cJSON* IGain = cJSON_CreateNumber(_IGain);
    if (IGain == nullptr) {
        ESP_LOGW(ControlTuning::Name, "Error creating IGain JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, ControlTuning::IGainStr, IGain);

    // Add D gain
    cJSON* DGain = cJSON_CreateNumber(_DGain);
    if (DGain == nullptr) {
        ESP_LOGW(ControlTuning::Name, "Error creating IGain JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, ControlTuning::DGainStr, DGain);

    // Add LPF cutoff
    cJSON* LPFCutoff = cJSON_CreateNumber(_LPFCutoff);
    if (LPFCutoff == nullptr) {
        ESP_LOGW(ControlTuning::Name, "Error creating LPF cutoff JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, ControlTuning::LPFCutoffStr, LPFCutoff);

    // Copy JSON to string. cJSON requires printing to a char* pointer. Copy into
    // std::string and free memory to avoid memory leak
    char* stringPtr = cJSON_Print(root);
    JSONstr = std::string(stringPtr);
    cJSON_Delete(root);
    free(stringPtr);

    return PBRet::SUCCESS;
}

PBRet ControlTuning::deserialize(const cJSON* root)
{
    // Load the ControlTuning object from JSON

    if (root == nullptr) {
        ESP_LOGW(ControlTuning::Name, "root JSON object was null");
        return PBRet::FAILURE;
    }

    // Read setpoint
    cJSON* setpointNode = cJSON_GetObjectItem(root, ControlTuning::SetpointStr);
    if (cJSON_IsNumber(setpointNode)) {
        _setpoint = setpointNode->valuedouble;
    } else {
        ESP_LOGI(ControlTuning::Name, "Unable to read setpint from JSON");
        return PBRet::FAILURE;
    }

    // Read P gain
    cJSON* PGainNode = cJSON_GetObjectItem(root, ControlTuning::PGainStr);
    if (cJSON_IsNumber(PGainNode)) {
        _PGain = PGainNode->valuedouble;
    } else {
        ESP_LOGI(ControlTuning::Name, "Unable to read P gain from JSON");
        return PBRet::FAILURE;
    }

    // Read I gain
    cJSON* IGainNode = cJSON_GetObjectItem(root, ControlTuning::IGainStr);
    if (cJSON_IsNumber(IGainNode)) {
        _IGain = IGainNode->valuedouble;
    } else {
        ESP_LOGI(ControlTuning::Name, "Unable to read I gain from JSON");
        return PBRet::FAILURE;
    }

    // Read D gain
    cJSON* DGainNode = cJSON_GetObjectItem(root, ControlTuning::DGainStr);
    if (cJSON_IsNumber(DGainNode)) {
        _DGain = DGainNode->valuedouble;
    } else {
        ESP_LOGI(ControlTuning::Name, "Unable to read D gain from JSON");
        return PBRet::FAILURE;
    }

    // Read LPF cutoff
    cJSON* LPFCutoffNode = cJSON_GetObjectItem(root, ControlTuning::LPFCutoffStr);
    if (cJSON_IsNumber(LPFCutoffNode)) {
        _LPFCutoff = LPFCutoffNode->valuedouble;
    } else {
        ESP_LOGI(ControlTuning::Name, "Unable to read LPF cutoff from JSON");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet ControlCommand::serialize(std::string &JSONStr) const
{
    // Write the ControlCommand object to JSON

    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        ESP_LOGW(ControlCommand::Name, "Unable to create root JSON object");
        return PBRet::FAILURE;
    }

    if (cJSON_AddStringToObject(root, "MessageType", ControlCommand::Name) == nullptr)
    {
        ESP_LOGW(ControlCommand::Name, "Unable to add MessageType to control command JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    // Add fanstate
    cJSON* fanStateNode = cJSON_CreateNumber(static_cast<int>(fanState));
    if (fanStateNode == nullptr) {
        ESP_LOGW(ControlCommand::Name, "Error creating fanState JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, ControlCommand::FanStateStr, fanStateNode);

    // Add low power element
    cJSON* LPElementNode = cJSON_CreateNumber(LPElementDutyCycle);
    if (LPElementNode == nullptr) {
        ESP_LOGW(ControlCommand::Name, "Error creating low power element JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, ControlCommand::LPElementStr, LPElementNode);

    // Add high power element
    cJSON* HPElementNode = cJSON_CreateNumber(HPElementDutyCycle);
    if (HPElementNode == nullptr) {
        ESP_LOGW(ControlCommand::Name, "Error creating high power element JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, ControlCommand::HPElementStr, HPElementNode);

    char* stringPtr = cJSON_Print(root);
    JSONStr = std::string(stringPtr);
    cJSON_Delete(root);
    free(stringPtr);

    return PBRet::SUCCESS;
}

PBRet ControlCommand::deserialize(const cJSON *root)
{
    // Load the ControlCommand object from JSON

    if (root == nullptr) {
        ESP_LOGW(ControlCommand::Name, "root JSON object was null");
        return PBRet::FAILURE;
    }

    // Read fanState
    const cJSON* fanStateNode = cJSON_GetObjectItem(root, ControlCommand::FanStateStr);
    if (cJSON_IsNumber(fanStateNode)) {
        fanState = static_cast<ComponentState>(fanStateNode->valueint);
    } else {
        ESP_LOGI(ControlCommand::Name, "Unable to read fanState from JSON");
        return PBRet::FAILURE;
    }

    // Read LPElement
    const cJSON* LPElementNode = cJSON_GetObjectItem(root, ControlCommand::LPElementStr);
    if (cJSON_IsNumber(LPElementNode)) {
        LPElementDutyCycle = LPElementNode->valuedouble;
    } else {
        ESP_LOGI(ControlCommand::Name, "Unable to read LP Element duty cycle from JSON");
        return PBRet::FAILURE;
    }

    // Read HPElement
    const cJSON* HPElementNode = cJSON_GetObjectItem(root, ControlCommand::HPElementStr);
    if (cJSON_IsNumber(HPElementNode)) {
        HPElementDutyCycle = HPElementNode->valuedouble;
    } else {
        ESP_LOGI(ControlCommand::Name, "Unable to read HP Element duty cycle from JSON");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet ControlSettings::serialize(std::string &JSONStr) const
{
    // Write the ControlSettings object to JSON

    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        ESP_LOGW(ControlSettings::Name, "Unable to create root JSON object");
        return PBRet::FAILURE;
    }

    if (cJSON_AddStringToObject(root, "MessageType", ControlSettings::Name) == nullptr)
    {
        ESP_LOGW(ControlSettings::Name, "Unable to add MessageType to ControlSettings JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    // Add refluxPumpMode
    cJSON* refluxPumpModeNode = cJSON_CreateNumber(static_cast<int>(refluxPumpMode));
    if (refluxPumpModeNode == nullptr) {
        ESP_LOGW(ControlSettings::Name, "Error creating refluxPumpMode JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, ControlSettings::refluxPumpModeStr, refluxPumpModeNode);

    // Add productPumpMode
    cJSON* productPumpModeNode = cJSON_CreateNumber(static_cast<int>(productPumpMode));
    if (productPumpModeNode == nullptr) {
        ESP_LOGW(ControlSettings::Name, "Error creating productPumpMode JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, ControlSettings::productPumpModeStr, productPumpModeNode);

    // Add reflux pump manual speed
    cJSON* reluxPumpManualSpeedNode = cJSON_CreateNumber(static_cast<int>(manualPumpSpeeds.refluxPumpSpeed));
    if (reluxPumpManualSpeedNode == nullptr) {
        ESP_LOGW(ControlSettings::Name, "Error creating reflux pump manual speed JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, ControlSettings::refluxPumpSpeedManualStr, reluxPumpManualSpeedNode);

    // Add product pump manual speed
    cJSON* productPumpManualSpeedNode = cJSON_CreateNumber(static_cast<int>(manualPumpSpeeds.productPumpSpeed));
    if (productPumpManualSpeedNode == nullptr) {
        ESP_LOGW(ControlSettings::Name, "Error creating product pump manual speed JSON object");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }
    cJSON_AddItemToObject(root, ControlSettings::productPumpSpeedManualStr, productPumpManualSpeedNode);

    char* stringPtr = cJSON_Print(root);
    JSONStr = std::string(stringPtr);
    cJSON_Delete(root);
    free(stringPtr);

    return PBRet::SUCCESS;
}

PBRet ControlSettings::deserialize(const cJSON *root)
{
    // Load the ControlSettings object from JSON

    if (root == nullptr) {
        ESP_LOGW(ControlSettings::Name, "Root JSON object was null");
        return PBRet::FAILURE;
    }

    // Read reflux pump mode
    const cJSON* refluxPumpModeNode = cJSON_GetObjectItem(root, ControlSettings::refluxPumpModeStr);
    if (cJSON_IsNumber(refluxPumpModeNode)) {
        refluxPumpMode = static_cast<PumpMode>(refluxPumpModeNode->valueint);
    } else {
        ESP_LOGI(ControlSettings::Name, "Unable to read reflux pump mode from JSON");
        return PBRet::FAILURE;
    }

    // Read product pump mode
    const cJSON* productPumpModeNode = cJSON_GetObjectItem(root, ControlSettings::productPumpModeStr);
    if (cJSON_IsNumber(productPumpModeNode)) {
        productPumpMode = static_cast<PumpMode>(productPumpModeNode->valueint);
    } else {
        ESP_LOGI(ControlSettings::Name, "Unable to read product pump mode from JSON");
        return PBRet::FAILURE;
    }

    // Read reflux pump manual speed
    const cJSON* refluxPumpManualSpeedNode = cJSON_GetObjectItem(root, ControlSettings::refluxPumpSpeedManualStr);
    if (cJSON_IsNumber(refluxPumpManualSpeedNode)) {
        manualPumpSpeeds.refluxPumpSpeed = refluxPumpManualSpeedNode->valueint;
    } else {
        ESP_LOGI(ControlSettings::Name, "Unable to read reflux pump manual speed from JSON");
        return PBRet::FAILURE;
    }

    // Read product pump manual speed
    const cJSON* productPumpManualSpeedNode = cJSON_GetObjectItem(root, ControlSettings::productPumpSpeedManualStr);
    if (cJSON_IsNumber(productPumpManualSpeedNode)) {
        manualPumpSpeeds.productPumpSpeed = productPumpManualSpeedNode->valueint;
    } else {
        ESP_LOGI(ControlSettings::Name, "Unable to read product pump manual speed from JSON");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet ControllerState::serialize(std::string &JSONStr) const
{
    // Write the ControllerState object to JSON

    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        ESP_LOGW(ControllerState::Name, "Unable to create root JSON object");
        return PBRet::FAILURE;
    }

    if (cJSON_AddStringToObject(root, "MessageType", ControllerState::Name) == nullptr)
    {
        ESP_LOGW(ControllerState::Name, "Unable to add MessageType to ControllerState JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    // Add proportional term
    if (cJSON_AddNumberToObject(root, ControllerState::proportionalStr, propOutput) == nullptr) {
        ESP_LOGW(ControllerState::Name, "Unable to add proportional output to ControllerState JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    // Add integral term
    if (cJSON_AddNumberToObject(root, ControllerState::integralStr, integralOutput) == nullptr) {
        ESP_LOGW(ControllerState::Name, "Unable to add integral output to ControllerState JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    // Add derivative term
    if (cJSON_AddNumberToObject(root, ControllerState::derivativeStr, derivOutput) == nullptr) {
        ESP_LOGW(ControllerState::Name, "Unable to add derivative output to ControllerState JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    // Add total output term
    if (cJSON_AddNumberToObject(root, ControllerState::totalOutputStr, totalOutput) == nullptr) {
        ESP_LOGW(ControllerState::Name, "Unable to add totalOutput to ControllerState JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    // Add uptime
    if (cJSON_AddNumberToObject(root, ControllerState::UptimeStr, _timeStamp) == nullptr) {
        ESP_LOGW(ControllerState::Name, "Unable to add timestamp to ControllerState JSON string");
        cJSON_Delete(root);
        return PBRet::FAILURE;
    }

    char* stringPtr = cJSON_Print(root);
    JSONStr = std::string(stringPtr);
    cJSON_Delete(root);
    free(stringPtr);

    return PBRet::SUCCESS;
}

PBRet ControllerState::deserialize(const cJSON *root)
{
    // Load the ControllerState object from JSON

    if (root == nullptr) {
        ESP_LOGW(ControllerState::Name, "Root JSON object was null");
        return PBRet::FAILURE;
    }

    // Read proportional output
    const cJSON* propTermNode = cJSON_GetObjectItem(root, ControllerState::proportionalStr);
    if (cJSON_IsNumber(propTermNode)) {
        propOutput = propTermNode->valuedouble;
    } else {
        ESP_LOGI(ControllerState::Name, "Unable to read proportional output from JSON");
        return PBRet::FAILURE;
    }

    // Read integral output
    const cJSON* integralTermNode = cJSON_GetObjectItem(root, ControllerState::integralStr);
    if (cJSON_IsNumber(integralTermNode)) {
        integralOutput = integralTermNode->valuedouble;
    } else {
        ESP_LOGI(ControllerState::Name, "Unable to read integral output from JSON");
        return PBRet::FAILURE;
    }

    // Read derivative output
    const cJSON* derivTermNode = cJSON_GetObjectItem(root, ControllerState::derivativeStr);
    if (cJSON_IsNumber(derivTermNode)) {
        derivOutput = derivTermNode->valuedouble;
    } else {
        ESP_LOGI(ControllerState::Name, "Unable to read derivative output from JSON");
        return PBRet::FAILURE;
    }

    // Read total output
    const cJSON* totalOutputNode = cJSON_GetObjectItem(root, ControllerState::totalOutputStr);
    if (cJSON_IsNumber(totalOutputNode)) {
        totalOutput = totalOutputNode->valuedouble;
    } else {
        ESP_LOGI(ControllerState::Name, "Unable to read total output from JSON");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}