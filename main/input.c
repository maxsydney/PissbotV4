#include <driver/gpio.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio.h"
#include "sensors.h"
#include "main.h"
#include "pinDefs.h"
#include "input.h"

static char tag[] = "Input";
xQueueHandle inputQueue;

esp_err_t init_input(void)
{
    inputQueue = xQueueCreate(10, sizeof(buttonPress));
    return ESP_OK;
}

void IRAM_ATTR input_ISR_handler(void* arg)
{
    buttonPress btnEvent;
    btnEvent.button = (uint32_t) arg;
    btnEvent.time = esp_timer_get_time();
    xQueueSendFromISR(inputQueue, &btnEvent, pdFALSE);
}

bool debounceInput(buttonPress btnEvent)
{
    static int64_t currTime = 0;
    bool validPress = false;

    if (btnEvent.time - currTime > 500000) {
        currTime = btnEvent.time;
        validPress = true;
    }

    return validPress;
}
