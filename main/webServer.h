#pragma once

/*
*   --------------------------------------------------------------------  
*   websocket_task
*   --------------------------------------------------------------------
*   Handles websocket connection with browser clients and passes data
*   back and forth between ESP32 and the browser
*/
void websocket_task(void *pvParameters);

/*
*   --------------------------------------------------------------------  
*   webServer_init
*   --------------------------------------------------------------------
*   Spins up an http server to serve static browser client files
*/
void webServer_init(void);