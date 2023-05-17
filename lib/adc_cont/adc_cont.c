#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_adc/adc_continuous.h"
#include "hal/adc_types.h"

#include "adc_cont.h"

static adc_continuous_handle_t adc1 = NULL;

bool adc_setup() {
    adc_continuous_handle_cfg_t adc_handle_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = 128
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_handle_config, &adc1));

    adc_continuous_config_t adc_config = {
        .sample_freq_hz = 20e3,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,    // use ADC1
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
        .pattern_num = 1
    };

    int channel_num, unit_num;
    ESP_ERROR_CHECK(adc_continuous_io_to_channel(34, &unit_num, &channel_num));

    adc_digi_pattern_config_t single_channel_config = { // create an array when going in multi-channel mode
        .atten = ADC_ATTEN_DB_0,
        .channel = channel_num,
        .unit = unit_num,
        .bit_width = ADC_BITWIDTH_12
    };
    adc_config.adc_pattern = &single_channel_config;
    ESP_ERROR_CHECK(adc_continuous_config(adc1, &adc_config));

    return true;
}

void adc_start() {
    // Start ADC conversion
    adc_continuous_start(adc1);
}

int adc_read(uint8_t *dst, uint32_t length_max, uint32_t *out_length) {
    int ret = adc_continuous_read(adc1, dst, length_max, out_length, 5000 / portTICK_PERIOD_MS);

    return ret;
}
