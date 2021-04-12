#ifndef __bme280_h
#define __bme280_h

#include "esp_err.h"

#define BMP280_ADDR 0x76

esp_err_t bmp280_read_register(uint8_t addr, uint8_t reg, uint8_t *dst, int n);
int bmp280_get_id();

#endif