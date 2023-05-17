#pragma once

#include <stdbool.h>
#include <stdint.h>

bool adc_setup();
void adc_start();
int adc_read(uint8_t *buf, uint32_t length_max, uint32_t *out_length);
