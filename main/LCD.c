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
static menuStack_t menuStack;
static bool menuChangedFlag;

// Allocate memory for menu item data fields
static int32_t setpoint, P_gain, I_gain, D_gain;

// Screens
void mainScreen(int btn);
void function1(int btn);

// Menu handling functions
static void runMenu(menu_t *menu, inputType btn);
static void drawMenuItem(menu_t *menu, int index);

// Define menus 
static menu_t tuneControllerMenu = {
    .title = "Tune Controller",
    .n_items = 3,
    .optionTable = {
        {.fieldName = "P Gain", .type = menuItem_int, .intVal = &P_gain, .fn = function1},
        {.fieldName = "I Gain", .type = menuItem_int, .intVal = &I_gain, .fn = NULL},
        {.fieldName = "D Gain", .type = menuItem_int, .intVal = &D_gain, .fn = NULL},
        {.fieldName = "Setpoint", .type = menuItem_int, .intVal = &P_gain, .fn = NULL}
    }
};

static menu_t mainMenu = {
    .title = "Pissbot V3.0",
    .n_items = 3,
    .optionTable = {
        {.fieldName = "Main Screen", .type = menuItem_none, .fn = mainScreen},
        {.fieldName = "Tune PID", .type = menuItem_none, .subMenu = &tuneControllerMenu},
        {.fieldName = "Settings", .type = menuItem_string, .stringVal = "Gaan", .fn = NULL}
    }
};

// Menu stack functions
static void menuStack_init()
{
    menuStack.menus[0] = &mainMenu;
}

static void menuStack_push(menu_t *menu)
{
    if (menuStack.n_items < (MAX_MENU_DEPTH - 1)) {
        menuChangedFlag = true;
        menuStack.menus[++menuStack.n_items] = menu;
    }
}

static esp_err_t menuStack_pop(void)
{
    if (menuStack.n_items > 0) {
        menuStack.n_items--;
        return ESP_OK;
    }

    return ESP_FAIL;
}

static menu_t* menuStack_peek(void)
{
    return menuStack.menus[menuStack.n_items];
}

// Menu handling functions
void menu_task(void* param)
{
    buttonPress btnEvent;
    menuStack_init();
    runMenu(menuStack_peek(), input_none);

    while (true) {
        btnEvent.button = input_none;

        if (uxQueueMessagesWaiting(inputQueue)) {
            xQueueReceive(inputQueue, &btnEvent, 50 / portTICK_PERIOD_MS);
            if (debounceInput(btnEvent)) {
                ESP_LOGI(tag, "Button pressed: %d", btnEvent.button);
            }
        }

        runMenu(menuStack_peek(), btnEvent.button);
        vTaskDelay(10 / portTICK_RATE_MS);
    }
}

static void runMenu(menu_t *menu, inputType btn)
{
    if (menu->init ) {
        LCD_clearScreen();
        menu->init = false;
    }

    // Handle keypress logic
    if (!menu->runFunction) {
        // Handle keypresses to navigate menus
        if ((btn == input_down) && (menu->currIndex < menu->n_items - 1)) {
            menu->currIndex++;
        } else if ((btn == input_up) && (menu->currIndex > 0)) {
            menu->currIndex--;
        } else if (btn == input_mid) {
            if (menu->optionTable[menu->currIndex].subMenu == NULL && menu->optionTable[menu->currIndex].fn != NULL) {
                // Case where menu item is a screen/function
                menu->runFunction = true;
                menu->init = true;
            } else {
                // Enter submenu
                menuStack_push(menu->optionTable[menu->currIndex].subMenu);
                menu->init = true;      // Reset old menu so when we return we re-init
                return;
            }
        } else if (btn == input_left) {
            menuStack_pop();
            return;
        } else if (btn == input_none) {
            return;
        }
    } else {
        // Handle escape from menu and pass button to screen/function
        if (btn == input_left) {
            menu->runFunction = false;
        } 
        menu->optionTable[menu->currIndex].fn (btn);
        return;
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

// Menu item functions
void mainScreen(int btn)
{
    char txtBuf[8];
    float temps[n_tempSensors];
    static bool initScreen = true;

    // Only draw static items on screen if entering from button press. Otherwise just fill variables
    if (initScreen) {
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
        initScreen = false;
    }

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

void function1(int unused)
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