#pragma once

#define MENU_STATIC 0
#define MAX_MENU_ITEMS 6

void menu_task(void* param);

typedef enum {
    menuItem_none,
    menuItem_int,
    menuItem_float,
    menuItem_string
} menuItemTyepe_t;

typedef enum {
    menu_return,
    menu_exit
} menuReturn_t;

typedef enum {
    btnEvent_none,
    btnEvent_valid,
    signal_init
} signal_t;

typedef struct {
    char* fieldName;
    menuItemTyepe_t type;
    int intVal;
    float floatVal;
    char* stringVal;
    void (*fn)(void);
} menuItem_t;

typedef struct {
    char* title;
    int n_items;
    int currIndex;
    menuItem_t optionTable[MAX_MENU_ITEMS];
} menu_t;

