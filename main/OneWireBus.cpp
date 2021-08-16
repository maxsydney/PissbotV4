#include "OneWireBus.h"
#include "SensorManager.h"
#include "Generated/SensorManagerMessaging.h"

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

PBRet PBOneWire::_scanForDevices(DeviceVector& devices) const
{
    if (_owb == nullptr) {
        ESP_LOGW(PBOneWire::Name, "Onewire bus pointer was null");
        return PBRet::FAILURE;
    }

    if (xSemaphoreTake(_OWBMutex, 250 / portTICK_PERIOD_MS) == pdTRUE) {
        OneWireBus_SearchState search_state {};
        bool found = false;
        devices.clear();

        owb_search_first(_owb, &search_state, &found);
        while (found) {
            devices.emplace_back(Ds18b20Config(search_state.rom_code, 1.0, 0.0, _cfg.tempSensorResolution, _owb));
            owb_search_next(_owb, &search_state, &found);
        }
        xSemaphoreGive(_OWBMutex);
    } else {
        ESP_LOGW(PBOneWire::Name, "Unable to access PBOneWire shared resource");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet PBOneWire::_broadcastDeviceAddresses(const DeviceVector& devices) const
{
    // Broadcast the available device addresses to all listening tasks

    PBDeviceData deviceData {};
    for (const Ds18b20& sensor : devices) {
        deviceData.add_sensors(sensor.toSerialConfig());
    }

    PBMessageWrapper wrapped = MessageServer::wrap(deviceData, PBMessageType::DeviceData);
    return MessageServer::broadcastMessage(wrapped);
}

PBRet PBOneWire::broadcastAvailableDevices(void) const
{
    DeviceVector devices {};
    if (_scanForDevices(devices) != PBRet::SUCCESS) {
        ESP_LOGW(PBOneWire::Name, "Scan of OneWire bus failed");
        return PBRet::FAILURE;
    }

    return _broadcastDeviceAddresses(devices);
}

PBRet PBOneWire::_oneWireConvert(void) const
{
    // Command all temperature sensors on the bus to convert temperatures.
    // NOTE: This method is not protected by a semaphore. If you are calling
    //       this method, it is your responsibility to make sure that the bus
    //       is not locked out

    if (_assignedSensors.size() == 0) {
        ESP_LOGW(PBOneWire::Name, "No assigned devices. Cannot convert temperatures");
        return PBRet::FAILURE;
    }

    ds18b20_convert_all(_owb);

    // Wait until the first sensor in the map has finished converting measurement
    ds18b20_wait_for_conversion(&_assignedSensors.begin()->second->getInfo());

    return PBRet::SUCCESS;
}

PBRet PBOneWire::readTempSensors(TemperatureData& Tdata) const
{
    // Read all available temperature sensors
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

        // Print a warning for this one, as it required for control
        if (_readTemperatureSensor(DS18B20Role::HEAD_TEMP, Tdata.mutable_headTemp().get()) != PBRet::SUCCESS) {
            ESP_LOGW(PBOneWire::Name, "Failed to read head temperature sensor");
        }

        // Read all other sensors
        _readTemperatureSensor(DS18B20Role::REFLUX_TEMP, Tdata.mutable_refluxCondensorTemp().get());
        _readTemperatureSensor(DS18B20Role::PRODUCT_TEMP, Tdata.mutable_prodCondensorTemp().get());
        _readTemperatureSensor(DS18B20Role::RADIATOR_TEMP, Tdata.mutable_radiatorTemp().get());
        _readTemperatureSensor(DS18B20Role::BOILER_TEMP, Tdata.mutable_boilerTemp().get());

        // Set timestamp
        Tdata.set_timeStamp(esp_timer_get_time());

        xSemaphoreGive(_OWBMutex);
    } else {
        ESP_LOGW(PBOneWire::Name, "Unable to access PBOneWire shared resource");
        return PBRet::FAILURE;
    }

    // // Reject temperature measurements if head temperature is invalid
    // // TODO: This shouldn't live here. Move to controller
    // if ((headTemp < PBOneWire::MinValidTemp) || (headTemp > PBOneWire::MaxValidTemp)) {
    //     ESP_LOGW(PBOneWire::Name, "Head temperature (%.2f) was invalid", headTemp);
    //     return PBRet::FAILURE;
    // }
    
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

    return PBRet::SUCCESS;
}

 PBRet PBOneWire::serialize(Writable& buffer) const
{
    // Write the assigned sensors to buffer

    PBAssignedSensorRegistry registry {};

    auto it = _assignedSensors.find(DS18B20Role::HEAD_TEMP);
    if (it != _assignedSensors.end())
    {
        registry.set_headTempSensor((*it->second).toSerialConfig());
    }

    it = _assignedSensors.find(DS18B20Role::REFLUX_TEMP);
    if (it != _assignedSensors.end())
    {
        registry.set_refluxTempSensor((*it->second).toSerialConfig());
    }

    it = _assignedSensors.find(DS18B20Role::PRODUCT_TEMP);
    if (it != _assignedSensors.end())
    {
        registry.set_productTempSensor((*it->second).toSerialConfig());
    }

    it = _assignedSensors.find(DS18B20Role::RADIATOR_TEMP);
    if (it != _assignedSensors.end())
    {
        registry.set_radiatorTempSensor((*it->second).toSerialConfig());
    }

    it = _assignedSensors.find(DS18B20Role::BOILER_TEMP);
    if (it != _assignedSensors.end())
    {
        registry.set_boilerTempSensor((*it->second).toSerialConfig());
    }

    // Serialize data structure into buffer
    EmbeddedProto::Error e = registry.serialize(buffer);

    if (e != EmbeddedProto::Error::NO_ERRORS) {
        ESP_LOGW(PBOneWire::Name, "Failed to serialize message");
        MessageServer::printErr(e);
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

// TODO: Pass in available sensors as an argument
bool PBOneWire::isAvailableSensor(const Ds18b20& sensor) const
{
    // Returns true if a ds18b20 sensor exists in the list of available 
    // sensors

    // for (const std::shared_ptr<Ds18b20> available : _availableSensors) {
    //     if (sensor == *available) {
    //         return true;
    //     }
    // }

    return false;
}

PBRet PBOneWire::setTempSensor(DS18B20Role type, const std::shared_ptr<Ds18b20>& sensor)
{
    // First, we must "unassign" the sensor if it is already assigned
    auto it = std::find_if(_assignedSensors.begin(), _assignedSensors.end(), 
                           [&sensor] (const auto& p) { return *p.second == *sensor; });

    if (it != _assignedSensors.end()) { 
        _assignedSensors.erase(it); 
    }

    // Assign sensor to new type
    _assignedSensors[type] = sensor;

    return PBRet::SUCCESS;
}

PBRet PBOneWire::_readTemperatureSensor(DS18B20Role sensor, double& T) const
{
    // Attempt to read the temperature of a sensor with a prescribed role.
    SensorMap::const_iterator it = _assignedSensors.find(sensor);

    if (it == _assignedSensors.end()) {
        // Role has no assigned sensor
        return PBRet::FAILURE;
    }

    // Compiler doesn't like getting the address for a double inside this fn
    // TODO: Fix this weirdness
    float temp = 0.0;
    if (it->second->readTemp(temp) == PBRet::SUCCESS) {
        // Successfully read temperature sensor
        T = temp;
        return PBRet::SUCCESS;
    }

    // Sensor read failed
    return PBRet::FAILURE;
}