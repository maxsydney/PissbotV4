#include <driver/i2c.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "HD44780.h"
#include "LCD.h"
#include "controller.h"

static char tag[] = "LCD test";

void LCD_DemoTask(void* param)
{
    char txtBuf[8];
    float hotTemp = 0, coldTemp = 0;
    LCD_home();
    LCD_clearScreen();
    LCD_writeStr("Pissbot V3.0");
    LCD_setCursor(0, 1);
    LCD_writeStr("T1: ");
    LCD_setCursor(0, 2);
    LCD_writeStr("T2: ");
    while (true) {
        hotTemp = get_hot_temp();
        coldTemp = get_cold_temp();
        
        LCD_setCursor(4, 1);
        snprintf(txtBuf, 8, "%.2f", hotTemp);
        LCD_writeStr(txtBuf);

        LCD_setCursor(4, 2);
        snprintf(txtBuf, 8, "%.2f", coldTemp);
        LCD_writeStr(txtBuf);

        vTaskDelay(250 / portTICK_RATE_MS);
    }
}