#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "i2c.h"
#include "my_bmp280.h"

void app_main() {
    i2c_configure();

    printf("ID of BMP280: %02x.\n", bmp280_get_id());

    vTaskDelay(100 / portTICK_RATE_MS);

    struct bmp280_compensation_params p;
    ESP_ERROR_CHECK(bmp280_get_compensation_params(&p));

    printf("Compensation parameters:\n"
            "dig_t1 %d\n"
            "dig_t2 %d\n"
            "dig_t3 %d\n"
            "dig_p1 %d\n"
            "dig_p2 %d\n"
            "dig_p3 %d\n"
            "dig_p4 %d\n"
            "dig_p5 %d\n"
            "dig_p6 %d\n"
            "dig_p7 %d\n"
            "dig_p8 %d\n"
            "dig_p9 %d\n",
            p.dig_T1, p.dig_T2, p.dig_T3, p.dig_P1, p.dig_P2, p.dig_P3, p.dig_P4, p.dig_P5, p.dig_P6, p.dig_P7, p.dig_P8, p.dig_P9
    );


    //Wait indefinitely without trigger watchdog
    SemaphoreHandle_t s = xSemaphoreCreateBinary();
    while(true) {
        xSemaphoreTake(s, 1000 / portTICK_RATE_MS);
    }
}
