#pragma once

char OTA_IP[128];

/*
*   --------------------------------------------------------------------  
*   ota_update_task
*   --------------------------------------------------------------------
*   Runs task that handles over-the-air software updates
*/
void ota_update_task(void *pvParameter);