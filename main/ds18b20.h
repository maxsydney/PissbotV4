/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef DS18B20_H_  
#define DS18B20_H_

int sensorCount;

#include "main.h"

/*
*   --------------------------------------------------------------------  
*   ds18b20_send
*   --------------------------------------------------------------------
*   Sends one bit to the oneWire bus
*/
void ds18b20_send(char bit);

/*
*   --------------------------------------------------------------------  
*   ds18b20_read
*   --------------------------------------------------------------------
*   Reads one bit from the oneWire bus
*/
unsigned char ds18b20_read(void);

/*
*   --------------------------------------------------------------------  
*   ds18b20_send_byte
*   --------------------------------------------------------------------
*   Sends one byte to the oneWire bus
*/
void ds18b20_send_byte(char data);

/*
*   --------------------------------------------------------------------  
*   ds18b20_read_byte
*   --------------------------------------------------------------------
*   Reads one byte from the oneWire bus
*
*   RETURNS:
*       Byte read from bus
*/
unsigned char ds18b20_read_byte(void);

/*
*   --------------------------------------------------------------------  
*   ds18b20_RST_PULSE
*   --------------------------------------------------------------------
*   Sends a reset pulse signal to the oneWire bus
*
*   RETURNS:
*       Success/failure
*/
unsigned char ds18b20_RST_PULSE(void);

/*
*   --------------------------------------------------------------------  
*   ds18b20_get_temp
*   --------------------------------------------------------------------
*   Retrieves the temperature from a single sensor on the oneWire bus
*   
*   INPUTS:
*       sens - index of the sensor to request temperature from
*
*   RETURNS:
*       Temperature in degrees celsius
*/
float ds18b20_get_temp(tempSensor sens);

/*
*   --------------------------------------------------------------------  
*   ds18b20_init
*   --------------------------------------------------------------------
*   Initialises the oneWire bus. Searches the bus for all avalilable
*   sensors and stores addresses (up to 5 sensors). Sets the 
*   resolution for all discovered sensors to 10 bit
*   
*   INPUTS:
*       GPIO - pin to intialise oneWire bus on
*/
void ds18b20_init(int GPIO);

/*
*   --------------------------------------------------------------------  
*   ds18b20_set_resolution
*   --------------------------------------------------------------------
*   Sets the resolution for a sensor
*   
*   INPUTS:
*       addr - Address of sensor to set resolution
*       res  - Resolution (9, 10, 11, 12 bits)
*
*   RETURNS:
*       Sucess/failure
*/
esp_err_t ds18b20_set_resolution(char* addr, int res);

/*
*   --------------------------------------------------------------------  
*   ds18b20_get_resolution
*   --------------------------------------------------------------------
*   Retrieves the resolution for a sensor
*   
*   INPUTS:
*       addr - Sensor address
*
*   RETURNS:
*       Sensor resolution in bits
*/
int ds18b20_get_resolution(char* addr);

/*
*   --------------------------------------------------------------------  
*   swapTempSensors
*   --------------------------------------------------------------------
*   Swaps the refluxHot/refluxCold sensors
*/
void swapTempSensors(void);

/*
*   --------------------------------------------------------------------  
*   readPowerSupply
*   --------------------------------------------------------------------
*   Reads the power supply mode of a sensor (parasitic, external)
*   
*   INPUTS:
*       sens - Sensor ID
*
*   RETURNS:
*       Sensor power mode
*/
int readPowerSupply(tempSensor sens);

/*
*   --------------------------------------------------------------------  
*   initiateConversion
*   --------------------------------------------------------------------
*   Initiates temperature conversion for all sensors on bus
*   
*   RETURNS:
*       Sucess/failure
*/
int initiateConversion(void);

#endif
