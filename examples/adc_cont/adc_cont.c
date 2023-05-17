#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "hal/adc_types.h"

#include "adc.h"


void app_main() {
    adc_setup();
    adc_start();

    int ret;
    adc_digi_output_data_t result[128];
    int out_length;

    while(true) {
        ret = adc_read(result, sizeof(result), &out_length);

        if(ret == ESP_OK) {
            ESP_LOGI("ADC", "ADC returned %d bytes of result", out_length);
        } else {
            ESP_LOGW("ADC", "ADC error %d", ret);
        }
    }
}

