#pragma once

#include "menu.h"

#define MENU_STATIC 0
#define MAX_MENU_ITEMS 6
#define MAX_MENU_DEPTH 5

void menu_task(void* param);

typedef struct menu menu_t;

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

typedef struct menuItem {
    char* fieldName;
    menuItemTyepe_t type;
    int* intVal;
    float* floatVal;
    char* stringVal;
    menu_t* subMenu;
    void (*fn) (int);
} menuItem_t;

typedef struct menu {
    char* title;
    int n_items;
    bool init;
    int currIndex;
    bool runFunction;
    menuItem_t optionTable[MAX_MENU_ITEMS];
} menu_t;

typedef struct {
    int n_items;
    menu_t *menus[MAX_MENU_DEPTH];
} menuStack_t;

