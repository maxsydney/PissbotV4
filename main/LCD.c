#include <driver/i2c.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "HD44780.h"
#include "LCD.h"
#include "controller.h"
#include "main.h"

static char tag[] = "LCD test";

void LCD_DemoTask(void* param)
{
    char txtBuf[8];
    float temps[n_tempSensors];
    LCD_home();
    LCD_clearScreen();
    LCD_writeStr("Pissbot V3.0");
    LCD_setCursor(0, 1);
    LCD_writeStr("T1: ");
    LCD_setCursor(10, 1);
    LCD_writeStr("T2: ");
    LCD_setCursor(0, 2);
    LCD_writeStr("T3: ");
    LCD_setCursor(10, 2);
    LCD_writeStr("T4: ");
    LCD_setCursor(0, 3);
    LCD_writeStr("Boiler: ");
    while (true) {
        getTemperatures(temps);
        
        LCD_setCursor(4, 1);
        snprintf(txtBuf, 6, "%.2f", temps[T_refluxHot]);
        LCD_writeStr(txtBuf);

        LCD_setCursor(14, 1);
        snprintf(txtBuf, 6, "%.2f", temps[T_refluxCold]);
        LCD_writeStr(txtBuf);

        LCD_setCursor(4, 2);
        snprintf(txtBuf, 6, "%.2f", temps[T_productHot]);
        LCD_writeStr(txtBuf);

        LCD_setCursor(14, 2);
        snprintf(txtBuf, 6, "%.2f", temps[T_productCold]);
        LCD_writeStr(txtBuf);

        LCD_setCursor(8, 3);
        snprintf(txtBuf, 4, "%.2f", temps[T_boiler]);
        LCD_writeStr(txtBuf);

        vTaskDelay(200 / portTICK_RATE_MS);
    }
}