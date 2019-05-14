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

void ds18b20_send(char bit);
unsigned char ds18b20_read(void);
void ds18b20_send_byte(char data);
unsigned char ds18b20_read_byte(void);
unsigned char ds18b20_RST_PULSE(void);
float ds18b20_get_temp(tempSensor sens);
void ds18b20_init(int GPIO);
esp_err_t ds18b20_set_resolution(char* addr, int res);
int ds18b20_get_resolution(char* addr);
int OWFirst();
int OWNext();
int OWSearch();
unsigned char docrc8(unsigned char value);
void doSearch(void);
void swapTempSensors(void);
int readPowerSupply(tempSensor sens);
int initiateConversion(void);

#endif
