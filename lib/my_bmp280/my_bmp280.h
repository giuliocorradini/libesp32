#ifndef __bme280_h
#define __bme280_h

#include "esp_err.h"

#define BMP280_ADDR 0x76

struct __attribute__((__packed__)) bmp280_compensation_params {
    unsigned short dig_T1;
    signed short dig_T2;
    signed short dig_T3;
    unsigned short dig_P1;
    signed short dig_P2;
    signed short dig_P3;
    signed short dig_P4;
    signed short dig_P5;
    signed short dig_P6;
    signed short dig_P7;
    signed short dig_P8;
    signed short dig_P9;
};

esp_err_t bmp280_read_register(uint8_t reg, uint8_t *dst, int n);
int bmp280_get_id();
esp_err_t bmp280_get_compensation_params(struct bmp280_compensation_params *p);

#endif