#pragma once
#include "controlLoop.h"
#include "cJSON.h"

/*
*   --------------------------------------------------------------------  
*   readCtrlParams
*   --------------------------------------------------------------------
*   Decodes incoming messages with type INFO and subtype ctrlParams. 
*/
ctrlParams_t* readCtrlParams(cJSON* JSON_root);

/*
*   --------------------------------------------------------------------  
*   readCtrlSettings
*   --------------------------------------------------------------------
*   Decodes incoming messages with type INFO and subtype ctrlSettings. 
*/
ctrlSettings_t* readCtrlSettings(cJSON* JSON_root);


