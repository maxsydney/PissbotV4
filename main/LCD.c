#include <driver/i2c.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "HD44780.h"
#include "LCD.h"
#include "controller.h"
#include "main.h"
#include "menu.h"
#include "input.h"

static char tag[] = "LCD test";
static menu_t currMenu;

void function1(void);
void function2(void);
void function3(void);
void mainScreen(void);

static void runMenu(menu_t *menu, signal_t signal, inputType btn);
static void drawMenuItem(menu_t *menu, int index);

static menu_t mainMenu = {
    .title = "Pissbot V3.0",
    .n_items = 3,
    .optionTable = {
        {.fieldName = "Main Screen", .type = menuItem_none, .fn = mainScreen},
        {.fieldName = "Tune PID", .type = menuItem_none, .fn = NULL},
        {.fieldName = "Settings", .type = menuItem_string, .stringVal = "Gaan", .fn = NULL}
    }
};

void menu_task(void* param)
{
    currMenu = mainMenu;
    printf("MainScreen address: %p\n", mainMenu.optionTable[0].fn);
    buttonPress btnEvent;
    signal_t signal;
    runMenu(&currMenu, signal_init, 0);

    while (true) {
        signal = btnEvent_none;

        if (uxQueueMessagesWaiting(inputQueue)) {
            xQueueReceive(inputQueue, &btnEvent, 50 / portTICK_PERIOD_MS);
            if (debounceInput(btnEvent)) {
                signal = btnEvent_valid; 
            }
        }

        runMenu(&currMenu, signal, btnEvent.button);
        vTaskDelay(50 / portTICK_RATE_MS);
    }
}

static void runMenu(menu_t *menu, signal_t signal, inputType btn)
{
    static bool runFunction = false;

    // Run function if required
    if (runFunction) {
        menu->optionTable[menu->currIndex].fn ();
        if(btn == input_left) {
            runFunction = false;
            LCD_clearScreen();
        }
        return;
    }
    
    // If no button has been pressed, and the menu does not require init, then we don't need to do anything
    if (signal == btnEvent_none) {
        return;
    }

    // Change highlighted menu item if required
    if (signal == btnEvent_valid) {
        if ((btn == input_down) && (menu->currIndex < menu->n_items - 1)) {
            menu->currIndex++;
        } else if ((btn == input_up) && (menu->currIndex > 0)) {
            menu->currIndex--;
        }
    }

    // Change runFunction if required
    if (btn == input_mid) {
        runFunction = true;
    }

    // Draw the menu
    LCD_home();
    LCD_writeStr(menu->title);
    for (int i = 0; i < menu->n_items; i++) {
        drawMenuItem(menu, i);
    }
}

static void drawMenuItem(menu_t *menu, int index)
{
    if (menu->currIndex == index) {
        LCD_setCursor(0, index + 1);
        LCD_writeStr("> ");
        LCD_writeStr(menu->optionTable[index].fieldName);
    } else {
        LCD_setCursor(0, index + 1);
        LCD_writeStr("  ");
        LCD_writeStr(menu->optionTable[index].fieldName);
    }
}

void mainScreen(void)
{
    static bool drawScreen = true;
    char txtBuf[8];
    float temps[n_tempSensors];

    // Only draw static items on screen if entering from menu. Otherwise just fill variables
    if (drawScreen) {
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
        drawScreen = false;
    }
    printf("Updating temperatures\n");
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
}

void LCD_task(void* param)
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

void function1(void)
{
    printf("Accessed menu item 1");
}

void function2(void)
{
    printf("Accessed menu item 2");
}

void function3(void)
{
    printf("Accessed menu item 3");
}