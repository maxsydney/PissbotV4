#ifdef __cplusplus
extern "C" {
#endif

#include <driver/i2c.h>
#include <string.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "HD44780.h"
#include "LCD.h"
#include "controlLoop.h"
#include "main.h"
#include "menu.h"
#include "input.h"
#include "networking.h"
#include "sensors.h"

static char tag[] = "LCD test";
static menuStack_t menuStack;
static bool menuChangedFlag;

// Allocate memory for menu item data fields
static int32_t setpoint, P_gain, I_gain, D_gain;

// Screens
void mainScreen(int btn);
void tunePGain(int btn);
void tuneIGain(int btn);
void tuneDGain(int btn);
void tuneSetpoint(int btn);
void scanSensorHead(int btn);
// void scanSensorBoiler(int btn);
// void scanSensorReflux(int btn);
// void scanSensorProduct(int btn);
// void scanSensorRadiator(int btn);

// Menu handling functions
static void runMenu(menu_t *menu, inputType btn);
static void drawMenuItem(menu_t *menu, int index, int offset);
static void drawMenuItems(menu_t* menu);

// Get data for menu items
static void loadControllerSettings(void);

// Define menus 
static menu_t tuneControllerMenu = {
    .title = "Tune Controller",
    .n_items = 4,
    .init = true,
    .optionTable = {
        {.fieldName = "P Gain   ", .type = menuItem_int, .intVal = &P_gain, .fn = tunePGain},
        {.fieldName = "I Gain   ", .type = menuItem_int, .intVal = &I_gain, .fn = tuneIGain},
        {.fieldName = "D Gain   ", .type = menuItem_int, .intVal = &D_gain, .fn = tuneDGain},
        {.fieldName = "Setpoint ", .type = menuItem_int, .intVal = &setpoint, .fn = tuneSetpoint}
    }
};

static menu_t assignSensorsMenu = {
    .title = "Assign Sensors",
    .n_items = 5,
    .init = true,
    .optionTable = {
        {.fieldName = "Head     ", .type = menuItem_none, .fn = scanSensorHead},
        {.fieldName = "Boiler   ", .type = menuItem_none, .fn = mainScreen},
        {.fieldName = "Reflux   ", .type = menuItem_none, .fn = mainScreen},
        {.fieldName = "Product  ", .type = menuItem_none, .fn = mainScreen},
        {.fieldName = "Radiator ", .type = menuItem_none, .fn = mainScreen}
    }
};

static menu_t mainMenu = {
    .title = "Pissbot V3.0",
    .n_items = 3,
    .init = true,
    .optionTable = {
        {.fieldName = "Main Screen    ", .type = menuItem_none, .fn = mainScreen},
        {.fieldName = "Tune PID       ", .type = menuItem_none, .subMenu = &tuneControllerMenu},
        {.fieldName = "Assign Sensors ", .type = menuItem_none, .subMenu = &assignSensorsMenu}
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

        loadControllerSettings();

        if (uxQueueMessagesWaiting(inputQueue)) {
            xQueueReceive(inputQueue, &btnEvent, 50 / portTICK_PERIOD_MS);
            xQueueReset(inputQueue);
        }

        runMenu(menuStack_peek(), btnEvent.button);
        vTaskDelay(150 / portTICK_RATE_MS);
    }
}

static void runMenu(menu_t *menu, inputType btn)
{
    if (menu->init && !menu->runFunction) {
        LCD_clearScreen();
        menu->init = false;
        drawMenuItems(menu);
        return;
    }

    // Handle keypress logic
    if (!menu->runFunction) {
        // Handle keypresses to navigate menus
        if ((btn == input_down) && (menu->currIndex < (menu->n_items - 1))) {
            menu->currIndex++;
        } else if ((btn == input_up) && (menu->currIndex > 0)) {
            menu->currIndex--;
        } else if (btn == input_mid) {
            if (menu->optionTable[menu->currIndex].subMenu == NULL && menu->optionTable[menu->currIndex].fn != NULL) {
                // Case where menu item is a screen/function
                menu->runFunction = true;
                menu->init = true;
            } else if (menu->optionTable[menu->currIndex].subMenu != NULL && menu->optionTable[menu->currIndex].fn == NULL)  {
                // Enter submenu
                menu->init = true;      // Reset old menu so when we return we re-init
                menuStack_push(menu->optionTable[menu->currIndex].subMenu);
                return;
            } else {
                // Middle button should do nothing
                return;
            }
        } else if (btn == input_left) {
            if (menuStack_pop() == ESP_OK) {
                menu->init = true;
            }
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

    drawMenuItems(menu);
}

static void drawMenuItems(menu_t* menu)
{
    int offset = 0;
    if (menu->currIndex >= 3) {
        offset = menu->currIndex - 2;
    }
    // Draw the menu
    LCD_home();
    LCD_writeStr(menu->title);
    for (int i = offset; i < offset + 3; i++) {
        drawMenuItem(menu, i, offset);
    }
}

static void drawMenuItem(menu_t *menu, int index, int offset)
{
    char dataBuffer[16];
    if (menu->currIndex == index) {
        LCD_setCursor(0, index - offset + 1);
        LCD_writeStr("> ");
    } else {
        LCD_setCursor(0, index - offset + 1);
        LCD_writeStr("  ");
    }

    // Restrict access to valid menu items in array
    if (index < menu->n_items) {
        LCD_writeStr(menu->optionTable[index].fieldName);
        LCD_setCursor(14, index - offset + 1);
        if(menu->optionTable[index].type == menuItem_int) {
            snprintf(dataBuffer, sizeof(dataBuffer), "%5d", *(menu->optionTable[index].intVal));
            LCD_writeStr(dataBuffer);
        }
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

    if (btn == input_left) {
        initScreen = true;
    }

    updateTemperatures(temps);
        
    LCD_setCursor(4, 1);
    snprintf(txtBuf, 6, "%.2f", getTemperature(temps, T_refluxHot));
    LCD_writeStr(txtBuf);

    LCD_setCursor(14, 1);
    snprintf(txtBuf, 6, "%.2f", getTemperature(temps, T_refluxCold));
    LCD_writeStr(txtBuf);

    LCD_setCursor(4, 2);
    snprintf(txtBuf, 6, "%.2f", getTemperature(temps, T_productHot));
    LCD_writeStr(txtBuf);

    LCD_setCursor(14, 2);
    snprintf(txtBuf, 6, "%.2f", getTemperature(temps, T_productCold));
    LCD_writeStr(txtBuf);

    LCD_setCursor(8, 3);
    snprintf(txtBuf, 6, "%.2f", getTemperature(temps, T_boiler));
    LCD_writeStr(txtBuf);
}

static void loadControllerSettings(void)
{
    Data settings = get_controller_settings();
    setpoint = settings.setpoint;
    P_gain = settings.P_gain;
    I_gain = settings.I_gain;
    D_gain = settings.D_gain;
}

void tunePGain(int btn)
{
    static bool initScreen = true;
    static int32_t P_gain_local;
    char dataBuffer[5];

    if (initScreen) {
        LCD_clearScreen();
        LCD_setCursor(0, 0);
        LCD_writeStr("P Gain - ");
        P_gain_local = P_gain;
        initScreen = false;
    }

    if (btn == input_up) {
        P_gain_local++;
    } else if (btn == input_down) {
        P_gain_local--;
    } else if (btn == input_left) {
        Data updateData = get_controller_settings();
        updateData.P_gain = P_gain_local;
        P_gain = P_gain_local;
        write_nvs(&updateData);
        xQueueSend(dataQueue, &updateData, 50);
        initScreen = true;
    }

    snprintf(dataBuffer, 5, "%4d", P_gain_local);
    LCD_setCursor(10, 0);
    LCD_writeStr(dataBuffer);
}

void tuneIGain(int btn)
{
    static bool initScreen = true;
    static int32_t I_gain_local;
    char dataBuffer[5];

    if (initScreen) {
        LCD_clearScreen();
        LCD_setCursor(0, 0);
        LCD_writeStr("I Gain - ");
        I_gain_local = I_gain;
        initScreen = false;
    }

    if (btn == input_up) {
        I_gain_local++;
    } else if (btn == input_down) {
        I_gain_local--;
    } else if (btn == input_left) {
        Data updateData = get_controller_settings();
        updateData.I_gain = I_gain_local;
        I_gain = I_gain_local;
        write_nvs(&updateData);
        xQueueSend(dataQueue, &updateData, 50);
        initScreen = true;
    }

    snprintf(dataBuffer, 5, "%4d", I_gain_local);
    LCD_setCursor(10, 0);
    LCD_writeStr(dataBuffer);
}

void tuneDGain(int btn)
{
    static bool initScreen = true;
    static int32_t D_gain_local;
    char dataBuffer[5];

    if (initScreen) {
        LCD_clearScreen();
        LCD_setCursor(0, 0);
        LCD_writeStr("P Gain - ");
        D_gain_local = D_gain;
        initScreen = false;
    }

    if (btn == input_up) {
        D_gain_local++;
    } else if (btn == input_down) {
        D_gain_local--;
    } else if (btn == input_left) {
        Data updateData = get_controller_settings();
        updateData.D_gain = D_gain_local;
        D_gain = D_gain_local;
        write_nvs(&updateData);
        xQueueSend(dataQueue, &updateData, 50);
        initScreen = true;
    }

    snprintf(dataBuffer, 5, "%4d", D_gain_local);
    LCD_setCursor(10, 0);
    LCD_writeStr(dataBuffer);
}

void tuneSetpoint(int btn)
{
    static bool initScreen = true;
    static int32_t setpoint_local;
    char dataBuffer[5];

    if (initScreen) {
        LCD_clearScreen();
        LCD_setCursor(0, 0);
        LCD_writeStr("P Gain - ");
        setpoint_local = setpoint;
        initScreen = false;
    }

    if (btn == input_up) {
        setpoint_local++;
    } else if (btn == input_down) {
        setpoint_local--;
    } else if (btn == input_left) {
        Data updateData = get_controller_settings();
        updateData.setpoint = setpoint_local;
        setpoint = setpoint_local;
        write_nvs(&updateData);
        xQueueSend(dataQueue, &updateData, 50);
        initScreen = true;
    }

    snprintf(dataBuffer, 5, "%4d", setpoint_local);
    LCD_setCursor(10, 0);
    LCD_writeStr(dataBuffer);
}

void scanSensorHead(int btn)
{
    static bool initScreen = true;      // Replace these with state machine
    static bool searched = false;
    static int n_found = 0;
    static bool written = false;
    static OneWireBus_ROMCode rom_codes[MAX_DEVICES] = {0};     // A bit wasteful of memory, can change max devices to 1 if necessary

    if (btn == input_left) {
        initScreen = true;
        searched = false;
        n_found = 0;
        memset(rom_codes, 0, sizeof(OneWireBus_ROMCode[MAX_DEVICES]));
    } else if (btn == input_mid && n_found == 1) {
        saved_rom_codes[T_refluxHot] = rom_codes[0];
        writeDeviceRomCodes(saved_rom_codes);
        written = true;
    }

    if (written) {
        LCD_clearScreen();
        LCD_setCursor(0, 0);
        LCD_writeStr("Head");
        LCD_setCursor(0, 2);
        LCD_writeStr("Sensor saved!");
        written = false;
    }

    if (searched) {
        LCD_clearScreen();
        LCD_setCursor(0, 0);
        LCD_writeStr("Head");
        LCD_setCursor(0, 2);

        if (n_found == 0) {
            LCD_writeStr("No sensors found");
        } else if (n_found == 1) {
            LCD_writeStr("1 sensor found");
            LCD_setCursor(0, 3);
            LCD_writeStr("Enter to accept");
        } else {
            LCD_writeStr("More than 1");
            LCD_setCursor(0, 3);
            LCD_writeStr("sensor found");
        }
        searched = false;
    }

    if (initScreen) {
        LCD_clearScreen();
        LCD_setCursor(0, 0);
        LCD_writeStr("Head");
        LCD_setCursor(0, 2);
        LCD_writeStr("Scanning");
        n_found = scanTempSensorNetwork(rom_codes);
        searched = true;
        initScreen = false;
    }
}

#ifdef __cplusplus
}
#endif