#include "OneWireBus.h"
#include "SensorManager.h"

PBOneWire::PBOneWire(const PBOneWireConfig& cfg)
{
    // Check input parameters
    if (checkInputs(cfg) != PBRet::SUCCESS) {
        ESP_LOGE(PBOneWire::Name, "Failed to configure OneWire bus");
        _configured = false;
    }

    // Initialize PBOneWire
    if (_initFromParams(cfg) == PBRet::SUCCESS) {
        ESP_LOGI(PBOneWire::Name, "PBOneWire configured!");
        _configured = true;
    } else {
        ESP_LOGW(PBOneWire::Name, "Unable to configure PBOneWire");
        _configured = false;
    }
}

PBRet PBOneWire::checkInputs(const PBOneWireConfig& cfg)
{
    // Check pin is valid GPIO
    if ((cfg.oneWirePin <= GPIO_NUM_NC) || (cfg.oneWirePin > GPIO_NUM_MAX)) {
        ESP_LOGE(PBOneWire::Name, "OneWire pin %d is invalid. Bus was not configured", cfg.oneWirePin);
        return PBRet::FAILURE;
    }

    if ((cfg.refluxFlowPin <= GPIO_NUM_NC) || (cfg.refluxFlowPin > GPIO_NUM_MAX)) {
        ESP_LOGE(PBOneWire::Name, "Reflux flowmeter GPIO %d is invalid. SensorManager was not configured", cfg.refluxFlowPin);
        return PBRet::FAILURE;
    }

    if ((cfg.productFlowPin <= GPIO_NUM_NC) || (cfg.productFlowPin > GPIO_NUM_MAX)) {
        ESP_LOGE(PBOneWire::Name, "Product flowmeter GPIO %d is invalid. SensorManager was not configured", cfg.productFlowPin);
        return PBRet::FAILURE;
    }

    if (cfg.tempSensorResolution == DS18B20_RESOLUTION_INVALID) {
        ESP_LOGE(PBOneWire::Name, "Temperature sensor resolution was invalid");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet PBOneWire::loadFromJSON(PBOneWireConfig& cfg, const cJSON* cfgRoot)
{
    // Load PBOneWire struct from JSON

    if (cfgRoot == nullptr) {
        ESP_LOGW(PBOneWire::Name, "cfgRoot was null");
        return PBRet::FAILURE;
    }

    // Get Onewire GPIO
    cJSON* GPIOOneWireNode = cJSON_GetObjectItem(cfgRoot, "GPIO_onewire");
    if (cJSON_IsNumber(GPIOOneWireNode)) {
        cfg.oneWirePin = static_cast<gpio_num_t>(GPIOOneWireNode->valueint);      // TODO: Update name to GPIOOnewire
    } else {
        ESP_LOGI(PBOneWire::Name, "Unable to read Onewire GPIO from JSON");
        return PBRet::FAILURE;
    }

    // Get DS18B20 resolution
    cJSON* resolutionNode = cJSON_GetObjectItem(cfgRoot, "DS18B20Resolution");
    if (cJSON_IsNumber(resolutionNode)) {
        cfg.tempSensorResolution = static_cast<DS18B20_RESOLUTION>(resolutionNode->valueint);      // TODO: Typecast here?
    } else {
        ESP_LOGI(PBOneWire::Name, "Unable to read DS18B20 resolution from JSON");
        return PBRet::FAILURE;
    }

    // Get reflux flowmeter GPIO
    cJSON* refluxFlowNode = cJSON_GetObjectItem(cfgRoot, "GPIO_refluxFlow");
    if (cJSON_IsNumber(refluxFlowNode)) {
        cfg.refluxFlowPin = static_cast<gpio_num_t>(refluxFlowNode->valueint);
    } else {
        ESP_LOGI(PBOneWire::Name, "Unable to read reflux flowmeter pin from JSON");
        return PBRet::FAILURE;
    }

    // Get product flowmeter GPIO
    cJSON* prodFlowNode = cJSON_GetObjectItem(cfgRoot, "GPIO_prodFlow");
    if (cJSON_IsNumber(prodFlowNode)) {
        cfg.productFlowPin = static_cast<gpio_num_t>(prodFlowNode->valueint);
    } else {
        ESP_LOGI(PBOneWire::Name, "Unable to read product flowmeter pin from JSON");
        return PBRet::FAILURE;
    }

    // Success by here
    return PBRet::SUCCESS;
}

PBRet PBOneWire::_initOWB()
{
    _rmtDriver = new owb_rmt_driver_info;
    if (_rmtDriver == nullptr) {
        ESP_LOGE(PBOneWire::Name, "Unable to allocate memory for RMT driver");
        return PBRet::FAILURE;
    }

    // Fields are statically allocated within owb_rmt_initialize
    _owb = owb_rmt_initialize(_rmtDriver, _cfg.oneWirePin, RMT_CHANNEL_1, RMT_CHANNEL_0);
    if (_owb == nullptr) {
        ESP_LOGE(PBOneWire::Name, "OnewWireBus initialization returned nullptr");
        return PBRet::FAILURE;
    }

    // Enable CRC check for ROM code. The return type from this method indicates 
    // if bus was configured correctly
    if (owb_use_crc(_owb, true) != OWB_STATUS_OK) {
        ESP_LOGE(PBOneWire::Name, "OnewWireBus initialization failed");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet PBOneWire::_scanForDevices(void)
{
    if (_owb == nullptr) {
        ESP_LOGW(PBOneWire::Name, "Onewire bus pointer was null");
        return PBRet::FAILURE;
    }

    if (xSemaphoreTake(_OWBMutex, 250 / portTICK_PERIOD_MS) == pdTRUE) {
        OneWireBus_SearchState search_state {};
        bool found = false;
        _connectedDevices = 0;
        _availableSensors.clear();

        owb_search_first(_owb, &search_state, &found);
        while (found) {
            _availableSensors.emplace_back(Ds18b20(search_state.rom_code, _cfg.tempSensorResolution, _owb));
            _connectedDevices++;
            owb_search_next(_owb, &search_state, &found);
        }
        xSemaphoreGive(_OWBMutex);
    } else {
        ESP_LOGW(PBOneWire::Name, "(_scanForDevices) Unable to access PBOneWire shared resource");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet PBOneWire::_broadcastDeviceAddresses(void) const
{
    // Broadcast the available device addresses to all listening tasks
    std::shared_ptr<DeviceData> msg = std::make_shared<DeviceData> (_availableSensors);

    return MessageServer::broadcastMessage(msg);
}

PBRet PBOneWire::broadcastAvailableDevices(void)
{
    if (_scanForDevices() != PBRet::SUCCESS) {
        ESP_LOGW(PBOneWire::Name, "Scan of OneWire bus failed");
        return PBRet::FAILURE;
    }

    return _broadcastDeviceAddresses();
}

PBRet PBOneWire::initialiseTempSensors(void)
{
    // TODO:
    // Attempt to connect to each sensor and check that they are responding
    // 

    // // Connect to sensors and create PBds18b20 object
    // if (_connectedDevices == 0) {
    //     ESP_LOGW(PBOneWire::Name, "No available devices");
    //     return PBRet::FAILURE;
    // }

    // // NOTE: Just assigning the first for now. Assign properly from saved sensors JSON
    // // Assign sensors
    // if (_headTempSensor.isConfigured() == false) {
    //     _headTempSensor = _availableSensors.at(0);
    // }

    // // Connect other devices here
    
    return PBRet::SUCCESS;
}

PBRet PBOneWire::_oneWireConvert(void) const
{
    // Command all temperature sensors on the bus to convert temperatures.
    // NOTE: This method is not protected by a semaphore. If you are calling
    //       this method, it is your responsibility to make sure that the bus
    //       is not locked out

    if (_connectedDevices == 0) {
        ESP_LOGW(PBOneWire::Name, "No available devices. Cannot convert temperatures");
        return PBRet::FAILURE;
    }

    ds18b20_convert_all(_owb);
    ds18b20_wait_for_conversion(&_availableSensors.front().getInfo());

    return PBRet::SUCCESS;
}

PBRet PBOneWire::readTempSensors(TemperatureData& Tdata) const
{
    // Read all available temperature sensors
    
    // TODO: Replace these with direct memory access
    float headTemp = 0.0;
    float refluxTemp = 0.0;
    float productTemp = 0.0;
    float radiatorTemp = 0.0;
    float boilerTemp = 0.0;

    if (_configured == false) {
        ESP_LOGW(PBOneWire::Name, "Cannot read temperatures before PBOneWireBuse is configured");
        return PBRet::FAILURE;
    }

    if (xSemaphoreTake(_OWBMutex, 10 / portTICK_PERIOD_MS) == pdTRUE) {
        if (_oneWireConvert() != PBRet::SUCCESS) {
            ESP_LOGW(PBOneWire::Name, "Temperature sensor conversion failed");
            xSemaphoreGive(_OWBMutex);
            return PBRet::FAILURE;
        }

        // Read all available temperature sensors
        if (_headTempSensor.isConfigured()) {
            if (_headTempSensor.readTemp(headTemp) != PBRet::SUCCESS) {
                ESP_LOGW(PBOneWire::Name, "Head temperature sensor is configured but was not able to be read");
            }
        }

        if (_refluxTempSensor.isConfigured()) {
            if (_refluxTempSensor.readTemp(refluxTemp) != PBRet::SUCCESS) {
                ESP_LOGW(PBOneWire::Name, "Reflux condensor temperature sensor is configured but was not able to be read");
            }
        }

        if (_productTempSensor.isConfigured()) {
            if (_productTempSensor.readTemp(productTemp) != PBRet::SUCCESS) {
                ESP_LOGW(PBOneWire::Name, "Product condensor temperature sensor is configured but was not able to be read");
            }
        }

        if (_radiatorTempSensor.isConfigured()) {
            if (_radiatorTempSensor.readTemp(radiatorTemp) != PBRet::SUCCESS) {
                ESP_LOGW(PBOneWire::Name, "Radiator temperature sensor is configured but was not able to be read");
            }
        }

        if (_boilerTempSensor.isConfigured()) {
            if (_boilerTempSensor.readTemp(boilerTemp) != PBRet::SUCCESS) {
                ESP_LOGW(PBOneWire::Name, "Boiler temperature sensor is configured but was not able to be read");
            }
        }

        xSemaphoreGive(_OWBMutex);
    } else {
        ESP_LOGW(PBOneWire::Name, "Unable to access PBOneWire shared resource");
        return PBRet::FAILURE;
    }

    // Read other sensors here

    // Fill data structure
    Tdata = TemperatureData(headTemp, refluxTemp, productTemp, radiatorTemp, boilerTemp);

    return PBRet::SUCCESS;
}

PBRet PBOneWire::_initFromParams(const PBOneWireConfig& cfg)
{
    _cfg = cfg;

    // Initialize the bus
    if (_initOWB() != PBRet::SUCCESS) {
        ESP_LOGE(PBOneWire::Name, "Failed to configure OneWire bus");
        return PBRet::FAILURE;
    }

    // Initialise bus semaphore
    _OWBMutex = xSemaphoreCreateMutex();
    if (_OWBMutex == NULL) {
        ESP_LOGW(PBOneWire::Name, "Unable to create mutex");
        return PBRet::FAILURE;
    }

    // The following cases can fail and be recovered from. Don't return failure
    // Find available devices on bus
    if (_scanForDevices() != PBRet::SUCCESS) {
        ESP_LOGW(PBOneWire::Name, "Failed to scan OneWire bus for deveices");
    }
    if (_connectedDevices <= 0) {
        ESP_LOGW(PBOneWire::Name, "No devices were found on OneWire bus");
    }

    // // This is a temporary method to load a temperature sensor into the headTemp channel
    // // for testing
    if (initialiseTempSensors() != PBRet::SUCCESS) {
        ESP_LOGW(PBOneWire::Name, "Sensors were not initialized");
    }

    return PBRet::SUCCESS;
}

 PBRet PBOneWire::serialize(std::string& JSONstr) const
{
    // Helper method for writing out the sensor configuration to JSON
    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        ESP_LOGW(PBOneWire::Name, "Unable to create root JSON object");
        return PBRet::FAILURE;
    }

    // Create an object for temperature sensors
    cJSON* tempSensors = cJSON_CreateObject();
    if (tempSensors == NULL) {
        ESP_LOGW(PBOneWire::Name, "Unable to create tempSensors JSON object");
        return PBRet::FAILURE;
    }

    // Write individual sensors to tempSensors
    if (_headTempSensor.isConfigured()) {
        // Write out its data
        cJSON* headTemp = cJSON_CreateObject();
        if (headTemp == NULL) {
            ESP_LOGW(PBOneWire::Name, "Unable to create headTemp JSON object");
            return PBRet::FAILURE;
        }

        // TODO: Add headTemp to tempSensors and the modify inplace?
        if (_headTempSensor.serialize(headTemp) != PBRet::SUCCESS) {
            ESP_LOGW(PBOneWire::Name, "Couldn't serialize head temp sensor");
            cJSON_Delete(headTemp);
            return PBRet::FAILURE;
        }

        cJSON_AddItemToObject(tempSensors, SensorManager::HeadTempSensorKey, headTemp);
    }

    if (_refluxTempSensor.isConfigured()) {
        // Write out its data
        cJSON* refluxTempSensor = cJSON_CreateObject();
        if (refluxTempSensor == NULL) {
            ESP_LOGW(PBOneWire::Name, "Unable to create reflux temp sensor JSON object");
            return PBRet::FAILURE;
        }

        if (_refluxTempSensor.serialize(refluxTempSensor) != PBRet::SUCCESS) {
            ESP_LOGW(PBOneWire::Name, "Couldn't serialize reflux condensor temp sensor");
            cJSON_Delete(refluxTempSensor);
            return PBRet::FAILURE;
        }

        cJSON_AddItemToObject(tempSensors, SensorManager::RefluxTempSensorKey, refluxTempSensor);
    }

    if (_productTempSensor.isConfigured()) {
        // Write out its data
        cJSON* productTempSensor = cJSON_CreateObject();
        if (productTempSensor == NULL) {
            ESP_LOGW(PBOneWire::Name, "Unable to create product temp sensor JSON object");
            return PBRet::FAILURE;
        }

        if (_productTempSensor.serialize(productTempSensor) != PBRet::SUCCESS) {
            ESP_LOGW(PBOneWire::Name, "Couldn't serialize product condensor temp sensor");
            cJSON_Delete(productTempSensor);
            return PBRet::FAILURE;
        }

        cJSON_AddItemToObject(tempSensors, SensorManager::ProductTempSensorKey, productTempSensor);
    }

    if (_radiatorTempSensor.isConfigured()) {
        // Write out its data
        cJSON* radiatorTempSensor = cJSON_CreateObject();
        if (radiatorTempSensor == NULL) {
            ESP_LOGW(PBOneWire::Name, "Unable to create radiator temp sensor JSON object");
            return PBRet::FAILURE;
        }

        if (_radiatorTempSensor.serialize(radiatorTempSensor) != PBRet::SUCCESS) {
            ESP_LOGW(PBOneWire::Name, "Couldn't serialize radiator temp sensor");
            cJSON_Delete(radiatorTempSensor);
            return PBRet::FAILURE;
        }

        cJSON_AddItemToObject(tempSensors, SensorManager::RadiatorTempSensorKey, radiatorTempSensor);
    }

    if (_radiatorTempSensor.isConfigured()) {
        // Write out its data
        cJSON* radiatorTempSensor = cJSON_CreateObject();
        if (radiatorTempSensor == NULL) {
            ESP_LOGW(PBOneWire::Name, "Unable to create radiator temp sensor JSON object");
            return PBRet::FAILURE;
        }

        if (_radiatorTempSensor.serialize(radiatorTempSensor) != PBRet::SUCCESS) {
            ESP_LOGW(PBOneWire::Name, "Couldn't serialize radiator temp sensor");
            cJSON_Delete(radiatorTempSensor);
            return PBRet::FAILURE;
        }

        cJSON_AddItemToObject(tempSensors, SensorManager::RadiatorTempSensorKey, radiatorTempSensor);
    }

    if (_boilerTempSensor.isConfigured()) {
        // Write out its data
        cJSON* boilerTempSensor = cJSON_CreateObject();
        if (boilerTempSensor == NULL) {
            ESP_LOGW(PBOneWire::Name, "Unable to create boiler temp sensor JSON object");
            return PBRet::FAILURE;
        }

        if (_boilerTempSensor.serialize(boilerTempSensor) != PBRet::SUCCESS) {
            ESP_LOGW(PBOneWire::Name, "Couldn't serialize boiler temp sensor");
            cJSON_Delete(boilerTempSensor);
            return PBRet::FAILURE;
        }

        cJSON_AddItemToObject(tempSensors, SensorManager::BoilerTempSensorKey, boilerTempSensor);
    }

    cJSON_AddItemToObject(root, "TempSensors", tempSensors);

    // Copy JSON to string. cJSON requires printing to a char* pointer. Copy into
    // std::string and free memory to avoid memory leak
    char* stringPtr = cJSON_Print(root);
    JSONstr = std::string(stringPtr);
    cJSON_Delete(root);
    free(stringPtr);

    return PBRet::SUCCESS;
}

bool PBOneWire::isAvailableSensor(const Ds18b20& sensor) const
{
    // Returns true if a ds18b20 sensor exists in the list of available 
    // sensors

    for (const Ds18b20& available : _availableSensors) {
        if (sensor == available) {
            return true;
        }
    }

    return false;
}

PBRet PBOneWire::setTempSensor(SensorType type, const Ds18b20& sensor)
{
    // TODO: This function is very manual and could be tidied up a lot
    
    // First, we must "unassign" the sensor if it is already assigned
    if (_headTempSensor == sensor) {
        _headTempSensor = Ds18b20();
    } else if (_refluxTempSensor == sensor) {
        _refluxTempSensor = Ds18b20();
    } else if (_productTempSensor == sensor) {
        _productTempSensor = Ds18b20();
    } else if (_radiatorTempSensor == sensor) {
        _radiatorTempSensor = Ds18b20();
    } else if (_boilerTempSensor == sensor) {
        _boilerTempSensor = Ds18b20();
    }

    // Assign the sensor to the selected task
    switch (type)
    {
        case SensorType::Head:
        {
            _headTempSensor = sensor;
            break;
        }
        case SensorType::Reflux:
        {
            _refluxTempSensor = sensor;
            break;
        }
        case SensorType::Product:
        {
            _productTempSensor = sensor;
            break;
        }
        case SensorType::Boiler:
        {
            _boilerTempSensor = sensor;
            break;
        }
        case SensorType::Radiator:
        {
            _radiatorTempSensor = sensor;
            break;
        }
        case SensorType::Unknown:
        {
            ESP_LOGE(PBOneWire::Name, "Cannot set sensor with type unknown");
            return PBRet::FAILURE;
        }
        default:
        {
            ESP_LOGE(PBOneWire::Name, "Sensor type is not supported");
            return PBRet::FAILURE;
        }
    }

    return PBRet::SUCCESS;
}

// TODO: Replace this with a more robust mapping
SensorType PBOneWire::mapSensorIDToType(int sensorID)
{
    switch (sensorID)
    {
        case (0):
        {
            return SensorType::Head;
        }
        case (1):
        {
            return SensorType::Reflux;
        }
        case (2):
        {
            return SensorType::Product;
        }
        case (3):
        {
            return SensorType::Radiator;
        }
        case (4):
        {
            return SensorType::Boiler;
        }
        default:
        {
            return SensorType::Unknown;
        }
    }
}