#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "i2c.h"
#include "bmp280.h"

void app_main() {
    i2c_configure();

    while(true) {

        printf("ID of BME280: %02x.\n", bmp280_get_id());

        vTaskDelay(100 / portTICK_RATE_MS);
    }
}
