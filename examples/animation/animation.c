/*
 *  GIF example
 */

#include <stdio.h>

#include "driver/gpio.h"

#include "driver/i2c.h"
#include "ssd1306_hal/io.h"
#include "ssd1306.h"

#include "rick.h"

#define FRAMERATE 27

void app_main() {
    ssd1306_platform_i2cConfig_t cfg = {
        .sda = 21,
        .scl = 22
    };
    ssd1306_platform_i2cInit(I2C_NUM_0, 0, &cfg);

    ssd1306_128x64_i2c_init();

    ssd1306_clearScreen();

    int i = 0;
    while(true) {

        ssd1306_drawBitmap(0, 0, 128, 64, rick + 1024 * i);
        i++;
        i %= FRAMERATE;

        vTaskDelay(37 / portTICK_RATE_MS);
    }

}