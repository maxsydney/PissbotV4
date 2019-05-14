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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "ds18b20.h"
#include <esp_err.h>
#include <esp_log.h>
#include "string.h"
#include "main.h"

static const char* tag = "ds18b20";

int DS_GPIO;
int init = 0;
int sensorCount;

char Ds18B20_Addresses[5][8];

static unsigned char dscrc_table[] = {
        0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
      157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
       35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
      190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
       70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
      219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
      101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
      248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
      140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
       17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
      175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
       50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
      202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
       87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
      233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
      116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};

unsigned char ROM_NO[8];
int LastDiscrepancy;
int LastFamilyDiscrepancy;
int LastDeviceFlag;
unsigned char crc8;

static uint8_t _crc_ibutton_update(uint8_t crc, uint8_t data);
static bool selectSensor(char* addr);

void ds18b20_init(int GPIO)
{
  DS_GPIO = GPIO;
  gpio_pad_select_gpio(DS_GPIO);
  init=1;
  printf("Beginning search routine\n");
  doSearch();
  ds18b20_set_resolution(Ds18B20_Addresses[0], 10);
  ds18b20_set_resolution(Ds18B20_Addresses[1], 10);
  ds18b20_set_resolution(Ds18B20_Addresses[2], 10);
  ds18b20_set_resolution(Ds18B20_Addresses[3], 10);
}

/// Sends one bit to bus
void ds18b20_send(char bit)
{
    gpio_set_direction(DS_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DS_GPIO, 0);
    ets_delay_us(6);
    if(bit == 1) {
        gpio_set_level(DS_GPIO,1);
    }
    ets_delay_us(64);
    gpio_set_level(DS_GPIO,1);
}

// Reads one bit from bus
unsigned char ds18b20_read(void)
{
    unsigned char bit = 0;
    gpio_set_direction(DS_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DS_GPIO,0);
    ets_delay_us(1);
    // gpio_set_level(DS_GPIO,1);
    gpio_set_direction(DS_GPIO, GPIO_MODE_INPUT);
    ets_delay_us(15);
    bit = gpio_get_level(DS_GPIO);
    ets_delay_us(15);
    return(bit);
}

// Sends one byte to bus
void ds18b20_send_byte(char data){
    unsigned char i;
    unsigned char x;
    for(i=0;i<8;i++) {
        x = data>>i;
        x &= 0x01;
        ds18b20_send(x);
    }
}

// Reads one byte from bus
unsigned char ds18b20_read_byte(void)
{
    unsigned char i;
    unsigned char data = 0;
    for (i=0;i<8;i++) {
        if(ds18b20_read()) {
            data |= 0x01<<i;
        }
    }
    return(data);
}

// Sends reset pulse
unsigned char ds18b20_RST_PULSE(void)
{
    unsigned char PRESENCE;
    gpio_set_direction(DS_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DS_GPIO,0);
    ets_delay_us(480);
    gpio_set_level(DS_GPIO,1);
    gpio_set_direction(DS_GPIO, GPIO_MODE_INPUT);
    ets_delay_us(70);
    if(gpio_get_level(DS_GPIO) == 0) {
        PRESENCE = 1;
    } else {
        PRESENCE = 0;
    }
    ets_delay_us(405);
    return PRESENCE;
}

esp_err_t ds18b20_set_resolution(char* addr, int res)
{
    char configByte;
    if (res < 9 || res > 12) {
        return ESP_FAIL;
    }

    configByte = ((res - 9) << 5) | 0x1F;
    if (selectSensor(addr)) {
        ds18b20_send_byte(0x4E);
        ds18b20_send_byte(0x00);
        ds18b20_send_byte(0x00);
        ds18b20_send_byte(configByte);
    }

    if (ds18b20_get_resolution(addr) == res) {
        ESP_LOGI(tag, "Resolution sucessfully set to %d bits", res);
        return ESP_OK;
    } else {
        ESP_LOGE(tag, "Resolution failed to set to %d bits", res);
        return ESP_FAIL;
    }
}

int readPowerSupply(tempSensor sens)
{
    char ret = 0;
    char* addr = Ds18B20_Addresses[sens];
    if (selectSensor(addr)) {
        ds18b20_send_byte(0xB4);
        ret = ds18b20_read_byte();
    } 
    return ret;
}

int ds18b20_get_resolution(char* addr)
{
    char scratchPad[8];
    char inByte, config, sensor_crc, crc = 0x00;
    int res = -1;
    if (selectSensor(addr)) {
        ds18b20_send_byte(0xBE);
        for (int i = 0; i < 8; i++) {
            inByte = ds18b20_read_byte();
            scratchPad[i] = inByte;
            crc = _crc_ibutton_update(crc, inByte);
        }
        sensor_crc = ds18b20_read_byte();
        if (crc == sensor_crc) {
            config = scratchPad[4];
            res = ((config & 0x60) >> 5) + 9;
        }
    }
    return res;
}

static bool selectSensor(char* addr)
{
    unsigned char check = ds18b20_RST_PULSE();
    if (check) {
        ds18b20_send_byte(0x55);
        for (int i = 0; i < 8; i++) {
            ds18b20_send_byte(addr[i]);
        }
        return true;
    }
    return false;
}

int initiateConversion(void)
{
    int ret = 0;
    if (ds18b20_RST_PULSE()) {
        ds18b20_send_byte(0xCC);
        ds18b20_send_byte(0x44);
        ret = 1;
    }
    return ret;
}

// Returns temperature from sensor
float ds18b20_get_temp(tempSensor sens) 
{
    char* addr = Ds18B20_Addresses[sens];
    char dataBuffer[8];
    char crc = 0x00;
    unsigned char check;
    float temp = 0;
    check = ds18b20_RST_PULSE();

    if (check == 1) {

        selectSensor(addr);
        ds18b20_send_byte(0xBE);

        for (int i = 0; i < 8; i++) {
            uint8_t newByte = (uint8_t) ds18b20_read_byte();
            crc = _crc_ibutton_update(crc, newByte);
            dataBuffer[i] = newByte;
        }

        char sensor_crc = ds18b20_read_byte();
        
        if (crc == sensor_crc) {
            temp = (float) (dataBuffer[0] | dataBuffer[1] << 8) * 0.0625;
            return temp;
        } else {
            ESP_LOGE(tag, "CRC failed on channel %d", sens);
            ESP_LOGE(tag, "Failed data read from sensor: (%02X - %02X)", crc, sensor_crc);
            for (int i = 0; i < 8; i++) {
                printf("0x%02X ", dataBuffer[i]);
            }
            printf("\n");
            return 0;
        }
    } else {
        ESP_LOGE(tag, "Sensor failed to respond on channel %d", sens);
        return 0;
    }
}

static uint8_t _crc_ibutton_update(uint8_t crc, uint8_t data)
{
	uint8_t i;

	crc = crc ^ data;
	for (i = 0; i < 8; i++) {
	    if (crc & 0x01) {
            crc = (crc >> 1) ^ 0x8C;
        } else {
            crc >>= 1;
        }
	}

	return crc;
}

//--------------------------------------------------------------------------
// Find the 'first' devices on the 1-Wire bus
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : no device present
//
int OWFirst()
{
   // reset the search state
   LastDiscrepancy = 0;
   LastDeviceFlag = false;
   LastFamilyDiscrepancy = 0;

   return OWSearch();
}

//--------------------------------------------------------------------------
// Find the 'next' devices on the 1-Wire bus
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : device not found, end of search
//
int OWNext()
{
   // leave the search state alone
   return OWSearch();
}

//--------------------------------------------------------------------------
// Perform the 1-Wire Search Algorithm on the 1-Wire bus using the existing
// search state.
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : device not found, end of search
//
int OWSearch()
{
    int id_bit_number;
    int last_zero, rom_byte_number, search_result;
    int id_bit, cmp_id_bit;
    unsigned char rom_byte_mask, search_direction;

    // initialize for search
    id_bit_number = 1;
    last_zero = 0;
    rom_byte_number = 0;
    rom_byte_mask = 1;
    search_result = 0;
    crc8 = 0;

    // if the last call was not the last one
    if (!LastDeviceFlag)
    {
        // 1-Wire reset
        if (!ds18b20_RST_PULSE()) {
            // reset the search
            LastDiscrepancy = 0;
            LastDeviceFlag = false;
            LastFamilyDiscrepancy = 0;
            return false;
        }

        // issue the search command 
        ds18b20_send_byte(0xF0);  

        // loop to do the search
        do {
            // read a bit and its complement
            id_bit = ds18b20_read();
            cmp_id_bit = ds18b20_read();

            // check for no devices on 1-wire
            if ((id_bit == 1) && (cmp_id_bit == 1)) {
                printf("No devices on bus\n");
                break;
            } else {
                // all devices coupled have 0 or 1
                if (id_bit != cmp_id_bit) {
                    search_direction = id_bit;  // bit write value for search
                } else {
                    // if this discrepancy if before the Last Discrepancy
                    // on a previous next then pick the same as last time
                    if (id_bit_number < LastDiscrepancy) {
                            search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
                    } else {
                        // if equal to last pick 1, if not then pick 0
                        search_direction = (id_bit_number == LastDiscrepancy);
                    }

                    // if 0 was picked then record its position in LastZero
                    if (search_direction == 0) {
                        last_zero = id_bit_number;

                        // check for Last discrepancy in family
                        if (last_zero < 9) {
                            LastFamilyDiscrepancy = last_zero;
                        }
                    }
                }

                // set or clear the bit in the ROM byte rom_byte_number
                // with mask rom_byte_mask
                if (search_direction == 1) {
                    ROM_NO[rom_byte_number] |= rom_byte_mask;
                } else {
                    ROM_NO[rom_byte_number] &= ~rom_byte_mask;
                }
                
                // serial number search direction write bit
                ds18b20_send(search_direction);

                // increment the byte counter id_bit_number
                // and shift the mask rom_byte_mask
                id_bit_number++;
                rom_byte_mask <<= 1;

                // if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
                if (rom_byte_mask == 0) {
                    docrc8(ROM_NO[rom_byte_number]);  // accumulate the CRC
                    rom_byte_number++;
                    rom_byte_mask = 1;
                }
            }
        }
        while(rom_byte_number < 8);  // loop until through all ROM bytes 0-7

        // if the search was successful then
        if (!((id_bit_number < 65) || (crc8 != 0))) {
            // search successful so set LastDiscrepancy,LastDeviceFlag,search_result
            LastDiscrepancy = last_zero;

            // check for last device
            if (LastDiscrepancy == 0) {
                LastDeviceFlag = true;
            }    
            
            search_result = true;
        }
    }

    // if no device found then reset counters so next 'search' will be like a first
    if (!search_result || !ROM_NO[0]) {
        LastDiscrepancy = 0;
        LastDeviceFlag = false;
        LastFamilyDiscrepancy = 0;
        search_result = false;
    }

   return search_result;
}

unsigned char docrc8(unsigned char value)
{
   // See Application Note 27
   crc8 = dscrc_table[crc8 ^ value];
   return crc8;
}

void doSearch(void)
{
    int addrIndex = 0;
    bool rslt = OWFirst();
    while (rslt) {
        // print device found
        printf("New device found!\n");
        printf("ID: ");
        for (int i = 7; i >= 0; i--) {
            Ds18B20_Addresses[addrIndex][i] = ROM_NO[i];
            printf("%02X", ROM_NO[i]);
        }
            
        printf("\n");
        rslt = OWNext();
        addrIndex++;
    }

    printf("Found %d devices on OneWire bus\n", addrIndex);
    sensorCount = addrIndex;
}

void swapTempSensors(void)
{
    char temp[8];
    printf("Address 1: ");
    for (int i = 7; i >= 0; i--) {
        printf("%x ", Ds18B20_Addresses[0][i]);
    }

    printf("\nAddress 2: ");
    for (int i = 7; i >= 0; i--) {
        printf("%x ", Ds18B20_Addresses[1][i]);
    }
    printf("------------------\n");
    memcpy(temp, Ds18B20_Addresses[0], 8);
    memcpy(Ds18B20_Addresses[0], Ds18B20_Addresses[1], 8);
    memcpy(Ds18B20_Addresses[1], temp, 8);

    printf("Address 1: ");
    for (int i = 7; i >= 0; i--) {
        printf("%x ", Ds18B20_Addresses[0][i]);
    }

    printf("\nAddress 2: ");
    for (int i = 7; i >= 0; i--) {
        printf("%x ", Ds18B20_Addresses[1][i]);
    }

}