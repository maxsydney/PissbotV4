#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_timer.h>
#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/timer.h"
#include "ds18b20.h" 
#include "sensors.h"
#include "controlLoop.h"
#include "main.h"
#include "pinDefs.h"
#include "owb.h"
#include "owb_rmt.h"
#include "ds18b20.h"

#define SAMPLE_PERIOD        (400)   // milliseconds

static const char* tag = "Sensors";
static volatile double timeVal;

static OneWireBus_ROMCode device_rom_codes[MAX_DEVICES] = {0};
OneWireBus_ROMCode saved_rom_codes[MAX_DEVICES] = {0};
static int savedSensorMap[MAX_DEVICES] = {0};
static DS18B20_Info * devices[MAX_DEVICES] = {0};
static OneWireBus * owb;
static owb_rmt_driver_info rmt_driver_info;
int num_devices = 0;

xQueueHandle tempQueue;
xQueueHandle flowRateQueue;

esp_err_t loadSavedSensors(OneWireBus_ROMCode saved_devices[MAX_DEVICES])
{
    nvs_handle nvs;

    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGI(tag, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        ESP_LOGI(tag, "NVS handle opened successfully\n");
    }

    // Read the size of memory space required for blob
    size_t required_size = 0;  // value will default to 0, if not set yet in NVS
    err = nvs_get_blob(nvs, "devices", NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(tag, "Error (%s) reading size of devices!", esp_err_to_name(err));
    } else {
        ESP_LOGI(tag, "Size of devices is %zu", required_size);
    }

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        esp_err_t internal = nvs_set_blob(nvs, "devices", saved_devices, sizeof(OneWireBus_ROMCode[MAX_DEVICES]));

        if (internal != ESP_OK) {
            ESP_LOGI(tag, "Error (%s) writing empty data structure", esp_err_to_name(internal));
        } else {
            ESP_LOGI(tag, "Successfully wrote empty data structure to nvs");
            ESP_ERROR_CHECK(nvs_commit(nvs));
        }
    }

    if (err == ESP_OK) {
        ESP_ERROR_CHECK(nvs_get_blob(nvs, "devices", saved_devices, &required_size));
    }

    nvs_close(nvs);
    return ESP_OK;
}

esp_err_t writeDeviceRomCodes(OneWireBus_ROMCode code[MAX_DEVICES])
{
    nvs_handle nvs;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGI(tag, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        ESP_LOGI(tag, "NVS handle opened successfully\n");
    }
 
    if (err == ESP_OK) {
        esp_err_t internal = nvs_set_blob(nvs, "devices", code, sizeof(OneWireBus_ROMCode[MAX_DEVICES]));

        if (internal != ESP_OK) {
            ESP_LOGI(tag, "Error (%s) writing empty data structure", esp_err_to_name(internal));
        } else {
            ESP_LOGI(tag, "Successfully wrote rom code to NVS");
            ESP_ERROR_CHECK(nvs_commit(nvs));
        }
    }
    
    nvs_close(nvs);

    return err;
}

esp_err_t sensor_init(uint8_t ds_pin, DS18B20_RESOLUTION res)
{
    // Create a 1-Wire bus, using the RMT timeslot driver
    owb = owb_rmt_initialize(&rmt_driver_info, ds_pin, RMT_CHANNEL_1, RMT_CHANNEL_0);
    owb_use_crc(owb, true);  // enable CRC check for ROM code

    num_devices = scanTempSensorNetwork(device_rom_codes);
    printf("Found %d device%s on oneWire network\n", num_devices, num_devices == 1 ? "" : "s");

    loadSavedSensors(saved_rom_codes);
    int n_knownSensors = generateSensorMap();

    ESP_LOGI(tag, "Recognized %d device%s", n_knownSensors, n_knownSensors == 1 ? "" : "s");

    // Create DS18B20 devices on the 1-Wire bus
    for (int i = 0; i < num_devices; ++i) {
        DS18B20_Info * ds18b20_info = ds18b20_malloc();  // heap allocation
        devices[i] = ds18b20_info;

        if (num_devices == 1) {
            printf("Single device optimisations enabled\n");
            ds18b20_init_solo(ds18b20_info, owb);          // only one device on bus
        } else {
            ds18b20_init(ds18b20_info, owb, device_rom_codes[i]); // associate with bus and device
        }
        ds18b20_use_crc(ds18b20_info, true);           // enable CRC check for temperature readings
        ds18b20_set_resolution(ds18b20_info, res);
    }

    ESP_LOGI(tag, "Setting up tempQueue");
    tempQueue = xQueueCreate(10, sizeof(float[5]));
    flowRateQueue = xQueueCreate(10, sizeof(float));
    ESP_LOGI(tag, "TempQueue initialized");
    ESP_LOGI(tag, "Sensor network initialized");

    return ESP_OK;
}

int scanTempSensorNetwork(OneWireBus_ROMCode rom_codes[MAX_DEVICES])
{
    OneWireBus_SearchState search_state = {0};
    bool found = false;
    int n_devices = 0;
    owb_search_first(owb, &search_state, &found);
    while (found) {
        char rom_code_s[17];
        owb_string_from_rom_code(search_state.rom_code, rom_code_s, sizeof(rom_code_s));
        printf("  %d : %s\n", n_devices, rom_code_s);
        rom_codes[n_devices] = search_state.rom_code;
        ++n_devices;
        owb_search_next(owb, &search_state, &found);
    }

    return n_devices;
}

esp_err_t init_timer(void)
{
    timer_config_t config;
    config.divider = 2;
    config.counter_dir = TIMER_COUNT_UP;
    config.alarm_en = TIMER_ALARM_DIS;
    timer_init(TIMER_GROUP_0, TIMER_0, &config);

    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);
    timer_start(TIMER_GROUP_0, TIMER_0);

    return ESP_OK;
}

void temp_sensor_task(void *pvParameters) 
{
    float sensorTemps[5] = {0};
    portTickType xLastWakeTime = xTaskGetTickCount();
    BaseType_t ret;

    while (1) 
    {
        // Zero temperature array
        for (int i = 0; i < 5; i++) {
            sensorTemps[i] = 0;
        }

        readTemps(sensorTemps);
        ret = xQueueSend(tempQueue, sensorTemps, 100 / portTICK_PERIOD_MS);
        if (ret == errQUEUE_FULL) {
            ESP_LOGI(tag, "Flow rate queue full");
        }
        
        vTaskDelayUntil(&xLastWakeTime, SAMPLE_PERIOD / portTICK_PERIOD_MS);
    }
}

void flowmeter_task(void *pvParameters) 
{
    float flowRate;
    BaseType_t ret;
    portTickType xLastWakeTime = xTaskGetTickCount();
    double currTime;
    ESP_LOGI(tag, "Running flowmeter task");

    while (true) {
        timer_get_counter_time_sec(TIMER_GROUP_0, TIMER_0, &currTime);
        if (currTime > 1) {
            flowRate = 0;
        } else {
            flowRate = (float) 1 / (timeVal * 7.5);
            printf("Flowrate: %.2f\n", flowRate);
        }
        ret = xQueueSend(flowRateQueue, &flowRate, 100 / portTICK_PERIOD_MS);
        if (ret == errQUEUE_FULL) {
            ESP_LOGI(tag, "Flow rate queue full");
        }
        vTaskDelayUntil(&xLastWakeTime, 500 / portTICK_PERIOD_MS);
    }
}

void IRAM_ATTR flowmeter_ISR(void* arg)
{
    double timeTemp;
    timer_get_counter_time_sec(TIMER_GROUP_0, TIMER_0, &timeTemp);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);
    timeVal = timeTemp;
}

void readTemps(float sensorTemps[])
{
    // Read temperatures more efficiently by starting conversions on all devices at the same time
    if (num_devices > 0) {
        ds18b20_convert_all(owb);

        // In this application all devices use the same resolution,
        // so use the first device to determine the delay
        ds18b20_wait_for_conversion(devices[0]);

        for (int i = 0; i < num_devices; ++i) {
            ds18b20_read_temp(devices[i], &sensorTemps[i]);
        }
    }
}

int generateSensorMap(void)
{
    int matchedDevices = 0;
    ESP_LOGI(tag, "Generating sensor map");

    // Forget all devices
    for (int i = 0; i < MAX_DEVICES; i++) {
        savedSensorMap[i] = MAX_DEVICES - 1;
    }

    for (int i = 0; i < MAX_DEVICES; i++) {
        for (int j = 0; j < MAX_DEVICES; j++) {
            if (matchSensor(saved_rom_codes[i], device_rom_codes[j])) {
                ESP_LOGI(tag, "Mapping sensor %d to address index %d", i, j);
                savedSensorMap[i] = j;
                matchedDevices++;
            }
        }
    }

    return matchedDevices;
}

bool matchSensor(OneWireBus_ROMCode matchAddr, OneWireBus_ROMCode deviceAddr)
{
    bool matched = true;

    for (int i = 0; i < 8; i++) {
        if (matchAddr.bytes[i] != deviceAddr.bytes[i]) {
            matched = false;
            break;
        }
    }

    return matched;
}

float getTemperature(float storedTemps[n_tempSensors], tempSensor sensor)
{
    int sensorIdx = savedSensorMap[sensor];

    if (sensorIdx >= MAX_DEVICES) {
        ESP_LOGW(tag, "READ FAILED: Attempted to access sensor index out of range (%d).", sensorIdx);
        return 0.0;
    }

    return storedTemps[sensorIdx];
}

#ifdef __cplusplus
}
#endif