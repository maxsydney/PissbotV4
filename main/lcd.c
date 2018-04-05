#include <driver/i2c.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include "sdkconfig.h"

#define SDA_PIN 18
#define SCL_PIN 19
#define LCD_ADDR 0x27
#define CMD_OUT 0x20
static char tag[] = "i2cscanner";

// LCD pin mappings
// A0 -> VCC 
// A1 -> VCC >> These are supplied by 5v, but electrically insulated from VCC pin
// A2 -> VCC 
// P0 -> RS 
// P1 -> RW 
// P2 -> E 
// P3 -> None 
// VSS -> GND 
// P4 -> D4 
// P5 -> D5 
// P6 -> D6 
// P7 -> D7 
// INT -> None 
// SCL -> SCL 
// SDA -> SDA 
// VDD -> VCC

void i2c_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SDA_PIN,
        .scl_io_num = SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };
	i2c_param_config(I2C_NUM_0, &conf);
	i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}

void i2c_write_255(void)
{
    i2c_cmd_handle_t cmd;
    esp_err_t espRc;
	vTaskDelay(200/portTICK_PERIOD_MS);
    int count = 0;
    uint8_t write;
    while (1) {
        if (count++ % 2 == 0) {
            write = 0xFF;
        } else {
            write = 0x00;
        }
        cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (LCD_ADDR << 1) | I2C_MASTER_WRITE, 1);
        i2c_master_write_byte(cmd, write, 1);
        i2c_master_stop(cmd);
        espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
        if (espRc == ESP_FAIL) {
            printf("Sending command error, slave doesn’t ACK the transfer\n");
        } else if (espRc == ESP_ERR_INVALID_STATE) {
            printf("I2C driver not installed or not in master mode\n");
        } else if (espRc == ESP_OK) {
            printf("Success\n");
        }
        i2c_cmd_link_delete(cmd);
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}

static void send_nibble(i2c_cmd_handle_t cmd_handle, uint8_t nibble)
{
    // i2c_master_write(cmd_handle, &nibble, 4, 1);
    vTaskDelay(50 / portTICK_PERIOD_MS);
}

static void send_byte(i2c_cmd_handle_t cmd_handle, uint8_t data)
{
    i2c_master_write_byte(cmd_handle, (CMD_OUT & 0xF0)|((data >> 4) & 0x0F), 1);
    i2c_master_write_byte(cmd_handle, (CMD_OUT & 0xF0)|(data & 0x0F), 1);
    // send_nibble(cmd_handle, (send_byte & 0xF0);
    // send_nibble(cmd_handle, send_byte & 0x0F);
}

void lcd_init(void)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (LCD_ADDR << 1) | I2C_MASTER_WRITE, 1);
    send_byte(cmd, 0x33);
    send_byte(cmd, 0x32);
    send_byte(cmd, 0x28);
    // send_byte(cmd, 0x0E);
    send_byte(cmd, 0x0F);
    send_byte(cmd, 0x01);
    send_byte(cmd, 0x06);
    send_byte(cmd, 0x80);
    i2c_master_stop(cmd);
    esp_err_t espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);   

    if (espRc == ESP_FAIL) {
        printf("Sending command error, slave doesn’t ACK the transfer\n");
    } else if (espRc == ESP_ERR_INVALID_STATE) {
        printf("I2C driver not installed or not in master mode\n");
    } else if (espRc == ESP_OK) {
        printf("Success\n");
    }
}

void task_i2cscanner(void *ignore) {
	ESP_LOGD(tag, ">> i2cScanner");
	i2c_config_t conf;
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = SDA_PIN;
	conf.scl_io_num = SCL_PIN;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = 100000;
	i2c_param_config(I2C_NUM_0, &conf);

	i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);

	int i;
	esp_err_t espRc;
	printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
	printf("00:         ");
	for (i=3; i< 0x78; i++) {
		i2c_cmd_handle_t cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
		i2c_master_stop(cmd);

		espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
		if (i%16 == 0) {
			printf("\n%.2x:", i);
		}
		if (espRc == 0) {
			printf(" %.2x", i);
		} else {
			printf(" --");
		}
		//ESP_LOGD(tag, "i=%d, rc=%d (0x%x)", i, espRc, espRc);
		i2c_cmd_link_delete(cmd);
	}
	printf("\n");
	vTaskDelete(NULL);
}