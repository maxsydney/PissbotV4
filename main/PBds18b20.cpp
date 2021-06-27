#include "PBds18b20.h"
#include "Utilities.h"

Ds18b20::Ds18b20(OneWireBus_ROMCode romCode, DS18B20_RESOLUTION res, 
                 const OneWireBus* bus, const Ds18b20Calibration& cal)
{
    if (Ds18b20::checkInputs(romCode, res, bus, cal) != PBRet::SUCCESS) {
        ESP_LOGE(Ds18b20::Name, "ds18b20 sensor was not initialized");
        _configured = false;
    }

    if (_initFromParams(romCode, res, bus, cal) == PBRet::SUCCESS) {
        ESP_LOGI(Ds18b20::Name, "Device successfully configured!");
        _configured = true;
    } else {
        ESP_LOGW(Ds18b20::Name, "ds18b20 sensor was not initialized");
        _configured = false;
    }
}

Ds18b20::Ds18b20(const cJSON* JSONConfig, DS18B20_RESOLUTION res, const OneWireBus* bus)
{
    // TODO: Move checkInputs into _initFromParams so can be checked when
    // initializing from JSON
    if (_initFromJSON(JSONConfig, res, bus) == PBRet::SUCCESS) {
        ESP_LOGI(Ds18b20::Name, "Device successfully configured!");
        _configured = true;
    } else {
        ESP_LOGW(Ds18b20::Name, "ds18b20 sensor was not initialized");
        _configured = false;
    }
}

PBRet Ds18b20::_initFromJSON(const cJSON* root, DS18B20_RESOLUTION res, const OneWireBus* bus)
{
    // Initialize a Ds18b20 object from a JSON file and bus pointer
    // Calibration and rom code are loaded from JSON file
    if (root == nullptr) {
        ESP_LOGW(Ds18b20::Name, "JSON pointer was null. Ds18b20 device not configured");
        return PBRet::FAILURE;
    }

    // Read ROM code
    const cJSON* romCode = cJSON_GetObjectItemCaseSensitive(root, Ds18b20::romCodeStr);
    if (romCode == nullptr) {
        ESP_LOGW(Ds18b20::Name, "Failed to find romCode property of JSON object");
        return PBRet::FAILURE;
    }

    const cJSON* byte = nullptr;
    OneWireBus_ROMCode romCodeIn {};
    size_t i = 0;
    cJSON_ArrayForEach(byte, romCode)
    {
        if (byte == nullptr) {
            ESP_LOGW(Ds18b20::Name, "Failed to read byte from romCode");
            return PBRet::FAILURE;
        }

        if (cJSON_IsNumber(byte) == false) {
            ESP_LOGW(Ds18b20::Name, "Byte in romCode array was not of cJSON type number");
            return PBRet::FAILURE;
        }

        romCodeIn.bytes[i++] = byte->valueint;
    }

    // Read calibration
    const cJSON* calNode = cJSON_GetObjectItemCaseSensitive(root, Ds18b20::calibrationStr);
    if (romCode == nullptr) {
        ESP_LOGW(Ds18b20::Name, "Failed to find calibration property of JSON object");
        return PBRet::FAILURE;
    }

    // Read linear scaling coefficient
    double calCoeffA = 0.0;
    const cJSON* calCoeffANode = cJSON_GetObjectItem(calNode, Ds18b20::calCoeffAStr);
    if (cJSON_IsNumber(calCoeffANode)) {
        calCoeffA = calCoeffANode->valuedouble;
    } else {
        ESP_LOGW(Ds18b20::Name, "Unable to read linear scaling calibration coefficient");
        return PBRet::FAILURE;
    }

    // Read linear scaling coefficient
    double calCoeffB = 0.0;
    const cJSON* calCoeffBNode = cJSON_GetObjectItem(calNode, Ds18b20::calCoeffBStr);
    if (cJSON_IsNumber(calCoeffBNode)) {
        calCoeffB = calCoeffBNode->valuedouble;
    } else {
        ESP_LOGW(Ds18b20::Name, "Unable to read constant offset calibration coefficient");
        return PBRet::FAILURE;
    }

    Ds18b20Calibration cal(calCoeffA, calCoeffB);
    return _initFromParams(romCodeIn, res, bus, cal);
}

PBRet Ds18b20::checkInputs(OneWireBus_ROMCode romCode, DS18B20_RESOLUTION res,
                           const OneWireBus* bus, const Ds18b20Calibration& cal)
{
    if (bus == nullptr) {
        ESP_LOGW(Ds18b20::Name, "Onewire bus pointer was null. Unable to initialize sensor");
        return PBRet::FAILURE;
    }

    // TODO: Check address

    // TODO: Check resolution

    // Check calibration
    if (Utilities::check(cal.A) == false) {
        ESP_LOGW(Ds18b20::Name, "Calibration linear scaling coefficient was invalid");
        return PBRet::FAILURE;
    }

    if (Utilities::check(cal.B) == false) {
        ESP_LOGW(Ds18b20::Name, "Calibration constant offset coefficient was invalid");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet Ds18b20::readTemp(double& temp) const
{
    if (_configured == false) {
        ESP_LOGW(Ds18b20::Name, "Sensor is not configured. Temperature read failed");
        return PBRet::FAILURE;
    }

    float rawTemp = 0.0;
    if (ds18b20_read_temp(&_info, &rawTemp) != DS18B20_OK) {
        ESP_LOGW(Ds18b20::Name, "Temperature read failed");
        return PBRet::FAILURE;
    }

    // Calibrate measurement
    temp = _cal.A * rawTemp + _cal.B;

    return PBRet::SUCCESS;
}

PBRet Ds18b20::serialize(cJSON* root) const
{
    // Write sensor configuration to JSON. Sensor configuration is
    // DS18B20_Info (and maybe calibration data later)

    if (root == nullptr) {
        ESP_LOGW(Ds18b20::Name, "JSON pointer was null");
        return PBRet::FAILURE;
    }

    // Write the ROM code
    cJSON* romCode = cJSON_CreateArray();
    if (romCode == nullptr) {
        ESP_LOGW(Ds18b20::Name, "Failed to create JSON array for sensor ROM code");
        return PBRet::FAILURE;
    }

    for (int j=0; j < 8; j++) {
        cJSON* byte = cJSON_CreateNumber((int) _info.rom_code.bytes[j]);
        if (byte == nullptr) {
            ESP_LOGW(Ds18b20::Name, "Error creating byte in JSON ROM code byte array");
            cJSON_Delete(romCode);
            return PBRet::FAILURE;
        }
        cJSON_AddItemToArray(romCode, byte);
    }
    cJSON_AddItemToObject(root, Ds18b20::romCodeStr, romCode);

    // Write sensor calibration
    cJSON* calibNode = cJSON_CreateObject();
    if (calibNode == nullptr) {
        ESP_LOGW(Ds18b20::Name, "Failed to create JSON object for calibration coefficients");
        return PBRet::FAILURE;
    }

    if (cJSON_AddNumberToObject(calibNode, Ds18b20::calCoeffAStr, _cal.A) == nullptr) {
        ESP_LOGW(Ds18b20::Name, "Unable to write linear scaling calibration coefficient to JSON");
        cJSON_Delete(root);
        cJSON_Delete(calibNode);
        return PBRet::FAILURE;
    }

    if (cJSON_AddNumberToObject(calibNode, Ds18b20::calCoeffBStr, _cal.B) == nullptr) {
        ESP_LOGW(Ds18b20::Name, "Unable to write constant offset calibration coefficient to JSON");
        cJSON_Delete(root);
        cJSON_Delete(calibNode);
        return PBRet::FAILURE;
    }

    cJSON_AddItemToObject(root, Ds18b20::calibrationStr, calibNode);

    return PBRet::SUCCESS;
}

PBRet Ds18b20::_initFromParams(OneWireBus_ROMCode romCode, DS18B20_RESOLUTION res, 
                               const OneWireBus* bus, const Ds18b20Calibration& cal)
{
    // Initialize info
    ds18b20_init(&_info, bus, romCode);
    ds18b20_use_crc(&_info, true);
    ds18b20_set_resolution(&_info, res);

    // Check if sensor responds on bus
    if (ds18b20_read_resolution(&_info) == DS18B20_RESOLUTION_INVALID) {
        ESP_LOGW(Ds18b20::Name, "Device was initialised but didn't respond on bus");
        return PBRet::FAILURE;
    }

    // Store calibration
    _cal = cal;

    return PBRet::SUCCESS;
}

bool Ds18b20::operator==(const Ds18b20& other) const
{
    // Sensors are equal if their rom codes match
    bool equal = true;
    for (size_t i = 0; i < 8; i++) {
        if (_info.rom_code.bytes[i] != other._info.rom_code.bytes[i]) {
            equal = false;
            break;
        }
    }

    return equal;
}

const OneWireBus_ROMCode* Ds18b20::getROMCode(void) const
{
    return &_info.rom_code;
}